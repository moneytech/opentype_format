
typedef struct _Image Image;

void ftlib_free(void);
int ftlib_init(const char *filename,int pxsize);

int ftlib_getHeight(void);
int ftlib_getCharWidth_GID(int gid);

int ftlib_drawChar_GID(Image *img,int x,int y,int gid,uint32_t col);
