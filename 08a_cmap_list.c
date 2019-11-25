/*******************************
 * cmap テーブルの一覧を表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* cmap テーブル読み込み */

void read_cmap_table(Font *p)
{
	uint16_t ver,num,platform,encoding,format;
	uint32_t offset;

	printf("---- cmap  ----\n\n");

	font_printf(p, "version: $+H\n", &ver);
	font_printf(p, "numTables: $+H\n\n", &num);

	for(; num > 0; num--)
	{
		//EncodingRecord

		platform = font_read16(p);
		encoding = font_read16(p);
		offset = font_read32(p);

		printf("[platformID: %d ", platform);

		if(platform == 0)
			printf("(Unicode)");
		else if(platform == 1)
			printf("(Mac)");
		else if(platform == 3)
			printf("(Windows)");
			
		printf(" | encodingID: %d | offset: %u]\n", encoding, offset);

		//サブテーブル

		font_save_pos(p);
		font_seek_from_table(p, offset);

		format = font_read16(p);
		printf("= format:%d ", format);

		if(format == 4)
			printf("(Unicode BMP)");
		else if(format == 12)
			printf("(Unicode full)");
		else if(format == 14)
			printf("(Unicode UVS)");

		printf("\n\n");

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

	//cmap テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('c','m','a','p')) == 0)
		read_cmap_table(p);

	font_close(p);
	
	return 0;
}
