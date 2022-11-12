
void MMCacheInit( char * root_dirname);
int MMCacheFindData(char *aurl_wa , char *aurl, int fdw, MimeHeaderStruct *mhs);
void MMCachePutDataInCache(char *fname_r, char *aurl_wa, char *aurl,
	MimeHeaderStruct * mhs);
void MMCacheClearCache(void);
void MMCacheWriteCache(void );

