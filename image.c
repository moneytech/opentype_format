/*******************************
 * 画像 (8bit)
 *******************************/

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "image.h"
#include "ftlib.h"

//-------------

#define _MAKE_COL(r,g,b)   (((r) << 16) | ((g) << 8) | (b))

#define _GET_R(c)  ((uint8_t)((c) >> 16))
#define _GET_G(c)  ((uint8_t)((c) >> 8))
#define _GET_B(c)  ((uint8_t)(c))

//-------------



/* LE 2byte 出力 */

static void _write16(FILE *fp,uint16_t v)
{
	uint8_t b[2];

	b[0] = (uint8_t)v;
	b[1] = (uint8_t)(v >> 8);

	fwrite(b, 1, 2, fp);
}

/* LE 4byte 出力 */

static void _write32(FILE *fp,uint32_t v)
{
	uint8_t b[4];

	b[0] = (uint8_t)v;
	b[1] = (uint8_t)(v >> 8);
	b[2] = (uint8_t)(v >> 16);
	b[3] = (uint8_t)(v >> 24);

	fwrite(b, 1, 4, fp);
}


//==============================


/* 解放 */

void Image_free(Image *p)
{
	if(p)
	{
		if(p->buf) free(p->buf);

		free(p);
	}
}

/* 作成 */

Image *Image_new(int width,int height)
{
	Image *p;

	p = (Image *)calloc(1, sizeof(Image));
	if(!p) return NULL;

	p->width = width;
	p->height = height;

	p->buf = (uint32_t *)malloc(width * height * 4);
	if(!p->buf)
	{
		free(p);
		return NULL;
	}

	return p;
}

/* クリア */

void Image_clear(Image *p,uint32_t c)
{
	uint32_t *pd = p->buf;
	int i;

	for(i = p->width * p->height; i > 0; i--)
		*(pd++) = c;
}

/* 水平線を描画 */

void Image_drawHLine(Image *p,int x,int y,int w,uint32_t c)
{
	for(; w > 0; w--)
		Image_setPixel(p, x++, y, c);
}

/* 垂直線を描画 */

void Image_drawVLine(Image *p,int x,int y,int h,uint32_t c)
{
	for(; h > 0; h--)
		Image_setPixel(p, x, y++, c);
}

/* 四角形塗りつぶし */

void Image_fillBox(Image *p,int x,int y,int w,int h,uint32_t c)
{
	int x2,y2,ix,iy,next;
	uint32_t *pd;

	x2 = x + w - 1;
	y2 = y + h - 1;

	if(x2 < 0 || y2 < 0 || x >= p->width || y >= p->height)
		return;

	if(x < 0) x = 0;
	if(y < 0) y = 0;
	if(x2 >= p->width) x2 = p->width - 1;
	if(y2 >= p->height) y2 = p->height - 1;

	//

	pd = p->buf + y * p->width + x;
	next = p->width - (x2 - x + 1);

	for(iy = y; iy <= y2; iy++)
	{
		for(ix = x; ix <= x2; ix++)
			*(pd++) = c;

		pd += next;
	}
}

/* 点を打つ */

void Image_setPixel(Image *p,int x,int y,uint32_t c)
{
	if(x >= 0 && y >= 0 && x < p->width && y < p->height)
		*(p->buf + y * p->width + x) = c;
}

/* 色を合成 */

void Image_blendPixel(Image *p,int x,int y,uint32_t c,int alpha)
{
	uint32_t *pd,dstc;
	uint8_t dr,dg,db,sr,sg,sb;

	if(x >= 0 && y >= 0 && x < p->width && y < p->height)
	{
		pd = p->buf + y * p->width + x;
		dstc = *pd;

		sr = _GET_R(c);
		sg = _GET_G(c);
		sb = _GET_B(c);

		dr = _GET_R(dstc);
		dg = _GET_G(dstc);
		db = _GET_B(dstc);

		//アルファ合成

		dr = (sr - dr) * alpha / 255 + dr;
		dg = (sg - dg) * alpha / 255 + dg;
		db = (sb - db) * alpha / 255 + db;

		*pd = _MAKE_COL(dr, dg, db);
	}
}

/* BMP 画像で出力 */

void Image_writeBitmap(Image *p,const char *filename)
{
	FILE *fp;
	uint32_t *ps,c;
	uint8_t *buf,*pd;
	int ix,iy,imgsize,pitch;

	fp = fopen(filename, "wb");
	if(!fp) return;

	pitch = (p->width * 3 + 3) & (~3);
	imgsize = pitch * p->height;

	//1行用バッファ

	buf = (uint8_t *)malloc(pitch);
	if(!buf)
	{
		fclose(fp);
		return;
	}

	//ファイルヘッダ

	fputs("BM", fp);
	_write32(fp, 14 + 40 + imgsize);
	_write32(fp, 0);
	_write32(fp, 14 + 40);

	//情報ヘッダ

	_write32(fp, 40);
	_write32(fp, p->width);
	_write32(fp, p->height);
	_write16(fp, 1);
	_write16(fp, 24);
	_write32(fp, 0);
	_write32(fp, imgsize);
	_write32(fp, 3780);
	_write32(fp, 3780);
	_write32(fp, 0);
	_write32(fp, 0);

	//イメージ (ボトムアップ)

	ps = p->buf + (p->height - 1) * p->width;

	for(iy = p->height; iy > 0; iy--)
	{
		pd = buf;
		
		for(ix = p->width; ix > 0; ix--, pd += 3)
		{
			c = *(ps++);

			pd[0] = _GET_B(c);
			pd[1] = _GET_G(c);
			pd[2] = _GET_R(c);
		}
		
		fwrite(buf, 1, pitch, fp);

		ps -= p->width * 2;
	}

	//

	fclose(fp);

	free(buf);
}


//===========================
// DrawGlyph
//===========================


/* BMP ファイルに出力 */

static void _write_bmp(DrawGlyph *p)
{
	char m[32];

	snprintf(m, 32, "page%02d.bmp", p->curpage);

	Image_writeBitmap(p->img, m);
}

/* 初期化
 *
 * [!] ftlib の初期化後に実行すること */

int DrawGlyph_init(DrawGlyph *p,int width,int height)
{
	p->img = Image_new(width, height);
	if(!p->img) return 1;

	Image_clear(p->img, 0xffffff);

	p->font_height = ftlib_getHeight();
	p->padding = 10;
	p->curpage = 1;
	p->curx = p->cur_colx = p->padding;
	p->cury = p->padding;
	p->x_max = 0;
	p->col_width = 50;

	return 0;
}

/* 画像終了処理 */

void DrawGlyph_end(DrawGlyph *p)
{
	if(!p->img) return;

	//新規状態でなければ、現在の画像出力

	if(!(p->x_max == 0 && p->curx == p->padding))
	{
		_write_bmp(p);
		p->curpage++;
	}

	//出力した画像の情報

	if(p->curpage == 1)
		printf("[!] no output image\n");
	else if(p->curpage == 2)
		printf("\n=> page01.bmp\n");
	else
		printf("\n=> page01-%02d.bmp\n", p->curpage - 1);

	//解放

	Image_free(p->img);
}

/* 次の行へ */

void DrawGlyph_nextLine(DrawGlyph *p)
{
	int padding = p->padding;

	//次の Y 位置へ

	p->curx = p->cur_colx;
	p->cury += p->font_height + 6;

	if(p->cury + p->font_height + padding > p->img->height)
	{
		//次の横列へ
		
		Image_drawVLine(p->img,
			p->x_max, padding,
			p->img->height - padding * 2, 0);
	
		p->curx = p->cur_colx = p->x_max + 5;
		p->cury = padding;
		p->x_max = 0;

		//次のページへ

		if(p->curx + p->col_width + padding > p->img->width)
		{
			_write_bmp(p);
		
			p->curpage++;
			p->curx = p->cur_colx = padding;

			Image_clear(p->img, 0xffffff);
		}
	}
}

/* 現在の行にグリフ描画 (横に位置を進める) */

void DrawGlyph_draw(DrawGlyph *p,uint16_t gid,uint32_t col)
{
	int x;

	x = ftlib_drawChar_GID(p->img, p->curx, p->cury, gid, col);
	x += 4;

	if(x > p->x_max) p->x_max = x;

	p->curx = x;
}

/* 1行に1ペアのグリフ描画 */

void DrawGlyph_drawPair(DrawGlyph *p,
	uint16_t gid1,uint16_t gid2,uint32_t col1,uint32_t col2)
{
	DrawGlyph_draw(p, gid1, col1);
	DrawGlyph_draw(p, gid2, col2);

	DrawGlyph_nextLine(p);
}

/* 現在の行に区切りの横線を描画 */

void DrawGlyph_drawRowLine(DrawGlyph *p)
{
	int w;

	if(p->x_max == 0)
		w = 20;
	else
		w = p->x_max - p->cur_colx - 1;

	Image_drawHLine(p->img, p->cur_colx, p->cury - 3, w, 0x009900);
}
