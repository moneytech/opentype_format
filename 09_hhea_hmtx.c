/*******************************
 * hhea/hmtx テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"

int numberOfHMetrics = 0;


/* hhea テーブル読み込み */

void read_hhea_table(Font *p)
{
	printf("---- hhea ----\n\n");

	font_printf(p, "version: $H.$H\n");
	font_printf(p, "ascender: $h\n");
	font_printf(p, "descender: $h\n");
	font_printf(p, "lineGap: $h\n");
	font_printf(p, "advanceWidthMax: $H\n");
	font_printf(p, "minLeftSideBearing: $h\n");
	font_printf(p, "minRightSideBearing: $h\n");
	font_printf(p, "xMaxExtent: $h\n");
	font_printf(p, "caretSlopeRise: $h\n");
	font_printf(p, "caretSlopeRun: $h\n");
	font_printf(p, "caretOffset: $h\n");
	font_seek_cur(p, 2 * 4);
	font_printf(p, "metricDataFormat: $h\n");
	font_printf(p, "numberOfHMetrics: $+H\n", &numberOfHMetrics);
}

/* hmtx テーブル読み込み */

void read_hmtx_table(Font *p)
{
	int i;
	int16_t aw = 0,lsb;

	printf("\n---- hmtx ----\n\n");
	printf("[ gid ] aw, lsb\n");

	//hMetrics

	for(i = 0; i < numberOfHMetrics; i++)
	{
		aw = font_read16(p);
		lsb = font_read16(p);

		printf("[%05d] %d, %d\n", i, aw, lsb);
	}

	//numberOfHMetrics 以上
	//(aw は最後の hMetrics の値を使う)

	for(; i < p->glyph_nums; i++)
	{
		lsb = font_read16(p);
		printf("[%05d] %d, %d\n", i, aw, lsb);
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

	//グリフ数を読み込み

	font_read_table_maxp(p);

	printf("numGlyphs: %d\n\n", p->glyph_nums);

	//hhea テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('h','h','e','a')) == 0)
		read_hhea_table(p);

	//hmtx テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('h','m','t','x')) == 0)
		read_hmtx_table(p);

	font_close(p);
	
	return 0;
}
