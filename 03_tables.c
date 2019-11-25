/*******************************
 * テーブル一覧表示
 *
 * [usage] exe <files>
 *******************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>


/* 2byte 読み込み */

uint16_t read16(FILE *fp)
{
	uint8_t b[2];

	fread(b, 1, 2, fp);

	return (b[0] << 8) | b[1];
}

/* 4byte 読み込み */

uint32_t read32(FILE *fp)
{
	uint8_t b[4];

	fread(b, 1, 4, fp);

	return ((uint32_t)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

/* Tag を表示 */

void put_tag(uint32_t t)
{
	printf("'%c%c%c%c' (0x%08X)",
		t >> 24, (t >> 16) & 255, (t >> 8) & 255, t & 255,
		t);
}

/* オフセットテーブル */

void read_offset_table(FILE *fp)
{
	uint32_t ver,name,cksum,offset,len;
	uint16_t num,srange,esel,rshi;

	//バージョン
	ver = read32(fp);
	//テーブル数
	num = read16(fp);
	//searchRange
	srange = read16(fp);
	//entrySelector
	esel = read16(fp);
	//rangeShift
	rshi = read16(fp);

	printf("\n==== offset table ====\n");
	printf("version: 0x%08X | numTables: %d\n", ver, num);
	printf("searchRange: %d | entrySelector: %d | rangeShift: %d\n\n",
		srange, esel, rshi);

	printf(" tag                | checksum   | offset     | len\n");
	printf("-----------------------------------------------------------\n");

	//TableRecord

	for(; num > 0; num--)
	{
		//識別子
		name = read32(fp);
		//チェックサム
		cksum = read32(fp);
		//オフセット位置
		offset = read32(fp);
		//長さ
		len = read32(fp);

		put_tag(name);
		printf(" | 0x%08X | 0x%08X | %u\n", cksum, offset, len);
	}
}

/* TTC ヘッダ */

void read_ttc_header(FILE *fp)
{
	uint32_t i,htag,num,dsig_tag,dsig_len,dsig_offset;
	uint32_t *pfont_offset;
	uint16_t majver,minver;

	//ヘッダ
	htag = read32(fp);
	//メジャーバージョン
	majver = read16(fp);
	//マイナーバージョン
	minver = read16(fp);
	//フォント数
	num = read32(fp);

	printf("\n---- TTC header ----\n");
	printf("tag:");
	put_tag(htag);
	printf(" | version: %d.%d | font nums: %u\n",
		majver, minver, num);

	//各フォントのオフセット位置

	pfont_offset = (uint32_t *)malloc(4 * num);
	if(!pfont_offset) return;

	for(i = 0; i < num; i++)
		pfont_offset[i] = read32(fp);

	//ver 2.0

	if(majver == 2 && minver == 0)
	{
		//タグ
		dsig_tag = read32(fp);
		//長さ
		dsig_len = read32(fp);
		//オフセット
		dsig_offset = read32(fp);

		printf("[DSIG] tag:");
		put_tag(dsig_tag);
		printf(" | len: %u | offset: %u\n", dsig_len, dsig_offset);
	}

	//各フォント

	for(i = 0; i < num; i++)
	{
		fseek(fp, pfont_offset[i], SEEK_SET);

		read_offset_table(fp);
	}

	free(pfont_offset);
}

/* ファイル読み込み */

void read_file(char *filename)
{
	FILE *fp;
	uint32_t ver;

	printf("\n<<<< %s >>>>\n", filename);

	fp = fopen(filename, "rb");
	if(!fp)
	{
		printf("can't open '%s'\n", filename);
		return;
	}

	//先頭4byteで判定

	ver = read32(fp);
	rewind(fp);

	if(ver == 0x74746366)
		read_ttc_header(fp);
	else
		read_offset_table(fp);

	fclose(fp);
}

/* main */

int main(int argc,char **argv)
{
	int i;

	for(i = 1; i < argc; i++)
		read_file(argv[i]);
	
	return 0;
}
