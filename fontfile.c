/***************************
 * フォントファイル処理
 **************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdarg.h>

#include "fontfile.h"


//=============================
// 基本処理
//=============================


/* オフセットテーブルを読み込み */

static void _read_offset_table(Font *p)
{
	TableItem *ti;
	int num;
	
	//バージョン
	font_read32(p);
	//テーブル数
	num = font_read16(p);
	//skip
	font_seek_cur(p, 2 * 3);

	//データ確保

	ti = (TableItem *)malloc(sizeof(TableItem) * (num + 1));
	if(!ti) return;

	p->table = ti;

	//TableRecord

	for(; num > 0; num--, ti++)
	{
		//識別子
		ti->name = font_read32(p);
		//チェックサム
		font_seek_cur(p, 4);
		//オフセット位置
		ti->offset = font_read32(p);
		//長さ
		font_seek_cur(p, 4);
	}

	//データ終了

	ti->name = ti->offset = 0;
}

/* フォント閉じる*/

void font_close(Font *p)
{
	if(p)
	{
		if(p->gind_table)
			free(p->gind_table);
	
		if(p->table)
			free(p->table);

		if(p->fp_output)
			fclose(p->fp_output);
	
		fclose(p->fp);
	}
}

/* フォントファイルを開く
 *
 * フォントコレクションの場合は、1番目のフォント。
 *
 * return: NULL で失敗 */

Font *font_open_file(char *filename)
{
	Font *p;
	FILE *fp;
	uint32_t ver;

	//ファイル開く

	fp = fopen(filename, "rb");
	if(!fp)
	{
		printf("can't open '%s'\n", filename);
		return NULL;
	}

	//Font 確保

	p = (Font *)calloc(1, sizeof(Font));
	if(!p)
	{
		fclose(fp);
		return NULL;
	}

	p->fp = fp;

	//オフセットテーブルへ

	ver = font_read32(p);

	if(ver == 0x74746366)
	{
		//[フォントコレクション]
		
		//バージョン・フォント数をスキップ
		font_seek_cur(p, 8);

		//1番目のフォントのオフセット位置へ
		font_seek(p, font_read32(p));
	}
	else
		//ファイル位置クリア
		rewind(p->fp);

	//オフセットテーブル読み込み
	
	_read_offset_table(p);

	return p;
}

/* 出力用のテキストファイルを開く */

int font_open_output_file(Font *p,char *filename)
{
	p->fp_output = fopen(filename, "wt");

	if(!p->fp_output)
	{
		printf("! can't open '%s'\n", filename);
		return 1;
	}

	return 0;
}

/* 指定テーブルの位置へ
 *
 * return: 0 で成功、1 で見つからなかった */

int font_goto_table(Font *p,uint32_t name)
{
	TableItem *ti;

	for(ti = p->table; ti->name; ti++)
	{
		if(ti->name == name)
		{
			p->cur_table_offset = ti->offset;
			font_seek(p, ti->offset);
			return 0;
		}
	}

	return 1;
}

/* GSUB テーブルへ移動し、先頭データ読み込み
 *
 * offset_dat に各テーブルのオフセット位置が入る。
 *
 * return: 0 で成功、1 で失敗 */

int font_goto_GSUB(Font *p)
{
	uint16_t majver,minver;

	if(font_goto_table(p, FONT_MAKE_TAG('G','S','U','B')))
	{
		printf("not found 'GSUB'\n");
		return 1;
	}

	majver = font_read16(p);
	minver = font_read16(p);

	p->offset_dat[0] = font_read16(p);
	p->offset_dat[1] = font_read16(p);
	p->offset_dat[2] = font_read16(p);

	printf("---- GSUB ----\n\n");
	printf("version: %d.%d\n", majver, minver);
	printf("scriptListOffset: %d\n", p->offset_dat[0]);
	printf("featureListOffset: %d\n", p->offset_dat[1]);
	printf("lookupListOffset: %d\n\n", p->offset_dat[2]);

	return 0;
}

/* GPOS テーブルへ移動し、先頭データ読み込み
 *
 * offset_dat に各テーブルのオフセット位置が入る。
 *
 * return: 0 で成功、1 で失敗 */

int font_goto_GPOS(Font *p)
{
	uint16_t majver,minver;

	if(font_goto_table(p, FONT_MAKE_TAG('G','P','O','S')))
	{
		printf("not found 'GPOS'\n");
		return 1;
	}

	majver = font_read16(p);
	minver = font_read16(p);

	p->offset_dat[0] = font_read16(p);
	p->offset_dat[1] = font_read16(p);
	p->offset_dat[2] = font_read16(p);

	printf("---- GPOS ----\n\n");
	printf("version: %d.%d\n", majver, minver);
	printf("scriptListOffset: %d\n", p->offset_dat[0]);
	printf("featureListOffset: %d\n", p->offset_dat[1]);
	printf("lookupListOffset: %d\n\n", p->offset_dat[2]);

	return 0;
}

/* head テーブルを読み込み
 *
 * units_per_em, loca_format */

void font_read_table_head(Font *p)
{
	if(font_goto_table(p, FONT_MAKE_TAG('h','e','a','d')))
		return;

	font_seek_cur(p, 18);
	p->units_per_em = font_read16(p);

	font_seek_cur(p, 30);
	p->loca_format = (int16_t)font_read16(p);
}

/* maxp テーブルを読み込み
 *
 * glyph_nums */

void font_read_table_maxp(Font *p)
{
	if(font_goto_table(p, FONT_MAKE_TAG('m','a','x','p')))
		return;

	//version
	font_seek_cur(p, 4);
	//glyphNums
	p->glyph_nums = font_read16(p);
}


//=============================
// cmap
//=============================


/* format 4 : Unicode BMP */

static void _read_cmap_format4(Font *p)
{
	uint8_t *dat,*pend,*pstart,*pdelta,*poffset;
	uint16_t end,start,rangeoffset;
	int16_t delta;
	uint32_t *ptable;
	int i,c,gind,len,segcnt;

	//format
	if(font_read16(p) != 4) return;
	//len
	len = font_read16(p);
	//language
	font_seek_cur(p, 2);
	//segCountX2
	segcnt = font_read16(p) / 2;
	//skip
	font_seek_cur(p, 2 * 3);

	//endCode 以降のデータを読み込み

	len -= 2 * 7;

	dat = (uint8_t *)malloc(len);
	if(!dat) return;

	fread(dat, 1, len, p->fp);

	pend = dat;
	pstart = dat + segcnt * 2 + 2;
	pdelta = pstart + segcnt * 2;
	poffset = pdelta + segcnt * 2;

	ptable = p->gind_table;

	//

	for(i = 0; i < segcnt; i++)
	{
		//endCode
		end = readbuf16(pend);
		//startCode
		start = readbuf16(pstart);
		//idDelta
		delta = (int16_t)readbuf16(pdelta);
		//idRangeOffset
		rangeoffset = readbuf16(poffset);

		//Unicode:start - end までのグリフインデックスを算出し、セット

		for(c = start; c <= end; c++)
		{
			if(rangeoffset == 0)
				gind = (c + delta) & 0xffff;
			else
				gind = readbuf16(poffset + rangeoffset + (c - start) * 2);

			if(gind)
				ptable[gind] = c;
		}

		//次のセグメントへ

		pend += 2;
		pstart += 2;
		pdelta += 2;
		poffset += 2;
	}

	free(dat);
}

/* format 12 : Unicode full */

static void _read_cmap_format12(Font *p)
{
	uint32_t num,start,end,gid,c,*ptable;

	//format
	if(font_read16(p) != 12) return;
	//skip
	font_seek_cur(p, 2 + 4 + 4);
	//グループ数
	num = font_read32(p);

	ptable = p->gind_table;

	for(; num > 0; num--)
	{
		start = font_read32(p);
		end = font_read32(p);
		gid = font_read32(p);

		for(c = start; c <= end; c++)
			ptable[c - start + gid] = c;
	}
}

/* グリフインデックス->Unicode 変換テーブルの生成 */

void font_make_gindex_table(Font *p)
{
	uint32_t *pd,offset;
	uint16_t ver,num,platform,encoding,flags = 0;

	//cmap テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('c','m','a','p')))
		return;

	//確保

	pd = (uint32_t *)calloc(1, 4 * 65536);
	if(!pd) return;

	p->gind_table = pd;

	//cmap テーブル読み込み

	ver = font_read16(p);
	num = font_read16(p);

	if(ver != 0) return;

	for(; num > 0; num--)
	{
		platform = font_read16(p);
		encoding = font_read16(p);
		offset = font_read32(p);

		//Unicode BMP

		if(!(flags & 1)
			&& ((platform == 0 && encoding == 3)
				|| (platform == 3 && encoding == 1)))
		{
			font_save_pos(p);
			font_seek_from_table(p, offset);
			
			_read_cmap_format4(p);

			font_load_pos(p);
			
			flags |= 1;
		}

		//Unicode full

		if(!(flags & 2)
			&& ((platform == 0 && encoding == 4)
				|| (platform == 3 && encoding == 10)))
		{
			font_save_pos(p);
			font_seek_from_table(p, offset);
			
			_read_cmap_format12(p);

			font_load_pos(p);

			flags |= 2;
		}
	}
}

/* cmap を使って、Unicode -> グリフID
 *
 * return: 見つからなかったら 0 */

int font_cmap_get_glyph_id(Font *p,uint32_t code)
{
	uint16_t num1,plat,enc,end,st,range;
	int16_t delta;
	uint32_t offset;
	int i,segcnt,seek_segcnt;

	//cmap テーブルへ

	if(font_goto_table(p, FONT_MAKE_TAG('c','m','a','p')))
		return 0;

	//version

	if(font_read16(p) != 0) return 0;

	//Unicode BMP を見つける

	num1 = font_read16(p);

	for(; num1 > 0; num1--)
	{
		plat = font_read16(p);
		enc = font_read16(p);
		offset = font_read32(p);

		if((plat == 0 && enc == 3) || (plat == 3 && enc == 1))
		{
			font_seek_from_table(p, offset);
			break;
		}
	}

	if(num1 == 0) return 0;

	//format

	if(font_read16(p) != 4) return 0;

	font_seek_cur(p, 4);
	segcnt = font_read16(p) / 2;
	font_seek_cur(p, 6);

	//endCode

	for(i = 0; i < segcnt; i++)
	{
		end = font_read16(p);
		if(end == 0xffff) return 0;
		if(code <= end) break;
	}

	if(i == segcnt) return 0;

	seek_segcnt = (segcnt - 1) * 2;

	//startCode

	font_seek_cur(p, seek_segcnt + 2);

	st = font_read16(p);

	if(code < st) return 0;

	//idDelta

	font_seek_cur(p, seek_segcnt);

	delta = (int16_t)font_read16(p);

	//idRangeOffset

	font_seek_cur(p, seek_segcnt);

	range = font_read16(p);

	//

	if(range == 0)
		//delta
		return (code + delta) & 0xffff;
	else
	{
		font_seek_cur(p, -2 + range + (code - st) * 2);
		return font_read16(p);
	}
}


//=============================
// テキストに出力
//=============================


/* Coverage テーブルをリスト化して出力
 * (対象となるグリフのリスト) */

void font_output_coverage_list(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t format,num,first,end;

	format = font_read16(p);
	num = font_read16(p);

	fprintf(fp, "<format:%d, count:%d>\n", format, num);

	if(format == 1)
	{
		for(; num > 0; num--)
		{
			font_output_gid_to_uni(p, font_read16(p));
			fputc('\n', fp);
		}
	}
	else if(format == 2)
	{
		for(; num > 0; num--)
		{
			first = font_read16(p);
			end = font_read16(p);
			fprintf(fp, "<startCoverageIndex: %d>\n", font_read16(p));

			for(; first <= end; first++)
			{
				font_output_gid_to_uni(p, first);
				fputc('\n', fp);
			}
		}
	}
}

/* タグを文字列としてテキスト出力 */

void font_output_tag(Font *p,uint32_t t)
{
	fprintf(p->fp_output, "'%c%c%c%c'",
		t >> 24, (t >> 16) & 255, (t >> 8) & 255, t & 255);
}

/* UTF-16BE の文字列を読み込んで、テキストに出力 */

void font_output_utf16be_str(Font *p,int len)
{
	uint32_t c,c2;

	for(; len > 0; len -= 2)
	{
		//UTF-16BE -> UTF-32
		
		c = font_read16(p);
		
		if(c >= 0x10000)
		{
			c2 = font_read16(p);
			c = (((c & 0x03ff) << 16) | (c2 & 0x03ff)) + 0x10000;
		}

		//UTF-8

		put_uni_to_utf8(p->fp_output, c);
	}

	fputc('\n', p->fp_output);
}

/* テーブルを使って、GID -> Unicode し、テキスト出力 */

void font_output_gid_to_uni(Font *p,uint16_t gid)
{
	FILE *fp = p->fp_output;
	uint32_t c;

	c = p->gind_table[gid];

	fprintf(fp, "[%d] ", gid);

	if(c == 0)
		//対応する Unicode がない
		fputs("(none)", fp);
	else
	{
		fputc('(', fp);
		put_uni_to_utf8(fp, c);
		fprintf(fp, ") U+%04X", c);
	}	
}

/* GID -> Unicode し、テキスト出力 (GID は出力しない) */

void font_output_gid_to_uni_char(Font *p,uint16_t gid)
{
	FILE *fp = p->fp_output;
	uint32_t c;

	c = p->gind_table[gid];

	if(c == 0)
		fprintf(fp, "(none)");
	else
	{
		put_uni_to_utf8(fp, c);

		fprintf(fp, " (U+%04X)", c);
	}	
}

/* GID の置き換えを、テキスト出力 */

void font_output_gid_rep(Font *p,uint16_t gid_src,uint16_t gid_dst)
{
	font_output_gid_to_uni(p, gid_src);
	fputs(" => ", p->fp_output);
	font_output_gid_to_uni(p, gid_dst);
	fputs("\n", p->fp_output);
}

/* ClassDef テーブルのデータを出力 */

void font_output_classDef(Font *p)
{
	FILE *fp = p->fp_output;
	uint16_t format,gid,end,cnt,class;
	int i;

	format = font_read16(p);

	if(format == 1)
	{
		//gid 〜 gid + cnt -  1 の範囲
	
		gid = font_read16(p);
		cnt = font_read16(p);

		for(; cnt > 0; cnt--)
		{
			class = font_read16(p);

			font_output_gid_to_uni(p, gid++);
			fprintf(fp, " <class %d>\n", class);
		}
	}
	else if(format == 2)
	{
		//指定グリフ ID 範囲

		cnt = font_read16(p);

		for(; cnt > 0; cnt--)
		{
			gid = font_read16(p);
			end = font_read16(p);
			class = font_read16(p);

			for(i = gid; i <= end; i++)
			{
				font_output_gid_to_uni(p, i);
				fprintf(fp, " <class %d>\n", class);
			}
		}
	}
}

/* ValueRecord の値を出力
 *
 * flags : ValueRecord の値フラグ
 * last_enter : 最後に改行を出力 */

void font_output_ValueRecord(Font *p,uint16_t flags,int last_enter)
{
	FILE *fp = p->fp_output;

	if(flags == 0) return;

	fprintf(fp, "[");

	if(flags & 1)
		font_fprintf(p, fp, "xPlacement:$h, ");
	
	if(flags & 2)
		font_fprintf(p, fp, "yPlacement:$h, ");

	if(flags & 4)
		font_fprintf(p, fp, "xAdvance:$h, ");

	if(flags & 8)
		font_fprintf(p, fp, "yAdvance:$h, ");

	if(flags & 16)
		font_fprintf(p, fp, "xPlaDeviceOffset:$H, ");

	if(flags & 32)
		font_fprintf(p, fp, "yPlaDeviceOffset:$H, ");

	if(flags & 64)
		font_fprintf(p, fp, "xAdvDeviceOffset:$H, ");

	if(flags & 128)
		font_fprintf(p, fp, "xAdvDeviceOffset:$H, ");

	if(last_enter)
		fputs("]\n", fp);
	else
		fputc(']', fp);
}


//=============================
// 
//=============================


/* 現在位置からシーク */

void font_seek_cur(Font *p,int n)
{
	fseek(p->fp, n, SEEK_CUR);
}

/* ファイル先頭から指定位置へシーク */

void font_seek(Font *p,uint32_t pos)
{
	fseek(p->fp, pos, SEEK_SET);
}

/* 現在のテーブル位置からのオフセット位置へシーク */

void font_seek_from_table(Font *p,uint32_t pos)
{
	fseek(p->fp, p->cur_table_offset + pos, SEEK_SET);
}

/* 現在のファイル位置を記録 */

void font_save_pos(Font *p)
{
	if(p->filepos_cur < FONT_FILEPOS_NUM)
	{
		fgetpos(p->fp, p->filepos + p->filepos_cur);
		p->filepos_cur++;
	}
}

/* 記録した一つ前のファイル位置に戻る */

void font_load_pos(Font *p)
{
	if(p->filepos_cur > 0)
	{
		p->filepos_cur--;
		fsetpos(p->fp, p->filepos + p->filepos_cur);
	}
}

/* 1byte 読み込み */

uint8_t font_read8(Font *p)
{
	uint8_t b;

	fread(&b, 1, 1, p->fp);

	return b;
}

/* 2byte 読み込み */

uint16_t font_read16(Font *p)
{
	uint8_t b[2];

	fread(b, 1, 2, p->fp);

	return (b[0] << 8) | b[1];
}

/* 4byte 読み込み */

uint32_t font_read32(Font *p)
{
	uint8_t b[4];

	fread(b, 1, 4, p->fp);

	return ((uint32_t)b[0] << 24) | (b[1] << 16) | (b[2] << 8) | b[3];
}

/* 8byte 読み込み */

int64_t font_read64(Font *p)
{
	int64_t ret = 0;
	uint8_t b[8];
	int i;

	fread(b, 1, 8, p->fp);

	for(i = 0; i < 8; i++)
		ret |= (int64_t)b[i] << ((7 - i) << 3);

	return ret;
}


//========================


/* fprintf 実体 */

#define _FORM_F_SET  1
#define _FORM_F_X    2
#define _FORM_F_BITS 4

static void _font_fprintf(Font *p,FILE *fp,const char *format,va_list ap)
{
	char c;
	uint8_t flags;
	uint16_t u16,*pu16;
	int16_t i16,*pi16;
	uint32_t u32,*pu32;

	while(1)
	{
		c = *(format++);
		if(c == 0) break;

		if(c != '$')
			fputc(c, fp);
		else
		{
			c = *(format++);
			if(c == 0) break;

			//オプション

			flags = 0;

			while(1)
			{
				if(c == 'x')
					flags |= _FORM_F_X;
				else if(c == '+')
					flags |= _FORM_F_SET;
				else if(c == 'b')
					flags |= _FORM_F_BITS;
				else
					break;

				c = *(format++);
				if(c == 0) return;
			}

			//

			switch(c)
			{
				//uint16
				case 'H':
					u16 = font_read16(p);

					if(flags & _FORM_F_BITS)
						put_bits(u16, 16);
					else if(flags & _FORM_F_X)
						fprintf(fp, "0x%04X", u16);
					else
						fprintf(fp, "%d", u16);

					if(flags & _FORM_F_SET)
					{
						pu16 = va_arg(ap, uint16_t *);
						*pu16 = u16;
					}
					break;
				//int16
				case 'h':
					i16 = (int16_t)font_read16(p);
					fprintf(fp, "%d", i16);

					if(flags & _FORM_F_SET)
					{
						pi16 = va_arg(ap, int16_t *);
						*pi16 = i16;
					}
					break;
				//uint32
				case 'I':
					u32 = font_read32(p);

					if(flags & _FORM_F_X)
						fprintf(fp, "0x%08X", u32);
					else
						fprintf(fp, "%u", u32);

					if(flags & _FORM_F_SET)
					{
						pu32 = va_arg(ap, uint32_t *);
						*pu32 = u32;
					}
					break;
				//tag
				case 't':
					u32 = font_read32(p);
					
					font_put_tag_str(u32);

					if(flags & _FORM_F_SET)
					{
						pu32 = va_arg(ap, uint32_t *);
						*pu32 = u32;
					}
					break;
			}
		}
	}
}

/* printf */

void font_printf(Font *p,const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	
	_font_fprintf(p, stdout, format, ap);

	va_end(ap);
}

/* fprintf
 *
 * $[options][type]
 *
 * <type>
 * h : int16
 * H : uint16
 * I : uint32
 * t : tag
 *
 * <option>
 * + : 引数のポインタに読み込んだ値を代入
 * x : 16進数で表示
 * b : 2進数で表示 */

void font_fprintf(Font *p,FILE *fp,const char *format,...)
{
	va_list ap;

	va_start(ap, format);
	
	_font_fprintf(p, fp, format, ap);

	va_end(ap);
}


//========================


/* メモリから 2byte 読み込み */

uint16_t readbuf16(uint8_t *buf)
{
	return (buf[0] << 8) | buf[1];
}

/* メモリから 4byte 読み込み */

uint32_t readbuf32(uint8_t *buf)
{
	return ((uint32_t)buf[0] << 24) | (buf[1] << 16) | (buf[2] << 8) | buf[3];
}

/* [GPOS] ValueRecord のフラグから、ValueRecord のサイズ取得 */

int get_ValueRecord_size(uint16_t flags)
{
	int size = 0;

	//ビットが ON の数だけ +2

	for(; flags; flags >>= 1)
	{
		if(flags & 1)
			size += 2;
	}

	return size;
}

/* Tag の値を表示 (数値付き) */

void font_put_tag(uint32_t t)
{
	if(t)
	{
		printf("'%c%c%c%c' (0x%08X)",
			t >> 24, (t >> 16) & 255, (t >> 8) & 255, t & 255,
			t);
	}
}

/* Tag の値を表示 (文字列のみ) */

void font_put_tag_str(uint32_t t)
{
	if(t)
	{
		printf("'%c%c%c%c'",
			t >> 24, (t >> 16) & 255, (t >> 8) & 255, t & 255);
	}
}

/* ビット値を文字列出力 */

void put_bits(uint32_t val,int cnt)
{
	uint32_t f;
	int pos;

	pos = cnt - 1;
	f = 1 << pos;

	for(; f > 0; f >>= 1, pos--)
	{
		putchar((val & f)? '1': '0');

		if((pos & 3) == 0 && pos != 0)
			putchar('_');
	}

	printf("b");
}

/* Unicode 1文字 -> UTF8 出力 */

void put_uni_to_utf8(FILE *fp,uint32_t c)
{
	uint8_t b[4];
	int len = 0;

	if(c < 0x80)
	{
		b[0] = (uint8_t)c;
		len = 1;
	}
	else if(c <= 0x7ff)
	{
		b[0] = 0xc0 | (c >> 6);
		b[1] = 0x80 | (c & 0x3f);
		
		len = 2;
	}
	else if(c <= 0xffff)
	{
		b[0] = 0xe0 | (c >> 12);
		b[1] = 0x80 | ((c >> 6) & 0x3f);
		b[2] = 0x80 | (c & 0x3f);
		
		len = 3;
	}
	else if(c <= 0x1fffff)
	{
		b[0] = 0xf0 | (c >> 18);
		b[1] = 0x80 | ((c >> 12) & 0x3f);
		b[2] = 0x80 | ((c >> 6) & 0x3f);
		b[3] = 0x80 | (c & 0x3f);

		len = 4;
	}

	if(len)
		fwrite(b, 1, len, fp);
}
