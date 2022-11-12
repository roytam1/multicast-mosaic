/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <memory.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "gui-popup.h"
#include "gui-documents.h"
#include "mime.h"
#include "navigate.h"
#include "util.h"
#include "mailto.h"
#include "paf.h"

#ifdef DEBUG
#define DEBUG_GUI
#endif

extern void 		FreeMarkUpList(struct mark_up *List);

/* ----------------------------- HISTORY LIST ----------------------------- */

/* navigation */

/* This could be cached, but since it shouldn't take too long... */
void mo_back_possible (mo_window *win)
{
        mo_tool_state(&(win->tools[BTN_PREV]),XmxSensitive,BTN_PREV);
        XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxSensitive);
        mo_popup_set_something("Back", XmxSensitive, NULL);
}

 
/* purpose: Can't go back (nothing in the history list).  
 */
void mo_back_impossible (mo_window *win)
{
        XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxNotSensitive);
        mo_tool_state(&(win->tools[BTN_PREV]),XmxNotSensitive,BTN_PREV);
        mo_popup_set_something("Back", XmxNotSensitive, NULL);
}                                     

void mo_forward_possible (mo_window *win)
{
        mo_tool_state(&(win->tools[BTN_NEXT]),XmxSensitive,BTN_NEXT);
        XmxRSetSensitive(win->menubar, (XtPointer)mo_forward, XmxSensitive);
        mo_popup_set_something("Forward", XmxSensitive, NULL);
}                                     
       
/* purpose: Can't go forward (nothing in the history list). */
void mo_forward_impossible (mo_window *win)
{
	mo_tool_state(&(win->tools[BTN_NEXT]),XmxNotSensitive,BTN_NEXT);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_forward, XmxNotSensitive);
	/*mo_popup_set_something("Forward", XmxNotSensitive, NULL);*/
}      

/* ---------------------------- kill functions ---------------------------- */

/* Free the data contained in an mo_node.  Currently we only free
 * the text itself. */
void mo_free_node_data (mo_node *node)
{
	free (node->title);
	free (node->aurl_wa);
	free (node->aurl);
	free (node->base_url);
	free (node->text);
	if(node->goto_anchor)
		free (node->goto_anchor);
	node->title = NULL; 			/* sanity */
	node->aurl_wa = NULL; 			/* sanity */
	node->aurl = NULL; 			/* sanity */
	node->base_url = NULL; 			/* sanity */
	node->text = NULL; 			/* sanity */
	node->goto_anchor = NULL;		/* sanity */
	FreeMarkUpList(node->m_list);
	node->m_list = NULL;		/* sanity */
	FreeMimeStruct(node->mhs);
	node->mhs = NULL;		/* sanity */
	memset(node,0,sizeof(mo_node)); /* sanity */
}

/* Iterate through all descendents of an mo_node, but not the given
   mo_node itself, and kill them.  This is equivalent to calling
   mo_kill_node on each of those nodes, except this is faster since
   all the Motif list entries can be killed at once. */
static void mo_kill_node_descendents (mo_window *win, mo_node *node)
{
	mo_node *foo, *next;
	int count = 0;

	if (node == NULL)
		return ;
	for (foo = node->next; foo != NULL; ) {
		next=foo->next;
		mo_free_node_data (foo);
		count++;
		free(foo); foo=next;
	}
	node->next=NULL;

/* Free count number of items from the end of the list... */
	if (win->history_list && count) {
		XmListDeleteItemsPos(win->history_list,count, node->position + 1);
	}
}

/* ################## */

/* Add a new node to the navigation's history */

void MMUpdNavigationOnNewURL(mo_window *win, char * aurl_wa, char *aurl,
	char *goto_anchor, char * base_url, char * base_target,
	char *title, char *text, MimeHeaderStruct * mhs, int docid,
	struct mark_up * mlist)
{
	mo_node *node;
	NavigationType nt = win->navigation_action;

	node = (mo_node *)calloc (1, sizeof (mo_node));
	node->aurl_wa = strdup(aurl_wa); /* aurl with anchor */
	node->aurl = strdup(aurl);	/* THE absolute url of doc. */
	if(goto_anchor)
		node->goto_anchor = strdup(goto_anchor);
	if (base_url) {
		node->base_url = strdup(base_url); /* detect a tag <BASE> */
	} else {
		node->base_url = strdup(aurl);
	}
	node->m_list = mlist;
	node->docid = 1;
	if(text)
		node->text = strdup(text);	/* ### where to free ? ### */
	if(title)
		node->title = strdup(title);
	if(base_target)	
		node->base_target = strdup(base_target);
	node->mhs = mhs;

	/* If there is no current node, this is our first time through. */
	if (win->first_node == NULL) {
		win->first_node = node;
		node->previous = NULL;
		node->next = NULL;
		node->position = 1;
/* if we are here then win->current_node is NULL */
		win->current_node = node;
		if ( win->menubar == NULL) { /* ###FIXME (win is a frame , a sub_win) */
				/* try to enable navigation in frame..*/
			return;
		}
		mo_back_impossible (win);
	} else {
		mo_node *Cur = win->current_node;
		switch (nt) {
		case NAVIGATE_NEW: /* Node becomes end of history list. */
				/*Kill descendents of current node,since we'll 
				 *never be able to go forward to them again. */
			mo_kill_node_descendents (win, Cur);
			node->previous = Cur; /* Point back at current node. */
			Cur->next = node; /* Current node points forward to this. */
			node->next = NULL; /* Point forward to nothing. */
/* position in the history list widget */
			node->position = Cur->position + 1;
			win->current_node = node; /* Current node now becomes new node. */
			if ( win->menubar == NULL) { /* ###FIXME (win is a frame , a sub_win) try to enable navigation in frame..*/
				return;
			}
			mo_forward_impossible (win);
			mo_back_possible (win);
			break;
		case NAVIGATE_OVERWRITE:
			node->previous = Cur->previous;
			node->next = Cur->next;
			node->position = Cur->position;
			win->current_node = node;
			if(Cur->previous)
				Cur->previous->next = node;
			else
				win->first_node = node;
			if(Cur->next)
				Cur->next->previous = node;
			mo_free_node_data(Cur);
			free(Cur);
			return;
		case NAVIGATE_BACK:
			{
				mo_node *Old = Cur->previous;

				node->previous = Old->previous;
				node->next = Cur;
				node->position = Old->position;
				Cur->previous  = node;
				if(Old->previous)
					Old->previous->next = node;
				else
					win->first_node = node;
				win->current_node = node;
				mo_free_node_data(Old);
				free(Old);
				if (win->menubar == NULL) { /* FIXME (win is a frame)*/
					return;
				}
				if (node->previous == NULL)
					mo_back_impossible (win);
				mo_forward_possible(win);
				return;
			}
		case NAVIGATE_FORWARD:
			{
				mo_node *Old = Cur->next;
				node->previous = Cur;
				node->next = Old->next;
				node->position = Old->position;
				Cur->next = node;
				if(Old->next)
					Old->next->previous = node;
                        	win->current_node = node;
				mo_free_node_data(Old);
				free(Old);

				if (win->menubar == NULL) { /* FIXME (win is a frame)*/
					return;
				}
				if (node->next == NULL)
					mo_forward_impossible (win);
				mo_back_possible (win);
				return;
			}
		default:
			assert(0);
		}
	}
	if (win->history_list) {
		XmString xmstr = XmxMakeXmstrFromString(node->title);
		XmListAddItemUnselected(win->history_list, xmstr, node->position);
		XmStringFree (xmstr);
	}
/* This may or may not be filled in later! (AF) */
  	mo_add_to_rbm_history(win, node->aurl_wa, node->title);
}

/* ################## */


/* ---------------------------- mo_grok_title ----------------------------- */

/* Make up an appropriate title for a document that does not otherwise
   have one associated with it. */
static char *mo_grok_alternate_title (char *url, char *ref)
{
	char *title, *foo1, *foo2;

	if (!strncmp (url, "gopher:", 7)) { /* It's a gopher server. */
		if (ref) {	 /* Do we have a ref? */
			char *tmp = ref;

			while (*tmp && (*tmp == ' ' || *tmp == '\t'))
				tmp++;
			title = strdup (tmp);
			goto done;
		} else { /* Nope, no ref.  Make up a title. */
			foo1 = url + 9;
			foo2 = strstr (foo1, ":");
/* If there's a trailing colon (always should be.. ??)... */
			if (foo2) {
				char *server =(char *) malloc ((foo2 - foo1 + 2));

				memcpy(server, foo1, (foo2 - foo1));
				server[(foo2 - foo1)] = '\0';
				title = (char *) malloc ((strlen (server) + 32) * sizeof (char));
				sprintf (title, "%s %s", "Gopher server at" , server);
/* OK, we got a title... */
				free (server);
				goto done;
			} else { /* Aw hell... */
				title = strdup ("Gopher server" );
				goto done;
			}
		}
	}
/* If we got here, assume we should use 'ref' if possible
 * for the WAIS title. */
	if (!strncmp (url, "wais:", 5) ) {
/* It's a WAIS server. */
		if (ref) { /* Do we have a ref? */
			title = strdup (ref);
			goto done;
		} else { /* Nope, no ref.  Make up a title. */
			foo1 = url + 7;
			foo2 = strstr (foo1, ":");
/* If there's a trailing colon (always should be.. ??)... */
			if (foo2) {
				char *server = (char *)malloc ((foo2 - foo1 + 2));

				memcpy(server, foo1, (foo2 - foo1));
				server[(foo2 - foo1)] = '\0';
				title = (char *) malloc ((strlen (server) + 32) * sizeof (char));
				sprintf (title, "%s %s", "WAIS server at",server);
/* OK, we got a title... */
				free (server);
				goto done;
			} else { /* Aw hell... */
				title = strdup ("WAIS server" );
				goto done;
			}
		}
	}

	if (!strncmp (url, "news:", 5)) { /* It's a news source. */
		if (strstr (url, "@")) { /* It's a news article. */
			foo1 = url + 5;
			title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
			sprintf (title, "%s %s", "USENET article" , foo1);
			goto done;
		} else { /* It's a newsgroup. */
			foo1 = url + 5;
			title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
			sprintf (title, "%s %s", "USENET newsgroup" , foo1);
			goto done;
		}
	}

	if (!strncmp (url, "file:", 5)) { 	/* It's a file. */
		if (strncmp (url, "file:///", 8) == 0) { /* It's a local file. */
			foo1 = url + 7;
			title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
			sprintf (title, "%s %s", "Local file" , foo1);
			goto done;
		} else if (strncmp (url, "file://localhost/", 17) == 0) {
						/* It's a local file. */
			foo1 = url + 16;
			title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
			sprintf (title, "%s %s", "Local file" , foo1);
			goto done;
		} else { /* It's a remote file. */
			foo1 = url + 7;
			title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
			sprintf (title, "%s %s", "Remote file" , foo1);
			goto done;
		}
	}
	if (!strncmp (url, "ftp:", 4)) {	 /* It's a remote file. */
		foo1 = url + 6;
		title = (char *)malloc ((strlen (foo1) + 32) * sizeof (char));
		sprintf (title, "%s %s", "Remote file" , foo1);
		goto done;
	}
/* Punt... */
	title = (char *) malloc ((strlen (url) + 24) * sizeof (char));
	sprintf (title, "%s %s", "Untitled" , url);
done:
	return title;
}

/* Figure out a title for the given URL.  'ref', if it exists,
   was the text used for the anchor that pointed us to this URL;
   it is not required to exist. */
char *mo_grok_title (mo_window *win, char *url, char *ref)
{
	char *title, *t;

	title = win->current_node->title;
	if (!title)
		t = mo_grok_alternate_title (url, ref);
	else if (!strcmp (title, "Document"))
		t = mo_grok_alternate_title (url, ref);
	else {
		char *tmp = title;

		while (*tmp && (*tmp == ' ' || *tmp == '\t'))
			tmp++;
		if (*tmp)
			t = strdup (tmp);
		else
			t = mo_grok_alternate_title (url, ref);
	}
	mo_convert_newlines_to_spaces (t);
	return t;
}

/* ------------------------- navigation functions ------------------------- */

/* Back up a node. */
void mo_back (Widget w, XtPointer clid, XtPointer calld)
{
	RequestDataStruct rds;
	mo_window *win = (mo_window*) clid;

	if (win->pafd )		/* transfert in progress */
		(*win->pafd->call_me_on_stop)(win->pafd); /* stop it */
/* If there is no previous node, choke. */
	if (!win->current_node || win->current_node->previous == NULL){
		fprintf(stderr,"Severe Bug , Please report...\n");
		fprintf(stderr,"mo_back: current_node->previous == NULL\n");
		fprintf(stderr,"Aborting...\n");
		assert(0);
	}

        rds.req_url = win->current_node->previous->aurl_wa;
        rds.post_data = NULL;
        rds.ct = NULL;
        rds.is_reloading = False;
        rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_BACK;
        MMPafLoadHTMLDocInWin(win, &rds);
	win->navigation_action = NAVIGATE_NEW; /* reset to default */
/*  mo_gui_apply_default_icon(win); */
/*  mo_set_win_current_node (win, win->current_node->previous); */
}

/* Go forward a node. */
void mo_forward (Widget w, XtPointer clid, XtPointer calld)
{
	RequestDataStruct rds;
	mo_window *win = (mo_window*) clid;

	if (win->pafd )		/* transfert in progress */
		(*win->pafd->call_me_on_stop)(win->pafd); /* stop it */
	/* If there is no next node, choke. */
	if (!win->current_node || win->current_node->next == NULL) {
		fprintf(stderr,"Severe Bug , Please report...\n");
		fprintf(stderr,"mo_forward: current_node->previous == NULL\n");
		fprintf(stderr,"Aborting...\n");
		assert(0);
	}

        rds.req_url = win->current_node->next->aurl_wa;
        rds.post_data = NULL;
        rds.ct = NULL;
        rds.is_reloading = False;
        rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_FORWARD;
        MMPafLoadHTMLDocInWin(win, &rds);
	win->navigation_action = NAVIGATE_NEW; /* reset to default */
/*  mo_gui_apply_default_icon(win); */
/*  mo_set_win_current_node (win, win->current_node->next); */
}

/* Visit an arbitrary position.  This is called when a history
   list entry is double-clicked upon.

   Iterate through the window history; find the mo_node associated
   with the given position.  Call mo_set_win_current_node. */
mo_status mo_visit_position (mo_window *win, int pos)
{
  mo_node *node;
  
  for (node = win->first_node; node != NULL; node = node->next) {
      if (node->position == pos) {
          mo_set_win_current_node (win, node);
          goto done;
        }
    }

#ifdef DEBUG_GUI
  if (mMosaicSrcTrace) {
	fprintf (stderr, "UH OH BOSS, asked for position %d, ain't got it.\n",
		 pos);
  }
#endif

 done:
  return mo_succeed;
}

/* ----------------------------- HISTORY GUI ------------------------------ */

/* We've just init'd a new history list widget; look at the window's
   history and load 'er up. */
static void mo_load_history_list (mo_window *win, Widget list)
{
  mo_node *node;
  
  for (node = win->first_node; node != NULL; node = node->next) {
      XmString xmstr = 
	XmxMakeXmstrFromString (node->title);
      XmListAddItemUnselected (list, xmstr, 0);
      XmStringFree (xmstr);
    }
  
  XmListSetBottomPos (list, 0);
  if (win->current_node)
    XmListSelectPos (win->history_list, win->current_node->position, False);

  return;
}

/* ----------------------------- mail history ----------------------------- */

static XmxCallback (mailhist_win_cb1)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->mailhist_win);
}
static XmxCallback (mailhist_win_cb2)
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-nested-hotlists.html"));
}
static XmxCallback (mailhist_win_cb0)
{
	mo_window *win = (mo_window*)client_data;
	char *to, *subj;
	FILE *fp;
	mo_node *node;

	XtUnmanageChild (win->mailhist_win);
	to = XmTextGetString (win->mailhist_to_text);
	if (!to)
		return;
	if (to[0] == '\0')
		return;
	subj = XmTextGetString (win->mailhist_subj_text);
				/* Open a file descriptor to sendmail. */
	fp = mo_start_sending_mail_message (to, subj, "text/x-html", NULL);
	if (fp){
		free (to);
		free (subj);
		return;
	}
	fprintf (fp, "<HTML>\n");
	fprintf (fp, "<H1>History Path From %s</H1>\n",
		mMosaicAppData.author_full_name);
	fprintf (fp, "<DL>\n");
	for (node = win->first_node; node != NULL; node = node->next) {
		fprintf (fp, "<DT>%s\n<DD><A HREF=\"%s\">%s</A>\n", 
			node->title, node->aurl_wa, node->aurl_wa);
	}
	fprintf (fp, "</DL>\n");
	fprintf (fp, "</HTML>\n");
	mo_finish_sending_mail_message ();
	free (to);
	free (subj);
}

static mo_status mo_post_mailhist_win (mo_window *win)
{
  /* This shouldn't happen. */
  if (!win->history_win)
    return mo_fail;

  if (!win->mailhist_win) {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget mailhist_form, to_label, subj_label;
      
      /* Create it for the first time. */
      win->mailhist_win = XmxMakeFormDialog 
        (win->history_win, "NCSA Mosaic: Mail Window History" );
      dialog_frame = XmxMakeFrame (win->mailhist_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      mailhist_form = XmxMakeForm (dialog_frame);
      
      to_label = XmxMakeLabel (mailhist_form, "Mail To:" );
      XmxSetArg (XmNwidth, 335);
      win->mailhist_to_text = XmxMakeTextField (mailhist_form);
      
      subj_label = XmxMakeLabel (mailhist_form, "Subject:" );
      win->mailhist_subj_text = XmxMakeTextField (mailhist_form);

      dialog_sep = XmxMakeHorizontalSeparator (mailhist_form);
      
      buttons_form = XmxMakeFormAndThreeButtons (mailhist_form,
		"Mail" , "Dismiss" , "Help..." , 
		mailhist_win_cb0, mailhist_win_cb1, mailhist_win_cb2,
		(XtPointer) win);

      /* Constraints for mailhist_form. */
      XmxSetOffsets (to_label, 14, 0, 10, 0);
      XmxSetConstraints
        (to_label, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
         NULL, NULL, NULL, NULL);
      XmxSetOffsets (win->mailhist_to_text, 10, 0, 5, 10);
      XmxSetConstraints
        (win->mailhist_to_text, XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET,
         XmATTACH_FORM, NULL, NULL, to_label, NULL);

      XmxSetOffsets (subj_label, 14, 0, 10, 0);
      XmxSetConstraints
        (subj_label, XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, 
         XmATTACH_NONE,
         win->mailhist_to_text, NULL, NULL, NULL);
      XmxSetOffsets (win->mailhist_subj_text, 10, 0, 5, 10);
      XmxSetConstraints
        (win->mailhist_subj_text, XmATTACH_WIDGET, XmATTACH_NONE, 
         XmATTACH_WIDGET,
         XmATTACH_FORM, win->mailhist_to_text, NULL, subj_label, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints 
        (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, 
         XmATTACH_FORM,
         win->mailhist_subj_text, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, NULL, NULL, NULL);
    }
  
  XtManageChild (win->mailhist_win);
  
  return mo_succeed;
}

/* ---------------------------- history_win_cb ---------------------------- */

static XmxCallback (history_win_cb0)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->history_win);
}
static XmxCallback (history_win_cb1)
{
	mo_window *win = (mo_window*)client_data;

	mo_post_mailhist_win (win);
}
static XmxCallback (history_win_cb2)
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("docview-menubar-navigate.html#history"));
}

static XmxCallback (history_list_cb)
{
	mo_window *win = (mo_window*)client_data;
	XmListCallbackStruct *cs = (XmListCallbackStruct *)call_data;
  
	mo_visit_position (win, cs->item_position);
	return;
}

mo_status mo_post_history_win (mo_window *win)
{
  if (!win->history_win) {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget history_label;
      Widget history_form;
      XtTranslations listTable;
      static char listTranslations[] =
	"~Shift ~Ctrl ~Meta ~Alt <Btn2Down>: ListKbdSelectAll() ListBeginSelect() \n\
	 ~Shift ~Ctrl ~Meta ~Alt <Btn2Up>:   ListEndSelect() ListKbdActivate()";

      listTable = XtParseTranslationTable(listTranslations);
      
      /* Create it for the first time. */
      win->history_win = XmxMakeFormDialog(win->base, 
				"NCSA Mosaic: Window History" );
      dialog_frame = XmxMakeFrame (win->history_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      /* Main form. */
      history_form = XmxMakeForm (dialog_frame);

      XmxSetArg (XmNalignment, XmALIGNMENT_BEGINNING);
      history_label = XmxMakeLabel (history_form, "Where you've been:" );

      /* History list itself. */
      XmxSetArg (XmNresizable, False);
      XmxSetArg (XmNscrollBarDisplayPolicy, XmSTATIC);
      XmxSetArg (XmNlistSizePolicy, XmCONSTANT);
      XmxSetArg (XmNwidth, 380);
      XmxSetArg (XmNheight, 184);
      win->history_list = XmxMakeScrolledList(history_form, 
		history_list_cb, (XtPointer) win);
      XtAugmentTranslations (win->history_list, listTable);
      dialog_sep = XmxMakeHorizontalSeparator (history_form);
      buttons_form = XmxMakeFormAndThreeButtons(history_form, 
		"Mail To..." , "Dismiss" , "Help..." , 
		history_win_cb1, history_win_cb0, history_win_cb2,
		(XtPointer) win);

      /* Constraints for history_form. */
      XmxSetOffsets (history_label, 8, 0, 10, 10);
      XmxSetConstraints(history_label,XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM,
         XmATTACH_NONE, NULL, NULL, NULL, NULL);
      /* History list is stretchable. */
      XmxSetOffsets (XtParent (win->history_list), 0, 10, 10, 10);
      XmxSetConstraints (XtParent (win->history_list), 
         XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
         history_label, dialog_sep, NULL, NULL);
      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints (dialog_sep, 
         XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, NULL, NULL, NULL, NULL);

      /* Go get the history up to this point set up... */
      mo_load_history_list (win, win->history_list);
    }
  XmxManageRemanage (win->history_win);
  return mo_succeed;
}
