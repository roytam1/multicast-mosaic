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

mo_status mo_set_win_current_node (mo_window *, mo_node *);
extern void mo_reload_document(Widget w, XtPointer clid, XtPointer calld);
mo_status mo_refresh_window_text (mo_window *);
void mo_set_win_headers (mo_window *win, char* aurl_wa);


#endif
