/*******************************
 * GPOS テーブルの LookupList の
 * LookupType = 4,5,6 のデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"


/* Anchor */

void put_anchor(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t format;

	font_fprintf(p, fp, "{Anchor} format:$+H | x:$h | y:$h",
		&format);

	switch(format)
	{
		case 2:
			font_fprintf(p, fp, " | anchorPoint:$H\n");
			break;
		case 3:
			font_fprintf(p, fp, " | xDeviceOffset:$H | yDeviceOffset:$H\n");
			break;
		default:
			fputc('\n', fp);
			break;
	}
}

/* Coverage テーブル */

void put_coverage(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t format,cnt,gid,st,ed,stind;
	int i;

	format = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "\n## Coverage (format:%d) ##\n\n", format);

	//

	if(format == 1)
	{
		//グリフIDの配列
		
		for(i = 0; i < cnt; i++)
		{
			gid = font_read16(p);

			font_output_gid_to_uni(p, gid);
			fprintf(fp, " | %d\n", i);
		}
	}
	else if(format == 2)
	{
		//範囲

		for(; cnt > 0; cnt--)
		{
			st = font_read16(p);
			ed = font_read16(p);
			stind = font_read16(p);

			for(i = st; i <= ed; i++)
			{
				font_output_gid_to_uni(p, i);
				fprintf(fp, " | %d\n", i - st + stind);
			}
		}
	}
}

/* MarkArray */

void put_mark_array(Font *p,uint32_t offset_markarray)
{
	FILE *fp = p->fp_output;
	uint16_t cnt,class,offset;
	int i;

	font_seek_from_table(p, offset_markarray);

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		class = font_read16(p);
		offset = font_read16(p);
	
		fprintf(fp, "[%d] class:%d / ", i, class);

		//Anchor

		font_save_pos(p);
		font_seek_from_table(p, offset_markarray + offset);

		put_anchor(p);

		font_load_pos(p);
	}
}

/* サブテーブル */

void put_subtable(Font *p,int lookup_type,uint32_t offset_subst)
{
	FILE *fp = p->fp_output;
	uint32_t offset_top,offset_ligatt;
	uint16_t offset,cnt,cnt2,markclass_cnt;
	int i,j,k;

	font_fprintf(p, fp, "\n*** SubTable (format:$H) ***\n");

	//Coverage (mark)

	offset = font_read16(p);
	font_seek_from_table(p, offset_subst + offset);

	put_coverage(p);

	//Coverage (base)

	font_seek_from_table(p, offset_subst + 4);

	offset = font_read16(p);

	font_seek_from_table(p, offset_subst + offset);

	put_coverage(p);

	//markClassCount

	font_seek_from_table(p, offset_subst + 6);

	font_fprintf(p, fp, "\nmarkClassCount: $+H\n", &markclass_cnt);

	//MarkArray

	fprintf(fp, "\n## MarkArray ##\n\n");

	offset = font_read16(p);

	put_mark_array(p, offset_subst + offset);

	//

	font_seek_from_table(p, offset_subst + 10);

	offset = font_read16(p);
	offset_top = offset_subst + offset;

	font_seek_from_table(p, offset_top);

	if(lookup_type == 5)
	{
		fprintf(fp, "\n## LigatureArray ##\n\n");

		//LigatureArray

		cnt = font_read16(p);
		
		for(i = 0; i < cnt; i++)
		{
			font_seek_from_table(p, offset_top + 2 + i * 2);
		
			offset = font_read16(p);
			offset_ligatt = offset_top + offset;

			//LigatureAttach

			font_seek_from_table(p, offset_ligatt);
		
			cnt2 = font_read16(p);
			
			for(j = 0; j < cnt2; j++)
			{
				for(k = 0; k < markclass_cnt; k++)
				{
					offset = font_read16(p);

					fprintf(fp, "[%d] comp<%d> mark<%d> / ", i, j, k);

					font_save_pos(p);
					font_seek_from_table(p, offset_ligatt + offset);

					put_anchor(p);

					font_load_pos(p);
				}
			}
		}
	}
	else
	{
		if(lookup_type == 4)
			fprintf(fp, "\n## BaseArray ##\n\n");
		else
			fprintf(fp, "\n## Mark2Array ##\n\n");
	
		cnt = font_read16(p);

		for(i = 0; i < cnt; i++)
		{
			for(j = 0; j < markclass_cnt; j++)
			{
				offset = font_read16(p);

				fprintf(fp, "[%d] mark<%d> / ", i, j);

				font_save_pos(p);
				font_seek_from_table(p, offset_top + offset);

				put_anchor(p);

				font_load_pos(p);
			}
		}
	}
}

/* Lookup テーブル */

void put_lookup(Font *p,int lookup_index,uint32_t offset_lookup)
{
	FILE *fp = p->fp_output;
	uint16_t type,type2,flags,cnt,format;
	uint32_t offset_subtable,offset32;

	//LookupType = 4,5,6,9 のみ処理

	type = font_read16(p);

	if((type < 4 || type > 6) && type != 9) return;

	//

	flags = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "\n==== Lookup [%d] ====\n\n", lookup_index);

	fprintf(fp,
		"{Lookup} lookupType <%d> | lookupFlag:0x%04X | subTableCount:%d\n",
		type, flags, cnt);

	//

	for(; cnt > 0; cnt--)
	{
		offset_subtable = offset_lookup + font_read16(p);

		font_save_pos(p);
		font_seek_from_table(p, offset_subtable);

		//LookupType = 9 の場合

		if(type == 9)
		{
			format = font_read16(p);
			type2 = font_read16(p);
			offset32 = font_read32(p);

			//LookupType = 4,5,6 でなければスキップ

			if(type2 < 4 || type2 > 6)
			{
				font_load_pos(p);
				continue;
			}

			//実体のサブテーブルへ

			offset_subtable += offset32;

			font_seek_from_table(p, offset_subtable);

			fprintf(fp, "==> lookupType <%d>\n", type2);
		}
		else
			type2 = type;

		//subtable

		put_subtable(p, type2, offset_subtable);

		//

		font_load_pos(p);
	}
}

/* LookupList テーブル */

void put_lookup_list(Font *p)
{
	FILE *fp = p->fp_output;
	uint32_t offset_lookuplist;
	uint16_t offset;
	int i,cnt;

	offset_lookuplist = p->offset_dat[2];

	font_seek_from_table(p, offset_lookuplist);

	fprintf(fp, "---- LookupList ----\n");

	//LookupList

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		offset = font_read16(p);

		//Lookup

		font_save_pos(p);
		font_seek_from_table(p, offset_lookuplist + offset);

		put_lookup(p, i, offset_lookuplist + offset);
		
		font_load_pos(p);
	}
}

/* main */

int main(int argc,char **argv)
{
	Font *p;

	if(argc < 2)
	{
		printf("[usage] %s <fontfile>\n", argv[0]);
		return 0;
	}

	p = font_open_file(argv[1]);
	if(!p) return 1;

	//GID 変換テーブル生成

	font_make_gindex_table(p);

	//GPOS

	if(font_goto_GPOS(p) == 0)
	{
		if(font_open_output_file(p, "output.txt") == 0)
		{
			put_lookup_list(p);

			printf("=> output.txt\n");
		}
	}

	font_close(p);
	
	return 0;
}
