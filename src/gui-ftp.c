/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "gui-ftp.h"
#include "gui-popup.h"
#include "gui-dialogs.h"
#include "libnut/system.h"
#include "../libwww2/HTFTP.h"

#define MAX_BUF_LEN 512  /* Length of all of the buffers used for dialog message output */

static void mo_handle_ftpremove (mo_window *win, char *urlNsite);


/* gui-ftp handles all of the gui bits of the FTP send, remove, and mkdir 
   functionality.  All of the actual transfer stuff is in libwww2/HTFTP.c   
*/

/*---------------------  mo_handle_ftpput ---------------------------------*/
mo_status mo_handle_ftpput(mo_window *win)
{
char tbuf[MAX_BUF_LEN+1];

if (!win->current_node)
return mo_fail;

/* Check to see if the url is somethin' like ftp://somewarez.31337.com */
if((strlen(win->current_node->url)>4) && strncmp("ftp:", win->current_node->url, 4)==0) {
win->ftp_site = strdup(win->current_node->url);
mo_post_ftpput_window(win);
} else {
sprintf(tbuf, "FTP Send requires you to be on a page with an FTP url."); 
application_user_info_wait(tbuf);
return mo_fail;
}
return mo_succeed;  
} 

/* ---------------------- mo_post_ftpput_window ----------------------- */
static XmxCallback (ftpput_win_cb)
{
	char *fname = NULL, efname[MO_LINE_LENGTH];
	char tbuf[MAX_BUF_LEN+1];
	int i, count, ret;
	Widget fsbList;
	XmString st;
	XmStringTable selected_items;
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->ftpput_win);  /* Down with the box */
	fsbList = XmFileSelectionBoxGetChild (win->ftpput_win, XmDIALOG_LIST);
	XtVaGetValues (fsbList,
		XmNselectedItems, &selected_items,
		XmNselectedItemCount, &count,
		NULL);

	if (count) {
		for (i=0; i<count; i++) {
			XmStringGetLtoR (selected_items[i], 
				XmSTRING_DEFAULT_CHARSET, &fname);
			pathEval (efname, fname);
			XtFree (fname);
/* Make the url something HTFTPSend will understand */
			sprintf (tbuf, "%s&%s", win->ftp_site, efname);
			if ((ret = HTFTPSend (tbuf)) != 0) {   /* !HT_LOADED */
				if (ret != -2) { 
/* If the user interrupted us, forget about telling them */ 
					sprintf(tbuf, "FTP Send Failed!  The file %s could not be sent.", efname);
					application_user_info_wait(tbuf);
				}
				break;
			} else {
				mo_reload_window_text (win,0);
			}
		}
	} else {
/* Get the filename out of the filespec box in case they typed something in */
		XtVaGetValues (win->ftpput_win,
			XmNdirSpec, &st,
			NULL);
		XmStringGetLtoR (st, XmSTRING_DEFAULT_CHARSET, &fname);
		pathEval (efname, fname);        
		XtFree (fname);                  
		sprintf (tbuf, "%s&%s", win->ftp_site, efname);
		if ((ret = HTFTPSend (tbuf)) != 0) {   /* !HT_LOADED */
			if (ret != -2) { 
/* If the user interrupted us, forget about telling them */
				sprintf(tbuf, "FTP Send Failed!  The file %s could not be sent.", efname);
				application_user_info_wait(tbuf);
			}
		} else {
			mo_reload_window_text (win,0);
		}
	}
/* Clear out the selections, we have to do this because the XmFSB has no clue 
 * it is being used in extended selection mode. */
	XmListDeselectAllItems(fsbList);
	free(win->ftp_site);
	win->ftp_site = NULL;
	return;
}

mo_status mo_post_ftpput_window (mo_window *win)
{
	char tbuf[MAX_BUF_LEN+1];
	Widget fsbList;
 	
	if( win->ftp_site == NULL )
		return mo_fail;

	sprintf( tbuf, "NCSA Mosaic: Send file to %s", win->ftp_site);
	if (!win->ftpput_win) {
		win->ftpput_win = XmxMakeFileSBDialog(win->base, tbuf,
			"Name of local file to send:", ftpput_win_cb,(XtPointer) win);
/* Change the selection mode */
		fsbList = XmFileSelectionBoxGetChild (win->ftpput_win,
			XmDIALOG_LIST); 
		XtVaSetValues (fsbList,
			XmNselectionPolicy, XmEXTENDED_SELECT,
			NULL);
	} else {
		XmFileSelectionDoSearch (win->ftpput_win, NULL);
	}
	XmxManageRemanage (win->ftpput_win);
	return mo_succeed;
}


/*---------------------  mo_handle_ftpmkdir ---------------------------------*/
mo_status mo_handle_ftpmkdir(mo_window *win)
{
	char tbuf[MAX_BUF_LEN+1];

	if (!win->current_node) 
		return mo_fail;

/* Check to see if the url is somethin' like ftp://somewarez.31337.com */
	if((strlen(win->current_node->url)>4) && strncmp("ftp:", win->current_node->url, 4)==0) {
		win->ftp_site = strdup(win->current_node->url);
		mo_post_ftpmkdir_window(win);
	} else {
		sprintf(tbuf, "FTP MkDir requires you to be on a page with an FTP url."); 
		application_user_info_wait(tbuf);
		return mo_fail;
	}
	return mo_succeed;  
} 

/*-------------------  mo_post_ftpmkdir_window ------------------------------*/
static XmxCallback (ftpmkdir_win_cb0) 		/* Create dir */
{
	mo_window *win = (mo_window*) client_data;
	char *dirpath, tbuf[MAX_BUF_LEN+1];
	int ret;

	XtUnmanageChild (win->ftpmkdir_win);
	dirpath = XmxTextGetString (win->ftpmkdir_text);
	if (!dirpath || !(*dirpath)) 		/* nothing here so do nothing */
		return;
	sprintf (tbuf, "%s&%s", win->ftp_site, dirpath);
	if( (ret = HTFTPMkDir (tbuf)) != 0) {
		if (ret != -2) { /* If the user interrupted us, 
				  * forget about telling them */
			sprintf(tbuf, 
				"FTP MkDir Failed! The directory %s could not be created.", dirpath);
			application_user_info_wait(tbuf);
		}
	} else {
		mo_reload_window_text (win, 0);
	}
	free(win->ftp_site);
	win->ftp_site = NULL;
}
static XmxCallback (ftpmkdir_win_cb1)		/* Dismiss */
{
	mo_window *win = (mo_window*) client_data;

	XtUnmanageChild (win->ftpmkdir_win);
}
static XmxCallback (ftpmkdir_win_cb2)	 	/* Help */
{
	mo_window *win = (mo_window*) client_data;

	/* mo_open_another_window(win, 
		mo_assemble_help_url("docview-menubar-file.html"), NULL, NULL);*/
}
static XmxCallback (ftpmkdir_win_cb3) 		/* Clear */
{
	mo_window *win = (mo_window*) client_data;

	XmxTextSetString (win->ftpmkdir_text, "");
}

mo_status mo_post_ftpmkdir_window (mo_window *win)
{
	if (!win->ftpmkdir_win) {
		Widget dialog_frame;
		Widget dialog_sep, buttons_form;
		Widget form, label;
      
/* Create it for the first time. */
		win->ftpmkdir_win = XmxMakeFormDialog (win->base, 
			"Mosaic: FTP MkDir");
		dialog_frame = XmxMakeFrame (win->ftpmkdir_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
/* Main form. */
		form = XmxMakeForm (dialog_frame);
		label = XmxMakeLabel (form, "Directory to Create: ");
		XmxSetArg (XmNwidth, 310);
		win->ftpmkdir_text = XmxMakeTextField (form);
		XmxAddCallbackToText (win->ftpmkdir_text, ftpmkdir_win_cb0,(XtPointer) win);
		dialog_sep = XmxMakeHorizontalSeparator (form);
		buttons_form = XmxMakeFormAndFourButtons(form, 
			"Create", "Clear", "Dismiss", "Help...", 
			ftpmkdir_win_cb0, ftpmkdir_win_cb3, ftpmkdir_win_cb1, 
			ftpmkdir_win_cb2,(XtPointer)win);

/* Constraints for form. */
		XmxSetOffsets (label, 14, 0, 10, 0);
		XmxSetConstraints (label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetOffsets (win->ftpmkdir_text, 10, 0, 5, 10);
		XmxSetConstraints(win->ftpmkdir_text, 
			XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
			XmATTACH_FORM, NULL, NULL, label, NULL);
		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints(dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET,
			XmATTACH_FORM, XmATTACH_FORM,
			win->ftpmkdir_text, buttons_form, NULL, NULL);
		XmxSetConstraints (buttons_form, XmATTACH_NONE, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->ftpmkdir_win);
	return mo_succeed;
}

/* ---------------------- mo_handle_ftpremove ----------------------- */
static void mo_handle_ftpremove (mo_window *win, char *urlNsite) 
{
	int ret;
	char tbuf[MAX_BUF_LEN+1];

	if ((ret = HTFTPRemove (urlNsite)) != 0) {
		if (ret != -2) { 
/* If the user interrupted us, forget about telling them */
			sprintf(tbuf, "FTP Remove Failed!  The file could not be removed.");
			application_user_info_wait(tbuf);
		}
	} else {
		mo_reload_window_text (win, 0);
	}
}

/* Ftp callback for the right mouse button menu */
void ftp_rmbm_cb(Widget w, XtPointer client_data, XtPointer call_data)
{
	struct act_struct *acst = (struct act_struct *) client_data;
	int which;
	char *xurl, tbuf[MAX_BUF_LEN+1];
	struct ele_rec *eptr;
 
	which = (int) acst->act_code;
	eptr = acst->eptr;
  
	switch(which) {
/* ###################################
	case mo_ftp_put:
		mo_handle_ftpput (current_win);
		break;
	case mo_ftp_mkdir:
		mo_handle_ftpmkdir (current_win);
		break;
*/
	case mo_ftp_remove:
		xurl = strrchr (eptr->anchor_tag_ptr->anchor_href, '/');
		sprintf (tbuf, "%s%s", current_win->current_node->url, xurl);
		mo_handle_ftpremove (current_win, tbuf);
		break;
	}
}
