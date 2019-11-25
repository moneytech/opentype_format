#ifndef _FONTFILE_H_
#define _FONTFILE_H_

#define FONT_MAKE_TAG(a,b,c,d)  (((uint32_t)(a) << 24) | ((b) << 16) | ((c) << 8) | (d))

#define FONT_FILEPOS_NUM  5

typedef struct _Font Font;
typedef struct _TableItem TableItem;

/* テーブルの各アイテム */

struct _TableItem
{
	uint32_t name,offset;
};

/* Font */

struct _Font
{
	FILE *fp,			//フォントファイル
		*fp_output;		//出力テキストファイル

	int filepos_cur;	//現在のファイル位置記録の数
	fpos_t filepos[FONT_FILEPOS_NUM]; //一時的なファイル位置記録用

	uint32_t cur_table_offset;	//現在のテーブルのオフセット位置
	uint16_t offset_dat[3];		//各テーブル用のオフセットデータ
	
	TableItem *table;		//テーブルデータ。name = 0 でデータの終わり
	uint32_t *gind_table;	//グリフインデックス->Unicode テーブル (65536個の配列)

	int glyph_nums,		//フォント内のグリフ数 [maxp]
		units_per_em,	//1em のユニット数 [head]
		loca_format;	//loca のオフセットフォーマット (0:Offset16, 1:Offset32) [head]
};


/* function */

Font *font_open_file(char *filename);
int font_open_output_file(Font *p,char *filename);
void font_close(Font *p);

int font_goto_table(Font *p,uint32_t name);
int font_goto_GSUB(Font *p);
int font_goto_GPOS(Font *p);
void font_make_gindex_table(Font *p);
void font_read_table_maxp(Font *p);
void font_read_table_head(Font *p);
int font_cmap_get_glyph_id(Font *p,uint32_t code);

void font_output_coverage_list(Font *p);
void font_output_tag(Font *p,uint32_t t);
void font_output_utf16be_str(Font *p,int len);
void font_output_gid_to_uni(Font *p,uint16_t gid);
void font_output_gid_to_uni_char(Font *p,uint16_t gid);
void font_output_gid_rep(Font *p,uint16_t gid_src,uint16_t gid_dst);
void font_output_classDef(Font *p);
void font_output_ValueRecord(Font *p,uint16_t flags,int last_enter);

void font_seek_cur(Font *p,int n);
void font_seek(Font *p,uint32_t pos);
void font_seek_from_table(Font *p,uint32_t pos);

void font_save_pos(Font *p);
void font_load_pos(Font *p);

uint8_t font_read8(Font *p);
uint16_t font_read16(Font *p);
uint32_t font_read32(Font *p);
int64_t font_read64(Font *p);

void font_printf(Font *p,const char *format,...);
void font_fprintf(Font *p,FILE *fp,const char *format,...);

//

uint16_t readbuf16(uint8_t *buf);
uint32_t readbuf32(uint8_t *buf);

int get_ValueRecord_size(uint16_t flags);

void font_put_tag(uint32_t t);
void font_put_tag_str(uint32_t t);

void put_bits(uint32_t val,int cnt);
void put_uni_to_utf8(FILE *fp,uint32_t c);

#endif
