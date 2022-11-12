/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <Xm/LabelG.h>
#include <Xm/PushB.h>
#include <Xm/ScrolledW.h>
#include <Xm/List.h>
#include <Xm/ToggleB.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>

#include <Xm/Protocols.h>

#include <pwd.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"

#include "../libwww2/HText.h"
#include "newsrc.h"
#include "libwww2/HTNews.h"
#include "libwww2/HTAlert.h"
#include "gui.h"
#include "gui-news.h"
#include "libnut/system.h"
#include "gui-dialogs.h"
#include "gui-documents.h"

#define MAX_BUF 512

void gui_news_post_subgroupwin (mo_window *win) 
{
}

void gui_news_updateprefs (mo_window *win)
{

  if (newsShowAllGroups) {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp0, XmxSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp1, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp2, XmxNotSet);
  } else if (newsShowReadGroups) {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp0, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp1, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp2, XmxSet);
  } else {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp0, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp1, XmxSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_grp2, XmxNotSet);
  }

  if (newsShowAllArticles) {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_art0, XmxSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_art1, XmxNotSet);
  } else {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_art0, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_art1, XmxSet);
  }

  if (ConfigView) {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt0, XmxNotSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt1, XmxSet);
  } else {
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt0, XmxSet);
    XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt1, XmxNotSet);
  }
}

void gui_news_subgroup(mo_window *win)
{
	char buf[MAX_BUF+1];

	if (NewsGroup) {
		subscribegroup (NewsGroup);
		newsrc_flush ();
		sprintf (buf, "%s successfully subscribed", NewsGroup);
		mo_gui_notify_progress (buf,win);
	}
}

void gui_news_unsubgroup(mo_window *win)
{
	char buf[MAX_BUF+1];
	if (NewsGroup) {
		unsubscribegroup (NewsGroup);
		newsrc_flush ();
		sprintf (buf, "%s successfully unsubscribed", NewsGroup);
		mo_gui_notify_progress (buf,win);
	}
}

void gui_news_flush(mo_window *win)
{
	mo_gui_notify_progress ("Flushing newsrc data",win);
	newsrc_flush ();
	mo_gui_notify_progress ("",win);
}

void gui_news_flushgroup(mo_window *win)
{
	char buf[1024+1];

	if (!NewsGroupS)
		return;
	sprintf (buf, "Flushing newsrc data for %s", 
			NewsGroupS?NewsGroupS->name : "current group");
	mo_gui_notify_progress (buf,win);
  	newsrc_flush ();
	mo_gui_notify_progress (" ",win);
}

void gui_news_list(mo_window *win)
{
	mo_load_window_text(win,"news:*",NULL);
}

void gui_news_showAllGroups (mo_window *win)
{
	gui_news_flush (win);
	newsGotList = 0;
	HTSetNewsConfig (-1, -1, 1, 1, -1, -1,-1,-1);
	gui_news_updateprefs (win);
	mo_load_window_text (win, "news:*", NULL);
}

void gui_news_showGroups (mo_window *win)
{
	/* Show only subbed groups */
	HTSetNewsConfig (-1,-1,0,0,-1,-1,-1,-1);
	gui_news_updateprefs (win);
	mo_load_window_text (win, "news:*", NULL);
}

void gui_news_showReadGroups (mo_window *win)
{
	HTSetNewsConfig (-1,-1,0,1,-1,-1,-1,-1); 
	gui_news_updateprefs (win);
	mo_load_window_text (win, "news:*", NULL);
}

void gui_news_showAllArticles (mo_window *win)
{
	char buf[512+1];

	HTSetNewsConfig (-1,1,-1,-1,-1,-1,-1,-1); 
	gui_news_updateprefs (win);

	if (!NewsGroup && !NewsGroupS)
		return;
	if (NewsGroupS)
		sprintf (buf, "news:%s", NewsGroupS->name);
	else 
		sprintf (buf, "news:%s", NewsGroup);
	mo_load_window_text (win, buf, NULL);
}

void gui_news_showArticles (mo_window *win)
{
	char buf[512+1];

	HTSetNewsConfig (-1,0,-1,-1,-1,-1,-1,-1); 
	gui_news_updateprefs (win);

	if (!NewsGroup && !NewsGroupS)
		return;
	if (NewsGroup)
		sprintf (buf, "news:%s", NewsGroup);
	else 
		sprintf (buf, "news:%s", NewsGroupS->name);
	mo_load_window_text (win, buf, NULL);
}

void gui_news_markGroupRead (mo_window *win)
{
	char buf[512+1];

	if (!NewsGroupS)
		return;
	markrangeread (NewsGroupS, NewsGroupS->minart, NewsGroupS->maxart);
	sprintf (buf, "All articles in %s marked read", NewsGroupS->name);
	mo_gui_notify_progress (buf,win);

/* Return to newsgroup list */
	sprintf (buf, "news:*");
	mo_load_window_text (win, buf, NULL);
}

void gui_news_markGroupUnread (mo_window *win)
{
	char buf[512+1];

	if (!NewsGroupS)
		return;
	markrangeunread (NewsGroupS, NewsGroupS->minart, NewsGroupS->maxart);
	sprintf (buf, "All articles in %s marked unread", NewsGroupS->name);
	mo_gui_notify_progress (buf,win);
/* Return to newsgroup list */
	sprintf (buf, "news:*");
	mo_load_window_text (win, buf, NULL);
}

void gui_news_markArticleUnread (mo_window *win)
{
	char buf[512+1];

	if (!NewsGroupS || !CurrentArt)
		return;
	markunread (NewsGroupS, CurrentArt->num);
	sprintf (buf, "Article %s marked unread", CurrentArt->ID);
	mo_gui_notify_progress (buf,win);
	sprintf (buf, "news:%s", NewsGroup);
	mo_load_window_text (win, buf, NULL);
}

void gui_news_initflush (mo_window *win)
{
	newsrc_initflush ();
}

void gui_news_index(mo_window *win)
{
	char url[128];

	newsrc_flush ();
	strcpy(url,win->current_node->url);
	news_index(url);
	if(url[0]) 
		mo_load_window_text(win,url,NULL);
}

void gui_news_prev(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->url);
	news_prev(url);
	if(url[0]) mo_load_window_text(win,url,NULL);
}

void gui_news_next(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->url);
	news_next(url);
	if(url[0]) mo_load_window_text(win,url,NULL);
}

void gui_news_prevt(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->url);
	news_prevt(url);
	if(url[0]) mo_load_window_text(win,url,NULL);
}

void gui_news_nextt(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->url);
	news_nextt(url);
	if(url[0]) mo_load_window_text(win,url,NULL);
}

static XmxCallback (include_fsb_cb)
{
	char *fname, efname[MO_LINE_LENGTH];
	FILE *fp;
	char line[MO_LINE_LENGTH], *status;
	mo_window *win = (mo_window*)client_data;

	if (!win)
		return;

	XtUnmanageChild (win->news_fsb_win);
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

		sprintf(final,"\nUnable to Open Include File:\n   %s\n\nOpen Error:\n   %s\n" ,(!efname || !*efname?" ":efname),buf);

		XmxMakeErrorDialog (win->news_win, final, "News Include Error" );
		XtManageChild (Xmx_w);
		if (final) {
			free(final);
			final=NULL;
		}
		return;
	}
	while (1) {
		long pos;
		status = fgets (line, MO_LINE_LENGTH, fp);
		if (!status || !(*line))
			goto done;
		XmTextInsert (win->news_text,
			pos = XmTextGetInsertionPosition (win->news_text),
			line);
/* move insertion position to past this line to avoid inserting the
 * lines in reverse order */
		XmTextSetInsertionPosition (win->news_text, pos + strlen(line));
	}
done:
	fclose(fp);
	return;
}

static XmxCallback (include_button_cb) /* Why is this here ?*/
{
	mo_window *win = (mo_window*)client_data;
	return;
}

/* ----------------------- mo_post_news_window -------------------------- */

int NNTPpost(char *from, char *subj, char *ref, char *groups, char *msg, caddr_t appd);
int NNTPgetarthdrs(char *art,char **ref, char **grp, char **subj, char **from,
   caddr_t appd);
char *NNTPgetquoteline(char *art,caddr_t appd);

static XmxCallback (news_win_cb0)			/* POST */
{
	mo_window *win = (mo_window*)client_data;
	char *msg,*subj,*group,*from;

	XtUnmanageChild (win->news_win);
	msg = XmTextGetString (win->news_text);
	from = XmTextGetString (win->news_text_from);
	subj = XmTextGetString (win->news_text_subj);
	group = XmTextGetString (win->news_text_group);
	if (!msg)
		return;
	if (msg[0] == '\0')
		return;
	NNTPpost(from, subj, NULL, group, msg, (caddr_t) win);
	free(msg);
	free(from);
	free(group);
	free(subj);
}

static XmxCallback (news_win_cb1) 		/* DISMISS */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->news_win);
		/* since we're going to re-use this in different configs
			we'll destroy it */
	XtDestroyWidget (win->news_win);
	win->news_win = NULL;
	win->news_fsb_win = NULL;
}
static XmxCallback (news_win_cb2) 		/* HELP */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-news.html"),
		NULL, NULL);
}
static XmxCallback (news_win_cb3) 		/* INSERT FILE */
{
	mo_window *win = (mo_window*)client_data;

	if (!win->news_fsb_win) {
		win->news_fsb_win = XmxMakeFileSBDialog (win->news_win,
			"NCSA Mosaic: Include File for News" ,
			"Name of file to include:" ,
			include_fsb_cb, (XtPointer)win);
	} else {
		XmFileSelectionDoSearch (win->news_fsb_win, NULL);
	}
	XmxManageRemanage (win->news_fsb_win);
}
static XmxCallback (news_win_cb4) 		/* QUOTE */
{
	mo_window *win = (mo_window*)client_data;
}

static XmxCallback (follow_win_cb0)	 	/* POST */
{
	mo_window *win = (mo_window*)client_data;
	char *msg,*subj,*group,*from;

	XtUnmanageChild (win->news_win);
	msg = XmTextGetString (win->news_text);
	from = XmTextGetString (win->news_text_from);
	subj = XmTextGetString (win->news_text_subj);
	group = XmTextGetString (win->news_text_group);
	if (!msg)
		return;
	if (msg[0] == '\0')
		return;
	NNTPpost(from, subj, win->newsfollow_ref, group, msg, (caddr_t) win); 
	free(msg);
	free(from);
	free(group);
	free(subj);
}

static XmxCallback (follow_win_cb1)		/* DISMISS */
{
	mo_window *win = (mo_window*)client_data;

	if(win->newsfollow_ref) free(win->newsfollow_ref);    
	if(win->newsfollow_grp) free(win->newsfollow_grp);    
	if(win->newsfollow_subj) free(win->newsfollow_subj);    
	if(win->newsfollow_from) free(win->newsfollow_from);    
	if(win->newsfollow_artid) free(win->newsfollow_artid);    
	XtUnmanageChild (win->news_win);
	/* since we're going to re-use this in different configs
			we'll destroy it */
	XtDestroyWidget (win->news_win);
	win->news_win = NULL;
	win->news_fsb_win = NULL;
}
static XmxCallback (follow_win_cb2)		/* HELP */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, mo_assemble_help_url ("help-on-news.html"),
		NULL, NULL);
}
static XmxCallback (follow_win_cb3)		/* INSERT FILE */
{
	mo_window *win = (mo_window*)client_data;

	if (!win->news_fsb_win) {
		win->news_fsb_win = XmxMakeFileSBDialog (win->news_win,
			"NCSA Mosaic: Include File for News" ,
			"Name of file to include:" ,
			include_fsb_cb, (XtPointer)win);
	} else {
		XmFileSelectionDoSearch (win->news_fsb_win, NULL);
	}
	XmxManageRemanage (win->news_fsb_win);
}
static XmxCallback (follow_win_cb4)		/* QUOTE */
{
	mo_window *win = (mo_window*)client_data;
	char *line;
	int pos;

	line = (char*)malloc(strlen(win->newsfollow_from)+30);
	sprintf(line,"%s writes:\n\n",win->newsfollow_from);

	XmTextInsert(win->news_text, 
		pos = XmTextGetInsertionPosition (win->news_text), line);
/* move insertion position to past this line to avoid 
 * inserting the lines in reverse order */
	XmTextSetInsertionPosition (win->news_text, pos+strlen(line));

	if(line = NNTPgetquoteline(win->newsfollow_artid, (caddr_t)win)){
		do {
			XmTextInsert(win->news_text,
				pos = XmTextGetInsertionPosition (win->news_text),
				line);
/* move insertion position to past this line to avoid 
 * inserting the lines in reverse order */
			XmTextSetInsertionPosition (win->news_text, 
					pos+strlen(line));
		} while (line = NNTPgetquoteline(NULL, (caddr_t)win));
	}
}

mo_status mo_post_news_win (mo_window *win)
{
    return mo_post_generic_news_win(win,0); 
}

mo_status mo_post_follow_win (mo_window *win)
{
    char *s;

    if(strncmp("news:",win->current_node->url,5)) 
	return mo_fail; /* fix me ########### */
    

    NNTPgetarthdrs(&(win->current_node->url)[5], 
		   &(win->newsfollow_ref), 
		   &(win->newsfollow_grp), 
		   &(win->newsfollow_subj), 
		   &(win->newsfollow_from),
		   (caddr_t) win);
    
    /* add a re: if needed*/
    if(strncmp("Re: ",win->newsfollow_subj,4) && 
       strncmp("re: ",win->newsfollow_subj,4)){
	s = (char*)malloc(strlen(win->newsfollow_subj)+5); /* this sucks -bjs*/
	sprintf(s,"Re: %s",win->newsfollow_subj);
	free(win->newsfollow_subj);
	win->newsfollow_subj = s;
    }

    /* add this article to ref */
    win->newsfollow_artid = (char*)malloc(strlen(win->current_node->url));
    strcpy(win->newsfollow_artid, &(win->current_node->url)[5]);

    if(!win->newsfollow_ref){
	win->newsfollow_ref = (char*)malloc(strlen(win->current_node->url));
	sprintf(win->newsfollow_ref,"<%s>",&(win->current_node->url)[5]);
    } else {
	s = (char*)malloc(strlen(win->newsfollow_ref)+
			strlen(win->current_node->url)); /* this sucks -bjs*/
	sprintf(s,"%s <%s>",win->newsfollow_ref,&(win->current_node->url)[5]);
	free(win->newsfollow_ref);
	win->newsfollow_ref = s;
    }
    return mo_post_generic_news_win(win,1); 
}

mo_status mo_post_generic_news_win(mo_window *win, int follow)
{
  char namestr[1024], tmp[1024];
  Widget dialog_frame;
  Widget dialog_sep, buttons_form;
  Widget news_form, yap_label, f_label, s_label, g_label;
  FILE *fp;
  long pos;

  sprintf (namestr, "%s <%s>", 
           mMosaicAppData.author_full_name,
           mMosaicAppData.author_email);
  
  if (!win->news_win) {
      /* Create it for the first time. */
      Xmx_n = 0;
      win->news_win = XmxMakeFormDialog 
        (win->base, "NCSA Mosaic: News" );
      dialog_frame = XmxMakeFrame (win->news_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints 
        (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      news_form = XmxMakeForm (dialog_frame);
      XmxSetArg(XmNalignment, XmALIGNMENT_END);
      f_label = XmxMakeLabel (news_form, "From:" );
      XmxSetArg(XmNalignment, XmALIGNMENT_END);
      s_label = XmxMakeLabel (news_form, "Subject:" );
      XmxSetArg(XmNalignment, XmALIGNMENT_END);
      g_label = XmxMakeLabel (news_form, "Groups:" );
      if(follow)
	  yap_label = XmxMakeLabel (news_form, "Follow-up to UseNet News Article" );
      else
	  yap_label = XmxMakeLabel (news_form, "Post a UseNet News Article" );
      XmxSetArg (XmNcolumns, 65);
      win->news_text_subj = XmxMakeText (news_form);
      XmxSetArg (XmNcolumns, 65);
      win->news_text_group = XmxMakeText (news_form);
      XmxSetArg (XmNcolumns, 65);
      XmxSetArg (XmNeditable, False);
      win->news_text_from = XmxMakeText (news_form);
      XmxSetArg (XmNscrolledWindowMarginWidth, 10);
      XmxSetArg (XmNscrolledWindowMarginHeight, 10);
      XmxSetArg (XmNcursorPositionVisible, True);
      XmxSetArg (XmNeditable, True);
      XmxSetArg (XmNeditMode, XmMULTI_LINE_EDIT);
      XmxSetArg (XmNrows, 30);
      XmxSetArg (XmNcolumns, 80);
      XmxSetArg (XmNwordWrap, True); 
      XmxSetArg (XmNscrollHorizontal, False); 
      win->news_text = XmxMakeScrolledText (news_form);
      dialog_sep = XmxMakeHorizontalSeparator (news_form);
      if(follow)
	  buttons_form = XmxMakeFormAndFiveButtons (news_form, 
		"Post" , "Quote" , "Include File..." , "Dismiss" , "Help..." , 
		follow_win_cb0, follow_win_cb4, follow_win_cb3, 
		follow_win_cb1, follow_win_cb2, (XtPointer)win);
      else
	  buttons_form = XmxMakeFormAndFourButtons (news_form, 
		"Post" , "Include File..." , "Dismiss" , "Help..." , 
		news_win_cb0, news_win_cb3, news_win_cb1, news_win_cb2,
		(XtPointer)win);
      /* Constraints for news_form. */

      XmxSetOffsets (yap_label, 10, 20, 0, 0);
      XmxSetConstraints (yap_label, 
	 XmATTACH_FORM, XmATTACH_NONE ,XmATTACH_FORM, XmATTACH_FORM,
         NULL, NULL, NULL, NULL);
      XmxSetOffsets (win->news_text_from, 10, 10, 10, 10);
      XmxSetConstraints (win->news_text_from, 
	 XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
	 yap_label, NULL, f_label, NULL);

      XmxSetOffsets (f_label, 14, 10, 10, 10);
      XmxSetConstraints (f_label,
	 XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_NONE, XmATTACH_NONE,
	 yap_label, NULL, NULL, NULL);

      XmxSetOffsets (win->news_text_subj, 10, 10, 10, 10);
      XmxSetConstraints (win->news_text_subj, 
	 XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
	 win->news_text_from, NULL, s_label, NULL);

      XmxSetOffsets (s_label, 14, 10, 10, 10);
      XmxSetConstraints (s_label,
	 XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_NONE, XmATTACH_NONE,
	 win->news_text_from, NULL, NULL, NULL);

      XmxSetOffsets (win->news_text_group, 10, 10, 10, 10);
      XmxSetConstraints (win->news_text_group, 
	 XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
	 win->news_text_subj, NULL, g_label, NULL);

      XmxSetOffsets (g_label, 14, 10, 10, 10);
      XmxSetConstraints (g_label,
	 XmATTACH_WIDGET ,XmATTACH_NONE, XmATTACH_NONE, XmATTACH_NONE,
	 win->news_text_subj, NULL, NULL, NULL);

      XmxSetOffsets (XtParent (win->news_text), 10, 0, 3, 3);
      XmxSetConstraints (XtParent (win->news_text), 
	 XmATTACH_WIDGET, XmATTACH_WIDGET,XmATTACH_FORM, XmATTACH_FORM,
         win->news_text_group, dialog_sep, NULL, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints (dialog_sep, 
	 XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);

      XmxSetConstraints (buttons_form, 
	 XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM,
         NULL, NULL, NULL, NULL);

      XmxTextSetString (win->news_text, "");
      
          /* tack signature on the end if it exists - code from Martin Hamilton */
      if (mMosaicAppData.signature) {
          XmxTextSetString (win->news_text, "\n\n");
              /* leave a gap... */
          XmTextSetInsertionPosition (win->news_text, 2);
          if ((fp = fopen(mMosaicAppData.signature, "r")) != NULL) {
              while(fgets(tmp, sizeof(tmp) - 1, fp)) {
                  XmTextInsert(win->news_text,
                               pos = XmTextGetInsertionPosition (win->news_text),
                               tmp);
                  XmTextSetInsertionPosition (win->news_text, pos + strlen(tmp));
              }
              fclose(fp);
          } else {
              XmxTextSetString (win->news_text, "");
          }
          
      }
      XmTextSetInsertionPosition (win->news_text, 0);
      
      if(follow){
	  XmxTextSetString (win->news_text_group, win->newsfollow_grp);
	  XmxTextSetString (win->news_text_subj, win->newsfollow_subj);
      } else {
	  XmxTextSetString (win->news_text_group, "");
	  XmxTextSetString (win->news_text_subj, "");
      }
	  XmxTextSetString (win->news_text_from, namestr);
    }
  XmxManageRemanage (win->news_win);
  return mo_succeed;
}
