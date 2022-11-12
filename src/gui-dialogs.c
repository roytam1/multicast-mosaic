/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui-dialogs.h" 
#include "gui-documents.h"
#include "gui.h"
#include "mo-www.h"

#include <Xm/XmAll.h>

#include <pwd.h>
#include "libnut/system.h"

#include "child.h"

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

/*swp -- header*/
extern int HTML_Print_Headers;
extern int HTML_Print_Footers;
extern int HTML_Print_Paper_Size_A4;

extern int is_uncompressed;

/*swp -- for ~ expansion*/
extern int sys_nerr;
extern char *sys_errlist[];
extern int errno;
#define __MAX_HOME_LEN__ 256
int pathEval(char *dest, char *src);
char *getFileName(char *file_src);

extern char *saveFileName;
extern Widget tolevel;
extern mo_window *current_win;
extern int cci_docommand;
void print_sensitive(mo_window *win, int format) ;

XmxOptionMenuStruct *format_opts;

static void (save_print_header_cb)(Widget w, XtPointer clid, XtPointer calld)
{
}

static void (mail_print_header_cb)(Widget w, XtPointer clid, XtPointer calld)
{
}

static void print_print_header_cb(Widget w, XtPointer clid, XtPointer calld)
{
}

static void save_print_footer_cb(Widget w, XtPointer clid, XtPointer calld)
{
}

static void mail_print_footer_cb(Widget w, XtPointer clid, XtPointer calld)
{
}

static void print_print_footer_cb(Widget w, XtPointer clid, XtPointer calld)
{
}

static XmxCallback (save_print_a4_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_save,
			   !XmToggleButtonGetState(win->print_a4_toggle_save));
	XmxSetToggleButton(win->print_us_toggle_save,
			   !XmToggleButtonGetState(win->print_us_toggle_save));
}
 
static XmxCallback (print_url_cb)
{
    mo_window *win = (mo_window*)client_data;
 
    XmxSetToggleButton(win->print_doc_only,
                       !XmToggleButtonGetState(win->print_doc_only));
    XmxSetToggleButton(win->print_url_only,
                       !XmToggleButtonGetState(win->print_url_only));
}

static XmxCallback (mail_print_a4_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_mail,
			   !XmToggleButtonGetState(win->print_a4_toggle_mail));
	XmxSetToggleButton(win->print_us_toggle_mail,
			   !XmToggleButtonGetState(win->print_us_toggle_mail));
}

static XmxCallback (print_print_a4_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_print,
			   !XmToggleButtonGetState(win->print_a4_toggle_print));
	XmxSetToggleButton(win->print_us_toggle_print,
			   !XmToggleButtonGetState(win->print_us_toggle_print));
}

static XmxCallback (save_print_us_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_save,
			   !XmToggleButtonGetState(win->print_a4_toggle_save));
	XmxSetToggleButton(win->print_us_toggle_save,
			   !XmToggleButtonGetState(win->print_us_toggle_save));
}

static XmxCallback (mail_print_us_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_mail,
			   !XmToggleButtonGetState(win->print_a4_toggle_mail));
	XmxSetToggleButton(win->print_us_toggle_mail,
			   !XmToggleButtonGetState(win->print_us_toggle_mail));
}

static XmxCallback (print_print_us_cb)
{
	mo_window *win = (mo_window*)client_data;

	XmxSetToggleButton(win->print_a4_toggle_print,
			   !XmToggleButtonGetState(win->print_a4_toggle_print));
	XmxSetToggleButton(win->print_us_toggle_print,
			   !XmToggleButtonGetState(win->print_us_toggle_print));
}

/* ----------------------------- SAVE WINDOW ------------------------------ */

mo_status mo_save_window(mo_window *win, char *fname, 
				mo_format_token save_format)
{
	char *efname = (char *)malloc (sizeof (char) * (__MAX_HOME_LEN__ * 2));
	FILE *fp;

	if (pathEval(efname, fname)<0) {
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"Error in evaluating the path. (gui-dialogs.c)\n");
		}
#endif
	}
	fp = fopen (efname, "w");
	if (!fp) {
		char *buf, *final, tmpbuf[80];
		int final_len;

/* don't display dialog if command issued by cci application */
		if (cci_docommand)
			return mo_fail;

		buf=my_strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Uknown Error" );
			buf=tmpbuf;
		}
		final_len=30+((!efname || !*efname?3:strlen(efname))+13)+
				15+(strlen(buf)+3);
		final=(char *)calloc(final_len,sizeof(char));

		sprintf(final,"\nUnable to save document:\n   %s\n\nSave Error:\n   %s\n" ,(!efname || !*efname?" ":efname),buf);

		application_error(final,"Save Error");
		if (final) {
			free(final);
			final=NULL;
		}
	} else {
		if (!cci_docommand) {
			HTML_Print_Headers=
				XmToggleButtonGetState(win->print_header_toggle_save);
			HTML_Print_Footers=
				XmToggleButtonGetState(win->print_footer_toggle_save);
			HTML_Print_Paper_Size_A4= 
				XmToggleButtonGetState(win->print_a4_toggle_save);
		} else { /* cci app telling mosaic to save a file */
			if (save_format == mo_postscript) {
				HTML_Print_Headers = 1;
				HTML_Print_Footers = 1;
				HTML_Print_Paper_Size_A4 = 0;
			} else {
				HTML_Print_Headers = 0;
				HTML_Print_Footers = 0;
				HTML_Print_Paper_Size_A4 = 0;
			}
		}

		if (save_format == mo_plaintext) {
			char *text = HTMLGetText (
				win->scrolled_win, 0, win->current_node->url, 0);

			if (text) {
				fputs (text, fp);
				free (text);
			}
		} else if (save_format == mo_formatted_text) {
			char *text = HTMLGetText (

			win->scrolled_win, 1, win->current_node->url, 0);
			if (text) {
				fputs (text, fp);
				free (text);
			}
		} else if (save_format == mo_postscript) {
			char *text = HTMLGetText (win->scrolled_win, 2 + win->font_family, 

			win->current_node->url, win->current_node->last_modified);
			if (text) {
				fputs (text, fp);
				free (text);
			}
		} else if (win->current_node && win->current_node->text) {
/* HTML source */
			fputs (win->current_node->text, fp);
		}
		fclose (fp);
	}
	free (efname);
	return(mo_succeed);
}

/* ------------------------- mo_post_save_window -------------------------- */

static XmxCallback (save_win_cb)
{
  	char *fname = (char *)malloc (sizeof (char) * 128), efname[128+1];
	mo_window *win = (mo_window*)client_data;
  	char *ptr=NULL;

  	mo_busy ();

	XtUnmanageChild (win->save_win);
	XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
		XmSTRING_DEFAULT_CHARSET,
		&fname);

  	pathEval (efname, fname);           

	mo_save_window(win, efname, win->save_format);
  	mo_not_busy ();
	free (fname);
	return;
}

void format_sensitive(mo_window *win, int format) 
{
	Arg args[2];
	int n;
  
	if (format==mo_plaintext) { /*PLAIN*/
		XmxSetToggleButton(win->print_header_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_a4_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_us_toggle_save,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_save,args,n);
		XtSetValues(win->print_footer_toggle_save,args,n);
		XtSetValues(win->print_a4_toggle_save,args,n);
		XtSetValues(win->print_us_toggle_save,args,n);
	} else if (format==mo_formatted_text) { /*FORMATTED*/
		XmxSetToggleButton(win->print_header_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_a4_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_us_toggle_save,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_save,args,n);
		XtSetValues(win->print_footer_toggle_save,args,n);
		XtSetValues(win->print_a4_toggle_save,args,n);
		XtSetValues(win->print_us_toggle_save,args,n);
	} else if (format==mo_postscript) { /*POSTSCRIPT*/
                XmxSetToggleButton(win->print_header_toggle_save,get_pref_boolean(ePRINT_BANNERS));
                XmxSetToggleButton(win->print_footer_toggle_save,get_pref_boolean(ePRINT_FOOTNOTES));
                XmxSetToggleButton(win->print_a4_toggle_save,!get_pref_boolean(ePRINT_PAPER_SIZE_US));
                XmxSetToggleButton(win->print_us_toggle_save,get_pref_boolean(ePRINT_PAPER_SIZE_US));
		n=0;
		XtSetArg(args[n],XmNsensitive,TRUE); n++;
		XtSetValues(win->print_header_toggle_save,args,n);
		XtSetValues(win->print_footer_toggle_save,args,n);
		XtSetValues(win->print_a4_toggle_save,args,n);
		XtSetValues(win->print_us_toggle_save,args,n);
	} else if (format==mo_html) { /*HTML*/
		XmxSetToggleButton(win->print_header_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_a4_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_us_toggle_save,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_footer_toggle_save,args,n);
		XtSetValues(win->print_header_toggle_save,args,n);
		XtSetValues(win->print_a4_toggle_save,args,n);
		XtSetValues(win->print_us_toggle_save,args,n);
	} else { /*Boom...Bam...Error...*/
#ifndef DISABLE_TRACE
		if (srcTrace) {
			printf("ERROR! Format callback has no format!\n");
		}
#endif
		XmxSetToggleButton(win->print_header_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_a4_toggle_save,XmxNotSet);
		XmxSetToggleButton(win->print_us_toggle_save,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_save,args,n);
		XtSetValues(win->print_footer_toggle_save,args,n);
		XtSetValues(win->print_a4_toggle_save,args,n);
		XtSetValues(win->print_us_toggle_save,args,n);
	}
	return;
}

XmxCallback (mo_plaintext_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->save_format = mo_plaintext;
	format_sensitive(win,win->save_format);
}

XmxCallback (mo_formatted_text_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->save_format = mo_formatted_text;
	format_sensitive(win,win->save_format);
}

XmxCallback (mo_postscript_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->save_format = mo_postscript;
	format_sensitive(win,win->save_format);
}

XmxCallback (mo_html_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->save_format = mo_html;
	format_sensitive(win,win->save_format);
}

mo_status mo_post_save_window (mo_window *win)
{
	int i;
	XmString sfn,fbfn;
	char fileBuf[2048],*fileBoxFileName;

	if (!win->save_win) {
		Widget frame, workarea, format_label;
		Widget paper_size_toggle_box;

		win->save_win = XmxMakeFileSBDialog(win->base, 
			"Mosaic: Save Document",
			"Name for saved document:",
			save_win_cb, (XtPointer)win);
/* This makes a frame as a work area for the dialog box. */
		XmxSetArg (XmNmarginWidth, 5);
		XmxSetArg (XmNmarginHeight, 5);
		frame = XmxMakeFrame (win->save_win, XmxShadowEtchedIn);
		workarea = XmxMakeForm (frame);
		win->print_header_toggle_save=XmxMakeToggleButton(workarea, 
				"Include Banners",
				save_print_header_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_header_toggle_save,
				get_pref_boolean(ePRINT_BANNERS));
		win->print_footer_toggle_save=XmxMakeToggleButton(workarea,
				"Include Footnotes",
				save_print_footer_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_footer_toggle_save,
				get_pref_boolean(ePRINT_FOOTNOTES));
		paper_size_toggle_box=XmxMakeRadioBox(workarea);
		win->print_a4_toggle_save = XmxMakeToggleButton(
				paper_size_toggle_box,
				"A4 Paper Size",
				save_print_a4_cb,(XtPointer)win);
		win->print_us_toggle_save = XmxMakeToggleButton (
				paper_size_toggle_box,
				"US Letter Paper Size",
				save_print_us_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_a4_toggle_save,
				!get_pref_boolean(ePRINT_PAPER_SIZE_US));
		XmxSetToggleButton(win->print_us_toggle_save,
				get_pref_boolean(ePRINT_PAPER_SIZE_US));      
		format_label = XmxMakeLabel (workarea, "Format for document:" );
/* SWP -- 10/23/95 -- Set the default mode */
		if (!(get_pref_string(eSAVE_MODE)) || 
		    !*(get_pref_string(eSAVE_MODE))) {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [NULL], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,
				"save","plain text save");
			application_user_info_wait(tbuf);
			set_pref(eSAVE_MODE, (void *)strdup(MODE_PLAIN));
		}
		for (i=0; i<4; i++) {
			format_opts[i].set_state=XmxNotSet;
			format_opts[i].win = win;
		}
	        format_opts[0].data=mo_plaintext_cb;
	        format_opts[1].data=mo_formatted_text_cb;
	        format_opts[2].data=mo_postscript_cb;
	        format_opts[3].data=mo_html_cb;
		if (!strcasecmp(get_pref_string(eSAVE_MODE), MODE_HTML)) {
                	format_opts[3].set_state=XmxSet;
			win->save_format=mo_html;
		}
        	else if(!strcasecmp(get_pref_string(eSAVE_MODE),MODE_POSTSCRIPT)){
                	format_opts[2].set_state=XmxSet;
			win->save_format=mo_postscript;
		}
        	else if(!strcasecmp(get_pref_string(eSAVE_MODE),MODE_FORMATTED)){
                	format_opts[1].set_state=XmxSet;
			win->save_format=mo_formatted_text;
		}
        	else if (!strcasecmp(get_pref_string(eSAVE_MODE),MODE_PLAIN)) {
                	format_opts[0].set_state=XmxSet;
			win->save_format=mo_plaintext;
		} else {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [%d], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,
			   "save",get_pref_boolean(eSAVE_MODE),"plain text save");

			application_user_info_wait(tbuf);
                	format_opts[0].set_state=XmxSet;
			win->save_format=mo_plaintext;
		}
		win->format_optmenu = XmxRMakeOptionMenu (workarea, "",
				format_opts);
		XmxSetArg(XmNtopOffset,7);
		XmxSetConstraints(format_label, 
			XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetConstraints(win->format_optmenu->base, 
			XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_WIDGET,
			XmATTACH_FORM, NULL, NULL, format_label, NULL);
/*swp*/
		XmxSetArg(XmNtopOffset, 15);
		XmxSetConstraints(win->print_header_toggle_save, 
			XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE,
			format_label,NULL,NULL,NULL);
		XmxSetConstraints(win->print_footer_toggle_save, 
			XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE,
			win->print_header_toggle_save,NULL,NULL,NULL);
		XmxSetConstraints(paper_size_toggle_box, 
			XmATTACH_WIDGET, XmATTACH_FORM,
			XmATTACH_FORM, XmATTACH_NONE,
			win->print_footer_toggle_save,NULL,NULL,NULL);
			format_sensitive(win,win->save_format);
	} else {
		XmFileSelectionDoSearch (win->save_win, NULL);
	}
  
/*SWP -- 10.12.95 -- Save File now goes to a specific filename*/
	XtVaGetValues(win->save_win, XmNdirSpec, &fbfn, NULL);
	if (!XmStringGetLtoR(fbfn,XmSTRING_DEFAULT_CHARSET,&fileBoxFileName)) {
		fprintf(stderr,
			"Internal Error In Save As... PLEASE REPORT THIS!\n");
		return mo_fail;
	}
	if (*fileBoxFileName && win && win->current_node && 
	    win->current_node->url && *(win->current_node->url)) {
/*no need to check on NULL from getFileName as we know url exists*/
		sprintf(fileBuf,"%s%s",fileBoxFileName,
			getFileName(win->current_node->url));
		sfn=XmStringCreateLtoR(fileBuf,XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(win->save_win, XmNdirSpec, sfn, NULL);
	}
	XmxManageRemanage (win->save_win);
	return mo_succeed;
}

/* -------------------------- SAVEBINARY WINDOW --------------------------- */

/* This is used by libwww/HTFormat.c to present a user interface
   for retrieving files in binary transfer mode.  Obviously a redesign
   of the interface between the GUI and the commo library really needs
   to happen -- hopefully libwww2 will make this easy.  HA!!! */

/* Thanks to Martha Weinberg (lyonsm@hpwcsvp.mayfield.hp.com) for
   idea and code starting point. */
static char *temp_binary_fnam;

static XmxCallback (savebinary_cancel_cb)
{
	mo_window *win = (mo_window*)client_data;

	if (unlink(temp_binary_fnam)<0) {
		char *buf, *final, tmpbuf[80];
		int final_len;

		buf=my_strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Uknown Error" );
			buf=tmpbuf;
		}

		final_len=30+
			((!temp_binary_fnam || !*temp_binary_fnam?3:strlen(temp_binary_fnam))+13)
			+15+(strlen(buf)+13);
		final=(char *)calloc(final_len,sizeof(char));

		sprintf(final,"\nUnable to Remove Local File:\n   %s\n\nRemove Error:\n   %s\n" ,(!temp_binary_fnam || !*temp_binary_fnam?" ":temp_binary_fnam),buf);

		XmxMakeErrorDialog (win->base, final, "Remove Error" );
		XtManageChild (Xmx_w);

		if (final) {
			free(final);
			final=NULL;
		}
	}

/* This was dup'd down below... */
	free (temp_binary_fnam);
	return;
}

static XmxCallback (savebinary_win_cb)
{
	char *fname = (char *)malloc (sizeof (char) * 128), efname[128+1];
	char *cmd;
	mo_window *win = (mo_window*)client_data;
	char retBuf[BUFSIZ];
	int status;

  	mo_busy ();
	XtUnmanageChild (win->savebinary_win);
	XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
			XmSTRING_DEFAULT_CHARSET, &fname);

  	pathEval (efname, fname);
/*SWP -- New "mv" fucntion to take care of these /bin/mv things*/

	if((status=my_move(temp_binary_fnam,efname,retBuf,BUFSIZ,1))!=
		SYS_SUCCESS) {
		application_user_info_wait(retBuf);
	}
	free (temp_binary_fnam); 	/* This was dup'd down below... */
	free (fname);
  	mo_not_busy (); 
	return;
}


static mo_status mo_post_savebinary_window (mo_window *win)
{
	Widget Text;
	XmString sfn,fbfn;
	char fileBuf[2048],*fileBoxFileName;

  	if (!win->savebinary_win) {
      		XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
      		win->savebinary_win = XmxMakeFileSBDialog (win->base, 
			"NCSA Mosaic: Save Binary File To Local Disk" , 
         		"Name for binary file on local disk:" ,
         		savebinary_win_cb, (XtPointer)win);
      		XtAddCallback (win->savebinary_win, XmNcancelCallback, 
                      	savebinary_cancel_cb, win);
    	} else {
      		XmFileSelectionDoSearch (win->savebinary_win, NULL);
    	}

  	/*SWP -- 9.21.95 -- Save File now goes to a specific filename*/
  	XtVaGetValues(win->savebinary_win,
			XmNdirSpec, &fbfn,
			NULL);
  	if (!XmStringGetLtoR(fbfn,XmSTRING_DEFAULT_CHARSET,&fileBoxFileName)) {
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"Internal Error In Save Binary... PLEASE REPORT THIS!\n");
		}
#endif

        	return mo_fail;
  	}
  	if (*fileBoxFileName && saveFileName && *saveFileName) {
		char *sptr=NULL,*ptr=NULL;

		/*no need to check on NULL from getFileName as we know url exists*/
		sptr=getFileName(saveFileName);
                if (is_uncompressed && (ptr=strrchr(sptr,'.'))) { /* There is a
"." in it */                          
                        if (!strncmp(ptr,".Z",2) || !strncmp(ptr,".gz",3)) { /*
get rid of it! */                     
                                *ptr='\0';
                        }             
                } 
		sprintf(fileBuf,"%s%s",fileBoxFileName,sptr);
		sfn=XmStringCreateLtoR(fileBuf,XmSTRING_DEFAULT_CHARSET);
		XtVaSetValues(win->savebinary_win,
		      	XmNdirSpec, sfn,
		      	NULL);
  	}

  	XmxManageRemanage (win->savebinary_win);

  	return mo_succeed;
}

void rename_binary_file (char *fnam)
{
	mo_window *win = current_win;
	temp_binary_fnam = strdup (fnam);
	mo_post_savebinary_window (win);
}

/* ---------------------- mo_post_open_local_window ----------------------- */

static XmxCallback (open_local_win_cb)
{
  	char *fname = NULL, efname[128+1];
	char *url;
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->open_local_win);
	XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
			XmSTRING_DEFAULT_CHARSET,
			&fname);

  	pathEval (efname, fname);           

	url = mo_url_canonicalize_local (efname);
	if (url[strlen(url)-1] == '/')
		url[strlen(url)-1] = '\0';
	mo_load_window_text (win, url, NULL);
	free (fname);
	return;
}

mo_status mo_post_open_local_window (mo_window *win)
{
	if (!win->open_local_win) {
		win->open_local_win = XmxMakeFileSBDialog(win->base, 
			"Mosaic: Open Local Document" , 
			"Name of local document to open:" ,
			open_local_win_cb, (XtPointer)win);
	} else {
		XmFileSelectionDoSearch (win->open_local_win, NULL);
	}
	XmxManageRemanage (win->open_local_win);
	return mo_succeed;
}

/* ----------------------- mo_post_open_window ------------------------ */

static XmxCallback (open_win_cb0)
{
	mo_window *win = (mo_window*) client_data;
	char *url,*xurl;

	XtUnmanageChild (win->open_win);
	url = XmxTextGetString (win->open_text);
	if (!url || (!strlen(url))) /* nothing here so do nothing */
		return;
	mo_convert_newlines_to_spaces (url);
	/* if URL is enclosed inside <brackets> then extract it */
	if ( strstr(url, "<") )
		url = strtok(url, "<>");
	xurl=mo_url_prepend_protocol(url);
	mo_load_window_text (win, xurl, NULL);
}

static XmxCallback (open_win_cb1)
{
	mo_window *win = (mo_window*) client_data;

	XtUnmanageChild (win->open_win);
}
static XmxCallback (open_win_cb2)
{
	mo_window *win = (mo_window*) client_data;

	mo_open_another_window (win, 
			mo_assemble_help_url("docview-menubar-file.html"),
			NULL, NULL);
}
static XmxCallback (open_win_cb3)
{
	mo_window *win = (mo_window*) client_data;

	XmxTextSetString (win->open_text, "");
}

mo_status mo_post_open_window (mo_window *win)
{
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget open_form, label;

	if (!win->open_win) {
/* Create it for the first time. */
		win->open_win = XmxMakeFormDialog(win->base,
					"NCSA Mosaic: Open Document");
		dialog_frame = XmxMakeFrame (win->open_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
 
/* Main form. */
		open_form = XmxMakeForm (dialog_frame);
		label = XmxMakeLabel (open_form, "URL To Open: " );
		XmxSetArg (XmNwidth, 310);
		win->open_text = XmxMakeTextField (open_form);
		XmxAddCallbackToText (win->open_text,
					open_win_cb0, (XtPointer)win);
 
		dialog_sep = XmxMakeHorizontalSeparator (open_form);
		buttons_form = XmxMakeFormAndFourButtons (open_form,
			"Open" , "Clear" , "Dismiss" , "Help...",
			open_win_cb0, open_win_cb3, 
			open_win_cb1, open_win_cb2,(XtPointer)win);

/* Constraints for open_form. */
		XmxSetOffsets (label, 14, 0, 10, 0);
		XmxSetConstraints(label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetOffsets (win->open_text, 10, 0, 5, 10);
		XmxSetConstraints (win->open_text, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_WIDGET, XmATTACH_FORM, NULL, NULL, label, NULL);
		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, 
			XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
			win->open_text, buttons_form, NULL, NULL);
		XmxSetConstraints (buttons_form, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM, 
			NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->open_win);
	return mo_succeed;
}

/* ------------------------- mo_post_mail_window -------------------------- */

static XmxCallback (mail_win_cb1)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->mail_win);
}
static XmxCallback (mail_win_cb2)
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("docview-menubar-file.html"), 
		NULL, NULL);
}
static XmxCallback (mail_win_cb0)
{
	mo_window *win = (mo_window*)client_data;
	char *to, *subj, *text = 0, *content_type;
	int free_text;

	XtUnmanageChild (win->mail_win);

	HTML_Print_Headers=XmToggleButtonGetState(win->print_header_toggle_mail);
	HTML_Print_Footers=XmToggleButtonGetState(win->print_footer_toggle_mail);
       HTML_Print_Paper_Size_A4=XmToggleButtonGetState(win->print_a4_toggle_mail);
	to = XmxTextGetString (win->mail_to_text);
	if (!to)
		return;
	if (to[0] == '\0')
		return;

	subj = XmxTextGetString (win->mail_subj_text);
	if (!subj)
		subj = strdup ("\0");

	if (win->mail_format == mo_plaintext) {
		text =HTMLGetText(win->scrolled_win, 0, win->current_node->url,0);
		content_type = "text/plain";
		free_text = 1;
	} else 
		if (win->mail_format == mo_formatted_text) {
			text = HTMLGetText (win->scrolled_win, 1, win->current_node->url,0);
			content_type = "text/plain";
			free_text = 1;
		} else 
			if (win->mail_format == mo_postscript) {
				text = HTMLGetText (win->scrolled_win, 2 + win->font_family, win->current_node->url, win->current_node->last_modified);
				content_type = "application/postscript";
				free_text = 1;
			} else 
				if(win->current_node && win->current_node->text) {
							/* HTML source. */
					text = win->current_node->text;
					content_type = "text/x-html";
					free_text = 0;
				}

	if (text)
		mo_send_mail_message (text, to, subj, 
			XmToggleButtonGetState(win->print_url_only)?
				"url_only" : content_type,
			win->current_node ? win->current_node->url : NULL);
	if (free_text && text)
		free (text);
	free (to);
	free (subj);
}

void mail_sensitive(mo_window *win, int format) 
{
	Arg args[2];
	int n;
  
	if (format==mo_plaintext) { /*PLAIN*/
		XmxSetToggleButton(win->print_header_toggle_mail,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_mail,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_mail,args,n);
		XtSetValues(win->print_footer_toggle_mail,args,n);
                XtSetValues(win->print_a4_toggle_mail,args,n);
                XtSetValues(win->print_us_toggle_mail,args,n);
	} else if (format==mo_formatted_text) { /*FORMATTED*/
		XmxSetToggleButton(win->print_header_toggle_mail,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_mail,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_mail,args,n);
		XtSetValues(win->print_footer_toggle_mail,args,n);
                XtSetValues(win->print_a4_toggle_mail,args,n);
                XtSetValues(win->print_us_toggle_mail,args,n);
	} else if (format==mo_postscript) { /*POSTSCRIPT*/
                XmxSetToggleButton(win->print_header_toggle_mail,
					get_pref_boolean(ePRINT_BANNERS));
                XmxSetToggleButton(win->print_footer_toggle_mail,
					get_pref_boolean(ePRINT_FOOTNOTES));
                XmxSetToggleButton(win->print_a4_toggle_mail,
					!get_pref_boolean(ePRINT_PAPER_SIZE_US));
                XmxSetToggleButton(win->print_us_toggle_mail,
					get_pref_boolean(ePRINT_PAPER_SIZE_US));
		n=0;
		XtSetArg(args[n],XmNsensitive,TRUE); n++;
		XtSetValues(win->print_header_toggle_mail,args,n);
		XtSetValues(win->print_footer_toggle_mail,args,n);
                XtSetValues(win->print_a4_toggle_mail,args,n);
                XtSetValues(win->print_us_toggle_mail,args,n);
	} else if (format==mo_html) { /*HTML*/
		XmxSetToggleButton(win->print_header_toggle_mail,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_mail,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_mail,args,n);
		XtSetValues(win->print_footer_toggle_mail,args,n);
                XtSetValues(win->print_a4_toggle_mail,args,n);
                XtSetValues(win->print_us_toggle_mail,args,n);
	} else { /*Boom...Bam...Error...*/
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"ERROR! Format callback has no format!\n");
		}
#endif
		XmxSetToggleButton(win->print_header_toggle_mail,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_mail,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_mail,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_mail,args,n);
		XtSetValues(win->print_footer_toggle_mail,args,n);
                XtSetValues(win->print_a4_toggle_mail,args,n);
                XtSetValues(win->print_us_toggle_mail,args,n);
	}
	return;
}

XmxCallback (mo_plaintext_mail_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->mail_format = mo_plaintext;
	mail_sensitive(win,win->mail_format);
}

XmxCallback (mo_formatted_text_mail_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->mail_format = mo_formatted_text;
	mail_sensitive(win,win->mail_format);
}

XmxCallback (mo_postscript_mail_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->mail_format = mo_postscript;
	mail_sensitive(win,win->mail_format);
}

XmxCallback (mo_html_mail_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->mail_format = mo_html;
	mail_sensitive(win,win->mail_format);
}

mo_status mo_post_mail_window (mo_window *win)
{
	int i;
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget mail_form, to_label, subj_label;
	Widget frame, workarea, format_label;
	Widget paper_size_toggle_box;
	Widget frame2, url_toggle_box;  

	if (!win->mail_win) {

/* Create it for the first time. */
		win->mail_win = XmxMakeFormDialog(win->base, "NCSA Mosaic: Mail Document" );
		dialog_frame = XmxMakeFrame (win->mail_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
/* Main form. */
		mail_form = XmxMakeForm (dialog_frame);
      
		to_label = XmxMakeLabel (mail_form, "Mail To: " );
		XmxSetArg (XmNwidth, 335);
		win->mail_to_text = XmxMakeTextField (mail_form);
      
		subj_label = XmxMakeLabel (mail_form, "Subject: " );
		win->mail_subj_text = XmxMakeTextField (mail_form);

		XmxSetArg (XmNmarginWidth, 5);
		XmxSetArg (XmNmarginHeight, 5);
		frame = XmxMakeFrame (mail_form, XmxShadowEtchedIn);
		workarea = XmxMakeForm (frame);
        
/*swp*/
		win->print_header_toggle_mail=XmxMakeToggleButton(workarea,
			"Include Banners",mail_print_header_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_header_toggle_mail,get_pref_boolean(ePRINT_BANNERS));

		win->print_footer_toggle_mail=XmxMakeToggleButton(workarea,
			"Include Footnotes",mail_print_footer_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_footer_toggle_mail,get_pref_boolean(ePRINT_FOOTNOTES));

		paper_size_toggle_box=XmxMakeRadioBox(workarea);
		win->print_a4_toggle_mail = XmxMakeToggleButton(paper_size_toggle_box,
			"A4 Paper Size" ,mail_print_a4_cb,(XtPointer)win);
		win->print_us_toggle_mail = XmxMakeToggleButton(paper_size_toggle_box,
			"US Letter Paper Size",mail_print_us_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_a4_toggle_mail,!get_pref_boolean(ePRINT_PAPER_SIZE_US));
		XmxSetToggleButton(win->print_us_toggle_mail,get_pref_boolean(ePRINT_PAPER_SIZE_US));

		format_label = XmxMakeLabel (workarea, "Format for document:" );

/* SWP -- 10/23/95 -- Set the default mode */
		if (!(get_pref_string(eMAIL_MODE)) || !*(get_pref_string(eMAIL_MODE))) {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [NULL], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,"mail","plain text mail");
			application_user_info_wait(tbuf);
			set_pref(eMAIL_MODE, (void *)strdup(MODE_PLAIN));
		}

		for (i=0; i<4; i++) {
			format_opts[i].set_state=XmxNotSet;
			format_opts[i].win = win;
		}

		format_opts[0].data=mo_plaintext_mail_cb;
		format_opts[1].data=mo_formatted_text_mail_cb;
		format_opts[2].data=mo_postscript_mail_cb;
		format_opts[3].data=mo_html_mail_cb;
		if (!strcasecmp(get_pref_string(eMAIL_MODE),MODE_HTML)) {
                	format_opts[3].set_state=XmxSet;
			win->mail_format=mo_html;
		} else if (!strcasecmp(get_pref_string(eMAIL_MODE),MODE_POSTSCRIPT)) {
                	format_opts[2].set_state=XmxSet;
			win->mail_format=mo_postscript;
		} else if (!strcasecmp(get_pref_string(eMAIL_MODE),MODE_FORMATTED)) {
			format_opts[1].set_state=XmxSet;
			win->mail_format=mo_formatted_text;
		} else if (!strcasecmp(get_pref_string(eMAIL_MODE),MODE_PLAIN)) {
			format_opts[0].set_state=XmxSet;
			win->mail_format=mo_plaintext;
		} else {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [%s], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,"mail",get_pref_string(eMAIL_MODE),"plain text mail");

			application_user_info_wait(tbuf);
			format_opts[0].set_state=XmxSet;
			win->mail_format=mo_plaintext;
		}

		win->mail_fmtmenu = XmxRMakeOptionMenu (workarea, "", format_opts);

		XmxSetArg(XmNtopOffset,7);
		XmxSetConstraints (format_label, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetConstraints (win->mail_fmtmenu->base, XmATTACH_FORM, 
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, 
			NULL, NULL, format_label, NULL);
/*swp*/
		XmxSetArg(XmNtopOffset, 15);
		XmxSetConstraints (win->print_header_toggle_mail, 
			XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
			XmATTACH_NONE, format_label,NULL,NULL,NULL);
		XmxSetConstraints (win->print_footer_toggle_mail, 
			XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
			XmATTACH_NONE, win->print_header_toggle_mail,NULL,NULL,NULL);
		XmxSetConstraints (paper_size_toggle_box, XmATTACH_WIDGET, 
			XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE, 
			win->print_footer_toggle_mail,NULL,NULL,NULL);

		mail_sensitive(win,win->mail_format);
		frame2 = XmxMakeFrame (mail_form, XmxShadowEtchedIn); 
		url_toggle_box=XmxMakeRadioBox(frame2);
		win->print_doc_only = XmxMakeToggleButton(url_toggle_box,
				"Mail Entire Document",
				print_url_cb, win);  
		win->print_url_only = XmxMakeToggleButton(url_toggle_box,
				"Mail URL Only",
				print_url_cb, win);
		XmxSetToggleButton(win->print_doc_only, 1);
		XmxSetToggleButton(win->print_url_only, 0);
		dialog_sep = XmxMakeHorizontalSeparator (mail_form);
		buttons_form = XmxMakeFormAndThreeButtons(mail_form, 
				"Mail" , "Dismiss" , "Help..." , 
				mail_win_cb0, mail_win_cb1, mail_win_cb2, (XtPointer) win);

/* Constraints for mail_form. */
		XmxSetOffsets (to_label, 14, 0, 10, 0);
		XmxSetConstraints (to_label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetOffsets (win->mail_to_text, 10, 0, 5, 10);
		XmxSetConstraints (win->mail_to_text, XmATTACH_FORM, 
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, 
			NULL, NULL, to_label, NULL);

		XmxSetOffsets (subj_label, 14, 0, 10, 0);
		XmxSetConstraints (subj_label, XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE, 
			win->mail_to_text, NULL, NULL, NULL);
		XmxSetOffsets (win->mail_subj_text, 10, 0, 5, 10);
		XmxSetConstraints (win->mail_subj_text, XmATTACH_WIDGET, 
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, 
			win->mail_to_text, NULL, subj_label, NULL);

		XmxSetOffsets (frame, 10, 0, 10, 10);
		XmxSetConstraints (frame, XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM, 
			win->mail_subj_text, NULL, NULL, NULL);

		XmxSetOffsets (frame2, 10, 0, 10, 10);
		XmxSetConstraints(frame2, XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM,
			frame, NULL, NULL, NULL);
		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, 
			XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
			frame2, buttons_form, NULL, NULL);
		XmxSetConstraints (buttons_form, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM, 
			NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->mail_win);
	return mo_succeed;
}

mo_status mo_print_window(mo_window *win, 
			mo_format_token print_format, char *lpr)
{
	char *fnam, *cmd;
	FILE *fp;

	fnam = mo_tmpnam(win->current_node->url);

	if (cci_docommand) {
		if (print_format == mo_postscript) {
			HTML_Print_Headers= 1;
			HTML_Print_Footers= 1;
			HTML_Print_Paper_Size_A4=0;
		} else {
			HTML_Print_Headers= 0;
			HTML_Print_Footers= 0;
			HTML_Print_Paper_Size_A4=0;
		}
	} else {
		HTML_Print_Headers=XmToggleButtonGetState(win->print_header_toggle_print);
		HTML_Print_Footers=XmToggleButtonGetState(win->print_footer_toggle_print);
		HTML_Print_Paper_Size_A4=XmToggleButtonGetState(win->print_a4_toggle_print);
	}

	fp = fopen (fnam, "w");
	if (!fp)
		goto oops;
	if (win->print_format == mo_plaintext) {
		char *text = HTMLGetText (win->scrolled_win, 0, win->current_node->url,0);

		if (text) {
			fputs (text, fp);
			free (text);
		}
	} else if (win->print_format == mo_formatted_text) {
		char *text = HTMLGetText (win->scrolled_win, 1, win->current_node->url,0);
		if (text) {
			fputs (text, fp);
			free (text);
		}
	} else if (win->print_format == mo_postscript) {
		char *text = HTMLGetText (win->scrolled_win, 2 + win->font_family, win->current_node->url,win->current_node->last_modified);
		if (text) {
			fputs (text, fp);
			free (text);
		}
	} else if (win->current_node && win->current_node->text) {
/* HTML source */
		fputs (win->current_node->text, fp);
	}
	fclose (fp);
	cmd = (char *)malloc ((strlen (lpr) + strlen (fnam) + 24));
	sprintf (cmd, "%s %s", lpr, fnam);
	System(cmd,"Print Information");
	free (cmd);
oops:
	free (lpr);
	unlink(fnam); 
	free (fnam);
	return mo_succeed;
}

/* ----------------------- mo_post_print_window ------------------------ */

static XmxCallback (print_win_cb0)
{
	mo_window *win = (mo_window*)client_data;
	char *lpr;

	XtUnmanageChild (win->print_win);
	lpr = XmxTextGetString (win->print_text);
	if (!lpr)
		return;
	if (lpr[0] == '\0')
		return;
	mo_print_window(win, win->print_format, lpr);
}

static XmxCallback (print_win_cb1)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->print_win);
}
static XmxCallback (print_win_cb2)
{
	mo_window *win = (mo_window*)client_data;
	char *lpr;

	mo_open_another_window (win, 
		mo_assemble_help_url ("docview-menubar-file.html"),
		NULL, NULL);
}

XmxCallback (mo_plaintext_print_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->print_format = mo_plaintext;
	print_sensitive(win,win->print_format);
}

XmxCallback (mo_formatted_text_print_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->print_format = mo_formatted_text;
	print_sensitive(win,win->print_format);
}

XmxCallback (mo_postscript_print_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->print_format = mo_postscript;
	print_sensitive(win,win->print_format);
}

XmxCallback (mo_html_print_cb) 
{
	mo_window *win = (mo_window*)client_data;

	win->print_format = mo_html;
	print_sensitive(win,win->print_format);
}

void print_sensitive(mo_window *win, int format) 
{
	Arg args[2];
	int n;
  
	if (format==mo_plaintext) { /*PLAIN*/
		XmxSetToggleButton(win->print_header_toggle_print,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_print,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_print,args,n);
		XtSetValues(win->print_footer_toggle_print,args,n);
                XtSetValues(win->print_a4_toggle_print,args,n);
                XtSetValues(win->print_us_toggle_print,args,n);
	} else if (format==mo_formatted_text) { /*FORMATTED*/
		XmxSetToggleButton(win->print_header_toggle_print,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_print,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_print,args,n);
		XtSetValues(win->print_footer_toggle_print,args,n);
                XtSetValues(win->print_a4_toggle_print,args,n);
                XtSetValues(win->print_us_toggle_print,args,n);
	} else if (format==mo_postscript) { /*POSTSCRIPT*/
		XmxSetToggleButton(win->print_header_toggle_print,get_pref_boolean(ePRINT_BANNERS));
		XmxSetToggleButton(win->print_footer_toggle_print,get_pref_boolean(ePRINT_FOOTNOTES));
                XmxSetToggleButton(win->print_a4_toggle_print,!get_pref_boolean(ePRINT_PAPER_SIZE_US));
                XmxSetToggleButton(win->print_us_toggle_print,get_pref_boolean(ePRINT_PAPER_SIZE_US));
		n=0;
		XtSetArg(args[n],XmNsensitive,TRUE); n++;
		XtSetValues(win->print_header_toggle_print,args,n);
		XtSetValues(win->print_footer_toggle_print,args,n);
                XtSetValues(win->print_a4_toggle_print,args,n);
                XtSetValues(win->print_us_toggle_print,args,n);
	} else if (format==mo_html) { /*HTML*/
		XmxSetToggleButton(win->print_header_toggle_print,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_print,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_print,args,n);
		XtSetValues(win->print_footer_toggle_print,args,n);
                XtSetValues(win->print_a4_toggle_print,args,n);
                XtSetValues(win->print_us_toggle_print,args,n);
	} else { /*Boom...Bam...Error...*/
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"ERROR! Format callback has no format!\n");
		}
#endif
		XmxSetToggleButton(win->print_header_toggle_print,XmxNotSet);
		XmxSetToggleButton(win->print_footer_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_a4_toggle_print,XmxNotSet);
                XmxSetToggleButton(win->print_us_toggle_print,XmxNotSet);
		n=0;
		XtSetArg(args[n],XmNsensitive,FALSE); n++;
		XtSetValues(win->print_header_toggle_print,args,n);
		XtSetValues(win->print_footer_toggle_print,args,n);
                XtSetValues(win->print_a4_toggle_print,args,n);
                XtSetValues(win->print_us_toggle_print,args,n);
	}
	return;
}

mo_status mo_post_print_window (mo_window *win)
{
	int i;

	if (!win->print_win) {
		Widget dialog_frame;
		Widget dialog_sep, buttons_form;
		Widget print_form, print_label;
		Widget frame, workarea, format_label;
		Widget paper_size_toggle_box;

/* Create it for the first time. */
		win->print_win = XmxMakeFormDialog(win->base, 
					"Mosaic: Print Document" );
		dialog_frame = XmxMakeFrame (win->print_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints(dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
/* Main form. */
		print_form = XmxMakeForm (dialog_frame);
		print_label = XmxMakeLabel (print_form, "Print Command: " );
		XmxSetArg (XmNwidth, 270);
		win->print_text = XmxMakeTextField (print_form);
		XmxTextSetString(win->print_text,(char*)get_pref(ePRINT_COMMAND));

		XmxSetArg (XmNmarginWidth, 5);
		XmxSetArg (XmNmarginHeight, 5);
		frame = XmxMakeFrame (print_form, XmxShadowEtchedIn);
		workarea = XmxMakeForm (frame);
        
/*swp*/
		win->print_header_toggle_print=XmxMakeToggleButton(workarea,
			"Include Banners",print_print_header_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_header_toggle_print,get_pref_boolean(ePRINT_BANNERS));

		win->print_footer_toggle_print=XmxMakeToggleButton(workarea,
			"Include Footnotes",print_print_footer_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_footer_toggle_print,get_pref_boolean(ePRINT_FOOTNOTES));

		paper_size_toggle_box=XmxMakeRadioBox(workarea);
		win->print_a4_toggle_print = XmxMakeToggleButton(paper_size_toggle_box,
			"A4 Paper Size" ,print_print_a4_cb,(XtPointer)win);
		win->print_us_toggle_print = XmxMakeToggleButton(paper_size_toggle_box,
			"US Letter Paper Size",print_print_us_cb,(XtPointer)win);
		XmxSetToggleButton(win->print_a4_toggle_print,!get_pref_boolean(ePRINT_PAPER_SIZE_US));
		XmxSetToggleButton(win->print_us_toggle_print,get_pref_boolean(ePRINT_PAPER_SIZE_US));

		format_label = XmxMakeLabel (workarea, "Format for document:" );

/* SWP -- 10/23/95 -- Set the default mode */
		if (!(get_pref_string(ePRINT_MODE)) || !*(get_pref_string(ePRINT_MODE))) {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [NULL], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,"print","plain text print");

			application_user_info_wait(tbuf);
			set_pref(ePRINT_MODE, (void *)strdup(MODE_PLAIN));
		}

		for (i=0; i<4; i++) {
			format_opts[i].set_state=XmxNotSet;
			format_opts[i].win = win;
		}
		format_opts[3].data=mo_html_print_cb;
		format_opts[2].data=mo_postscript_print_cb;
		format_opts[1].data=mo_formatted_text_print_cb;
		format_opts[0].data=mo_plaintext_print_cb;

		if (!strcasecmp(get_pref_string(ePRINT_MODE),MODE_HTML)) {
			format_opts[3].set_state=XmxSet;
			win->print_format=mo_html;
		} else if (!strcasecmp(get_pref_string(ePRINT_MODE),MODE_POSTSCRIPT)) {
			format_opts[2].set_state=XmxSet;
			win->print_format=mo_postscript;
		} else if (!strcasecmp(get_pref_string(ePRINT_MODE),MODE_FORMATTED)) {
			format_opts[1].set_state=XmxSet;
			win->print_format=mo_formatted_text;
		} else if (!strcasecmp(get_pref_string(ePRINT_MODE),MODE_PLAIN)) {
			format_opts[0].set_state=XmxSet;
			win->print_format=mo_plaintext;
		} else {
			char tbuf[BUFSIZ];

			sprintf(tbuf,"You have set the default %s mode to:\n     [%s], which is not valid. Defaulting to %s mode.\n\nPlease use one of the following:\n     plain, formatted, postscript, or html." ,"print",get_pref_string(ePRINT_MODE),"plain text print");

			application_user_info_wait(tbuf);
			format_opts[0].set_state=XmxSet;
			win->print_format=mo_plaintext;
		}
		win->print_fmtmenu = XmxRMakeOptionMenu (workarea, "", format_opts);

		XmxSetArg(XmNtopOffset, 7);
		XmxSetConstraints (format_label, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetConstraints (win->print_fmtmenu->base, XmATTACH_FORM,
			XmATTACH_NONE, XmATTACH_NONE, XmATTACH_FORM, NULL,
			NULL, NULL, NULL);
/*swp*/
		XmxSetArg(XmNtopOffset, 15);
		XmxSetConstraints (win->print_header_toggle_print, XmATTACH_WIDGET,
			XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
			format_label,NULL,NULL,NULL);
		XmxSetConstraints (win->print_footer_toggle_print, XmATTACH_WIDGET,
			XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
		win->print_header_toggle_print,NULL,NULL,NULL);
		XmxSetConstraints (paper_size_toggle_box, XmATTACH_WIDGET,
			XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			win->print_footer_toggle_print,NULL,NULL,NULL);

		print_sensitive(win,win->print_format);

		dialog_sep = XmxMakeHorizontalSeparator (print_form);

		buttons_form = XmxMakeFormAndThreeButtons (print_form,
			"Print" , "Dismiss" , "Help..." ,
			print_win_cb0, print_win_cb1, print_win_cb2,
			(XtPointer) win);

/* Constraints for print_form. */
		XmxSetOffsets (print_label, 14, 0, 10, 0);
		XmxSetConstraints (print_label, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
		XmxSetOffsets (win->print_text, 10, 0, 5, 10);
		XmxSetConstraints (win->print_text, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_WIDGET, XmATTACH_FORM, NULL, NULL, print_label, NULL);

		XmxSetOffsets (frame, 10, 0, 10, 10);
		XmxSetConstraints (frame, XmATTACH_WIDGET, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_FORM, win->print_text, NULL, NULL, NULL);

		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET,
			XmATTACH_FORM, XmATTACH_FORM, frame, buttons_form, NULL, NULL);

		XmxSetConstraints (buttons_form, XmATTACH_NONE, XmATTACH_FORM,
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->print_win);
	return mo_succeed;
}

/* ----------------------- mo_post_source_window ------------------------ */

/*
Okay...forward caseless search works...I think forward caseful search works.
      
Reverse searching is screwed up as far as where the point that gets highlighted
and the actual cursor position. Should be easy to fix with a little work.
      
*/    
                                     
void source_position(Widget source_view, int pos, int end) 
{
        XmTextSetSelection(source_view, pos, (end==(-1)?pos:end), CurrentTime);
        return;                      
}          

mo_status mo_source_search_window(mo_window *win,char *str, int backward,
                                  int caseless) 
{
	int searchlen,start,str_len=strlen(str);
	char *ptr=NULL,*tptr=NULL,*my_str=NULL;

	if (!(win) || 
	    !(win->current_node) ||
	    !(win->current_node->text) ||
	    !*(win->current_node->text)) {
		application_user_info_wait("This is a bug! Please report what you were\ndoing and the URL you are current at to:\n\nmosaic-x@ncsa.uiuc.edu\n\nThank You!!");
		return(mo_fail); 
	}

	searchlen=strlen(win->current_node->text);

/* search the first hit every time if by cci application */
	if (cci_docommand) {
		win->src_search_pos=0;
	}
                                     
/*                           
* If we are going forwards, the start position is the current
*   search position.        
* If we are going backwards, the start position is the current
*   search position - the current search string length.
* If the current position is non-zero, this is a "find again" type
*   search which is why the "backwards" way works.
*/                          
	if (!backward) { /* Forward Search */
		start=win->src_search_pos; 
		if (start>=searchlen) {
			if (win->src_search_pos) {
				application_user_info_wait("Sorry, no more matches in this document.");
			} else {       
				application_user_info_wait("Sorry, no matches in this document.");                 
			}
			return(mo_fail);
		}                    
		my_str=strdup((win->current_node->text+start));
		ptr=my_str;          
	} else { /* Backwards Search */
		if (!win->src_search_pos) { /* First time...go to end */
			start=searchlen;
		} else { /* "Find Again" */  
			start=win->src_search_pos-str_len;
			if (start<0) {
				if (win->src_search_pos) {
					application_user_info_wait("Sorry, no more matches in this document.");    
				} else {
					application_user_info_wait("Sorry, no matches in this document.");         
				}    
				return(mo_fail);
			}            
		}                    
		my_str=strdup(win->current_node->text);
		*(my_str+start)='\0';
	}                            
	while (1) {                  
		if (!backward) {     
			if (tptr) {  
				ptr++;
				if (!ptr || !*ptr) {
					ptr=NULL;
					break;
				}    
			}            
			if (caseless) {
				tptr=ptr=strcasechr(ptr,*str);/* Find occurence */
			} else {
				tptr=ptr=strchr(ptr,*str); /* Find occurence */
			}
		} else {
			if (tptr) {  
				*tptr='\0';
			}            
			if (caseless) {
				tptr=ptr=strrcasechr(my_str,*str); /* Find occurence */ 
			} else {
				tptr=ptr=strrchr(my_str,*str); /* Find occurence */
			}
		}                    
		if (!ptr) {          
			break;       
		}      
		if (caseless) {      
			if (!strncasecmp(ptr,str,str_len)) {
				break;
			}
			continue;
		} else {
			if (!strncmp(ptr,str,str_len)) {
				break;
			}
			continue;
		}
	}
	if (!ptr) { 
		free(my_str);        
		if (win->src_search_pos) {
			application_user_info_wait("Sorry, no more matches in this document.");
		} else { 
			application_user_info_wait("Sorry, no matches in this document.");
		}
		return(mo_fail);
	}
	if (!backward) { 
		win->src_search_pos=(ptr-my_str)+start+str_len;
		source_position(win->source_text,
				win->src_search_pos-str_len,
				win->src_search_pos);
	} else {
		win->src_search_pos=(ptr-my_str);
		source_position(win->source_text,
				win->src_search_pos,
				win->src_search_pos+str_len);
	}
	free(my_str);
	return(mo_succeed);
}

static XmxCallback(source_search_win_cb0)  /* search */  
{
	mo_window *win= (mo_window *)client_data;
	char *str= XmxTextGetString(win->src_search_win_text);

	if (str && *str) {
		mo_source_search_window(win, str,
			XmToggleButtonGetState(win->src_search_backwards_toggle), 
			XmToggleButtonGetState(win->src_search_caseless_toggle));
	}
}

static XmxCallback(source_search_win_cb1)  /* reset */  
{
	mo_window *win= (mo_window *)client_data;

/* Clear out the search text. */
	XmxTextSetString(win->src_search_win_text, "");

/* Subsequent searches start at the beginning. */
	win->src_search_pos=0;
                                       
/* Reposition document at top of screen. */
	source_position(win->source_text, 0, (-1));
}

static XmxCallback(source_search_win_cb2)  /* dismiss */  
{
	mo_window *win= (mo_window *)client_data;

        XtUnmanageChild(win->src_search_win);
}                    
static XmxCallback(source_search_win_cb3)  /* help */  
{
	mo_window *win= (mo_window *)client_data;

	mo_open_another_window(win, mo_assemble_help_url(
					"docview-menubar-file.html"),
				    NULL, NULL);
}                    

mo_status mo_post_source_search_window(mo_window *win) 
{
        if (!win->src_search_win) {
                Widget dialog_frame;           
                Widget dialog_sep, buttons_form;
                Widget search_form, label;
                        
                /* Create it for the first time. */ 
                win->src_search_win= XmxMakeFormDialog(win->base,
                                          "NCSA Mosaic: Search in Source View");
                dialog_frame= XmxMakeFrame(win->src_search_win, XmxShadowOut);

                /* Constraints for base. */
                XmxSetConstraints(dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
                     XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);

                /* Main form. */     
                search_form= XmxMakeForm(dialog_frame);
                                    
                label= XmxMakeLabel(search_form, "Find string in Source View: " );
                XmxSetArg(XmNcolumns, 25);      
                win->src_search_win_text=  XmxMakeText(search_form);
                XmxAddCallbackToText(win->src_search_win_text,
                       source_search_win_cb0, (XtPointer)win);
                                    
                win->src_search_caseless_toggle= XmxMakeToggleButton(search_form,
                         "Caseless Search", NULL,(XtPointer)win);  
                XmxSetToggleButton(win->src_search_caseless_toggle, XmxSet);
                win->src_search_backwards_toggle= XmxMakeToggleButton(search_form,
                          "Backwards Search", NULL,(XtPointer)win); 

                dialog_sep= XmxMakeHorizontalSeparator(search_form);

                buttons_form= XmxMakeFormAndFourButtons(search_form,
                     "Find", "Reset", "Dismiss", "Help...",
                     source_search_win_cb0, source_search_win_cb1,
                     source_search_win_cb2, source_search_win_cb3,
		     (XtPointer)win);
                                    
                /* Constraints for search_form. */
                XmxSetOffsets(label, 13,   0,    10,   0);   
                /* Label attaches top to form, bottom to nothing,
                   left to form, right to nothing. */   
                XmxSetConstraints(label, XmATTACH_FORM, XmATTACH_NONE,
                     XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
                XmxSetOffsets(win->src_search_win_text, 10,   0,    5,    8);   
                /* search_win_text attaches top to form, bottom to nothing,
                   left to label, right to form. */     
                XmxSetConstraints(win->src_search_win_text,
		    XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
                    NULL, NULL, label, NULL);

                /* search_caseless_toggle attaches top to search_win_text, bottom to nothing,                     
                   left to position, right to position. */
                XmxSetConstraints(win->src_search_caseless_toggle,
                   XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_NONE,
                   win->src_search_win_text, NULL, label, NULL);
                XmxSetOffsets(win->src_search_caseless_toggle, 8, 0, 2, 0);

                /* search_backwards_toggle attaches top to search_caseless_toggle,
                   bottom to nothing, left to position, right to position. */
                XmxSetConstraints(win->src_search_backwards_toggle,
                  XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_NONE, 
                  win->src_search_caseless_toggle, NULL, label, NULL);
                XmxSetOffsets(win->src_search_backwards_toggle, 8, 0, 2, 0);

                XmxSetOffsets(dialog_sep,  8,    0,    0,    0);   
                /* dialog_sep attaches top to search_backwards_toggle,
                   bottom to buttons_form, left to form, right to form */
                XmxSetConstraints(dialog_sep,
                   XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
                   win->src_search_backwards_toggle, buttons_form, NULL, NULL);
                XmxSetConstraints(buttons_form,
                   XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM, 
                   NULL, NULL, NULL, NULL);
        }                           
        XmxManageRemanage(win->src_search_win);
        return(mo_succeed);          
}           

static XmxCallback (source_win_cb0)  /* Dismiss */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->source_win);
        if (win->src_search_win && XtIsManaged(win->src_search_win)) {
        	XtUnmanageChild(win->src_search_win);
        }
}

static XmxCallback (source_win_cb1)  /* Help */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("docview-menubar-file.html"),
		NULL, NULL);
}

static XmxCallback (source_win_cb2)  /* Search */
{
	mo_window *win = (mo_window*)client_data;
	mo_post_source_search_window(win);
}

mo_status mo_post_source_window (mo_window *win)
{
	if (!win->source_win) {
		Widget dialog_frame;
		Widget dialog_sep, buttons_form;
		Widget source_form, label,dlabel;
      
/* Create it for the first time. */
		win->source_win = XmxMakeFormDialog(win->base, 
			"Mosaic: Document Source View" );
		dialog_frame = XmxMakeFrame (win->source_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
/* Main form. */
		source_form = XmxMakeForm (dialog_frame);
      
		label = XmxMakeLabel (source_form, "URL: " );
		dlabel= XmxMakeLabel(source_form, "Last Modified: ");
		XmxSetArg (XmNcursorPositionVisible, False);
		XmxSetArg (XmNeditable, False);
		win->source_url_text = XmxMakeText (source_form);
		XmxSetArg(XmNcursorPositionVisible, False);    
		XmxSetArg(XmNeditable, False);    
		win->source_date_text= XmxMakeText(source_form);

/* Info window: text widget, not editable. */
		XmxSetArg (XmNscrolledWindowMarginWidth, 10);
		XmxSetArg (XmNscrolledWindowMarginHeight, 10);
		XmxSetArg (XmNcursorPositionVisible, True);
		XmxSetArg (XmNeditable, False);
		XmxSetArg (XmNeditMode, XmMULTI_LINE_EDIT);
		XmxSetArg (XmNrows, 15);
		XmxSetArg (XmNcolumns, 80);
		win->source_text = XmxMakeScrolledText (source_form);
      
		dialog_sep = XmxMakeHorizontalSeparator (source_form);
		buttons_form = XmxMakeFormAndThreeButtonsSqueezed(source_form, 
			"Search...","Dismiss","Help...", 
			source_win_cb2, source_win_cb0, source_win_cb1,
			(XtPointer) win);

/* Constraints for source_form. */
		XmxSetOffsets (label, 13, 0, 10, 0);
		XmxSetConstraints(label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE,
			NULL, NULL, NULL, NULL);
		XmxSetOffsets(dlabel, 18, 0, 10, 0);    
		XmxSetConstraints(dlabel, XmATTACH_WIDGET, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE,
			label, NULL, NULL, NULL);
		XmxSetOffsets (win->source_url_text, 10, 0, 5, 10);
		XmxSetConstraints(win->source_url_text, 
			XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
			XmATTACH_FORM, NULL, NULL, label, NULL);
		XmxSetOffsets(win->source_date_text, 10, 0, 5, 10);   
		XmxSetConstraints(win->source_date_text, 
			XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
			win->source_url_text, NULL, dlabel, NULL);

		XmxSetConstraints(XtParent (win->source_text), 
			XmATTACH_WIDGET, XmATTACH_WIDGET, 
			XmATTACH_FORM, XmATTACH_FORM, win->source_date_text, dialog_sep, 
			NULL, NULL);
		XmxSetArg (XmNtopOffset, 10);
		XmxSetConstraints(dialog_sep, XmATTACH_NONE, XmATTACH_WIDGET, 
			XmATTACH_FORM, XmATTACH_FORM,
			NULL, buttons_form, NULL, NULL);
		XmxSetConstraints(buttons_form, XmATTACH_NONE, XmATTACH_FORM, 
			XmATTACH_FORM, XmATTACH_FORM,
		NULL, NULL, NULL, NULL);
	}
  
	XmxManageRemanage (win->source_win);
	if (win->current_node) {
		XmxTextSetString (win->source_text, win->current_node->text);
		XmxTextSetString (win->source_url_text, win->current_node->url);
		XmxTextSetString(win->source_date_text,
			(win->current_node->last_modified?win->current_node->last_modified:"Unknown")); 
	}
	return mo_succeed;
}

mo_status mo_search_window(mo_window *win,char *str, int backward, int caseless,
	int news)
{
	int rc;

/* search the first hit every time if by cci application */ 
	if (cci_docommand|| news) {
		((ElementRef *)win->search_start)->id = 0;
	}

	if (!backward) {
/* Either win->search_start->id is 0, in which case the search
 * should start from the beginning, or it's non-0, in which case
 * at least one search step has already been taken.
 * If the latter, it should be incremented so as to start
 * the search after the last hit.  Right? */

		if (((ElementRef *)win->search_start)->id) {
			((ElementRef *)win->search_start)->id =
				((ElementRef *)win->search_end)->id;
			((ElementRef *)win->search_start)->pos =
				((ElementRef *)win->search_end)->pos;
		}
	}

	if (news) {                        
		rc=HTMLSearchNews(win->scrolled_win,
			(ElementRef *)win->search_start,
			(ElementRef *)win->search_end); 
	} else {
		rc = HTMLSearchText(win->scrolled_win, str, 
			(ElementRef *)win->search_start,
			(ElementRef *)win->search_end,  
			backward, caseless);
	}

	if (rc == -1) {
		if (cci_docommand) {             
			if (news) {                  
				((ElementRef *)win->search_start)->id = 0;
			}                            
			return mo_fail;               
		} else {
/* No match was found. */
			if (!news) {                  
				if (((ElementRef *)win->search_start)->id)
					application_user_info_wait("Sorry, no more matches in this document.");
				else
					application_user_info_wait("Sorry, no matches in this document.");
			}
			if (news) {                   
				((ElementRef *)win->search_start)->id = 0;
			}                            
			return mo_fail;
		}
	} else {
/* Now search_start and search_end are starting and ending points of the match. */
		HTMLGotoId(win->scrolled_win,
			((ElementRef *)win->search_start)->id,(news?(-1):0));

/* Set the selection. */
		if (!news) {                  
			HTMLSetSelection (win->scrolled_win, (ElementRef *)win->search_start,
				(ElementRef *)win->search_end);
		} 
	} /* found a target */
	if (news) {                   
		((ElementRef *)win->search_start)->id = 0;
	}
	return mo_succeed;
}

/* ----------------------- mo_post_search_window ------------------------ */

static XmxCallback (search_win_cb0)
{
	mo_window *win = (mo_window*)client_data;
	char *str = XmxTextGetString (win->search_win_text);

	if (str && *str) {
		int backward=XmToggleButtonGetState(win->search_backwards_toggle);
		int caseless =XmToggleButtonGetState(win->search_caseless_toggle);
		mo_search_window(win,str, backward, caseless,0);
	}
}

static XmxCallback (search_win_cb1) /* reset */
{
	mo_window *win = (mo_window*)client_data;

			/* Clear out the search text. */
	XmxTextSetString (win->search_win_text, "");
			/* Subsequent searches start at the beginning. */
	((ElementRef *)win->search_start)->id = 0;
			/* Reposition document at top of screen. */
	HTMLGotoId(win->scrolled_win, 0,0);
}

static XmxCallback (search_win_cb2) /* dismiss */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->search_win);
}
static XmxCallback (search_win_cb3)  /* help */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window(win,
		mo_assemble_help_url("docview-menubar-file.html"),
		NULL, NULL);
}

mo_status mo_post_search_window (mo_window *win)
{
	if (!win->search_win) {
		Widget dialog_frame;
		Widget dialog_sep, buttons_form;
		Widget search_form, label;
      
/* Create it for the first time. */
		win->search_win = XmxMakeFormDialog (win->base, 
			"Mosaic: Find In Document" );
		dialog_frame = XmxMakeFrame (win->search_win, XmxShadowOut);

/* Constraints for base. */
		XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
/* Main form. */
		search_form = XmxMakeForm (dialog_frame);
      
		label = XmxMakeLabel (search_form, "Find string in document: " );
		XmxSetArg (XmNcolumns, 25);
		win->search_win_text = XmxMakeText (search_form);
		XmxAddCallbackToText(win->search_win_text, search_win_cb0, (XtPointer)win);

		win->search_caseless_toggle = XmxMakeToggleButton(search_form,
			"Caseless Search" , NULL, (XtPointer)win);
		XmxSetToggleButton (win->search_caseless_toggle, XmxSet);
		win->search_backwards_toggle = XmxMakeToggleButton(search_form,
			"Backwards Search" , NULL, (XtPointer)win);

		dialog_sep = XmxMakeHorizontalSeparator (search_form);
		buttons_form = XmxMakeFormAndFourButtons (search_form, 
			"Find" , "Reset" , "Dismiss" , "Help..." ,
			search_win_cb0, search_win_cb1, search_win_cb2, search_win_cb3,
			(XtPointer)win);

/* Constraints for search_form. */
		XmxSetOffsets (label, 13, 0, 10, 0);
/* Label attaches top to form, bottom to nothing,left to form, right to nothing.*/
		XmxSetConstraints (label, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
			XmxSetOffsets (win->search_win_text, 10, 0, 5, 8);
/* search_win_text attaches top to form, bottom to nothing, left to label, right to form. */
		XmxSetConstraints (win->search_win_text, XmATTACH_FORM,
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
			NULL, NULL, label, NULL);

/* search_caseless_toggle attaches top to search_win_text, bottom to nothing,
 * left to position, right to position. */
		XmxSetConstraints (win->search_caseless_toggle, XmATTACH_WIDGET,
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_NONE,
			win->search_win_text, NULL, label, NULL);
		XmxSetOffsets (win->search_caseless_toggle, 8, 0, 2, 0);

/* search_backwards_toggle attaches top to search_caseless_toggle,
 * bottom to nothing, left to position, right to position. */
		XmxSetConstraints (win->search_backwards_toggle, XmATTACH_WIDGET,
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_NONE,
		win->search_caseless_toggle, NULL, label, NULL);
		XmxSetOffsets (win->search_backwards_toggle, 8, 0, 2, 0);

		XmxSetOffsets (dialog_sep, 8, 0, 0, 0);
/* dialog_sep attaches top to search_backwards_toggle,
 * bottom to buttons_form, left to form, right to form */
		XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET,
			XmATTACH_FORM,XmATTACH_FORM, win->search_backwards_toggle,
			buttons_form, NULL, NULL);
		XmxSetConstraints (buttons_form, XmATTACH_NONE, XmATTACH_FORM,
			XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
	}
	XmxManageRemanage (win->search_win);
	return mo_succeed;
}

/*------------------------------------------------------------*/

void mo_done_editing(EditFile *e, int pid)
{
	char *url;

	/****** Check to see if e->win still exists */

#ifndef DISABLE_TRACE
	if (srcTrace) {
		fprintf(stderr,"Done Editing: pid = %d, file %s, url=%s\n",
			pid,e->fileName,e->url);
	}
#endif

  	url = mo_url_canonicalize_local(e->fileName);
/*	url = mo_url_canonicalize_keep_anchor(e->fileName,e->url);*/
	if (url[strlen(url)-1] == '/') {
		url[strlen(url)-1] = '\0';
	}

	mo_load_window_text(e->win, url, e->url);

	/* should I stay or should I go...du du dun da dun da da */
	/*unlink(e->fileName); */

	free(e->fileName);
	free(e->url);
	free(e);
}

mo_status mo_source_date(mo_window *win)
{
	char msg[200];

	if (win->current_node->last_modified) {
		sprintf(msg,"Source Last Modified Date:\n  %s\n" ,win->current_node->last_modified);
	} else {
		sprintf(msg,"Source Last Modified Date is not available.\n" );
	}
	application_user_info_wait(msg);
	return(mo_succeed);
}

mo_status mo_edit_source(mo_window *win)
{
	char *sourceFileName;
	FILE *fp;
	int length;
	char *editorName;
	char execString[1024];
	char editorTitle[1024];
	char editorCommand[1024];
	char *execArg[20];
	int  argCount;
	int  pid;
	EditFile *e;
	char *edit_command;

	if (!win->current_node) {
		return mo_fail;
	}
	if (!win->current_node->text) {
		return mo_fail;
	}

/* get editor */
	edit_command = get_pref_string(eEDIT_COMMAND);
	if ((edit_command) && (strlen(edit_command))) {
		editorName = edit_command;
	} else {
		editorName = getenv("EDITOR");
		if ((!editorName) || (!strlen(editorName))) {
/*
			XmxMakeErrorDialog (win->save_win,
				"Unable to get EDITOR environment variable.", 
				"Edit Source Error");
			return mo_fail;
*/
			editorName="vi"; /* default to vi */
		}
	}
/* write out source to tmp file with .html extension */
	sourceFileName = (char*)malloc(1024);
	strcpy(sourceFileName, mo_tmpnam(NULL));
	strcat(sourceFileName, ".html");

	if (!(fp = fopen(sourceFileName,"w"))) {
		char *buf, *final, tmpbuf[80];
		int final_len;

		buf=my_strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Uknown Error" );
			buf=tmpbuf;
		}

		final_len=30+
			((!sourceFileName || !*sourceFileName?3:strlen(sourceFileName))+13)
			+15+(strlen(buf)+3);
		final=(char *)calloc(final_len,sizeof(char));

		sprintf(final,"\nUnable to Open Editor Temp File:\n   %s\n\nOpen Error:\n   %s\n" ,(!sourceFileName || !*sourceFileName?" ":sourceFileName),buf);

		XmxMakeErrorDialog (win->save_win, final, "Edit Source Error" );
		if (final) {
			free(final);
			final=NULL;
		}
		return mo_fail;
	}
	length = strlen(win->current_node->text);
	if (length != fwrite(win->current_node->text,sizeof(char),length,fp)) {
		char *buf, *final, tmpbuf[80];
		int final_len;

		fclose(fp);

		buf=my_strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Uknown Error" );
			buf=tmpbuf;
		}
		final_len=30+((!sourceFileName || !*sourceFileName?3:strlen(sourceFileName))+13)+15+(strlen(buf)+3);
		final=(char *)calloc(final_len,sizeof(char));

		sprintf(final,"\nUnable to Write Editor Temp File:\n   %s\n\nWrite Error:\n   %s\n" ,(!sourceFileName || !*sourceFileName?" ":sourceFileName),buf);

		XmxMakeErrorDialog (win->save_win, final, "Edit Write Error" );
		if (final) {
			free(final);
			final=NULL;
		}
		return mo_fail;
	}
	fclose(fp);

	sprintf(editorCommand,"%s %s",editorName,sourceFileName);
	sprintf(editorTitle,"(NCSA Mosaic) Editing Copy of: %s",
		win->current_node->url);

	argCount=0;
	if (get_pref_boolean(eEDIT_COMMAND_USE_XTERM)) {
		sprintf(execString,"%s -T %s -e %s",
			get_pref_string(eXTERM_COMMAND),
			editorTitle, 
			editorCommand);

		execArg[argCount++] = get_pref_string(eXTERM_COMMAND);
		execArg[argCount++] = "-T";
		execArg[argCount++] = editorTitle;
		execArg[argCount++] = "-e";
	} else {
		sprintf(execString,"%s %s\n",editorName,sourceFileName);
	}
	execArg[argCount++] = editorName; /* problem if there are spaces 
					in this edit command....will have 
					to parse and break up */
	execArg[argCount++] = sourceFileName;
	execArg[argCount++] = NULL;

#ifdef __sgi
	pid = fork();
#else
	pid = vfork();
#endif
	if (!pid) {
/* I'm the child */
		execvp(execArg[0], execArg); 
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"Couldn't execute:\n%s\n",execString);
		}
#endif
		_exit(-1); /*don't use regular exit() or mom's I/O channels 
				will close */
	}

/* need to save file name and pid for later reading of source*/
	if (!(e = (EditFile *) malloc(sizeof(EditFile)))) {

#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"Out of Memory!\n");
		}
#endif
		return mo_fail;
	}
	e->fileName = sourceFileName;
	e->url = strdup(win->current_node->url);
	e->win = win;
	AddChildProcessHandler(pid, mo_done_editing, e);
	return mo_succeed;
} 

/*---------------------------Utility Functions-----------------------------*/

/* DA FORMAT:
 *	"src" is the pathname to check for ~ expansion. If a tilda is the
 *	first character, I expand it, store it in "dest", and return a 1.
 *	If the frist character is not a tilda, I return a 0. If "dest" does
 *	not exist, I return a -1.
 *
 * DA RULES:
 *	1) If the tilda is alone, expand it.
 *	Ex: '~'
 *	2) If the tilda is first, followed by an alphanumeric,
 *		stick the "path" to the home directory in front of
 *		it.
 *	Ex: '~spowers'
 *	3) Otherwise, leave it alone.
 *
 * DA FORMULA:
 *	1) If there is a HOME variable, use it.
 *	2) If there is a password entry, use the dir from it.
 *	3) Else...use /tmp.
 *
 */
int pathEval(char *dest, char *src) 
{
	int i;
	char *sptr, *hptr, home[__MAX_HOME_LEN__];
	struct passwd *pwdent;

	/*
	 * There is no place to store the result...punt.
	 */
	if (!dest) {
#ifndef DISABLE_TRACE
		if (srcTrace) {
			fprintf(stderr,"No place to put the Evaluated Path!\n");
		}
#endif
		return(-1);
	}

	/*
	 * There's nothing to expand
	 */
	if (!src || !*src) {
		*dest='\0';
		return(0);
	}
	if (*src!='~') {
		strcpy(dest,src);
		return(0);
	}

	/*
	 * Once here, we are gonna need to know what the expansion is...
	 *
	 * Try the HOME environment variable, then the password file, and
	 *   finally give up and use /tmp.
	 */
	if (!(hptr=getenv("HOME"))) {
		if (!(pwdent=getpwuid(getuid()))) {
			strcpy(home,"/tmp");
		} else {
			strcpy(home,pwdent->pw_dir);
		}
	} else {
		strcpy(home,hptr);
	}

	sptr=src;
	sptr++;
	/*
	 * Nothing after the tilda, just give dest a value and return...
	 */
	if (!sptr || !*sptr) {
		strcpy(dest,home);
		return(1);
	}

	/*
	 * The next character is a slash...so prepend home to the rest of
	 *   src and return.
	 */
	if (*sptr=='/') {
		strcpy(dest,home);
		strcat(dest,sptr);
		return(1);
	}

	/*
	 * Make the assumption that they want whatever comes after to be
	 *   appended to the "HOME" path, sans the last directory (e.g.
	 *   HOME=/opt/home/spowers, we would use /opt/home<REST OF "src">)
	 */
	/*
	 * Search backwards through home for a "/" on the conditions that
	 *   this is not the slash that could possibly be at the _very_ end
	 *   of home, home[i] is not a slash, and i is >= 0.
	 *
	 * If a slash is not found (i<0), then we assume that HOME is a
	 *   directory off of the root directory, or something strange like
	 *   that...so we simply ignore "home" and return the src without
	 *   the ~.
	 *
	 * If we do find a slash, we set the position of the slash + 1 to
	 *   NULL and store that in dest, then cat the rest of src onto
	 *   dest and return.
	 */
	for (i=strlen(home); (i>=0 && home[i]!='/') || i==strlen(home); i--);
	if (i<0) {
		strcpy(dest,sptr);
	} else {
		home[i+1]='\0';
		strcpy(dest,home);
		strcat(dest,sptr);
	}
	return(1);
}
