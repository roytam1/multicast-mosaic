
void MMCacheInit( char * root_dirname);
int MMCacheFetchCachedData(char * url , char ** fname_ret);
int MMCachePutDataInCache(char * data, int len, char * url, char ** fname_ret);
int MMCacheGetDataAndCache( char * url, char ** fname_ret, int * ldata_ret, mo_window * win);
void MMCacheClearCache(void);
void MMCacheWriteCache(void );

