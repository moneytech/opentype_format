/*******************************
 * GSUB テーブルの LookupList データの
 * LookupType = 1 のグリフを、
 * FreeType で描画して BMP 画像で出力。
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"
#include "image.h"
#include "ftlib.h"

//-----------

#define COL_GLYPH_SRC 0
#define COL_GLYPH_DST 0x0000ff

DrawGlyph g_img;

//-----------


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

void read_coverage(Font *p,int16_t delta,uint32_t offset_format2)
{
	uint16_t format,cnt,gid,gid_dst,st,ed,stind;
	int i;

	format = font_read16(p);
	cnt = font_read16(p);

	printf("{Coverage} format <%d>\n", format);

	if(format == 1)
	{
		//グリフIDの配列
		
		for(i = 0; i < cnt; i++)
		{
			gid = font_read16(p);
			gid_dst = convert_glyph(p, gid, i, delta, offset_format2);
			
			DrawGlyph_drawPair(&g_img, gid, gid_dst, COL_GLYPH_SRC, COL_GLYPH_DST);
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
				
				DrawGlyph_drawPair(&g_img, i, gid_dst, COL_GLYPH_SRC, COL_GLYPH_DST);
			}
		}
	}
}

/* Lookup テーブル */

void read_lookup(Font *p,int lookuplist_index,uint32_t offset_lookup)
{
	uint16_t type,type2,flags,cnt,offset,format;
	int16_t delta;
	uint32_t offset_subtable,offset_format2,offset32;

	//LookupType = 1,7 のみ処理

	type = font_read16(p);

	if(type != 1 && type != 7) return;

	//

	flags = font_read16(p);
	cnt = font_read16(p);

	printf("\n==== [%d] ====\n\n", lookuplist_index);
	printf("{Lookup} lookupType <%d> | lookupFlag:0x%04X | subTableCount:%d\n\n",
		type, flags, cnt);

	//

	for(; cnt > 0; cnt--)
	{
		offset_subtable = offset_lookup + font_read16(p);

		//subtable

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

			printf("==> lookupType <%d>\n", type2);
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

		printf("# {SubTable} format <%d> / ", format);

		//Coverage

		font_seek_from_table(p, offset_subtable + offset);

		read_coverage(p, delta, offset_format2);

		//

		font_load_pos(p);

		DrawGlyph_drawRowLine(&g_img);
	}
}

/* LookupList */

void read_lookup_list(Font *p)
{
	uint32_t offset_lookuplist,offset_lookup;
	int i,cnt;

	offset_lookuplist = p->offset_dat[2];

	font_seek_from_table(p, offset_lookuplist);

	printf("---- LookupList ----\n\n");

	//LookupList

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		offset_lookup = offset_lookuplist + font_read16(p);

		//Lookup

		font_save_pos(p);
		font_seek_from_table(p, offset_lookup);

		read_lookup(p, i, offset_lookup);

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

	//FreeType

	if(ftlib_init(argv[1], 18)) return 1;

	//画像

	if(DrawGlyph_init(&g_img, 700, 700))
		goto END;

	g_img.col_width = 50;

	//

	p = font_open_file(argv[1]);
	if(!p) goto END;

	if(font_goto_GSUB(p) == 0)
		read_lookup_list(p);

	font_close(p);

	//
END:
	DrawGlyph_end(&g_img);

	ftlib_free();
	
	return 0;
}
