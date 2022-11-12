/*             FTP access module for libwww
                                   FTP ACCESS FUNCTIONS

   This isn't really a valid protocol module -- it is lumped together
   with HTFile . That could be changed easily.

   Author: Tim Berners-Lee. Public Domain. Please mail changes to
   timbl@info.cern.ch

 */
#ifndef HTFTP_H
#define HTFTP_H

#include "HTUtils.h"
#include "HTAnchor.h"
#include "HTStream.h"
#include "HTAlert.h"

/* Retrieve File from Server
  ON EXIT,
  returns                 Socket number for file if good.<0 if bad.
*/
extern int HTFTPLoad (char *name, HTParentAnchor *anchor,
	HTFormat format_out, HTStream *sink, caddr_t appd);

/* Return Host Name */
extern WWW_CONST char * HTHostName NOPARAMS;

/*
 * NLST parameters -- SWP
 */
#define NLST_PARAMS "-Lla"

/* Send file to server */
extern int HTFTPSend ( char * name , caddr_t appd);
extern int HTFTPMkDir( char * name , caddr_t appd);
extern int HTFTPRemove( char * name , caddr_t appd);
extern void HTFTPClearCache (void);

#endif
