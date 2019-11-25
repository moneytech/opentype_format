/*******************************
 * GSUB テーブルの LookupList の一覧を表示
 *
 * [usage] exe <fontfile>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "fontfile.h"

//--------------

uint32_t *g_lookup_tag = NULL;
int g_lookup_num = 0;

//--------------


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

		printf("# {SubTable} offset:%d | ", offset);
		font_printf(p, "format <$H>\n");

		//LookupType = 7 の場合

		if(type == 7)
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

	printf("---- LookupList ----\n");

	//LookupList

	cnt = font_read16(p);

	for(i = 0; i < cnt; i++)
	{
		offset = font_read16(p);

		printf("\n==== [%d] offset:%d | ", i, offset);

		if(g_lookup_tag[i])
			font_put_tag_str(g_lookup_tag[i]);

		printf(" ====\n");

		//Lookup

		font_save_pos(p);
		font_seek_from_table(p, offset_lookuplist + offset);

		put_lookup(p, offset_lookuplist + offset);
		
		font_load_pos(p);
	}
}

/* FeatureList テーブル読み込み */

void read_feature_list(Font *p)
{
	uint32_t offset_featurelist,tag;
	uint16_t feature_cnt,cnt,offset,index;
	int i;

	//FeatureList へ

	offset_featurelist = p->offset_dat[1];

	font_seek_from_table(p, offset_featurelist);

	//FeatureList

	feature_cnt = font_read16(p);

	for(i = 0; i < feature_cnt; i++)
	{
		//FeatureRecord

		tag = font_read32(p);
		offset = font_read16(p);

		//Feature

		font_save_pos(p);
		font_seek_from_table(p, offset_featurelist + offset);

		font_seek_cur(p, 2);
		cnt = font_read16(p);

		for(; cnt > 0; cnt--)
		{
			index = font_read16(p);

			if(index < g_lookup_num && g_lookup_tag[index] == 0)
				g_lookup_tag[index] = tag;
		}

		font_load_pos(p);
	}
}

/* 作業用データ準備 */

void init(Font *p)
{
	//LookupList のデータ数取得

	font_seek_from_table(p, p->offset_dat[2]);

	g_lookup_num = font_read16(p);

	//メモリ確保

	g_lookup_tag = (uint32_t *)calloc(1, g_lookup_num * 4);
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

	//GSUB

	if(font_goto_GSUB(p) == 0)
	{
		init(p);
		
		read_feature_list(p);
		put_lookup_list(p);

		free(g_lookup_tag);
	}

	font_close(p);
	
	return 0;
}
