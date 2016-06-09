#define TSD_PATH_ERROR		-1
#define TSD_PATH_NOTEXIST	0
#define TSD_PATH_ISFILE		1
#define TSD_PATH_ISDIR		2
#define TSD_PATH_OTHER		3

int path_join(TSDCHAR *dst, const TSDCHAR *base, const TSDCHAR *addition);
int path_getdir(TSDCHAR *dst, const TSDCHAR *path);
TSDCHAR* path_getfile(const TSDCHAR *path);
TSDCHAR* path_getext(const TSDCHAR *path);
void path_removeext(TSDCHAR *path);
int path_addext(TSDCHAR *path, const TSDCHAR *ext);
int path_isexist(const TSDCHAR *path);
int path_isdir(const TSDCHAR *path);
int path_isfile(const TSDCHAR *path);
