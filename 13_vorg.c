/*******************************
 * VORG テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* VORG テーブル読み込み */

void read_vorg_table(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t num,gid;

	fprintf(fp, "---- VORG ----\n\n");

	font_fprintf(p, fp, "version: $H.$H\n");
	font_fprintf(p, fp, "defaultVertOriginY: $h\n");
	font_fprintf(p, fp, "numVertOriginYMetrics: $+H\n", &num);

	//vertOriginYMetrics

	if(num != 0)
	{
		fprintf(fp, "\nGID | char | originY\n");
		fprintf(fp, "---------------------\n");

		for(; num > 0; num--)
		{
			gid = font_read16(p);

			fprintf(fp, "%d | ", gid);
			font_output_gid_to_uni_char(p, gid);
			fprintf(fp, " | %d\n", (int16_t)font_read16(p));
		}
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

	//変換テーブル生成

	font_make_gindex_table(p);

	//VORG テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('V','O','R','G')))
		printf("not found 'VORG'\n");
	else
	{
		if(font_open_output_file(p, "output.txt") == 0)
		{
			printf("=> output.txt\n");
			read_vorg_table(p);
		}
	}

	font_close(p);
	
	return 0;
}
