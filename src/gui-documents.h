/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __GUIDOCUMENTS_H__
#define __GUIDOCUMENTS_H__

mo_status mo_back_impossible (mo_window *win);
mo_status mo_forward_impossible (mo_window *win);
mo_status mo_set_win_current_node (mo_window *, mo_node *);
mo_status mo_reload_window_text (mo_window *);
mo_status mo_refresh_window_text (mo_window *);
mo_status mo_load_window_text (mo_window *, char *, char *);
mo_status mo_duplicate_window_text (mo_window *, mo_window *);

mo_status mo_do_window_text (mo_window *win, char *url, char *txt,
                             int register_visit,
                             char *ref, char *last_modified, char *expires);
mo_status mo_post_access_document (mo_window *win, char *url,
                                   char *content_type, char *post_data);



#endif
