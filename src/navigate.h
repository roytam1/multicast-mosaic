/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __HISTORY_H__
#define __HISTORY_H__

mo_status mo_add_node_to_history (mo_window *, mo_node *);
extern void mo_back (Widget w, XtPointer clid, XtPointer calld);
extern void mo_forward (Widget w, XtPointer clid, XtPointer calld);
extern void mo_post_history_win (mo_window *);
extern void mo_delete_rbm_history_win(mo_window *win);

extern void mo_free_node_data (mo_node *);
extern void mo_back_impossible(mo_window *);
extern void mo_back_possible(mo_window *);
extern void mo_forward_impossible (mo_window *);
extern void mo_forward_possible (mo_window *win);
extern void MMUpdNavigationOnNewURL( mo_window *win, char * aurl_wa, char *aurl,
        char *goto_anchor, char *text, MimeHeaderStruct *mhs, int docid,
        HtmlTextInfo * htinfo);
extern void mo_kill_node_descendents_frame(mo_window *win, mo_node *node,
        mo_node ** ret_next);
extern void mo_kill_node_descendents(mo_window *win, mo_node *node);

#endif
