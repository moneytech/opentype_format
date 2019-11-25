/*****************************
 * FreeType ライブラリ処理
 *****************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_OUTLINE_H
#include FT_BITMAP_H
#include FT_LCD_FILTER_H

#include "ftlib.h"
#include "image.h"


//-----------------

typedef struct
{
	FT_Library lib;
	FT_Face face;

	int height,
		baseline;
}FTlib;

FTlib *g_ftlib = NULL;

//-----------------


//==========================
// sub
//==========================


/* FT_Face から文字高さなどの情報を取得 */

static void _get_info(FTlib *p,FT_Face face)
{
	FT_Size_Metrics *pm;

	pm = &face->size->metrics;

	p->height = pm->height >> 6;
	p->baseline = pm->ascender >> 6;
}

/* グリフインデックスから、ビットマップグリフ取得
 *
 * return : NULL で失敗 */

static FT_BitmapGlyph _get_bitmap_glyph(FTlib *p,int gid)
{
	FT_Face face = p->face;
	FT_Glyph glyph;
		
	//グリフスロットにロード
	
	if(FT_Load_Glyph(face, gid, FT_LOAD_NO_BITMAP | FT_LOAD_TARGET_NORMAL))
		return NULL;
	
	//グリフのコピー取得
	
	if(FT_Get_Glyph(face->glyph, &glyph))
		return NULL;
		
	//ビットマップに変換 (グレイスケール)
	
	if(FT_Glyph_To_Bitmap(&glyph, FT_RENDER_MODE_NORMAL, NULL, 1))
	{
		FT_Done_Glyph(glyph);
		return NULL;
	}
	
	return (FT_BitmapGlyph)glyph;
}

/* FT_BitmapGlyph の1文字を描画 */

static void _draw_char(FTlib *p,Image *img,int x,int y,uint32_t col,FT_BitmapGlyph glyph)
{
	FT_Bitmap *bm;
	uint8_t *buf;
	int ix,iy,xend,yend,w,h,pitch;

	bm = &glyph->bitmap;
	w = bm->width;
	h = bm->rows;
	buf = bm->buffer;

	pitch = bm->pitch;
	if(pitch < 0) buf += -pitch * (h - 1);
	
	xend = x + w;
	yend = y + h;
	pitch -= w;

	for(iy = y; iy < yend; iy++, buf += pitch)
	{
		for(ix = x; ix < xend; ix++, buf++)
		{
			if(*buf)
				Image_blendPixel(img, ix, iy, col, *buf);
		}
	}
}


//==========================
// main
//==========================


/* 解放 */

void ftlib_free(void)
{
	FTlib *p = g_ftlib;

	if(p)
	{
		if(p->face)
			FT_Done_Face(p->face);
	
		if(p->lib)
			FT_Done_FreeType(p->lib);

		//

		free(g_ftlib);
		g_ftlib = NULL;
	}
}

/* 初期化 */

int ftlib_init(const char *filename,int pxsize)
{
	FTlib *p;
	FT_Face face;

	//確保

	p = g_ftlib = (FTlib *)calloc(1, sizeof(FTlib));
	if(!p) return 1;

	//FreeType 初期化

	FT_Init_FreeType(&p->lib);
	if(!p->lib)
	{
		printf("failed init FreeType\n");
		goto ERR;
	}

	//フォント読み込み

	if(FT_New_Face(p->lib, filename, 0, &face))
	{
		printf("failed load font file\n");
		goto ERR;
	}

	p->face = face;

	//スケーラブルフォントのみ

	if(!FT_IS_SCALABLE(face))
	{
		printf("need scalable font\n");
		goto ERR;
	}

	//px サイズセット

	FT_Set_Pixel_Sizes(face, 0, pxsize);

	//情報取得

	_get_info(p, face);

	return 0;

ERR:
	ftlib_free();
	return 1;
}

/* 高さ取得 */

int ftlib_getHeight(void)
{
	return g_ftlib->height;
}

/* グリフインデックスから、1文字の幅取得 */

int ftlib_getCharWidth_GID(int gid)
{
	FTlib *p = g_ftlib;
	FT_BitmapGlyph glyph;
	int w = 0;
	
	glyph = _get_bitmap_glyph(p, gid);
	if(glyph)
	{
		w = glyph->root.advance.x >> 16;
	
		FT_Done_Glyph((FT_Glyph)glyph);
	}
	
	return w;
}

/* グリフインデックスから、1文字描画
 *
 * return : 次の文字の x 位置 */

int ftlib_drawChar_GID(Image *img,int x,int y,int gid,uint32_t col)
{
	FT_BitmapGlyph glyph;
	int top;

	glyph = _get_bitmap_glyph(g_ftlib, gid);
	if(glyph)
	{
		top = y + g_ftlib->baseline - glyph->top;
		
		//グリフの範囲を塗りつぶし
		
		Image_fillBox(img,
			x, top, glyph->root.advance.x >> 16, glyph->bitmap.rows, 0xe0e0e0);

		//グリフ

		_draw_char(g_ftlib, img, x + glyph->left, top, col, glyph);

		x += glyph->root.advance.x >> 16;
		
		FT_Done_Glyph((FT_Glyph)glyph);
	}

	return x;
}
