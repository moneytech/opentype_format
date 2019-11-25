/*******************************
 * BASE テーブルのデータを表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"

#define BASETAG_NUM 7

uint32_t g_basetag[BASETAG_NUM];


/* BaseCoord */

void put_base_coord(Font *p,uint32_t offset,uint32_t tag)
{
	uint16_t format;

	font_seek_from_table(p, offset);

	printf("{BaseCoord} ");

	if(tag)
	{
		putchar('<');
		font_put_tag_str(tag);
		printf("> ");
	}
	
	font_printf(p, "format:$+H", &format);
	font_printf(p, " | coordinate:$h");

	switch(format)
	{
		case 2:
			font_printf(p, " | referenceGlyph:$H | baseCoordPoint:$H");
			break;
		case 3:
			font_printf(p, " | deviceTable:$H");
			break;
	}

	printf("\n");
}

/* BaseValue */

void put_base_value(Font *p,uint32_t offset_basevalue)
{
	uint16_t i,cnt,offset;

	font_seek_from_table(p, offset_basevalue);

	printf("\n- BaseValue -\n\n");
	font_printf(p, "defaultBaselineIndex: $H\n");
	font_printf(p, "baseCoordCount: $+H\n\n", &cnt);
	
	for(i = 0; i < cnt; i++)
	{
		offset = font_read16(p);

		//BaseCoord

		font_save_pos(p);

		put_base_coord(p,
			offset_basevalue + offset,
			(i < BASETAG_NUM)? g_basetag[i]: 0);

		font_load_pos(p);
	}
}

/* MinMax */

void put_minmax(Font *p,uint32_t offset_minmax)
{
	uint16_t offset_min,offset_max,cnt;

	font_seek_from_table(p, offset_minmax);

	printf("\n** MinMax **\n\n");
	font_printf(p, "minCoord: $+H\n", &offset_min);
	font_printf(p, "maxCoord: $+H\n", &offset_max);
	font_printf(p, "featMinMaxCount: $+H\n", &cnt);

	if(offset_min)
	{
		printf("\n# minCoord\n");
		put_base_coord(p, offset_minmax + offset_min, 0);
	}

	if(offset_max)
	{
		printf("\n# maxCoord\n");
		put_base_coord(p, offset_minmax + offset_max, 0);
	}
}

/* BaseScript */

void put_base_script(Font *p,uint32_t offset_basescript)
{
	uint16_t offset_basevalue,offset_minmax,cnt;

	printf("\n- BaseScript -\n\n");

	font_printf(p, "baseValuesOffset: $+H\n", &offset_basevalue);
	font_printf(p, "defaultMinMaxOffset: $+H\n", &offset_minmax);
	font_printf(p, "baseLangSysCount: $+H\n", &cnt);

	//BaseValue

	if(offset_basevalue)
		put_base_value(p, offset_basescript + offset_basevalue);

	//MinMax

	if(offset_minmax)
		put_minmax(p, offset_basescript + offset_minmax);
}

/* BaseScriptList */

void put_base_script_list(Font *p,uint32_t offset_basescriptlist)
{
	uint16_t cnt,offset;

	font_seek_from_table(p, offset_basescriptlist);

	cnt = font_read16(p);

	for(; cnt > 0; cnt--)
	{
		//BaseScriptRecord

		font_printf(p, "\n-- BaseScriptRecord [$t] --\n\n");
		font_printf(p, "baseScriptOffset: $+H\n", &offset);
	
		//BaseScript

		font_save_pos(p);
		font_seek_from_table(p, offset_basescriptlist + offset);

		put_base_script(p, offset_basescriptlist + offset);

		font_load_pos(p);
	}
}

/* BaseTagList */

void put_base_tag_list(Font *p)
{
	int cnt,i;
	uint32_t tag;

	printf("\n-- BaseTagList --\n\n");

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		font_printf(p, "$+t\n", &tag);

		if(i < BASETAG_NUM)
			g_basetag[i] = tag;
	}
}

/* Axis */

void put_axis(Font *p,uint16_t offset_axis)
{
	uint16_t offset_tag,offset_script;

	font_seek_from_table(p, offset_axis);

	//Axis

	offset_tag = font_read16(p);
	offset_script = font_read16(p);

	printf("baseTagListOffset: %d\n", offset_tag);
	printf("baseScriptListOffset: %d\n", offset_script);

	//BaseTagList

	if(offset_tag)
	{
		font_seek_from_table(p, offset_axis + offset_tag);
		put_base_tag_list(p);
	}

	//BaseScriptList

	put_base_script_list(p, offset_axis + offset_script);
}

/* BASE テーブル */

void put_base_table(Font *p)
{
	uint16_t majver,minver,offset_h,offset_v;

	majver = font_read16(p);
	minver = font_read16(p);
	offset_h = font_read16(p);
	offset_v = font_read16(p);

	printf("---- BASE ----\n\n");
	printf("version: %d.%d\n", majver, minver);
	printf("horizAxisOffset: %d\n", offset_h);
	printf("vertAxisOffset: %d\n", offset_v);

	if(majver == 1 && minver > 0)
		font_printf(p, "itemVarStoreOffset: $I\n");

	//Axis

	if(offset_h)
	{
		printf("\n==== Horz Axis ====\n\n");
		put_axis(p, offset_h);
	}

	if(offset_v)
	{
		printf("\n==== Vert Axis ====\n\n");
		put_axis(p, offset_v);
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

	if(font_goto_table(p, FONT_MAKE_TAG('B','A','S','E')))
		printf("not found 'BASE'\n");
	else
		put_base_table(p);

	font_close(p);
	
	return 0;
}
