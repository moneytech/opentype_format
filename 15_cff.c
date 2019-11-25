/*******************************
 * CFF テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"
#include "cff.h"


/* ヘッダ */

void read_header(CFF *p)
{
	uint8_t hsize;

	printf("---- Header ----\n\n");

	printf("version: %d.%d\n",
		cff_read8(p), cff_read8(p));

	hsize = cff_read8(p);
	printf("hdrSize: %d\n", hsize);

	p->abs_offsize = cff_read8(p);
	printf("offSize: %d\n", p->abs_offsize);

	//Name INDEX へ

	cff_seek_abs(p, hsize);
}

/* 文字列データ */

void func_putstr(CFF *p,int no,int len)
{
	putchar('"');
	cff_put_str(p, len);
	printf("\" (%d)\n", len);
}

/* Top DICT データ */

void func_topDICT_data(CFF *p,uint16_t key,int val,int val2)
{
	if(key == 15)
		p->offset_charset = val;
	else if(key == 17)
		p->offset_charstrings = val;
	else if(key == 18)
	{
		p->private_offset = val;
		p->private_size = val2;
	}
	else if(key == CFF_KEY2(36))
		p->offset_fdarray = val;
	else if(key == CFF_KEY2(37))
		p->offset_fdselect = val;
}

void func_topDICT(CFF *p,int no,int len)
{
	cff_procDICT(p, len, func_topDICT_data);
}

/* String INDEX データ */

void func_stringINDEX(CFF *p,int no,int len)
{
	printf("SID[%d] \"", 391 + no);
	cff_put_str(p, len);
	printf("\" (%d)\n", len);
}

/* Charsets */

void read_charsets(CFF *p)
{
	uint8_t format;
	uint16_t sid;
	int i,len,gid;

	printf("\n---- Charsets ----\n\n");

	if(p->offset_charset <= 2)
		//定義済み文字セットID
		printf("charset ID: %d\n", p->offset_charset);
	else
	{
		cff_seek_abs(p, p->offset_charset);

		//フォーマット
		
		format = cff_read8(p);
		printf("(glyphs: %d)\n", p->glyphs);
		printf("format: %d\n\n", format);

		//

		switch(format)
		{
			//Format 0 (配列)
			case 0:
				for(i = p->glyphs - 1; i > 0; i--)
					printf("%d,", cff_read16(p));

				printf("\n");
				break;

			//Format 1,2 (範囲)
			case 1:
			case 2:
				gid = 1;
				do
				{
					sid = cff_read16(p);

					if(format == 1)
						len = cff_read8(p);
					else
						len = cff_read16(p);

					if(len == 0)
						printf("GID %d: %d\n", gid, sid);
					else
						printf("GID %d-%d: %d-%d\n", gid, gid + len, sid, sid + len);

					gid += len + 1;
				} while(gid < p->glyphs);
				break;
		}
	}
}

/* FDSelect */

void read_fdselect(CFF *p)
{
	uint8_t format;
	int i,num;

	cff_seek_abs(p, p->offset_fdselect);

	format = cff_read8(p);

	printf("\n---- FDSelect ----\n\n");
	printf("format: %d\n\n", format);

	if(format == 0)
	{
		//Format 0

		for(i = p->glyphs; i > 0; i--)
			printf("%d,", cff_read8(p));
		printf("\n");
	}
	else if(format == 3)
	{
		//Format 3

		num = cff_read16(p);

		for(; num > 0; num--)
			printf("%d, %d\n", cff_read16(p), cff_read8(p));

		printf("sentinel: %d\n", cff_read16(p));
	}
}

/* Font DICT */

void func_fontDICT(CFF *p,uint16_t key,int val,int val2)
{
	if(key == 18)
	{
		//Private
		p->private_offset = val;
		p->private_size = val2;
	}
}

/* FDArray (Font DICT INDEX) */

void func_fdarray(CFF *p,int no,int size)
{
	printf("## %d ##\n\n", no);

	p->private_offset = 0;
	cff_procDICT(p, size, func_fontDICT);

	//Private DICT

	if(p->private_offset)
	{
		printf("\n<< Private DICT >>\n\n");

		cff_seek_abs(p, p->private_offset);
		cff_procDICT(p, p->private_size, NULL);
	}

	printf("\n");
}


//====================


/* CFF テーブル読み込み */

void read_cff_table(CFF *p)
{
	read_header(p);

	//Name INDEX

	printf("\n---- Name INDEX ----\n\n");
	cff_readINDEX(p, func_putstr);

	//Top DICT INDEX

	printf("\n---- Top DICT INDEX ----\n\n");
	cff_readINDEX(p, func_topDICT);

	//String INDEX

	printf("\n---- String INDEX ----\n\n");
	cff_readINDEX(p, func_stringINDEX);

	//CharStrings INDEX から、グリフ数読み込み

	cff_seek_abs(p, p->offset_charstrings);

	p->glyphs = cff_read16(p);

	//Charsets

	read_charsets(p);

	//Private DICT

	if(p->private_offset)
	{
		printf("\n---- Private DICT ----\n\n");

		cff_seek_abs(p, p->private_offset);
		cff_procDICT(p, p->private_size, NULL);
	}

	//FDSelect

	if(p->offset_fdselect)
		read_fdselect(p);

	//FDArray (Font DICT INDEX)

	if(p->offset_fdarray)
	{
		printf("\n---- Font DICT INDEX ----\n\n");

		cff_seek_abs(p, p->offset_fdarray);
		cff_readINDEX(p, func_fdarray);
	}
}

/* main */

int main(int argc,char **argv)
{
	Font *p;
	CFF *cff = NULL;

	if(argc < 2)
	{
		printf("[usage] %s <fontfile>\n", argv[0]);
		return 0;
	}

	p = font_open_file(argv[1]);
	if(!p) return 1;

	//CFF テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('C','F','F',' ')))
		printf("not found 'CFF '\n");
	else
	{
		cff = cff_new(p);
		if(cff)
			read_cff_table(cff);
	}

	cff_free(cff);
	font_close(p);
	
	return 0;
}
