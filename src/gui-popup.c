/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <assert.h>

#include "../libhtmlw/HTMLP.h"
#include "mosaic.h"  
#include "gui.h"
#include "gui-popup.h"
#include <X11/Xmu/StdSel.h>
#include "../libnut/system.h"
#include "../libhtmlw/HTMLPutil.h"
#include "hotlist.h"
#include "gui-documents.h"
#include "gui-dialogs.h"
#include "gui-menubar.h"
#include "URLParse.h"
#include "mime.h"
#include "paf.h"
#include "navigate.h"

#ifdef DEBUG
#define DEBUG_GUI
#endif

int do_meta;	/*############ FIXME*/

/* static void	fsb(mo_window *win, char *src); */
void 		rbm_ballonify(Widget w,XtPointer clid, XtPointer calld);

static void fmenu_cb ( Widget w , XtPointer clid, XtPointer calld)
{
        act_struct *acst = (act_struct *) clid;
        struct ele_rec *eptr;
        int which;
	mo_window * win;
	RequestDataStruct rds;
              
	assert(acst);		/* when is it NULL? ; let me know */
	win = acst->win;
        which = (int)acst->act_code;
        switch (which) { 
        case M_FileData:              /* ### FIXME : get Meta file Data... */
        	eptr = acst->eptr;
        	if(!eptr)                     
                	return; /* oh, well */
                do_meta=1;		/* ###FIXME: do_meta doesnot work */
		rds.req_url = win->current_node->aurl_wa;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
		MMPafLoadHTMLDocInWin(win, &rds);
		do_meta=0;             
		break;    
        default:                      
		(*(acst->act_code))(w, (XtPointer) acst->win, calld); 
               break;                 
        }
        return; 
}

static void hotmenu_cb ( Widget w , XtPointer clid, XtPointer calld)
{
/* ###############*/
}

static Widget save_link_fsb_dialog = NULL;

static void save_link_as_ok_cb(Widget w, XtPointer clid, XtPointer calld)
{
	XmFileSelectionBoxCallbackStruct *cbs =
		(XmFileSelectionBoxCallbackStruct *) calld;
	char *filename, *url = (char *) clid ;

/* Remove the widget from the screen.  */
	XtUnmanageChild ( w );
/* Retrieve the character string from the compound string format.  */
	XmStringGetLtoR ( cbs->value, XmSTRING_DEFAULT_CHARSET, &filename );

/* Next step is to alloc an environnement and popup a autonom scale */
/* we have the url to get and the filename for saving data. */
/* that is enought to copy url data to filename */

	MMPafSaveData(mMosaicToplevelWidget, url, filename);
}

static void save_link_as_cancel_cb(Widget w, XtPointer clid, XtPointer calld)
{
	XtUnmanageChild ( w );
}

void PopSaveLinkFsbDialog(char * url)
{
	XmString defdir;
	char * sdefdir;
	char * fname;
	int i;
	Arg args[2];

	fname = getFileName(url);
	fname = strdup(fname);

/* Now we have the canon URL and the filename part of this url */
/* Next step is to create a Unique fsb with XmDIALOG_FULL_APPLICATION_MODAL */
/* if it does not exist yet */
	if (!save_link_fsb_dialog) { /* create an fsb */
		i = 0;
		XtSetArg(args[i], XmNdialogStyle,
			XmDIALOG_FULL_APPLICATION_MODAL);i++;
		save_link_fsb_dialog = XmCreateFileSelectionDialog(
			mMosaicToplevelWidget,
			"Save Link As", args, i);
		i = 0;
		XtAddCallback(save_link_fsb_dialog,
			XmNcancelCallback, save_link_as_cancel_cb, NULL);
		XtAddCallback(save_link_fsb_dialog,
			XmNokCallback, save_link_as_ok_cb, url);

		XtSetSensitive(XmFileSelectionBoxGetChild(save_link_fsb_dialog,
                                XmDIALOG_HELP_BUTTON), False);
		XtVaSetValues(save_link_fsb_dialog,
				XmNfileTypeMask, XmFILE_REGULAR, NULL);
	} else {
		/* delete previous callback not need */
		XtRemoveAllCallbacks(save_link_fsb_dialog,XmNokCallback);
		/* add our callback */
		XtAddCallback( save_link_fsb_dialog,
			XmNokCallback, save_link_as_ok_cb, url);
		/* Init the box */
		XmFileSelectionDoSearch(save_link_fsb_dialog,NULL);
	}	
/* set default file string in selection box */
	XtVaGetValues(save_link_fsb_dialog, XmNdirSpec, &defdir, NULL);
	XmStringGetLtoR(defdir,XmSTRING_DEFAULT_CHARSET,&sdefdir);
	XmStringFree(defdir);
	if (sdefdir){
		char * tmpbuf;
		tmpbuf = (char*) malloc ( strlen(sdefdir) + strlen(fname) + 10);
		sprintf(tmpbuf, "%s%s",sdefdir,fname);
		defdir = XmStringCreateLtoR(tmpbuf,XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(save_link_fsb_dialog, XmNdirSpec,defdir, NULL);
		XmStringFree(defdir);
		free(tmpbuf);
	}
	XtManageChild(save_link_fsb_dialog);
}

static void save_link_as_cb ( Widget w , XtPointer clid, XtPointer calld)
{
        act_struct *acst = (act_struct *) clid;
        struct ele_rec *eptr;
	mo_window * win;
	char * url ;

	assert(acst);		/* let me know why */
	win = acst->win;

        if(!acst->eptr)
                return;              

        eptr = acst->eptr;           

        if( !eptr->anchor_tag_ptr && !eptr->anchor_tag_ptr->anc_href)
                return;		/* no ref for this anchor !!! */

	url = eptr->anchor_tag_ptr->anc_href;
	url = mo_url_canonicalize(url, win->current_node->base_url);
	PopSaveLinkFsbDialog(url);
}

static void popup_back_cb ( Widget w , XtPointer clid, XtPointer calld)
{
	act_struct *acst = (act_struct *) clid;
	mo_window *win;

	assert(acst);	/* let me know */

	win=acst->win;
	mo_back (w, (XtPointer)win, NULL);
}

static void popup_forward_cb ( Widget w , XtPointer clid, XtPointer calld)
{
	act_struct *acst = (act_struct *) clid;
	mo_window *win;                

	assert(acst);	/* let me know */
	
	win=acst->win;                 
	mo_forward(w, (XtPointer)win, NULL);
}

static void image_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	char *xurl;
	struct ele_rec *eptr;
	int which; 
	mo_window * win;
	RequestDataStruct rds;

	assert(acst);

	eptr = acst->eptr;
	which = (int)acst->act_code;
	win = acst->win;

	if(!eptr || !eptr->pic_data->image_data || !eptr->pic_data->src){
		printf("Lost source.\n");
		return; /* oh, well */
	}

	switch(which) {
	case I_Save:	 /* #### look at save_link_as_cb  ### */
		xurl=eptr->pic_data->src;
		xurl = mo_url_canonicalize(xurl, win->current_node->base_url);
		PopSaveLinkFsbDialog(xurl);
		break;
	case I_ViewExternal:		/* ### FIXME : is it in external viewer */
		xurl=eptr->pic_data->src;
		rds.req_url = xurl;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
               	MMPafLoadHTMLDocInWin(win, &rds);
		break;
	case I_ViewInternal:	/* ### FIXME : is it in mMosaic ? */
		xurl=eptr->pic_data->src;
		rds.req_url = xurl;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
               	MMPafLoadHTMLDocInWin(win, &rds);
		break;
	case I_Reload:
		mo_reload_document(w, (XtPointer) win, NULL);
		break;
	}
}

static Boolean convert_selection(Widget w, Atom *sel, Atom *tar, Atom *typ_ret,
				 XtPointer *val_ret, unsigned long *val_len,
				 int *format);
static char* the_copyed_url = NULL;

static void copy_link_cb(Widget w, XtPointer clid, XtPointer calld)
{
	act_struct *acst = (act_struct *) clid;
	XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct *)calld;
	mo_window * win = acst->win;
	int i;
	char *copy_str;

	assert(acst);		/* let me know */
	if( !acst->eptr || !acst->eptr->anchor_tag_ptr->anc_href ||
	   !*acst->eptr->anchor_tag_ptr->anc_href)
		return;

	if (the_copyed_url){
		free(the_copyed_url);	/* free previous */
		the_copyed_url = NULL;
	}

	the_copyed_url= mo_url_canonicalize(acst->eptr->anchor_tag_ptr->anc_href, 
			win->current_node->base_url);

	if(!the_copyed_url)
		return;

	if(XtOwnSelection(win->scrolled_win, XA_PRIMARY, 
	    cbs->event->xbutton.time, convert_selection, NULL, NULL) == False) {
		fprintf(stderr, "Mosaic: Error: Could not copy selection, try again.\n");
		if (the_copyed_url) {
			free(the_copyed_url);
			the_copyed_url = NULL;
		}
	}

/* Selection succes */

	copy_str = (char*) malloc(2+ strlen(the_copyed_url) +
			    strlen("URL:   has been copied  ") );

	sprintf(copy_str, "URL: %s  has been copied",the_copyed_url);
	mo_gui_notify_progress(copy_str,win);
}

static void metadata_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	char *xurl;                          
	struct ele_rec *eptr;                
	int which;                           
	mo_window * win;
	RequestDataStruct rds;

	assert(acst);	/* let me know */

	if(!acst->eptr)
		return;              

	eptr = acst->eptr;           
	which = (int)acst->act_code;      
	win = acst->win;
                                     
	switch (which) {             
	case M_ImageData:  
		if (!eptr->pic_data) { /* do what? */
			return;
		}            
		xurl=eptr->pic_data->src;
		do_meta=1;   	/* ### FIXME : what do 'do_meta' ? */
		rds.req_url = xurl;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
		MMPafLoadHTMLDocInWin(win, &rds);
		do_meta=0;   
		break; 
	case M_LinkData:   
		if (!eptr->anchor_tag_ptr->anc_href || 
		    !*eptr->anchor_tag_ptr->anc_href) {
			return;
		}            
		xurl=mo_url_canonicalize(eptr->anchor_tag_ptr->anc_href,
			win->current_node->base_url);
		do_meta=2;		/* ### FIXME : what do 'do_meta' ? */
		rds.req_url = xurl;
		rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
		MMPafLoadHTMLDocInWin(win, &rds);
		do_meta=0;   
		break;       
	default:
		fprintf(stderr,"Smells like an error...\n");
		break;  
	}                            
	return;                      
}                                    

static PopupItem model_image_menu[] = {
  {  PushButton, 0, 0, 0, 0, "Save", 
     { (XtCallbackProc)I_Save, NULL, NULL,NULL},
     image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "Reload", 
     {(XtCallbackProc)I_Reload, NULL, NULL,NULL},
     image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "View External", 
     {(XtCallbackProc)I_ViewExternal, NULL, NULL,NULL}, 
     image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "View Internal",
     {(XtCallbackProc)I_ViewInternal, NULL, NULL,NULL}, 
     image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "Get Image Metadata", 
     {(XtCallbackProc)M_ImageData, NULL, NULL,NULL},
     metadata_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
  { LastItem }
};

static PopupItem model_file_menu[] = {

  {PushButton, 0, 0, 0, 0, "Save Page",
    {mo_save_document, NULL, NULL,NULL},
    fmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "Print", 
    {mo_print_document, NULL, NULL,NULL},
    fmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "Mail To", 
    {mo_mail_document, NULL, NULL,NULL},
    fmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, 0, 0, 0, 0, "Get File Metadata", 
    {(XtCallbackProc)M_FileData, NULL, NULL,NULL},
    fmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
  { LastItem },
};

static PopupItem model_popup_items[] = {	 /* Permanent stuff */

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE,"Back", 
   {NULL, NULL, NULL,NULL},
   popup_back_cb, 0, NULL, NULL, NULL, 0, NULL, 1,NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Forward", 
   {NULL, NULL, NULL,NULL},
   popup_forward_cb, 0, NULL, NULL, NULL, 0, NULL, 1,NULL}, 

  {CascadeButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Session History",
   {(XtCallbackProc)-2, NULL, NULL,NULL},
    NULL, 0, NULL, NULL, NULL, 0, NULL, 1,NULL},

/* Stuff if on a html page and not on a image or anchor */

  {Separator, (E_TEXT | E_BULLET | E_LINEFEED | E_WIDGET | E_HRULE |E_TABLE),
   LOOSE, moMODE_ALL, LOOSE, "Separator",
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1,NULL}, 

  {CascadeButton, (E_TEXT | E_BULLET | E_LINEFEED | E_WIDGET | E_HRULE |
                   E_TABLE),LOOSE,  moMODE_ALL, LOOSE, "File",
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, model_file_menu, sizeof(model_file_menu), NULL, 1, NULL},

/*---------------------------------------------------------------
    Stuff if on any page and an anchor (including image anchor)
---------------------------------------------------------------*/
                                     
  {Separator, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, "Separator",
   {0, NULL, NULL,NULL},                  
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
                                     
/* Load to local Disk  when in an anchor (binary mode) */
  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, "Save Link As ...",
   {0, NULL, NULL,NULL},
    save_link_as_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, COPY_URL_LABEL,
   {0, NULL, NULL,NULL},
   copy_link_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
                                     
  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE,"Get Link Metadata",
   {(XtCallbackProc)M_LinkData, NULL, NULL,NULL},
   metadata_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

/* Stuff if on any page and a image (not including image link) */

  {Separator, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Separator", 
   {0, NULL, NULL,NULL}, NULL, 0, NULL, NULL, NULL, 0, NULL, 1, NULL}, 

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Save", 
   {(XtCallbackProc)I_Save, NULL, NULL,NULL},
   image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Reload", 
   { (XtCallbackProc)I_Reload, NULL, NULL,NULL},
   image_cb, 0, NULL, NULL, NULL,  0, NULL, 1, NULL},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "View External", 
   { (XtCallbackProc)I_ViewExternal, NULL, NULL,NULL},
   image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "View Internal", 
   { (XtCallbackProc)I_ViewInternal, NULL, NULL,NULL},
   image_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Get Image Metadata",
   {(XtCallbackProc)M_ImageData, NULL, NULL,NULL},
    metadata_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

/* Stuff if on any page and a image link
 ---------------------------------------------------------------*/

  {Separator, E_IMAGE | E_ANCHOR, TIGHT, moMODE_PLAIN, LOOSE, "Separator",  
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1,NULL}, 

  {CascadeButton, E_IMAGE | E_ANCHOR, TIGHT, moMODE_PLAIN, LOOSE, "Image",  
   {0, NULL, NULL,NULL}, 
   NULL, 0, NULL, NULL, model_image_menu, sizeof(model_image_menu), NULL, 1,NULL},

#ifdef NEWS
/* Stuff if on a news page and not a link
---------------------------------------------------------------*/

  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Separator", 
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1, NULL}, 

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Next Article", 
   {mo_news_next, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Previous Article", 
   {mo_news_prev, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Next Thread", 
   {mo_news_nextt, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Previous Thread", 
   {mo_news_prevt, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Article Index", 
   {mo_news_index, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Group Index", 
   {mo_news_groups, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
  
  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, NULL, 
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Post", 
   {mo_news_post, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Followup", 
   {mo_news_follow, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  
  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, NULL, 
   {0, NULL, NULL,NULL},
   NULL, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Subscribe",
   {mo_news_sub, NULL, NULL,NULL},
   /*######*/hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Unsubscribe",
   {mo_news_unsub, NULL, NULL,NULL},
/*######*/hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Subscribe",
   {mo_news_sub_anchor, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
   
  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Unsubscribe",
   {mo_news_unsub_anchor, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
   
  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Mark Group Read",
   {mo_news_mread, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},

  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Mark Group Read",
   {mo_news_mread_anchor, NULL, NULL,NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, 0, NULL, 1, NULL},
#endif

  { LastItem },
};

static Widget _PopupMenuBuilder(mo_window * win, Widget parent, int type, char *title, 
			char mnem, PopupItem *items)
{
	Widget menu, cascade;
	XmString str;
	int i, mapping_del;

	mapping_del = mMosaicAppData.popupCascadeMappingDelay;
	if(type == XmMENU_POPUP) {
		menu = XmCreatePopupMenu(mMosaicToplevelWidget, title, NULL, 0);
	} else if(type == XmMENU_PULLDOWN) { 
		menu = XmCreatePulldownMenu(parent, title, NULL, 0);
		str = XmStringCreateLtoR(title, XmSTRING_DEFAULT_CHARSET);
		cascade =XtVaCreateManagedWidget(title,xmCascadeButtonGadgetClass,
				parent, XmNsubMenuId, menu,
				XmNlabelString, str,
				XmNmnemonic, mnem, 
				XmNmappingDelay, mapping_del, NULL);
		XmStringFree(str);
	} else
		return (Widget) NULL; /* this shouldn't happen */

	for(i=0;(items[i].classw!=LastItem); i++) {
		switch(items[i].classw) {
		case PushButton:
			items[i]._w = XtVaCreateManagedWidget(items[i].label, 
					xmPushButtonGadgetClass, menu,
					XmNuserData, (XtPointer) win, NULL);
			items[i].win = win;
			if(items[i].mnemonic)
				XtVaSetValues(items[i]._w,
					XmNmnemonic, items[i].mnemonic, NULL);
			if(items[i].accel) {
				XtVaSetValues(items[i]._w, 
					XmNaccelerator , items[i].accel, 
					NULL);
			}
			if(items[i].accel_text) {
				str = XmStringCreateLtoR(items[i].accel_text,
					XmSTRING_DEFAULT_CHARSET);
				XtVaSetValues(items[i]._w, 
					XmNacceleratorText, str, NULL);
				XmStringFree(str);
			}
			if(items[i].cbfp)
				XtAddCallback(items[i]._w, XmNactivateCallback, 
					items[i].cbfp, &(items[i].acst));
			XtSetSensitive(items[i]._w, items[i].startup);
			break;
		case Separator:
			items[i].win = win;
			items[i]._w = XtVaCreateManagedWidget(items[i].label, 
					xmSeparatorGadgetClass, menu, NULL);
			break;
		case ToggleButton:
			items[i].win = win;
			items[i]._w = XtVaCreateManagedWidget(items[i].label, 
					xmToggleButtonGadgetClass, menu, NULL);
			break;
		case CascadeButton:
			items[i].win = win;
			if(items[i].sub_items && 
			   (items[i].acst.act_code !=(XtCallbackProc) -2)) {
				PopupItem *private_items;
/* that's a model */
				private_items = (PopupItem *)calloc(1,items[i].sub_items_size);
				memcpy(private_items,items[i].sub_items,items[i].sub_items_size);
				items[i].sub_items = private_items;
				items[i]._w = _PopupMenuBuilder(win, menu,
					XmMENU_PULLDOWN, items[i].label, 
					items[i].mnemonic, items[i].sub_items);
			} else {                      
				items[i]._w = XtVaCreateManagedWidget(
					"Session History",
					xmCascadeButtonGadgetClass,
					menu, XmNsubMenuId,
					win->session_menu,
					XmNmappingDelay, mapping_del, 
					NULL);
			} 
		}
	}
	return type == XmMENU_POPUP ? menu : cascade;
}

static void _set_eptr_field(PopupItem *items, struct ele_rec *eptr, mo_window *win)
{
	int i;

	for(i=0; items[i].classw != LastItem; i++) {
		if(items[i].sub_items)
			_set_eptr_field(items[i].sub_items, eptr,win);
		items[i].acst.eptr = eptr;
		items[i].acst.win = win;
	}
}

static void ThirdButtonMenu(Widget w, XtPointer clid, XEvent *event,
     Boolean *ctd)
{
	mo_window * win = (mo_window*) clid;
	Widget popup = win->popup_b3;
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *) event;
	static struct ele_rec *eptr;
	int epos, mode, type, i, del=12;
	int sens=False;
	HTMLWidget hw = (HTMLWidget) w;
	PopupItem *popup_items;

#ifdef DEBUG_B3
	fprintf(stderr,"win->frame_type = %d\n", win->frame_type);
#endif

	if( ! (BuEvent->type == ButtonPress))
		return;
	if( ! (BuEvent->button == Button3) )
		return;

/* le popup appartient au frameset ou au frame */
/* le popup doit etre creer a chaque creation de page... */

      	while((hw != NULL) && (XtClass((Widget) hw) != htmlWidgetClass))
        	hw = (HTMLWidget) XtParent((Widget) hw);
      	if(hw == NULL)                                                        
        	return; 

	assert(popup);

      	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);
	mode = win->mode;

	if(eptr) {
		type = eptr->type; 
		if((type == E_IMAGE) && (eptr->anchor_tag_ptr->anc_href)) { 
				/* turn on anchor and off text */
			type |= E_ANCHOR;
			if(type & E_TEXT)
				type -= E_TEXT;
		}
		if((type == E_TEXT) && (eptr->anchor_tag_ptr->anc_href))
			type = E_ANCHOR;
	} else
		type = E_HRULE; /* pick a good normal little element */
 
	popup_items = win->popup_b3_items;

	for(i = 0; popup_items[i].classw != LastItem; i++) {
		if(popup_items[i]._w) {/*anything is possible in Mosaic */
			int good = True;
      
              /* take care of session menu */
              if(popup_items[i].acst.act_code ==(XtCallbackProc) -2)
                XtVaSetValues(popup_items[i]._w, XmNsubMenuId,
                              win->session_menu, NULL);
                                     
              /* determine if we want this guy */
			if(popup_items[i].types_method == LOOSE)
				good=good &&(popup_items[i].types & type);
			else
				good=good &&(popup_items[i].types ==type);
			if(popup_items[i].modes_method == LOOSE)
				good=good &&(popup_items[i].modes & mode);
			else
				good=good &&(popup_items[i].modes ==mode);
			if(good) {
				if(popup_items[i].classw == Separator)
					del+=4;
				else {
					switch(mode) {
					case moMODE_PLAIN:
						if(!(type & E_IMAGE)) {
							if(!XtIsSensitive(popup_items[i]._w) && !sens)
								del+=24;
							else
								sens=True;
							break;
						}
					case moMODE_NEWS:
					case moMODE_FTP:
						if(i > 3){
								 /* skip forward, backward, hotlist, and sep */
							if(!XtIsSensitive(popup_items[i]._w) && !sens)
								del+=24;
							else
								sens=True;
						} else
							del+=24;
						break;
					}
				}
				XtManageChild(popup_items[i]._w); 
			} else
				XtUnmanageChild(popup_items[i]._w);
		}
	} /* for */
			/* set all the widgets eptr data */
	_set_eptr_field(popup_items, eptr,win);

			/*motif puts menu in a boring place lets fix it*/ 
			/* BuEvent->x_root -= 40;  middle of buttons*/
	BuEvent->y_root -= del; /* first active button or first specialty
				 *item if we're over element that has em*/
	XmMenuPosition(popup, BuEvent);
	XtManageChild(popup);
}

void mo_make_popup(mo_window * win)
{
	Widget view;
	PopupItem *private_items;

	view = win->view;
	win->popup_b3 = NULL;
	win->popup_b3_items = NULL;

	private_items = (PopupItem *)calloc(1,sizeof(model_popup_items));
	memcpy(private_items,model_popup_items,sizeof(model_popup_items));
	win->popup_b3 = _PopupMenuBuilder(win, (Widget) view, XmMENU_POPUP, 
		"popup", 0, private_items);
	win->popup_b3_items = private_items;

	XtInsertEventHandler(view, ButtonPressMask, False,
                       (XtEventHandler)ThirdButtonMenu, (XtPointer)win, 
                       XtListHead);
}

void mo_destroy_popup(mo_window * win)
{
	XtDestroyWidget(win->popup_b3);
	win->popup_b3 = NULL;
	free(win->popup_b3_items);
	win->popup_b3_items = NULL;
}

void mo_popup_set_something(char *what, int to, PopupItem *items)
{
	Widget w=NULL;
	int i;

	assert(items);

	for(i=0; (items[i].classw != LastItem) && !w; i++) {
		if(items[i].label && (items[i].label[0] == *what)) {
			if(!strcmp(items[i].label, what)) {
				w=items[i]._w;
				if(!w) {
					items[i].startup = to;
					break;
				}
			}
		}

		if(items[i].sub_items)
			mo_popup_set_something(what, to,items[i].sub_items);
	}
	if(w)
		XtSetSensitive(w, to);
}

void fsb_CancelCallback ( Widget w, XtPointer clientData, XtPointer callData )
{
	XtUnmanageChild ( w );
}

void ungrab_the_____ing_pointer(XtPointer client_data)
{
	XUngrabPointer (mMosaicDisplay, CurrentTime);
}

static Boolean convert_selection(Widget w, Atom *sel, Atom *tar, Atom *typ_ret,
				 XtPointer *val_ret, unsigned long *val_len,
				 int *format)
{
	char *url;
 
	if(*tar == XA_STRING) {
#ifdef DEBUG_GUI
		if (mMosaicSrcTrace) { 
			fprintf (stderr, "Pasting text selection.\n");
		}
#endif
		if(the_copyed_url)
			url = the_copyed_url;
		else
			return False;
		*val_ret = strdup(url);
		*val_len = strlen(url);
		*typ_ret = XA_STRING;
		*format = 8;
		return(True);
	}
	return(False); 
}

void rbm_ballonify(Widget w, XtPointer clid, XtPointer calld)
{
	char *url = (char *) clid;
	mo_window * win;

	XtVaGetValues(w, XmNuserData, (XtPointer) &win, NULL);
	mo_gui_notify_progress(url,win);
}
