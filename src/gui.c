/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <signal.h>
#include <sys/types.h>
#include <ctype.h>
#include <pwd.h>
#include <X11/keysym.h>
#include <Xm/XmAll.h>
#include <X11/Xmu/Editres.h>

#include "../libnut/mipcf.h"
#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "../libnut/list.h"
#include "URLParse.h"
#include "xresources.h"
#include "mime.h"
#include "img.h"
#include "gui.h"
#include "gui-documents.h"
#include "gui-menubar.h"
#include "gui-dialogs.h"
#include "hotlist.h"
#include "navigate.h"
#include "globalhist.h"
#include "pixmaps.h"
#include "libnut/system.h"
#include "gui-popup.h"
#include "mailto.h"
#include "util.h"
#include "paf.h"
#include "cache.h"

#ifdef MULTICAST
#include "../libmc/mc_main.h"
extern mo_window * mc_send_win;
#endif

#include "bitmaps/security.xbm"

#include "pixmaps.h"


#ifdef MULTICAST
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
extern XmxCallback (mc_frame_callback);
#endif

#ifdef DEBUG
#define DEBUG_FORM
#define DEBUG_FRAME
#endif

/* Because Sun cripples the keysym.h file. */
#ifndef XK_KP_Up
#define XK_KP_Home              0xFF95  /* Keypad Home */
#define XK_KP_Left              0xFF96  /* Keypad Left Arrow */
#define XK_KP_Up                0xFF97  /* Keypad Up Arrow */
#define XK_KP_Right             0xFF98  /* Keypad Right Arrow */
#define XK_KP_Down              0xFF99  /* Keypad Down Arrow */
#define XK_KP_Prior             0xFF9A  /* Keypad Page Up */
#define XK_KP_Next              0xFF9B  /* Keypad Page Down */
#define XK_KP_End               0xFF9C  /* Keypad End */
#endif

#define SLAB_MENU 0
#define SLAB_URL 1
#define SLAB_TOOLS 2
#define SLAB_STATUS 3
#define SLAB_VIEW 4
#define SLAB_GLOBE 5


/* SWP -- Spoof Agent stuff */
extern int 	selectedAgent;
extern char 	**agent;

#ifdef NEWS
/* PLB */
extern int 	newsShowAllGroups;
extern int 	newsShowAllArticles;
extern int 	newsShowReadGroups;
extern int 	newsNoThreadJumping;
#endif

/* int securityType=HTAA_NONE; */

extern int 	noLength;	 /*SWP -- 10.27.95 -- No Content Length*/


/*
 * Globals used by the pixmaps for the animated icon.
 * Marc, I imagine you might want to do something cleaner
 * with these?
 */
extern int 	IconWidth, IconHeight, WindowWidth, WindowHeight;
extern Pixmap 	*IconPix,*IconPixSmall,*IconPixBig;

Widget 		mo_fill_toolbar(mo_window *win, Widget top, int w, int h);
char *		MakeFilename();
static void 	mo_extra_buttons(mo_window *win, Widget top);

/* ------------------------------ variables ------------------------------- */

int sarg[7],smalllogo=0,stexttools=0;

/* ------------------------------------------------------ */


/* This is exported to libwww, like altogether too many other variables here. */

/* Now we cache the current window right before doing a binary
   transfer, too.  Sheesh, this is not pretty. */

XColor fg_color, bg_color;

/* Forward declaration of test predicate. */
int anchor_visited_predicate (Widget, char *, char *);

/* When we first start the application, we call mo_startup()
   after creating the unmapped toplevel widget.  mo_startup()
   either sets the value of this to 1 or 0.  If 0, we don't
   open a starting window. */

int defer_initial_window;

static Pixmap security_pix;		/* Pixmaps for security button. */
extern Pixmap toolbarBack, toolbarForward, toolbarHome, toolbarReload,
    toolbarOpen, toolbarSave, toolbarClone, toolbarNew, toolbarClose,
    toolbarBackGRAY, toolbarForwardGRAY,
    toolbarSearch, toolbarPrint, toolbarPost, toolbarFollow,
    tearv, tearh, toolbarPostGRAY, toolbarFollowGRAY,
    toolbarNewsFwd, toolbarNewsFFwd, toolbarNewsRev, toolbarNewsFRev,
    toolbarNewsIndex, toolbarAddHotlist, toolbarNewsGroups,
    toolbarNewsFwdGRAY, toolbarNewsFFwdGRAY, toolbarNewsRevGRAY, toolbarNewsFRevGRAY,
    toolbarNewsIndexGRAY, toolbarFTPput, toolbarFTPmkdir;

extern Pixmap securityKerberos4, securityBasic, securityMd5, securityNone,
    securityUnknown, securityKerberos5, securityDomain, securityLogin,
    enc_not_secure;


/* --------------BalloonHelpStuff---------------------------------------- */

static BalloonInfoData * AlloBalloonInfoData ( mo_window * win, char * s)
{
	BalloonInfoData * info;

	info =  (BalloonInfoData*) malloc(sizeof(BalloonInfoData));
	info->win = win;
	info->msg = s;
	return info;
}

/* ----------------------------- WINDOW LIST ------------------------------ */

static mo_window * main_winlist = NULL;
#ifdef MULTICAST
/*################################*/
static mo_window *mc_rcv_winlist = NULL;
#endif

/* Return either the first window in the window list or the next
   window after the current window. */
/*   ##################### */
mo_window *mo_main_next_window (mo_window *win)
{
	if (win == NULL)
		return main_winlist;
	return win->next;
}

#ifdef MULTICAST
mo_window *mo_rcv_next_window (mo_window *win)
{
	if (win == NULL)
		return mc_rcv_winlist;
	return win->next;
}
#endif

/* Register a window in the window list. */
static void mo_add_window_to_list (mo_window *win)
{
#ifdef MULTICAST
	switch (win->mc_type){
	case MC_MO_TYPE_UNICAST:
	case MC_MO_TYPE_MAIN:
#endif
		if (main_winlist == NULL) {
			win->next = NULL;
			main_winlist = win;
		} else {
			win->next = main_winlist;
			main_winlist = win;
		}
		return;
#ifdef MULTICAST
	case MC_MO_TYPE_RCV_URL_ONLY:
	case MC_MO_TYPE_RCV_ALL:
		if (mc_rcv_winlist == NULL) {
			win->next = NULL;
			mc_rcv_winlist = win;
		} else {
			win->next = mc_rcv_winlist;
			mc_rcv_winlist = win;
		}
		return;
	}
#endif
}

/* Remove a window from the window list. */
static void mo_remove_window_from_list (mo_window *win)
{
	mo_window *w = NULL, *prev = NULL;

#ifdef MULTICAST
	switch (win->mc_type){
	case MC_MO_TYPE_UNICAST:
	case MC_MO_TYPE_MAIN:
#endif
		while ( (w = mo_main_next_window (w)) ) {
			if (w == win) { /* Delete w. */
				if (!prev) { /* No previous window. */
					main_winlist = w->next;
					free (w);
					w = NULL;
					if (!main_winlist) /* Maybe exit. */
						mo_exit ();
					return;
				} else { /* Previous window. */
					prev->next = w->next;
					free (w);
					w = NULL;
					return ;
				}
			}
			prev = w;
		}
#ifdef MULTICAST
		break;
	case MC_MO_TYPE_RCV_URL_ONLY:
	case MC_MO_TYPE_RCV_ALL:
		while (w = mo_rcv_next_window (w)) {
			if (w == win) { /* Delete w. */
				if (!prev) { /* No previous window. */
					mc_rcv_winlist = w->next;
					free (w);
					w = NULL;
					return;
				} else { /* Previous window. */
					prev->next = w->next;
					free (w);
					w = NULL;
					return ;
				}
			}
			prev = w;
		}
		break;
	}
#endif
	fprintf(stderr,"[mo_remove_window_from_list] unable to remove\n");
}

/*
 * name:    mo_assemble_help_url
 * purpose: Make a temporary, unique filename.
 * inputs:  
 *   - char *file: Filename to be appended to Rdata.docs_directory.
 * returns: 
 *   The desired help url (a malloc'd string).
 */
char *mo_assemble_help_url (char *file)
{
	char *tmp;
	char *docs_directory = mMosaicAppData.docs_directory;

	if (!file)
		return strdup ("http://lose.lose/lose");

	tmp = (char *)malloc((strlen (file) + strlen (docs_directory) + 4));
	if (docs_directory[strlen(docs_directory)-1] == '/') {
				/* Trailing slash in docs_directory spec. */
		sprintf (tmp, "%s%s", docs_directory, file);
	} else {		/* No trailing slash. */
		sprintf (tmp, "%s/%s", docs_directory, file);
	}
	return tmp;
}

/*
 * name:    mo_redisplay_window
 * purpose: Cause the current window's HTML widget to be refreshed.
 *          This causes the anchors to be reexamined for visited status.
 * inputs:  
 *   - mo_window *win: Current window.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   
 */
void mo_redisplay_window (mo_window *win)
{
	HTMLRetestAnchors (win->scrolled_win, anchor_visited_predicate,
		win->current_node->base_url);
}


static XmxCallback (icon_pressed_cb)
{
	mo_window * win = (mo_window*) client_data;

	if (!win->pafd)		/* no transfert in progress */
		return;
	if (win->pafd->paf_child) { /* a embedded object in progress */
/* if we stop a child , we have a mark_up list in parent. Update view */
		HTMLSetHTMLmark (win->scrolled_win,
			win->current_node->m_list,
			win->current_node->docid,
			win->current_node->goto_anchor,
			win->current_node->aurl);
	}
	(*win->pafd->call_me_on_stop)(win->pafd); /* call up->down proc */
}

static XmxCallback (security_pressed_cb)
{
/*
	mo_window *win = (mo_window *)client_data;
	char buf[BUFSIZ];

	if (!win || !win->current_node || !win->current_node)
		return;
	mo_gui_check_security_icon_in_win(win->current_node->authType,win);

	switch(win->current_node->authType) {
	case HTAA_NONE:
		strcpy(buf,"There is no authentication for this URL." );
		break;
	case HTAA_BASIC:
		strcpy(buf,"The authentication method for this URL is\nBasic (uuencode/uudecode)." );
		break;
	case HTAA_KERBEROS_V4:
		strcpy(buf,"The authentication method for this URL is\nKerberos v4." );
		break;
	case HTAA_KERBEROS_V5:
		strcpy(buf,"The authentication method for this URL is\nKerberos v5." );
		break;
	case HTAA_MD5:
		strcpy(buf,"The authentication method for this URL is\nMD5." );
		break;
	case HTAA_DOMAIN:
		strcpy(buf,"This URL is Domain Restricted." );
		break;
	case HTAA_LOGIN:
		strcpy(buf,"This FTP URL is authenticated through logging into the\nFTP server machine." );
		break;
	case HTAA_UNKNOWN:
	default:
		strcpy(buf,"The authentication method for this URL is unknown.\nA default of no authentication was used, which was okayed\nby the server." );
		break;
	}
	application_user_info_wait(buf);
	return;
*/
}

/* ----------------------- editable URL field callback -------------------- */
/* If the user hits return in the URL text field at the top of the display, */
/* then go fetch that URL  -- amb                                           */

static XmxCallback (url_field_cb)
{
	mo_window *win=(mo_window*)client_data;
	char *url,*xurl;
	RequestDataStruct rds;

	url = XmTextGetString (win->url_widget);
	if (!url || (!strlen(url)))
		return;
	mo_convert_newlines_to_spaces (url);
	xurl=UrlGuess(url);
	rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	MMPafLoadHTMLDocInWin (win, &rds);

	if (xurl==url) {
		free(xurl);
	} else {
		free(xurl);
		free(url);
	}
	return;
}

static mo_window * get_frame_target( mo_window *win, char *base_target, char* target)
{
	mo_window * pwin, *son;
	int i;
	char * default_target = "_self";	/* default target for mMosaic */

	if (win->frame_parent == NULL)
		return win;
	if ( target == NULL) 
		target = base_target;
	if ( target == NULL)
		target = default_target;
/* _self   : load the document in the same frame as the element that refers
             to this target
/* _blank  : load doc. in a new , unamed window
/* _parent : load doc. into the immediate FRAMESET parent if the current frame.
             equiv. to _self if current frame has no parent
/* _top    : load doc. into full, original window (canceling all other frames).
             equiv to _self if current frame has no parent
*/
	if ( !strcmp(target, "_self") ) {
		return win;
	}
	if ( !strcmp(target, "_top") ) {
		pwin = win->frame_parent;
		pwin->frame_name = NULL;
		pwin->frame_parent =NULL;
		pwin->frame_sons = NULL;
		pwin->frame_sons_nbre = 0;
		return pwin;
	}
	if ( !strcmp(target, "_blank") ) {     /* like top */
		pwin = win->frame_parent;
		pwin->frame_name = NULL;
		pwin->frame_parent =NULL;
		pwin->frame_sons = NULL;
		pwin->frame_sons_nbre = 0;
		return pwin;
	}
	if ( !strcmp(target, "_parent") ) {	/* like top */
		pwin = win->frame_parent;
		pwin->frame_name = NULL;
		pwin->frame_parent =NULL;
		pwin->frame_sons = NULL;
		pwin->frame_sons_nbre = 0;
		return pwin;
	}

	pwin = win->frame_parent;
	for( i = 0; i < pwin->frame_sons_nbre; i++){
		son = pwin->frame_sons[i];
		if (strcmp(son->frame_name, target) == 0){
			return son;
		}
	}
/* not found=> equiv to top */
	pwin->frame_name = NULL;
	pwin->frame_parent =NULL;
	pwin->frame_sons = NULL;
	pwin->frame_sons_nbre = 0;
	return pwin;
}

/*
 * name:    anchor_cb
 * purpose: Callback for triggering anchor in HTML widget.
 * inputs:  
 *   - as per XmxCallback
 * returns: 
 *   nothing
 * remarks: 
 *   This is too complex and should be broken down.
 *   We look at the button event passed in through the callback;
 *   button1 == same window, button2 == new window.
 *   Call mo_open_another_window or MMPafLoadHTMLDocInWin to get
 *   the actual work done.
 */
static void anchor_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	char *href, *reftext;
	mo_window *win = (mo_window*)client_data;
	XButtonReleasedEvent *event = 
	      (XButtonReleasedEvent *)((WbAnchorCallbackData *)call_data)->event;
	int force_newwin = (event->button == Button2 ? 1 : 0);
	WbAnchorCallbackData *wacd = (WbAnchorCallbackData *)call_data;
	RequestDataStruct rds;
	int is_shifted = event->state & ShiftMask;

/* check if w is equal to win->scrolled_win */
	if ( w != win->scrolled_win)
		abort();
	
	if (wacd->href)
		href = strdup (wacd->href);
	else
		href = strdup ("Unlinked");

	if ( *href == '#' ) {	/* anchor in doc */
		HTMLGotoAnchor(w, href + 1);
		free(href);
		return;
	}

/* it is a mailto: anchor */
	if (!strncasecmp (href, "mailto:", 7)){ /* mailto:user@... */
		char *email = href+7;
		char * subj = wacd->title;

		if (!*email){
			free(href);
			href = strdup("no-email-address");
			email = href;
		}
		mo_post_mailto_win(win, email, subj);
		free (href);
		return;
	}

/*Just do MMPafLoadHTMLDocInWin go get xterm forked off. */
	if (!strncmp (href, "telnet:", 7) || !strncmp (href, "rlogin:", 7)) {
		fprintf(stderr,"anchor_cb: Bug with telnet/rlogin anchor\n");
		fprintf(stderr,"Please report\n");
		abort();
/*		MMPafLoadHTMLDocInWin (win, href); */
		return;
	}
#ifdef MULTICAST
	if (win->mc_type == MC_MO_TYPE_RCV_ALL){
		force_newwin =1;
	}
#endif
	if (mMosaicAppData.protect_me_from_myself) {
		int answer = XmxModalYesOrNo (win->base, mMosaicAppContext,
	 "BEWARE: mMosaic disclaims all responsibility regarding your emotional and mental health.\n\nAre you *sure* you want to follow this hyperlink?" , "I'm sure.",
		"No! Get me outta here." );
		if (!answer)
			return;
	}
	if (((WbAnchorCallbackData *)call_data)->text)
		reftext = strdup (((WbAnchorCallbackData *)call_data)->text);
	else
		reftext = strdup ("Untitled");

	mo_convert_newlines_to_spaces (href);
	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	if (!force_newwin){
		rds.req_url = href;
		rds.gui_action = HTML_LOAD_CALLBACK;
		win = get_frame_target(win, win->current_node->base_target, wacd->target);
		MMPafLoadHTMLDocInWin (win, &rds);
	} else {
		if (is_shifted) {
			char * url = mo_url_canonicalize(href,
					win->current_node->base_url);
			PopSaveLinkFsbDialog(url);
		} else {
			mo_window * nwin;
			char *url = mo_url_canonicalize_keep_anchor(href,
				win->current_node->base_url);

			nwin = mo_make_window(win, MC_MO_TYPE_UNICAST);
			rds.req_url = url;
	rds.gui_action = HTML_LOAD_CALLBACK;
			MMPafLoadHTMLDocInWin(nwin, &rds);
/* Now redisplay this window. because visited url*/
/*			mo_redisplay_window (win);*/
		}
	}
	free (href);
	return;
}

/* name:    anchor_visited_predicate (PRIVATE)
 * purpose: Called by the HTML widget to determine whether a given URL
 *          has been previously visited.
 * inputs:  
 *   - Widget   w: HTML widget that called this routine.
 *   - char *href: URL to test.
 * returns: 
 *   1 if href has been visited previously; 0 otherwise.
 * remarks: 
 *   All this does is canonicalize the URL and call
 *   mo_been_here_before_huh_dad() to figure out if we've been
 *   there before.
 ****************************************************************************/
int anchor_visited_predicate (Widget w, char *href, char * base_url)
{
	int rv;

	if (!mMosaicAppData.track_visited_anchors || !href)
		return 0;

	/* This doesn't do special things for data elements inside
	 * an HDF file, because it's faster this way. */

	href = mo_url_canonicalize (href, base_url);
	rv = (mo_been_here_before_huh_dad (href) == mo_succeed ? 1 : 0);
	free (href);
	return rv;
}

static void pointer_motion_callback (Widget w, XtPointer clid, XtPointer calld)
{
	PointerMotionCBStruct *pmcbs = (PointerMotionCBStruct*) calld;
	XmString xmstr;
	char *to_free = NULL, *to_free_2 = NULL;
	mo_window * win = (mo_window*) clid;
	char * href= pmcbs->href;

#ifdef MULTICAST
		/* Si je suis l'emetteur */
	if (mc_send_win) {
		if (mc_send_win == win || win->frame_parent == mc_send_win ) {
			XEvent *ev = pmcbs->ev;
				/* - recuperer x et y du pointeur */
				/* - translater les coordonee because les frames */
				/* - envoyer en multicast */
			McEmitCursor(mc_send_win, ev);
		}
	}
#endif
	if (!mMosaicAppData.track_pointer_motion)
		return;
	if (href && *href) {
		href = mo_url_canonicalize_keep_anchor (href, win->current_node->base_url);
		to_free = href;
		mo_convert_newlines_to_spaces (href); /* Sigh... */

		/* This is now the option wherein the URLs are just spit up there;
		 * else we put up something more friendly. */
		if (mMosaicAppData.track_full_url_names) {
			/* Everything already done... */
		} else {
			/* This is where we go get a good description. */
			/* href = HTDescribeURL (href); */
			/* to_free_2 = href; */
		}
	} else 
		href = " ";

	xmstr = XmStringCreateSimple (href);
	XtVaSetValues (win->tracker_widget, XmNlabelString, (XtArgVal)xmstr, NULL);
	XmStringFree (xmstr);
	if (to_free)
		free (to_free);
	if (to_free_2)
		free (to_free_2);
}

XmxCallback (submit_form_callback)
{
	mo_window *win = (mo_window*) client_data;
	char *url = NULL, *method = NULL, *enctype = NULL, *query;
	int len, i;
	WbFormCallbackData *cbdata = (WbFormCallbackData *)call_data;
	int do_post_urlencoded = 0;
	RequestDataStruct rds;

	if (!cbdata)
		return;
				/* Initial query: Breathing space. */
	len = 16;
				/* Add up lengths of strings. */
	for (i = 0; i < cbdata->attribute_count; i++) {
		if (cbdata->attribute_names[i]) {
			len += strlen (cbdata->attribute_names[i]) * 3;
			if (cbdata->attribute_values[i])
				len += strlen (cbdata->attribute_values[i]) * 3;
		}
		len += 2;
	}
	if (cbdata->href && *(cbdata->href)) /* Get the URL. */
		url = cbdata->href;
	else
		url = win->current_node->base_url;

	if (cbdata->method && *(cbdata->method))
		method = cbdata->method;
	else
		method = strdup ("GET");
			/* Grab enctype if it's there. */
	if (cbdata->enctype && *(cbdata->enctype))
		enctype = cbdata->enctype;
#ifdef DEBUG_FORM
	if (mMosaicSrcTrace) {
		fprintf (stderr, "[submit_form_callback] method is '%s' enctype is '%s'\n",
				method, enctype);
	}
#endif
	if (strcasecmp (method, "POST") == 0 )
		do_post_urlencoded = 1;

	len += strlen (url);
	query = (char *)malloc (sizeof (char) * len);
	if (!do_post_urlencoded) {
		strcpy (query, url);
		strtok (query, "#");	 /* Clip out anchor. */
		strtok (query, "?");	 /* Clip out old query. */
		if (query[strlen(query)-1] != '?')
			strcat (query, "?");
	} else 
		query[0] = 0;
				/* Take isindex possibility into account. */
	if (cbdata->attribute_count == 1 &&
	    strcmp (cbdata->attribute_names[0], "isindex") == 0) {
		if (cbdata->attribute_values[0]) {
			char *c = EscapeUrl (cbdata->attribute_values[0]);
			strcat (query, c);
			free (c);
		}
	} else {
		for (i = 0; i < cbdata->attribute_count; i++) {
			/* For all but the first, lead off with an ampersand. */
			if (i != 0)
				strcat (query, "&");
			/* Rack 'em up. */
			if (cbdata->attribute_names[i]) {
				char *c = EscapeUrl(
					cbdata->attribute_names[i]);
				strcat (query, c);
				free (c);
				strcat (query, "=");
				if (cbdata->attribute_values[i]) {
					c = EscapeUrl(
						cbdata->attribute_values[i]);
					strcat (query, c);
					free (c);
				}
			}
		}
	}
	if (do_post_urlencoded) {
		rds.req_url = url;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.ct = "application/x-www-form-urlencoded";
		rds.post_data = query;
		rds.is_reloading = True;
		MMPafLoadHTMLDocInWin(win, &rds);
	} else {
		rds.req_url = query;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.ct = NULL;
		rds.post_data = NULL;
		rds.is_reloading = True;
		MMPafLoadHTMLDocInWin (win, &rds);
	}
	if (query)
		free (query);
}

XmxCallback (frame_callback)
{
	mo_window *win = (mo_window*) client_data;
	mo_window *sub_win=NULL, *parent;
	Widget htmlw;
	char *url = NULL , *frame_name;
	XmHTMLFrameCallbackStruct *cbs = (XmHTMLFrameCallbackStruct *)call_data;
	RequestDataStruct rds;

	if (!cbs) {
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: NULL call_data\n");
#endif
		return;
	}
	switch (cbs->reason) {
	case XmCR_HTML_FRAMEDONE:
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMEDONE\n");
#endif
		break;
	case XmCR_HTML_FRAMEDESTROY:
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMEDESTROY\n");
#endif
		break;
	case XmCR_HTML_FRAMECREATE:
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMECREATE\n");
#endif
		break;
	case XmCR_HTML_FRAMESETDESTROY:
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMESETDESTROY\n");
#endif
		parent = win;
		free(parent->frame_sons) ;
                win->frame_name = NULL;
                win->frame_parent =NULL;
                win->frame_sons = NULL;
		win->frame_sons_nbre =0;
		win->number_of_frame_loaded = 0;
		break;
	case XmCR_HTML_FRAMESET_INIT:
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMESET_INIT\n");
#endif
                win->frame_name = NULL;
                win->frame_parent =NULL;
                win->frame_sons = NULL;
/*		win->frame_sons_nbre =0; */
/*??? = cbs.nframe */
		win->frame_sons_nbre =  cbs->nframe ;
		win->number_of_frame_loaded = 0;
		parent = win;
		parent->frame_sons = (mo_window **) realloc(parent->frame_sons,
			(parent->frame_sons_nbre) * sizeof(mo_window *));
		break;
	default:
		abort();
#ifdef DEBUG_FRAME
		fprintf(stderr, "frame_callback: reason: Unknowed...\n");
#endif
		break;
	}
	if (cbs->reason != XmCR_HTML_FRAMEDONE ){
		return;		/* just for test now */
	}
#ifdef DEBUG_FRAME
	fprintf(stderr,"cbs.event = %08x\n cbs.src = %s\n cbs.name = %s\n",
		cbs->event, cbs->src, cbs->name);
	fprintf(stderr,"bs.html = %08x\n cbs.doit = %d\n",
		cbs->html, cbs->doit);
#endif
/* reason = XmCR_HTML_FRAMEDONE */
	parent = win;
	htmlw = cbs->html;
	url = strdup(cbs->src);
	frame_name = cbs->name;
	sub_win = MMMakeSubWindow(parent, htmlw, url, frame_name);

	sub_win->frame_sons_nbre = 0;
	sub_win->frame_dot_index = cbs->index;
	parent->frame_sons[cbs->index] = sub_win;
	
	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = url;
	rds.gui_action = FRAME_CALLBACK;
	MMPafLoadHTMLDocInWin (sub_win, &rds);
	free(url);
}
/* Exported to libwww2. */
void mo_gui_notify_progress (char *msg, mo_window * win)
{
	XmString xmstr;
/*#################*/

	if (!mMosaicAppData.track_pointer_motion)
		return;
	if (!msg)
		msg = " ";
	xmstr = XmStringCreateSimple (msg);
	XtVaSetValues (win->tracker_widget, XmNlabelString, (XtArgVal)xmstr, NULL);
	XmStringFree (xmstr);
	XmUpdateDisplay (win->base);
	return;
}

/*SWP 7/3/95*/
void mo_gui_check_security_icon_in_win(int type, mo_window *win)
{
/*
	Pixmap pix;
	static int current=HTAA_NONE;

	if (!mMosaicAppData.securityIcon) 
		return;
	switch (type) {
	case HTAA_UNKNOWN:
		pix=securityUnknown;
		current=type;
		break;
	case HTAA_NONE:
		pix=securityNone;
		current=type;
		break;
	case HTAA_KERBEROS_V4:
		pix=securityKerberos4;
		current=type;
		break;
	case HTAA_KERBEROS_V5:
		pix=securityKerberos5;
		current=type;
		break;
	case HTAA_MD5:
		pix=securityMd5;
		current=type;
		break;
	case HTAA_BASIC:
		pix=securityBasic;
		current=type;
		break;
	case HTAA_DOMAIN:
		pix=securityDomain;
		current=type;
		break;
	case HTAA_LOGIN:
		pix=securityLogin;
		current=type;
		break;
	default:
		pix=securityUnknown;
		current=type;
		break;
	}

	if ((char *)pix != NULL) {
		XtVaSetValues(win->security,
			XmNlabelPixmap, pix,
			XmNlabelType, XmPIXMAP,
			NULL);
	}
	UpdateButtons (win);
	XmUpdateDisplay (win->base);
	return;
*/
}

void twirl_icon_cb(XtPointer clid, XtIntervalId * id)
{
	TwirlStruct * ts = (TwirlStruct*) clid;

	AnimatePixmapInWidget(ts->logo_widget,IconPix[ts->logo_count]);
	ts->logo_count++;
	if (ts->logo_count >= NUMBER_OF_FRAMES)
		ts->logo_count = 0;
	XFlush(mMosaicDisplay);
	ts->time_id = XtAppAddTimeOut(mMosaicAppContext,
                100L, twirl_icon_cb, ts);
}

void mo_gui_apply_default_icon( mo_window *win)
{
        XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);
}

extern void ungrab_the_____ing_pointer(XtPointer client_data);
void mo_gui_done_with_icon (mo_window * win)
{
	XClearArea(mMosaicDisplay, XtWindow(win->logo), 0, 0, 0, 0, True);
	/* this works dammit (trust me) - TPR */
	XtAppAddTimeOut(mMosaicAppContext, 10,
                    (XtTimerCallbackProc)ungrab_the_____ing_pointer, NULL);
}

/* name:    mo_view_keypress_handler (PRIVATE)
 * purpose: This is the event handler for the HTML widget and associated
 *          scrolled window; it handles keypress events and enables the
 *          hotkey support.
 * inputs:  
 *   - as per XmxEventHandler
 * returns: 
 *   nothing
 * remarks: 
 *   Hotkeys and their actions are currently hardcoded.  This is probably
 *   a bad idea, and Eric hates it.
 */
static void mo_view_keypress_handler(Widget w, XtPointer clid,
	XEvent *event, Boolean *cont)
{
	mo_window *win = (mo_window*) clid;
	int _bufsize = 3, _count;
	char _buffer[3];
	KeySym _key;
	XComposeStatus _cs;
	Widget sb;
	String params[1];

/* Go get ascii translation. */
	_count = XLookupString (&(event->xkey), _buffer, _bufsize, 
                          	&_key, &_cs);

/* I don't know why this is necessary but for some reason the rbm was making
 *	the above function return 0 as the _key, this fixes it -- TPR */
	if(!_key)
		_key=XKeycodeToKeysym(XtDisplay(win->view),event->xkey.keycode,0);
  
	_buffer[_count] = '\0';		 /* Insert trailing Nil. */
	params[0] = "0";
	switch(_key){
	case XK_Prior: /* Page up. */
	case XK_KP_Prior:
	case XK_BackSpace:
	case XK_Delete:
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XtCallActionProc (sb, "PageUpOrLeft", event, params, 1);
		}
		break;
	case XK_Next:  /* Page down. */
	case XK_KP_Next:
	case XK_Return:
	case XK_space:
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XtCallActionProc (sb, "PageDownOrRight", event, params, 1);
		}
		break;
	case XK_Home: /* Home -- Top */
		HTMLGotoId(win->scrolled_win, 0,0);
		break;
	case XK_End: /* End -- Bottom */
		HTMLGotoId(win->scrolled_win, HTMLLastId(win->scrolled_win),0);
		break;
	case XK_Down:
	case XK_KP_Down:
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XtCallActionProc (sb, "IncrementDownOrRight", event, params, 1);
		}
		break;
	case XK_Right:
	case XK_KP_Right:
		params[0] = "1";
		XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XtCallActionProc (sb, "IncrementDownOrRight", event, params, 1);
		}
		break;
	case XK_Up:
	case XK_KP_Up:
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)){
			XtCallActionProc (sb, "IncrementUpOrLeft", event, params, 1);
		}
		break;
	case XK_Left:
	case XK_KP_Left:
		params[0] = "1";
		XtVaGetValues (win->scrolled_win, XmNhorizontalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XtCallActionProc (sb, "IncrementUpOrLeft", event, params, 1);
		}
		break;
#ifdef NEWS
      case XK_less:
          gui_news_prevt(win);
          break;
      case XK_greater:
          gui_news_nextt(win);
          break;
      case XK_comma:
          gui_news_prev(win);
	  break;
      case XK_period:
          gui_news_next(win);
          break;
#endif
      case XK_B: /* Back. */
      case XK_b:
          mo_back (w, (XtPointer)win, NULL);
          break;
      case XK_C: /* Clone */
      case XK_c:
		{
			mo_window * neww;
			RequestDataStruct rds;

			rds.ct = rds.post_data = NULL;
			rds.is_reloading = False;
			rds.req_url = win->current_node->aurl_wa;
	rds.gui_action = HTML_LOAD_CALLBACK;
			neww = mo_make_window(win,MC_MO_TYPE_UNICAST);
			MMPafLoadHTMLDocInWin (neww, &rds);
		}
		break;
      case XK_D: /* Document source. */
      case XK_d:
          mo_document_source (w, (XtPointer) win, NULL);
          break;
      case XK_E: /* Edit */
      case XK_e:
          mo_edit_source (win);
          break;
      case XK_F:
      case XK_f:
          mo_forward (w, (XtPointer) win, NULL);
          break;
      case XK_H: /* Hotlist */
          mo_post_hotlist_win (win);
          break;
      case XK_h: /* History */
          mo_post_history_win (win);
          break;
      case XK_L: /* Open Local */
      case XK_l:
          mo_post_open_local_window (win);
          break;
      case XK_M: /* Mailto */
      case XK_m:
          mo_post_mail_window (win);
          break;
      case XK_N: /* New */
      case XK_n:
          mo_open_another_window (win, mMosaicAppData.home_document);
        break;                        
      case XK_O: /* Open */
      case XK_o:
          mo_post_open_window (win);
          break;
      case XK_P: /* Print */
      case XK_p:
          mo_post_print_window (win); 
          break;
      case XK_r: /* reload */
          mo_reload_document(w, (XtPointer) win, NULL);
          break;
      case XK_R: /* Refresh */
          mo_refresh_window_text (win);
          break;
      case XK_S: /* Search. */
      case XK_s:
          mo_post_search_window (win);
          break;
      case XK_Escape:
          mo_delete_window (win);
          break;
      default:
          break;
      }
  return;
}

/* Callback to redraw the meter -bjs */
static void ResizeMeter(Widget meter, XtPointer client, XtPointer call);

static void DrawMeter(Widget meter, XtPointer client, XtPointer call)
{
    mo_window *win = (mo_window *) client;
    GC gc;
    long mask = 0;
    XGCValues values;
    int l;
    char *ss;
    char s[256];
    static int last_len=0;
    int current_len;
    int resize=0;
    static char *finished="100%";

    gc = XtGetGC( meter,0,NULL);
    if(win->meter_width== -1) {
	resize=1;
	ResizeMeter(meter,(XtPointer)win,NULL);
    }

    if(win->meter_width== -1) ResizeMeter(meter,(XtPointer)win,NULL);
    current_len=(win->meter_width * win->meter_level)/100;

    XSetForeground( XtDisplay(meter),gc,win->meter_bg);
    XFillRectangle(XtDisplay(meter),XtWindow(meter),gc,
		   0,0,win->meter_width,win->meter_height);

    XSetForeground( XtDisplay(meter),gc,win->meter_fg);
    XFillRectangle(XtDisplay(meter),XtWindow(meter),gc,
		   0,0,current_len,
                   win->meter_height);

    if (win->meter_level==100 && !win->meter_text) {
	win->meter_text=finished;
    }

    if(!win->meter_notext && (win->meter_level<=100 || win->meter_text)){

	if (!win->meter_font) {
		XtVaGetValues(win->scrolled_win,
			      XtNfont, &(win->meter_font),
			      NULL);
		if (!win->meter_font) {
			puts("METER Cannot Get Font -- Please set 'mMosaic*MeterFont: NEW_FONT'\n  Where NEW_FONT is a 14 point font on your system.");
			win->meter_notext=1;
		} else {
			win->meter_notext=0;
			win->meter_fontW = win->meter_font->max_bounds.rbearing;
			win->meter_fontH = win->meter_font->max_bounds.ascent;

		}
	}

	if (!win->meter_notext) {
		if(!win->meter_text) {
			l = 3;
			ss = s;
			s[0]=win->meter_level>9 ? '0'+(win->meter_level/10) : ' ';
			s[1]='0'+(win->meter_level%10);
			s[2]='%';
			s[3]=0;
		} else {
			ss = win->meter_text;
			l = strlen(ss);
		}

		values.font = win->meter_font->fid;
		mask = GCFont;
		XChangeGC(XtDisplay(meter), gc,  mask, &values);
		XSetForeground( XtDisplay(meter),gc,win->meter_font_bg);
		XDrawString(XtDisplay(meter), XtWindow(meter), gc, 
			    (win->meter_width/2-(win->meter_fontW*l)/2)+2,
			    ((win->meter_height/2)+(win->meter_fontH/2)), 
			    ss, l);
                XSetForeground( XtDisplay(meter),gc,win->meter_font_fg);
                XDrawString(XtDisplay(meter), XtWindow(meter), gc,
                            (win->meter_width/2-(win->meter_fontW*l)/2),
                            ((win->meter_height/2)+(win->meter_fontH/2))-2,
                            ss, l);
	}
    }
    last_len=current_len;
    XtReleaseGC(meter,gc);
}

static void ResizeMeter(Widget meter, XtPointer client, XtPointer call)
{
	XWindowAttributes wattr;
	mo_window *win = (mo_window *) client;

	if(!XtWindow(meter)) 
		return;
	XGetWindowAttributes(mMosaicDisplay, XtWindow(meter),&wattr);
	win->meter_width = wattr.width;
	win->meter_height = wattr.height;
}

/* Exported to libwww2 */
void mo_gui_update_meter(int level, char *text, mo_window * win)
{
    win->meter_text = text;

    if(win->meter_level == -1) return;
    if(level<0) level = 0;
    if(level>100) level = 100;
    win->meter_level = level;
    DrawMeter(win->meter,(XtPointer) win, NULL);
}


struct tool mo_tools[] = { 
    {"<-","Previous page",
	mo_back,&toolbarBack, &toolbarBackGRAY, moMODE_ALL, NULL},
    {"->","Next page",
	mo_forward,&toolbarForward, &toolbarForwardGRAY, moMODE_ALL, NULL},
    {"Rel","Reload this page",
	mo_reload_document,&toolbarReload,NULL, moMODE_ALL, NULL},
    {"Home","Go Home!",
	mo_home_document,&toolbarHome,NULL, moMODE_ALL, NULL},
    {"Open","Open a new URL",
	mo_open_document,&toolbarOpen,NULL, moMODE_ALL, NULL},
    {"Save","Save current page as ...",
	mo_save_document,&toolbarSave,NULL, moMODE_ALL, NULL},
    {"New","Create a new Mosaic window",
	mo_new_window,&toolbarNew,NULL, moMODE_ALL, NULL},
    {"Clone","Clone this Mosaic window",
	mo_clone_window,&toolbarClone,NULL, moMODE_ALL, NULL},
    {"Close","Destroy this Mosaic window",
	mo_close_window,&toolbarClose,NULL, moMODE_ALL, NULL},
    {"+ Hot","Add current page to hotlist",
	mo_register_node_in_default_hotlist,&toolbarAddHotlist,NULL, moMODE_PLAIN, NULL},
    {"Find","Search this document", 
	mo_search, &toolbarSearch, NULL, moMODE_ALL, NULL},
    {"Prt","Print this document", 
	mo_print_document, &toolbarPrint, NULL, moMODE_ALL, NULL},
#ifdef NEWS
    {"Grps","Newsgroups index",
	mo_news_groups,&toolbarNewsGroups, NULL, moMODE_ALL, NULL},
/* News Mode */
    {"Idx","Newsgroup article index",
	mo_news_index,&toolbarNewsIndex, NULL, moMODE_NEWS, NULL},
    {"<Thr","Go to previous thread",
	mo_news_prevt,&toolbarNewsFRev, &toolbarNewsFRevGRAY, moMODE_NEWS, NULL},
    {"<Art","Go to previous article",
	mo_news_prev,&toolbarNewsRev, &toolbarNewsRevGRAY, moMODE_NEWS, NULL},
    {"Art>","Go to next article",
	mo_news_next,&toolbarNewsFwd, &toolbarNewsFwdGRAY, moMODE_NEWS, NULL},
    {"Thr>","Go to next thread",
	mo_news_nextt,&toolbarNewsFFwd, &toolbarNewsFFwdGRAY, moMODE_NEWS, NULL},
    {"Post","Post a UseNet Article",
	mo_news_post,&toolbarPost, &toolbarPostGRAY, moMODE_NEWS, NULL},
    {"Foll","Follow-up to UseNet Article",
	mo_news_follow,&toolbarFollow, &toolbarFollowGRAY, moMODE_NEWS, NULL},
#endif
    {NULL, NULL, 0, NULL, NULL, 0, NULL}
};

/* NOTE: THESE MUST COINCIDE EXACTLY WITH mo_tools!!! */
char *tool_names[] = {
	"BACK", "FORWARD", "RELOAD", "HOME",
	"OPEN", "SAVE", "NEW", "CLONE", "CLOSE",
	"ADD_TO_HOTLIST", "FIND", "PRINT",
#ifdef NEWS
	"GROUPS", "INDEX", "PREVIOUS_THREAD", "PREVIOUS_ARTICLE", "NEXT_ARTICLE",
	"NEXT_THREAD", "POST", "FOLLOW_UP",
#endif
	NULL
};

void mo_make_globe(mo_window *win, Widget parent, int small)
{
	if(!small){
		IconPix = IconPixBig;
		IconWidth = IconHeight = 64;
		WindowWidth = WindowHeight = 0;
	} else {
		IconPix = IconPixSmall;
		IconWidth = IconHeight = 32;
		WindowWidth = WindowHeight = 0;
	}

	win->logo = XmxMakePushButton(parent, "logo",
				icon_pressed_cb, (XtPointer)win);
	XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);
	XtVaSetValues(win->logo,
			XmNmarginWidth, 0,
			XmNmarginHeight, 0,
			XmNmarginTop, 0,
			XmNmarginBottom, 0,
			XmNmarginLeft, 0,
			XmNmarginRight, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment, !win->biglogo || (!win->smalllogo && win->toolbardetached) ? XmATTACH_FORM : XmATTACH_NONE,
			XmNrightAttachment, XmATTACH_FORM,
                  XmNtraversalOn, False,
			NULL);

	if(win->biglogo){
		if(win->smalllogo){
			mo_extra_buttons(win,win->slab[SLAB_GLOBE]);
			XtVaSetValues(win->security,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_NONE,
				NULL);
			XtVaSetValues(win->encrypt,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				XmNleftAttachment, XmATTACH_WIDGET,
				XmNleftWidget, win->security,
				XmNrightAttachment, XmATTACH_WIDGET,
				XmNrightWidget, win->logo,
				NULL);
		} else {
			if(!win->toolbardetached){
				mo_extra_buttons(win,win->slab[SLAB_GLOBE]);
				XtVaSetValues(win->security,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_NONE,
					XmNleftAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_WIDGET,
					XmNrightWidget, win->logo,
					NULL);
				XtVaSetValues(win->encrypt,
					XmNtopAttachment, XmATTACH_WIDGET,
					XmNtopWidget, win->security,
					XmNbottomAttachment, XmATTACH_FORM,
					XmNleftAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_WIDGET,
					XmNrightWidget, win->logo,
					NULL);
			}           
		}
	}
	XtVaSetValues(win->logo,
		XmNuserData, (XtPointer)AlloBalloonInfoData(win,  "Logo Button - Abort a Transaction"),
		NULL);
	XtOverrideTranslations(win->logo, XtParseTranslationTable(mMosaicBalloonTranslationTable));
}

void mo_tool_detach_cb(Widget wx, XtPointer cli, XtPointer call)
{
    Atom WM_DELETE_WINDOW;
    mo_window *win = (mo_window *) cli;
    int h,w;

        /* Xmx sucks */
    XtUnmanageChild(XtParent(win->scrolled_win));
    if(win->toolbardetached){
        win->toolbardetached = 0;
        XtUnmanageChild(win->toolbarwin);
        XtDestroyWidget(win->toolbarwin);
        win->toolbarwin = NULL;
        win->topform = win->slab[SLAB_TOOLS];
        mo_fill_toolbar(win,win->topform,0,0);        
        if(win->biglogo && !win->smalllogo){
            mo_make_globe(win, win->slab[SLAB_GLOBE], 0);
        }
    } else {
        win->toolbardetached = 1;
        XtUnmanageChild(win->button_rc);
        XtUnmanageChild(win->button2_rc);
        XtDestroyWidget(win->button_rc);
        XtDestroyWidget(win->button2_rc);
        if(win->biglogo && !win->smalllogo){
            XtUnmanageChild(win->logo);
            XtDestroyWidget(win->logo);
            XtUnmanageChild(win->security);
            XtDestroyWidget(win->security);
            XtUnmanageChild(win->encrypt);
            XtDestroyWidget(win->encrypt);            
        }
        win->toolbarwin = XtVaCreatePopupShell ("ToolBox",
             xmDialogShellWidgetClass,
             win->base,
             XmNminHeight, h = win->toolbarorientation?640:40,
             XmNminWidth, w = win->toolbarorientation?40:640,
             XmNmaxHeight, h,
             XmNmaxWidth, w,
             XmNheight, h,
             XmNwidth, w,
             XmNallowShellResize, FALSE,
             NULL);
        XtManageChild(win->toolbarwin);
        win->topform = XtVaCreateWidget ("slab_tools",
             xmFormWidgetClass, win->toolbarwin, 
             XmNminHeight, h,
             XmNminWidth, w,
             XmNmaxHeight, h,
             XmNmaxWidth, w,
             XmNheight, h,
             XmNwidth, w,
             NULL);
        mo_fill_toolbar(win,win->topform,0,0);
        XtManageChild(win->topform);
        WM_DELETE_WINDOW = XmInternAtom(mMosaicDisplay, "WM_DELETE_WINDOW", False);
        XmAddWMProtocolCallback(win->toolbarwin, WM_DELETE_WINDOW,
                                mo_tool_detach_cb, (XtPointer)win);
        XtPopup(win->toolbarwin, XtGrabNone);
    }    
    XtManageChild(XtParent(win->scrolled_win));
}

void mo_switch_mode(mo_window *win)
{
	int i;

	for(i=0;mo_tools[i].label;i++){
		if(win->tools[i].w){
			if(!(mo_tools[i].toolset & win->mode)) {
				if(XtIsManaged(win->tools[i].w))
					XtUnmanageChild(win->tools[i].w);
			} else {   
				if(!XtIsManaged(win->tools[i].w))
					XtManageChild(win->tools[i].w);
			}
		}
	}
}

void mo_tool_state(struct toolbar *t,int state,int index)
{
	XmxSetSensitive (t->w, t->gray = state);
}

static void mo_extra_buttons(mo_window *win, Widget top)
{
    win->security = XmxMakePushButton (top, "sec",
                                            security_pressed_cb, (XtPointer)win);
    XmxApplyPixmapToLabelWidget (win->security, securityUnknown);
    XtVaSetValues(win->security,
                  XmNmarginWidth, 0,
                  XmNmarginHeight, 0,
                  XmNmarginTop, 0,
                  XmNmarginBottom, 0,
                  XmNmarginLeft, 0,
                  XmNmarginRight, 0,
                  XmNuserData, (XtPointer) AlloBalloonInfoData(win,"Security Stats Information"),
                  XmNtraversalOn, False,
                  NULL);
    XtOverrideTranslations(win->security,
                            XtParseTranslationTable(mMosaicBalloonTranslationTable));
    win->encrypt = XtVaCreateManagedWidget(" ", xmPushButtonWidgetClass, top,
         XmNmarginWidth, 0,
         XmNmarginHeight, 0,
         XmNmarginTop, 0,
         XmNmarginBottom, 0,
         XmNmarginLeft, 0,
         XmNmarginRight, 0,
         XmNuserData, (XtPointer) AlloBalloonInfoData(win, "Encryption Status (not in this release)"),
         XmNtraversalOn, False,       
         NULL);
    XmxApplyPixmapToLabelWidget (win->encrypt, enc_not_secure);
    XtOverrideTranslations(win->encrypt,XtParseTranslationTable(mMosaicBalloonTranslationTable));
            /* insure we set the security icon! */
    if (win->current_node) {
        mo_gui_check_security_icon_in_win(win->current_node->authType,win);
    }
}

/* create topform and fill it with toolbar bits'n'pieces */
Widget mo_fill_toolbar(mo_window *win, Widget top, int w, int h)
{
	Widget tearbutton;
	int i,vert = win->toolbarorientation && win->toolbardetached;
	int textbuttons = win->texttools;
	static XFontStruct *tmpFont=NULL;
	static XmFontList tmpFontList;

	if (!tmpFont) {
		XtVaGetValues(win->scrolled_win,
			XtNfont, &tmpFont,
			NULL);
		if (!tmpFont) {
			fprintf(stderr,"Toolbar Font: Could not load! The X Resource is Mosaic*ToolbarFont\nDefault font is: -adobe-times-bold-r-normal-*-12-*-*-*-*-*-iso8859-1\nExiting Mosaic.");
			exit(1);
		}
		tmpFontList = XmFontListCreate(tmpFont,XmSTRING_DEFAULT_CHARSET);
	}
					/* Which tools to show */
	win->topform = top;
					/* Xmx sucks */
	win->button2_rc = XtVaCreateWidget ("buttonrc2", xmRowColumnWidgetClass,
				win->topform,
				XmNorientation, vert?XmVERTICAL:XmHORIZONTAL,
				XmNmarginWidth, 0,
				XmNmarginHeight, 0,
				XmNspacing, 0,
				XmNleftOffset, 0,
				XmNrightOffset, 0,
				XmNtopOffset, 0,
				XmNbottomOffset, 0,
				XmNleftAttachment, XmATTACH_NONE,
				XmNrightAttachment, XmATTACH_FORM,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
	win->button_rc = XtVaCreateWidget ("buttonrc", xmRowColumnWidgetClass,
				win->topform,
				XmNorientation, vert?XmVERTICAL:XmHORIZONTAL,
				XmNpacking, XmPACK_TIGHT,
				XmNmarginWidth, 0,
				XmNmarginHeight, 1,
				XmNspacing, 0,
				XmNleftOffset, 0,
				XmNrightOffset, 0,
				XmNtopOffset, 2,
				XmNbottomOffset, 2,
				XmNleftAttachment, XmATTACH_FORM,
				XmNrightAttachment, XmATTACH_WIDGET,
				XmNrightWidget, win->button2_rc,
				XmNtopAttachment, XmATTACH_FORM,
				XmNbottomAttachment, XmATTACH_FORM,
				NULL);
	tearbutton = XtVaCreateManagedWidget ("|",xmPushButtonWidgetClass,
				win->button_rc,
				XmNuserData, (XtPointer)AlloBalloonInfoData(win, "Toolbar Tearoff Control"),
				XmNlabelType, textbuttons ? XmSTRING : XmPIXMAP,
				XmNlabelPixmap, vert?tearh:tearv,
				XmNfontList, tmpFontList,
         			XmNtraversalOn, False,
				NULL);
	XtOverrideTranslations(tearbutton,XtParseTranslationTable(mMosaicBalloonTranslationTable));
	XtAddCallback(tearbutton, XmNactivateCallback, mo_tool_detach_cb,
				(XtPointer) win);
	for(i=0;mo_tools[i].label;i++) {
		if(mo_tools[i].action){
			win->tools[i].w = XtVaCreateManagedWidget(
				mo_tools[i].text,
				xmPushButtonWidgetClass,
				win->button_rc,
				XmNuserData, (XtPointer)AlloBalloonInfoData(win, mo_tools[i].label),
				XmNmarginWidth, 0,
				XmNmarginHeight, 0,
				XmNmarginTop, 0,
				XmNmarginBottom, 0,
				XmNmarginLeft, 0,
				XmNmarginRight, 0,
				XmNalignment, XmALIGNMENT_CENTER,
				XmNlabelType,
					textbuttons ? XmSTRING : XmPIXMAP,
				XmNlabelPixmap, *(mo_tools[i].image),
				XmNfontList, tmpFontList,
               			XmNtraversalOn, False,
				NULL);
			XtOverrideTranslations(win->tools[i].w,
				XtParseTranslationTable(mMosaicBalloonTranslationTable));
			if(mo_tools[i].greyimage != NULL)
				XtVaSetValues(win->tools[i].w,
					XmNlabelInsensitivePixmap,
					*(mo_tools[i].greyimage),
					NULL);
			XmxSetSensitive(win->tools[i].w,
				win->tools[i].gray);
			XtAddCallback(win->tools[i].w,
				XmNactivateCallback, mo_tools[i].action,
				win);
			if(!(mo_tools[i].toolset & win->mode))
				XtUnmanageChild(win->tools[i].w);
		} else {
			win->tools[i].w=NULL;
			XtVaCreateManagedWidget(" ",xmSeparatorWidgetClass,
				win->button_rc,
				XmNorientation, vert?XmHORIZONTAL:XmVERTICAL, 
				vert?XmNheight:XmNwidth, vert?3:4,
				XmNseparatorType, XmNO_LINE,
                 		XmNtraversalOn, False,
				NULL);
		}
	}
	if(!win->biglogo || (!win->smalllogo && win->toolbardetached)){
		mo_extra_buttons(win, win->button2_rc);
		mo_make_globe(win, win->button2_rc, 1);
	}  
	XtManageChild(win->button2_rc);
	XtManageChild(win->button_rc);
	return (win->topform);
}

/*
 * name:    mo_fill_window (PRIVATE)
 * purpose: Take a new (empty) mo_window struct and fill in all the 
 *          GUI elements.
 * inputs:  
 *   - mo_window *win: The window.
 * returns: 
 *   mo_succeed
 */
#define SLABCOUNT (6)
static mo_status mo_fill_window (mo_window *win)
{
	Widget form;
	Widget url_label;
	int i;
  
	form=XtVaCreateManagedWidget("form0",xmFormWidgetClass,win->base,NULL);
	win->smalllogo = 0;
	win->texttools = 0;
	win->slabpart[0] = SLAB_MENU;
	win->slabpart[1] = SLAB_GLOBE;
	win->slabpart[2] = SLAB_TOOLS;
	win->slabpart[3] = SLAB_URL;
	win->slabpart[4] = SLAB_VIEW;
	win->slabpart[5] = SLAB_STATUS;
	win->biglogo=1;
	/* no active toolset, horiz, not detached */
	win->toolset = 0;
	win->toolbarorientation = 0;
	win->toolbardetached = 0;
	win->toolbarwin = NULL;

/*********************** SLAB_GLOBE ****************************/
	win->slab[SLAB_GLOBE] = XtVaCreateWidget("slab_globe",
                                        xmFormWidgetClass, form, NULL);
	mo_make_globe(win,win->slab[SLAB_GLOBE],0);

/*********************** SLAB_MENU ****************************/
	win->menubar = mo_make_document_view_menubar (form,win);
	win->slab[SLAB_MENU] = win->menubar->base;
	XtUnmanageChild(win->slab[SLAB_MENU]);

/*********************** SLAB_URL ****************************/
	win->slab[SLAB_URL] = XtVaCreateWidget("slab_url",
					xmFormWidgetClass, form,
					XmNheight, 36, NULL);
	url_label = XtVaCreateManagedWidget("URL:",xmLabelWidgetClass,
					win->slab[SLAB_URL],
					XmNleftOffset, 3,
					XmNleftAttachment, XmATTACH_FORM,
					XmNrightAttachment, XmATTACH_NONE,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_FORM,
					NULL);
	win->url_widget = XtVaCreateManagedWidget("text",xmTextFieldWidgetClass,
					win->slab[SLAB_URL],
					XmNrightOffset, 3,
					XmNleftOffset, 3,
					XmNtopOffset, 3,
					XmNleftAttachment, XmATTACH_WIDGET,
					XmNleftWidget, url_label,
					XmNrightAttachment, XmATTACH_FORM,
					XmNtopAttachment, XmATTACH_FORM,
					XmNbottomAttachment, XmATTACH_NONE,
					XmNcursorPositionVisible, True,
					XmNeditable, True,
                                        XmNtraversalOn, True,
					NULL);
	/* DO THIS WITH THE SLAB MANAGER - BJS */
	XtAddCallback (win->url_widget, XmNactivateCallback, url_field_cb, (XtPointer)win);

/*********************** SLAB_VIEW ****************************/
	win->slab[SLAB_VIEW]= win->scrolled_win= XtVaCreateManagedWidget ("view",
		htmlWidgetClass, form, 
       		XmNresizePolicy, XmRESIZE_ANY,
		WbNpreviouslyVisitedTestFunction, anchor_visited_predicate,
		XmNshadowThickness, 2,
		NULL);
/*########### a mettre sous forme de callback ################ */
	XtAddCallback(win->scrolled_win,
		WbNpointerMotionCallback, pointer_motion_callback,win);
	XtAddCallback (win->scrolled_win, WbNanchorCallback, anchor_cb, win);
	XtAddCallback (win->scrolled_win, WbNsubmitFormCallback,
					submit_form_callback, win);
#ifdef MULTICAST
	if ( win->mc_type == MC_MO_TYPE_RCV_ALL) {
		XtAddCallback (win->scrolled_win, WbNframeCallback,
			mc_frame_callback, win);
	} else {
#endif
		XtAddCallback (win->scrolled_win, WbNframeCallback,
                        frame_callback, win);
#ifdef MULTICAST
	}
#endif
	XtVaGetValues(win->scrolled_win, WbNview, (long)(&win->view), NULL);
	XtAddEventHandler(win->view, KeyPressMask, False, 
			mo_view_keypress_handler, win);
	XtVaSetValues(form,
		XmNinitialFocus, win->scrolled_win,
		NULL);
	/* now that the htmlWidget is created we can do this  */
/*############################################################*/
	mo_make_popup(win); /* c'est pour le cut&paste */

/*********************** SLAB_STATUS ****************************/
	win->slab[SLAB_STATUS] = XtVaCreateWidget("slab_status",
					xmFormWidgetClass, form, NULL);
	/* meter */
	win->meter_text = NULL;
	win->meter_notext = 0;
	win->meter_font = 0;
	win->meter_frame=XmxMakeFrame(win->slab[SLAB_STATUS],XmxShadowIn);
	XtVaSetValues(win->meter_frame,
			XmNrightOffset, 3,
			XmNtopOffset, 2,
			XmNbottomOffset, 2,
			XmNleftAttachment, XmATTACH_NONE,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			NULL);
	win->meter = XtVaCreateManagedWidget(
			"meter", xmDrawingAreaWidgetClass,
			win->meter_frame,
			XmNuserData, (XtPointer)AlloBalloonInfoData(win,  "Progress Meter"),
			XmNheight, 16,
			XmNwidth, 96,
			NULL);
	XtOverrideTranslations(win->meter,
			XtParseTranslationTable(mMosaicBalloonTranslationTable));
	win->meter_level = 0;
	win->meter_width = -1;

	XtAddCallback(win->meter, XmNexposeCallback, DrawMeter, (XtPointer)win);
	XtAddCallback(win->meter, XmNresizeCallback,ResizeMeter,(XtPointer)win);


	win->meter_fg = mMosaicAppData.meterForeground;
	win->meter_bg = mMosaicAppData.meterBackground;
	win->meter_font_fg = mMosaicAppData.meterFontForeground;
       	win->meter_font_bg = mMosaicAppData.meterFontBackground;

	win->tracker_widget = XtVaCreateManagedWidget(" ",xmLabelWidgetClass,
			win->slab[SLAB_STATUS],
			XmNalignment, XmALIGNMENT_BEGINNING,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_WIDGET ,
			XmNrightWidget, win->meter_frame,
			XmNtopAttachment, XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);

/*********************** SLAB_TOOLS ****************************/
	win->slab[SLAB_TOOLS] = XtVaCreateWidget("slab_tools",
				xmFormWidgetClass, form,
				NULL);

	mo_fill_toolbar(win,win->slab[SLAB_TOOLS],640,32);
/*############################################################*/

	/* chain those slabs together 'n stuff */
	XtVaSetValues(win->slab[SLAB_MENU],
			XmNleftOffset, 0,
			XmNrightOffset, 0,
			XmNtopOffset, 0,
			XmNbottomOffset, 0,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopWidget, NULL,
			XmNbottomAttachment, XmATTACH_NONE,
			XmNbottomWidget, NULL,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNrightWidget, win->slab[SLAB_GLOBE],
			NULL);
	XtVaSetValues(win->slab[SLAB_GLOBE],
			XmNleftAttachment, XmATTACH_NONE,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET ,
			XmNtopWidget, win->slab[SLAB_MENU],
			XmNbottomAttachment, XmATTACH_NONE,
			NULL);
	XtVaSetValues(win->slab[SLAB_TOOLS],
                       	XmNleftOffset, 0,
                       	XmNrightOffset, 0,
                       	XmNtopOffset, 0,
                       	XmNbottomOffset, 0,
                       	XmNtopAttachment, XmATTACH_WIDGET,
                       	XmNtopWidget, win->slab[SLAB_MENU],
                       	XmNbottomAttachment, XmATTACH_NONE,
                       	XmNbottomWidget, NULL,
                       	XmNleftAttachment, XmATTACH_FORM,
                       	XmNrightAttachment, XmATTACH_WIDGET,
                       	XmNrightWidget, win->slab[SLAB_GLOBE],
                       	NULL);
	XtVaSetValues(win->slab[SLAB_URL],
                       	XmNleftOffset, 0,
                       	XmNrightOffset, 0,
                       	XmNtopOffset, 0,
                       	XmNbottomOffset, 0,
                       	XmNtopAttachment, XmATTACH_WIDGET,
                       	XmNtopWidget, win->slab[SLAB_TOOLS],
                       	XmNbottomAttachment, XmATTACH_NONE,
                       	XmNbottomWidget, NULL,
                       	XmNleftAttachment, XmATTACH_FORM,
                       	XmNrightAttachment, XmATTACH_WIDGET,
                       	XmNrightWidget, win->slab[SLAB_GLOBE],
                       	NULL);
	XtVaSetValues(win->slab[SLAB_VIEW],
                       	XmNleftOffset, 0,
                       	XmNrightOffset, 0,
                       	XmNtopOffset, 0,
                       	XmNbottomOffset, 0,
                       	XmNtopAttachment, XmATTACH_WIDGET,
                       	XmNtopWidget, win->slab[SLAB_URL],
                       	XmNbottomAttachment, XmATTACH_WIDGET,
                       	XmNbottomWidget, win->slab[SLAB_STATUS],
                       	XmNleftAttachment, XmATTACH_FORM,
                       	XmNrightAttachment, XmATTACH_FORM,
                       	XmNrightWidget, win->slab[SLAB_GLOBE],
                       	NULL);
	XtVaSetValues(win->slab[SLAB_STATUS],
                       	XmNleftOffset, 0,
                       	XmNrightOffset, 0,
                       	XmNtopOffset, 0,
                       	XmNbottomOffset, 0,
                       	XmNtopAttachment, XmATTACH_NONE,
                       	XmNtopWidget, NULL,
                       	XmNbottomAttachment, XmATTACH_FORM,
                       	XmNbottomWidget, NULL,
                       	XmNleftAttachment, XmATTACH_FORM,
                       	XmNrightAttachment, XmATTACH_FORM,
                       	XmNrightWidget, win->slab[SLAB_GLOBE],
                       	NULL);

	for(i=0;i<SLABCOUNT;i++)
		XtManageChild(win->slab[win->slabpart[i]]);
	XtManageChild(form);

/* Can't go back or forward if we haven't gone anywhere yet... */
	mo_back_impossible (win);
	mo_forward_impossible (win);
	return mo_succeed;
}

/* name:    mo_delete_window
 * purpose: Shut down a window.
 * inputs:  
 *   - mo_window *win: The window.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   This can be called, among other places, from the WM_DELETE_WINDOW
 *   handler.  By the time we get here, we must assume the window is already
 *   in the middle of being shut down.
 *   We must explicitly close every dialog that be open as a child of
 *   the window, because window managers too stupid to do that themselves
 *   will otherwise allow them to stay up.
 */
#define POPDOWN(x) 	if (win->x) XtUnmanageChild (win->x)

mo_status mo_delete_window (mo_window *win)
{
	mo_node *node;

	if (!win)
		return mo_fail;
	if (!win->base)
		return mo_fail;

	node = win->first_node;

	POPDOWN (source_win);
	POPDOWN (save_win);
/* ########### */
	POPDOWN (open_win);
	POPDOWN (mail_win);
	POPDOWN (mailhist_win);
	POPDOWN (print_win);
	POPDOWN (history_win);
	POPDOWN (open_local_win);
	if (win->hotlist_win)
		XtDestroyWidget(win->hotlist_win);
	POPDOWN (techsupport_win);
	POPDOWN (search_win);
	POPDOWN (mailto_win);
	POPDOWN (mailto_form_win);
	POPDOWN (news_win);
	POPDOWN (links_win);
	XtPopdown (win->base);

/* we really should be doing this :-) BJS */
	XtDestroyWidget(win->base);
	win->base=NULL;

#ifdef MULTICAST
/*	if(win->mc_user){ */
/*		win->mc_user->win = NULL; ### */
/*		McRemoveMoWin(win->mc_user); */
/*####      McDeleteUser(win->mc_user,win->mc_user->mc_list_number);*/
/*	} */
#endif

/*################ */
/* Free up some of the HTML Widget's state */
/*  if (win && win->scrolled_win) {     
        HTMLFreeImageInfo (win->scrolled_win);
}
################ */

	while (node) {
		mo_node *tofree = node;
		node = node->next;
/*		mo_free_node_data (tofree); ####### FIXME*/
		free (tofree);
	}
	win->first_node=NULL;
	free (win->search_start);
	win->search_start=NULL;
	free (win->search_end);
	win->search_end=NULL;

/* free the RBM stuff */
	mo_delete_rbm_history_win(win);
	free(win->session_items);
/* free menubar ###### */
	_XmxRDestroyMenubar(win->menubar);
	free(win->menubar);
	free(win->agspd_cbd);

/* This will free the win structure (but none of its elements
 * individually) and exit if this is the last window in the list. */

	mo_remove_window_from_list (win);
	return mo_succeed;
}

int mo_get_font_size_from_res(char *userfontstr,int *fontfamily)
{
	char *lowerfontstr = strdup(userfontstr);
	int   x,len;

	len = strlen(userfontstr);
	for (x=0; x<len; x++)
		lowerfontstr[x]=tolower(userfontstr[x]);
  
	*fontfamily = 0;
	if (strstr(lowerfontstr, "times")!=NULL) {
		if (strstr(lowerfontstr, "large")!=NULL){
			free(lowerfontstr);
			return mo_large_fonts_tkn;
		}
		if (strstr(lowerfontstr, "regular")!=NULL){
			free(lowerfontstr);
			return mo_regular_fonts_tkn;
		}
		if (strstr(lowerfontstr, "small")!=NULL){
			free(lowerfontstr);
			return mo_small_fonts_tkn;
		}
		free(lowerfontstr);
		return mo_regular_fonts_tkn;
	}
	if (strstr(lowerfontstr, "helvetica")!=NULL) {
		*fontfamily = 1;
		if (strstr(lowerfontstr, "large")!=NULL) {
			free(lowerfontstr);
			return mo_large_helvetica_tkn;
		}
		if (strstr(lowerfontstr, "regular")!=NULL) {
			free(lowerfontstr);
			return mo_regular_helvetica_tkn;
		}
		if (strstr(lowerfontstr, "small")!=NULL) {
			free(lowerfontstr);
			return mo_small_helvetica_tkn;
		}
		free(lowerfontstr);
		return mo_regular_helvetica_tkn;
	}
	if (strstr(lowerfontstr, "century")!=NULL) {
		*fontfamily = 2;
		if (strstr(lowerfontstr, "large")!=NULL) {
			free(lowerfontstr);
			return mo_large_newcentury_tkn;
		}
		if (strstr(lowerfontstr, "regular")!=NULL) {
			free(lowerfontstr);
			return mo_regular_newcentury_tkn;
		}
		if (strstr(lowerfontstr, "small")!=NULL) {
			free(lowerfontstr);
			return mo_small_newcentury_tkn;
		}
		free(lowerfontstr);
		return mo_regular_newcentury_tkn;
	}
	if (strstr(lowerfontstr, "lucida")!=NULL) {
		*fontfamily = 3;
		if (strstr(lowerfontstr, "large")!=NULL) {
			free(lowerfontstr);
			return mo_large_lucidabright_tkn;
		}
		if (strstr(lowerfontstr, "regular")!=NULL) {
			free(lowerfontstr);
			return mo_regular_lucidabright_tkn;
		}
		if (strstr(lowerfontstr, "small")!=NULL) {
			free(lowerfontstr);
			return mo_small_lucidabright_tkn;
		}
		free(lowerfontstr);
		return mo_regular_lucidabright_tkn;
	}
	free(lowerfontstr);
	return mo_regular_fonts_tkn;
}

/* name:    delete_cb (PRIVATE)
 * purpose: Callback for the WM_DELETE_WINDOW protocol.
 * inputs:  
 *   - as per XmxCallback
 * returns: 
 *   nothing
 * remarks: 
 *   By the time we get called here, the window has already been popped
 *   down.  Just call mo_delete_window to clean up.
 */
static XmxCallback (delete_cb)
{
	mo_window *win = (mo_window *)client_data;
	mo_delete_window (win);
	return;
}

/* Make a framewindow like a mo_window, execpt a frame window is IN a mo_window */
/* we create a 'like' mo_window that can be used in 'Paf' routine */
/* this mo_window have no button , no menu, and a mini popup (back & forward) */
/* no decoration is used . Finaly this is a 'lite' mo_window */
/* inputs:
	- parent : the parent mo_window for this frame (framset)
	- htmlw : a htmlWdgetClass Widget , still created, where to display
		 the html part of this frame.
	- url : the href to get (load)
	- frame_name: the frame name (attribute of frame tag)
*/

mo_window * MMMakeSubWindow(mo_window *parent, Widget htmlw,
	char *url, char *frame_name)
{
	mo_window *swin;

	Xmx_n = 0;

	swin = (mo_window *)calloc (1, sizeof (mo_window));
	swin->base = parent->base;
	swin->mode = moMODE_PLAIN;
#ifdef MULTICAST
	swin->mc_type = parent->mc_type;	/* ????? ############# */
	swin->dot = NULL;
	swin->n_do = 0;
	swin->moid_ref = -1;
#endif
	swin->frame_name = strdup(frame_name);
	swin->frame_parent =parent;
/*
	swin->frame_dot_index = parent->frame_sons_nbre;
	parent->frame_sons = (mo_window **) realloc(parent->frame_sons,
		(parent->frame_sons_nbre + 1) * sizeof(mo_window *));
	parent->frame_sons[parent->frame_sons_nbre] = swin;
	parent->frame_sons_nbre++ ;
*/

 	swin->navigation_action = NAVIGATE_NEW;
	swin->source_win = 0;
	swin->save_win = 0;

/*  ################################################ 
 *	win->open_win = win->open_text = win->open_local_win = 0;
 *	win->mail_win = win->mailhot_win = 0;
 *	win->edithot_win = win->mailhist_win = win->inserthot_win = 0;
 *	win->print_win = 0;
 *	win->history_win = win->history_list = 0;
 *	win->hotlist_win = win->hotlist_list = 0;
 *	win->techsupport_win = win->techsupport_text = 0;
 *	win->mailto_win = win->mailto_text = 0;
 *	win->mailto_form_win = win->mailto_form_text = 0;
 *	win->post_data=0;
 *	win->news_win = 0;
 *	win->links_win = 0;
 *	win->news_fsb_win = 0;
 *	win->mail_fsb_win = 0;
 *	win->search_win = win->search_win_text = 0;
 *	win->src_search_win=0;
 *	win->src_search_win_text=0;
 *	win->first_node = NULL;
*/
	swin->current_node = parent->current_node;
/*	win->source_text = 0;
 *	win->format_optmenu = 0;
*/
	swin->save_format = parent->save_format;
	swin->agent_state=parent->agent_state;
/*
 *	win->underlines_snarfed = 0;
 *	win->mail_format = 0;
 *	win->print_text = 0;
*/
	swin->print_format = parent->print_format;
	swin->search_start = (void *)malloc (sizeof (ElementRef));
	swin->search_end = (void *)malloc (sizeof (ElementRef));
	swin->src_search_pos=0;
	swin->delay_object_loads = parent->delay_object_loads;
	swin->font_size = parent->font_size;
	swin->underlines_state = parent->underlines_state;
	swin->font_family = parent->font_family;

/*********************** SLAB_GLOBE ****************************/
	swin->logo = parent->logo;
	swin->biglogo = parent->biglogo;

	swin->scrolled_win= htmlw;
/*######	WbNpreviouslyVisitedTestFunction, anchor_visited_predicate, */
/*########### a mettre sous forme de callback ################ */
	XtAddCallback(swin->scrolled_win,
		WbNpointerMotionCallback, pointer_motion_callback,swin);
	XtAddCallback (swin->scrolled_win, WbNanchorCallback, anchor_cb, swin);
	XtAddCallback (swin->scrolled_win, WbNsubmitFormCallback,
					submit_form_callback, swin);
#ifdef MULTICAST
	if ( swin->mc_type == MC_MO_TYPE_RCV_ALL) {
		XtAddCallback (swin->scrolled_win, WbNframeCallback,
			mc_frame_callback, swin);
	} else {
#endif
		XtAddCallback (swin->scrolled_win, WbNframeCallback,
                        frame_callback, swin);
#ifdef MULTICAST
	}
#endif
	XtVaGetValues(swin->scrolled_win, WbNview, (long)(&swin->view), NULL);
	XtAddEventHandler(swin->view, KeyPressMask, False, 
			mo_view_keypress_handler, swin);
	/* now that the htmlWidget is created we can do this  */
/*############################################################*/
	mo_make_popup(swin); /* c'est pour le cut&paste */

/*********************** SLAB_STATUS ****************************/
	/* meter */
	swin->meter_text = parent->meter_text;
	swin->meter_notext = parent->meter_notext;
	swin->meter_font = parent->meter_font;
	swin->meter_frame=parent->meter_frame;
	swin->meter = parent->meter;
	swin->meter_level = parent->meter_level;
	swin->meter_width = parent->meter_width;
	swin->tracker_widget = parent->tracker_widget;


/* take care of session history for rbm */
/*     	win->session_menu = NULL;       
 *     	win->num_session_items = 0;     
 *   	win->session_items = (Widget*) malloc(sizeof(Widget) *
 *                   mMosaicAppData.numberOfItemsInRBMHistory);
 *	XtPopup (win->base, XtGrabNone);
*/
	XFlush (mMosaicDisplay);
	XSync (mMosaicDisplay, False);

/* init paf data */
	swin->pafd = NULL;
	return swin;
}

/* purpose: Make a new window from scratch.
 * inputs:  
 *   - mo_window *parentw: Parent window, if one exists (may be NULL).
 *   - mc_t	is multicast enable for this window.
 * returns: 
 *   The new window (mo_window *).
 * remarks: 
 *   The 'parent window' parentw is the window being cloned, or the
 *   window in which the 'new window' command was triggered, etc.
 *   Some GUI properties are inherited from it, if it exists (fonts,
 *   anchor appearance, etc.).
 */
mo_window *mo_make_window ( mo_window *parent, McMoWType mc_t)
{
	Widget base;
	mo_window *win;
	Atom WM_DELETE_WINDOW;
	char buf[80];
	int i;

	sprintf(buf,"mMosaic %s: ",MO_VERSION_STRING);
	Xmx_n = 0;
	XmxSetArg (XmNtitle, (long)buf);
	XmxSetArg (XmNiconName, (long)"mMosaic");
	XmxSetArg (XmNallowShellResize, False);
	XmxSetArg (XmNwidth, mMosaicAppData.default_width);
	XmxSetArg (XmNheight, mMosaicAppData.default_height);
	if (mMosaicAppData.initial_window_iconic)
		XmxSetArg (XmNiconic, True);
	if (mMosaicAppData.install_colormap) 
		XmxSetArg(XmNcolormap,mMosaicColormap);
	base = XtCreatePopupShell ("shell", topLevelShellWidgetClass,
			mMosaicToplevelWidget, Xmx_wargs, Xmx_n);
	Xmx_n = 0;
	XtAddEventHandler(base, (EventMask) 0, TRUE,
			(XtEventHandler) _XEditResCheckMessages, NULL);

	win = (mo_window *)calloc (1, sizeof (mo_window));
	win->base = base;
	win->mode = moMODE_PLAIN;

	win->frame_name = NULL;
	win->frame_parent =NULL;
	win->frame_sons = NULL;
	win->frame_sons_nbre = 0;
	win->frame_dot_index = -1;

#ifdef MULTICAST
	win->mc_type = mc_t;
	win->dot = NULL;
	win->n_do = 0;
	win->moid_ref = -1;
#endif
	WM_DELETE_WINDOW = XmInternAtom(mMosaicDisplay, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(base,WM_DELETE_WINDOW,delete_cb,(XtPointer)win);

 	win->navigation_action = NAVIGATE_NEW;
	win->source_win = 0;
	win->save_win = 0;
/*  ########### */
	for(i=0;i<BTN_COUNT;i++)
		win->tools[i].gray = XmxSensitive;
	win->open_win = win->open_text = win->open_local_win = 0;
	win->mail_win = win->mailhot_win = 0;
	win->edithot_win = win->mailhist_win = win->inserthot_win = 0;
	win->print_win = 0;
	win->history_win = win->history_list = 0;
	win->hotlist_win = win->hotlist_list = 0;
	win->techsupport_win = win->techsupport_text = 0;
	win->mailto_win = win->mailto_text = 0;
	win->mailto_form_win = win->mailto_form_text = 0;
	win->post_data=0;
	win->news_win = 0;
	win->links_win = 0;
	win->news_fsb_win = 0;
	win->mail_fsb_win = 0;
	win->search_win = win->search_win_text = 0;
	win->src_search_win=0;
	win->src_search_win_text=0;
	win->first_node = NULL;
	win->current_node = 0;
	win->source_text = 0;
	win->format_optmenu = 0;
	win->save_format = mo_plaintext;
	win->agent_state=selectedAgent;
	win->underlines_snarfed = 0;
	win->mail_format = 0;
	win->print_text = 0;
	win->print_format = mo_plaintext;
	win->search_start = (void *)malloc (sizeof (ElementRef));
	win->search_end = (void *)malloc (sizeof (ElementRef));
	win->src_search_pos=0;
	win->delay_object_loads = mMosaicAppData.delay_object_loads;
	win->font_size = mo_get_font_size_from_res(
		mMosaicAppData.default_font_choice, &(win->font_family));
	win->underlines_state = mo_default_underlines_tkn;
	if (parent) {
		win->font_size = parent->font_size;
		win->font_family = parent->font_family;
		win->underlines_state = parent->underlines_state;
		win->agent_state = parent->agent_state;
	}
	mo_fill_window (win);		/* Install all the GUI bits & pieces. */
	if (win->font_size != mo_regular_fonts_tkn)	/* Set the font size. */
		mo_set_fonts (win, win->font_size);
	mo_set_underlines (win, win->underlines_state);
	mo_set_agents(win, win->agent_state);
#ifdef ISINDEX
	win->keyword_search_possible = -1;	/* We don't know yet. */
#endif
	XmxRSetToggleState (win->menubar, (XtPointer)win->font_size, XmxSet);
						/* setup news default states */
#ifdef NEWS
	gui_news_updateprefs (win);
#endif

	XmxRSetToggleState (win->menubar, (XtPointer)mo_delay_object_loads,
		 win->delay_object_loads ? XmxSet : XmxNotSet);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_expand_object_current,
		win->delay_object_loads ? XmxSensitive : XmxNotSensitive);

/* take care of session history for rbm */
      	win->session_menu = NULL;       
      	win->num_session_items = 0;     
    	win->session_items = (Widget*) malloc(sizeof(Widget) *
                   mMosaicAppData.numberOfItemsInRBMHistory);
	XtPopup (win->base, XtGrabNone);
	XFlush (mMosaicDisplay);
	XSync (mMosaicDisplay, False);

	mo_add_window_to_list (win); /*Register win with internal window list.*/
  	if(parent) {                        
		win->body_color = parent->body_color;
		XtVaSetValues(win->scrolled_win,
			WbNbodyColors, win->body_color,
			NULL);
		XmxRSetToggleState (win->menubar, (char*)mo_body_color,
			win->body_color ? XmxSet : XmxNotSet);

                win->body_image = parent->body_image;
                XtVaSetValues(win->scrolled_win,
                        WbNbodyImages, win->body_image,
                        NULL);        
                XmxRSetToggleState (win->menubar, (char*)mo_body_image,
                        win->body_image ? XmxSet : XmxNotSet);

		win->delay_object_loads = parent->delay_object_loads;
		XmxRSetSensitive (win->menubar, (char*)mo_expand_object_current,
			win->delay_object_loads ? XmxSensitive : XmxNotSensitive);
		XmxRSetToggleState (win->menubar, (char*)mo_delay_object_loads,
			win->delay_object_loads ? XmxSet : XmxNotSet);
	}

/* register icon */

	XtVaSetValues(win->base,
		XmNiconMask, mMosaicWinIconMaskPixmap,
		XmNiconPixmap, mMosaicWinIconPixmap,
		NULL);

/* set/notset boby color and body image */

	XtVaGetValues(win->scrolled_win,
		WbNbodyColors, &(win->body_color),
		NULL);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_body_color,
			(win->body_color ? XmxSet : XmxNotSet));

        XtVaGetValues(win->scrolled_win,
                WbNbodyImages, &(win->body_image),
                NULL);                
        XmxRSetToggleState (win->menubar, (XtPointer)mo_body_image,
                        (win->body_image ? XmxSet : XmxNotSet));
/* init paf data */
	win->pafd = NULL;
	return win;
}

/* name:    mo_open_another_window
 * purpose: Open another window to view a given URL, unless the URL
 *          indicates that it's pointless to do so
 *	    Open in UNICAST mode
 * inputs:  
 *   - mo_window      *pwin: The existing window.
 *   - char           *url: The URL to view in the new window.
 */
void mo_open_another_window (mo_window *win, char *url)
{
	mo_window *neww;
	RequestDataStruct rds;

	neww = mo_make_window(win,MC_MO_TYPE_UNICAST);
	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = url;
	rds.gui_action = HTML_LOAD_CALLBACK;
	MMPafLoadHTMLDocInWin (neww, &rds);
}

/****************************************************************************
 * name:    mo_process_external_directive
 * purpose: Handle an external directive given to the application via
 *          a config file read in response to a SIGUSR1.
 * inputs:  
 *   - char *directive: The directive; either "goto" or "newwin".
 *   - char       *url: The URL corresponding to the directive.
 * returns: 
 *   nothing
 * remarks: 
 *   
 ****************************************************************************/
#define CLIP_TRAILING_NEWLINE(url) \
  if (url[strlen (url) - 1] == '\n') \
    url[strlen (url) - 1] = '\0';

static XEvent *mo_manufacture_dummy_event (Widget foo)
{
	/* This is fucking hilarious. */
	XAnyEvent *a = (XAnyEvent *)malloc (sizeof (XAnyEvent));
	a->type = 1; /* HAHA! */
	a->serial = 1; /* HAHA AGAIN! */
	a->send_event = False;
	a->display = XtDisplay (foo);
	a->window = XtWindow (foo);
	return (XEvent *)a;
}

void mo_process_external_directive (char *directive, char *url)
{
	/* Process a directive that we received externally. */
	mo_window *win = NULL;
	Widget sb;
	String params[1];
	RequestDataStruct rds;

	/* Make sure we have a window. */
	win = mo_main_next_window (NULL);

	if (!strncmp (directive, "goto", 4)) {
		CLIP_TRAILING_NEWLINE(url);
		rds.req_url = url;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		MMPafLoadHTMLDocInWin (win, &rds);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "newwin", 6)) {
		CLIP_TRAILING_NEWLINE(url);
		/* Force a new window to open. */ 
		mo_open_another_window (win, url);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "pagedown", 8)) {
		params[0] = "0";
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
				(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XEvent *event = mo_manufacture_dummy_event (sb);
			XtCallActionProc(sb,"PageDownOrRight",event,params,1);
		}
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "pageup", 6)) {
		params[0] = "0";
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
			(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XEvent *event = mo_manufacture_dummy_event (sb);
			XtCallActionProc(sb, "PageUpOrLeft", event, params, 1);
		}
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "scrolldown", 9)) {
		params[0] = "0";
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
			(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XEvent *event = mo_manufacture_dummy_event (sb);
		      XtCallActionProc(sb,"IncrementDownOrRight",event,params,1);
		}
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "scrollup", 7)) {
		params[0] = "0";
		XtVaGetValues (win->scrolled_win, XmNverticalScrollBar, 
			(long)(&sb), NULL);
		if (sb && XtIsManaged (sb)) {
			XEvent *event = mo_manufacture_dummy_event (sb);
			XtCallActionProc(sb,"IncrementUpOrLeft",event,params,1);
		}
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "flushcache", 10)) {
		MMCacheClearCache ();
		return;
	}
	if (!strncmp (directive, "backnode", 8)) {
		mo_back (win->scrolled_win, (XtPointer) win, NULL);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "forwardnode", 11)) {
		mo_forward (win->scrolled_win, (XtPointer) win, NULL);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "reloaddocument", 14)) {
		mo_reload_document(win->scrolled_win, (XtPointer) win, NULL);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "reloadimages", 12)) {
		mo_reload_document(win->scrolled_win, (XtPointer) win, NULL);
		XFlush(mMosaicDisplay);
		return;
	}
	if (!strncmp (directive, "refresh", 7)) {
		mo_refresh_window_text (win);
		XFlush(mMosaicDisplay);
		return;
	}
	return;
}

void mo_flush_passwd_cache (mo_window *win)
{
	fprintf(stderr,"mo_flush_passwd_cache: Bug \n");
	fprintf(stderr,"Please report\n");
	abort();
/*	HTFTPClearCache (); */
/*	HTAAServer_clear (); */
	mo_gui_notify_progress ("Password cache flushed",win);
}
