/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <assert.h>

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


void mo_set_win_headers (mo_window *win, char* aurl_wa, char *title)
{
/* only NOTFRAME_TYPE or FRAMESET_TYPE have a headers */
	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAMESET_TYPE);

	XmxTextSetString (win->url_widget, aurl_wa);
	XtVaSetValues(win->base, XmNtitle, title, NULL);
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
/*###	win->navigation_action = NAVIGATE_NEW; ### */
}

/*########################### */
/* Update source text if necessary. */
/*	if(win->source_text && XtIsManaged(win->source_text) &&
/*	   win->current_node) {
/*		XmxTextSetString (win->source_text, win->current_node->text);
/*		XmxTextSetString (win->source_url_text, win->current_node->aurl_wa);
/*     		XmxTextSetString (win->source_date_text, (win->current_node->last_modified?win->current_node->last_modified:"Unknown"));
/*	}
/* vvv HREF ListBox Stuff -- BJS 10/2/95 */
/*	if(win->links_win) 
/*		mo_update_links_window(win);
/*
/* Every time we view the document, we reset the search_start
 * struct so searches will start at the beginning of the document. */
/*
/*	((ElementRef *)win->search_start)->id = 0;
 /* 	win->src_search_pos=0;             
/* ####### current_node ##### ????? */
/*	mo_gui_check_security_icon_in_win(win->current_node->authType,win);
/*	if (win->history_list && win->current_node) {
/*		XmListSelectPos(win->history_list, win->current_node->position, False);
/*		XmListSetBottomPos(win->history_list,win->current_node->position);
/*	}
/* ########################## */
