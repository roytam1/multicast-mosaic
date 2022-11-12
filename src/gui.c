/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

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
#define __SRC__
#include "../libwww2/HText.h"
#include "../libwww2/HTAAUtil.h"
#include "../libwww2/HTAABrow.h"
#include "../libwww2/HTFTP.h"
#include "../libwww2/HTAlert.h"
#include "../libwww2/HTFile.h"
#include "xresources.h"
#include "img.h"
#include "gui.h"
#include "gui-documents.h"
#include "main.h"
#include "mo-www.h"
#include "gui-menubar.h"
#include "hotlist.h"
#include "history.h"
#include "globalhist.h"
#include "pixmaps.h"
#include "libnut/system.h"
#include "gui-popup.h"
#include "cache.h"

#include "bitmaps/security.xbm"
#include "bitmaps/iconify.xbm"
#include "bitmaps/iconify_mask.xbm"

#include "pixmaps.h"

#ifdef MULTICAST
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../libmc/mc_rtp.h"
#include "../libmc/mc_defs.h"
#include "../libmc/mc_misc.h"
#include "../libmc/mc_sockio.h"
#include "../libmc/mc_dispatch.h"
#include "../libmc/mc_action.h"
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

/*##############*/
/*Pixmap imap,imaskmap;   */
/*##############*/

char 		pre_title[80];	 /*SWP -- 9/7/95*/
int 		makeBusy=0;

/* SWP -- Spoof Agent stuff */
extern int 	numAgents;
extern int 	selectedAgent;
extern char 	**agent;

/* PLB */
extern int 	newsShowAllGroups;
extern int 	newsShowAllArticles;
extern int 	newsShowReadGroups;
extern int 	newsNoThreadJumping;

extern int 	securityType;
extern int 	noLength;	 /*SWP -- 10.27.95 -- No Content Length*/

extern int 	cci_event; /* from cciBindings.c */

/*
 * Globals used by the pixmaps for the animated icon.
 * Marc, I imagine you might want to do something cleaner
 * with these?
 */
extern int 	IconWidth, IconHeight, WindowWidth, WindowHeight;
extern Pixmap 	*IconPix,*IconPixSmall,*IconPixBig;

Widget 		mo_fill_toolbar(mo_window *win, Widget top, int w, int h);
char *		MakeFilename();

/* ------------------------------ variables ------------------------------- */

int sarg[7],smalllogo=0,stexttools=0;

/* ------------------------------------------------------ */

extern int cci_get;		/* from cciBindings.c */

char *HTReferer = NULL;

/* This is exported to libwww, like altogether too many other variables here. */

int binary_transfer;

/* Now we cache the current window right before doing a binary
   transfer, too.  Sheesh, this is not pretty. */

XColor fg_color, bg_color;

static Cursor busy_cursor;
static int busy = 0;
char *cached_url = NULL;

/* Forward declaration of test predicate. */
int anchor_visited_predicate (Widget, char *);

/* When we first start the application, we call mo_startup()
   after creating the unmapped toplevel widget.  mo_startup()
   either sets the value of this to 1 or 0.  If 0, we don't
   open a starting window. */

int defer_initial_window;

/* Pixmaps for interrupt button. */

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
typedef struct _InfoData {
	mo_window * win;
	char * msg;
} InfoData;

static InfoData * AlloInfoData ( mo_window * win, char * s)
{
	InfoData * info;

	info =  (InfoData*) malloc(sizeof(InfoData));
	info->win = win;
	info->msg = s;
	return info;
}
static void BalloonHelpMe(Widget w, XEvent * event)
{
        InfoData *info;

        XtVaGetValues(w, XmNuserData, (XtPointer) &info, NULL);
        mo_gui_notify_progress(info->msg,info->win);
}

static void UnBalloonHelpMe(Widget w, XEvent * event)
{                                      
        InfoData *info;

        XtVaGetValues(w, XmNuserData, (XtPointer) &info, NULL);
        mo_gui_notify_progress(" ",info->win);   
}                                      

static char xlattab[] = "<Enter>: BalloonHelpMe()\n\
                        <Leave>: UnBalloonHelpMe()";

static XtActionsRec balloon_action[] = {
    {"BalloonHelpMe", (XtActionProc)BalloonHelpMe},
    {"UnBalloonHelpMe", (XtActionProc)UnBalloonHelpMe}
};                                     
/* to use balloon help, add these bits to your widget ...  BJS 2/7/96
 *    XmNtranslations, XtParseTranslationTable(xlattab),
 *    XmNuserData, (xtpointer) "Balloon Help String!",
 */                                    
/* ------------------------------------------------------ */


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
		while (w = mo_main_next_window (w)) {
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

/* ----------------------------- busy cursor ------------------------------ */

static int animateCursor(mo_window * win)
{

	if (!makeBusy) {	/* stop busy animation */
		if (busy) {
			XUndefineCursor (mMosaicDisplay, XtWindow (mMosaicToplevelWidget));
			XUndefineCursor (mMosaicDisplay, XtWindow (win->base));
			if (win->history_win)
				XUndefineCursor(mMosaicDisplay,
					XtWindow(win->history_win));
			if (win->hotlist_win)
				XUndefineCursor(mMosaicDisplay,
					XtWindow(win->hotlist_win));
			XFlush (mMosaicDisplay);
			busy = 0;
		}
		return 0;
	}
	XDefineCursor(mMosaicDisplay, XtWindow(mMosaicToplevelWidget), busy_cursor);
	XDefineCursor(mMosaicDisplay,XtWindow(win->base),busy_cursor);
	if (win->history_win)
		XDefineCursor(mMosaicDisplay, XtWindow(win->history_win),
			      busy_cursor);
	if (win->hotlist_win)
		XDefineCursor(mMosaicDisplay, XtWindow(win->hotlist_win),
			      busy_cursor);
	XFlush(mMosaicDisplay);
	busy=1;
	return(1);
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
mo_status mo_redisplay_window (mo_window *win)
{
	char *curl = cached_url;

	cached_url = win->cached_url;
	HTMLRetestAnchors (win->scrolled_win, anchor_visited_predicate);
	cached_url = curl;
	return mo_succeed;
}

static connect_interrupt = 0;
extern int sleep_interrupt;

XmxCallback (icon_pressed_cb)
{
	mo_window * win = (mo_window*) client_data;

	sleep_interrupt = connect_interrupt = 1;
	if (cci_event)
		MoCCISendEventOutput(MOSAIC_GLOBE);
}

static XmxCallback (security_pressed_cb)
{
	mo_window *win = (mo_window *)client_data;
	char buf[BUFSIZ];

	if (!win || !win->current_node || !win->current_node)
		return;
	if (cci_event)
		MoCCISendEventOutput(AUTHENTICATION_BUTTON);
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
}

/* ----------------------- editable URL field callback -------------------- */
/* If the user hits return in the URL text field at the top of the display, */
/* then go fetch that URL  -- amb                                           */

static XmxCallback (url_field_cb)
{
	mo_window *win=(mo_window*)client_data;
	char *url,*xurl;
	XmTextVerifyCallbackStruct *cbs =(XmTextVerifyCallbackStruct *) call_data;

	if (cci_event)
		MoCCISendEventOutput(MOSAIC_URL_TEXT_FIELD);
	url = XmTextGetString (win->url_text);
	if (!url || (!strlen(url)))
		return;
	mo_convert_newlines_to_spaces (url);
	xurl=mo_url_prepend_protocol(url);
	mo_load_window_text (win, xurl, NULL);

	if (xurl==url) {
		free(xurl);
	} else {
		free(xurl);
		free(url);
	}
	if (cci_event)
		MoCCISendEventOutput(LINK_LOADED);
	return;
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
 *   Call mo_open_another_window or mo_load_window_text to get
 *   the actual work done.
 */
static XmxCallback (anchor_cb)
{
	char *href, *reftext;
	static char *referer = NULL;
	mo_window *win = (mo_window*)client_data;
	XButtonReleasedEvent *event = 
	      (XButtonReleasedEvent *)((WbAnchorCallbackData *)call_data)->event;
	int force_newwin = (event->button == Button2 ? 1 : 0);
	int old_binx_flag;

	
	if (cci_event) MoCCISendEventOutput(MOSAIC_URL_TRIGGER);
		/* if shift was down, make this a Load to Local Disk -- amb */
	old_binx_flag = win->binary_transfer;
	if ( (event->state & ShiftMask) == ShiftMask)
		win->binary_transfer = 1;

#ifdef MULTICAST
	if (win->mc_type == MC_MO_TYPE_RCV_ALL){
		force_newwin =1;
	}
#endif
	if (mMosaicAppData.protect_me_from_myself) {
		int answer = XmxModalYesOrNo (win->base, mMosaicAppContext,
	 "BEWARE: NCSA disclaims all responsibility regarding your emotional and mental health.\n\nAre you *sure* you want to follow this hyperlink?" , "I'm sure.",
		"No! Get me outta here." );
		if (!answer)
			return;
	}
	if (referer!=NULL) {
		free(referer);
		referer=NULL;
	}
	/*SWP*/
	if (!strncasecmp(win->current_node->url, "http://", 7)) {
				/* what if hostname is a partial local? */
		referer = strdup (win->current_node->url);
		HTReferer = referer;
	} else
		HTReferer = NULL;
  
	if (((WbAnchorCallbackData *)call_data)->href)
		href = strdup (((WbAnchorCallbackData *)call_data)->href);
	else
		href = strdup ("Unlinked");
	if (((WbAnchorCallbackData *)call_data)->text)
		reftext = strdup (((WbAnchorCallbackData *)call_data)->text);
	else
		reftext = strdup ("Untitled");

	mo_convert_newlines_to_spaces (href);
	if (!force_newwin)
		mo_load_window_text (win, href, reftext);
	else {
		char *target = mo_url_extract_anchor (href);
		char *url = 
			mo_url_canonicalize_keep_anchor(href,
					win->current_node->url);
				/* @@ should we be keeping the anchor here?? */
		if(strncmp (url, "telnet:", 7) && 
		   strncmp (url, "tn3270:", 7) &&
		   strncmp (url, "rlogin:", 7)) { /* Not a telnet anchor. */
			   /* Open the window (generating a new cached_url). */
			mo_open_another_window (win, url, reftext, target);
					/* Now redisplay this window. */
			mo_redisplay_window (win);
		} else
			/*Just do mo_load_window_text go get xterm forked off. */
			mo_load_window_text (win, url, reftext);
	}
	if (cci_event) MoCCISendEventOutput(LINK_LOADED);
	win->binary_transfer = old_binx_flag;
	free (href);
	return;
}

/*
 * name:    anchor_visited_predicate (PRIVATE)
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
int anchor_visited_predicate (Widget w, char *href)
{
	int rv;

	if (!mMosaicAppData.track_visited_anchors || !href)
		return 0;

	/* This doesn't do special things for data elements inside
	 * an HDF file, because it's faster this way. */

	href = mo_url_canonicalize (href, cached_url);
	rv = (mo_been_here_before_huh_dad (href) == mo_succeed ? 1 : 0);
	free (href);
	return rv;
}

static void pointer_motion_callback (Widget w, XtPointer clid, XtPointer calld)
{
	char *href = (char*) calld;
	XmString xmstr;
	char *to_free = NULL, *to_free_2 = NULL;
	mo_window * win = (mo_window*) clid;

	if (!mMosaicAppData.track_pointer_motion)
		return;
	if (href && *href) {
		href = mo_url_canonicalize_keep_anchor (href, win->cached_url);
		to_free = href;
		mo_convert_newlines_to_spaces (href); /* Sigh... */

		/* This is now the option wherein the URLs are just spit up there;
		 * else we put up something more friendly. */
		if (mMosaicAppData.track_full_url_names) {
			/* Everything already done... */
		} else {
			/* This is where we go get a good description. */
			href = HTDescribeURL (href);
			to_free_2 = href;
		}
	} else 
		href = " ";

	xmstr = XmStringCreateSimple (href);
	XtVaSetValues (win->tracker_label, XmNlabelString, (XtArgVal)xmstr, NULL);
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

	if (!cbdata)
		return;

	if (cci_event)
		MoCCISendEventOutput(FORM_SUBMIT);
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
		url = win->current_node->url;

	if (cbdata->method && *(cbdata->method))
		method = cbdata->method;
	else
		method = strdup ("GET");
			/* Grab enctype if it's there. */
	if (cbdata->enctype && *(cbdata->enctype))
		enctype = cbdata->enctype;
	if (mMosaicSrcTrace) {
		fprintf (stderr, "[submit_form_callback] method is '%s' enctype is '%s'\n",
				method, enctype);
	}
	if ((strcasecmp (method, "POST") == 0) ||
	    (strcasecmp (method, "cciPOST") == 0))
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
			char *c = mo_escape_part (cbdata->attribute_values[0]);
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
				char *c = mo_escape_part(
					cbdata->attribute_names[i]);
				strcat (query, c);
				free (c);
				strcat (query, "=");
				if (cbdata->attribute_values[i]) {
					c = mo_escape_part(
						cbdata->attribute_values[i]);
					strcat (query, c);
					free (c);
				}
			}
		}
	}
	if (do_post_urlencoded) {
		if (!strcasecmp(method,"cciPOST"))
			MoCCIFormToClient(NULL, NULL, NULL,NULL,1);
		mo_post_access_document (win, url, 
				"application/x-www-form-urlencoded", query);
	} else {
		mo_load_window_text (win, query, NULL);
	}
	if (query)
		free (query);
}

/* This only gets called by the widget in the middle of set_text. */
static XmxCallback (link_callback)
{
	mo_window *win = (mo_window*)client_data;
	LinkInfo *linfo = (LinkInfo *)call_data;
	extern char *url_base_override;

	if (!linfo)
		return;
	if (linfo->href) {		/* Free -nothing- in linfo. */
		url_base_override = strdup (linfo->href);

		/* Set the URL cache variables to the correct values NOW. */
		cached_url = mo_url_canonicalize (url_base_override, "");
		win->cached_url = cached_url;
	}
	if (linfo->role) {
		/* Do nothing with the role crap yet. */
	}
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
	XtVaSetValues (win->tracker_label, XmNlabelString, (XtArgVal)xmstr, NULL);
	XmStringFree (xmstr);
	XmUpdateDisplay (win->base);
	return;
}

static void UpdateButtons (mo_window * win)
{
	XEvent event;
  
	XSync (mMosaicDisplay, 0);
	while (XCheckMaskEvent(mMosaicDisplay,(ButtonPressMask|ButtonReleaseMask),
                         &event)) {
		XButtonEvent *bevent = &(event.xbutton);
		if (bevent->window == XtWindow (win->logo)) 
			XtDispatchEvent(&event);

		/* else just throw it away... users shouldn't be pressing buttons
		in the middle of transfers anyway... */
	}
}

/*SWP 7/3/95*/
void mo_gui_check_security_icon_in_win(int type, mo_window *win)
{
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
}

static int logo_count = 0;

int mo_gui_check_icon (int twirl, mo_window * win)
{
	int ret;
	static int cnt=0;

	if(twirl>0) {
		if (!makeBusy) {
			makeBusy=1;
		}
		cnt++;
		if (cnt==2) {
			animateCursor(win);
			cnt=0;
		}
		if (mMosaicAppData.twirling_transfer_icon) {
			AnimatePixmapInWidget(win->logo,IconPix[logo_count]);
			logo_count++;
			if (logo_count >= NUMBER_OF_FRAMES)
				logo_count = 0;
		}
	}
	UpdateButtons (win);
	XmUpdateDisplay (win->base);
	ret = connect_interrupt;
	connect_interrupt = 0;
	return(ret);
}

void mo_gui_clear_icon (mo_window * win)
{
	connect_interrupt = 0;
}

void mo_gui_apply_default_icon( mo_window *win)
{
 
        XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);
}

extern void ungrab_the_____ing_pointer(XtPointer client_data);
void mo_gui_done_with_icon (mo_window * win)
{
	XClearArea(mMosaicDisplay, XtWindow(win->logo), 0, 0, 0, 0, True);
	makeBusy = 0;
	animateCursor(win);
	logo_count = 0;
	HTMLSetAppSensitive((Widget) win->scrolled_win);
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
		break;
	case XK_Next:  /* Page down. */
	case XK_KP_Next:
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
      case XK_B: /* Back. */
      case XK_b:
          mo_back_node (win);
          break;
      case XK_C: /* Clone */
      case XK_c:
          mo_duplicate_window (win);
          break;
      case XK_D: /* Document source. */
      case XK_d:
          mo_post_source_window (win);
          break;
      case XK_E: /* Edit */
      case XK_e:
          mo_edit_source (win);
          break;
      case XK_F:
      case XK_f:
          mo_forward_node (win);
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
          mo_open_another_window (win, mMosaicAppData.home_document, NULL, NULL);
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
          mo_reload_window_text (win);
          break;
      case XK_R: /* Refresh */
          mo_refresh_window_text (win);
          break;
      case XK_S: /* Search. */
      case XK_s:
          mo_post_search_window (win);
          break;
        /* Tag 'n Bag */              
              /*                      
      case XK_T:                      
      case XK_t:                      
          mo_tagnbag_url (win);       
          break;                      
          */     
/* Not active */                      
#ifdef SWP_NOT_DONE                   
      case XK_U: /* Upload a file (method of put) */
      case XK_u:                      
          mo_post_upload_window(win); 
          break;                      
#endif                                
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
			      WbNmeterFont, &(win->meter_font),
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
/* FTP Mode */
    {"Put","Send file to remote host",
	mo_ftp_put,&toolbarFTPput, NULL, moMODE_FTP, NULL},
    {"Mkdir","Make remote directory",
	mo_ftp_mkdir,&toolbarFTPmkdir, NULL, moMODE_FTP, NULL},
    {NULL, NULL, 0, NULL, NULL, 0, NULL}
};

/* NOTE: THESE MUST COINCIDE EXACTLY WITH mo_tools!!! */
char *tool_names[] = {
	"BACK", "FORWARD", "RELOAD", "HOME",
	"OPEN", "SAVE", "NEW", "CLONE", "CLOSE",
	"ADD_TO_HOTLIST", "FIND", "PRINT", "GROUPS",
	"INDEX", "PREVIOUS_THREAD", "PREVIOUS_ARTICLE", "NEXT_ARTICLE",
	"NEXT_THREAD", "POST", "FOLLOW_UP", "PUT", "MKDIR",
	NULL
};

void mo_make_globe(mo_window *win, Widget parent, int small);

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

void mo_extra_buttons(mo_window *win, Widget top)
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
                  XmNuserData, (XtPointer) AlloInfoData(win,"Security Stats Information"),
                  XmNtraversalOn, False,
                  NULL);
    XtOverrideTranslations(win->security,
                            XtParseTranslationTable(xlattab));
    win->encrypt = XtVaCreateManagedWidget(" ", xmPushButtonWidgetClass, top,
         XmNmarginWidth, 0,
         XmNmarginHeight, 0,
         XmNmarginTop, 0,
         XmNmarginBottom, 0,
         XmNmarginLeft, 0,
         XmNmarginRight, 0,
         XmNuserData, (XtPointer) AlloInfoData(win, "Encryption Status (not in this release)"),
         XmNtraversalOn, False,       
         NULL);
    XmxApplyPixmapToLabelWidget (win->encrypt, enc_not_secure);
    XtOverrideTranslations(win->encrypt,XtParseTranslationTable(xlattab));
            /* insure we set the security icon! */
    if (win->current_node) {
        mo_gui_check_security_icon_in_win(win->current_node->authType,win);
    }
}

void mo_make_globe(mo_window *win, Widget parent, int small)
{
	if(!small){
		IconPix = IconPixBig;
		IconWidth = IconHeight = 64;
		WindowWidth = WindowHeight = 0;
		logo_count = 0;
	} else {
		IconPix = IconPixSmall;
		IconWidth = IconHeight = 32;
		logo_count = 0;
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
		XmNuserData, (XtPointer)AlloInfoData(win,  "Logo Button - Abort a Transaction"),
		NULL);
	XtOverrideTranslations(win->logo, XtParseTranslationTable(xlattab));
}

/* create topform and fill it with toolbar bits'n'pieces */
Widget mo_fill_toolbar(mo_window *win, Widget top, int w, int h)
{
	int tmp = 25;
	Widget tearbutton;
	int i,vert = win->toolbarorientation && win->toolbardetached;
	int textbuttons = win->texttools;
	static XFontStruct *tmpFont=NULL;
	static XmFontList tmpFontList;

	if (!tmpFont) {
		XtVaGetValues(win->scrolled_win,
			WbNtoolbarFont, &tmpFont,
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
				XmNuserData, (XtPointer)AlloInfoData(win, "Toolbar Tearoff Control"),
				XmNlabelType, textbuttons ? XmSTRING : XmPIXMAP,
				XmNlabelPixmap, vert?tearh:tearv,
				XmNfontList, tmpFontList,
         			XmNtraversalOn, False,
				NULL);
	XtOverrideTranslations(tearbutton,XtParseTranslationTable(xlattab));
	XtAddCallback(tearbutton, XmNactivateCallback, mo_tool_detach_cb,
				(XtPointer) win);
	for(i=0;mo_tools[i].label;i++) {
		if(mo_tools[i].action){
			win->tools[i].w = XtVaCreateManagedWidget(
				mo_tools[i].text,
				xmPushButtonWidgetClass,
				win->button_rc,
				XmNuserData, (XtPointer)AlloInfoData(win, mo_tools[i].label),
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
				XtParseTranslationTable(xlattab));
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
	win->url_text = XtVaCreateManagedWidget("text",xmTextFieldWidgetClass,
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
	XtAddCallback (win->url_text, XmNactivateCallback, url_field_cb, (XtPointer)win);

/*********************** SLAB_VIEW ****************************/
	win->slab[SLAB_VIEW]= win->scrolled_win= XtVaCreateManagedWidget ("view",
		htmlWidgetClass, form, 
		WbNtext, 0,
       		XmNresizePolicy, XmRESIZE_ANY,
		WbNpreviouslyVisitedTestFunction, anchor_visited_predicate,
		XmNshadowThickness, 2,
		NULL);
/*########### a mettre sous forme de callback ################ */
	XtAddCallback(win->scrolled_win,WbNimageCallback,ImageResolve,win);
	XtAddCallback(win->scrolled_win,WbNgetUrlDataCB,GetUrlData,win);

	XtAddCallback(win->scrolled_win,
		WbNpointerMotionCallback, pointer_motion_callback,win);
	XtAddCallback (win->scrolled_win, WbNanchorCallback, anchor_cb, win);
	XtAddCallback (win->scrolled_win, WbNlinkCallback, link_callback, win);
	XtAddCallback (win->scrolled_win, WbNsubmitFormCallback,
					submit_form_callback, win);
	XtVaGetValues(win->scrolled_win, WbNview, (long)(&win->view), NULL);
	XtAddEventHandler(win->view, KeyPressMask, False, 
			mo_view_keypress_handler, win);
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
			XmNuserData, (XtPointer)AlloInfoData(win,  "Progress Meter"),
			XmNheight, 16,
			XmNwidth, 96,
			NULL);
	XtOverrideTranslations(win->meter,
			XtParseTranslationTable(xlattab));
	win->meter_level = 0;
	win->meter_width = -1;

	XtAddCallback(win->meter, XmNexposeCallback,
               		DrawMeter, (XtPointer) win);
	XtAddCallback(win->meter, XmNresizeCallback,
               		ResizeMeter, (XtPointer) win);

	/* grab some colors */
	{
		XColor ccell1,ccell2;

		XAllocNamedColor(mMosaicDisplay, mMosaicColormap ,
			mMosaicAppData.meterForeground,
			&ccell1,&ccell2);
		win->meter_fg = ccell2.pixel;
		XAllocNamedColor(mMosaicDisplay, mMosaicColormap ,
			mMosaicAppData.meterBackground,
			&ccell1,&ccell2);
		win->meter_bg = ccell2.pixel;
		XAllocNamedColor(mMosaicDisplay, mMosaicColormap ,
			mMosaicAppData.meterFontForeground,
			&ccell1,&ccell2);
		win->meter_font_fg = ccell2.pixel;
		XAllocNamedColor(mMosaicDisplay, mMosaicColormap ,
                        mMosaicAppData.meterFontBackground,
                        &ccell1,&ccell2);
       		win->meter_font_bg = ccell2.pixel;
	}

	win->tracker_label = XtVaCreateManagedWidget(" ",xmLabelWidgetClass,
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

	node = win->history;

	POPDOWN (source_win);
	POPDOWN (save_win);
	POPDOWN (savebinary_win);
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
	if(win->mc_user){
		win->mc_user->win = NULL;
		McRemoveMoWin(win->mc_user);
/*####      McDeleteUser(win->mc_user,win->mc_user->mc_list_number);*/
	}
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
		mo_free_node_data (tofree);
		free (tofree);
	}
	win->history=NULL;
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

extern gui_news_updateprefs (mo_window *win);


/* name:    mo_make_window 
 * purpose: Make a new window from scratch.
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

	sprintf(pre_title,"mMosaic %s",MO_VERSION_STRING);
	sprintf(buf,"%s: ",pre_title);
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

	win = (mo_window *)malloc (sizeof (mo_window));
	win->base = base;
	win->mode = moMODE_PLAIN;

	win->upload_win = 0;
#ifdef MULTICAST
	win->mc_type = mc_t;
	win->mc_user = NULL;
#endif
	WM_DELETE_WINDOW = XmInternAtom(mMosaicDisplay, "WM_DELETE_WINDOW", False);
	XmAddWMProtocolCallback(base,WM_DELETE_WINDOW,delete_cb,(XtPointer)win);

	win->source_win = 0;
	win->save_win = 0;
	win->savebinary_win = 0;
	win->ftpput_win = win->ftpremove_win = win->ftpmkdir_win = 0;
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
	win->cci_win = win->cci_win_text = (Widget) 0;
	win->cci_accept_toggle = win->cci_off_toggle = (Widget) 0;
	win->history = NULL;
	win->current_node = 0;
	win->source_text = 0;
	win->format_optmenu = 0;
	win->save_format = mo_plaintext;
	win->agent_state=selectedAgent;
	win->underlines_snarfed = 0;
	win->mail_format = 0;
	win->print_text = 0;
	win->print_format = mo_plaintext;
	win->target_anchor = 0;
	win->search_start = (void *)malloc (sizeof (ElementRef));
	win->search_end = (void *)malloc (sizeof (ElementRef));
	win->src_search_pos=0;
	win->delay_image_loads = mMosaicAppData.delay_image_loads;
	win->font_size = mo_get_font_size_from_res(
		mMosaicAppData.default_font_choice, &(win->font_family));
	win->underlines_state = mo_default_underlines_tkn;
	imageViewInternal = win->image_view_internal = 
				mMosaicAppData.imageViewInternal;
	if (parent) {
		imageViewInternal = win->image_view_internal = 
				 parent->image_view_internal;
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
	XmxRSetToggleState (win->menubar, (XtPointer)mo_image_view_internal,
			(win->image_view_internal ? XmxSet : XmxNotSet));
#ifdef ISINDEX
	win->keyword_search_possible = -1;	/* We don't know yet. */
#endif
	XmxRSetToggleState (win->menubar, (XtPointer)win->font_size, XmxSet);
						/* setup news default states */
	gui_news_updateprefs (win);

	win->binary_transfer = 0;
	XmxRSetToggleState(win->menubar, (XtPointer)mo_binary_transfer,XmxNotSet);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_delay_image_loads,
		 win->delay_image_loads ? XmxSet : XmxNotSet);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_expand_images_current,
		win->delay_image_loads ? XmxSensitive : XmxNotSensitive);

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

		win->body_images = parent->body_images;
		XtVaSetValues(win->scrolled_win,
			WbNbodyImages, win->body_images,
			NULL);
		XmxRSetToggleState (win->menubar, (char*)mo_body_images,
			win->body_images ? XmxSet : XmxNotSet);

		win->delay_image_loads = parent->delay_image_loads;
		XmxSetArg (WbNdelayImageLoads,
			win->delay_image_loads ? True : False);
		XmxSetValues (win->scrolled_win);
		XmxRSetSensitive (win->menubar, (char*)mo_expand_images_current,
			win->delay_image_loads ? XmxSensitive : XmxNotSensitive);
		XmxRSetToggleState (win->menubar, (char*)mo_delay_image_loads,
			win->delay_image_loads ? XmxSet : XmxNotSet);
	}     
	return win;
}

/* name:    mo_duplicate_window
 * purpose: Clone a existing window as intelligently as possible.
 * inputs:  
 *   - mo_window *win: The existing window.
 * returns: 
 *   The new window.
 */
mo_window *mo_duplicate_window (mo_window *win)
{
	mo_window *neww;

	if (win && win->current_node)
		securityType=win->current_node->authType;
	neww = mo_make_window ( win,MC_MO_TYPE_UNICAST);
	mo_duplicate_window_text (win, neww);
	mo_gui_update_meter(100,NULL, neww);
	return neww;
}

/* name:    mo_open_another_window
 * purpose: Open another window to view a given URL, unless the URL
 *          indicates that it's pointless to do so
 *	    Open in UNICAST mode
 * inputs:  
 *   - mo_window      *win: The existing window.
 *   - char           *url: The URL to view in the new window.
 *   - char           *ref: The reference (hyperlink text contents) for this
 *                          URL; can be NULL.
 *   - char *target_anchor: The target anchor to view open opening the
 *                          window, if any.
 * returns: 
 *   The new window.
 */
mo_window *mo_open_another_window (mo_window *win, char *url, char *ref,
                                   char *target_anchor)
{
	mo_window *neww;
	mo_status return_stat = mo_succeed;

	/* Check for reference to telnet.  Never open another window
         * if reference to telnet exists; instead, call mo_load_window_text,
         * which knows how to manage current window's access to telnet. */
	if (!strncmp (url, "telnet:", 7) || !strncmp (url, "tn3270:", 7) ||
	    !strncmp (url, "rlogin:", 7)) {
		mo_load_window_text (win, url, NULL);
		return NULL;
	}
	neww = mo_make_window( win,MC_MO_TYPE_UNICAST);
	/* Set it here; hope it gets handled in mo_load_window_text_first
         * (it probably won't, now. */
	neww->target_anchor = target_anchor;
	return_stat = mo_load_window_text (neww, url, ref); 
	if ((cci_get) && (return_stat == mo_fail))
		return (mo_window *) NULL;
	return neww;
}

extern MO_SIGHANDLER_RETURNTYPE ProcessExternalDirective (MO_SIGHANDLER_ARGS);

/* name:    mo_do_gui
 * purpose: This is basically the real main routine of the application.
 * inputs:  
 *   - int    argc: Number of arguments.
 *   - char **argv: The argument vector.
 * returns: 
 *   nothing
 */
void mo_do_gui ()
{
	mo_window *win;
	McMoWType mc_t; /* Soit type MAIN ou type UNICAST */

/* ##################
	XWMHints *whints=XAllocWMHints();     

        imap = XCreatePixmapFromBitmapData(mMosaicDisplay,
		XtWindow(mMosaicToplevelWidget),
		iconify_bits, iconify_width, iconify_height, 1, 0, 1);
        imaskmap = XCreatePixmapFromBitmapData(mMosaicDisplay,
		XtWindow(mMosaicToplevelWidget),
         	iconify_mask_bits, iconify_mask_width,
		iconify_mask_height, 1, 0, 1);
        whints->flags=IconPixmapHint|IconMaskHint;
        whints->icon_pixmap=imap;     
        whints->icon_mask=imaskmap;   
        XSetWMHints(mMosaicDisplay, XtWindow(mMosaicToplevelWidget),
		whints);
###########*/

	busy_cursor = XCreateFontCursor (mMosaicDisplay, XC_watch);

        XtAppAddActions(mMosaicAppContext, balloon_action, 2);

	mo_init_menubar();	/* definie les boutons avec cb etc... */

	XtRealizeWidget (mMosaicToplevelWidget);

	MakePixmaps(mMosaicToplevelWidget);
	logo_count = 0;

	signal (SIGUSR1, (void(*)(int))ProcessExternalDirective);

/*##################################*/
/* ### Create the FIRST MAIN WINDOW */
/*##################################*/
	mc_t = MC_MO_TYPE_UNICAST;
#ifdef MULTICAST
	if (mc_multicast_enable)
		mc_t = MC_MO_TYPE_MAIN;
#endif
	win = mo_make_window(NULL, mc_t);
	mo_load_window_text (win,
		mMosaicStartupDocument ? mMosaicStartupDocument : mMosaicAppData.home_document, NULL);
	XtVaGetValues(win->scrolled_win,
		WbNbodyColors, &(win->body_color),
		NULL);
	XtVaGetValues(win->scrolled_win,
		WbNbodyImages, &(win->body_images),
		NULL);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_body_color,
			(win->body_color ? XmxSet : XmxNotSet));
	XmxRSetToggleState (win->menubar, (XtPointer)mo_body_images,
			(win->body_images ? XmxSet : XmxNotSet));
#ifdef MULTICAST
	McInit(win);	/* Initialize the multicast database listen mode*/
			/* mc_send_enable become 0 */
#endif

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

	/* Make sure we have a window. */
	win = mo_main_next_window (NULL);

	if (!strncmp (directive, "goto", 4)) {
		CLIP_TRAILING_NEWLINE(url);
		mo_load_window_text (win, url,NULL);
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "newwin", 6)) {
		CLIP_TRAILING_NEWLINE(url);
		/* Force a new window to open. */ 
		mo_open_another_window (win, url, NULL, NULL);
		XmUpdateDisplay (win->base);
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
		XmUpdateDisplay (win->base);
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
		XmUpdateDisplay (win->base);
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
		XmUpdateDisplay (win->base);
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
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "flushcache", 10)) {
		MMCacheClearCache ();
		return;
	}
	if (!strncmp (directive, "backnode", 8)) {
		mo_back_node (win);
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "forwardnode", 11)) {
		mo_forward_node (win);
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "reloaddocument", 14)) {
		mo_reload_window_text (win);
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "reloadimages", 12)) {
		mo_reload_window_text (win);
		XmUpdateDisplay (win->base);
		return;
	}
	if (!strncmp (directive, "refresh", 7)) {
		mo_refresh_window_text (win);
		XmUpdateDisplay (win->base);
		return;
	}
	return;
}

void mo_flush_passwd_cache (mo_window *win)
{
	HTFTPClearCache ();
	HTAAServer_clear ();
	mo_gui_notify_progress ("Password cache flushed",win);
}  
