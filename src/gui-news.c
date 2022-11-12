/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#ifdef NEWS
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

#include "../libnut/list.h"
#include "newsrc.h"
#include "gui.h"
#include "gui-news.h"
#include "libnut/system.h"
#include "gui-dialogs.h"
#include "gui-documents.h"
#include "gui-menubar.h"
#include "mime.h"
#include "paf.h"

/*###### */
/*      Module-wide variables */
int ConfigView = 0;                      /* view format configure */
int newsShowAllArticles = 0;
int newsShowReadGroups = 0;
char *NewsGroup = NULL;
newsgroup_t *NewsGroupS = NULL;
int newsGotList = 0;
int newsShowAllGroups = 0;
NewsArt *CurrentArt = NULL;


#define LINE_LENGTH 512            /* Maximum length of line of ARTICLE etc */

/* Goto the previous (unread) thread */
void news_prevt(char *url)
{
	abort();
/*
  NewsArt *art, *p;

  if (art = is_news_url (url)) {
    if ((p = prevUnreadThread (art)) != NULL) {
      sprintf (url, "news:%s", p->ID);
      return;
    }
  }
  url[0] = 0;
*/
}

/* Goto next (unread) article in this thread */ 
void news_next(char *url)  
{  
	abort();
/*
  NewsArt *art, *p;                   
      
  if ((art = is_news_url(url)) == NULL) {
    url[0] = 0;
    return;
  }   
  url[0] = 0;
  if ((p=nextUnread (art, 0))) {
    sprintf (url, "news:%s", p->ID);
  } else if (!newsNoThreadJumping) {
    if ((p = nextUnreadThread (art))) {
      sprintf (url, "news:%s", p->ID);
    } 
  }   
*/
} 

   
/* Goto first (unread) article in next (unread) thread */
void news_nextt(char *url)            
{  
	abort();
/*
  NewsArt *art, *p;
                                      
  if ((art = is_news_url(url)) != NULL) {
    if ((p=nextUnreadThread (art))) {
      sprintf (url, "news:%s", p->ID);
      return;
    }
  }
  url[0] = 0;                         
*/
}                                     
  
/* Goto the previous (unread) article */
void news_prev(char *url)
{                                     
	abort();
/*
    NewsArt *art, *p;
                                      
    if ((art = is_news_url(url)) == NULL) { 
      url[0] = 0;                     
      return;                         
    }                                 
 
    url[0] = 0;                       
    if ((p = prevUnread (art,0)) != NULL) { 
      sprintf (url, "news:%s", p->ID);
    } else if (!newsNoThreadJumping) {
      if ((p=prevUnreadThread (art))) {
        sprintf (url, "news:%s", p->ID);
      }
    }
*/
}                                     

void news_index(char *url)            
{
	abort();
/*
    if(NewsGroup && is_news_url(url))
        sprintf(url,"news:%s",NewsGroup);
    else                              
        url[0] = 0;                  
*/
}                                     
 
/* Returns the status of the news buttons */
void news_status(char *url, int *prevt, int *nextt, int *prev, int *next, int *follow)
{ 
	abort();
/*
    NewsArt *art;
  
    if( art = is_news_url(url) ) {
      if(prevUnread(art,!newsNoThreadJumping))
        *prev = 1;                    
      else                            
        *prev = 0;
      if(prevUnreadThread(art))       
        *prevt = 1;
      else
        *prevt = 0;                   
      if (nextUnread (art,!newsNoThreadJumping))
        *next = 1;                    
      else                            
        *next = 0;                    
      if (nextUnreadThread (art))     
        *nextt = 1;                   
      else                            
        *nextt = 0;                   
      *follow = 1;                    
    } else {                          
      *follow=0;                      
      *prevt=0;                       
      *nextt=0;                       
      *next=0;                        
      *prev=0;                        
    }                                 
*/
}                                     

                
int NNTPgetarthdrs(char *art,char **ref, char **grp, char **subj, char **from,
        caddr_t appd)
{   
	abort();
/*
    int status, done;                 
    char *aname,*p;                   
    char line[LINE_LENGTH+1]; 
    char buffer[LINE_LENGTH+1];       
                                      
    *ref = *grp = *subj = *from = NULL;
                                      
    if (!initialized)                 
        initialized = initialize();   
    if (!initialized){                
        if(wWWParams.trace) fprintf(stderr,"No init?\n");
        HTProgress ("Could not set up news connection.",appd);
*/
        return -1;
/*
    }                                 
    if(s < 0) {                       
        HTProgress("Attempting to connect to news server",appd);
        if(OpenNNTP(appd)){           
            if(wWWParams.trace) fprintf(stderr,"No OpenNNTP?\n");
            HTProgress ("Could not connect to news server.",appd);
            return -1;
        }                             
    }                                 

/* FLUSH!!! */                    
/*
    HTInitInput(s);                   
    sprintf(buffer, "HEAD <%s>%c%c", art, CR, LF);
    status = response(buffer,appd);   

    if (status == 221) {        /* Head follows - parse it:*/
/*
        p = line;                               /* Write pointer */
/*
        done = NO;                    
        while(!done){                 
            char ch = *p++ = HTGetCharacter (appd);
            if (ch==(char)EOF) {      
                abort_socket(appd);     /* End of file, close socket */
/*
                return HT_LOADED;               /* End of file on response */
/*
            }                         
                                      
            if ((ch == LF)            
                || (p == &line[LINE_LENGTH]) ) {
                                      
                *--p=0;         /* Terminate  & chop LF*/
/*
                p = line;               /* Restart at beginning */
/*
                if (wWWParams.trace) fprintf(stderr, "G %s\n", line);
                switch(line[0]) {     
                                      
                case '.':             
                    done = (line[1]<' ');       /* End of article? */
/*
                    break;            
                                      
                case 'S':             
                case 's':             
                    if (match(line, "SUBJECT:"))
                        StrAllocCopy(*subj, line+9);/* Save subject */
/*
                    break;            
                                      
                case 'R':             
                case 'r':             
                    if (match(line, "REFERENCES:")) {
                        p = line + 12;
                        StrAllocCopy(*ref,p+1);
                        StrAllocCopy(*ref,p+1);
                    }                 
                    break;            
                                      
                case 'N':             
                case 'n':             
                    if (match(line, "NEWSGROUPS:")) {
                        p = line + 11;
                        StrAllocCopy(*grp,p+1);
                    }                 
                    break;            
                                      
                case 'f':             
                case 'F':             
                    if (match(line, "FROM:")) {
                      char author[1024+1]; 
                        parseemail (strchr(line,':')+1, author, NULL);
                        aname = author;
                        if (aname && *aname){
                          StrAllocCopy(*from, aname);
                          p = *from + strlen(*from) - 1;
                          if (*p==LF) *p = 0;   /* Chop off newline */
/*
                        } else {      
                          StrAllocCopy(*from, "Unknown");
                        }             
                    }                 
                    break;            
                                      
                } /* end switch on first character */
/*
                                      
                p = line;               /* Restart at beginning */
/*
            } /* if end of line */    
/*
        } /* Loop over characters */  
/*
    } /* If good response */
/*
*/
}

                 
int NNTPpost(char *from, char *subj, char *ref, char *groups, char *msg,
        caddr_t appd)
{
	abort();
/*
    char buf[1024];
                                      
    if (!initialized)                 
        initialized = initialize();   
    if (!initialized){                
        if(wWWParams.trace) fprintf(stderr,"No init?\n");
        HTProgress ("Could not set up news connection.",appd);
*/
        return -1;
/*
    }
                                      
    if(s < 0) {
        HTProgress("Attempting to connect to news server",appd);
        if(OpenNNTP(appd)){           
            if(wWWParams.trace) fprintf(stderr,"No OpenNNTP?\n");
            HTProgress ("Could not connect to news server.",appd);
            return -1;
        }                             
    }                                 
                                      
    if(response("POST\r\n",appd) != 340) { 
        HTProgress("Server does not allow posting.",appd);
        return 0;                     
    }                                 
 
    HTProgress("Posting your article...",appd);
    sprintf(buf,"From: %s\r\n",from);
    newswrite(buf);                   
    sprintf(buf,"Subject: %s\r\n",subj);
    newswrite(buf);                   
    if(ref){ 
    if(ref){                          
        sprintf(buf,"References: %s\r\n",ref);
        newswrite(buf);               
    }                                 
    sprintf(buf,"Newsgroups: %s\r\n",groups);
    newswrite(buf);                   
    sprintf(buf,"X-Newsreader: NCSA Mosaic\r\n\r\n");
    newswrite(buf);                   
    newswrite(msg);                   
    if(response("\r\n.\r\n",appd) != 240)  
        HTProgress("Article was not posted.",appd);
    else                              
        HTProgress("Article was posted successfully.",appd);
                                      
    HTDoneWithIcon (appd);            
*/
}                                     
    
      
/* this is VERY non-reentrant.... */
/* static char qline[LINE_LENGTH+1]; */
char *NNTPgetquoteline(char *art, caddr_t appd)
{       
	abort();
/*
    char *p;
    int i,status ;
            
    if (!initialized)
        initialized = initialize();
    if (!initialized){                
        if(wWWParams.trace) fprintf(stderr,"No init?\n");
        HTProgress ("Could not set up news connection.",appd);
        return NULL;                  
    }                                 
    if(s < 0) {
        HTProgress("Attempting to connect to news server",appd);
        if(OpenNNTP(appd)){
            if(wWWParams.trace) fprintf(stderr,"No OpenNNTP?\n");
            HTProgress ("Could not connect to news server.",appd);
            return NULL;
        }                             
    }                                 
    if(art){ /* FLUSH!!! */           
/*
        HTInitInput(s);               
        sprintf(qline, "BODY <%s>%c%c", art, CR, LF);
        status = response(qline,appd);
        if (status != 222) return NULL;
    }                                 
    qline[0] = '>';                   
    qline[1] = ' ';                   
    for(p = &qline[2],i=0;;p++,i++){  
        *p = HTGetCharacter(appd);    
        if (*p==(char)EOF) { 
            abort_socket(appd); /* End of file, close socket */
/*
            return NULL;        /* End of file on response */
/*
        }                             
        if(*p == '\n'){               
            *++p = 0;                 
            break;                    
        }                             
        if(i == LINE_LENGTH-4){       
            *p = 0;                   
            break;                    
        }                             
    }                                 
    if(qline[2]=='.' && qline[3] < ' ') return NULL;
    return qline;                     
*/
	return NULL;
}    


    
/* HTSetNewsConfig ()
   Expects: artView    -- Article View configuration: 0 = Article View,
                          1 = Thread View
            artAll     -- Show All Articles? 0 = No, non zero = yes
            grpAll     -- Show All Groups? 0 = no, non zero = yes
            grpRead    -- Show Read Groups? 0 = no, non zero = yes
            noThrJmp   -- Don't jump threads? 0 = no, non zero = yes
            newsRC     -- Use the newsrc? 0 = no, non zero = yes
            nxtUnread  -- Next thread should be the next unread?
                          0 = no, non zero = yes
            prevUnread -- Prev thread should be the prev unread?
                          0 = no, non zero = yes
    Returns: Nothing
    
    Sets the current news config.
*/
   
void HTSetNewsConfig (int artView, int artAll, int grpAll, int grpRead,
                      int noThrJmp, int newsRC, int nxtUnread, int prevUnread)
{  
	abort();
/*
  if (artView != NO_CHANGE) {
    ConfigView = !artView;
  }                                   
                                      
  if (artAll != NO_CHANGE) {          
    newsShowAllArticles = artAll;     
  }                                   
                                      
  if (grpAll != NO_CHANGE) {          
    newsShowAllGroups = grpAll;       
  }                                   
                                      
  if (grpRead != NO_CHANGE) { 
    newsShowReadGroups = grpRead;     
  }                                   
                                      
  if (noThrJmp != NO_CHANGE) {        
    newsNoThreadJumping = noThrJmp;   
  }                                   
                                      
  if (nxtUnread != NO_CHANGE) {       
    newsNextIsUnread = nxtUnread;     
  }                                   
                                      
  if (prevUnread != NO_CHANGE) {      
    newsPrevIsUnread = prevUnread;    
  }                                   
*/                                    
}                                     
       

/*###### */

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
/*	MMPafLoadHTMLDocInWin(win,"news:*"); */
}

void gui_news_showAllGroups (mo_window *win)
{
	gui_news_flush (win);
	newsGotList = 0;
	HTSetNewsConfig (-1, -1, 1, 1, -1, -1,-1,-1);
	gui_news_updateprefs (win);
/*	MMPafLoadHTMLDocInWin (win, "news:*"); */
}

void gui_news_showGroups (mo_window *win)
{
	/* Show only subbed groups */
	HTSetNewsConfig (-1,-1,0,0,-1,-1,-1,-1);
	gui_news_updateprefs (win);
/*	MMPafLoadHTMLDocInWin (win, "news:*"); */
}

void gui_news_showReadGroups (mo_window *win)
{
	HTSetNewsConfig (-1,-1,0,1,-1,-1,-1,-1); 
	gui_news_updateprefs (win);
/*	MMPafLoadHTMLDocInWin (win, "news:*"); */
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
/*	MMPafLoadHTMLDocInWin (win, buf); */
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
/*	MMPafLoadHTMLDocInWin (win, buf); */
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
/*	MMPafLoadHTMLDocInWin (win, buf); */
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
/*	MMPafLoadHTMLDocInWin (win, buf); */
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
/*	MMPafLoadHTMLDocInWin (win, buf); */
}

void gui_news_initflush (mo_window *win)
{
	newsrc_initflush ();
}

void gui_news_index(mo_window *win)
{
	char url[128];

	newsrc_flush ();
	strcpy(url,win->current_node->aurl_wa);
	news_index(url);
/*	if(url[0]) MMPafLoadHTMLDocInWin(win,url); */
}

void gui_news_prev(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->aurl_wa);
	news_prev(url);
/*	if(url[0]) MMPafLoadHTMLDocInWin(win,url); */
}

void gui_news_next(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->aurl_wa);
	news_next(url);
/*	if(url[0]) MMPafLoadHTMLDocInWin(win,url); */
}

void gui_news_prevt(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->aurl_wa);
	news_prevt(url);
/*	if(url[0]) MMPafLoadHTMLDocInWin(win,url); */
}

void gui_news_nextt(mo_window *win)
{
	char url[128];

	strcpy(url,win->current_node->aurl_wa);
	news_nextt(url);
/*	if(url[0]) MMPafLoadHTMLDocInWin(win,url); */
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

	mo_open_another_window (win, mo_assemble_help_url("help-on-news.html"));
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

	mo_open_another_window (win, mo_assemble_help_url("help-on-news.html"));
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

    if(strncmp("news:",win->current_node->aurl_wa,5)) 
	return mo_fail; /* fix me ########### */
    

    NNTPgetarthdrs(&(win->current_node->aurl_wa)[5], 
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
    win->newsfollow_artid = (char*)malloc(strlen(win->current_node->aurl_wa));
    strcpy(win->newsfollow_artid, &(win->current_node->aurl_wa)[5]);

    if(!win->newsfollow_ref){
	win->newsfollow_ref = (char*)malloc(strlen(win->current_node->aurl_wa));
	sprintf(win->newsfollow_ref,"<%s>",&(win->current_node->aurl_wa)[5]);
    } else {
	s = (char*)malloc(strlen(win->newsfollow_ref)+
			strlen(win->current_node->aurl_wa)); /* this sucks -bjs*/
	sprintf(s,"%s <%s>",win->newsfollow_ref,&(win->current_node->aurl_wa)[5]);
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
#endif
