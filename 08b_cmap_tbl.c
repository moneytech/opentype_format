/*******************************
 * cmap テーブルから、
 * グリフID -> Unicode のマッピングテーブルを生成し、
 * output.txt に一覧を出力する。
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* テキストに出力 */

void output_text(Font *p)
{
	FILE *fp;
	uint32_t *pt;
	int x,y;

	fp = fopen("output.txt", "wt");
	if(!fp) return;

	fputs("----| 0| 1| 2| 3| 4| 5| 6| 7| 8| 9| A| B| C| D| E| F|\n", fp);

	pt = p->gind_table;
	
	for(y = 0; y < 65536 / 16; y++)
	{
		fprintf(fp, "%04X|", y << 4);
		
		for(x = 0; x < 16; x++, pt++)
		{
			if(*pt == 0)
				fputs("--|", fp);
			else
			{
				put_uni_to_utf8(fp, *pt);
				fputs("|", fp);
			}
		}

		fputs("\n", fp);
	}

	fclose(fp);

	printf("=> output.txt\n");
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

	//テーブル生成

	font_make_gindex_table(p);

	//出力

	output_text(p);

	font_close(p);
	
	return 0;
}
