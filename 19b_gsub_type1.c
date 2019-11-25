/*******************************
 * GSUB テーブルの LookupList の
 * LookupType = 1 のデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"


/* グリフID変換
 *
 * gid : 元グリフID
 * index : Coverage インデックス
 * delta : (sustFormat = 1) 増減値
 * offset_format2 : (substFormat = 2) 配列のオフセット位置 */

uint16_t convert_glyph(Font *p,uint16_t gid,int index,int16_t delta,uint32_t offset_format2)
{
	uint16_t dst;

	if(offset_format2 == 0)
		//増減
		dst = (gid + delta) & 0xffff;
	else
	{
		//配列

		font_save_pos(p);
		font_seek_from_table(p, offset_format2 + index * 2);

		dst = font_read16(p);
		
		font_load_pos(p);
	}

	return dst;
}

/* Coverage テーブル
 *
 * delta : 変換後の増減値 (offset_format2 = 0 時)
 * offset_format2 : 変換後のグリフ ID 配列のオフセット位置 (0 でない時) */

void put_coverage(Font *p,int16_t delta,uint32_t offset_format2)
{
	uint16_t format,cnt,gid,gid_dst,st,ed,stind;
	int i;

	format = font_read16(p);
	cnt = font_read16(p);

	fprintf(p->fp_output, "{Coverage} format <%d>\n\n", format);

	if(format == 1)
	{
		//グリフIDの配列
		
		for(i = 0; i < cnt; i++)
		{
			gid = font_read16(p);
			gid_dst = convert_glyph(p, gid, i, delta, offset_format2);
			
			font_output_gid_rep(p, gid, gid_dst);
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

			//範囲内のグリフインデックスを処理

			for(i = st; i <= ed; i++)
			{
				gid_dst = convert_glyph(p, i, i - st + stind, delta, offset_format2);
				
				font_output_gid_rep(p, i, gid_dst);
			}
		}
	}
}

/* Lookup テーブル */

void put_lookup(Font *p,int lookup_index,uint32_t offset_lookup)
{
	FILE *fp = p->fp_output;
	uint16_t type,type2,flags,cnt,offset,format;
	int16_t delta;
	uint32_t offset_subtable,offset_format2,offset32;

	//LookupType = 1,7 のみ処理

	type = font_read16(p);

	if(type != 1 && type != 7) return;

	//

	flags = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "\n==== [%d] ====\n\n", lookup_index);

	fprintf(fp,
		"{Lookup} lookupType <%d> | lookupFlag:0x%04X | subTableCount:%d\n\n",
		type, flags, cnt);

	//

	for(; cnt > 0; cnt--)
	{
		offset_subtable = offset_lookup + font_read16(p);

		font_save_pos(p);
		font_seek_from_table(p, offset_subtable);

		//LookupType = 7 の場合

		if(type == 7)
		{
			format = font_read16(p);
			type2 = font_read16(p);
			offset32 = font_read32(p);

			//LookupType = 1 でなければスキップ

			if(type2 != 1)
			{
				font_load_pos(p);
				continue;
			}

			//実体のサブテーブルへ

			offset_subtable += offset32;

			font_seek_from_table(p, offset_subtable);

			fprintf(fp, "==> lookupType <%d>\n", type2);
		}

		//subtable

		format = font_read16(p);
		offset = font_read16(p);

		if(format == 1)
		{
			//増減値
			delta = (int16_t)font_read16(p);
			offset_format2 = 0;
		}
		else if(format == 2)
		{
			//配列のオフセット位置
			delta = 0;
			offset_format2 = offset_subtable + 6;
		}

		fprintf(fp, "# {SubTable} format <%d> / ", format);

		//Coverage

		font_seek_from_table(p, offset_subtable + offset);

		put_coverage(p, delta, offset_format2);

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

	//GSUB

	if(font_goto_GSUB(p) == 0)
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
