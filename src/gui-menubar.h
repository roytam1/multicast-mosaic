/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __GUIMENUBAR_H__
#define __GUIMENUBAR_H__

extern void mo_home_document(Widget w, XtPointer clid, XtPointer calld);
extern void mo_open_document(Widget w, XtPointer clid, XtPointer calld);
extern void mo_new_window(Widget w, XtPointer clid, XtPointer calld);
extern void mo_clone_window(Widget w, XtPointer clid, XtPointer calld);
extern void mo_close_window(Widget w, XtPointer clid, XtPointer calld);
extern void mo_register_node_in_default_hotlist(Widget w,XtPointer clid, XtPointer calld);
extern void mo_mail_document(Widget w, XtPointer clid, XtPointer calld);
extern void mo_print_document(Widget w, XtPointer clid, XtPointer calld);
extern void mo_delay_object_loads(Widget w, XtPointer clid, XtPointer calld);
extern void mo_expand_object_current(Widget w, XtPointer clid, XtPointer calld);
extern void mo_body_color(Widget w, XtPointer clid, XtPointer calld);
extern void mo_search(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_groups(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_index(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_prevt(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_prev(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_next(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_nextt(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_post(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_follow(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_fmt0(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_fmt1(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_grp0(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_grp1(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_grp2(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_art0(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_art1(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_sub(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_unsub(Widget w, XtPointer clid, XtPointer calld);
extern void mo_news_mread(Widget w, XtPointer clid, XtPointer calld);


mo_status mo_set_fonts (mo_window *, int);
mo_status mo_set_underlines (mo_window *, int);
XmxMenuRecord *mo_make_document_view_menubar (Widget,mo_window *); /*mjr*/
void mo_set_agents(mo_window *win, int which);
void mo_news_sub_anchor(Widget w, XtPointer clid, XtPointer calld);
void mo_news_unsub_anchor(Widget w, XtPointer clid, XtPointer calld);
void mo_news_mread_anchor(Widget w, XtPointer clid, XtPointer calld);

#endif
