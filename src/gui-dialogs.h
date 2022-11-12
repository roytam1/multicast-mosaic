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

mo_status mo_post_save_window (mo_window *);
/* called from libwww */
void rename_binary_file (char *);
mo_status mo_post_open_local_window (mo_window *);
mo_status mo_post_open_window (mo_window *);
#ifdef HAVE_DTM
mo_status mo_send_document_over_dtm (mo_window *);
mo_status mo_post_dtmout_window (mo_window *);
#endif
mo_status mo_post_mail_window (mo_window *);
mo_status mo_post_print_window (mo_window *);
mo_status mo_post_source_window (mo_window *);
mo_status mo_post_search_window (mo_window *);
mo_status mo_post_subscribe_win (mo_window *);

int pathEval(char *dest, char *src);

#endif
