CCMD := gcc -g

EXES := 03_tables 04_head 05_name 06_os2 07_maxp_post\
 08a_cmap_list 08b_cmap_tbl 09_hhea_hmtx 10_glyf 12_vhea_vmtx\
 13_vorg 15_cff 17_gsub_script 18_gsub_feature 19a_gsub_lookuplist\
 19b_gsub_type1 19c_gsub_glyph 21_gsub_type2 22_base 23a_gpos_list\
 23b_gpos_type1 24_gpos_type4

OBJS := fontfile.o ftlib.o image.o cff.o

OBJ1 := fontfile.o
OBJ2 := fontfile.o ftlib.o image.o

CCMD2 := $(CCMD) -I/usr/include/freetype2
LINK2 := -lfreetype

###

.PHONY: all clean

all: $(EXES)

clean:
	-rm -f $(EXES) *.o

ftlib.o: ftlib.c
	$(CCMD2) -c -o $@ $^

%.o: %.c
	$(CCMD) -c -o $@ $^

03_tables: 03_tables.c
	$(CCMD) -o $@ $^

04_head: 04_head.c $(OBJ1)
	$(CCMD) -o $@ $^

05_name: 05_name.c $(OBJ1)
	$(CCMD) -o $@ $^

06_os2: 06_os2.c $(OBJ1)
	$(CCMD) -o $@ $^

07_maxp_post: 07_maxp_post.c $(OBJ1)
	$(CCMD) -o $@ $^

08a_cmap_list: 08a_cmap_list.c $(OBJ1)
	$(CCMD) -o $@ $^

08b_cmap_tbl: 08b_cmap_tbl.c $(OBJ1)
	$(CCMD) -o $@ $^

09_hhea_hmtx: 09_hhea_hmtx.c $(OBJ1)
	$(CCMD) -o $@ $^

10_glyf: 10_glyf.c $(OBJ1)
	$(CCMD) -o $@ $^

12_vhea_vmtx: 12_vhea_vmtx.c $(OBJ1)
	$(CCMD) -o $@ $^

13_vorg: 13_vorg.c $(OBJ1)
	$(CCMD) -o $@ $^

15_cff: 15_cff.c $(OBJ1) cff.o
	$(CCMD) -o $@ $^

17_gsub_script: 17_gsub_script.c $(OBJ1)
	$(CCMD) -o $@ $^

18_gsub_feature: 18_gsub_feature.c $(OBJ1)
	$(CCMD) -o $@ $^

19a_gsub_lookuplist: 19a_gsub_lookuplist.c $(OBJ1)
	$(CCMD) -o $@ $^

19b_gsub_type1: 19b_gsub_type1.c $(OBJ1)
	$(CCMD) -o $@ $^

19c_gsub_glyph: 19c_gsub_glyph.c $(OBJ2)
	$(CCMD2) -o $@ $^ $(LINK2)

21_gsub_type2: 21_gsub_type2.c $(OBJ2)
	$(CCMD2) -o $@ $^ $(LINK2)

22_base: 22_base.c $(OBJ1)
	$(CCMD) -o $@ $^

23a_gpos_list: 23a_gpos_list.c $(OBJ1)
	$(CCMD) -o $@ $^

23b_gpos_type1: 23b_gpos_type1.c $(OBJ1)
	$(CCMD) -o $@ $^

24_gpos_type4: 24_gpos_type4.c $(OBJ1)
	$(CCMD) -o $@ $^


