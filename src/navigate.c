/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <time.h>
#include <assert.h>
#include <sys/types.h>
#include <memory.h>

#include "../libnut/system.h"
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
#include "URLParse.h"

#ifdef DEBUG
#define DEBUG_GUI
#endif

static void mo_add_to_rbm_history(mo_window *win, char *url, char *title);

/* ----------------------------- HISTORY LIST ----------------------------- */

/* navigation */

/* This could be cached, but since it shouldn't take too long... */
void mo_back_possible (mo_window *win)
{
        mo_tool_state(&(win->tools[BTN_PREV]),XmxSensitive,BTN_PREV);
        XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxSensitive);
        mo_popup_set_something("Back", XmxSensitive, win->popup_b3_items);
}

 
/* purpose: Can't go back (nothing in the history list).  
 */
void mo_back_impossible (mo_window *win)
{
        XmxRSetSensitive (win->menubar, (XtPointer)mo_back, XmxNotSensitive);
        mo_tool_state(&(win->tools[BTN_PREV]),XmxNotSensitive,BTN_PREV);
        mo_popup_set_something("Back", XmxNotSensitive, win->popup_b3_items);
}                                     

void mo_forward_possible (mo_window *win)
{
        mo_tool_state(&(win->tools[BTN_NEXT]),XmxSensitive,BTN_NEXT);
        XmxRSetSensitive(win->menubar, (XtPointer)mo_forward, XmxSensitive);
        mo_popup_set_something("Forward", XmxSensitive, win->popup_b3_items);
}                                     
       
/* purpose: Can't go forward (nothing in the history list). */
void mo_forward_impossible (mo_window *win)
{
	mo_tool_state(&(win->tools[BTN_NEXT]),XmxNotSensitive,BTN_NEXT);
	XmxRSetSensitive (win->menubar, (XtPointer)mo_forward, XmxNotSensitive);
	mo_popup_set_something("Forward", XmxNotSensitive, win->popup_b3_items);
}      

/* ---------------------------- kill functions ---------------------------- */

/* Free the data contained in an mo_node.  Currently we only free
 * the text itself. */
void mo_free_node_data (mo_node *node)
{
	int i;

	FreeHtmlTextInfo(node->htinfo);
	free (node->aurl_wa);
	free (node->aurl);
	free (node->base_url);
	free (node->text);
	FreeMimeStruct(node->mhs);
	if(node->goto_anchor)
		free (node->goto_anchor);

	if(node->frame_tab && (node->node_type == NODE_FRAMESET_TYPE) &&
	    (node->win_type == FRAMESET_TYPE) &&
	    (node->nframe != 0)) {
		for(i=node->nframe; i<node->nframe; i++){
			free(node->frame_tab[i].aframe_src);
			mo_free_node_data(node->frame_tab[i].frame_node);
		}
		free(node->frame_tab);
	}
	if(node->frame_tab && (node->node_type == NODE_FRAMESET_TYPE) &&
	    (node->win_type == FRAME_TYPE) &&
	    (node->nframe != 0)) {
		free(node->frame_tab[node->num_frame_target].aframe_src);
		mo_free_node_data(node->frame_tab[node->num_frame_target].frame_node);
		free(node->frame_tab);
	}

	memset(node,0,sizeof(mo_node)); /* sanity */
}

static void mo_kill_node_descendents_frame(mo_window *win, mo_node *node,
	mo_node ** ret_next)
{
	mo_node *foo, *next;

	*ret_next = NULL;
	if (node == NULL)
		return ;

	for (foo = node->next; foo != NULL; ) {
		if ( foo->node_type == NODE_FRAMESET_TYPE && foo->win_type == FRAME_TYPE) {
			next=foo->next;
/* free only frame part */
			free(foo->frame_tab[foo->num_frame_target].aframe_src);
			mo_free_node_data (foo->frame_tab[foo->num_frame_target].frame_node);
			free(foo->frame_tab);
			memset(foo,0,sizeof(mo_node)); /* sanity */
			free(foo); foo=next;
		} else {
			*ret_next = foo;
			break;
		}
	}
	node->next=*ret_next;
}

/* Iterate through all descendents of an mo_node, but not the given
   mo_node itself, and kill them.  This is equivalent to calling
   mo_kill_node on each of those nodes, except this is faster since
   all the Motif list entries can be killed at once. */
static void mo_kill_node_descendents (mo_window *win, mo_node *node)
{
	mo_node *foo, *next;

	if (node == NULL)
		return ;
	for (foo = node->next; foo != NULL; ) {
		if (foo->node_type == NODE_FRAMESET_TYPE && foo->win_type == FRAMESET_TYPE) {
			mo_kill_node_descendents_frame(win, foo, &next);
			foo->next = next;
			mo_free_node_data (foo);
			free(foo); foo = next;
		} else {
			assert(foo->node_type == NODE_NOTFRAME_TYPE);
			next=foo->next;
			mo_free_node_data (foo);
			free(foo); foo=next;
		}
	}
	node->next=NULL;
}

/* ################## */

/* HTML 'normal' page is loading*/
static void NavigateHtml(mo_window *win, char * aurl_wa, char *aurl,
	char *goto_anchor, char *text, MimeHeaderStruct *mhs, int docid,
	HtmlTextInfo * htinfo)
{
	NavigationType na = win->navigation_action;
	mo_node *node;
	mo_node *Cur;
	mo_node *Old;

/* allocate node */
	node = (mo_node *)calloc (1, sizeof (mo_node));
	node->aurl_wa = aurl_wa; 	/* aurl with anchor */
	node->aurl = aurl;		/* THE absolute url of doc. */
	node->docid = 1;
	node->htinfo = htinfo;
	node->node_type = NODE_NOTFRAME_TYPE;
	node->win_type = NOTFRAME_TYPE;
	node->num_frame_target = -1;	/* numero du frame qui provoque la navigation */
			/* dans le cas NODE_FRAMESET_TYPE && NAVIGATE_TARGET_SELF && FRAME_TYPE */
	node->nframe = 0;
	node->frame_tab = NULL;

	if(goto_anchor)
		node->goto_anchor = strdup(goto_anchor);

	if (htinfo->base_url) {
		node->base_url = strdup(htinfo->base_url); /* detect a tag <BASE> */
	} else {
		node->base_url = strdup(aurl); /* baseurl we refer when clic in doc */
	}
	if(text)
		node->text = strdup(text);	/* ### where to free ? ### */

	node->mhs = mhs;

	/* If there is no current node, this is our first time through. */
	if (win->first_node == NULL) {
		win->first_node = node;
		node->previous = NULL;
		node->next = NULL;
/* if we are here then win->current_node is NULL */
		win->current_node = node;
		mo_back_impossible (win);
  		mo_add_to_rbm_history(win, node->aurl_wa, node->htinfo->title);
		return;
	}

/* win->first_node != NULL */
	Cur = win->current_node;
	switch (na) {
	case NAVIGATE_NEW: /* Node becomes end of history list. */
			/*Kill descendents of current node,since we'll 
			 *never be able to go forward to them again. */
		mo_kill_node_descendents (win, Cur);
		node->previous = Cur; /* Point back at current node. */
		Cur->next = node; /* Current node points forward to this. */
		node->next = NULL; /* Point forward to nothing. */
		win->current_node = node; /* Current node now becomes new node. */
		mo_forward_impossible (win);
		mo_back_possible (win);
/* This may or may not be filled in later! (AF) */
	  	mo_add_to_rbm_history(win, node->aurl_wa, node->htinfo->title);
		break;
	case NAVIGATE_OVERWRITE:
		node->previous = Cur->previous;
		node->next = Cur->next;
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
		Old = Cur->previous;
		node->previous = Old->previous;
		node->next = Cur;
		Cur->previous  = node;
		if(Old->previous)
			Old->previous->next = node;
		else
			win->first_node = node;
		win->current_node = node;
		mo_free_node_data(Old);
		free(Old);
		if (node->previous == NULL)
			mo_back_impossible (win);
		mo_forward_possible(win);
		return;
	case NAVIGATE_FORWARD:
		Old = Cur->next;
		node->previous = Cur;
		node->next = Old->next;
		Cur->next = node;
		if(Old->next)
			Old->next->previous = node;
               	win->current_node = node;
		mo_free_node_data(Old);
		free(Old);

		if (node->next == NULL)
			mo_forward_impossible (win);
		mo_back_possible (win);
		return;
	default:
		assert(0);
	}
}

static void NavigateFrameset(mo_window *win, char * aurl_wa, char *aurl,
        char *goto_anchor, char *text, MimeHeaderStruct *mhs, int docid,
        HtmlTextInfo * htinfo)
{
	NavigationType na = win->navigation_action;
	mo_node *node;
	mo_node *Cur;
	mo_node *ne;
	int i;

/* Dans le cas BACK et FORWARD On bypass le rechargement d'un frameset. */
/* la mise a jour se fait dans mo_back ou mo_forward */
/* Dans le cas NAVIGATE_OVERWRITE, on suprime la navigation a L'INTERIEUR */
/* du frameset et on laisse le dernier node->next telquel */
/* la navigation d'un frameset est tres special... */

	assert(na == NAVIGATE_NEW || na == NAVIGATE_OVERWRITE);

/* allocate node */     
        node = (mo_node *)calloc (1, sizeof (mo_node));
        node->aurl_wa = aurl_wa;        /* aurl with anchor */
        node->aurl = aurl;              /* THE absolute url of doc. */
        node->docid = 1;                 
        node->htinfo = htinfo;           
        node->node_type = NODE_FRAMESET_TYPE;
        node->win_type = FRAMESET_TYPE;
        node->num_frame_target = -1;    /* numero du frame qui provoque la navigation */                       
                        /* dans le cas NODE_FRAMESET_TYPE && NAVIGATE_TARGET_SELF && FRAME_TYPE */     
        node->nframe = htinfo->nframes;
        node->frame_tab = (NavFrameInfo*)calloc(node->nframe,
			sizeof(NavFrameInfo));

	for(i=0; i<node->nframe; i++) {	/* set some frame info */
		node->frame_tab[i].aframe_src = strdup(htinfo->frameset_info->frames[i].frame_src);
		node->frame_tab[i].is_loaded = False;
		node->frame_tab[i].frame_node = NULL; /* to be update later */
	}

        if(goto_anchor)
                node->goto_anchor = strdup(goto_anchor);
                
        if (htinfo->base_url) {
                node->base_url = strdup(htinfo->base_url); /*detect a tag <BASE>*/
        } else {                         
                node->base_url = strdup(aurl); /* baseurl we refer when clic in doc */
        }       
        if(text)
                node->text = strdup(text);      /* ### where to free ? ### */
        node->mhs = mhs;                 
                                         
        /* If there is no current node, this is our first time through. */
        if (win->first_node == NULL) {   
                win->first_node = node;  
                node->previous = NULL;   
                node->next = NULL;       
/* if we are here then win->current_node is NULL */
                win->current_node = node;
                mo_back_impossible (win);
                mo_add_to_rbm_history(win, node->aurl_wa, node->htinfo->title);
                return;                  
        }
/* win->first_node != NULL */
	Cur = win->current_node;
	switch (na) {
	case NAVIGATE_NEW:
		mo_kill_node_descendents (win, Cur);
		node->previous = Cur; /* Point back at current node. */
                Cur->next = node; /* Current node points forward to this. */
                node->next = NULL; /* Point forward to nothing. */
                win->current_node = node; /* Current node now becomes new node. */
                mo_forward_impossible (win);
                mo_back_possible (win);
		mo_add_to_rbm_history(win, node->aurl_wa, node->htinfo->title);
                break;	
	case NAVIGATE_OVERWRITE:
		/* car le reload implique un rechargement et une possible modif.*/
		/* du code du frameset, alors kill node descents only in frameset */
		/* reset to the root frameset */
		while(Cur->node_type == NODE_FRAMESET_TYPE) {
			if(Cur->win_type == FRAMESET_TYPE) {
				break;
			}
			Cur = Cur->previous;
		}
			
		mo_kill_node_descendents_frame(win, Cur, &ne);
		Cur->next = ne;
/* le reste c'est comme le html... */
		node->previous = Cur->previous;
                node->next = Cur->next;
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
	}
}

static void NavigateFrame(mo_window *fwin, char * aurl_wa, char *aurl,
        char *goto_anchor, char *text, MimeHeaderStruct *mhs, int docid,
        HtmlTextInfo * htinfo)
{
	NavigationType na = fwin->navigation_action;
	NavigationTargetType nt = fwin->navigation_target_type;
	mo_node *fnode, *pnode, *new_pnode, *Pcur, *Pold, *Pnext, *ne;
	mo_window *pwin;	/* parent win */
	int findex;

/* the real navigation for FRAME is in its parent (FRAMESET) */
/* c.a.d. que le frameset assure la navigation */

	pwin = fwin->frame_parent;

/* allocate node */
        fnode = (mo_node *)calloc (1, sizeof (mo_node));
        fnode->aurl_wa = aurl_wa;        /* aurl with anchor */
        fnode->aurl = aurl;              /* THE absolute url of doc. */
        fnode->docid = 1;
        fnode->htinfo = htinfo;
        fnode->node_type = NODE_FRAMESET_TYPE;
        fnode->win_type = FRAME_TYPE;

/* numero du frame qui provoque la navigation */
/* dans le cas NODE_FRAMESET_TYPE && NAVIGATE_TARGET_SELF && FRAME_TYPE */
        fnode->num_frame_target = fwin->frame_dot_index;
        fnode->nframe = 0;
        fnode->frame_tab = NULL;
                
        if(goto_anchor)                  
                fnode->goto_anchor = strdup(goto_anchor);
                                         
        if (htinfo->base_url) {          
                fnode->base_url = strdup(htinfo->base_url); /*detect a tag <BASE>*/
        } else {                         
                fnode->base_url = strdup(aurl); /* baseurl we refer when clic in doc */
        }                                
        if(text)                         
                fnode->text = strdup(text);      /* ### where to free ? ### */
        fnode->mhs = mhs;                 

	assert(fwin->first_node == NULL);

	fwin->current_node = fnode;	/* set info we needed */
					/* frame have only ONE node */

/* two case for nt:
	- NAVIGATE_TARGET_ROOT, because of FRAMESET load
	- NAVIGATE_TARGET_SELF, because frame change 
*/
	findex = fwin->frame_dot_index;
	pnode = pwin->current_node;
	switch(nt){
	case NAVIGATE_TARGET_ROOT:	/* update slot */
		/* only when frameset Navigate NEW or OVERWRITE */
		assert(pwin->navigation_action == NAVIGATE_NEW || pwin->navigation_action == NAVIGATE_OVERWRITE );
		pnode->frame_tab[findex].aframe_src = strdup(aurl_wa);
		pnode->frame_tab[findex].is_loaded = True;
		pnode->frame_tab[findex].frame_node = fnode;
		if(pnode->next){
               		mo_forward_possible (pwin);
		} else {
               		mo_forward_impossible (pwin);
		}
		if (pnode->previous) {
			mo_back_possible(pwin);
		} else {
               		mo_back_possible (pwin);
		}
		return;
	case NAVIGATE_TARGET_SELF:
		switch(na){
		case NAVIGATE_OVERWRITE:
			/* we reload only this frame */
			if (pnode->frame_tab[findex].frame_node){
				mo_free_node_data(pnode->frame_tab[findex].frame_node);
				free(pnode->frame_tab[findex].frame_node);
			}
			pnode->frame_tab[findex].aframe_src = strdup(aurl_wa);
			pnode->frame_tab[findex].is_loaded = True;
			pnode->frame_tab[findex].frame_node = fnode;
			return;
		case NAVIGATE_NEW: /* some frame content is new */
				/* make a new node for parent (frameset) */
				/* create a new pnode (frameset) */
			new_pnode = (mo_node *)calloc(1, sizeof(mo_node));
/* make a copy */
			memcpy(new_pnode, pnode, sizeof(mo_node));
			new_pnode->win_type = FRAME_TYPE;
			new_pnode->num_frame_target = fwin->frame_dot_index;
			new_pnode->frame_tab=(NavFrameInfo*)calloc(pnode->nframe,
				sizeof(NavFrameInfo));
			memcpy(new_pnode->frame_tab, pnode->frame_tab,
				pnode->nframe *sizeof(NavFrameInfo));
/* change the slot */
                        new_pnode->frame_tab[findex].aframe_src = strdup(aurl_wa);
                        new_pnode->frame_tab[findex].is_loaded = True;
                        new_pnode->frame_tab[findex].frame_node = fnode;

/* faire le chainage */
			/* Node becomes end of frameset history list. */
                        /*Kill descendents of current node,since we'll
                         *never be able to go forward to them again. */
			Pcur = pwin->current_node;
	                mo_kill_node_descendents_frame (pwin, Pcur, &ne);

                	new_pnode->previous = Pcur; /* Point back at current node. */
                	Pcur->next = new_pnode; /* Current node points forward to this. */
                	new_pnode->next = ne; /* Point forward to ne. */
					/* ne is the next node return by mo_kill_node_descendents_frame */
					/* pointing to next full page */
                	pwin->current_node = new_pnode; /* Current node now becomes new node. */
			if(ne){
                		mo_forward_possible (pwin);
			} else {
                		mo_forward_impossible (pwin);
			}
                	mo_back_possible (pwin);
			return;
		case NAVIGATE_BACK:
			Pcur = pwin->current_node;
			Pold = Pcur->previous;
			Pold->frame_tab[findex].frame_node = fnode;
			Pold->frame_tab[findex].is_loaded = True;
			pnode = pwin->current_node = Pold;
                	if (pnode->previous == NULL)
                        	mo_back_impossible (pwin);
                	mo_forward_possible(pwin);
			return;

		case NAVIGATE_FORWARD:
			Pcur = pwin->current_node;
			Pnext = Pcur->next;
			Pnext->frame_tab[findex].frame_node = fnode;
			Pnext->frame_tab[findex].is_loaded = True;
			pnode = pwin->current_node = Pnext;
                	if (pnode->next == NULL)
                        	mo_forward_impossible (pwin);
                	mo_back_possible(pwin);
			return;
		}
	}
	assert(0);
}

/* Add a new node to the navigation's history */

void MMUpdNavigationOnNewURL(mo_window *win, char * aurl_wa, char *aurl,
	char *goto_anchor, char *text, MimeHeaderStruct *mhs, int docid,
	HtmlTextInfo * htinfo)

{
	switch(win->frame_type) {
	case NOTFRAME_TYPE:     /* HTML 'normal' page is loading*/
		NavigateHtml(win, aurl_wa, aurl, goto_anchor, text,
			mhs, docid, htinfo);
		return;
	case FRAMESET_TYPE:     /* frameset code is loading */
		NavigateFrameset(win, aurl_wa, aurl, goto_anchor, text, 
                        mhs, docid, htinfo);
		return;
	case FRAME_TYPE:        /* frame is loading */
		NavigateFrame(win, aurl_wa, aurl, goto_anchor, text,
                        mhs, docid, htinfo);
		return;
	default:
		assert(0);
	}
}

/* ################## */

/* ------------------------- navigation functions ------------------------- */

static void back_from_frameset(mo_window *win, mo_node *cur_node, mo_node *new_node)
{
	RequestDataStruct rds;
	Widget * htmlw_tabs;
	int i;

	if( new_node->node_type == NODE_NOTFRAME_TYPE) { /* new is html */
        	rds.req_url = win->current_node->previous->aurl_wa;
        	rds.post_data = NULL;
        	rds.ct = NULL;
        	rds.is_reloading = False;
        	rds.gui_action = HTML_LOAD_CALLBACK;
		win->navigation_action = NAVIGATE_BACK;
        	MMPafLoadHTMLDocInWin(win, &rds);
		win->navigation_action = NAVIGATE_NEW; /* reset to default */
		return;
	}
/* new_node->node_type == NODE_FRAMESET_TYPE */

	if (cur_node->win_type == FRAME_TYPE ) {
/* we back in same frameset */
		mo_window * fwin;
		int findex;

		findex = cur_node->num_frame_target;
		fwin = win->frame_sons[findex];
/* we backed in same frameset: load only frame */
        	rds.req_url = new_node->frame_tab[findex].aframe_src;
        	rds.post_data = NULL;
        	rds.ct = NULL;
        	rds.is_reloading = False;
        	rds.gui_action = HTML_LOAD_CALLBACK;
		fwin->navigation_action = NAVIGATE_BACK;
		fwin->navigation_target_type = NAVIGATE_TARGET_SELF;
		MMPafLoadHTMLDocInFrame(fwin, &rds);
		return;
	}

/* cur_node->win_type == FRAMESET_TYPE */
/* we back from frameset in other frameset */
/* dewrap old (voir paf.c HTMLUnsetFrameSet ) */
#ifdef MULTICAST     
	win->mc_sbh_value = 0;
	win->mc_sbv_value = 0;
	win->mc_send_scrollbar_flag = False;
#endif               
	for(i = 0; i < win->frame_sons_nbre; i++) {
/* stop old plugins if exist */
#ifdef OBJECT           
		if (win->frame_sons[i]->htinfo) {
			MMStopPlugins(win->frame_sons[i],
			win->frame_sons[i]->htinfo->mlist);
		}
#endif                  
		MMDestroySubWindow(win->frame_sons[i]);
		win->frame_sons[i] = NULL; /* sanity */
	}    
	free(win->frame_sons);
	win->frame_sons = NULL;
	HTMLUnsetFrameSet (win->scrolled_win);
	win->frame_type = NOTFRAME_TYPE;

/* create new one from new_node  (comme back_from_html) */
	win->frame_type = FRAMESET_TYPE;
#ifdef MULTICAST
	win->dot=NULL;
	win->n_do = 0;
#endif
	HTMLSetFrameSet (win->scrolled_win, new_node->htinfo->mlist,
		new_node->aurl, new_node->htinfo->nframes,
		new_node->htinfo->frameset_info, &htmlw_tabs);
/* on met a jour immediatement la partie navigation. */
	win->current_node = new_node;
	win->navigation_action = NAVIGATE_OVERWRITE;
	mo_set_win_headers(win, new_node->aurl_wa, new_node->htinfo->title);
	XFlush(XtDisplay(win->scrolled_win));

/* init parent FRAMESET */
	win->frame_name = NULL;
	win->frame_parent =NULL;
	win->frame_sons = NULL;
	win->frame_sons_nbre =  new_node->htinfo->nframes;
	win->number_of_frame_loaded = new_node->htinfo->nframes;
	win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre , sizeof(mo_window *));
/* creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
	for(i = 0 ; i < new_node->nframe; i++ )  {
		Widget htmlw;
		mo_window *sub_win;
		char * url,*frame_name;

		htmlw = htmlw_tabs[i];
		url = strdup(new_node->frame_tab[i].aframe_src);
		frame_name = new_node->htinfo->frameset_info->frames[i].frame_name;
		sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
		sub_win->frame_sons_nbre = 0;
		sub_win->frame_dot_index = i;
		win->frame_sons[i] = sub_win;
		rds.ct = rds.post_data = NULL;
		rds.is_reloading = False;
		rds.req_url = mo_url_canonicalize_keep_anchor(url,
			win->current_node->base_url);
		rds.gui_action = FRAME_LOADED_FROM_FRAMESET;
		sub_win->navigation_action = NAVIGATE_BACK; /* doesnot really matter */
		sub_win->navigation_target_type = NAVIGATE_TARGET_ROOT; /* because of TARGET_ROOT */
		MMPafLoadHTMLDocInFrame (sub_win, &rds);
		free(url);     
		free(rds.req_url);
	}
}

static void back_from_html(mo_window *win, mo_node *cur_node, mo_node *new_node)
{
	RequestDataStruct rds;
	Widget * htmlw_tabs;
	int i;

	if( new_node->node_type == NODE_NOTFRAME_TYPE) {	/* same type */
        	rds.req_url = win->current_node->previous->aurl_wa;
        	rds.post_data = NULL;
        	rds.ct = NULL;
        	rds.is_reloading = False;
        	rds.gui_action = HTML_LOAD_CALLBACK;
		win->navigation_action = NAVIGATE_BACK;
        	MMPafLoadHTMLDocInWin(win, &rds);
		win->navigation_action = NAVIGATE_NEW; /* reset to default */
		return;
	}
/* new_node->node_type == NODE_FRAMESET_TYPE */
/*re-creer le frameset comme dans paf.c */

	win->frame_type = FRAMESET_TYPE;
#ifdef MULTICAST
	win->dot=NULL;
	win->n_do = 0;
#endif
	HTMLSetFrameSet (win->scrolled_win, new_node->htinfo->mlist,
		new_node->aurl, new_node->htinfo->nframes,
		new_node->htinfo->frameset_info, &htmlw_tabs);
/* on met a jour immediatement la partie navigation. */
	win->current_node = new_node;
	win->navigation_action = NAVIGATE_OVERWRITE;
	mo_set_win_headers(win, new_node->aurl_wa, new_node->htinfo->title);
	XFlush(XtDisplay(win->scrolled_win));

/* init parent FRAMESET */
	win->frame_name = NULL;
	win->frame_parent =NULL;
	win->frame_sons = NULL;
	win->frame_sons_nbre =  new_node->htinfo->nframes;
	win->number_of_frame_loaded = new_node->htinfo->nframes;
	win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre , sizeof(mo_window *));
/* creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
	for(i = 0 ; i < new_node->nframe; i++ )  {
		Widget htmlw;
		mo_window *sub_win;
		char * url,*frame_name;

		htmlw = htmlw_tabs[i];
		url = strdup(new_node->frame_tab[i].aframe_src);
		frame_name = new_node->htinfo->frameset_info->frames[i].frame_name;
		sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
		sub_win->frame_sons_nbre = 0;
		sub_win->frame_dot_index = i;
		win->frame_sons[i] = sub_win;
		rds.ct = rds.post_data = NULL;
		rds.is_reloading = False;
		rds.req_url = mo_url_canonicalize_keep_anchor(url,
			win->current_node->base_url);
		rds.gui_action = FRAME_LOADED_FROM_FRAMESET;
		sub_win->navigation_action = NAVIGATE_BACK; /* doesnot really matter */
		sub_win->navigation_target_type = NAVIGATE_TARGET_ROOT; /* because of TARGET_ROOT */
		MMPafLoadHTMLDocInFrame (sub_win, &rds);
		free(url);     
		free(rds.req_url);
	}
}

/* Back up a node. */
void mo_back (Widget w, XtPointer clid, XtPointer calld)
{
	RequestDataStruct rds;
	mo_window *ewin = (mo_window*) clid; /* event win */
	mo_window *stop_win ; 		/* win ou on envoie le stop */

	switch(ewin->frame_type) {
	case NOTFRAME_TYPE:
		stop_win = ewin;
		break;
	case FRAMESET_TYPE:
		stop_win = ewin;
		break;
	case FRAME_TYPE:
		stop_win = ewin->frame_parent;
		break;
	}

	if (stop_win->pafd )		/* transfert in progress */
		(*stop_win->pafd->call_me_on_stop)(stop_win->pafd); /* stop it */
/* If there is no previous node, choke. */
	if (!stop_win->current_node || stop_win->current_node->previous == NULL){
		fprintf(stderr,"Severe Bug , Please report...\n");
		fprintf(stderr,"mo_back: current_node->previous == NULL\n");
		fprintf(stderr,"Aborting...\n");
		assert(0);
	}

	switch(ewin->frame_type) {
	case NOTFRAME_TYPE:
		back_from_html(stop_win, stop_win->current_node, stop_win->current_node->previous);
		return;
	case FRAMESET_TYPE:
	case FRAME_TYPE:
		back_from_frameset(stop_win,stop_win->current_node, stop_win->current_node->previous);
		return;
	}
}

static void forward_from_frameset(mo_window *win, mo_node *cur_node, mo_node *new_node)
{
	RequestDataStruct rds;
	Widget * htmlw_tabs;
	int i;

	if( new_node->node_type == NODE_NOTFRAME_TYPE) { /* new is html */
        	rds.req_url = win->current_node->next->aurl_wa;
        	rds.post_data = NULL;
        	rds.ct = NULL;
        	rds.is_reloading = False;
        	rds.gui_action = HTML_LOAD_CALLBACK;
		win->navigation_action = NAVIGATE_FORWARD;
        	MMPafLoadHTMLDocInWin(win, &rds);
		win->navigation_action = NAVIGATE_NEW; /* reset to default */
		return;
	}
/* new_node->node_type == NODE_FRAMESET_TYPE */

	if (new_node->win_type == FRAME_TYPE ) {
/* we FORWARD in same frameset : load only frame */
		mo_window * fwin;
		int findex;

		findex = new_node->num_frame_target;
		fwin = win->frame_sons[findex];
		rds.req_url = new_node->frame_tab[findex].aframe_src;
		rds.post_data = NULL;
                rds.ct = NULL;
                rds.is_reloading = False;
                rds.gui_action = HTML_LOAD_CALLBACK;
                fwin->navigation_action = NAVIGATE_FORWARD;
		fwin->navigation_target_type = NAVIGATE_TARGET_SELF;
		MMPafLoadHTMLDocInFrame(fwin, &rds);
		return;
	}

/* new_node->win_type == FRAMESET_TYPE */
/* we FORWARD from frameset in other frameset */

/* dewrap old (voir paf.c HTMLUnsetFrameSet ) */

#ifdef MULTICAST     
	win->mc_sbh_value = 0;
	win->mc_sbv_value = 0;
	win->mc_send_scrollbar_flag = False;
#endif               
	for(i = 0; i < win->frame_sons_nbre; i++) {
/* stop old plugins if exist */
#ifdef OBJECT           
		if (win->frame_sons[i]->htinfo) {
			MMStopPlugins(win->frame_sons[i],
			win->frame_sons[i]->htinfo->mlist);
		}
#endif                  
		MMDestroySubWindow(win->frame_sons[i]);
		win->frame_sons[i] = NULL; /* sanity */
	}    
	free(win->frame_sons);
	win->frame_sons = NULL;
	HTMLUnsetFrameSet (win->scrolled_win);
	win->frame_type = NOTFRAME_TYPE;

/* create new one from new_node  (comme forward_from_html) */

	win->frame_type = FRAMESET_TYPE;
#ifdef MULTICAST
	win->dot=NULL;
	win->n_do = 0;
#endif
	HTMLSetFrameSet (win->scrolled_win, new_node->htinfo->mlist,
		new_node->aurl, new_node->htinfo->nframes,
		new_node->htinfo->frameset_info, &htmlw_tabs);
/* on met a jour immediatement la partie navigation. */
	win->current_node = new_node;
	win->navigation_action = NAVIGATE_OVERWRITE;
	mo_set_win_headers(win, new_node->aurl_wa, new_node->htinfo->title);
	XFlush(XtDisplay(win->scrolled_win));

/* init parent FRAMESET */
	win->frame_name = NULL;
	win->frame_parent =NULL;
	win->frame_sons = NULL;
	win->frame_sons_nbre =  new_node->htinfo->nframes;
	win->number_of_frame_loaded = new_node->htinfo->nframes;
	win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre , sizeof(mo_window *));
/* creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
	for(i = 0 ; i < new_node->nframe; i++ )  {
		Widget htmlw;
		mo_window *sub_win;
		char * url,*frame_name;

		htmlw = htmlw_tabs[i];
		url = strdup(new_node->frame_tab[i].aframe_src);
		frame_name = new_node->htinfo->frameset_info->frames[i].frame_name;
		sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
		sub_win->frame_sons_nbre = 0;
		sub_win->frame_dot_index = i;
		win->frame_sons[i] = sub_win;
		rds.ct = rds.post_data = NULL;
		rds.is_reloading = False;
		rds.req_url = mo_url_canonicalize_keep_anchor(url,
			win->current_node->base_url);
		rds.gui_action = FRAME_LOADED_FROM_FRAMESET;
		sub_win->navigation_action = NAVIGATE_FORWARD; /* doesnot really matter */
		sub_win->navigation_target_type = NAVIGATE_TARGET_ROOT; /* because of TARGET_ROOT */
		MMPafLoadHTMLDocInFrame (sub_win, &rds);
		free(url);     
		free(rds.req_url);
	}
}

static void forward_from_html(mo_window *win, mo_node *cur_node, mo_node *new_node)
{
	RequestDataStruct rds;
	Widget * htmlw_tabs;
	int i;

	if( new_node->node_type == NODE_NOTFRAME_TYPE) {	/* same type */
        	rds.req_url = win->current_node->next->aurl_wa;
        	rds.post_data = NULL;
        	rds.ct = NULL;
        	rds.is_reloading = False;
        	rds.gui_action = HTML_LOAD_CALLBACK;
		win->navigation_action = NAVIGATE_FORWARD;
        	MMPafLoadHTMLDocInWin(win, &rds);
		win->navigation_action = NAVIGATE_NEW; /* reset to default */
		return;
	}
/* new_node->node_type == NODE_FRAMESET_TYPE */
/*re-creer le frameset comme dans paf.c */

	win->frame_type = FRAMESET_TYPE;
#ifdef MULTICAST
	win->dot=NULL;
	win->n_do = 0;
#endif
	HTMLSetFrameSet (win->scrolled_win, new_node->htinfo->mlist,
		new_node->aurl, new_node->htinfo->nframes,
		new_node->htinfo->frameset_info, &htmlw_tabs);
/* on met a jour immediatement la partie navigation. */
	win->current_node = new_node;
	win->navigation_action = NAVIGATE_OVERWRITE;
	mo_set_win_headers(win, new_node->aurl_wa, new_node->htinfo->title);
	XFlush(XtDisplay(win->scrolled_win));

/* init parent FRAMESET */
	win->frame_name = NULL;
	win->frame_parent =NULL;
	win->frame_sons = NULL;
	win->frame_sons_nbre =  new_node->htinfo->nframes;
	win->number_of_frame_loaded = new_node->htinfo->nframes;
	win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre , sizeof(mo_window *));
/* creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
	for(i = 0 ; i < new_node->htinfo->nframes; i++ )  {
		Widget htmlw;
		mo_window *sub_win;
		char * url,*frame_name;

		htmlw = htmlw_tabs[i];
		url = strdup(new_node->frame_tab[i].aframe_src);
		frame_name = new_node->htinfo->frameset_info->frames[i].frame_name;
		sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
		sub_win->frame_sons_nbre = 0;
		sub_win->frame_dot_index = i;
		win->frame_sons[i] = sub_win;
		rds.ct = rds.post_data = NULL;
		rds.is_reloading = False;
		rds.req_url = mo_url_canonicalize_keep_anchor(url,
			win->current_node->base_url);
		rds.gui_action = FRAME_LOADED_FROM_FRAMESET;
		sub_win->navigation_action = NAVIGATE_FORWARD; /* doesnot really matter */
		sub_win->navigation_target_type = NAVIGATE_TARGET_ROOT; /* because of TARGET_ROOT */
		MMPafLoadHTMLDocInFrame (sub_win, &rds);
		free(url);     
		free(rds.req_url);
	}
}

/* Go forward a node. */
void mo_forward (Widget w, XtPointer clid, XtPointer calld)
{
	RequestDataStruct rds;
	mo_window *ewin = (mo_window*) clid;
	mo_window *stop_win ;           /* win ou on envoie le stop */

        switch(ewin->frame_type) {     
        case NOTFRAME_TYPE:            
                stop_win = ewin;       
                break;                 
        case FRAMESET_TYPE:            
                stop_win = ewin;       
                break;                 
        case FRAME_TYPE:               
                stop_win = ewin->frame_parent;
                break;                 
        }

	if (stop_win->pafd )		/* transfert in progress */
		(*stop_win->pafd->call_me_on_stop)(stop_win->pafd); /* stop it */
	/* If there is no next node, choke. */
	if (!stop_win->current_node || stop_win->current_node->next == NULL) {
		fprintf(stderr,"Severe Bug , Please report...\n");
		fprintf(stderr,"mo_forward: current_node->previous == NULL\n");
		fprintf(stderr,"Aborting...\n");
		assert(0);
	}

	switch(ewin->frame_type) {
	case NOTFRAME_TYPE:
		forward_from_html(stop_win, stop_win->current_node, stop_win->current_node->next);
		return;
	case FRAMESET_TYPE:
	case FRAME_TYPE:
		forward_from_frameset(stop_win,stop_win->current_node, stop_win->current_node->next);
		return;
	}
}

/* Visit an arbitrary position.  This is called when a history
 * list entry is double-clicked upon.

 * Iterate through the window history; find the mo_node associated
 * with the given position.  Call mo_set_win_current_node.
 */

static void mo_visit_position (mo_window *win, int pos)
{
	mo_hnode *node;
	int cnt = 0;

	for (node = win->hist_node; node != NULL; node = node->next) {
		cnt++;
		if (cnt == pos) {
			char *xurl = node->aurl;
			RequestDataStruct rds;
			rds.req_url = xurl;
			rds.gui_action = HTML_LOAD_CALLBACK;
			rds.post_data = NULL; 
			rds.ct = NULL; 
			rds.is_reloading = False;
			win->navigation_action = NAVIGATE_NEW;
			MMPafLoadHTMLDocInWin(win, &rds);
			return;
		}
	}
	fprintf (stderr, "UH OH BOSS, asked for position %d, ain't got it.\n",
		pos);
	assert(0);
}

/* ----------------------------- HISTORY GUI ------------------------------ */

/* We've just init'd a new history list widget; look at the window's
   history and load 'er up. */
static void mo_load_history_list (mo_window *win, Widget list)
{
	mo_hnode *node;

	for (node = win->hist_node; node != NULL; node = node->next) {
		XmString xmstr = XmxMakeXmstrFromString (node->title);

		XmListAddItemUnselected (list, xmstr, 0);
		XmStringFree (xmstr);
	}
	XmListSetBottomPos(list, 0);
	if (win->hist_node)
		XmListSelectPos(win->history_list_w, 0, False);
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
			node->htinfo->title, node->aurl_wa, node->aurl_wa);
	}
	fprintf (fp, "</DL>\n");
	fprintf (fp, "</HTML>\n");
	mo_finish_sending_mail_message ();
	free (to);
	free (subj);
}

static void mo_post_mailhist_win (mo_window *win)
{
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget mailhist_form, to_label, subj_label;

	assert(win->history_shell);

	if (win->mailhist_win) {
		XtManageChild (win->mailhist_win);
		return ;
	}

/* Create it for the first time. */
	win->mailhist_win = XmxMakeFormDialog(win->history_shell, 
		"mMosaic: Mail Window History" );
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
	XmxSetConstraints (to_label,
		XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
		NULL, NULL, NULL, NULL);
	XmxSetOffsets (win->mailhist_to_text, 10, 0, 5, 10);
	XmxSetConstraints (win->mailhist_to_text,
		XmATTACH_FORM, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
		NULL, NULL, to_label, NULL);
	XmxSetOffsets (subj_label, 14, 0, 10, 0);
	XmxSetConstraints(subj_label,
		XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
		win->mailhist_to_text, NULL, NULL, NULL);
	XmxSetOffsets (win->mailhist_subj_text, 10, 0, 5, 10);
	XmxSetConstraints(win->mailhist_subj_text,
		XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
		win->mailhist_to_text, NULL, subj_label, NULL);
	XmxSetArg (XmNtopOffset, 10);
	XmxSetConstraints(dialog_sep,
		XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
		win->mailhist_subj_text, buttons_form, NULL, NULL);
	XmxSetConstraints(buttons_form,
		XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM,
		NULL, NULL, NULL, NULL);
	XtManageChild (win->mailhist_win);
	return ;
}

/* ---------------------------- history_win_cb ---------------------------- */

static XmxCallback (history_win_cb0)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->history_shell);
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

static char hlistTranslations[] =
	"~Shift ~Ctrl ~Meta ~Alt <Btn2Down>: ListKbdSelectAll() ListBeginSelect() \n~Shift ~Ctrl ~Meta ~Alt <Btn2Up>:   ListEndSelect() ListKbdActivate()";

/* Create history popup and popup it */
void mo_post_history_win (mo_window *win)
{
	Widget dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget history_label;
	Widget history_form;
	XtTranslations listTable;

	if(win->history_shell) {		/* exist? : yes popup */
		XmxManageRemanage (win->history_shell);
		return ;
	}

/* if no window, create */
	listTable = XtParseTranslationTable(hlistTranslations);
      
/* Create it for the first time. */
	win->history_shell = XmxMakeFormDialog(win->base,
		"mMosaic: Window History");
	dialog_frame = XmxMakeFrame (win->history_shell, XmxShadowOut);
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
	win->history_list_w = XmxMakeScrolledList(history_form, 
		history_list_cb, (XtPointer) win);
	XtAugmentTranslations (win->history_list_w, listTable);

	dialog_sep = XmxMakeHorizontalSeparator (history_form);
	buttons_form = XmxMakeFormAndThreeButtons(history_form, 
		"Mail To..." , "Dismiss" , "Help..." , 
		history_win_cb1, history_win_cb0, history_win_cb2,
		(XtPointer) win);

/* Constraints for history_form. */
	XmxSetOffsets (history_label, 8, 0, 10, 10);
	XmxSetConstraints(history_label,
		XmATTACH_FORM, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_NONE,
		NULL, NULL, NULL, NULL);

/* History list is stretchable. */
	XmxSetOffsets (XtParent (win->history_list_w), 0, 10, 10, 10);
	XmxSetConstraints (XtParent (win->history_list_w), 
		XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
		history_label, dialog_sep, NULL, NULL);
	XmxSetArg (XmNtopOffset, 10);
	XmxSetConstraints (dialog_sep, 
		XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
		NULL, buttons_form, NULL, NULL);
	XmxSetConstraints(buttons_form,
		XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM,
		NULL, NULL, NULL, NULL);

/* Go get the history up to this point set up... */
	mo_load_history_list (win, win->history_list_w);

	XmxManageRemanage (win->history_shell);	/* popup */
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
        win->navigation_action = NAVIGATE_NEW;
        MMPafLoadHTMLDocInWin(win, &rds);
}

static void mo_add_to_rbm_history(mo_window *win, char *aurl, char *ti)
{ 
        char label[32];
        int max = mMosaicAppData.numberOfItemsInRBMHistory;
        int i; 
	mo_hnode *next=win->hist_node;
	mo_hnode *hn;

	hn = win->hist_node;
	while (hn != NULL) {	/* test if same exist */
		if ( !strcmp(hn->aurl, aurl) ) {
			return;
		}
		hn = hn->next;
	}

	hn = (mo_hnode *) calloc(1, sizeof(mo_hnode));
	hn->aurl = strdup(aurl);
	hn->title = strdup(ti);
	hn->next = next;
	win->hist_node = hn;

	if (win->history_list_w) {
		XmString xmstr = XmxMakeXmstrFromString(hn->title);
		XmListAddItemUnselected(win->history_list_w, xmstr, 1);
		XmStringFree (xmstr);
	}
        
        if(!win->session_menu)
                win->session_menu = XmCreatePulldownMenu(win->view,
                                "session_menu", NULL, 0);
        
        compact_string(hn->title, label, 31, 3, 3);
        
        if(win->num_session_items < max) {
                win->session_items[win->num_session_items] =
                        XtVaCreateManagedWidget(label, xmPushButtonGadgetClass,            
                                win->session_menu,
                                XmNuserData, (XtPointer) win,
                                NULL);
                XtAddCallback(win->session_items[win->num_session_items],
                        XmNactivateCallback, session_cb, hn->aurl);
                XtAddCallback(win->session_items[win->num_session_items],
                        XmNarmCallback, rbm_ballonify, hn->aurl);
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
                XmNactivateCallback, session_cb, hn->aurl);
                XtAddCallback(win->session_items[max-1],
                        XmNarmCallback, rbm_ballonify, hn->aurl);
                XtAddCallback(win->session_items[max-1],
                        XmNdisarmCallback, rbm_ballonify, " ");
        }                          
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
