/*******************************
 * maxp,post テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* maxp テーブル読み込み */

void read_maxp_table(Font *p)
{
	uint32_t ver;

	printf("---- maxp ----\n\n");

	//version

	ver = font_read32(p);

	printf("version: 0x%08X (%d.%d)\n",
		ver, ver >> 16, ((ver & 0xffff) == 0x5000)? 5:0);

	//グリフ数

	font_printf(p, "numGlyphs: $H\n");

	//ver 1.0

	if(ver == 0x00010000)
	{
		font_printf(p, "maxPoints: $H\n");
		font_printf(p, "maxContours: $H\n");
		font_printf(p, "maxCompositePoints: $H\n");
		font_printf(p, "maxCompositeContours: $H\n");
		font_printf(p, "maxZones: $H\n");
		font_printf(p, "maxTwilightPoints: $H\n");
		font_printf(p, "maxStorage: $H\n");
		font_printf(p, "maxFunctionDefs: $H\n");
		font_printf(p, "maxInstructionDefs: $H\n");
		font_printf(p, "maxStackElements: $H\n");
		font_printf(p, "maxSizeOfInstructions: $H\n");
		font_printf(p, "mmaxComponentElements: $H\n");
		font_printf(p, "maxComponentDepth: $H\n");
	}
}

/* post テーブル読み込み */

void read_post_table(Font *p)
{
	uint32_t ver,angle;

	printf("\n---- post ----\n\n");

	//version

	ver = font_read32(p);

	printf("version: 0x%08X (%d.%d)\n",
		ver, ver >> 16, ((ver & 0xffff) == 0x5000)? 5:0);

	//italicAngle

	angle = font_read32(p);
	printf("italicAngle: 0x%08X (%.3f)\n",
		angle, (double)angle / (1<<16));

	//

	font_printf(p, "underlinePosition: $h\n");
	font_printf(p, "underlineThickness: $h\n");
	font_printf(p, "isFixedPitch: $I\n");
	font_printf(p, "minMemType42: $I\n");
	font_printf(p, "maxMemType42: $I\n");
	font_printf(p, "minMemType1: $I\n");
	font_printf(p, "maxMemType1: $I\n");
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

	//maxp テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('m','a','x','p')) == 0)
		read_maxp_table(p);

	//post テーブル

	if(font_goto_table(p, FONT_MAKE_TAG('p','o','s','t')) == 0)
		read_post_table(p);

	font_close(p);
	
	return 0;
}
