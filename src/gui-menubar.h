/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __GUIMENUBAR_H__
#define __GUIMENUBAR_H__

mo_status mo_set_fonts (mo_window *, int);
mo_status mo_set_underlines (mo_window *, int);
XmxMenuRecord *mo_make_document_view_menubar (Widget,mo_window *); /*mjr*/
void mo_set_agents(mo_window *win, int which);
void mo_news_sub_anchor(Widget w, XtPointer clid, XtPointer calld);
void mo_news_unsub_anchor(Widget w, XtPointer clid, XtPointer calld);
void mo_news_mread_anchor(Widget w, XtPointer clid, XtPointer calld);

#endif
