

#define NO_ENCODING		0
#define GZIP_ENCODING		1
#define	COMPRESS_ENCODING	2

#define CACHE_CONTROL_NONE	0
#define CACHE_CONTROL_NO_STORE	 (1)
#define CACHE_CONTROL_NO_CACHE	(1<<1)
#define CACHE_CONTROL_PUBLIC	(1<<2)
#define CACHE_CONTROL_PRIVATE	(1<<3)


/* all mime field we understand */
typedef struct _MimeHeaderStruct {
	int content_encoding;
	int content_length;
	int status_code;
	int cache_control;
	char * content_type;
	char * expires;
	char * last_modified;
	char * location;

} MimeHeaderStruct;

extern const char * MMOSAIC_PRESENT;
extern void HTPresentationInit (void);
extern void HTExtensionMapInit (void);
extern char * MMct2Presentation( char * ct);
extern void FreeMimeStruct( MimeHeaderStruct *mhs);
extern void ParseMimeHeader(char * b, MimeHeaderStruct * mhs);

extern char *HTgeticonname(char *format, char *defaultformat);
extern char *HTFileName2ct(char *filename, char *default_type, int *compressed);
