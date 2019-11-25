/*******************************
 * OS/2 テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* OS/2 テーブル読み込み */

void read_os2_table(Font *p)
{
	uint16_t ver;
	uint32_t u32[4];
	uint8_t panose[10];
	int i;

	printf("---- OS/2 ----\n\n");

	font_printf(p, "version: $+H\n", &ver);
	font_printf(p, "xAvgCharWidth: $h\n");
	font_printf(p, "usWeightClass: $H\n");
	font_printf(p, "usWidthClass: $H\n");
	font_printf(p, "fsType: $bH\n");
	font_printf(p, "ySubscriptXSize: $h\n");
	font_printf(p, "ySubscriptYSize: $h\n");
	font_printf(p, "ySubscriptXOffset: $h\n");
	font_printf(p, "ySubscriptYOffset: $h\n");
	font_printf(p, "ySuperscriptXSize: $h\n");
	font_printf(p, "ySuperscriptYSize: $h\n");
	font_printf(p, "ySuperscriptXOffset: $h\n");
	font_printf(p, "ySuperscriptYOffset: $h\n");
	font_printf(p, "yStrikeoutSize: $h\n");
	font_printf(p, "yStrikeoutPosition: $h\n");
	font_printf(p, "sFamilyClass: $xH\n");

	fread(panose, 1, 10, p->fp);
	printf("panose: ");
	for(i = 0; i < 10; i++)
		printf("%d,", panose[i]);
	printf("\n");

	for(i = 0; i < 4; i++)
		u32[i] = font_read32(p);
	printf("ulUnicodeRange1-4: 0x%08X_%08X_%08X_%08X\n",
		u32[3], u32[2], u32[1], u32[0]);

	font_printf(p, "achVendID: $t\n");
	font_printf(p, "fsSelection: $bH\n");
	font_printf(p, "usFirstCharIndex: $xH\n");
	font_printf(p, "usLastCharIndex: $xH\n");
	font_printf(p, "sTypoAscender: $h\n");
	font_printf(p, "sTypoDescender: $h\n");
	font_printf(p, "sTypoLineGap: $h\n");
	font_printf(p, "usWinAscent: $H\n");
	font_printf(p, "usWinDescent: $H\n");

	//ver 1〜

	if(ver >= 1)
	{
		u32[0] = font_read32(p);
		u32[1] = font_read32(p);
		printf("ulCodePageRange1-2: 0x%08X_%08X\n", u32[1], u32[0]);
	}

	//ver 2〜

	if(ver >= 2)
	{
		font_printf(p, "sxHeight: $h\n");
		font_printf(p, "sCapHeight: $h\n");
		font_printf(p, "usDefaultChar: $xH\n");
		font_printf(p, "usBreakChar: $xH\n");
		font_printf(p, "usMaxContext: $H\n");
	}

	//ver 5

	if(ver == 5)
	{
		font_printf(p, "usLowerOpticalPointSize: $H\n");
		font_printf(p, "usUpperOpticalPointSize: $H\n");
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

	//OS/2 テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('O','S','/','2')) == 0)
		read_os2_table(p);

	font_close(p);
	
	return 0;
}
