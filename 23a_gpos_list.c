/*******************************
 * GPOS の
 * ScriptList/FeatureList/LookupList の一覧を表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdint.h>

#include "fontfile.h"


//=========================
// ScriptList
//=========================

/* LangSys */

void put_langSys(Font *p,int is_default)
{
	uint16_t cnt;

	font_seek_cur(p, 2);

	printf("<LangSys>%s", (is_default)? " [default]":"");

	font_printf(p, " requiredFeatureIndex: $H\n");

	cnt = font_read16(p);

	//FeatureList のインデックス配列

	printf("(");

	for(; cnt > 0; cnt--)
		printf("%d,", font_read16(p));

	printf(")\n");
}

/* LangSysRecord */

void put_langSysRecord(Font *p,uint32_t offset_script)
{
	uint16_t offset;

	font_printf(p, "\n-- LangSysRecord [$t] --\n");

	offset = font_read16(p);

	//LangSys

	font_save_pos(p);
	font_seek_from_table(p, offset_script + offset);
	
	put_langSys(p, 0);

	font_load_pos(p);
}

/* Script */

void put_script(Font *p,uint32_t offset_script)
{
	uint16_t def,cnt;

	font_save_pos(p);
	font_seek_from_table(p, offset_script);

	def = font_read16(p);
	cnt = font_read16(p);

	//defaultLangSys

	if(def)
	{
		font_save_pos(p);
		font_seek_from_table(p, offset_script + def);

		put_langSys(p, 1);

		font_load_pos(p);
	}

	//LangSysRecord (配列)

	for(; cnt > 0; cnt--)
		put_langSysRecord(p, offset_script);

	font_load_pos(p);
}

/* ScriptList */

void put_script_list(Font *p)
{
	uint32_t offset_scriptlist;
	uint16_t cnt,offset;

	offset_scriptlist = p->offset_dat[0];

	font_seek_from_table(p, offset_scriptlist);

	//ScriptList

	printf("---- Script List ----\n");

	cnt = font_read16(p);

	for(; cnt > 0; cnt--)
	{
		//ScriptRecord

		font_printf(p, "\n== $t ==\n\n");

		offset = font_read16(p);

		//Script

		put_script(p, offset_scriptlist + offset);
	}
}


//=======================
// FeatureList
//=======================


/* FeatureList テーブル表示 */

void put_feature_list(Font *p)
{
	uint32_t offset_featurelist;
	uint16_t cnt,lcnt,offset;
	int i;

	printf("\n---- FeatureList ----\n\n");

	//FeatureList へ

	offset_featurelist = p->offset_dat[1];

	font_seek_from_table(p, offset_featurelist);

	//FeatureList

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		//FeatureRecord

		printf("[%d] ", i);

		font_printf(p, "$t | ");

		offset = font_read16(p);

		//Feature

		font_save_pos(p);
		font_seek_from_table(p, offset_featurelist + offset);

		font_seek_cur(p, 2);
		lcnt = font_read16(p);

		for(; lcnt > 0; lcnt--)
			font_printf(p, "$H,");

		putchar('\n');

		font_load_pos(p);
	}
}


//=======================
// LookupList
//=======================


/* Lookup テーブル */

void put_lookup(Font *p,uint32_t offset_lookup)
{
	uint16_t type,flags,cnt,offset;
	uint32_t offset_subtable,offset32;

	//Lookup

	type = font_read16(p);
	flags = font_read16(p);
	cnt = font_read16(p);

	printf("\n{Lookup} lookupType <%d> | lookupFlag:0x%04X | subTableCount:%d\n\n",
		type, flags, cnt);

	//

	for(; cnt > 0; cnt--)
	{
		offset = font_read16(p);
		
		offset_subtable = offset_lookup + offset;

		//subtable

		font_save_pos(p);
		font_seek_from_table(p, offset_subtable);

		font_printf(p, "# {SubTable} format <$H>\n");

		//LookupType = 9 の場合

		if(type == 9)
		{
			font_printf(p, "==> lookupType <$H> | offset:$+I | ", &offset32);

			//実体のサブテーブルへ

			font_seek_from_table(p, offset_subtable + offset32);

			font_printf(p, "format <$H>\n");
		}

		font_load_pos(p);
	}
}

/* LookupList テーブル */

void put_lookup_list(Font *p)
{
	uint32_t offset_lookuplist;
	uint16_t offset;
	int i,cnt;

	offset_lookuplist = p->offset_dat[2];

	font_seek_from_table(p, offset_lookuplist);

	printf("\n---- LookupList ----\n");

	//LookupList

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		offset = font_read16(p);

		printf("\n==== [%d] ====\n", i);

		//Lookup

		font_save_pos(p);
		font_seek_from_table(p, offset_lookuplist + offset);

		put_lookup(p, offset_lookuplist + offset);
		
		font_load_pos(p);
	}
}


//================

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

	if(font_goto_GPOS(p) == 0)
	{
		put_script_list(p);
		put_feature_list(p);
		put_lookup_list(p);
	}

	font_close(p);
	
	return 0;
}
