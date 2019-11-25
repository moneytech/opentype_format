/**************************************************
 * loca/glyf テーブルから、
 * 指定グリフのアウトラインデータを表示
 *
 * ※ Unicode 値は、BMP (U+0000〜U+FFFF) の範囲のみ
 *
 * [usage] exe <fontfile> <char>
 *
 * 例: ./10_glyf font.ttf "A"
 **************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <locale.h>

#include "fontfile.h"

typedef struct
{
	int ct;	//輪郭番号
	int16_t x,y;
	uint8_t flags,
		repeat_num;
}PointData;


/* 単純なグリフ */

void read_glyf_simple(Font *p,int noc)
{
	PointData *ptbuf,*pt;
	int i,j,ptnum,n,x,y;
	uint8_t f;
	uint16_t instlen;

	//各輪郭の最後の座標のインデックス値

	font_save_pos(p);

	printf("endPtsOfContours: ");
	for(i = 0; i < noc; i++)
	{
		n = font_read16(p);
		printf("%d,", n);
	}
	printf("\n");

	//最後の値 + 1 = ポイント数

	ptnum = n + 1;

	//座標データ確保

	ptbuf = (PointData *)malloc(sizeof(PointData) * ptnum);
	if(!ptbuf) return;

	//輪郭番号セット

	font_load_pos(p);

	pt = ptbuf;

	for(i = 0, j = 0; i < noc; i++)
	{
		n = font_read16(p);

		for(; j <= n; j++, pt++)
			pt->ct = i;
	}

	//命令

	font_printf(p, "instructionLength: $+H\n", &instlen);

	if(instlen)
	{
		printf("instructions:\n");
		for(i = 0; i < instlen; i++)
			printf("%d,", font_read8(p));
		printf("\n");
	}

	//flags (各座標ごと)

	pt = ptbuf;
	
	for(i = 0; i < ptnum; i++)
	{
		f = font_read8(p);

		pt->flags = f;

		//REPEAT_FLAG の場合、同じフラグ値が次のバイトの値分繰り返される

		if(!(f & 8))
			pt++;
		else
		{
			n = font_read8(p);
			
			pt->repeat_num = n;
			pt++;

			for(j = 0; j < n; j++, pt++)
				pt->flags = f & (~8);

			i += n;
		}
	}

	//X 座標

	pt = ptbuf;

	for(i = 0; i < ptnum; i++, pt++)
	{
		if(pt->flags & 2)
		{
			//uint8
		
			pt->x = font_read8(p);

			//4bit:OFF なら、負の値

			if(!(pt->flags & 16))
				pt->x = -(pt->x);
		}
		else
		{
			//int16
			
			if(pt->flags & 16)
				//前の座標と同じ
				pt->x = 0;
			else
				pt->x = (int16_t)font_read16(p);
		}
	}

	//Y 座標

	pt = ptbuf;

	for(i = 0; i < ptnum; i++, pt++)
	{
		if(pt->flags & 4)
		{
			//uint8
		
			pt->y = font_read8(p);

			//4bit:OFF なら、負の値

			if(!(pt->flags & 32))
				pt->y = -(pt->y);
		}
		else
		{
			//int16
			
			if(pt->flags & 32)
				//前の座標と同じ
				pt->y = 0;
			else
				pt->y = (int16_t)font_read16(p);
		}
	}

	//表示

	printf("\npoints:\n");
	n = -1;

	for(i = 0, pt = ptbuf; i < ptnum; i++, pt++)
	{
		//輪郭の切り替わり
		
		if(pt->ct != n)
		{
			printf("== contour [%d] ==\n\n", pt->ct);
			n = pt->ct;

		}

		//座標
		//(最初の点は、原点からの位置。
		// それ以降は前の点からの相対位置)

		if(i == 0)
		{
			x = pt->x;
			y = pt->y;
		}
		else
		{
			x += pt->x;
			y += pt->y;
		}

		printf("(%d, %d) : raw (%d, %d)\n", x, y, pt->x, pt->y);

		//フラグ

		f = pt->flags;
		
		printf("{");
		
		if(f & 1)
			printf("ON_CURVE_POINT, ");

		if(f & 2)
			printf("X_SHORT_VECTOR, ");
		
		if(f & 4)
			printf("Y_SHORT_VECTOR, ");

		if(f & 8)
			printf("REPEAT_FLAG[%d], ", pt->repeat_num);

		if(f & 16)
			printf("X_IS_SAME_OR_POSITIVE_X_SHORT_VECTOR, ");

		if(f & 32)
			printf("Y_IS_SAME_OR_POSITIVE_Y_SHORT_VECTOR, ");

		if(f & 64)
			printf("OVERLAP_SIMPLE, ");

		printf("}\n\n");
	}

	//

	free(ptbuf);
}

/* glyf テーブル読み込み */

void read_glyf_table(Font *p)
{
	int16_t noc;

	printf("\n---- glyf ----\n\n");

	font_printf(p, "numberOfContours: $+h\n", &noc);
	font_printf(p, "xMin,yMin,xMax,yMax: $h, $h, $h, $h\n");

	if(noc >= 0)
		read_glyf_simple(p, noc);

	//noc < 0 の場合、複合グリフ
}

/* loca テーブル読み込み
 *
 * return: glyf テーブルのオフセット値 (-1 でなし) */

uint32_t read_loca_table(Font *p,int gid)
{
	uint32_t offset,offset2;

	if(p->loca_format == 0)
	{
		//Offset16

		font_seek_from_table(p, gid * 2);

		offset = font_read16(p) * 2;
		offset2 = font_read16(p) * 2;
	}
	else
	{
		//Offset32

		font_seek_from_table(p, gid * 4);

		offset = font_read32(p);
		offset2 = font_read32(p);
	}

	printf("---- loca ----\n\n");
	printf("format: %s\n", (p->loca_format)? "Offset32": "Offset16");
	printf("offset: %d (next: %d)\n", offset, offset2);

	if(offset == offset2)
	{
		//次の位置のオフセット値と同じ値なら、アウトラインなし
		printf("no outline\n");
		return (uint32_t)-1;
	}
	else
		return offset;
}

/* 1文字 -> Unicode -> glyphID 変換 */

int get_glyphID(Font *p,char *str)
{
	wchar_t wc;
	int gid;

	//マルチバイト文字列 -> ワイド文字

	if(mbtowc(&wc, str, 4) == -1)
	{
		printf("error mbtowc()\n");
		return -1;
	}

	//Unicode 範囲

	if(wc > 0xffff)
	{
		printf("Unicode is U+0000..U+FFFF\n");
		return -1;
	}

	//Unicode -> gid

	gid = font_cmap_get_glyph_id(p, wc);

	if(gid == 0)
	{
		printf("no glyph U+%04X\n", wc);
		return -1;
	}

	printf("U+%04X\nglyphID: %d\n\n", wc, gid);

	return gid;
}

/* main */

int main(int argc,char **argv)
{
	Font *p;
	int gid;
	uint32_t offset;

	if(argc < 3)
	{
		printf("[usage] %s <fontfile> <char>\n", argv[0]);
		return 0;
	}

	setlocale(LC_ALL, "");

	p = font_open_file(argv[1]);
	if(!p) return 1;

	//GID 取得

	gid = get_glyphID(p, argv[2]);
	if(gid == -1) goto END;

	//head テーブルからデータ取得

	font_read_table_head(p);

	//loca テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('l','o','c','a')))
	{
		printf("not found 'loca'\n");
		goto END;
	}
	else
	{
		offset = read_loca_table(p, gid);
		if(offset == (uint32_t)-1) goto END;
	}

	//glyf テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('g','l','y','f')) == 0)
	{
		font_seek_from_table(p, offset);
		read_glyf_table(p);
	}

END:
	font_close(p);
	
	return 0;
}
