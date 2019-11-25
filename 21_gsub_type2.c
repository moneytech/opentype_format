/*******************************
 * GSUB テーブルの LookupList の
 * LookupType = 2〜4 のデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"
#include "ftlib.h"
#include "image.h"


DrawGlyph g_img;


/* 元グリフIDから、グリフ置き換え情報表示
 *
 * gid_src : 元グリフID
 * index : Coverage インデックス
 * offset_subst : サブテーブルの位置 */

void put_glyph(Font *p,int lookup_type,uint16_t gid_src,int index,uint32_t offset_subst)
{
	FILE *fp = p->fp_output;
	uint16_t cnt,cnt2,offset,gid,gid2;
	uint32_t offset_ligature_set;

	font_save_pos(p);

	//オフセット値を読み込み、目的のデータへ飛ぶ

	font_seek_from_table(p, offset_subst + 6 + index * 2);

	offset = font_read16(p);

	font_seek_from_table(p, offset_subst + offset);

	//

	if(lookup_type < 4)
	{
		//LookupType = 2 or 3

		fputs("* ", fp);
		font_output_gid_to_uni(p, gid_src);
		fputc('\n', fp);

		DrawGlyph_draw(&g_img, gid_src, 0xff0000);
		
		cnt = font_read16(p);

		for(; cnt > 0; cnt--)
		{
			gid = font_read16(p);

			fputs("  ", fp);
			font_output_gid_to_uni(p, gid);
			fputc('\n', fp);

			DrawGlyph_draw(&g_img, gid, 0);
		}

		DrawGlyph_nextLine(&g_img);
	}
	else
	{
		//---- LookupType = 4

		offset_ligature_set = offset_subst + offset;

		//ligatureCount
		cnt = font_read16(p);

		for(; cnt > 0; cnt--)
		{
			offset = font_read16(p);

			//Ligature

			font_save_pos(p);
			font_seek_from_table(p, offset_ligature_set + offset);

			gid = font_read16(p);
			cnt2 = font_read16(p);

			font_output_gid_to_uni(p, gid_src); //合体前の最初の文字
			fputc('\n', fp);

			DrawGlyph_draw(&g_img, gid, 0xff0000);
			DrawGlyph_draw(&g_img, gid_src, 0);

			for(; cnt2 > 1; cnt2--)
			{
				//合体前の2番目以降の文字
				//cnt2 は最初の文字も含む数なので、1個引く

				gid2 = font_read16(p);
				
				fputs("+ ", fp);
				font_output_gid_to_uni(p, gid2);
				fputc('\n', fp);

				DrawGlyph_draw(&g_img, gid2, 0);
			}

			fputs("= ", fp);
			font_output_gid_to_uni(p, gid); //合体後の文字
			fputs("\n\n", fp);

			DrawGlyph_nextLine(&g_img);
			
			font_load_pos(p);
		}
	}

	font_load_pos(p);
}

/* Coverage テーブル */

void put_coverage(Font *p,int lookup_type,uint32_t offset_subst)
{
	uint16_t format,cnt,gid,st,ed,stind;
	int i;

	format = font_read16(p);
	cnt = font_read16(p);

	fprintf(p->fp_output, " / {Coverage} format <%d>\n\n", format);

	if(format == 1)
	{
		//グリフIDの配列
		
		for(i = 0; i < cnt; i++)
		{
			gid = font_read16(p);

			put_glyph(p, lookup_type, gid, i, offset_subst);
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
				put_glyph(p, lookup_type, i, i - st + stind, offset_subst);
		}
	}
}

/* Lookup テーブル */

void put_lookup(Font *p,int lookuplist_index,uint32_t offset_lookup)
{
	FILE *fp = p->fp_output;
	uint16_t type,type2,flags,cnt,offset,format;
	uint32_t offset_subst,offset32;

	//LookupType = 2〜4、7 のみ処理

	type = font_read16(p);

	if((type < 2 || type > 4) && type != 7) return;

	//

	flags = font_read16(p);
	cnt = font_read16(p);

	fprintf(fp, "\n==== [%d] ====\n\n", lookuplist_index);

	fprintf(fp,
		"{Lookup} lookupType <%d> | lookupFlag:0x%04X | subTableCount:%d\n\n",
		type, flags, cnt);

	//

	for(; cnt > 0; cnt--)
	{
		offset_subst = offset_lookup + font_read16(p);

		font_save_pos(p);
		font_seek_from_table(p, offset_subst);

		//LookupType = 7 の場合

		if(type == 7)
		{
			format = font_read16(p);
			type2 = font_read16(p);
			offset32 = font_read32(p);

			//LookupType = 2-4 でなければスキップ

			if(type2 < 2 || type2 > 4)
			{
				font_load_pos(p);
				continue;
			}

			//実体のサブテーブルへ

			offset_subst += offset32;

			font_seek_from_table(p, offset_subst);

			fprintf(fp, "==> lookupType <%d>\n", type2);
		}
		else
			type2 = type;

		//subtable

		format = font_read16(p);
		offset = font_read16(p);

		fprintf(fp, "# {SubTable} format <%d>", format);

		//

		if(format != 1)
			//LookupType 2-4 は、substFormat 1 のみ
			fputc('\n', fp);
		else
		{
			//Coverage

			font_seek_from_table(p, offset_subst + offset);

			put_coverage(p, type2, offset_subst);
		}

		font_load_pos(p);

		DrawGlyph_drawRowLine(&g_img);
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

	//FreeType

	if(ftlib_init(argv[1], 18)) return 1;

	//画像

	if(DrawGlyph_init(&g_img, 800, 700))
		goto END;

	g_img.col_width = 22 * 7;

	//-----

	p = font_open_file(argv[1]);
	if(!p) goto END;

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

	//-----

END:
	DrawGlyph_end(&g_img);

	ftlib_free();

	return 0;
}
