/*******************************
 * GSUB テーブルの FeatureList の一覧を表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"

//-------------

typedef struct
{
	uint32_t script,langsys;
}ScriptTag;

ScriptTag *g_feature_tag = NULL;
int g_feature_num = 0;

//-------------


/* LangSys テーブル読み込み */

void read_langSys(Font *p,uint32_t script_tag,uint32_t langsys_tag)
{
	uint16_t req,num,ind;

	font_seek_cur(p, 2);

	req = font_read16(p);
	num = font_read16(p);

	//必須

	if(req != 0xffff && req < g_feature_num)
	{
		g_feature_tag[req].script = script_tag;
		g_feature_tag[req].langsys = langsys_tag;
	}

	//FeatureList テーブルのインデックス配列

	for(; num > 0; num--)
	{
		ind = font_read16(p);

		if(ind < g_feature_num)
		{
			g_feature_tag[ind].script = script_tag;
			g_feature_tag[ind].langsys = langsys_tag;
		}
	}
}

/* ScriptList テーブル読み込み */

void read_script_list(Font *p)
{
	uint16_t offset,offset_def,script_cnt,langsys_cnt;
	uint32_t offset_scriptlist,offset_script;
	uint32_t script_tag,langsys_tag;

	//ScriptList のオフセット位置

	offset_scriptlist = p->offset_dat[0];

	font_seek_from_table(p, offset_scriptlist);

	//ScriptList

	script_cnt = font_read16(p);

	for(; script_cnt > 0; script_cnt--)
	{
		//ScriptRecord

		script_tag = font_read32(p);
		offset = font_read16(p);

		//Script

		offset_script = offset_scriptlist + offset;

		font_save_pos(p);
		font_seek_from_table(p, offset_script);

		offset_def = font_read16(p);
		langsys_cnt = font_read16(p);

		//LangSys (Default)

		if(offset_def != 0)
		{
			font_save_pos(p);
			font_seek_from_table(p, offset_script + offset_def);

			read_langSys(p, script_tag, FONT_MAKE_TAG('*','d','e','f'));

			font_load_pos(p);
		}

		//LangSysRecord

		for(; langsys_cnt > 0; langsys_cnt--)
		{
			langsys_tag = font_read32(p);
			offset = font_read16(p);

			//LangSys

			font_save_pos(p);
			font_seek_from_table(p, offset_script + offset);

			read_langSys(p, script_tag, langsys_tag);

			font_load_pos(p);
		}

		font_load_pos(p);
	}
}

/* FeatureList テーブル表示 */

void put_feature_list(Font *p)
{
	uint32_t offset_featurelist;
	uint16_t cnt,lcnt,offset;
	int i;

	printf("---- FeatureList ----\n\n");

	//FeatureList へ

	offset_featurelist = p->offset_dat[1];

	font_seek_from_table(p, offset_featurelist);

	//FeatureList

	font_printf(p, "featureCount: $+H\n\n", &cnt);

	for(i = 0; i < cnt; i++)
	{
		//FeatureRecord

		printf("[%d] ", i);

		font_printf(p, "$t | offset:$+H | ", &offset);

		//Feature

		font_save_pos(p);
		font_seek_from_table(p, offset_featurelist + offset);

		font_seek_cur(p, 2);
		lcnt = font_read16(p);

		for(; lcnt > 0; lcnt--)
			printf("%d,", font_read16(p));

		//Script タグ

		printf(" (");
		font_put_tag_str(g_feature_tag[i].script);
		printf(" ");
		font_put_tag_str(g_feature_tag[i].langsys);
		printf(")\n");

		font_load_pos(p);
	}
}

/* 作業用データ準備 */

void init(Font *p)
{
	//FeatureList からデータ数取得

	font_seek_from_table(p, p->offset_dat[1]);

	g_feature_num = font_read16(p);

	//メモリ確保

	g_feature_tag = (ScriptTag *)calloc(1, sizeof(ScriptTag) * g_feature_num);
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
	{
		init(p);
		
		read_script_list(p);
		put_feature_list(p);

		free(g_feature_tag);
	}

	font_close(p);
	
	return 0;
}
