/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __HISTORY_H__
#define __HISTORY_H__

mo_status mo_add_node_to_history (mo_window *, mo_node *);
char *mo_grok_title (mo_window *, char *, char *);
extern void mo_back (Widget w, XtPointer clid, XtPointer calld);
extern void mo_forward (Widget w, XtPointer clid, XtPointer calld);
mo_status mo_visit_position (mo_window *, int);
mo_status mo_post_history_win (mo_window *);

extern void mo_free_node_data (mo_node *);
extern void mo_back_impossible(mo_window *);
extern void mo_back_possible(mo_window *);
extern void mo_forward_impossible (mo_window *);
extern void mo_forward_possible (mo_window *win);
extern void MMUpdNavigationOnNewURL(mo_window *win, char *aurl_wa, char *aurl,
	char *goto_anchor, char *base_url, char *title, char *text,
	MimeHeaderStruct * mhs, int docid, struct mark_up * mlist);

#endif
