/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Interface for mailto: URLs, stolen from techsupport.c */

#include "../libhtmlw/HTML.h"
#include "mosaic.h"

#include "../libnut/url-utils.h"
#include "gui-dialogs.h"

extern int do_post;
extern char *post_content_type;
extern char *post_data;
extern char pre_title[80];

static mo_status mo_post_mailto_form_win (mo_window *win,char *to_address, char *subject);
mo_status mo_send_mailto_message (char *text, char *to, char *subj, 
				  char *content_type, char *url);
void do_mailto_post(mo_window *win, char *to, char *from, char *subject, 
		    char *body);

/* ----------------------- mo_post_mailto_window ------------------------ */
static XmxCallback (include_fsb_cb)
{
  char *fname, efname[MO_LINE_LENGTH];
  FILE *fp;
  char line[MO_LINE_LENGTH], *status;

mo_window *win = (mo_window*)client_data;

  if (!win)
    return;

  XtUnmanageChild (win->mail_fsb_win);
  fname = (char *)malloc (128 * sizeof (char));
  
  XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
                   XmSTRING_DEFAULT_CHARSET,
                   &fname);

  pathEval (efname, fname);

  fp = fopen (efname, "r");
  if (!fp) {
        char *buf, *final, tmpbuf[80];
	int final_len;

        buf=my_strerror(errno);
        if (!buf || !*buf || !strcmp(buf,"Error 0")) {
                sprintf(tmpbuf,"Unknown Error");
                buf=tmpbuf;
	}

	final_len=30+((!efname || !*efname?3:strlen(efname))+13)+15+(strlen(buf)+13);
        final=(char *)calloc(final_len,sizeof(char));

        strcpy(final,"\nUnable to Open File:\n");
        sprintf(final+strlen(final),"   %s\n",(!efname || !*efname?" ":efname));
        sprintf(final+strlen(final),"\nOpen Error:\n");
        sprintf(final+strlen(final),"   %s\n",buf);

	XmxMakeErrorDialog (win->mailto_win, 
                          final, 
                          "Open Error");
	XtManageChild (Xmx_w);

	if (final) {
		free(final);
		final=NULL;
	}
      return;
    }
  
  while (1)
    {
      long pos;
      status = fgets (line, MO_LINE_LENGTH, fp);
      if (!status || !(*line))
        break;
      
      XmTextInsert (win->mailto_text,
                    pos = XmTextGetInsertionPosition (win->mailto_text),
                    line);
      /* move insertion position to past this line to avoid inserting the
         lines in reverse order */
      XmTextSetInsertionPosition (win->mailto_text, pos + strlen(line));
    }

  return;
}

static XmxCallback (mailto_win_cb0)		/* send */
{
	mo_window *win = (mo_window*)client_data;
	char *msg, *subj, *to;
      
	XtUnmanageChild (win->mailto_win);
	msg = XmTextGetString (win->mailto_text);
	if (!msg)
		return;
	if (msg[0] == '\0')
		return;
	to = XmTextGetString (win->mailto_tofield);
	subj = XmTextGetString (win->mailto_subfield);
	mo_send_mailto_message(msg,to,subj,"text/plain",win->current_node->url);
	free (msg);
	free (to);
	free (subj);
}
static XmxCallback (mailto_win_cb1)		/* dismiss */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->mailto_win); 
}
static XmxCallback (mailto_win_cb2)		/* help */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-mailto.html"),
		NULL, NULL);
}
static XmxCallback (mailto_win_cb3)		/* insert file */
{
	mo_window *win = (mo_window*)client_data;
      
	if (!win->mail_fsb_win) {
		win->mail_fsb_win = XmxMakeFileSBDialog (win->mailto_win,
			"NCSA Mosaic: Include File In Mail Message",
			"Name of file to include:",
			include_fsb_cb, (XtPointer)win);
	} else {
		XmFileSelectionDoSearch (win->mail_fsb_win, NULL);
	}
	XmxManageRemanage (win->mail_fsb_win);
}

static XmxCallback (mailto_win_cb4)		/* insert url */
{
	mo_window *win = (mo_window*)client_data;
	long pos;

        if(win->current_node->url){   
            XmTextInsert (win->mailto_text,
                          pos = XmTextGetInsertionPosition (win->mailto_text),
                          win->current_node->url);
                /* move insertion position to past this line
                   to avoid inserting the lines in reverse order */
            XmTextSetInsertionPosition (win->mailto_text,
                                        pos + strlen(win->current_node->url));
        }  
}


static XmxCallback (mailto_form_win_cb0) 		/* send */
{
	mo_window *win =  (mo_window*)client_data;
	char *subj, *to, *namestr;

	XtUnmanageChild (win->mailto_form_win);
	to = XmTextGetString (win->mailto_form_tofield);
	subj = XmTextGetString (win->mailto_form_subfield);
	namestr = XmTextGetString(win->mailto_form_fromfield);
	do_mailto_post(win,to,namestr,subj,win->post_data);
	free (namestr);
	free (to);
	free (subj);
	if (win->post_data) {
		free (win->post_data);
		win->post_data=NULL;
	}
}

static XmxCallback (mailto_form_win_cb1)		/* dismiss */
{
	mo_window *win =  (mo_window*)client_data;

	XtUnmanageChild (win->mailto_form_win); 
	if (win->post_data) {
		free (win->post_data);
		win->post_data=NULL;
	}
}

static XmxCallback (mailto_form_win_cb2)		/* help */
{
	mo_window *win =  (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-mailto-form.html"),
		NULL, NULL);
}

mo_status mo_post_mailto_win (mo_window *win, char *to_address, char *subject)
{
  FILE *fp;
  long pos;
  char namestr[1024],tmp[1024];

  if (do_post) {
	if (!subject || !*subject) {
		char str[BUFSIZ];

		sprintf(str,"Form Result(s) Posted from %s",pre_title);
		return(mo_post_mailto_form_win(win,to_address,str));
	} else {
		return(mo_post_mailto_form_win(win,to_address,subject));
	}
  }

  if (!win->mailto_win) {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget mailto_form;
      Widget tolabel, sublabel, fromlabel;
      
      /* Create it for the first time. */
      win->mailto_win = XmxMakeFormDialog (win->base, 
		"NCSA Mosaic: Mail To Author");
      dialog_frame = XmxMakeFrame (win->mailto_win, XmxShadowOut);
      
      /* Constraints for base. */
      XmxSetConstraints 
        (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      mailto_form = XmxMakeForm (dialog_frame);
      
      XmxSetArg (XmNscrolledWindowMarginWidth, 10);
      XmxSetArg (XmNscrolledWindowMarginHeight, 10);
      XmxSetArg (XmNcursorPositionVisible, True);
      XmxSetArg (XmNeditable, True);
      XmxSetArg (XmNeditMode, XmMULTI_LINE_EDIT);
      XmxSetArg (XmNrows, 15);
      XmxSetArg (XmNcolumns, 80);
      /* XmxSetArg (XmNwordWrap, True); */
      /* XmxSetArg (XmNscrollHorizontal, False); */
      win->mailto_text = XmxMakeScrolledText (mailto_form);
      
      dialog_sep = XmxMakeHorizontalSeparator (mailto_form);

      /* create from, to, and subject widgets */
      fromlabel = XmxMakeLabel(mailto_form, "From:");
      XmxSetArg (XmNeditable, False); /* for now, at least */
      win->mailto_fromfield = XmxMakeTextField(mailto_form);

      tolabel = XmxMakeLabel(mailto_form, "To:");
      win->mailto_tofield = XmxMakeTextField(mailto_form);

      sublabel = XmxMakeLabel(mailto_form, "Subject:");
      win->mailto_subfield = XmxMakeTextField(mailto_form);

      /* constraints for FROM */
      XmxSetOffsets(fromlabel, 14, 10, 10, 10);
      XmxSetConstraints
	(fromlabel, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, 
	 XmATTACH_NONE, NULL, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_fromfield, 10, 10, 10, 10);
      XmxSetConstraints
	(win->mailto_fromfield, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
	 XmATTACH_FORM, NULL, NULL, fromlabel, NULL);

      /* constraints for TO */
      XmxSetOffsets(tolabel, 14, 10, 10, 10);
      XmxSetConstraints
	(tolabel, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
	 win->mailto_fromfield, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_tofield, 10, 10, 10, 10);
      XmxSetConstraints
	(win->mailto_tofield, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, 
	 XmATTACH_FORM, win->mailto_fromfield, NULL, tolabel, NULL);

      /* constraints for SUBJECT */
      XmxSetOffsets(sublabel, 14, 10, 10, 10);
      XmxSetConstraints
	(sublabel, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
	 XmATTACH_NONE, win->mailto_tofield, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_subfield, 10, 10, 10, 10);
      XmxSetConstraints (win->mailto_subfield, XmATTACH_WIDGET, XmATTACH_NONE, 
	 XmATTACH_WIDGET, XmATTACH_FORM, win->mailto_tofield, NULL, 
	 sublabel, NULL);

      /* create buttons */
      buttons_form = XmxMakeFormAndFiveButtons (mailto_form, 
		"Send", "Insert File", "Insert URL", "Dismiss", "Help...",
         	mailto_win_cb0, mailto_win_cb3, 
		mailto_win_cb4, mailto_win_cb1, mailto_win_cb2,
		(XtPointer)win);

      XmxSetOffsets (XtParent (win->mailto_text), 3, 0, 3, 3);
      XmxSetConstraints
        (XtParent (win->mailto_text), XmATTACH_WIDGET, XmATTACH_WIDGET, 
         XmATTACH_FORM, XmATTACH_FORM,
         win->mailto_subfield, dialog_sep, NULL, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints 
        (dialog_sep, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
	 XmATTACH_FORM, NULL, NULL, NULL, NULL);
    }

  /* fill in text fields */

  sprintf(namestr, "%s <%s>", mMosaicAppData.author_full_name,
          mMosaicAppData.author_email);
  
  XmxTextSetString (win->mailto_fromfield, namestr);
  XmxTextSetString (win->mailto_tofield, to_address);
  if (!subject || !*subject) {
	char str[BUFSIZ];

	sprintf(str,"Mail from %s",pre_title);
  	XmTextFieldSetString(win->mailto_subfield,str);
  }
  else {
  	XmTextFieldSetString(win->mailto_subfield,subject);
  }

  XmxTextSetString (win->mailto_text, "");
  
      /* tack signature on the end if it exists - code from Martin Hamilton */
  if (mMosaicAppData.signature) {
      XmxTextSetString (win->mailto_text, "\n\n");
          /* leave a gap... */
      XmTextSetInsertionPosition (win->mailto_text, 2);
      if ((fp = fopen(mMosaicAppData.signature, "r")) != NULL) {
          while(fgets(tmp, sizeof(tmp) - 1, fp)) {
              XmTextInsert(win->mailto_text,
                           pos = XmTextGetInsertionPosition (win->mailto_text),
                           tmp);
              XmTextSetInsertionPosition (win->mailto_text, pos + strlen(tmp));
          }
          fclose(fp);
      } else {
          XmxTextSetString (win->mailto_text, "");
      }
      
  }
  XmTextSetInsertionPosition (win->mailto_text, 0);
      
  XmxManageRemanage (win->mailto_win);

  return mo_succeed;
}


void eatSpace(char *s) {

char *p,*p0;

	for (p=s; *s; s++) {
		if (isspace(*s)) {
			*s=' '; /*Get rid of anything like TAB*/
			p=(++s);
			while (*s && isspace(*s)) s++;
			if (!*s) {
				*p='\0';

				return;
			}
			if (s>p) {
				for (p0=p;*s;s++,p++) *p=(*s);
				*p='\0';
				s=p0;
			}
		}
	}

	return;
}


char *makeReadable(char *str, int condense) {

char *buf=NULL,*name=NULL,*val=NULL;
char *b=NULL;

	b=strdup(str);

	/*decode the encoded info (str) into name-value pairs*/
	buf=(char *)calloc((strlen(b)*3),sizeof(char));
	if (!buf) {
		return(NULL);
	}
	sprintf(buf,"\n");
	for(; (b && *b) ;) {
		val = makeword(b,'&');
		plustospace(val);
		unescape_url(val);
		name = makeword(val,'=');
		if (condense) { /*take out all "multiple isspace()"*/
			eatSpace(val);
		}
		sprintf(buf,"%sNAME=[%s]\nVALUE=[%s]\n\n",(buf && *buf?buf:"\n"),(name && *name?name:"(NULL)"),(val && *val?val:"(NULL)"));
		if (val) {
			free(val);
			val=NULL;
		}
		if (name) {
			free(name);
			name=NULL;
		}
	}

	free(b);

	return(buf);
}


static mo_status mo_post_mailto_form_win (mo_window *win,char *to_address, char *subject)
{
  char namestr[1024],*buf=NULL;

  if (!do_post)
	return(mo_fail);

  if (!win->mailto_form_win) {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget mailto_form_form;
      Widget tolabel, sublabel, fromlabel;
      
      /* Create it for the first time. */
      win->mailto_form_win = XmxMakeFormDialog (win->base, 
			"NCSA Mosaic: Mail Form Results To Author");
      dialog_frame = XmxMakeFrame (win->mailto_form_win, XmxShadowOut);
      
      /* Constraints for base. */
      XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      mailto_form_form = XmxMakeForm (dialog_frame);
      
      XmxSetArg (XmNscrolledWindowMarginWidth, 10);
      XmxSetArg (XmNscrolledWindowMarginHeight, 10);
      XmxSetArg (XmNcursorPositionVisible, True);
      XmxSetArg (XmNeditable, False);
      XmxSetArg (XmNeditMode, XmMULTI_LINE_EDIT);
      XmxSetArg (XmNrows, 15);
      XmxSetArg (XmNcolumns, 80);
      XmxSetArg (XmNwordWrap, True);
      XmxSetArg (XmNscrollHorizontal, False);
      win->mailto_form_text = XmxMakeScrolledText (mailto_form_form);
      
      dialog_sep = XmxMakeHorizontalSeparator (mailto_form_form);

      /* create from, to, and subject widgets */
      fromlabel = XmxMakeLabel(mailto_form_form, "From:");
      XmxSetArg (XmNeditable, False);
      win->mailto_form_fromfield = XmxMakeTextField(mailto_form_form);

      tolabel = XmxMakeLabel(mailto_form_form, "To:");
      XmxSetArg (XmNeditable, False);
      win->mailto_form_tofield = XmxMakeTextField(mailto_form_form);

      sublabel = XmxMakeLabel(mailto_form_form, "Subject:");
      XmxSetArg (XmNeditable, False);
      win->mailto_form_subfield = XmxMakeTextField(mailto_form_form);

      /* constraints for FROM */
      XmxSetOffsets(fromlabel, 14, 10, 10, 10);
      XmxSetConstraints
	(fromlabel, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, 
	 XmATTACH_NONE, NULL, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_form_fromfield, 10, 10, 10, 10);
      XmxSetConstraints
	(win->mailto_form_fromfield, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
	 XmATTACH_FORM, NULL, NULL, fromlabel, NULL);

      /* constraints for TO */
      XmxSetOffsets(tolabel, 14, 10, 10, 10);
      XmxSetConstraints
	(tolabel, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
	 win->mailto_form_fromfield, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_form_tofield, 10, 10, 10, 10);
      XmxSetConstraints
	(win->mailto_form_tofield, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, 
	 XmATTACH_FORM, win->mailto_form_fromfield, NULL, tolabel, NULL);

      /* constraints for SUBJECT */
      XmxSetOffsets(sublabel, 14, 10, 10, 10);
      XmxSetConstraints
	(sublabel, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
	 XmATTACH_NONE, win->mailto_form_tofield, NULL, NULL, NULL);
      XmxSetOffsets(win->mailto_form_subfield, 10, 10, 10, 10);
      XmxSetConstraints
	(win->mailto_form_subfield, XmATTACH_WIDGET, XmATTACH_NONE, 
	 XmATTACH_WIDGET, XmATTACH_FORM, win->mailto_form_tofield, NULL, 
	 sublabel, NULL);

      /* create buttons */
      buttons_form = XmxMakeFormAndThreeButtons(mailto_form_form, 
		"Send", "Dismiss", "Help...", 
		mailto_form_win_cb0, mailto_form_win_cb1, 
		mailto_form_win_cb2,(XtPointer)win);

      XmxSetOffsets (XtParent (win->mailto_form_text), 3, 0, 3, 3);
      XmxSetConstraints
        (XtParent (win->mailto_form_text), XmATTACH_WIDGET, XmATTACH_WIDGET, 
         XmATTACH_FORM, XmATTACH_FORM,
         win->mailto_form_subfield, dialog_sep, NULL, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints 
        (dialog_sep, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
	 XmATTACH_FORM, NULL, NULL, NULL, NULL);
    }

  /* fill in text fields */

  sprintf(namestr, "%s <%s>", mMosaicAppData.author_full_name,
          mMosaicAppData.author_email);
  
  XmxTextSetString (win->mailto_form_fromfield, namestr);
  XmxTextSetString (win->mailto_form_tofield, to_address);
  if (subject != NULL)
  	XmTextFieldSetString(win->mailto_form_subfield,subject);
  else
  	XmTextFieldSetString(win->mailto_form_subfield,"");

  win->post_data=strdup(post_data);

  buf=makeReadable(post_data,1);
  XmTextSetString (win->mailto_form_text, buf);
  if (buf)
	free(buf);
  XmTextSetInsertionPosition (win->mailto_form_text, 0);
  XmxManageRemanage (win->mailto_form_win);
  return mo_succeed;
}

/* SWP -- 11.15.95 -- ACTION=mailto support for Forms */
void do_mailto_post(mo_window *win, char *to, char *from, char *subject, char *body) {

	if (!win || !win->current_node || !win->current_node->url ||
	    !*(win->current_node->url) || !to || !*to || !from || !*from ||
	    !body || !*body || !subject || !*subject) {
		return;
	}

	if (!strcmp(post_content_type,"text/plain")) {
		char *buf=NULL;

		buf=makeReadable(body,0);
		if (mMosaicSrcTrace) {
			fprintf(stderr,"To: [%s]\nFrom: [%s]\nSubj: [%s]\nBody: [%s]\n",to,from,subject,buf);
		}
		mo_send_mailto_message(buf, to, subject, post_content_type, 
				       win->current_node->url);
		if (buf)
			free(buf);
	} else {
		if (mMosaicSrcTrace) {
			fprintf(stderr,"To: [%s]\nFrom: [%s]\nSubj: [%s]\nBody: [%s]\n",to,from,subject,body);
		}

		mo_send_mailto_message(body, to, subject, post_content_type, 
				       win->current_node->url);
	}
}

/* these are not currently used.  We just use the functions in techsupport.c */
/* ------------------------------------------------------------------------ */

static FILE *_fp = NULL;

FILE *mo_start_sending_mailto_message (char *to, char *subj, 
	char *content_type, char *url)
{
  char cmd[2048];

  if (!to)
    return NULL;
  
  if (mMosaicAppData.mail_filter_command) {
      sprintf (cmd, "%s | %s", mMosaicAppData.mail_filter_command, 
               mMosaicAppData.sendmail_command);
    } else {
      sprintf (cmd, "%s", mMosaicAppData.sendmail_command);
    }

  if ((_fp = popen (cmd, "w")) == NULL)
    return NULL;

  fprintf (_fp, "To: %s\n", to);
  fprintf (_fp, "Subject: %s\n", subj);
  fprintf (_fp, "Content-Type: %s\n", content_type);
  fprintf (_fp, "Mime-Version: 1.0\n");
  fprintf (_fp, "X-Mailer: NCSA Mosaic %s on %s\n", 
           MO_VERSION_STRING, MO_MACHINE_TYPE);
  if (url)
    fprintf (_fp, "X-URL: %s\n", url);

  fprintf (_fp, "\n");
  
  /* Stick in BASE tag as appropriate. */
  if (url && content_type && strcmp (content_type, "text/x-html") == 0)
    fprintf (_fp, "<base href=\"%s\">\n", url);
  return _fp;
}

mo_status mo_finish_sending_mailto_message (void)
{
	if (_fp)
		pclose (_fp);
	_fp = NULL;
	return mo_succeed;
}

mo_status mo_send_mailto_message (char *text, char *to, char *subj, 
	char *content_type, char *url)
{
	FILE *fp;

	fp = mo_start_sending_mailto_message (to, subj, content_type, url);
	if (!fp)
		return mo_fail;
	fputs (text, fp);
	mo_finish_sending_mailto_message ();
	return mo_succeed;
}
