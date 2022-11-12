/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"
#include "../libnut/list.h"
#include "../libnut/mipcf.h"
#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "mosaic.h"
#include "gui-popup.h"
#include "gui.h"
#include "gui-documents.h"
#include "gui-menubar.h"
#include "mime.h"
#include "newsrc.h"
#include "gui-news.h"
#include "navigate.h"
#include "globalhist.h"
#include "paf.h"

#include "URLParse.h"

/* add more sense and sensibility to rbm */

/****************************************************************************
 * name:    mo_snarf_scrollbar_values
 * purpose: Store current viewing state in the current node, in case
 *          we want to return to the same location later.
 * inputs:  
 *   - mo_window *win: Current window.
 * returns: 
 *   mo_succeed
 *   (mo_fail if no current node exists)
 * remarks: 
 *   Snarfs current docid position in the HTML widget.
 ****************************************************************************/
static mo_status mo_snarf_scrollbar_values (mo_window *win)
{
	if (!win->current_node)	 /* Make sure we have a node. */
		return mo_fail;
	win->current_node->docid = HTMLPositionToId(win->scrolled_win, 0, 3);

	return mo_succeed;
}

/* ---------------------- mo_set_win_headers ----------------------- */

void mo_set_win_headers (mo_window *win, char* aurl_wa)
{
        if ( win->menubar == NULL) { /* ###FIXME (win is a frame , a sub_win) */
                                     /* try to enable multicast in frame..*/
                return;
        }
	XmxTextSetString (win->url_widget, aurl_wa);
	XtVaSetValues(win->base, XmNtitle, win->current_node->title, NULL);
}

static void mo_set_text(mo_window * win, Widget w, struct mark_up *mlist, int id, 
                         char *target_anchor)
{
	HTMLSetHTMLmark(w, mlist, id, target_anchor, win->current_node->base_url);
	mo_gui_done_with_icon (win);
}

/* purpose: Set a window's text and do lots of other housekeeping
 *          and GUI-maintenance things.
 * inputs:  
 *   - mo_window *win: The current window.
 *   - char      *url: The URL for the text; assumed to be canonicalized
 *                     and otherwise ready for inclusion in history lists,
 *                     the window's overhead URL display, etc.
 *   - char      *txt: The new text for the window.
 *   - int register_visit: If TRUE, then this text should be registerd
 *                         as a new node in the history list.  If FALSE,
 *                         then we're just moving around in the history list.
 *   - char      *ref: Reference (possible title) for this text.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This is the mother of all functions in Mosaic.  Probably should be
 *   rethought and broken down.
 */
mo_status mo_do_window_text (mo_window *win, char *url, struct mark_up *mlist,
		int register_visit, char *last_modified, char *expires)
{
	int newmode = moMODE_PLAIN;
	int id = 0;

/* TRACK APPLICATION MODE */
	if(!strncmp(url,"ftp:",4)) 
		newmode = moMODE_FTP;
	if(newmode != win->mode) {
		win->mode = newmode;
		mo_switch_mode(win);
	}

	if (mMosaicAppData.track_pointer_motion) {
		XmString xmstr=XmStringCreateLtoR (" ", XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues (win->tracker_widget,
			XmNlabelString, (XtArgVal)xmstr,
			NULL);
		XmStringFree (xmstr);
	}

/* If !register_visit, we're just screwing around with current_node
 * already, so don't bother snarfing scrollbar values. */

	if (register_visit)
		mo_snarf_scrollbar_values (win);

	if ((!register_visit) && win->current_node) {
		id = win->current_node->docid;
	}
/* ######################################### */
		mo_set_text(win, win->scrolled_win, mlist, id, win->current_node->goto_anchor);

/* vvv HREF ListBox Stuff -- BJS 10/2/95 */
	if(win->links_win) 
		mo_update_links_window(win);

/* Every time we view the document, we reset the search_start
 * struct so searches will start at the beginning of the document. */

	((ElementRef *)win->search_start)->id = 0;
  	win->src_search_pos=0;             

	mo_set_win_headers (win, win->current_node->aurl_wa );

	if (win->history_list && win->current_node) {
		XmListSelectPos(win->history_list, win->current_node->position, False);
		XmListSetBottomPos(win->history_list,win->current_node->position);
	}

/* Update source text if necessary. */
	if(win->source_text && XtIsManaged(win->source_text) &&
	   win->current_node) {
		XmxTextSetString (win->source_text, win->current_node->text);
		XmxTextSetString (win->source_url_text, win->current_node->aurl_wa);
      		XmxTextSetString (win->source_date_text, (win->current_node->last_modified?win->current_node->last_modified:"Unknown"));
	}
	if (win->current_node && win->current_node->previous != NULL)
		mo_back_possible (win);
	else
		mo_back_impossible (win);
	if (win->current_node && win->current_node->next != NULL)
		mo_forward_possible (win);
	else
		mo_forward_impossible (win);

/* ####### current_node ##### ????? */
	if (win->current_node)
		mo_gui_check_security_icon_in_win(win->current_node->authType,win);
	return mo_succeed;
}

/* name:    mo_set_win_current_node
 * purpose: Given a window and a node, set the window's current node.
 *          This assumes node is already all put together, in the history
 *          list for the window, etc.
 * inputs:  
 *   - mo_window *win: The current window.
 *   - mo_node  *node: The node to use.
 * returns: 
 *   Result of calling mo_do_window_text.
 * remarks: 
 *   This routine is meant to be used to move forward, backward,
 *   and to arbitrarily locations in the history list.
 */
mo_status mo_set_win_current_node (mo_window *win, mo_node *node)
{
	mo_status r;
  
	mo_snarf_scrollbar_values (win);
	win->current_node = node;

	r = mo_do_window_text (win, win->current_node->aurl_wa, 
			win->current_node->m_list, 
			FALSE, win->current_node->last_modified,
			win->current_node->expires);
	return r;
}

/* purpose: Reload the current window's text by pulling it over the
 *          network again.
 * inputs:  
 *   - mo_window *win: The current window.
 */
void mo_reload_document (Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	RequestDataStruct rds;

	if ( !win->current_node)	/* because of blank page or not loaded */
		return;
	rds.req_url = win->current_node->aurl_wa;
	rds.post_data = NULL;
	rds.ct = NULL;
	rds.is_reloading = True;
	rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_OVERWRITE;
	MMPafLoadHTMLDocInWin(win, &rds);
	win->navigation_action = NAVIGATE_NEW;
}

/* purpose: Reload the current window's text without pulling it over the net.
 * inputs:  
 *   - mo_window *win: The current window.
 */
mo_status mo_refresh_window_text (mo_window *win)
{
	if (!win->current_node)
		return mo_fail;

/* Clear out the cached stuff, if any exists. */
	mo_set_win_current_node (win, win->current_node);
	mo_gui_check_security_icon_in_win(win->current_node->authType,win);
	return mo_succeed;
}
