/***************************
 * CFF データ処理
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "cff.h"
#include "fontfile.h"


/* 解放 */

void cff_free(CFF *p)
{
	if(p)
	{
		free(p);
	}
}

/* CFF データの作成 */

CFF *cff_new(Font *font)
{
	CFF *p;

	p = (CFF *)calloc(1, sizeof(CFF));
	if(!p) return NULL;

	p->font = font;
	p->fp = font->fp;
	p->pos_cff = font->cur_table_offset;

	return p;
}

/* INDEX データを読み込み
 *
 * 各オブジェクトデータごとに func 関数が呼ばれる。
 * func(CFF *p,int no,int datasize)
 *
 * return: オブジェクトの数。0 で空データ。 */

int cff_readINDEX(CFF *p,void (*func)(CFF *,int,int))
{
	int i,count,offsize;
	uint32_t offset,offset2,data_pos;
	fpos_t pos;

	count = cff_read16(p);
	if(count == 0) return 0;
	
	offsize = cff_read8(p);

	data_pos = ftell(p->fp) + (count + 1) * offsize - 1;

	//最初のオフセット値を読み込んでおく
	offset = cff_read_offset(p, offsize);

	for(i = 0; i < count; i++)
	{
		offset2 = cff_read_offset(p, offsize);

		//

		fgetpos(p->fp, &pos);
		fseek(p->fp, data_pos + offset, SEEK_SET);

		(func)(p, i, offset2 - offset);

		fsetpos(p->fp, &pos);

		//

		offset = offset2;
	}

	//次のデータの位置へ

	fseek(p->fp, data_pos + offset, SEEK_SET);

	return count;
}

/* DICT データを処理して表示
 *
 * size : データサイズ
 * func : キーごとの処理関数 (NULL でなし)
 *
 * func(CFF *p,uint16_t key,int val1,int val2)
 * - val1: キーの直前の整数値
 * - val2: val1 の前の整数値 */

void cff_procDICT(CFF *p,uint32_t size,void (*func)(CFF *,uint16_t,int,int))
{
	uint8_t b0,b[4];
	int32_t val = 0,val2 = 0,pos;
	uint16_t key;

	while(size)
	{
		b0 = cff_read8(p);
		size--;

		//---- キー

		if(b0 <= 21)
		{

			printf("[%d", b0);
		
			if(b0 != 12)
				key = b0;
			else
			{
				//2byte

				b0 = cff_read8(p);
				size--;

				key = (12 << 8) | b0;

				printf(", %d", b0);
			}

			printf("]\n");

			if(func)
				(func)(p, key, val, val2);
			
			continue;
		}

		//---- 実数

		if(b0 == 30)
		{
			pos = 0;

			while(size)
			{
				//4bit 値
				
				if(pos & 1)
					val = b0 & 0xf;
				else
				{
					b0 = cff_read8(p);
					size--;

					val = b0 >> 4;
				}

				pos ^= 1;

				//

				if(val == 15)
					break;
				else if(val >= 0 && val <= 9)
					putchar('0' + val);
				else if(val == 10)
					putchar('.');
				else if(val == 11)
					putchar('E');
				else if(val == 12)
					printf("E-");
				else if(val == 14)
					putchar('-');
			}

			printf(", ");
			continue;
		}

		//---- 整数値

		val2 = val;

		if(b0 >= 32 && b0 <= 246)
			//-107..107
			val = b0 - 139;
		else if(b0 >= 247 && b0 <= 250)
		{
			//108..1131

			b[0] = cff_read8(p);
			size--;
			
			val = ((b0 - 247) << 8) + b[0] + 108;
		}
		else if(b0 >= 251 && b0 <= 254)
		{
			//-1131..-108

			b[0] = cff_read8(p);
			size--;
			
			val = -((b0 - 251) << 8) - b[0] - 108;
		}
		else if(b0 == 28)
		{
			//-32768..32767
			
			fread(b, 1, 2, p->fp);
			size -= 2;

			val = (b[0] << 8) | b[1];
		}
		else if(b0 == 29)
		{
			//5byte

			fread(b, 1, 4, p->fp);
			size -= 4;

			val = (int32_t)((uint32_t)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
		}

		printf("%d, ", val);
	}
}


//=======================


/* CFF の先頭からシーク */

void cff_seek_abs(CFF *p,uint32_t pos)
{
	fseek(p->fp, p->pos_cff + pos, SEEK_SET);
}

/* 1byte 読み込み */

uint8_t cff_read8(CFF *p)
{
	uint8_t b;

	fread(&b, 1, 1, p->fp);

	return b;
}

/* 2byte 読み込み */

uint16_t cff_read16(CFF *p)
{
	uint8_t b[2];

	fread(b, 1, 2, p->fp);

	return (b[0] << 8) | b[1];
}

/* Offset 値読み込み
 *
 * size: オフセットのバイトサイズ。-1 で abs_offsize。 */

uint32_t cff_read_offset(CFF *p,int size)
{
	uint8_t b[4];

	if(size == -1)
		size = p->abs_offsize;

	if(size < 1) size = 1;
	else if(size > 4) size = 4;

	fread(b, 1, size, p->fp);

	switch(size)
	{
		case 4:
			return ((uint32_t)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
		case 3:
			return (b[0] << 16) | (b[1] << 8) | b[2];
		case 2:
			return (b[0] << 8) | b[1];
		default:
			return b[0];
	}
}

/* 文字列を読み込んで表示 */

void cff_put_str(CFF *p,int len)
{
	char c;

	for(; len > 0; len--)
	{
		c = fgetc(p->fp);
		putchar(c);

		if(c == 0) break;
	}
}
