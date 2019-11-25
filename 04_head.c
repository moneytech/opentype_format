/*******************************
 * head テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>
#include <time.h>

#include "fontfile.h"


/* LONGDATETIME の日時を表示 */

void put_date(int64_t val)
{
	struct tm *ptm;
	time_t time;
	char m[32];

	/* LONGDATETIME: GMT 1904/01/01 00:00 からの秒数
	 * time 関数   : GMT 1970/01/01 00:00 からの秒数
	 *
	 * time 関数の基準に合わせる場合、
	 * 1904/01/01 から 1970/01/01 にかけての経過秒数を引く。 */

	time = val - 2082844800;

	ptm = localtime(&time);

	strftime(m, 32, "%Y/%m/%d %H:%M:%S", ptm);

	printf("%s\n", m);
}

/* head テーブル読み込み */

void read_head_table(Font *p)
{
	uint32_t revision;

	printf("--- head ----\n\n");

	font_printf(p, "version: $H.$H\n");

	revision = font_read32(p);
	printf("fontRevision       : 0x%08X (%.4f)\n",
		revision, revision / 65536.0);

	font_printf(p, "checkSumAdjustment : $xI\n");
	font_printf(p, "magicNumber        : $xI\n");
	font_printf(p, "flags              : $bH\n");
	font_printf(p, "unitsPerEm         : $H\n");

	printf("created            : ");
	put_date(font_read64(p));

	printf("modified           : ");
	put_date(font_read64(p));

	font_printf(p, "xMin,yMin,xMax,yMax: $h, $h, $h, $h\n");

	font_printf(p, "macStyle           : $bH\n");
	font_printf(p, "lowestRecPPEM      : $H\n");
	font_printf(p, "fontDirectionHint  : $h\n");
	font_printf(p, "indexToLocFormat   : $h\n");
	font_printf(p, "glyphDataFormat    : $h\n");
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

	//head テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('h','e','a','d')) == 0)
		read_head_table(p);

	font_close(p);
	
	return 0;
}
