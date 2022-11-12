/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "../libhtmlw/HTMLP.h"
#include "mosaic.h"  
#include "gui-ftp.h"
#include "mo-www.h"
#include "gui-popup.h"
#include <X11/Xmu/StdSel.h>
#include "../libnut/system.h"
#include "../libhtmlw/HTMLPutil.h"
#include "hotlist.h"
#include "gui-documents.h"
#include "gui-dialogs.h"
#include "gui-menubar.h"

extern int 	do_meta;

Boolean 	have_popup;
Widget 		popup = NULL;
void 		_set_eptr_field(PopupItem *items, struct ele_rec *eptr);
void 		fsb(char *src);
char 		**user_defs_get_entries(FILE *fp, int *num);
PopupItem 	*build_user_defs_items(char **entries, int num);
void 	rbm_ballonify(Widget w, XtPointer client_data, XtPointer call_data);
void 	session_cb(Widget w, XtPointer client_data, XtPointer call_data);

void hotmenu_cb ( Widget w , XtPointer clid, XtPointer calld)
{
/* ###############*/
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

PopupItem pan_menu[] = {
  {PushButton, 0, 0, 0, 0, "Right", {0, NULL, NULL}, NULL, 0,
   NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Up", {0, NULL, NULL}, NULL,
   0, NULL, NULL, NULL, NULL, 1}, 
 {PushButton, 0,  0, 0, 0, "Left", {0, NULL, NULL}, NULL, 0,
   NULL, NULL, NULL, NULL, 1},
 {PushButton, 0, 0, 0, 0, "Down", {0, NULL, NULL},  NULL,
   0, NULL, NULL, NULL, NULL, 1},
  { LastItem },
};

PopupItem photo_cd_sub_menu[] = {
  {PushButton, 0, 0, 0,  0,"Zoom In",  {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0,  0,"Zoom Out", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1}, 
  {PushButton, 0, 0, 0, 0, "Zoom Crop", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Crop", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1}, 
  {Separator, 0, 0, 0, 0, "Sep", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton,  0, 0,0, 0, "Enlarge", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Reduce", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1}, 
  {Separator, 0, 0, 0, 0, "Sep", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton,  0, 0,0, 0, "Undo",  {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1},
  {Separator, 0, 0, 0, 0, "Sep",  {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton,  0, 0, 0, 0, "Rotate Clockwise", {0, NULL, NULL} , NULL, 0, NULL, NULL, NULL, 
   NULL, 1},
  {PushButton, 0, 0, 0, 0, "Rotate Counter-Clockwise",  {0, NULL, NULL}, NULL,
   0, NULL, NULL, NULL, NULL, 1}, 
  {CascadeButton,  0, 0,0, 0, "Pan",  {0, NULL, NULL}, NULL, 0, NULL, NULL, pan_menu, NULL, 1}, 
  {Separator,  0, 0,0, 0, "Sep", {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1},
  {PushButton, 0, 0, 0, 0, "Full Image", {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1}, 
  {PushButton,  0, 0,0,  0,"Reset To Original", {0, NULL, NULL}, NULL, 0, NULL, NULL, 
   NULL, NULL, 1}, 
  {Separator,  0, 0,0, 0, "Separator", {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL},
  {PushButton,  0, 0,0,  0,"Print This Image", {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, 
   NULL, 1},
  {ToggleButton,  0, 0,0, 0, "Display PhotoCD Icon", {0, NULL, NULL}, NULL, 0, NULL, NULL, 
   NULL, NULL, 1}, 
  {PushButton,  0, 0, 0, 0, "Options",  {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1}, 
  { LastItem },
};

PopupItem file_menu[] = {

  {PushButton, 0, 0, 0, 0, "Save Page", {mo_save_document, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Print", {mo_print_document, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Mail To", {mo_mail_document, NULL, NULL},
   hotmenu_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, 0, 0, 0, 0, "Get File Metadata", {(XtCallbackProc)M_FileData, NULL, NULL},
   metadata_cb, 0, NULL, NULL, NULL, NULL, 1},
  { LastItem },
};

PopupItem popup_items[] = {		 /* Permanent stuff */

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE,"Back", 
   {mo_back, NULL, NULL},
   hotmenu_cb, 0, "B", NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Forward", 
   {mo_forward, NULL, NULL},
   hotmenu_cb, 0, "F", NULL, NULL, NULL, 1}, 

/* negative one means this is a hotlist */
  {CascadeButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "Hotlist", 
   { (XtCallbackProc)-1, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {CascadeButton, ALL_TYPES, LOOSE, moMODE_ALL, LOOSE, "User", 
   {0, NULL, NULL}, NULL, 0, NULL, NULL, NULL, NULL, 1}, 

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

/* Stuff if on a ftp page 
---------------------------------------------------------------*/

  {Separator, ALL_TYPES, LOOSE, moMODE_FTP, TIGHT, "Separator", 
   {0, NULL, NULL},
   NULL, 0, NULL, NULL, NULL, NULL, 1}, 

  {PushButton, ALL_TYPES, LOOSE, moMODE_FTP, TIGHT, "Put ...", 
   {mo_ftp_put, NULL, NULL},
   ftp_rmbm_cb, 0, NULL, NULL, NULL, NULL, 1},

  {PushButton, ALL_TYPES, LOOSE, moMODE_FTP, TIGHT, "Make Directory", 
   {mo_ftp_mkdir, NULL, NULL}, ftp_rmbm_cb, 0, NULL, NULL, NULL,  NULL, 1},

  {PushButton, E_ANCHOR, TIGHT, moMODE_FTP, TIGHT, "Remove", 
   {(XtCallbackProc)mo_ftp_remove, NULL, NULL}, ftp_rmbm_cb, 0, NULL, NULL, NULL,  NULL, 1},

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


  /*  {Separator, 0, 0, "Separator", 0, NULL, 0,
      NULL, NULL, NULL, NULL},

      {CascadeButton, 0, 0,  "Kodak Photo CD", 0, NULL, 0,
      NULL, NULL, photo_cd_sub_menu, NULL}, */

  { LastItem },
};

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

extern mo_window *current_win;
extern int imageViewInternal;
extern char *getFileName();

void ThirdButtonMenu(Widget w, XtPointer client_data, XEvent *event,
     Boolean *ctd)
{
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
	if(!popup) { 	/* before we build the popup see if the user 
			 * has a .mosiac/user-defs file and if so 
			 * use it to build a user menu */
		popup_items[3].sub_items = popup_build_user_defs();
			/* if we didn't have any turn it off */
		if(popup_items[3].sub_items == NULL)
			popup_items[3].types = 0;
		popup = _PopupMenuBuilder((Widget) w, XmMENU_POPUP, 
			"popup", 0, popup_items);
	}
	mode = current_win->mode;
	if(eptr) {
		type = eptr->type; 
		if((type == E_IMAGE) && (eptr->anchor_tag_ptr->anchor_href)) { 
				/* turn on anchor and off text */
			type |= E_ANCHOR;
			if(type & E_TEXT)
				type -= E_TEXT;
		}
		if((type == E_TEXT) && (eptr->anchor_tag_ptr->anchor_href))
			type = E_ANCHOR;
	} else
		type = E_HRULE; /* pick a good normal little element */
 
	for(i = 0; popup_items[i].classw != LastItem; i++) {
		if(popup_items[i]._w) {/*anything is possible in Mosaic */
			int good = True;
      
              /* take care of session menu */
              if(popup_items[i].acst.act_code ==(XtCallbackProc) -2)
                XtVaSetValues(popup_items[i]._w, XmNsubMenuId,
                              current_win->session_menu, NULL);
                                     
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
	_set_eptr_field(popup_items, eptr);

			/*motif puts menu in a boring place lets fix it*/ 
			/* BuEvent->x_root -= 40;  middle of buttons*/
	BuEvent->y_root -= del; /* first active button or first specialty
				 *item if we're over element that has em*/
	XmMenuPosition(popup, BuEvent);
	XtManageChild(popup);
}

PopupItem *popup_build_user_defs()
{
	PopupItem *items;
	char *str, *file, **entries;
	FILE *fp;
	int num, i;

	if ((num=get_home(&str))!=0) 
		return NULL;
	file = (char*)malloc((strlen(str)+strlen("/.mosaic-user-defs")+1));
	sprintf(file, "%s/.mosaic-user-defs", str);
	free(str);
	if(!file_exists(file)) {
		free(file);
		return NULL;
	}
	fp = fopen(file, "r");
	free(file);
	if(fp) {
		fseek(fp,0,SEEK_SET);
		entries = user_defs_get_entries(fp, &num);
		fclose(fp);
	} else
		return NULL;
	items = build_user_defs_items(entries, num);
	for(i=0;i<(num+2);i++)
		free(entries[i]);
	free(entries);
	if(items)
		return items;
	return NULL;
}

void _set_eptr_field(PopupItem *items, struct ele_rec *eptr)
{
	int i;

	for(i=0; items[i].classw != LastItem; i++) {
		if(items[i].sub_items)
		_set_eptr_field(items[i].sub_items, eptr);
		items[i].acst.eptr = eptr;
	}
}

void mo_make_popup(Widget view)
{
	have_popup = True; /* this will cause it to be created later */
	popup = NULL;
	XtInsertEventHandler(view, ButtonPressMask, False,
                       (XtEventHandler)ThirdButtonMenu, NULL, 
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

Widget _PopupMenuBuilder(Widget parent, int type, char *title, 
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
		mapping_del = get_pref_int(ePOPUPCASCADEMAPPINGDELAY);
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
					xmPushButtonGadgetClass, 
					menu, NULL);
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

			if(items[i].acst.str && 
			   items[i].acst.act_code==(XtCallbackProc)69) {
				XtAddCallback(items[i]._w,
					XmNarmCallback, rbm_ballonify,
					items[i].acst.str);
				XtAddCallback(items[i]._w,
					XmNdisarmCallback, rbm_ballonify, NULL);
			}                      
			break;
		case Separator:
			items[i]._w = XtVaCreateManagedWidget(items[i].label, 
					xmSeparatorGadgetClass, menu, NULL);
			break;
		case ToggleButton:
			items[i]._w = XtVaCreateManagedWidget(items[i].label, 
					xmToggleButtonGadgetClass, menu, NULL);
			break;
		case CascadeButton:
			if(items[i].sub_items && 
			   (items[i].acst.act_code !=(XtCallbackProc) -2))
				items[i]._w = _PopupMenuBuilder(menu,
					XmMENU_PULLDOWN, items[i].label, 
					items[i].mnemonic, items[i].sub_items);
			else {                      
				int mapping_del;     

				if(get_pref_boolean(eSESSION_HISTORY_ON_RBM)) {
					mapping_del = 
					  get_pref_int(ePOPUPCASCADEMAPPINGDELAY);
					items[i]._w = XtVaCreateManagedWidget(
						"Session History",
						xmCascadeButtonGadgetClass,
						menu, XmNsubMenuId,
						current_win->session_menu,
						XmNmappingDelay, mapping_del, 
						NULL);
				}
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
                                     
        if(!acst || !acst->eptr)
                return;              
                                     
        eptr = acst->eptr;           
        which = (int)acst->act_code;      
                                     
        if(!eptr)
                return; /* oh, well */
                                     
        switch (which) {             
        case M_ImageData:  
               if (!eptr->pic_data) { /* do what? */
                                return;
               }            
               xurl=mo_url_prepend_protocol(eptr->pic_data->src);
               do_meta=1;   
               mo_load_window_text(current_win, xurl, NULL);
               do_meta=0;   
               break; 
        case M_FileData:   
               do_meta=1;   
               mo_load_window_text(current_win,
                                            current_win->current_node->url,
                                            NULL);
               do_meta=0;   
               break;       
        case M_LinkData:   
               if (!eptr->anchor_tag_ptr->anchor_href || !*eptr->anchor_tag_ptr->anchor_href) {
               		return;
               }            
               xurl=mo_url_canonicalize(eptr->anchor_tag_ptr->anchor_href,
               		strdup(current_win->current_node->url));
               do_meta=2;
               mo_load_window_text(current_win, xurl, NULL);
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

	eptr = acst->eptr;
	which = (int)acst->act_code;

	if(!eptr || !eptr->pic_data->image_data || !eptr->pic_data->src){
		printf("Lost source.\n");
		return; /* oh, well */
	}

	switch(which) {
	case I_Save:
				/* FIXME: this should be 
				fsb(eptr->edata); */
		fsb(eptr->pic_data->src);
		break;
	case I_ViewExternal:
		xurl=mo_url_prepend_protocol(eptr->pic_data->src);
		tmp=imageViewInternal;
		imageViewInternal=0;
		mo_load_window_text (current_win, xurl, NULL);
		imageViewInternal=tmp;
		break;
	case I_ViewInternal:
		xurl=mo_url_prepend_protocol(eptr->pic_data->src);
		tmp=imageViewInternal;
		imageViewInternal=1;
		mo_load_window_text (current_win, xurl, NULL);
		imageViewInternal=tmp;
		break;
	case I_Reload:
		mo_reload_window_text (current_win, 1);
		break;
	}
}

void fsb(char *src)
{
	static Widget dialog;
	XmString str,fbfn;
	char *fname, fBuf[1024];
	static char *last_src=NULL;
  
	if ( !dialog ) {
		last_src=strdup(src);
		dialog = XmCreateFileSelectionDialog (current_win->view, 
				"Save Image File", 
				NULL, 0 );
		XtAddCallback(dialog,XmNcancelCallback, fsb_CancelCallback, NULL);
		XtAddCallback(dialog, XmNokCallback, fsb_OKCallback, last_src);

		XtSetSensitive(XmFileSelectionBoxGetChild(dialog, 
				XmDIALOG_HELP_BUTTON), False);
		XtVaSetValues(dialog, XmNfileTypeMask, XmFILE_REGULAR, NULL);
	} else {
		/*Dance with the callbacks so we get the correct URL later--SWP */
		XtRemoveCallback(dialog, XmNokCallback, fsb_OKCallback, last_src);
		if (last_src) 
			free(last_src);
		last_src=strdup(src);
		XtAddCallback(dialog, XmNokCallback, fsb_OKCallback, last_src);
				/* Re-Init the Stupid Box -- SWP */
		XmFileSelectionDoSearch(dialog,NULL);
	}
			/* set the save file string */
	XtVaGetValues(dialog, XmNdirSpec, &str, NULL);
	XmStringGetLtoR(str,XmSTRING_DEFAULT_CHARSET,&fname);
	XmStringFree(str);

	if (fname) {
		if(src && *src)
			sprintf(fBuf,"%s%s",fname,getFileName(src));
		else
			sprintf(fBuf,"%s",fname);
		str=XmStringCreateLtoR(fBuf,XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(dialog, XmNdirSpec, str, NULL);
		XmStringFree(str);
		free(fname);
	}
	XtManageChild ( dialog );
}

void fsb_OKCallback ( Widget w, XtPointer client_data, XtPointer call_data)
{
	XmFileSelectionBoxCallbackStruct *cbs = 
		(XmFileSelectionBoxCallbackStruct *) call_data;
	char *filename, *url = (char *) client_data, efilename[MO_LINE_LENGTH];
  
			/* Remove the widget from the screen, and kill it.  */
	XtUnmanageChild ( w );
	/* Retrieve the character string from the compound string format.  */
	XmStringGetLtoR ( cbs->value, XmSTRING_DEFAULT_CHARSET, &filename );
			/* Expand any ~ */                 
	pathEval (efilename, filename); 
		/* FIXME: the code below should just copy a file but since
		we don't keep the files around we have to beam it down again.
		This should be fixed with the disk cache */

		/* now copy src to filename */
	/*  if(!dest || !filename) {
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr, "Couldn't save file, internal error.\n");
		}
#endif
		return;
	}*/

	mo_pull_er_over_virgin(url, efilename);
		/* We need to reset the icons and let the user know -- SWP */
	mo_gui_done_with_icon(NULL);
	mo_gui_notify_progress("Image has been downloaded and saved.");
}
                
void fsb_CancelCallback ( Widget w, XtPointer clientData, XtPointer callData )
{
	XtUnmanageChild ( w );
}

void ungrab_the_____ing_pointer(XtPointer client_data)
{
	XUngrabPointer (dsp, CurrentTime);
}

mo_status mo_add_item_to_hotlist (mo_hotlist *list, mo_item_type type,
		char *title, char *url, int position,
		int rbm);


void hot_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	char *xurl;
	int i;
	mo_hot_item *hn = (mo_hot_item *) acst->str;
   
	switch((int)acst->act_code) {  
	case 1: /* add item */ 
		mo_add_item_to_hotlist ((mo_hotlist*)acst->str, mo_t_url,
			current_win->current_node->title,
			current_win->current_node->url, 0,
			get_pref_boolean(eADD_RBM_ADDS_RBM));
		break;  
	case 2: /* add list */
		if(xurl = XmxModalPromptForString (current_win->base,
		    XtWidgetToApplicationContext(current_win->base),
		    "Enter New List Name:",
		    "Okay","Dismiss")){
			mo_add_item_to_hotlist ((mo_hotlist*)acst->str,
					mo_t_list, xurl, NULL, 0,
					get_pref_boolean(eADD_RBM_ADDS_RBM));
		}       
		break;
	default: /* goto link */
		if(acst->str) {
			xurl=mo_url_prepend_protocol((char*)acst->str);
			mo_load_window_text (current_win, xurl, NULL);
		}  
		break;
	}
}

void mo_destroy_hot_menu(PopupItem *pmenu)
{
    int i;
    for(i=0;pmenu[i].classw != LastItem;i++) {
        if((pmenu[i].classw != Separator) &&
           (pmenu[i].acst.act_code != (XtCallbackProc)1) &&
	   (pmenu[i].acst.act_code != (XtCallbackProc)2)) 
            free(pmenu[i].label);
        
        if(pmenu[i].classw == CascadeButton)
            mo_destroy_hot_menu(pmenu[i].sub_items);
    }
    free(pmenu);
}
        
PopupItem *mo_assemble_hot_menu(mo_hotlist *list)
{
    mo_hot_item *item;
    char str[32];
    PopupItem *pmenu;
    int i;

        /* have to count it first. sigh */
    for(i = 0, item = list->nodelist; item ; item = item->any.next){
        if ((item->type==mo_t_url && item->hot.rbm) ||
            (item->type==mo_t_list && item->list.rbm)) {
                i++;                 
        }                            
    }  

/*###    pmenu = (PopupItem *) malloc(sizeof(PopupItem) * (i+3)); */
    pmenu = (PopupItem *) malloc(sizeof(PopupItem) * (i+5));
    for(i = 0, item = list->nodelist; item ; item = item->any.next) {
      
        if ((item->type==mo_t_url && !item->hot.rbm) ||
            (item->type==mo_t_list && !item->list.rbm)) {
                continue;            
        } 

        pmenu[i].types = 0;
        pmenu[i].modes = 0;
        compact_string(item->hot.title, str, 31, 3, 3);
        pmenu[i].label = strdup(str);
	if(item->type == mo_t_url){
	  pmenu[i].acst.str = item->hot.url;
          pmenu[i].acst.act_code = (XtCallbackProc)69; /* identifies this as a hotlist
                            #######     button so we can ballon it */
        }
        pmenu[i].cbfp = hot_cb;
        pmenu[i].mnemonic = 0;
        pmenu[i].accel_text = NULL;
        pmenu[i].accel = NULL;
        pmenu[i]._w = NULL;
	pmenu[i].startup=1;
        if (item->type == mo_t_url) {	/* URL item */
            pmenu[i].classw = PushButton;
            pmenu[i].sub_items = NULL;
        } else {
            pmenu[i].classw = CascadeButton;
            pmenu[i].sub_items = mo_assemble_hot_menu(&(item->list));
            pmenu[i].acst.act_code = 0;
        }
        i++;
    }
    pmenu[i].classw = Separator;
    pmenu[i].sub_items = NULL;
    pmenu[i].label = strdup("Sep");
    i++;
    
    pmenu[i].classw = PushButton;
    pmenu[i].label = "Add current URL...";
    pmenu[i].types = 0;
    pmenu[i].modes = 0;
    pmenu[i].cbfp = hot_cb;
    pmenu[i].acst.str = list;
    pmenu[i].acst.act_code = (XtCallbackProc)1;
    pmenu[i].mnemonic = 0;
    pmenu[i].accel_text = NULL;
    pmenu[i].accel = NULL;
    pmenu[i]._w = NULL;
    pmenu[i].startup=1;
    pmenu[i].sub_items = NULL;
    i++;
        
                                    
    pmenu[i].classw = PushButton;     
    pmenu[i].label = "Add New List...";
    pmenu[i].types = 0;              
    pmenu[i].modes = 0;              
    pmenu[i].cbfp = hot_cb;          
    pmenu[i].acst.str = list;        
    pmenu[i].acst.act_code =(XtCallbackProc) 2;      /*##############*/
    pmenu[i].mnemonic = 0;           
    pmenu[i].accel_text = NULL;      
    pmenu[i].accel = NULL;           
    pmenu[i]._w = NULL;              
    pmenu[i].startup=1;              
    pmenu[i].sub_items = NULL;       
    i++;                             
                     
    pmenu[i].classw = LastItem;
    return pmenu;
}

static int hot_button = 0;

void mo_init_hotlist_menu(mo_hotlist *list)
{
/* this doesn't check the first button but that is okay because
 * the first two buttons are always back and forward */

	while(popup_items[hot_button].acst.act_code != (XtCallbackProc)-1) 
		hot_button++;
  
	popup_items[hot_button].sub_items = mo_assemble_hot_menu(list);
}

void mo_reinit_hotlist_menu(mo_hotlist *list)
{
    if(!popup) return;
    mo_destroy_hot_menu(popup_items[hot_button].sub_items);
    popup_items[hot_button].sub_items = mo_assemble_hot_menu(list);
    XtDestroyWidget(popup);
    popup = _PopupMenuBuilder(current_win->view, XmMENU_POPUP, 
			      "popup", 0, popup_items);
}

char **user_defs_get_entries(FILE *fp, int *num)
{
	char **entries, str[512];
	int i=0;

	entries = (char**)malloc(sizeof(char *) * 100);
	while(fgets(str, 512, fp) != NULL) {
		int index=0;

		while(isspace(str[index]))
			index++;

		if(str[index] != '#' && str[index] != '\n' && str[index] != '\0'){
			if(i%2) { /* url spec line */
				switch(str[index]) {
				case 'G': /* GET: */
				case 'P': /* POST: */
				case 'F': /* FETCH: */
					entries[i]=strdup(&(str[index]));
					entries[i]=my_chop(entries[i]);
					i++;
					break;
				default: /* error */
					fprintf(stderr, 
				     	"User defined field wrong:%s. Ignoring it\n",
						&(str[index]));
				}
			} else { /* button name */
				if(strlen(str) > 50) {
					fprintf(stderr, 
					"User defined button name too long. Ignoring\n");
				} else {
					entries[i]=strdup(&(str[index]));
					entries[i]=my_chop(entries[i]); 
					i++;
				}
			}
		}
	}
	if(i%2 == 1) {
		fprintf(stderr, "Problem in gui_popup.c:%d \n", __LINE__);
		return NULL;
	}
	*num = i/2;
	return entries; /* they better free this */
}

PopupItem *build_user_defs_items(char **entries, int num)
{
	PopupItem *items = (PopupItem *)malloc(sizeof(PopupItem) * (num+1));
	int i;

	if(!items || !entries || num<=0)
		return NULL;
  
	for(i=0;i<num;i++) {
		items[i].classw = PushButton;
		items[i].label = strdup(entries[i*2]);
		items[i].types = 0;
		items[i].types_method = 0;
		items[i].modes = 0;
		items[i].modes_method = 0;
		items[i].cbfp = user_defs_cb;
		items[i].acst.str = strdup(entries[i*2+1]);
		items[i].acst.act_code = 0;
		items[i].acst.eptr = NULL;
		items[i].mnemonic = 0;
		items[i].accel_text = NULL;
		items[i].accel = NULL;
		items[i]._w = NULL;
		items[i].startup=1;
		items[i].sub_items = NULL;
	}
	items[num].classw = LastItem;
	items[num].sub_items = NULL;
	return items;
}

void select_cb(Widget w, XtPointer client_data, Atom *sel, Atom *type, 
	       XtPointer value, unsigned long *length, int *format)
{
  char *pt, *end = NULL, *bptr, 
    *select = (char *) value, 
    *str = (char *) client_data,
    *nselect, *begin;
  char mode;

  begin = strdup(str); /* we don't want to fuck with str */
  bptr=begin;

  /* this filters out empty strings and most possible errors */
  if(*type != XA_STRING || !select || !*select || !begin || !*begin ||
     *length <= 0)
    return;

  /* do this cause select is not null terminated sometimes */
  pt = my_strndup(select, *length);
  
  if(pt) {
      select = pt;
      pt=NULL;
    } else {
      XtFree((char*)value);
      free(begin);
      return;
    }

  switch(bptr[0]) {
    case 'G': /* GET: */
      bptr+=4;
      mode='G';
      bptr=my_chop(bptr);
      break;
    case 'P': /* POST: */
      bptr+=5;
      mode='P';
      bptr=my_chop(bptr);
      break;
    case 'F': /* FETCH: */
      bptr+=6;
      mode='F';
      bptr=my_chop(bptr);
      break;
    default: /* error */
      fprintf(stderr, "User defined field wrong:%s. Ignoring it\n",
	      begin);
      XtFree((char*)value);
      free(begin);
      return;
    }

  if(mode == 'F') {
      /* expand url */
      nselect = mo_url_prepend_protocol(select);
      XtFree((char*)value); /* this frees select */
    } else {
      /* make search string sendable */
      nselect = mo_escape_part(select);
      XtFree((char*)value); /* this frees select */
    }

  pt = strchr(bptr, '_');
  while(!end && pt && *pt) {
      if(!strncmp(pt, "__string__", 10))
	end = pt+10;
      else {
	  pt++;
	  if(pt && *pt)
	    pt = strchr(pt, '_');
	}
    } 

  if(pt && *pt && end && nselect) {
      if(srcTrace)
	fprintf(stderr, "Popup getting %s from user menu.\n", pt);
      
      if(mode=='P') {
	  char *ptr;
	  
	  ptr = strrchr(bptr, ' '); /* This shouldn't fail because bptr is 
				       chopped */
	  ptr[0]='\0'; /* make bptr not have name value pair */
	  ptr++; /* get back to a real string */
	  bptr=my_chop(bptr);
	  ptr=my_chop(ptr);

	  pt[0] = '\0'; /* make __string__ go away from ptr */
	  pt= (char*)malloc(sizeof(char) * (strlen(end)+strlen(nselect)+strlen(ptr)+1));
	  sprintf(pt,"%s%s%s", ptr, nselect, end);

	  mo_post_access_document (current_win, bptr, 
				   "application/x-www-form-urlencoded",
				   pt);
	  free(pt);
	} else if(mode=='G') {
	  pt[0] = '\0';
	  pt= (char*)malloc(sizeof(char) * (strlen(end)+strlen(bptr)+strlen(nselect)+1));
	  sprintf(pt,"%s%s%s", bptr, nselect, end);
	  mo_load_window_text(current_win, pt, NULL);      
	  free(pt);
	} else if(mode=='F') {
	  mo_load_window_text(current_win, nselect, NULL);
	}
    }
  free(begin);
  free(nselect);
}

void user_defs_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data; /* acst->str is the url */
	XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct *)call_data;
	char *str = (char*)acst->str;
  
	if(!str)
		return;
	XtGetSelectionValue(current_win->scrolled_win, XA_PRIMARY, XA_STRING, 
		select_cb, str, cbs->event->xbutton.time);
}

void copy_link_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	act_struct *acst = (act_struct *) client_data;
	XmPushButtonCallbackStruct *cbs = (XmPushButtonCallbackStruct *)call_data;
	char *url;

	if(!acst || !acst->eptr || !acst->eptr->anchor_tag_ptr->anchor_href ||
	   !*acst->eptr->anchor_tag_ptr->anchor_href)
		return;

	url = mo_url_canonicalize(acst->eptr->anchor_tag_ptr->anchor_href, 
			strdup(current_win->current_node->url));

	if(XtOwnSelection(current_win->scrolled_win, XA_PRIMARY, 
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
				mo_gui_notify_progress(copy_str);
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
#ifndef DISABLE_TRACE
		if (srcTrace) { 
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

	if(!get_pref_boolean(eSESSION_HISTORY_ON_RBM))
                return;
	if(win->num_session_items == 0)
		return;
	for(i = 0; i < win->num_session_items; i++)
		XtDestroyWidget(win->session_items[i]);
	if(win->session_menu)
		XtDestroyWidget(win->session_menu);
}

void mo_add_to_rbm_history(mo_window *win, char *url, char *title)
{ 
	char label[32];
	int max = get_pref_int(eNUMBER_OF_ITEMS_IN_RBM_HISTORY);
	int i;

	if(!get_pref_boolean(eSESSION_HISTORY_ON_RBM))
		return;
	else if(!win->session_menu)
		win->session_menu = XmCreatePulldownMenu(win->view, 
				"session_menu", NULL, 0);

	compact_string(title, label, 31, 3, 3);

	if(win->num_session_items < max) {
		win->session_items[win->num_session_items] =
			XtVaCreateManagedWidget(label, xmPushButtonGadgetClass,
					win->session_menu,NULL);
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
				win->session_menu, NULL);
		XtAddCallback(win->session_items[max-1],
		XmNactivateCallback, session_cb, url);
		XtAddCallback(win->session_items[max-1],
			XmNarmCallback, rbm_ballonify, url);
		XtAddCallback(win->session_items[max-1],
			XmNdisarmCallback, rbm_ballonify, " ");
	}
}
 
void session_cb(Widget w, XtPointer client_data, XtPointer call_data)
{                                     
	char *xurl = (char *) client_data;  

	mo_load_window_text (current_win, xurl, NULL);
}                                     

void rbm_ballonify(Widget w, XtPointer client_data, XtPointer call_data)
{                                     
	char *url = (char *) client_data;   

	mo_gui_notify_progress(url);        
}  
