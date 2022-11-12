
/* all mime field we understand */

extern void read_http_doc_prim_fd_cb( XtPointer clid, int * fd, XtInputId * id);
extern char * BuildDocHTTPCommand(PafDocDataStruct * pafd, int * len_ret);

extern void  MMStopHTTPReadDoc(PafDocDataStruct * pafd);
extern void MMCancelHTTPReadDocOnError(PafDocDataStruct * pafd, char * msg);
