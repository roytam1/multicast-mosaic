/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

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

int do_meta;	/*############*/
Boolean 	have_popup;
Widget 		popup = NULL;
static void	fsb(mo_window *win, char *src);
void 		rbm_ballonify(Widget w,XtPointer clid, XtPointer calld);
static Widget 	_PopupMenuBuilder(mo_window * win, Widget parent,
			int type, char *title, char mnem, PopupItem *items);

static void fmenu_cb ( Widget w , XtPointer clid, XtPointer calld)
{
/* ###############*/
        act_struct *acst = (act_struct *) clid;
        char *xurl;
        struct ele_rec *eptr;
        int which;
	mo_window * win = acst->win;
	RequestDataStruct rds;
              
        which = (int)acst->act_code;
        switch (which) { 
        case M_FileData:              
        	if(!acst || !acst->eptr)
                	return;
        	eptr = acst->eptr;
        	if(!eptr)                     
                	return; /* oh, well */
               do_meta=1;
		rds.req_url = win->current_node->aurl_wa;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
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
	char *filename, *url = (char *) clid, efilename[MO_LINE_LENGTH];

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
	mo_window * win = acst->win;
	char * url ;

        if(!acst || !acst->eptr)
                return;              
        eptr = acst->eptr;           
        if(!eptr && !eptr->anchor_tag_ptr && !eptr->anchor_tag_ptr->anc_href)
                return;		/* no ref for this anchor !!! */

	url = eptr->anchor_tag_ptr->anc_href;
	url = mo_url_canonicalize(url, win->current_node->base_url);
	PopSaveLinkFsbDialog(url);
}

PopupItem image_menu[] = {
  {PushButton, 0, 0, 0, 0, "Save", {(XtCallbackProc)I_Save, NULL, NULL}, image_cb, 0, NULL, 
   NULL,  NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Reload", {(XtCallbackProc)I_Reload, NULL, NULL}, image_cb, 0, 
   NULL, NULL, NULL,  NULL, 1},
  {PushButton, 0, 0, 0, 0, "View External", {(XtCallbackProc)I_ViewExternal, NULL, NULL}, 
   image_cb, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "View Internal", {(XtCallbackProc)I_ViewInternal, NULL, NULL}, 
   image_cb, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Get Image Metadata", {(XtCallbackProc)M_ImageData, NULL, NULL},
   metadata_cb, 0, NULL, NULL, NULL, NULL, 1},
  { LastItem }
};

PopupItem file_menu[] = {

  {PushButton, 0, 0, 0, 0, "Save Page", {mo_save_document, NULL, NULL},
   fmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Print", {mo_print_document, NULL, NULL},
   fmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Mail To", {mo_mail_document, NULL, NULL},
   fmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Get File Metadata", {(XtCallbackProc)M_FileData, NULL, NULL},
   fmenu_cb, 0, NULL, NULL, NULL, NULL, 1},
  { LastItem },
};

PopupItem popup_items[] = {		 /* Permanent stuff */

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE,"Back", 
   {mo_back, NULL, NULL},
   hotmenu_cb, 0, "B", NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Forward", 
   {mo_forward, NULL, NULL},
   hotmenu_cb, 0, "F", NULL, NULL, NULL, 1}, 

  {CascadeButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Session History",
   {(XtCallbackProc)-2, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1},

/* Stuff if on a html page and not on a image or anchor */

  {Separator, (E_TEXT | E_BULLET | E_LINEFEED | E_WIDGET | E_HRULE |E_TABLE),
   LOOSE, moMODE_ALL, LOOSE, "Separator", {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {CascadeButton, (E_TEXT | E_BULLET | E_LINEFEED | E_WIDGET | E_HRULE |
                   E_TABLE),LOOSE,  moMODE_ALL, LOOSE, "File",
   {0, NULL, NULL}, NULL, 0, NULL, NULL, file_menu, NULL, 1},
/*---------------------------------------------------------------
    Stuff if on any page and an anchor (including image anchor)
---------------------------------------------------------------*/
                                     
  {Separator, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, "Separator",
   {0, NULL, NULL},                  
   NULL, 0, NULL, NULL, NULL, NULL, 1},
                                     
/* Load to local Disk  when in an anchor (binary mode) */
  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, "Save Link As ...",
   {0, NULL, NULL}, save_link_as_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, COPY_URL_LABEL,
   {0, NULL, NULL}, copy_link_cb, 0, NULL, NULL, NULL, NULL, 1},
                                     
  {PushButton, E_ANCHOR | E_IMAGE, LOOSE, moMODE_ALL, LOOSE, "Get Link Metadata",                                  
   {(XtCallbackProc)M_LinkData, NULL, NULL}, metadata_cb, 0, NULL, NULL, NULL, NULL, 1},

/* Stuff if on any page and a image (not including image link) */

  {Separator, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Separator", 
   {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Save", 
   {I_Save, NULL, NULL}, image_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Reload", 
   { (XtCallbackProc)I_Reload, NULL, NULL},
   image_cb, 0, NULL, NULL, NULL,  NULL, 1},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "View External", 
   { (XtCallbackProc)I_ViewExternal, NULL, NULL}, image_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "View Internal", 
   { (XtCallbackProc)I_ViewInternal, NULL, NULL}, image_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_IMAGE, TIGHT, moMODE_ALL, LOOSE, "Get Image Metadata",
   {(XtCallbackProc)M_ImageData, NULL, NULL}, metadata_cb, 0, NULL, NULL, NULL, NULL, 1},

/* Stuff if on any page and a image link
 ---------------------------------------------------------------*/

  {Separator, E_IMAGE | E_ANCHOR, TIGHT, moMODE_PLAIN, LOOSE, "Separator",  
   {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {CascadeButton, E_IMAGE | E_ANCHOR, TIGHT, moMODE_PLAIN, LOOSE, "Image",  
   {0, NULL, NULL}, 
   NULL, 0, NULL, NULL, image_menu, NULL, 1},

#ifdef NEWS
/* Stuff if on a news page and not a link
---------------------------------------------------------------*/

  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Separator", 
   {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Next Article", 
   {mo_news_next, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Previous Article", 
   {mo_news_prev, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Next Thread", 
   {mo_news_nextt, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Previous Thread", 
   {mo_news_prevt, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Article Index", 
   {mo_news_index, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Group Index", 
   {mo_news_groups, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},
  
  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, NULL, 
   {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Post", 
   {mo_news_post, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, "Followup", 
   {mo_news_follow, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  
  {Separator, ALL_TYPES, LOOSE, moMODE_NEWS, TIGHT, NULL, 
   {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Subscribe",
   {mo_news_sub, NULL, NULL},
   /*######*/hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Unsubscribe",
   {mo_news_unsub, NULL, NULL},
/*######*/hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Subscribe",
   {mo_news_sub_anchor, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},
   
  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Unsubscribe",
   {mo_news_unsub_anchor, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},
   
  {PushButton, NEWS_NOANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Mark Group Read",
   {mo_news_mread, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, E_ANCHOR, LOOSE, moMODE_NEWS, TIGHT, "Mark Group Read",
   {mo_news_mread_anchor, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},
#endif

  { LastItem },
};

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
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *) event;
	static struct ele_rec *eptr;
	int epos, mode, type, i, del=12;
	int sens=False;
	HTMLWidget hw = (HTMLWidget) w;

	if( !(have_popup && (BuEvent->button == Button3)))
		return;

 
      	while((hw != NULL) && (XtClass((Widget) hw) != htmlWidgetClass))
        	hw = (HTMLWidget) XtParent((Widget) hw);
      	if(hw == NULL)                                                        
        	return; 
      	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);
	if(!popup) {
		popup = _PopupMenuBuilder(win, (Widget) w, XmMENU_POPUP, 
			"popup", 0, popup_items);
	}
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

	view = win->view;
	have_popup = True; /* this will cause it to be created later */
	popup = NULL;
	XtInsertEventHandler(view, ButtonPressMask, False,
                       (XtEventHandler)ThirdButtonMenu, (XtPointer)win, 
                       XtListHead);
}

void mo_popup_set_something(char *what, int to, PopupItem *items)
{
	Widget w=NULL;
	int i;

	if(items == NULL)
		items = popup_items;

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

static Widget _PopupMenuBuilder(mo_window * win, Widget parent, int type, char *title, 
			char mnem, PopupItem *items)
{
	Widget menu, cascade;
	XmString str;
	int i, mapping_del;

	if(type == XmMENU_POPUP) {
		menu =  XmCreatePopupMenu(parent, title, NULL, 0);
	} else if(type == XmMENU_PULLDOWN) { 
		menu = XmCreatePulldownMenu(parent, title, NULL, 0);
		str = XmStringCreateLtoR(title, XmSTRING_DEFAULT_CHARSET);
		mapping_del = mMosaicAppData.popupCascadeMappingDelay;
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
			   (items[i].acst.act_code !=(XtCallbackProc) -2))
				items[i]._w = _PopupMenuBuilder(win, menu,
					XmMENU_PULLDOWN, items[i].label, 
					items[i].mnemonic, items[i].sub_items);
			else {                      
				int mapping_del;     

				mapping_del = 
					mMosaicAppData.popupCascadeMappingDelay;
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

void metadata_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	char *xurl;                          
	struct ele_rec *eptr;                
	int which;                           
	mo_window * win;
	RequestDataStruct rds;

        if(!acst || !acst->eptr)
                return;              
                                     
        eptr = acst->eptr;           
        which = (int)acst->act_code;      
	win = acst->win;
                                     
        if(!eptr)
                return; /* oh, well */
                                     
        switch (which) {             
        case M_ImageData:  
               if (!eptr->pic_data) { /* do what? */
                                return;
               }            
               xurl=eptr->pic_data->src;
               do_meta=1;   
		rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
               MMPafLoadHTMLDocInWin(win, &rds);
               do_meta=0;   
               break; 
        case M_LinkData:   
               if (!eptr->anchor_tag_ptr->anc_href || !*eptr->anchor_tag_ptr->anc_href) {
               		return;
               }            
               xurl=mo_url_canonicalize(eptr->anchor_tag_ptr->anc_href,
               		win->current_node->base_url);
               do_meta=2;
		rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
               MMPafLoadHTMLDocInWin(win, &rds);
               do_meta=0;   
               break;       
        default:
               fprintf(stderr,"Smells like an error...\n");
               break;  
        }                            
        return;                      
}                                    

void image_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	char *xurl;
	struct ele_rec *eptr;
	int which,tmp; 
	mo_window * win;
	RequestDataStruct rds;

	eptr = acst->eptr;
	which = (int)acst->act_code;
	win = acst->win;

	if(!eptr || !eptr->pic_data->image_data || !eptr->pic_data->src){
		printf("Lost source.\n");
		return; /* oh, well */
	}

	switch(which) {
/* #### look at save_link_as_cb  ###
	case I_Save:
		/* FIXME: this should be fsb(eptr->edata); */
/*
		fsb(win,eptr->pic_data->src);
		break;
### */
	case I_ViewExternal:
		xurl=eptr->pic_data->src;
		rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
               MMPafLoadHTMLDocInWin(win, &rds);
		break;
	case I_ViewInternal:
		xurl=eptr->pic_data->src;
/*
		tmp=imageViewInternal;
		imageViewInternal=1;
*/
		rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
		rds.post_data = NULL;
		rds.ct = NULL;
		rds.is_reloading = False;
               MMPafLoadHTMLDocInWin(win, &rds);
/*
		imageViewInternal=tmp;
*/
		break;
	case I_Reload:
		mo_reload_document(w, (XtPointer) win, NULL);
		break;
	}
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
				 int *format);
void copy_link_cb(Widget w, XtPointer clid, XtPointer calld)
{
	act_struct *acst = (act_struct *) clid;
	XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct *)calld;
	char *url;
	mo_window * win = acst->win;

	if(!acst || !acst->eptr || !acst->eptr->anchor_tag_ptr->anc_href ||
	   !*acst->eptr->anchor_tag_ptr->anc_href)
		return;

	url = mo_url_canonicalize(acst->eptr->anchor_tag_ptr->anc_href, 
			win->current_node->base_url);

	if(XtOwnSelection(win->scrolled_win, XA_PRIMARY, 
	    cbs->event->xbutton.time, convert_selection,
	    NULL, NULL) == False) {
		fprintf(stderr, "Mosaic: Error: Could not copy selection, try again.\n");
		if (url) {
			free(url);
		}
	}  else {
		int i;

		for(i=0;popup_items[i].classw!=LastItem; i++) {
			if(!strcmp(popup_items[i].label, COPY_URL_LABEL) && url) {
				char *copy_str = (char*) malloc((strlen(url) +
					strlen("URL:   has been copied  ")) *
					 sizeof(char));

				if(popup_items[i].acst.str)
					free(popup_items[i].acst.str);
				popup_items[i].acst.str = url;
				sprintf(copy_str, "URL: %s  has been copied",url);
				mo_gui_notify_progress(copy_str,win);
				break;
			} else if (!strcmp(popup_items[i].label, COPY_URL_LABEL)
				   && !url) { 
				if(popup_items[i].acst.str)
					free(popup_items[i].acst.str);
				popup_items[i].acst.str=NULL;
				break;
			}
		}
	}
}

static Boolean convert_selection(Widget w, Atom *sel, Atom *tar, Atom *typ_ret,
				 XtPointer *val_ret, unsigned long *val_len,
				 int *format)
{
	char *url;
	int i;
 
	if(*tar == XA_STRING) {
#ifdef DEBUG_GUI
		if (mMosaicSrcTrace) { 
			fprintf (stderr, "Pasting text selection.\n");
		}
#endif
		for(i=0;popup_items[i].classw!=LastItem;i++) {
			if(!strcmp(popup_items[i].label, COPY_URL_LABEL)) {
				if(popup_items[i].acst.str)
					url = (char *) popup_items[i].acst.str;
				else
					return False;
				break;
			}
		}
		*val_ret = strdup(url);
		*val_len = strlen(url);
		*typ_ret = XA_STRING;
		*format = 8;
		return(True);
	}
	return(False); 
}

void mo_delete_rbm_history_win(mo_window *win)
{
	int i;

	if(win->num_session_items == 0)
		return;
	for(i = 0; i < win->num_session_items; i++)
		XtDestroyWidget(win->session_items[i]);
	if(win->session_menu)
		XtDestroyWidget(win->session_menu);
}

 
static void session_cb(Widget w, XtPointer clid, XtPointer calld)
{                                     
	char *xurl = (char *) clid;  
	mo_window * win;
	RequestDataStruct rds;

	XtVaGetValues(w, XmNuserData, (XtPointer) &win, NULL);

	rds.req_url = xurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
	rds.post_data = NULL;
	rds.ct = NULL;
	rds.is_reloading = False;
        MMPafLoadHTMLDocInWin(win, &rds);
}                                     

void mo_add_to_rbm_history(mo_window *win, char *url, char *title)
{ 
	char label[32];
	int max = mMosaicAppData.numberOfItemsInRBMHistory;
	int i;

	if(!win->session_menu)
		win->session_menu = XmCreatePulldownMenu(win->view, 
				"session_menu", NULL, 0);

	compact_string(title, label, 31, 3, 3);

	if(win->num_session_items < max) {
		win->session_items[win->num_session_items] =
			XtVaCreateManagedWidget(label, xmPushButtonGadgetClass,
				win->session_menu,
				XmNuserData, (XtPointer) win,
				NULL);
		XtAddCallback(win->session_items[win->num_session_items],
			XmNactivateCallback, session_cb, url);
		XtAddCallback(win->session_items[win->num_session_items],
			XmNarmCallback, rbm_ballonify, url);
		XtAddCallback(win->session_items[win->num_session_items],
			XmNdisarmCallback, rbm_ballonify, " ");
		win->num_session_items++;
	} else if (win && win->session_items) {
		XtDestroyWidget(win->session_items[0]);
/* scoot the widget pointers */
		for(i=0;i<max-1;i++)
			win->session_items[i] = win->session_items[i+1];
		win->session_items[max-1] =
			XtVaCreateManagedWidget(label, xmPushButtonGadgetClass,
				win->session_menu,
				XmNuserData, (XtPointer) win,
				NULL);
		XtAddCallback(win->session_items[max-1],
		XmNactivateCallback, session_cb, url);
		XtAddCallback(win->session_items[max-1],
			XmNarmCallback, rbm_ballonify, url);
		XtAddCallback(win->session_items[max-1],
			XmNdisarmCallback, rbm_ballonify, " ");
	}
}

void rbm_ballonify(Widget w, XtPointer clid, XtPointer calld)
{
	char *url = (char *) clid;
	mo_window * win;

	XtVaGetValues(w, XmNuserData, (XtPointer) &win, NULL);
	mo_gui_notify_progress(url,win);
}
