/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"
#include "../libnut/mipcf.h"
#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "mosaic.h"
#include "gui-popup.h"
#include "mo-www.h"
#include "gui.h"
#include "gui-documents.h"
#include "history.h"

#include "../libwww2/HTNews.h"
#include "../libwww2/HTAAUtil.h"

#ifdef MULTICAST
#include "../libmc/mc_rtp.h"
#include "../libmc/mc_sockio.h"
#include "../libmc/mc_defs.h"
#include "../libmc/mc_dispatch.h"

unsigned int             mc_local_url_id=1;
unsigned char            mc_len_local_url=0;
char                     mc_local_url[MC_MAX_URL_SIZE+1]={' ','\0'};
#endif

extern int do_meta;

/* ADC ugly hack ZZZ */
int CCIprotocol_handler_found;

/*SWP*/
extern char pre_title[80];
extern int cci_event;
extern char *cached_url;
extern int binary_transfer;
extern char *startup_document, *home_document;
extern Display *dsp;
extern char reloading;

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

/* from cciBindings.c */
extern int cci_get;

int loading_inlined_images = 0;
char *url_base_override = NULL;
int interrupted = 0;

/* Kludge to pass last modified time from HTMIME.c */
extern char *HTTP_last_modified;
extern char *HTTP_expires;
extern char *use_this_url_instead;

static Boolean check_imagedelay (char *url);
static mo_status mo_snarf_scrollbar_values (mo_window *win);
static mo_status mo_reset_document_headers (mo_window *win);
static void mo_back_possible (mo_window *win);
static void mo_forward_possible (mo_window *win);
static void mo_annotate_edit_possible (mo_window *win);
static void mo_annotate_edit_impossible (mo_window *win);
static void mo_set_text(mo_window * win, Widget w, char *txt, char *ans, int id, 
                         char *target_anchor, void *cached_stuff); /* mjr -- */
static mo_status mo_post_load_window_text (mo_window *win, char *url,
                                    char *content_type, char *post_data,
                                    char *ref);
 

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

/* Do the cached stuff thing. */
	win->current_node->cached_stuff = HTMLGetWidgetInfo (win->scrolled_win);
	return mo_succeed;
}

/* ---------------------- mo_reset_document_headers ----------------------- */

static mo_status mo_reset_document_headers (mo_window *win)
{

	char *buf=NULL;

	if (win->current_node) {
		XmxTextSetString (win->title_text, win->current_node->title);
		XmxTextSetString (win->url_text, win->current_node->url);
#ifdef MULTICAST
		if((win->mc_type == MC_MO_TYPE_MAIN) && mc_send_enable){
			McSendDataStruct * d = &mc_data_send_data_struct;

			mc_local_url_id++;
			strncpy(mc_local_url,win->current_node->url,MC_MAX_URL_SIZE);
			mc_len_local_url = strlen(mc_local_url);
			if (mc_send_win !=NULL ){
				McSendAllDataOnlyOnce(d);
			}
		}
#endif
	}
/*SWP -- 9/7/95 -- Make the menubar be the title space*/
	if (get_pref_boolean(eTITLEISWINDOWTITLE) ||
	    get_pref_boolean(eUSEICONBAR)) {
		if (win && win->base && win->current_node && 
		    win->current_node->title && *(win->current_node->title)) {
			buf=(char *)malloc(strlen(pre_title)+
					strlen(win->current_node->title)+15);
			if (!buf) {
				perror("Title Buffer");
				return(mo_fail);
			}
			sprintf(buf,"%s [%s",pre_title,win->current_node->title);
			/*annoying junk at end*/
			buf[strlen(buf)]='\0';
			strcat(buf,"]");
			buf[strlen(buf)]='\0';
			XtVaSetValues(win->base, XmNtitle,buf, NULL);
			free(buf);
		} else
		 	if (win && win->base) {
				buf=(char *)malloc(strlen(pre_title)+15);
				if (!buf) {
					perror("Title Buffer");
					return(mo_fail);
				}
				sprintf(buf,"%s: [%s]",pre_title,"No Title" );
				buf[strlen(buf)]='\0';
				XtVaSetValues(win->base, XmNtitle,buf, NULL);
				free(buf);
			}
	}
	return mo_succeed;
}

/* --------------------------- mo_back_possible --------------------------- */

/* This could be cached, but since it shouldn't take too long... */
static void mo_back_possible (mo_window *win)
{
	if (get_pref_boolean(eUSETEXTBUTTONBAR)) {
		mo_tool_state(&(win->tools[BTN_PREV]),XmxSensitive,BTN_PREV);
		XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxSensitive);
	}
	mo_popup_set_something("Back", XmxSensitive, NULL);
	return;
}

/*
 * name:    mo_back_impossible
 * purpose: Can't go back (nothing in the history list).
 */
mo_status mo_back_impossible (mo_window *win)
{
	if (get_pref_boolean(eUSETEXTBUTTONBAR)) {
		XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxNotSensitive);
		mo_tool_state(&(win->tools[BTN_PREV]),XmxNotSensitive,BTN_PREV);
	}
	mo_popup_set_something("Back", XmxNotSensitive, NULL);
	return mo_succeed;
}

static void mo_forward_possible (mo_window *win)
{
	if (get_pref_boolean(eUSETEXTBUTTONBAR)) {
		mo_tool_state(&(win->tools[BTN_NEXT]),XmxSensitive,BTN_NEXT);
		XmxRSetSensitive (win->menubar, (XtPointer)mo_forward, XmxSensitive);
	}
	mo_popup_set_something("Forward", XmxSensitive, NULL);
	return;
}

/*
 * name:    mo_forward_impossible
 * purpose: Can't go forward (nothing in the history list).
 */
mo_status mo_forward_impossible (mo_window *win)
{
	if (get_pref_boolean(eUSETEXTBUTTONBAR)) {
		mo_tool_state(&(win->tools[BTN_NEXT]),XmxNotSensitive,BTN_NEXT);
		XmxRSetSensitive (win->menubar, (XtPointer)mo_forward, XmxNotSensitive);
	}
	mo_popup_set_something("Forward", XmxNotSensitive, NULL);
	return mo_succeed;
}

/* ---------------------- mo_annotate_edit_possible ----------------------- */

static void mo_annotate_edit_possible (mo_window *win)
{
	XmxRSetSensitive (win->menubar, (XtPointer)mo_annotate_edit, XmxSensitive);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_annotate_delete, XmxSensitive);
	return;
}

static void mo_annotate_edit_impossible (mo_window *win)
{
	XmxRSetSensitive (win->menubar, (XtPointer)mo_annotate_edit, XmxNotSensitive);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_annotate_delete, XmxNotSensitive);
	return;
}

static void mo_set_text(mo_window * win, Widget w, char *txt, char *ans, int id, 
                         char *target_anchor, void *cached_stuff)
{
/* Any data transfer that takes place in here must be inlined image loading. */
	loading_inlined_images = 1;
	interrupted = 0;
	mo_set_image_cache_nuke_threshold ();
#ifdef MULTICAST
	if((win->mc_type == MC_MO_TYPE_MAIN) && mc_send_enable){
		if (mc_send_win !=NULL ){
			McSetHtmlTexte(txt);
		}
	}
#endif

	if (get_pref_boolean(eANNOTATIONS_ON_TOP))
		HTMLSetText (w, txt, ans ? ans : "\0", "\0", id, target_anchor, 
					cached_stuff);
	else
		HTMLSetText (w, txt, "\0", ans ? ans : "\0", id, target_anchor, 
				cached_stuff);
	loading_inlined_images = 0;
	interrupted = 0;
	mo_gui_done_with_icon (win);
}

/* name:    mo_do_window_text (PRIVATE)
 * purpose: Set a window's text and do lots of other housekeeping
 *          and GUI-maintenance things.
 * inputs:  
 *   - mo_window *win: The current window.
 *   - char      *url: The URL for the text; assumed to be canonicalized
 *                     and otherwise ready for inclusion in history lists,
 *                     the window's overhead URL display, etc.
 *   - char      *txt: The new text for the window.
 *   - char  *txthead: The start of the malloc'd block of text corresponding
 *                     to txt.
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
mo_status mo_do_window_text (mo_window *win, char *url, char *txt, char *txthead,
		int register_visit, char *ref,
		char *last_modified, char *expires)
{
	char *ans;
	int newmode = moMODE_PLAIN;

	if (txt != NULL) /* send document over cci if needed */
 		MoCCISendBrowserViewOutput(url, "text/html", txt, strlen(txt));

/* TRACK APPLICATION MODE */
	if(!strncmp(url,"ftp:",4)) 
		newmode = moMODE_FTP;
	if(!strncmp(url,"news:",4)) {
		int p,n,pt,nt,f;
		news_status(url,&pt,&nt,&p,&n,&f);
		mo_tool_state(&(win->tools[BTN_PTHR]),
			pt?XmxSensitive:XmxNotSensitive,BTN_PTHR);
			XmxRSetSensitive (win->menubar, (XtPointer)mo_news_prevt,
			pt?XmxSensitive:XmxNotSensitive);

		mo_tool_state(&(win->tools[BTN_NTHR]),
			nt?XmxSensitive:XmxNotSensitive,BTN_NTHR);
			XmxRSetSensitive (win->menubar, (XtPointer)mo_news_nextt, 
			nt?XmxSensitive:XmxNotSensitive);

		mo_tool_state(&(win->tools[BTN_PART]),
			p?XmxSensitive:XmxNotSensitive,BTN_PART);
			XmxRSetSensitive (win->menubar, (XtPointer)mo_news_prev, 
			p?XmxSensitive:XmxNotSensitive);

		mo_tool_state(&(win->tools[BTN_NART]),
			n?XmxSensitive:XmxNotSensitive,BTN_NART);
			XmxRSetSensitive (win->menubar, (XtPointer)mo_news_next, 
			n?XmxSensitive:XmxNotSensitive);

		mo_tool_state(&(win->tools[BTN_POST]),XmxSensitive,BTN_POST);
		mo_tool_state(&(win->tools[BTN_FOLLOW]),
			f?XmxSensitive:XmxNotSensitive,BTN_FOLLOW);
			XmxRSetSensitive(win->menubar, (XtPointer)mo_news_follow, 
			f?XmxSensitive:XmxNotSensitive);
/* set the popup too */
		mo_popup_set_something("Previous Thread", 
			pt?XmxSensitive:XmxNotSensitive, NULL);
		mo_popup_set_something("Next Thread",
			nt?XmxSensitive:XmxNotSensitive, NULL);
		mo_popup_set_something("Previous Article", 
			p?XmxSensitive:XmxNotSensitive, NULL);	      
		mo_popup_set_something("Next Article", 
			n?XmxSensitive:XmxNotSensitive, NULL);
		mo_popup_set_something("Followup",
			f?XmxSensitive:XmxNotSensitive, NULL);
		newmode = moMODE_NEWS;
	}              
	if(newmode != win->mode) {
		win->mode = newmode;
		mo_switch_mode(win);
	}
	mo_set_current_cached_win (win);

	if (get_pref_boolean(eTRACK_POINTER_MOTION)) {
		XmString xmstr=XmStringCreateLtoR (" ", XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues (win->tracker_label,
			XmNlabelString, (XtArgVal)xmstr,
			NULL);
		XmStringFree (xmstr);
	}

/* If !register_visit, we're just screwing around with current_node
 * already, so don't bother snarfing scrollbar values. */

	if (register_visit)
		mo_snarf_scrollbar_values (win);

/* cached_url HAS to be set here, since Resolve counts on it. */

	cached_url = mo_url_canonicalize (url, "");
	win->cached_url = cached_url;
	mo_here_we_are_son (url);
	{
/* Since mo_fetch_annotation_links uses the communications code,
 * we need to play games with binary_transfer. */
		int tmp = binary_transfer;
		binary_transfer = 0;
		ans=mo_fetch_annotation_links(url,
				get_pref_boolean(eANNOTATIONS_ON_TOP));
		binary_transfer = tmp;
	}

/* If there is a BASE tag in the document that contains a "real"
 * URL, this will be non-NULL by the time we exit and base_callback
 * will have been called. */
	url_base_override = NULL;
	{
		int id = 0, freeta = 0;
		void *cached_stuff = NULL;
		char *target_anchor = win->target_anchor;

		if ((!register_visit) && win->current_node) {
			id = win->current_node->docid;
			cached_stuff = win->current_node->cached_stuff;
		}

/* If the window doesn't have a target anchor already,
 * see if there's one in this node's URL. */
		if ((!target_anchor || !(*target_anchor)) && win->current_node) {
			target_anchor = mo_url_extract_anchor (win->current_node->url);
			freeta = 1;
		}
		if (!txt || !txthead) {/*Just make it look OK... band-aid city.*/
			txt = strdup ("\0");
			txthead = txt;
		}
/* #########################################*/
		mo_set_text(win, win->scrolled_win, txt, ans, id, target_anchor,
				cached_stuff);
		if (ans)
			free(ans);

/* vvv HREF ListBox Stuff -- BJS 10/2/95 */
		if(win->links_win) 
			mo_update_links_window(win);
		if (win->target_anchor)
			free (win->target_anchor);

		win->target_anchor = NULL;
		if (freeta)
			free (target_anchor);
	}

	if (url_base_override) {
/* Get the override URL -- this should be all we need to do here. */
		url = url_base_override;
		mo_here_we_are_son (url);
	}

/* Every time we view the document, we reset the search_start
 * struct so searches will start at the beginning of the document. */

	((ElementRef *)win->search_start)->id = 0;
  	win->src_search_pos=0;             

/* CURRENT_NODE ISN'T SET UNTIL HERE (assuming register_visit is 1). */
/* Now that WbNtext has been set, we can pull out WbNtitle. */
/* First, check and see if we have a URL.  If not, we probably */
/* are only jumping around inside a document. */
	if (url && *url) {
		if (register_visit)
			mo_record_visit (win, url, txt, txthead, ref,
					last_modified, expires);
		else {
			/* At the very least we want to pull out the new title,
			 * if one exists. */
			if (win->current_node) {
				if (win->current_node->title)
					free (win->current_node->title);
				win->current_node->title = mo_grok_title (win, url, ref);
			} else {
				mo_node *node = (mo_node *)malloc (sizeof (mo_node));

				node->url = url;
        			node->text = txt;
        			node->texthead = txthead;
        			node->ref = ref;
				node->title = mo_grok_title (win, url, ref);
				node->authType = HTAA_NONE;
				node->docid = 1;
				node->cached_stuff = NULL;
				node->last_modified = 0;
				node->expires = 0;
				node->previous = NULL;
				node ->next = NULL;
				win->current_node = node;
			}
		}
	}
	mo_reset_document_headers (win);

	if (win->history_list && win->current_node) {
		XmListSelectPos(win->history_list, win->current_node->position, False);
		XmListSetBottomPos(win->history_list,win->current_node->position);
	}

/* Update source text if necessary. */
	if(win->source_text && XtIsManaged(win->source_text) &&
	   win->current_node) {
		XmxTextSetString (win->source_text, win->current_node->text);
		XmxTextSetString (win->source_url_text, win->current_node->url);
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

	if (win->current_node && 
	    mo_is_editable_annotation (win, win->current_node->text))
		mo_annotate_edit_possible (win);
	else
		mo_annotate_edit_impossible (win);
/* ####### current_node ##### ????? */
	if (win->current_node)
		mo_gui_check_security_icon(win->current_node->authType);

/* every time we load a new page set the focus to hotkeys. we do
 * this because we may have gotten here via forms and since we
 * don't kill any widgets, some unmanaged widget out there could
 * have the focus */               

	if(!get_pref_boolean(eFOCUS_FOLLOWS_MOUSE) && win->have_focus) {
		XtSetKeyboardFocus(win->base, win->view);
/* make traversal start at top of document should there be forms */
	}                                
	HTMLTraverseTabGroups(win->view, XmTRAVERSE_HOME);
	mo_not_busy ();                    
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
	void *to_free = NULL;
	mo_status r;
  
	mo_snarf_scrollbar_values (win);
	if (win->current_node && win->reloading) {
		to_free = win->current_node->cached_stuff;
		win->current_node->cached_stuff = NULL;
	}
	win->current_node = node;
	mo_set_current_cached_win (win);

/********* Send Anchor history to CCI if CCI wants it */
	MoCCISendAnchorToCCI(win->current_node->url, 0);

	r = mo_do_window_text (win, win->current_node->url, 
			win->current_node->text, 
			win->current_node->texthead,
			FALSE, win->current_node->ref,
			win->current_node->last_modified,
			win->current_node->expires);
	if (win->reloading) {
		if (to_free)
			HTMLFreeWidgetInfo (to_free);
		win->reloading = 0;
	}
	return r;
}

/* name:    mo_reload_window_text
 * purpose: Reload the current window's text by pulling it over the
 *          network again.
 * inputs:  
 *   - mo_window *win: The current window.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This frees the current window's texthead.  This calls mo_pull_er_over
 *   directly, and needs to be smarter about handling HDF, etc.
 */
mo_status mo_reload_window_text (mo_window *win, int reload_images_also)
{
	mo_set_current_cached_win (win);

	if (!win->current_node)		 /* Uh oh, this is trouble... */
		return mo_load_window_text(win, startup_document ? 
				startup_document : home_document, NULL);

/* Free all images in the current document. */
	if (get_pref_boolean(eRELOAD_RELOADS_IMAGES) || reload_images_also)
		mo_zap_cached_images_here (win);

/* Free the current document's text. */
/* REALLY we shouldn't do this until we've verified we have new text that's
 * actually good here -- for instance, if we have a document on display,
 * then go to binary transfer mode, then do reload, we should pick up the
 * access override here and keep the old copy up on screen. */
	if (win->current_node->texthead != NULL) {
		free (win->current_node->texthead);
		win->current_node->texthead = NULL;
	}

/* Set binary_transfer as per current window. */
	binary_transfer = win->binary_transfer;
	mo_set_current_cached_win (win);
	interrupted = 0;
	if (get_pref_boolean(eRELOAD_PRAGMA_NO_CACHE))
		reloading=1;
	win->current_node->text = mo_pull_er_over (win->current_node->url, 
					&win->current_node->texthead);
	if (HTTP_last_modified)
		win->current_node->last_modified = strdup(HTTP_last_modified);
	if (HTTP_expires)
		win->current_node->expires = strdup(HTTP_expires);
	reloading=0;
/* Check use_this_url_instead from HTAccess.c. */
/* IS THIS GOOD ENOUGH FOR THIS CASE??? */
	if (use_this_url_instead)
		win->current_node->url = use_this_url_instead;
/* Clear out the cached stuff, if any exists. */
	win->reloading = 1;
	mo_set_win_current_node (win, win->current_node);
	win->reloading = 0;
                                     
/* If news: URL, then we need to auto-scroll to the >>> marker if it
 * is here. We use a hacked version of the searching function here
 * which will need to be updated when we rewrite. --SWP */

	if (win->current_node && win->current_node->url &&      
	    !strncmp(win->current_node->url,"news:",5)) {
		mo_search_window(win,">>>",0,1,1); 
	}             
	return mo_succeed;
}

/* name:    mo_refresh_window_text
 * purpose: Reload the current window's text without pulling it over the net.
 * inputs:  
 *   - mo_window *win: The current window.
 * returns: 
 *   mo_succeed
 * remarks: 
 */
mo_status mo_refresh_window_text (mo_window *win)
{
	mo_set_current_cached_win (win);

	if (!win->current_node)
		return mo_fail;

/* Clear out the cached stuff, if any exists. */
	win->reloading = 1;
	mo_set_win_current_node (win, win->current_node);
	mo_gui_check_security_icon(win->current_node->authType);
	return mo_succeed;
}

/* name:    mo_load_window_text
 * purpose: Given a window and a raw URL, load the window.  The window
 *          is assumed to already exist with a document inside, etc.
 * inputs:  
 *   - mo_window *win: The current window.
 *   - char      *url: The URL to load.
 *   - char      *ref: The reference ("parent") URL.
 *         NOTE: actually, the ref field is the citation hypertext - AMB
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This is getting ugly.
 */
mo_status mo_load_window_text (mo_window *win, char *url, char *ref)
{
	char *newtext = NULL, *newtexthead = NULL;
	char *last_modified = 0;
	char *expires = 0;
	mo_status return_stat = mo_succeed;

	mo_busy (); 
	win->target_anchor = mo_url_extract_anchor (url);

	/*If we're just refer an anchor inside a document,do the right thing.*/
	if (url && *url == '#') {
		/* Now we make a copy of the current text and make sure we ask
		 * for a new mo_node and entry in the history list.
		 * IF we're not dealing with an internal reference. */
		if(strncmp(url, "#hdfref;", 8) && strncmp(url, "#hdfdtm;", 8)) {
			if (win->current_node) {
				newtext = strdup (win->current_node->text);
				newtexthead = newtext;
			} else {
				newtext = strdup ("lose");
				newtexthead = newtext;
			}
		}
		url = mo_url_canonicalize_keep_anchor 
			(url, win->current_node ? win->current_node->url : "");
		/********* Send Anchor history to CCI if CCI wants it */
		MoCCISendAnchorToCCI(url, 1);
		/*****************************************************/
	} else {
		/* Get a full address for this URL. 
		 * Under some circumstances we may not have a current node yet
		 *and may wish to just run with it... so check for that. */

		/* Check use_this_url_instead from HTAccess.c. */
		char * canon;

		if (win->current_node && win->current_node->url) {
			url = mo_url_canonicalize_keep_anchor(
					url, win->current_node->url);
		}
		/* Set binary_transfer as per current window. */
		binary_transfer = win->binary_transfer;
		mo_set_current_cached_win (win);

		canon = mo_url_canonicalize (url, "");
		interrupted = 0;

/* ADC ZZZZ   ugly hack below:  */

		CCIprotocol_handler_found = 0;
/********* Send Anchor history to CCI if CCI wants to handle it */
		MoCCISendAnchorToCCI(url,3); 
/*****************************************************/

		if (CCIprotocol_handler_found)
			return return_stat;         /* success */

/********* Send Anchor history to CCI if CCI wants it */
		MoCCISendAnchorToCCI(url,1);

		newtext = mo_pull_er_over (canon, &newtexthead);
/*########### add the multicast sender code here ######## */

/* added so MCCIRequestGetURL could return failed when url fails */
		if (newtext)
			if (  (!strncmp(newtext, "<H1>ERROR<H1>", 10)) ||
			      (!strncmp(newtext, 
				"<HEAD><TITLE>404 Not Found</TITLE></HEAD>",28)))
				return_stat = mo_fail;

/* Yes this is a really big hack (ETG) */
		if (win->target_anchor && *(win->target_anchor)) 
			MoCCIAddAnchorToURL(canon, url);

/* AF */
		if (HTTP_last_modified) 
			last_modified = strdup(HTTP_last_modified);
		if (HTTP_expires)
			expires       = strdup(HTTP_expires);
		free (canon);

		if (use_this_url_instead) {
			mo_here_we_are_son (url);
			url = use_this_url_instead;
            
/* Go get another target_anchor. */
			if (win->target_anchor)
				free (win->target_anchor);
			win->target_anchor = mo_url_extract_anchor (url);
		}
	}

/* Now, if it's a telnet session, there should be no need
 * to do anything else.  Also check for override in text itself. */
	if(strncmp (url, "telnet:", 7) == 0 || 
	   strncmp (url, "tn3270:", 7) == 0 ||
	   strncmp (url, "rlogin:", 7) == 0 ||
	   (newtext && strncmp(newtext, "<mosaic-access-override>", 24) == 0)) {
		/* We don't need this anymore. */
		free (newtext);

/* We still want a global history entry but NOT a 
 * window history entry. */
		mo_here_we_are_son (url);
/* ... and we want to redisplay the current window to
 * get the effect of the history entry today, not tomorrow. */
		mo_redisplay_window (win);
/* We're not busy anymore... */
		mo_gui_done_with_icon (win);
	} else 
		if (newtext) {
			/* Not a telnet session and not an override,
			 * but text present (the "usual" case): */

			/* first check if we are using cci Get, if so,
			 * display the error message */

			if (cci_get && (return_stat == mo_fail) ) {
				fprintf(stderr, "MCCI GET has passed in a wrong url\n");
			} else {
				/* Set the window text. */
				mo_do_window_text(win,url, newtext,newtexthead,
					(do_meta==1?0:2), ref, last_modified, expires);
			}
		} else { 	/* No text at all. */
			mo_gui_done_with_icon (win);
		}

/********* Send Anchor history to CCI if CCI wants it */
	MoCCISendAnchorToCCI(url,2);

/* first check if we are using cci Get, display
 * the error message */
	if (cci_get && (return_stat == mo_fail) ) {
		fprintf(stderr,"MCCI GET has passed in a wrong url\n");
	} else
		if (win && win->current_node) {
			mo_gui_check_security_icon(win->current_node->authType);
		}
	if (last_modified)
		free(last_modified);
	if (expires)
		free(expires);
/* If news: URL, then we need to auto-scroll to the >>> marker if it
 * is here. We use a hacked version of the searching function here
 * which will need to be updated when we rewrite. --SWP */

	if (win->current_node && win->current_node->url &&
	    !strncmp(win->current_node->url,"news:",5)) {
		mo_search_window(win,">>>",0,1,1);
	}   
	return return_stat;
}

static mo_status mo_post_load_window_text (mo_window *win, char *url, 
                                    char *content_type, char *post_data, 
                                    char *ref)
{
	char *newtext = NULL, *newtexthead = NULL, *actionID;

	win->target_anchor = mo_url_extract_anchor (url);
	actionID = strdup(url);    /* make a copy of url for cci's register id */

/* If we're just referencing an anchor inside a document, do the right thing. */
	if (url && *url == '#') {
/* Now we make a copy of the current text and make sure we ask
 * for a new mo_node and entry in the history list.
 * IF we're not dealing with an internal reference. */
		if(strncmp (url, "#hdfref;", 8) && strncmp (url, "#hdfdtm;", 8)) {
			if (win->current_node) {
				newtext = strdup (win->current_node->text);
				newtexthead = newtext;
			} else {
				newtext = strdup ("lose");
				newtexthead = newtext;
			}
		}
		url = mo_url_canonicalize_keep_anchor(url, win->current_node ? 
					win->current_node->url : "");
	} else {
/* Get a full address for this URL. */
/* Under some circumstances we may not have a current node yet
 * and may wish to just run with it... so check for that. */
		if (win->current_node && win->current_node->url)
			url = mo_url_canonicalize_keep_anchor(url, 
				win->current_node->url);
/* Set binary_transfer as per current window. */
		binary_transfer = win->binary_transfer;
		mo_set_current_cached_win (win);
		{
			char *canon = mo_url_canonicalize (url, "");
			interrupted = 0;

			if (!MoCCIFormToClient(actionID, NULL, 
					content_type, post_data, 0))
				newtext = mo_post_pull_er_over(canon, 
						content_type, 
						post_data, &newtexthead);
			free (canon);
		}
		if (use_this_url_instead) {
			mo_here_we_are_son (url);
			url = use_this_url_instead;
		}
	}

/* Now, if it's a telnet session, there should be no need
 * to do anything else.  Also check for override in text itself. */
	if(strncmp (url,"telnet:",7) == 0 || strncmp(url, "tn3270:", 7) == 0 ||
	   strncmp (url, "rlogin:", 7) == 0 ||
	   (newtext && strncmp (newtext, "<mosaic-access-override>", 24) == 0)) {
/* We don't need this anymore. */
		free (newtext);

/* We still want a global history entry but NOT a window history entry. */
		mo_here_we_are_son (url);
/* ... and we want to redisplay the current window to
 * get the effect of the history entry today, not tomorrow. */
		mo_redisplay_window (win);
/* We're not busy anymore... */
		mo_gui_done_with_icon (win);
	} else 
		if (newtext) {
/* Not a telnet session and not an override, but text present(the "usual" case):*/
/* Set the window text. */
			mo_do_window_text (win, url, newtext, newtexthead, 
						1, ref, 0, 0);
		} else {	 /* No text at all. */
			mo_gui_done_with_icon (win);
		}
	return mo_succeed;
}

/* name:    mo_duplicate_window_text
 * purpose: Given an old window and a new window, make a copy of the text
 *          in the old window and install it in the new window.
 * inputs:  
 *   - mo_window *oldw: The old window.
 *   - mo_window *neww: The new window.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This is how windows are cloned: a new window is created and this
 *   call sets up its contents.
 */
mo_status mo_duplicate_window_text (mo_window *oldw, mo_window *neww)
{
/* We can get away with just cloning text here and forgetting
 * about texthead, obviously, since we're making a new copy. */
	char *newtext;

	if (!oldw->current_node)
		return mo_fail;

	newtext = strdup (oldw->current_node->text);
	mo_do_window_text (neww, strdup (oldw->current_node->url), 
		newtext, newtext, TRUE, 
		oldw->current_node->ref ? strdup (oldw->current_node->ref) : NULL,
		oldw->current_node->last_modified,
		oldw->current_node->expires);
	return mo_succeed;
}

/*
 * name:    mo_access_document
 * purpose: Given a URL, access the document by loading the current 
 *          window's text.
 * inputs:  
 *   - mo_window *win: The current window.
 *   - char      *url: URL to access.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This should be the standard call for accessing a document.
 */
mo_status mo_access_document (mo_window *win, char *url)
{
	mo_set_current_cached_win (win);
	mo_load_window_text (win, url, NULL);
	return mo_succeed;
}

mo_status mo_post_access_document (mo_window *win, char *url,
                                   char *content_type, char *post_data)
{
	mo_set_current_cached_win (win);
	mo_post_load_window_text (win, url, content_type, post_data, NULL);
	return mo_succeed;
}
