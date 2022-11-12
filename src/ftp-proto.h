
extern void MMCancelFTPReadDocOnStop(PafDocDataStruct *pafd);
extern void MMCancelFTPReadDocOnError(PafDocDataStruct *pafd, char * reason);
extern void read_ftp_doc_prim_fd_cb( XtPointer clid, int * fd, XtInputId * id);
