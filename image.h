
typedef struct _Image Image;

struct _Image
{
	uint32_t *buf;
	int width,
		height;
};

//グリフの描画

typedef struct
{
	Image *img;		//画像
	int padding,	//余白
		font_height,
		curpage,	//現在のページ
		curx,cury,	//現在の描画位置
		cur_colx,	//現在の横列の x 位置
		x_max,		//現在の横列の最大 x 位置
		col_width;	//横列の最低幅
}DrawGlyph;


void Image_free(Image *p);
Image *Image_new(int width,int height);

void Image_clear(Image *p,uint32_t c);
void Image_drawHLine(Image *p,int x,int y,int w,uint32_t c);
void Image_drawVLine(Image *p,int x,int y,int h,uint32_t c);
void Image_fillBox(Image *p,int x,int y,int w,int h,uint32_t c);

void Image_setPixel(Image *p,int x,int y,uint32_t c);
void Image_blendPixel(Image *p,int x,int y,uint32_t c,int alpha);

void Image_writeBitmap(Image *p,const char *filename);

//

int DrawGlyph_init(DrawGlyph *p,int width,int height);
void DrawGlyph_end(DrawGlyph *p);
void DrawGlyph_nextLine(DrawGlyph *p);
void DrawGlyph_draw(DrawGlyph *p,uint16_t gid,uint32_t col);
void DrawGlyph_drawPair(DrawGlyph *p,uint16_t gid1,uint16_t gid2,uint32_t col1,uint32_t col2);
void DrawGlyph_drawRowLine(DrawGlyph *p);
