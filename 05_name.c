/*******************************
 * name テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* name テーブル読み込み */

void read_name_table(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t format,count,stroffset,num,len,offset;
	uint16_t platformID,encodingID,languageID,nameID,prev_platID;

	fputs("---- name ----\n\n", fp);

	font_fprintf(p, fp,
		"format: $+H\ncount: $+H\nstringOffset: $+H\n\n",
		&format, &count, &stroffset);

	//LangTagRecord

	if(format == 1)
	{
		font_save_pos(p);
		
		//NameRecord をスキップ
		font_seek_cur(p, count * 12);

		num = font_read16(p);

		fprintf(fp, "=== LangTagRecord ===\n\n");

		for(; num > 0; num--)
		{
			//LangTagRecord
		
			len = font_read16(p);
			offset = font_read16(p);

			//文字列

			font_save_pos(p);
			font_seek_from_table(p, stroffset + offset);

			font_output_utf16be_str(p, len);

			font_load_pos(p);
		}

		font_load_pos(p);
	}

	//NameRecord

	fputs("=== NameRecord ===\n\n", fp);

	prev_platID = 0xffff;

	for(; count > 0; count--)
	{
		platformID = font_read16(p);
		encodingID = font_read16(p);
		languageID = font_read16(p);
		nameID = font_read16(p);
		len = font_read16(p);
		offset = font_read16(p);

		//platformID が変化した時、区切りを付ける

		if(platformID != prev_platID && prev_platID != 0xffff)
			fputs("----\n", fp);

		//

		fprintf(fp,
			"[platID:%d | encID:%d | langID:%d (0x%X) | nameID:%d | offset:%d]\n",
			platformID, encodingID, languageID, languageID, nameID, offset);

		//文字列 (UTF-16BE のみ)

		if(platformID == 3 || (platformID == 0 && encodingID <= 3))
		{
			font_save_pos(p);
			font_seek_from_table(p, stroffset + offset);

			font_output_utf16be_str(p, len);
			fputc('\n', fp);

			font_load_pos(p);
		}

		prev_platID = platformID;
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

	//name テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('n','a','m','e')) == 0)
	{
		if(font_open_output_file(p, "output.txt") == 0)
		{
			read_name_table(p);

			printf("=> output.txt\n");
		}
	}

	font_close(p);
	
	return 0;
}
