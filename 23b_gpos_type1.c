/*******************************
 * GPOS テーブルの LookupList の
 * LookupType = 1,2,9 のデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"


/* LookupType = 1 (format 1,2) */

void put_subtable_type1(Font *p,int subst_format,int index,uint16_t gid)
{
	uint16_t vf;

	font_output_gid_to_uni(p, gid);
	fputc(' ', p->fp_output);

	vf = font_read16(p);

	if(subst_format == 1)
	{
		//全てのグリフに同じ値を適用
	
		font_output_ValueRecord(p, vf, 1);
	}
	else if(subst_format == 2)
	{
		//各グリフに ValueRecord

		font_seek_cur(p, 2 + get_ValueRecord_size(vf) * index);

		font_output_ValueRecord(p, vf, 1);
	}
}

/* LookupType = 2 (format 1) */

void put_subtable_type2_format1(Font *p,int index,uint16_t gid,uint32_t offset_subst)
{
	FILE *fp = p->fp_output;
	uint16_t vf1,vf2,cnt,offset;

	vf1 = font_read16(p);
	vf2 = font_read16(p);
	cnt = font_read16(p);

	font_seek_cur(p, 2 * index);
	offset = font_read16(p);

	//PairSet

	font_seek_from_table(p, offset_subst + offset);

	cnt = font_read16(p);

	font_output_gid_to_uni(p, gid);
	fputc('\n', fp);

	for(; cnt > 0; cnt--)
	{
		//PairValueRecord

		fputs(" + ", fp);
		font_output_gid_to_uni(p, font_read16(p));

		if(vf1)
		{
			fputs(" (1)", fp);
			font_output_ValueRecord(p, vf1, 0);
		}

		if(vf2)
		{
			fputs(" (2)", fp);
			font_output_ValueRecord(p, vf2, 0);
		}

		fputc('\n', fp);
	}
}

/* LookupType = 2 (format 2) */

void put_subtable_type2_format2(Font *p,uint32_t offset_subst)
{
	FILE *fp = p->fp_output;
	uint16_t vf1,vf2,offset_class1,offset_class2,class1cnt,class2cnt;
	int i,j;

	font_seek_from_table(p, offset_subst + 4);

	vf1 = font_read16(p);
	vf2 = font_read16(p);
	offset_class1 = font_read16(p);
	offset_class2 = font_read16(p);
	class1cnt = font_read16(p);
	class2cnt = font_read16(p);

	fputs("\n##\n", fp);
	fprintf(fp, "classDef1Offset: %d\n", offset_class1);
	fprintf(fp, "classDef2Offset: %d\n", offset_class2);
	fprintf(fp, "class1Count: %d\n", class1cnt);
	fprintf(fp, "class2Count: %d\n", class2cnt);

	//ClassDef 1

	font_save_pos(p);
	font_seek_from_table(p, offset_subst + offset_class1);

	fputs("\n## ClassDef 1 ##\n\n", fp);
	font_output_classDef(p);
	
	//ClassDef 2

	font_seek_from_table(p, offset_subst + offset_class2);

	fputs("\n## ClassDef 2 ##\n\n", fp);
	font_output_classDef(p);
	
	font_load_pos(p);

	//Class1Record

	fputs("\n##\n\n", fp);

	for(i = 0; i < class1cnt; i++)
	{
		for(j = 0; j < class2cnt; j++)
		{
			fprintf(fp, "class1[%d] + class2[%d] |", i, j);

			if(vf1)
			{
				fputs(" (1)", fp);
				font_output_ValueRecord(p, vf1, 0);
			}

			if(vf2)
			{
				fputs(" (2)", fp);
				font_output_ValueRecord(p, vf2, 0);
			}

			fputc('\n', fp);
		}

		fputc('\n', fp);
	}

	fprintf(fp, "------\n\n");
}

/* サブテーブルデータ (各 GID ごと) */

void put_subtable(Font *p,int lookup_type,int subst_format,int index,uint16_t gid,uint32_t offset_subst)
{
	font_save_pos(p);
	font_seek_from_table(p, offset_subst + 4);

	switch(lookup_type)
	{
		case 1:
			put_subtable_type1(p, subst_format, index, gid);
			break;
		case 2:
			if(subst_format == 1)
				put_subtable_type2_format1(p, index, gid, offset_subst);
			break;
	}

	font_load_pos(p);
}

/* Coverage テーブル */

void put_coverage(Font *p,int lookup_type,int subst_format,uint32_t offset_subst)
{
	FILE *fp = p->fp_output;
	uint16_t format,cnt,gid,st,ed,stind;
	int i,flag_only_gid;

	format = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "{Coverage} format <%d>\n\n", format);

	//各グリフのみ表示

	flag_only_gid = 0;

	if(lookup_type == 2 && subst_format == 2)
		flag_only_gid = 1;

	if(flag_only_gid)
		fputs("## Coverage ##\n\n", fp);

	//

	if(format == 1)
	{
		//グリフIDの配列
		
		for(i = 0; i < cnt; i++)
		{
			gid = font_read16(p);

			if(flag_only_gid)
			{
				font_output_gid_to_uni(p, gid);
				fputc('\n', fp);
			}
			else
				put_subtable(p, lookup_type, subst_format, i, gid, offset_subst);
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
				if(flag_only_gid)
				{
					font_output_gid_to_uni(p, i);
					fputc('\n', fp);
				}
				else
					put_subtable(p, lookup_type, subst_format, i - st + stind, i, offset_subst);
			}
		}
	}
}

/* Lookup テーブル */

void put_lookup(Font *p,int lookup_index,uint32_t offset_lookup)
{
	FILE *fp = p->fp_output;
	uint16_t type,type2,flags,cnt,offset,format;
	uint32_t offset_subtable,offset32;

	//LookupType = 1,2,3,9 のみ処理

	type = font_read16(p);

	if(type > 3 && type != 9) return;

	//

	flags = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "\n==== [%d] ====\n\n", lookup_index);

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

			//LookupType = 1,2,3 でなければスキップ

			if(type2 > 3)
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

		format = font_read16(p);
		offset = font_read16(p);

		fprintf(fp, "\n# {SubTable} format <%d>\n", format);

		//Coverage

		font_seek_from_table(p, offset_subtable + offset);

		put_coverage(p, type2, format, offset_subtable);

		//サブテーブル表示

		if(type2 == 2 && format == 2)
			put_subtable_type2_format2(p, offset_subtable);

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
