/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <Xm/Xm.h>
#include <Xm/ScrolledW.h>
#include <Xm/List.h>
#include <Xm/Label.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "gui-documents.h"
#include "gui-dialogs.h"
#include "URLParse.h"
#include "mime.h"
#include "paf.h"

#include "libnut/system.h"

static XmxCallback (links_win_cb0)		 /* GOTO */
{
	mo_window *win = (mo_window*)client_data;
	int *posns, pcount;
	char *text;
	RequestDataStruct rds;
  
	if(XmListGetSelectedPos(win->links_list, &posns, &pcount)){
		if(pcount && XmStringGetLtoR(win->links_items[posns[0]-1],
		   XmSTRING_DEFAULT_CHARSET, &text)){
			if(strncmp(text,"===",3)){
				rds.req_url = text;
				rds.post_data = NULL;
				rds.ct = NULL;
				rds.is_reloading = True;
				MMPafLoadHTMLDocInWin(win, &rds);
			}
			XtFree(text);
		}
		XtFree((char *)posns);
	}
	mo_gui_done_with_icon(win);
}

static XmxCallback (links_win_cb1) 		/* DISMISS */
{
	mo_window *win = (mo_window*)client_data;
  
	XtUnmanageChild (win->links_win);
}

static XmxCallback (links_win_cb2) 		/* HELP */
{
	mo_window *win = (mo_window*)client_data;
  
	mo_open_another_window(win, mo_assemble_help_url("help-on-links.html"));
}
static XmxCallback (links_win_cb3)	 /* SAVE TO FILE */
{
	mo_window *win = (mo_window*)client_data;
	int *posns, pcount;
	char *text,*fnam,*url;
  
	if(XmListGetSelectedPos(win->links_list, &posns, &pcount)){
		if(pcount && XmStringGetLtoR(win->links_items[posns[0]-1],
		   XmSTRING_DEFAULT_CHARSET, &text)){
			if(strncmp(text,"===",3)){
				url = mo_url_canonicalize(
						text,win->current_node->base_url);
				fnam = tempnam (mMosaicTmpDir,"mMo");
				MMPafSaveData(mMosaicToplevelWidget,url,fnam);
				free(url);
				free(fnam);
			}
			XtFree(text);
		}
		XtFree((char *)posns);
	}
}

static void links_list_cb(Widget w, XtPointer client, XtPointer call)
{
	mo_window *win = (mo_window *) client;
	char *text;
	XmListCallbackStruct *cs = (XmListCallbackStruct *) call;
	RequestDataStruct rds;
  
	if(XmStringGetLtoR(win->links_items[cs->item_position-1],
	   XmSTRING_DEFAULT_CHARSET, &text)){
		if(strncmp(text,"===",3)){
			rds.req_url = text;
			rds.post_data = NULL;
			rds.ct = NULL;
			rds.is_reloading = True;
			MMPafLoadHTMLDocInWin(win, &rds);
		}
		XtFree(text);
	}
/* Don't unmanage the list. */
}

mo_status mo_post_links_window(mo_window *win)
{
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget links_form, list, scroller, label;

	if (!win->links_win) { /* Create it for the first time. */
		Xmx_n = 0;
		win->links_win = XmxMakeFormDialog(win->base, 
			"Mosaic: Document Links" );
		dialog_frame = XmxMakeFrame (win->links_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints(dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
/* Main form. */
		links_form = XmxMakeForm (dialog_frame);
      
		dialog_sep = XmxMakeHorizontalSeparator (links_form);
		buttons_form = XmxMakeFormAndFourButtons(links_form, 
			"Goto URL" , "Save" , "Dismiss" , "Help..." , 
			links_win_cb0, links_win_cb3, links_win_cb1, links_win_cb2,
			(XtPointer)win);

		label = XtVaCreateManagedWidget("Document Links & Images ..." ,
			xmLabelWidgetClass, links_form,
			XmNwidth, 500,
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_FORM,
			XmNtopOffset, 2,
			NULL);
      
		scroller = XtVaCreateWidget("scroller",
			xmScrolledWindowWidgetClass, links_form,
			XmNheight, 100,
/* form attachments */
			XmNleftAttachment, XmATTACH_FORM,
			XmNrightAttachment, XmATTACH_FORM,
			XmNtopAttachment, XmATTACH_WIDGET,
			XmNtopWidget, label,
			XmNbottomAttachment, XmATTACH_WIDGET,
			XmNbottomWidget, dialog_sep,
/* offsets */
			XmNtopOffset, 10,
			XmNbottomOffset, 10,
			XmNleftOffset, 8,
			XmNrightOffset, 8,
			NULL);
      
		list = XtVaCreateManagedWidget("list", xmListWidgetClass, 
			scroller,
			XmNvisibleItemCount, 10,
			XmNresizable, False,
			XmNscrollBarDisplayPolicy, XmSTATIC,
			XmNlistSizePolicy, XmCONSTANT,
			NULL);

		XtAddCallback(list, XmNdefaultActionCallback, links_list_cb, (XtPointer) win);
      
		win->links_list = list;
		win->links_items = NULL;
		win->links_count = 0;
      
		XtManageChild(scroller);

		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints (dialog_sep, XmATTACH_NONE, XmATTACH_WIDGET, 
			XmATTACH_FORM, XmATTACH_FORM,
			NULL, buttons_form, NULL, NULL);

		XmxSetConstraints (buttons_form, XmATTACH_NONE, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM,
			NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->links_win);
	mo_update_links_window(win);
	return mo_succeed;
}

mo_status mo_update_links_window(mo_window *win)
{
	char **hrefs,**imgs;
	int i,p,count,hcount,icount;
	XmString *xmstr;

	hrefs = HTMLGetHRefs(win->scrolled_win,&hcount);
	imgs = HTMLGetImageSrcs(win->scrolled_win,&icount);

	count = icount + hcount;
	if(!count){
		XtVaSetValues(win->links_list,
			XmNitemCount, 0,
			NULL);
	} else {
		if(hrefs) count++;
		if(imgs) count++;
		xmstr = (XmString *) XtMalloc(sizeof(XmString)*count);
		p=0;
		if(hrefs){
			xmstr[p++] = XmStringCreateLtoR("=== Links ===" ,
					XmSTRING_DEFAULT_CHARSET);
			for(i=0;i<hcount;i++,p++){
				xmstr[p] = XmStringCreateLtoR(hrefs[i],
						XmSTRING_DEFAULT_CHARSET);
				free(hrefs[i]);
			}
			free(hrefs);
		}
		if(imgs){
			xmstr[p++] = XmStringCreateLtoR("=== Images ===" ,
						XmSTRING_DEFAULT_CHARSET);
			for(i=0;i<icount;i++,p++){
				xmstr[p] = XmStringCreateLtoR(imgs[i],
						XmSTRING_DEFAULT_CHARSET);
				free(imgs[i]);
			}
			free(imgs);
		}
		XtVaSetValues(win->links_list,
			XmNitems, xmstr,
			XmNitemCount, count,
			NULL);
	}
	if(win->links_count) {
		XtFree((char *)(win->links_items));    
	}
	win->links_count = count;
	win->links_items = xmstr;
	return mo_succeed;
}

void System(char *cmd, char *title) 
{
	char buf[BUFSIZ], final[BUFSIZ*2];
	int retValue,skip_output=0;

	*final='\0';

	if ((retValue=my_system(cmd,buf,BUFSIZ))!=SYS_SUCCESS) {
		/*give them the error code message*/
		switch(retValue) {
			case SYS_NO_COMMAND:
				sprintf(final,"%s%s",final,"There was no command to execute.\n" );
				break;
			case SYS_FORK_FAIL:
				sprintf(final,"%s%s",final,"The fork call failed.\n" );
				break;
			case SYS_PROGRAM_FAILED:
				sprintf(final,"%s%s",final,"The program specified was not able to exec.\n" );
				break;
			case SYS_NO_RETBUF:
				sprintf(final,"%s%s",final,"There was no return buffer.\n" );
				break;
			case SYS_FCNTL_FAILED:
				sprintf(final,"%s%s",final,"Fcntl failed to set non-block on the pipe.\n" );
				break;
		}
		/*give them the output*/
		if (*buf) {
			sprintf(final,"%s%s",final,buf);
		}
	} else if (*buf) {
		/*give them the output*/
		sprintf(final,"%s%s",final,buf);
		XmxMakeErrorDialogWait(mMosaicToplevelWidget,mMosaicAppContext,final,title);
		return;
	}
	return;
}
