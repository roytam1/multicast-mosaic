/* Author : Gilles Dauphin 13 Oct 1997 */

#include <netinet/in.h>
#include <arpa/inet.h>

#ifdef IPV6
typedef struct sockaddr_in6 SockA;  /* See netinet/in.h */
#else
typedef struct sockaddr_in SockA;
#endif

#define FILE_CON_TYPE 0
#define HTTP_CON_TYPE 1
#define FTP_CON_TYPE 2
#define NNTP_CON_TYPE 3
#define GOPHER_CON_TYPE 4

typedef struct _EmbeddedObjectEntry {
	char * url;
	struct mark_up * mark;
	int num;
} EmbeddedObjectEntry;


typedef struct _SendParamsStruct {
	char *accept;
} SendParamsStruct;

typedef struct _IOBStruct {
	char * iobuf;		/* the io buffer */
	int len_iobuf;		/* len of iobuf = number of char in iobuf*/
	int size_iobuf;		/* size of malloced iobuf */
} IOBStruct;

typedef struct _WWWConType {
	int prim_fd;		/* fd of prim channel: generaly read */
	int second_fd;		/* fd of 2nd socket : example FTP (listen soc) */
	int third_fd;		/* fd of data_soc for FTP */
	SockA sin;
	void (*call_me_on_stop_cb)(struct _PafDocDataStruct *);
	void (*call_me_on_error_cb)(struct _PafDocDataStruct *, char *reason);
} WWWConType;

typedef struct _PafDocDataStruct {
	mo_window * win;	/* the mo_window making the request */
	void (*call_me_on_succes)(struct _PafDocDataStruct *);
	void (*call_me_on_error)(struct _PafDocDataStruct *, char *reason);
	void (*call_me_on_stop)(struct _PafDocDataStruct *);
	char * fname;		/* the name of the file to write */
	int fd;			/* file des. to write */
	char *aurl;		/* an Absolute url to get */
	char *aurl_wa;		/* same but with anchor */
				/* the original aurl request */
	char *goto_anchor;	/* #target in url */
	char * post_ct;		/* POST Content-Type */
	char * post_data;	/* data to post, if any */
	SendParamsStruct sps;	/* Some parameter for sending */
	IOBStruct iobs;

	int total_read_data;	/* number of data byte read */
                                /* in some case remplace the Content-length */ 
	MimeHeaderStruct *mhs;	/* all field analysed by MIME parser
				 * when the server respond. This info is
				 * assiocated with the received data */
	char * html_text;	/* the text we have read in memory */
	struct mark_up * mlist; /* Parsed part of html_text */
	EmbeddedObjectEntry * embedded_object_tab;
				/* A table of selected embedded object */
				/* need for retrieve all embedded object */
				/* in the html text */
	int num_of_eo;		/* number of embedded object to retrieve */
	int cur_processing_eo;	/* the numero of object to retrieve */
	struct _PafDocDataStruct * parent_paf; /* a child have a parent */
	struct _PafDocDataStruct *paf_child;
				/* a PAF Struct to get ONE/ONE html embedded */
				/* object */
				/* an embedded object inherit of some paf's */
				/* parent struct */
	TwirlStruct * twirl_struct;
/*####*/
	int read_stat;		/* state of read */

	char * lfcrlf_type;	/* kind of EOL : LF or CRLF */
				/* detect from status line */

	char * format_in;
	int pragma_no_cache;	/* no-cache request */
	int n_redirect;		/* number of redirect code 3xx */

	int con_type;		/* type of connection: HTTP FTP ... */
	WWWConType * www_con_type; /* see above */
	struct Proxy * proxent;	/* if !NULL get from proxy host*/
	int http_status;	/* status return by a HTTP connection */
	XtIntervalId cancel_connect_time_out_id; /* timeout id for cancel */
	XtIntervalId loop_connect_time_out_id;    /* loop for connect */
/*	XtInputId connect_succes_input_id; */
	XtInputId www_prim_fd_read_id;
	XtInputId www_third_fd_read_id;
} PafDocDataStruct;

extern void MMPafSaveData(Widget top, char * aurl, char * fname);
extern void MMPafLoadHTMLDocInWin( mo_window * win, RequestDataStruct * rds);
extern void MMFinishPafSaveData(PafDocDataStruct * pafd);
extern void MMFinishPafDocData(PafDocDataStruct * pafd);
