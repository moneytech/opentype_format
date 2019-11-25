/*******************************
 * GSUB の ScriptList の一覧を表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


/* LangSys */

void put_langSys(Font *p,int is_default)
{
	uint16_t cnt;

	font_seek_cur(p, 2);

	printf("<LangSys>%s\n", (is_default)? " [default]":"");

	font_printf(p, " requiredFeatureIndex: $H\n");
	font_printf(p, " featureIndexCount: $+H\n", &cnt);

	//FeatureList のインデックス配列

	printf(" featureIndices: ");

	for(; cnt > 0; cnt--)
		printf("%d,", font_read16(p));

	printf("\n");
}

/* LangSysRecord */

void put_langSysRecord(Font *p,int no,uint32_t offset_script)
{
	uint16_t offset;

	printf("\n## LangSysRecord [%d] ##\n\n", no);

	font_printf(p, "langSysTag: $t\n");
	font_printf(p, "langSysOffset: $+H\n\n", &offset);

	//LangSys

	font_save_pos(p);
	font_seek_from_table(p, offset_script + offset);
	
	put_langSys(p, 0);

	font_load_pos(p);
}

/* Script */

void put_script(Font *p,uint32_t offset_script)
{
	uint16_t defaultLangSys,langSysCount,i;

	font_save_pos(p);
	font_seek_from_table(p, offset_script);

	printf("** Script **\n\n");

	font_printf(p, "defaultLangSys: $+H\n", &defaultLangSys);
	font_printf(p, "langSysCount: $+H\n\n", &langSysCount);

	//defaultLangSys

	if(defaultLangSys)
	{
		font_save_pos(p);
		font_seek_from_table(p, offset_script + defaultLangSys);

		put_langSys(p, 1);

		font_load_pos(p);
	}

	//LangSysRecord (配列)

	for(i = 0; i < langSysCount; i++)
		put_langSysRecord(p, i, offset_script);

	font_load_pos(p);
}

/* ScriptRecord */

void put_scriptRecord(Font *p,int no,uint32_t offset_scriptlist)
{
	uint16_t offset;

	printf("== ScriptRecord [%d] ==\n\n", no);

	font_printf(p, "scriptTag: $t\n");
	font_printf(p, "scriptOffset: $+H\n\n", &offset);

	//Script

	put_script(p, offset_scriptlist + offset);
}

/* ScriptList */

void put_script_list(Font *p)
{
	uint32_t offset_scriptlist;
	uint16_t scriptCount,i;

	//ScriptList のオフセット位置 (GSUB 先頭から)

	offset_scriptlist = p->offset_dat[0];

	font_seek_from_table(p, offset_scriptlist);

	//ScriptList

	printf("---- Script List ----\n\n");

	font_printf(p, "scriptCount: $+H\n\n", &scriptCount);

	for(i = 0; i < scriptCount; i++)
	{
		//ScriptRecord (配列)

		put_scriptRecord(p, i, offset_scriptlist);

		printf("\n");
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

	if(font_goto_GSUB(p) == 0)
		put_script_list(p);

	font_close(p);
	
	return 0;
}
