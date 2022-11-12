/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __GUIDIALOGS_H__
#define __GUIDIALOGS_H__

extern void  mo_save_document (Widget w, XtPointer clid, XtPointer calld);
extern void mo_document_source (Widget w, XtPointer clid, XtPointer calld);
/* called from libwww */
mo_status mo_post_open_local_window (mo_window *);
mo_status mo_post_open_window (mo_window *);
#ifdef HAVE_DTM
mo_status mo_send_document_over_dtm (mo_window *);
mo_status mo_post_dtmout_window (mo_window *);
#endif
mo_status mo_post_mail_window (mo_window *);
mo_status mo_post_print_window (mo_window *);
mo_status mo_post_search_window (mo_window *);
mo_status mo_post_subscribe_win (mo_window *);

int pathEval(char *dest, char *src);

#endif
