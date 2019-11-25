/*******************************
 * vhea,vmtx テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"

uint16_t numMetrics = 0;


/* vhea テーブル読み込み */

void read_vhea_table(Font *p)
{
	uint32_t ver;
	int16_t asc,desc,linegap;

	printf("---- vhea ----\n\n");
	
	font_printf(p, "version: $+xI\n", &ver);

	//

	asc = font_read16(p);
	desc = font_read16(p);
	linegap = font_read16(p);

	if(ver == 0x10000)
	{
		printf("ascent: %d\n", asc);
		printf("descent: %d\n", desc);
		printf("lineGap: %d\n", linegap);
	}
	else
	{
		printf("vertTypoAscender: %d\n", asc);
		printf("vertTypoDescender: %d\n", desc);
		printf("vertTypoLineGap: %d\n", linegap);
	}

	//
	
	font_printf(p, "advanceHeightMax: $h\n");
	font_printf(p, "minTopSideBearing: $h\n");
	font_printf(p, "minBottomSideBearing: $h\n");
	font_printf(p, "yMaxExtent: $h\n");
	font_printf(p, "caretSlopeRise: $h\n");
	font_printf(p, "caretSlopeRun: $h\n");
	font_printf(p, "caretOffset: $h\n");
	font_seek_cur(p, 2 * 4);
	font_printf(p, "metricDataFormat: $h\n");

	font_printf(p, "numOfLongVerMetrics: $+H\n", &numMetrics);
}

/* vmtx テーブル読み込み */

void read_vmtx_table(Font *p)
{
	int i;
	int16_t ah = 0,tsb;

	printf("\n---- vmtx ----\n\n");
	printf("[ gid ] ah, tsb\n");

	for(i = 0; i < numMetrics; i++)
	{
		ah = font_read16(p);
		tsb = font_read16(p);

		printf("[%05d] %d, %d\n", i, ah, tsb);
	}

	for(; i < p->glyph_nums; i++)
	{
		tsb = font_read16(p);
		printf("[%05d] %d, %d\n", i, ah, tsb);
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

	//maxp テーブルから、グリフ数を読み込み

	font_read_table_maxp(p);

	//vhea テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('v','h','e','a')))
		printf("not found 'vhea'\n");
	else
	{
		printf("numGlyphs: %d\n\n", p->glyph_nums);

		read_vhea_table(p);

		//vmtx テーブル

		if(font_goto_table(p, FONT_MAKE_TAG('v','m','t','x')) == 0)
			read_vmtx_table(p);
	}


	font_close(p);
	
	return 0;
}
