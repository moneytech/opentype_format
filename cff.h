#ifndef _CFF_H_
#define _CFF_H_

#define CFF_KEY2(k2)  (0x0c00 | (k2))

typedef struct _CFF CFF;
typedef struct _Font Font;

struct _CFF
{
	Font *font;
	FILE *fp;
	uint32_t pos_cff;	//CFF のファイル位置

	uint8_t abs_offsize;	//CFF の先頭を0とするオフセット値のサイズ
	int offset_charset,		//Charsets データのオフセット値
		offset_charstrings,	//CharStrings データのオフセット値
		offset_fdselect,	//FDSelect のオフセット値
		offset_fdarray,		//FDArray のオフセット値
		private_size,
		private_offset,
		glyphs;				//グリフ数
};

CFF *cff_new(Font *font);
void cff_free(CFF *p);

int cff_readINDEX(CFF *p,void (*func)(CFF *,int,int));
void cff_procDICT(CFF *p,uint32_t size,void (*func)(CFF *,uint16_t,int,int));

void cff_seek_abs(CFF *p,uint32_t pos);

uint8_t cff_read8(CFF *p);
uint16_t cff_read16(CFF *p);
uint32_t cff_read_offset(CFF *p,int size);

void cff_put_str(CFF *p,int len);

#endif
