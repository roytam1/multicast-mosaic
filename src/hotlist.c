/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdio.h>
#include <time.h>
#include <Xm/List.h>
#include <Xm/TextF.h>
#include <Xm/ToggleBG.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include "../libhtmlw/HTMLparse.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "../libhtmlw/HTMLP.h"
#include "../libhtmlw/HTMLPutil.h"
#include "bitmaps/hotlist.xbm"
#include "../libnut/system.h"
#include "hotlist.h"
#include "gui.h"
#include "gui-popup.h"
#include "gui-dialogs.h"
#include "gui-documents.h"
#include "util.h"
#include "mailto.h"
#include "mime.h"
#include "paf.h"

#ifdef DEBUG
#define DEBUG_GUI
#endif

/* ####Prendre le format netscape: Voir ~/.netscape/bookmark.html##### */
/* <!DOCTYPE NETSCAPE-Bookmark-file-1>
/*
/* <!-- This is an automatically generated file.
/* It will be read and overwritten.
/* Do Not Edit! -->
/* <TITLE>Gilles Dauphin's Bookmarks</TITLE>
/* <H1>Gilles Dauphin's Bookmarks</H1>
/* 
/* <DD>primarybook
/* <DL><p>
/*      <DT><A HREF="http://sig.enst.fr/" ADD_DATE="790876892" LAST_VISIT="874571973" LAST_MODIFIED="869758245">Télécom Paris WWW Server</A>
/*      <DT><A HREF="http://www.ensta.fr/" ADD_DATE="790962869" LAST_VISIT="869757049" LAST_MODIFIED="869559368">Serveur WWW de l'ENSTA</A>
/* <DD>Oh le beau serveur de l'ENSTA
/*     <DT><H3 FOLDED ADD_DATE="842436201">java tool</H3>
/* <DD>Different site java
/*     <DL><p>
/*         <DT><A HREF="http://www.kaffe.org/" ADD_DATE="861286867" LAST_VISIT="869757074" LAST_MODIFIED="869680045">KAFFE - A virtual machine to run Java(tm)* code</A>
/*     </DL><p>
/*     <DT><H3 FOLDED ADD_DATE="842434326">midi</H3>
/* <DD>midi pointers                      
/*     <DL><p>                            
/*         <DT><A HREF="http://www.hut.fi/~titoivon/" ADD_DATE="863770674" LAST_VISIT="869756961" LAST_MODIFIED="861537900">Tuukka Toivonen / TiMidity</A>
/*     </DL><p>
/*     <DT><A HREF="http://www.netstore.de/Supply/http-analyze/" ADD_DATE="872183226" LAST_VISIT="872183437" LAST_MODIFIED="872183206">http-analyze - A fast Log Analyzer for web servers</A>
/* </DL><p>
/*           
*/

#define CHK_OUT_OF_MEM(x) { if ( x == NULL) {\
                                fprintf(stderr,"Out of memory\n");\
                                exit(1);\
                             }}

#define LISTINDIC "-> "

#define FindHotFromPos(hotnode, list, posi)			\
  do for (hotnode = list->nodelist; hotnode != NULL;		\
	  hotnode = hotnode->any.next)				\
    {								\
      if (hotnode->any.position == posi)			\
        break;							\
    } while(0)

typedef struct _edit_or_insert_hot_info {
	Widget title_text;
	int pos;
	Widget url_lab;
	Widget url_text;
	Widget comment_text;
	Widget tog_url;
	Widget tog_list;
	Widget insert_tog;
} edit_or_insert_hot_info;

mo_hotlist *mMosaicHotList = NULL;
 
static char * lhot_file_name = NULL;

/* Given a hotlist and a hotnode, append the node to the hotlist.
 * Change fields nodelist and nodelist_last in the hotlist, and fields next
 * and previous in the hotnode. Also fill in field position in the hotnode.
 */
void mo_append_item_to_hotlist (mo_hotlist *list, mo_hot_item *node)
{
	if (node->type == mo_t_list)
		node->list.parent = list;
	if (list->nodelist == 0) { /* Nothing yet. */
		list->nodelist = node;
		list->nodelist_last = node;
		node->any.next = 0;
		node->any.previous = 0;
		node->any.position = 1;
	} else { /* The new node becomes nodelist_last. */
/* But first, set up node. */
		node->any.previous = list->nodelist_last;
		node->any.next = 0;
		node->any.position = node->any.previous->any.position + 1;
      
/* Now point forward from previous nodelist_last. */
		list->nodelist_last->any.next = node;
      
/* Now set up new nodelist_last. */
		list->nodelist_last = node;
	}
}

/* Given a hotlist and a hotnode, rip the hotnode out of the hotlist.
   No check is made as to whether the hotnode is actually in the hotlist;
   it better be. */
static void mo_remove_hotnode_from_hotlist (mo_hotlist *list,
                                            mo_hot_item *hotnode)
{
	if (hotnode->any.previous == NULL) {
/* Node was the first member of the list. */
		if (hotnode->any.next != NULL) {
/* Node was the first member of the list and had a next node. */
/* The next node is now the first node in the list. */
			hotnode->any.next->any.previous = NULL;
			list->nodelist = hotnode->any.next;
		} else {
/* Node was the first member of the list and didn't have a next node. */
/* The list is now empty. */
			list->nodelist = NULL;
			list->nodelist_last = NULL;
		}
	} else {	 /* Node had a previous. */
		if (hotnode->any.next != NULL) {
/* Node had a previous and a next. */
			hotnode->any.previous->any.next = hotnode->any.next;
			hotnode->any.next->any.previous = hotnode->any.previous;
		} else { /* Node had a previous but no next. */
			hotnode->any.previous->any.next = NULL;
			list->nodelist_last = hotnode->any.previous;
		}
	}
	if (hotnode->type == mo_t_list) {
		mo_hot_item *item, *prev;
		for (item = hotnode->list.nodelist; item; free(prev)) {
			mo_remove_hotnode_from_hotlist (&(hotnode->list), item);
			prev = item; item = hotnode->list.nodelist;
		}
	}
}

/* Go through a hotlist  and assign position numbers for all of 'em. */
static void mo_recalculate_hotlist_positions (mo_hotlist *list)
{
	mo_hot_item *hotnode;
	int count = 1;

	for(hotnode=list->nodelist; hotnode != NULL; hotnode=hotnode->any.next)
		hotnode->any.position = count++;
}

/* Insert item in list at given position. If position is 0, then append it */

static void mo_insert_item_in_hotlist(mo_window * win,
	mo_hotlist *list, mo_hot_item *node, int position)
{
	if (!position) {
		mo_append_item_to_hotlist (list, node);
	} else {
		if (node->type == mo_t_list)
			node->list.parent = list;
		if (list->nodelist == 0) { /* Nothing yet. */
			list->nodelist = node;
			list->nodelist_last = node;
			node->any.next = 0;
			node->any.previous = 0;
			node->any.position = 1;
		} else {
			mo_hot_item *item, **prevNextPtr = &list->nodelist;
/* search the item at position 'position' */
			for (item = list->nodelist; item != NULL; item = item->any.next) {
				if (item->any.position == position)
					break;
				prevNextPtr = &item->any.next;
			}
			if (item == NULL)	/* item not found */
				mo_append_item_to_hotlist (list, node);
			else {
				*prevNextPtr = node;
				node->any.previous = item->any.previous;
				node->any.next = item;
				item->any.previous = node;
				mo_recalculate_hotlist_positions (list);
			}
		}
	}
}

/* Go Up The tree to check if a list is the ancestor of an item */

static int mo_is_ancestor (mo_hotlist *list, mo_hotlist *item)
{
	while (item && item != list)
		item = item->parent;
	return item == list;
}

/* recursive function that copy a hierarchy of hotlist */
static mo_hotlist *mo_copy_hot_hier (mo_hotlist *list)
{
	mo_hot_item *item;
	mo_hotnode *hot;
	mo_hotlist *hotlist = (mo_hotlist *)calloc(1, sizeof(mo_hotlist));

	hotlist->title = strdup(list->title);
	hotlist->desc = list->desc? strdup(list->desc) : (char*) NULL;
	hotlist->type = mo_t_list;
	hotlist->nodelist = hotlist->nodelist_last = 0;
	for (item = list->nodelist; item; item = item->any.next)
		if (item->type == mo_t_url) {
			hot = (mo_hotnode *)calloc(1,sizeof(mo_hotnode));
			hot->type = mo_t_url;
			hot->title = strdup(item->hot.title);
			hot->url = strdup(item->hot.url);
			hot->desc = item->hot.desc? strdup(item->hot.desc):
							(char*)NULL;
/*hot->lastdate = strdup(item->hot.lastdate);*/
			hot->lastdate = (char *) 0;
			mo_append_item_to_hotlist(hotlist, (mo_hot_item *)hot);
		} else {
			mo_append_item_to_hotlist(hotlist,
				(mo_hot_item *)mo_copy_hot_hier((mo_hotlist *)item));
		}
	return hotlist;
}

static char * mo_compute_hot_path (mo_hotlist *curr)
{
	char *str;
	char *prev = curr->parent ? strdup(curr->title) : strdup("/");

	for (str = prev, curr = curr->parent; curr; curr = curr->parent) {
		if (curr->parent) {
			str = (char *)malloc(strlen(prev)+strlen(curr->title)+2);
			strcat(strcat(strcpy(str, curr->title), "/"),prev);
		} else {
			str = (char *)malloc(strlen(prev)+2);
			strcat(strcpy(str, "/"), prev);
		}
		free(prev);
		prev = str;
	}
	return str;
}

static void mo_copy_hotlist_position (mo_window *win, int position)
{
	mo_hot_item *item;

	for (item = win->current_hotlist->nodelist; item != NULL && item->any.position != position; item = item->any.next) ;
		if (item)
			win->hot_cut_buffer = item;
}

static char * mo_highlight_hotlist (mo_hotlist *list)
{
	char *str = (char *)malloc(strlen(list->title)+strlen(LISTINDIC)+1);
	return strcat(strcpy(str,LISTINDIC), list->title);
}

static void mo_gui_add_hot_item (mo_hotlist *list, mo_hot_item *item)
{
	mo_window *win = NULL;

		/* Now we've got to update all active hotlist_list's. */
	while ( (win = mo_main_next_window (win)) )
		if (win->hotlist_list && win->current_hotlist == list) {
			char *highlight = NULL;

			XmString xmstr = XmxMakeXmstrFromString(
				item->type == mo_t_url ? item->hot.title :
				(highlight = mo_highlight_hotlist(&item->list)));
			if (item->type == mo_t_list && highlight)
				free(highlight);
			XmListAddItemUnselected (win->hotlist_list, xmstr,
					item->any.position);
			XmStringFree (xmstr);
			XmListSetBottomPos (win->hotlist_list, 0);
		}
}

mo_status mo_add_item_to_hotlist (mo_hotlist *list, mo_item_type type,
           char *title, char *url, char* desc, int position)
{
	mo_hot_item *item;
	mo_window *win = NULL;

	if((title == NULL || title[0] == '\0') && (url == NULL || url[0] == '\0'))
		return mo_fail;
	if (desc == NULL || *desc == '\0')
		desc = NULL;
	if (type == mo_t_url) {
		mo_hotnode *hotnode = (mo_hotnode*)calloc(1,sizeof(mo_hotnode));
		time_t foo = time (NULL);
		char *ts = ctime (&foo);

		item = (mo_hot_item *)hotnode;
		ts[strlen(ts)-1] = '\0';
		hotnode->type = mo_t_url;
		if (title)
			hotnode->title = strdup (title);
		else
			hotnode->title = strdup ("Unnamed");
		hotnode->desc = desc ? strdup(desc) : (char*) NULL;
		mo_convert_newlines_to_spaces (hotnode->title);
		mo_convert_newlines_to_spaces (hotnode->desc);
		hotnode->url = strdup (url);
		mo_convert_newlines_to_spaces (hotnode->url);
		hotnode->lastdate = strdup (ts);
	} else {
		mo_hotlist *hotlist =(mo_hotlist *)calloc(1,sizeof(mo_hotlist));

		item = (mo_hot_item *)hotlist;
		hotlist->type = mo_t_list;
		if (title)
			hotlist->title = strdup (title);
		else
			hotlist->title = strdup ("Unnamed");
		hotlist->desc = desc ? strdup(desc):(char*)NULL;
		mo_convert_newlines_to_spaces (hotlist->title);
		mo_convert_newlines_to_spaces (hotlist->desc);
		hotlist->nodelist = hotlist->nodelist_last = 0;
	}
	if (position)
		mo_insert_item_in_hotlist(win, list, item, position);
	else
		mo_append_item_to_hotlist (list,  item);
	mo_gui_add_hot_item (list, item);
	return mo_succeed;
}

static int notSpacesOrNewLine(char *s)
{
	int retc = 0;

	for (;*s && !retc; s++)
		if (!isspace(*s)) retc = 1;
	return retc;
}

/* extract an hotlist from any HTML document */
static void mo_extract_anchors(mo_hotlist *list, struct mark_up *mptr)
{
	mo_hotnode *node;
	char *name = NULL;
	char *last_text = NULL;
	char *url = NULL, *title = NULL;

	for (; mptr != NULL; mptr = mptr->next)
	switch (mptr->type) {
	case M_TITLE:		/* title tag */
		name = mo_convert_newlines_to_spaces (strdup(mptr->pcdata));
		break;
	case M_NONE:		/* text, not tag */
		if (notSpacesOrNewLine(mptr->text))
			last_text = mptr->text;
		break;
	case M_ANCHOR:
		if (!mptr->is_end) {			/* start anchor tag */
			last_text = NULL;
			url = ParseMarkTag(mptr->start, MT_ANCHOR, AT_HREF);
			title = ParseMarkTag(mptr->start, MT_ANCHOR, "title");
		} else {			/* end anchor tag */
			node = (mo_hotnode *)calloc (1,sizeof (mo_hotnode));
			node->type = mo_t_url;
			node->url = url;
/* if there is a title attribute in the anchor, take it,
 * otherwise take the last text */
			node->title = title ? title :
				(last_text ? strdup(last_text):strdup("Unnamed"));
			mo_convert_newlines_to_spaces (node->title);
			mo_append_item_to_hotlist (list, (mo_hot_item *)node);
			url = title = last_text = NULL;
		}
	default:
		break;
	}
	if(!name)
		name = strdup("Unnamed");
	list->title = name;
}

/* parse a structured hotlist file recursively. We are pointing just after
 * a <DL> tag. So we can have:
 *	</DL>	empty list
 *	<DT>	an entry of type anchor or list
 * we must parse until </DL> tag
*/
static void mo_parse_hotlist_list(mo_hotlist *list, struct mark_up  **current)
{
	mo_hotlist *hotlist;
	mo_hotnode *node;
	char *last_text = NULL;
	char *url = NULL, *url_title = NULL, *url_desc=NULL;
	char * fold_title = NULL, *fold_desc = NULL;
	struct mark_up *mptr;
	int done = 0;
	mo_item_type entry_type = mo_t_none;
	int have_dd = 0;
  
	for (mptr = *current; mptr && !done; mptr = mptr->next)
	switch (mptr->type) {
	case M_DESC_TITLE:		/* dt tag */
/* a DT tag ends a previous anchor entry */
		if (entry_type == mo_t_url) {	/* add an url entry */
			node = (mo_hotnode *)calloc (1,sizeof (mo_hotnode));
			node->type = mo_t_url;
			node->url = url;
/* if there is a title attribute in the anchor, take it,
 * otherwise take the last text */
			node->title = url_title ;
			node->desc = url_desc ? strdup(url_desc) : (char*)NULL;
			mo_convert_newlines_to_spaces (node->title);
			mo_convert_newlines_to_spaces (node->desc);
			mo_append_item_to_hotlist (list, (mo_hot_item *)node);
		}
		last_text = url = url_title = url_desc = NULL;
		fold_title = fold_desc = NULL;
		have_dd = 0;
		entry_type = mo_t_none;
		break;
	case M_ANCHOR:
		if (!mptr->is_end) {			/* start anchor tag */
			entry_type = mo_t_url;
			fold_title = fold_desc = NULL;
			last_text = NULL;
			url = ParseMarkTag(mptr->start, MT_ANCHOR, AT_HREF);
			url_title = ParseMarkTag(mptr->start, MT_ANCHOR, "title");
		} else {			/* </A> tag */
			if (entry_type != mo_t_url)	/* connard! */
				break;
			url_title = url_title ? url_title :
				(last_text? strdup(last_text) :strdup("Unnamed"));
		}
		have_dd = 0;
		break;
	case M_HEADER_3:
		if ( ! mptr->is_end) { 	/* start */
			entry_type = mo_t_list;
			fold_title = NULL;
		} else {		/* end */
			fold_title = last_text? strdup(last_text) :strdup("Unnamed");
		}
		fold_desc = last_text = url = url_title = url_desc = NULL;
		have_dd = 0;
		break;
	case M_NONE:		/* text, not tag */
		if (notSpacesOrNewLine(mptr->text)){
			last_text = mptr->text;
			switch (entry_type){
			case mo_t_none:
				url_desc = fold_desc = NULL;
				break;
			case mo_t_url:
				url_desc = have_dd ? last_text : (char*)NULL;
				fold_desc = NULL;
				break;
			case mo_t_list:
				url_desc = NULL;
				fold_desc = have_dd ? last_text : (char*)NULL;
				break;
			}
		}
		have_dd = 0;
		break;
	case M_DESC_LIST:
		if (!mptr->is_end) {			/* start dl tag */
			hotlist = (mo_hotlist *)calloc(1,sizeof(mo_hotlist));
			hotlist->type = mo_t_list;
			hotlist->nodelist = hotlist->nodelist_last = 0;
			hotlist->parent = list;
			hotlist->title = fold_title;
			hotlist->desc = fold_desc ? strdup(fold_desc): (char*)NULL;
			mo_convert_newlines_to_spaces (hotlist->title);
			mo_convert_newlines_to_spaces (hotlist->desc);
			mo_append_item_to_hotlist(list, (mo_hot_item *)hotlist);
			mptr = mptr->next;
			mo_parse_hotlist_list(hotlist, &mptr);
/* after this call, mptr is positionned on the end dl tag */
		} else{			/* end dl tag */
/* a /DL tag ends a previous anchor entry */
			if (entry_type == mo_t_url) {	/* add an url entry */
				node=(mo_hotnode*)calloc(1,sizeof (mo_hotnode));
				node->type = mo_t_url;
				node->url = url;
/* if there is a title attribute in the anchor, take it,
 * otherwise take the last text */
				node->title = url_title ;
				node->desc = url_desc ? strdup(url_desc) : (char*)NULL;
				mo_convert_newlines_to_spaces (node->title);
				mo_convert_newlines_to_spaces (node->desc);
				mo_append_item_to_hotlist (list, (mo_hot_item *)node);
			}
			*current = mptr, done = 1;
		}
		last_text = url = url_title = url_desc = NULL;
		fold_title = fold_desc = NULL;
		have_dd = 0;
		entry_type = mo_t_none;
		break;
	case M_DESC_TEXT:		/* dd tag */
		have_dd = 1;
		break;
	default:
		break;
	} /* end switch and for */
	if (!done)
		*current = NULL;
}

/* Read a hotlist from a file. fill the hotlist list given as parameter */
static void mo_read_hotlist(mo_hotlist *list, FILE *fp)
{
	int done, normal, has_list, depth;
	long size;
	struct mark_up *hot_mark_up, *mptr;
	char *text;
	HtmlTextInfo * htinfo;

	fseek(fp, 0L, SEEK_END);
	size = ftell(fp);
	text = (char*)malloc(size+1);
	CHK_OUT_OF_MEM(text);
	fseek(fp, 0L, SEEK_SET);
	fread(text, (size_t)1, (size_t)size, fp);
	text[size] = '\0';

/* parse the HTML document */
	htinfo = HTMLParseRepair(text);
        if (htinfo->nframes) { /* I am a frameset */
		assert(0); 	/* Y a pas de frameset dans une hotlist */
        }
	hot_mark_up = htinfo->mlist;
	free(text);

/* some pre-processing to see if this is in hotlist format or if this is a
 * normal document. The algo is as follow:
 * if an anchor is outside a list or if there are more than one top level list,
 * then it is not in hotlist format. The 'normal' flag at the end of the
 * pre-processing tells if it is a normal document or a hotlist.
*/
	done = 0;
	normal = 0;
	has_list = 0;
	depth = 0;
	for (mptr = hot_mark_up; mptr != NULL && !done; mptr = mptr->next) {
		switch (mptr->type) {
		case M_ANCHOR:
			if (!depth)
				done = 1, normal = 1;
			break;
		case M_DESC_LIST:
			if (!mptr->is_end)	/* start dl list tag */
				if (!depth && has_list)
					done = 1, normal = 1;
				else
					depth++, has_list = 1;
			else			/* end unum list tag */
				depth--;
		default:
			break;
		}
		if (done)
			break;
	}
	if ( depth ) 
		normal = 1;
/* now we know what kind of file we are dealing with */
	if (normal) {
		mo_extract_anchors(list, hot_mark_up);
	} else {
		char *last_text = NULL;
		char *title = NULL;
		char *desc = NULL;
		int have_dd = 0;

		done = 0;
		for(mptr = hot_mark_up; mptr != NULL && !done; mptr=mptr->next) {
			switch (mptr->type) {
			case M_NONE:		/* text, not tag */
				if (notSpacesOrNewLine(mptr->text)) {
					last_text = mptr->text;
					desc = have_dd ? last_text : (char*)NULL;
				}
				have_dd = 0;
				break;
			case M_DESC_LIST:
				desc = desc ? strdup(desc) : (char*)NULL;
				done = 1;
				break;
			case M_TITLE:
				title = mptr->pcdata;
				have_dd = 0;
				break;
			case M_DESC_TEXT:
				have_dd =1;
				break;
			default:
				have_dd = 0;
				break;
			}
		}
/* after this loop, mptr is positionned just after the dl tag */
		desc = mo_convert_newlines_to_spaces(desc);
		title = title ? mo_convert_newlines_to_spaces(title) : (char*)NULL;
		list->title = title;
		list->desc = desc;
		mo_parse_hotlist_list(list, &mptr);
	}
	FreeHtmlTextInfo(htinfo);
	return;
}

/* This function replace '>', '<' and '&' by their entity references
 * and output them.
 */
static void fputExpanded(char *s, FILE *fp)
{
	for (;*s;s++)
		if (*s == '<')
			fputs("&lt;", fp);
		else if (*s == '>')
			fputs("&gt;", fp);
		else if (*s == '&')
			fputs("&amp;", fp);
		else
			putc(*s, fp);
}

/* recursive function called to write a hotlist out to a file */
static void mo_write_list_r(mo_hotlist *list, FILE *fp)
{
	mo_hot_item *item;

	if (! list->nodelist)
		return;
	for (item = list->nodelist; item != NULL; item = item->any.next)
		if (item->type == mo_t_url)  {   /* URL item */
			if (!(item->hot.url)) {       
				continue;             
			}                             
			fputs("<DT><A HREF=\"", fp);
			fputExpanded(item->hot.url, fp);
			fputs("\"> ", fp);
			fputExpanded(item->hot.title, fp);
			fputs("</A>", fp);
			if (item->hot.desc) {
				fputs("<DD>",fp);
				fputExpanded(item->hot.desc, fp);
			}
			fputs("\n",fp);
		} else {		/* list item */
			fputs("<DT><H3>",fp);
			fputExpanded(item->list.title,fp);
			fputs("</H3>", fp);
			if(item->list.desc) {
				fputs("<DD>",fp);
				fputExpanded(item->list.desc, fp);
			}
			fputs("\n<DL>\n",fp);
			mo_write_list_r(&(item->list), fp);
			fputs("\n</DL>\n",fp);
		}
}

/* Write a hotlist out to a file.
 * Return mo_succeed if everything goes OK; mo_fail else.
 */
mo_status mo_write_hotlist (mo_hotlist *list, FILE *fp)
{
	fprintf(fp,"<HTML>\n<HEAD>\n<TITLE>Hotlist from user %s</TITLE>\n</HEAD>\n<BODY>\n",mMosaicAppData.author_full_name);
	fprintf(fp,"<H1>Hotlist from user %s</H1>\n",mMosaicAppData.author_full_name);
	fprintf(fp,"<DD>primaryhotlist\n<DL>\n");
	mo_write_list_r(list, fp);
	fprintf(fp,"</DL>\n</BODY>\n</HTML>\n");
	return mo_succeed;
}

/* ------------------------- gui support routines ------------------------- */

/* We've just init'd a new hotlist list widget; look at the default
   hotlist and load 'er up. */
static void mo_load_hotlist_list (mo_window *win, Widget list)
{
	mo_hot_item *node;
  
	if (win->edithot_win && XtIsManaged(win->edithot_win))
		XtUnmanageChild (win->edithot_win);
	for (node = win->current_hotlist->nodelist; node != NULL; node = node->any.next) {
		char *highlight = NULL;
		XmString xmstr = XmxMakeXmstrFromString (node->type == mo_t_url ?
				( node->hot.title) :
				(highlight = mo_highlight_hotlist(&node->list)));

		if (node->type == mo_t_list && highlight)
			free(highlight);
		XmListAddItemUnselected (list, xmstr, 0);
		XmStringFree (xmstr);
	}
}

static void mo_visit_hotlist_position (mo_window *win, int position)
{
	mo_hot_item *hotnode;
	RequestDataStruct rds;

	for (hotnode = win->current_hotlist->nodelist; hotnode != NULL; hotnode = hotnode->any.next) {
		if (hotnode->any.position == position) {
			if (hotnode->type == mo_t_url){
				rds.req_url = hotnode->hot.url;
	rds.gui_action = HTML_LOAD_CALLBACK;
				rds.post_data = NULL;
				rds.ct = NULL;
				rds.is_reloading = False;
		win->navigation_action = NAVIGATE_NEW;
				MMPafLoadHTMLDocInWin(win,&rds);
			} else {
				char *path =mo_compute_hot_path(&(hotnode->list));

				win->current_hotlist = &(hotnode->list);
				XmListDeleteAllItems(win->hotlist_list);
				XmxTextSetString(win->hotlist_label, path);
				free(path);
				mo_load_hotlist_list(win, win->hotlist_list);
			}
		}
	}
}
/* ----------- This part deals with the Edit and Insert features ---------- */

static XmxCallback (edit_or_insert_hot_cb1)		/* Dismiss Edit */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->edithot_win);
}
static XmxCallback (edit_or_insert_hot_cb2) 		/* Help... (Edit) */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url("help-on-hotlist-view.html"));
}
static XmxCallback (edit_or_insert_hot_cb4) 		/* Dismiss Insert */
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->inserthot_win);
	win->hot_cut_buffer = NULL;
}
static XmxCallback (edit_or_insert_hot_cb5)		/* Help... (Insert) */
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url("help-on-hotlist-view.html"));
}
static XmxCallback (edit_or_insert_hot_cb0)		/* Commit Edit */
{
	mo_window *win = (mo_window*)client_data;
	char *title, *comment;
	edit_or_insert_hot_info *eht_info;
	mo_hotlist *list = win->current_hotlist;
	mo_hot_item *hotnode;
	mo_window *ww = NULL;
	XmString xmstr ;

	XmxSetArg (XmNuserData, (XtArgVal)&eht_info);
	XtGetValues (win->edithot_win, Xmx_wargs, Xmx_n);
	Xmx_n = 0;
	XtUnmanageChild (win->edithot_win);
	title = XmTextGetString (eht_info->title_text);
/*####Commit#####
	char *txt;
	txt = XmTextGetString (win->annotate_text);
	mo_modify_pan (win->editing_id,
			XmTextGetString (win->annotate_title), 
			XmTextGetString (win->annotate_author), 
			txt);
#####
*/
	comment = XmTextGetString(eht_info->comment_text);
/* OK,now position is still cached in win->edithot_pos. */
	FindHotFromPos(hotnode, list, eht_info->pos);
	if (hotnode == NULL)
		return;
/* OK, now we have the hotnode. */
	if (hotnode->type == mo_t_url)
		hotnode->hot.url = XmTextGetString( eht_info->url_text);
	else 
		if (!strcmp(hotnode->any.title, title))
			return;
	hotnode->any.title = title;
/*Save the hotlist before we screw something up.*/
	mo_write_default_hotlist ();
	while ((ww = mo_main_next_window(ww))) { /* Change the extant hotlists. */
		if(ww->hotlist_list && ww->current_hotlist ==win->current_hotlist) {
			char *highlight = NULL;
			xmstr = XmxMakeXmstrFromString(hotnode->type == mo_t_url ?
	(title) :
	(highlight = mo_highlight_hotlist(&hotnode->list)));
			if(hotnode->type==mo_t_list && highlight)
				free(highlight);
			XmListDeletePos (ww->hotlist_list, hotnode->any.position);
#ifdef DEBUG_GUI
	if (mMosaicSrcTrace) {
		fprintf (stderr, 
		   "ww->hotlist_list 0x%08x, xmstr 0x%08x, hotnode->position %d\n",
			ww->hotlist_list, xmstr, hotnode->any.position);
	}
#endif
/* There is what appears to be a Motif UMR here... */
			XmListAddItemUnselected (ww->hotlist_list, xmstr, 
					hotnode->any.position);
			XmStringFree (xmstr);
		}
		if(ww->hotlist_list && hotnode->type==mo_t_list &&
		    mo_is_ancestor((mo_hotlist *)hotnode, ww->current_hotlist)) {
			char *path = mo_compute_hot_path( ww->current_hotlist);
			XmxTextSetString(ww->hotlist_label, path);
			free(path);
		}
	} /* while */
}

static XmxCallback (edit_or_insert_hot_cb3)		/* Commit Insert */
{
	mo_window *win = (mo_window*)client_data;
	char *title, *comment;
	edit_or_insert_hot_info *eht_info;
	Boolean isUrl ;
	Boolean useIns;
	int *pos_list;
	int pos_cnt, posi = 0;
	mo_status addOk = mo_succeed;

	XmxSetArg (XmNuserData, (XtArgVal)&eht_info);
	XtGetValues (win->inserthot_win, Xmx_wargs, Xmx_n);
	Xmx_n = 0;
	isUrl = XmToggleButtonGadgetGetState(eht_info->tog_url);
	useIns = XmToggleButtonGadgetGetState(eht_info->insert_tog);
	XtUnmanageChild (win->inserthot_win);
	title = XmTextGetString (eht_info->title_text);
/*
####Commit#####
	char *txt;
	txt = XmTextGetString (win->annotate_text);
	mo_modify_pan (win->editing_id,
			XmTextGetString (win->annotate_title), 
			XmTextGetString (win->annotate_author), 
			txt);
#####
*/
	comment = XmTextGetString (eht_info->comment_text);
	if (useIns)
		if(XmListGetSelectedPos(win->hotlist_list, &pos_list,&pos_cnt) &&
		   pos_cnt) {
			posi = pos_list[0]; XtFree((char *)pos_list); /* DXP */
		}

	if (isUrl)
		addOk=mo_add_item_to_hotlist(win->current_hotlist,mo_t_url, title,
				XmTextGetString(eht_info->url_text), 
				XmTextGetString(eht_info->comment_text),
				posi);
	else {
		if(win->hot_cut_buffer && win->hot_cut_buffer->type==mo_t_list &&
		   (!strcmp(title, win->hot_cut_buffer->any.title)) &&
		   (!mo_is_ancestor((mo_hotlist *)win->hot_cut_buffer,
		   win->current_hotlist))) {
			mo_insert_item_in_hotlist (win, win->current_hotlist,
				(mo_hot_item *)mo_copy_hot_hier
				((mo_hotlist *)win->hot_cut_buffer), posi);
/* Now we've got to update all active hotlist_list's. */
			mo_gui_add_hot_item (win->current_hotlist, 
				win->current_hotlist->nodelist_last);
		} else
			addOk = mo_add_item_to_hotlist(win->current_hotlist, 
					mo_t_list, title, NULL, 
					XmTextGetString(eht_info->comment_text),
					posi);
	}
	if (addOk == mo_succeed)
		mo_write_default_hotlist ();
	win->hot_cut_buffer = NULL;
	mo_compute_hot_path(win->current_hotlist);   /* AMB do a redisplay here */
	XmListDeleteAllItems(win->hotlist_list);
	mo_load_hotlist_list(win, win->hotlist_list);
}

/* this is used to destroy the edit_or_insert_hot_info structure
 * called from the "destroyCallback" list. */
static XmxCallback (mo_destroy_hot)
{
	free (client_data);
}

/* show or hide the url info with respect to the URL toggle */
static XmxCallback (url_or_list_cb)
{
edit_or_insert_hot_info *eht_info = (edit_or_insert_hot_info *)client_data;
if (((XmToggleButtonCallbackStruct *)call_data)->set) {
XtManageChild(eht_info->url_lab);
XtManageChild(eht_info->url_text);
} else {
XtUnmanageChild(eht_info->url_lab);
XtUnmanageChild(eht_info->url_text);
}
}

/* If it don't exist, make it...
   If isInsert is True, then we create an Insert Dialog window, otherwise
   we create an Edit dialog window. */
static mo_status mo_create_ed_or_ins_hot_win (mo_window *win, int isInsert)
{
	Widget ed_or_ins_w, dialog_frame;
	Widget dialog_sep, buttons_form;
	Widget eht_form, title_label, url_label, comment_label, url_val,
		comment_val, sep2;
	edit_or_insert_hot_info *eht_info;
	Widget togm, togm2, insert_tog, append_tog;

	eht_info = (edit_or_insert_hot_info *) calloc(1,
				sizeof(edit_or_insert_hot_info));
	XmxSetArg (XmNuserData, (XtArgVal)eht_info);
/*	XmxSetArg (XmNresizePolicy, XmRESIZE_GROW); */
	ed_or_ins_w = XmxMakeFormDialog(win->hotlist_win,
		isInsert ? "Insert Hotlist Entry" : "Edit Hotlist Entry");
  	XtAddCallback(ed_or_ins_w, XmNdestroyCallback,mo_destroy_hot, eht_info);

	if (isInsert)
		win->inserthot_win = ed_or_ins_w;
	else
		win->edithot_win = ed_or_ins_w;
	dialog_frame = XmxMakeFrame (ed_or_ins_w, XmxShadowOut);

/* Constraints for ed_or_ins_w. */
	XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
  
/* Main form. */
	eht_form = XmxMakeForm (dialog_frame);
  
	title_label = XmxMakeLabel (eht_form, "Title:" );
	XmxSetArg (XmNwidth, 335);
	eht_info->title_text = XmxMakeTextField (eht_form);
	if (isInsert){	
  		XtAddCallback(eht_info->title_text, XmNactivateCallback,
			edit_or_insert_hot_cb3, (XtPointer)win);
	} else {
		XtAddCallback(eht_info->title_text, XmNactivateCallback,
			edit_or_insert_hot_cb0, (XtPointer)win);
	}
	eht_info->url_lab = url_label = XmxMakeLabel (eht_form, "URL:" );
	XmxSetArg (XmNwidth, 335);
	eht_info->url_text = url_val = XmxMakeTextField(eht_form);
	comment_label = XmxMakeLabel (eht_form, "Comments:" );
	XmxSetArg (XmNwidth, 335);
	XmxSetArg (XmNscrolledWindowMarginWidth, 8);
	XmxSetArg (XmNscrolledWindowMarginHeight, 8);
	XmxSetArg (XmNcursorPositionVisible, True);
	XmxSetArg (XmNeditable, True);
	XmxSetArg (XmNeditMode, XmMULTI_LINE_EDIT);
	XmxSetArg (XmNrows, 10);
	XmxSetArg (XmNcolumns, 40);
	eht_info->comment_text = comment_val = XmxMakeScrolledText(eht_form);
	dialog_sep = XmxMakeHorizontalSeparator (eht_form);

	if (isInsert) {
		togm = XmxMakeRadioBox(eht_form);
		eht_info->tog_url = XtVaCreateManagedWidget("toggle",
			xmToggleButtonGadgetClass, togm,
			XtVaTypedArg, XmNlabelString,
			XtRString, "URL" , strlen("URL")+1,
			XmNmarginHeight, 0,
			XmNset, True, NULL);
		XtAddCallback(eht_info->tog_url, XmNvalueChangedCallback,
			url_or_list_cb, (XtPointer)eht_info);
		eht_info->tog_list = XtVaCreateManagedWidget("toggle",
			xmToggleButtonGadgetClass, togm,
			XtVaTypedArg, XmNlabelString,
			XtRString, "List", strlen("List")+1,
			XmNmarginHeight, 0, NULL);
		togm2 = XmxMakeRadioBox(eht_form);
		eht_info->insert_tog = insert_tog = XtVaCreateManagedWidget(
			"toggle", xmToggleButtonGadgetClass, togm2,
			XtVaTypedArg, XmNlabelString,
			XtRString, "Insert" , strlen("Insert" )+1,
			XmNmarginHeight, 0, NULL);
		append_tog = XtVaCreateManagedWidget("toggle",
			xmToggleButtonGadgetClass, togm2,
			XtVaTypedArg, XmNlabelString,
			XtRString, "Append" , strlen("Append" )+1,
			XmNmarginHeight, 0,
			XmNset, True, NULL);
		sep2 = XmxMakeHorizontalSeparator (eht_form);
	}
  
	if( isInsert){	/* isInsert is 0 or 1 */
		buttons_form = XmxMakeFormAndThreeButtons(eht_form, 
			"Commit" , "Dismiss" , "Help..." ,
			edit_or_insert_hot_cb3, 
			edit_or_insert_hot_cb4, 
			edit_or_insert_hot_cb5, (XtPointer)win);
	}else {
		buttons_form = XmxMakeFormAndThreeButtons(eht_form, 
			 "Commit" , "Dismiss" , "Help..." ,
			edit_or_insert_hot_cb0, 
			edit_or_insert_hot_cb1, 
			edit_or_insert_hot_cb2, (XtPointer)win);
	}
/* Constraints for eht_form. */
	XmxSetOffsets (title_label, 14, 0, 10, 0);
	XmxSetConstraints(title_label, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetOffsets (eht_info->title_text, 10, 0, 5, 10);
	XmxSetConstraints(eht_info->title_text, XmATTACH_FORM, XmATTACH_NONE, 
		XmATTACH_WIDGET, XmATTACH_FORM, NULL, NULL, title_label, NULL);
  
	XmxSetOffsets (url_label, 12, 0, 10, 0);
	XmxSetConstraints(url_label, XmATTACH_WIDGET, XmATTACH_NONE,
		XmATTACH_FORM, XmATTACH_NONE, title_label, NULL, NULL, NULL);
	XmxSetOffsets (url_val, 8, 10, 5, 10);
	XmxSetConstraints (url_val, XmATTACH_WIDGET, XmATTACH_NONE,
		XmATTACH_WIDGET, XmATTACH_FORM, title_label, NULL, url_label, NULL);

/* text_label: top url, bottom nothing, left form, right nothing */
	XmxSetOffsets (comment_label, 12, 0, 10, 0);
	XmxSetConstraints(comment_label, XmATTACH_WIDGET, XmATTACH_NONE,
		XmATTACH_FORM, XmATTACH_NONE, url_label, NULL, NULL, NULL);

	XmxSetOffsets (XtParent(eht_info->comment_text), 8, 10, 5, 10);
	XmxSetConstraints (XtParent(eht_info->comment_text), 
		XmATTACH_WIDGET, XmATTACH_NONE, 
		XmATTACH_WIDGET, XmATTACH_FORM,
		url_label, NULL, comment_label, NULL);

	XmxSetArg (XmNtopOffset, 10);
	XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET,
		XmATTACH_FORM, XmATTACH_FORM, 
		XtParent(eht_info->comment_text),
		isInsert ? togm : buttons_form, NULL, NULL);
	if (isInsert) {
		XmxSetConstraints(togm, XmATTACH_NONE, XmATTACH_WIDGET,
			XmATTACH_FORM, XmATTACH_NONE, NULL, sep2, NULL, NULL);
		XmxSetPositions(togm, XmxNoPosition, XmxNoPosition, XmxNoPosition, 50);
		XmxSetConstraints(togm2, XmATTACH_NONE, XmATTACH_WIDGET,
			XmATTACH_NONE, XmATTACH_FORM, NULL, sep2, NULL, NULL);
		XmxSetPositions (togm2, XmxNoPosition, XmxNoPosition, 50, XmxNoPosition);
		XmxSetConstraints(sep2, XmATTACH_NONE, XmATTACH_WIDGET,
			XmATTACH_FORM, XmATTACH_FORM, NULL, buttons_form, NULL, NULL);
	}
	XmxSetConstraints(buttons_form, XmATTACH_NONE, XmATTACH_FORM,
		XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
	return mo_succeed;
}

static mo_status mo_do_edit_hotnode_title_win (mo_window *win, mo_hot_item
					       *item, int position)
{
	edit_or_insert_hot_info *eht_info;

/* This shouldn't happen. */
	if (!win->hotlist_win)
		return mo_fail;
	if (!win->edithot_win)
		mo_create_ed_or_ins_hot_win (win, 0);
	XmxSetArg (XmNuserData, (XtArgVal)&eht_info);
	XtGetValues (win->edithot_win, Xmx_wargs, Xmx_n);
	Xmx_n = 0;
/* Cache the position. */
	eht_info->pos = position;
/* Manage the little sucker. */
	XmxManageRemanage (win->edithot_win);
/* Insert this title as a starting point. */
	XmxTextSetString (eht_info->title_text, item->hot.title);
	XmxTextSetString (eht_info->comment_text, item->hot.desc);
	if (item->type == mo_t_url) {
/* Insert URL */
		XmxTextSetString (eht_info->url_text, item->hot.url);
		XtManageChild(eht_info->url_lab);
		XtManageChild(eht_info->url_text);
	} else {
		XtUnmanageChild(eht_info->url_lab);
		XtUnmanageChild(eht_info->url_text);
	}
	return mo_succeed;
}

/* Edit the title of an element of the current hotlist.
 * The element is referenced by its position.
 * Algorithm for edit:
 *   Find hotnode with the position.
 *   Change the title.
 *   Cause redisplay.
 * Return status.
 */
static mo_status mo_edit_title_in_current_hotlist (mo_window *win,
						   int position)
{
  mo_hotlist *list = win->current_hotlist;
  mo_hot_item *hotnode;
  
  FindHotFromPos(hotnode, list, position);

  /* OK, now we have hotnode loaded. */
  /* hotnode->hot.title is the current title.
     hotnode->hot.position is the current position. */
  return
    ((hotnode != NULL) ?
     mo_do_edit_hotnode_title_win (win, hotnode, position) :
     mo_fail);
}

static void mo_insert_item_in_current_hotlist(mo_window *win)
{
  if (win->hotlist_win) {
      edit_or_insert_hot_info *eht_info;

      if (!win->inserthot_win)
	mo_create_ed_or_ins_hot_win(win, 1);

      XmxSetArg (XmNuserData, (XtArgVal)&eht_info);
      XtGetValues (win->inserthot_win, Xmx_wargs, Xmx_n);
      Xmx_n = 0;
      /* Manage the little sucker. */
      XmxManageRemanage (win->inserthot_win);

      if (win->hot_cut_buffer) {
	  /* Insert this title as a starting point. */
	  XmxTextSetString(eht_info->title_text,win->hot_cut_buffer->any.title);
	  XmxTextSetString(eht_info->comment_text,win->hot_cut_buffer->any.desc);
	  if (win->hot_cut_buffer->type == mo_t_url) {
	      /* Insert URL */
	      XmxTextSetString (eht_info->url_text,
				win->hot_cut_buffer->hot.url);
	      XtManageChild(eht_info->url_lab);
	      XtManageChild(eht_info->url_text);
	      XmToggleButtonGadgetSetState(eht_info->tog_list, False, False);
	      XmToggleButtonGadgetSetState(eht_info->tog_url, True, False);
	    } else {
	      /* Insert a List */
	      XtUnmanageChild(eht_info->url_lab);
	      XtUnmanageChild(eht_info->url_text);
	      XmToggleButtonGadgetSetState(eht_info->tog_list, True, False);
	      XmToggleButtonGadgetSetState(eht_info->tog_url, False, False);
	    }
	} else {
	  XmTextFieldSetString(eht_info->title_text, " ");
	  XmTextFieldSetString(eht_info->url_text, " ");
	  XmxTextSetString(eht_info->comment_text, "");
	}
    }
}

/* ----------------------------- HOTLIST GUI ------------------------------ */

/* Initial GUI support for hotlist will work like this:
 *
 * There will be a single hotlist. It will be persistent across all windows.
 * Upon program startup an attempt will be made to load it out
 * of its file; if this attempt isn't successful, it is created
 * Upon program exit it will be stored to its file.
 * Called on initialization. Tries to load the default hotlist.
*/

void MMHotlistInit (char * mmosaic_root_dir)
{
	char *tf=NULL,retBuf[BUFSIZ]; 
	char *hot_filename = "hot.html";
	struct stat s;
	FILE * fp;
 
	lhot_file_name = (char *)malloc(strlen (mmosaic_root_dir) + 
				strlen (hot_filename) + 8);
	sprintf (lhot_file_name, "%s/%s", mmosaic_root_dir, hot_filename);

	if ( stat(lhot_file_name,&s) !=0 ) { /* create empty file */
		fp = fopen(lhot_file_name, "w");
		fprintf(fp,"<HTML>\n<HEAD><TITLE>Hotlist from user %s</TITLE></HEAD>\n<BODY>\n<DD>primaryhotlist\n<DL>\n</DL></BODY>\n</HTML>\n",mMosaicAppData.author_full_name);
		fclose(fp);
	} else {		/* do a backup */
		tf=(char *)malloc(strlen(lhot_file_name)+13);
		sprintf(tf,"%s.backup",lhot_file_name);
		if (my_copy(lhot_file_name,tf,retBuf,BUFSIZ-1,1)!=SYS_SUCCESS) {
			fprintf(stderr,"%s\n",retBuf);
		}
		free(tf);
	}
/* Try to load the default hotlist. */
	if (!(fp=fopen(lhot_file_name,"r"))) {
		mMosaicHotList = NULL;
		fprintf(stderr,"Fail to read Hotlist %s\n",lhot_file_name);
		return ;
	}
	mMosaicHotList = (mo_hotlist *)calloc (1,sizeof (mo_hotlist));
	mMosaicHotList->type = mo_t_list;
	mMosaicHotList->title = NULL;
	mMosaicHotList->desc = NULL;
	mMosaicHotList->position = 1;
	mMosaicHotList->next = mMosaicHotList->previous = 0;
	mMosaicHotList->parent = 0;
	mMosaicHotList->nodelist = mMosaicHotList->nodelist_last = 0;
	mMosaicHotList->modified = 1;
      	mo_read_hotlist(mMosaicHotList, fp);
	fclose (fp);
}

/* Called on program exit. Tries to write the default hotlist. */

mo_status mo_write_default_hotlist (void)
{
	FILE *fp = fopen (lhot_file_name, "w");

	if (!fp)
		return mo_fail;
	mo_write_hotlist (mMosaicHotList, fp);
	if (fclose (fp))
		return mo_fail;
	mMosaicHotList->modified = 0;
	return mo_succeed;
}

static XmxCallback (save_hot_cb)
{
	char *fname = NULL, efname[MO_LINE_LENGTH];
	FILE *fp;
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->save_hotlist_win);
	XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
			XmSTRING_DEFAULT_CHARSET, &fname);
  	pathEval (efname, fname);
	fp = fopen (efname, "w");
	if (!fp) {
		char *buf, *final, tmpbuf[80];
		int final_len;

		buf=strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Unknown Error");
			buf=tmpbuf;
		}

	final_len=30+((!efname || !*efname?3:strlen(efname))+13)+15+(strlen(buf)+13);
	final=(char *)calloc(final_len,sizeof(char));

	sprintf(final,"\nUnable to save hotlist:\n   %s\n\nSave Error:\n   %s\n" ,(!efname || !*efname?" ":efname),buf);

	XmxMakeErrorDialog (win->save_hotlist_win,
			  final, "Save Error" );
	XtManageChild (Xmx_w);

	if (final) {
		free(final);
		final=NULL;
	}
    } else {
      mo_write_hotlist (win->current_hotlist, fp);
      fclose(fp);
    }
  free (fname);
}

static XmxCallback (load_hot_cb)
{
	char *fname = NULL,  efname[MO_LINE_LENGTH];
	FILE *fp;
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->load_hotlist_win);
	XmStringGetLtoR (((XmFileSelectionBoxCallbackStruct *)call_data)->value,
			XmSTRING_DEFAULT_CHARSET, &fname);
	pathEval (efname, fname);
	fp = fopen (efname, "r");
	if (!fp) {
		char *buf, *final, tmpbuf[80];
		int final_len;
		buf=strerror(errno);
		if (!buf || !*buf || !strcmp(buf,"Error 0")) {
			sprintf(tmpbuf,"Unknown Error");
			buf=tmpbuf;
		}
		final_len=30+((!efname || !*efname?3:strlen(efname))+13)+15+
				(strlen(buf)+13);
		final=(char *)calloc(final_len,sizeof(char));
		sprintf(final,
			"\nUnable to open hotlist:\n  %s\n\nOpen Error:\n  %s\n",
			(!efname || !*efname?" ":efname),buf);
		XmxMakeErrorDialog (win->load_hotlist_win, final, "Open Error" );
		XtManageChild (Xmx_w);

		if (final) {
			free(final);
			final=NULL;
		}
	} else {
		Widget tb;

		XmxSetArg (XmNuserData, (XtArgVal)&tb);
		XtGetValues (win->load_hotlist_win, Xmx_wargs, Xmx_n);
		Xmx_n = 0;
		if (XmToggleButtonGadgetGetState (tb)) {
			mo_hotlist *list = (mo_hotlist *)calloc(1,sizeof(mo_hotlist));

			list->type = mo_t_list;
			list->nodelist = list->nodelist_last = 0;
			list->title = NULL;
			mo_read_hotlist (list, fp);
			if (list->title == NULL)
				list->title = strdup("Unnamed" );
			mo_append_item_to_hotlist(win->current_hotlist, (mo_hot_item *)list);
			mo_gui_add_hot_item (win->current_hotlist, (mo_hot_item *)list);
		} else {
			mo_hot_item *item = win->current_hotlist->nodelist_last;
			mo_read_hotlist (win->current_hotlist, fp);
			if (item == NULL)
				item = win->current_hotlist->nodelist;
			else
				item = item->any.next;
			for (;item; item = item->any.next)
				mo_gui_add_hot_item (win->current_hotlist, item);
		}
		fclose (fp);
		mMosaicHotList->modified = 1;
		mo_write_default_hotlist ();
	}
	free (fname);
}

/* --------------- mo_delete_position_from_current_hotlist ---------------- */
/*
 * Delete an element of the default hotlist.
 * The element is referenced by its position.
 * Algorithm for removal:
 *   Find hotnode with the position.
 *   If it is a list, change the current list of the windows that has hotnode
 *	as an ancestor.
 *   Remove the hotnode from the hotlist data structure.
 *   Recalculate positions of the hotlist.
 *   Remove the element in the position in the list widgets.
 * Return status.
 */
static void delete_hot_from_list (mo_hotlist *list, mo_hot_item *hotnode,
				  int position)
{
	mo_window *win = NULL;

	if (hotnode == NULL)
		return;
	if (hotnode->type == mo_t_list)
		while ((win = mo_main_next_window (win)) ) {
			if (win->hotlist_list &&
			mo_is_ancestor (&(hotnode->list), win->current_hotlist)) {
				char *path = mo_compute_hot_path(list);

				XmListDeleteAllItems(win->hotlist_list);
				win->current_hotlist = list;
				XmxTextSetString(win->hotlist_label, path);
				free(path);
				mo_load_hotlist_list(win, win->hotlist_list);
			}
		}
			/* Pull the hotnode out of the hotlist. */
	mo_remove_hotnode_from_hotlist (list, hotnode);
	free (hotnode);
			/* Recalculate positions in this hotlist. */
	mo_recalculate_hotlist_positions (list);
  
			/* Do the GUI stuff. */
	while ( (win = mo_main_next_window(win)) ) {
		if (win->hotlist_list && win->current_hotlist == list)
			XmListDeletePos (win->hotlist_list, position);
		if (win->hot_cut_buffer == hotnode)
			win->hot_cut_buffer = NULL;
	}
}

static XmxCallback (remove_yes_cb)
{
	mo_window *win = (mo_window*)client_data;
	int position = win->delete_position_from_current_hotlist;

	if (position) {
		mo_hot_item *hotnode;
		FindHotFromPos(hotnode, win->current_hotlist, position);
		delete_hot_from_list(win->current_hotlist, hotnode, position);
	}
	XtDestroyWidget(w);
}
static XmxCallback (remove_no_cb)
{
	XtDestroyWidget(w);
}

static mo_status mo_delete_position_from_current_hotlist (mo_window *win,
							  int position)
{
	mo_hotlist *list = win->current_hotlist;
	mo_hot_item *hotnode;
  
	FindHotFromPos(hotnode, list, position);
	if (hotnode == NULL)
		return mo_fail;
	if (hotnode->type == mo_t_list) { /* OK, now we have hotnode loaded. */
		char *question;
		char *endquestion;
		char *buff;

		question=strdup("Are you sure you want to remove the \"" );
		endquestion=strdup("\" list?" );
		buff = (char *)malloc(strlen(question)+
				strlen(hotnode->list.title)+strlen(endquestion)+1);
		strcat(strcat(strcpy(buff, question), hotnode->list.title), 
				endquestion);
		win->delete_position_from_current_hotlist = position;
		XmxMakeQuestionDialog(win->hotlist_win, buff, 
			"NCSA Mosaic: Remove list" ,
			remove_yes_cb, remove_no_cb,  (XtPointer)win);
		free(buff);
		free(question);
		free(endquestion);
		XtManageChild (Xmx_w);
	} else {
		delete_hot_from_list(list, hotnode, position);
  	}
	return mo_succeed;
}


/* ----------------------------- mail hotlist ----------------------------- */

static XmxCallback (mailhot_win_cb1)
{
	mo_window *win = (mo_window*)client_data;
	XtUnmanageChild (win->mailhot_win);
}
static XmxCallback (mailhot_win_cb2)
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-hotlist-view.html"));
}
static XmxCallback (mailhot_win_cb0)
{
	mo_window *win = (mo_window*)client_data;
	char *to, *subj;
	FILE *fp;

	XtUnmanageChild (win->mailhot_win);
	to = XmTextGetString (win->mailhot_to_text);
/*
####Commit#####
	char *txt;
	txt = XmTextGetString (win->annotate_text);
	mo_modify_pan (win->editing_id,
			XmTextGetString (win->annotate_title), 
			XmTextGetString (win->annotate_author), 
			txt);
#####
*/
	if (!to)
		return;
	if (to[0] == '\0')
		return;
	subj = XmTextGetString (win->mailhot_subj_text);
/* Open a file descriptor to sendmail. */
	fp = mo_start_sending_mail_message (to, subj, "text/x-html", NULL);
	if (!fp)
		goto oops;
	mo_write_hotlist(win->current_hotlist, fp);
	mo_finish_sending_mail_message ();
oops:
	free (to);
	free (subj);
}

static mo_status mo_post_mailhot_win (mo_window *win)
{
  /* This shouldn't happen. */
  if (!win->hotlist_win)
    return mo_fail;

  if (!win->mailhot_win) {
      Widget dialog_frame;
      Widget dialog_sep, buttons_form;
      Widget mailhot_form, to_label, subj_label;
      
      /* Create it for the first time. */
      win->mailhot_win = XmxMakeFormDialog 
        (win->hotlist_win, "NCSA Mosaic: Mail Hotlist" );
      dialog_frame = XmxMakeFrame (win->mailhot_win, XmxShadowOut);

      /* Constraints for base. */
      XmxSetConstraints (dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      mailhot_form = XmxMakeForm (dialog_frame);
      
      to_label = XmxMakeLabel (mailhot_form, "Mail To:" );
      XmxSetArg (XmNwidth, 335);
      win->mailhot_to_text = XmxMakeTextField (mailhot_form);
      
      subj_label = XmxMakeLabel (mailhot_form, "Subject:" );
      win->mailhot_subj_text = XmxMakeTextField (mailhot_form);

      dialog_sep =XmxMakeHorizontalSeparator (mailhot_form);
      
      buttons_form = XmxMakeFormAndThreeButtons(mailhot_form, 
		"Mail" , "Dismiss" , "Help..." , 
		mailhot_win_cb0, mailhot_win_cb1, mailhot_win_cb2,
		(XtPointer)win);

      /* Constraints for mailhot_form. */
      XmxSetOffsets (to_label, 14, 0, 10, 0);
      XmxSetConstraints (to_label, XmATTACH_FORM, XmATTACH_NONE,
		 XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);
      XmxSetOffsets (win->mailhot_to_text, 10, 0, 5, 10);
      XmxSetConstraints (win->mailhot_to_text, XmATTACH_FORM,
		 XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM,
		 NULL, NULL, to_label, NULL);

      XmxSetOffsets (subj_label, 14, 0, 10, 0);
      XmxSetConstraints (subj_label, XmATTACH_WIDGET, XmATTACH_NONE,
		 XmATTACH_FORM, XmATTACH_NONE, win->mailhot_to_text,
		 NULL, NULL, NULL);
      XmxSetOffsets (win->mailhot_subj_text, 10, 0, 5, 10);
      XmxSetConstraints (win->mailhot_subj_text, XmATTACH_WIDGET, XmATTACH_NONE,
		 XmATTACH_WIDGET, XmATTACH_FORM,
		 win->mailhot_to_text, NULL, subj_label, NULL);

      XmxSetArg (XmNtopOffset, 10);
      XmxSetConstraints (dialog_sep, XmATTACH_WIDGET, XmATTACH_WIDGET,
		 XmATTACH_FORM, XmATTACH_FORM,
		 win->mailhot_subj_text, buttons_form, NULL, NULL);
      XmxSetConstraints (buttons_form, XmATTACH_NONE, XmATTACH_FORM,
		 XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
    }
  XtManageChild (win->mailhot_win);
  return mo_succeed;
}

static XmxCallback(hotlist_rbm_toggle_cb) 
{
	mo_window *win = (mo_window*)client_data;
	Boolean rv;
	int *pos_list;
	int pos_cnt;
	mo_hotlist *list;
	mo_hot_item *hotnode;

        list=win->current_hotlist;    
        rv = XmListGetSelectedPos (win->hotlist_list, &pos_list, &pos_cnt);
        if (rv && pos_cnt) {          
                FindHotFromPos(hotnode, list, pos_list[0]);
                if (!hotnode)       
                        return;       
        } else {                        
                XmxMakeErrorDialog    
                        (win->hotlist_win, "No entry in the hotlist is currently selected.\n\nTo go to an entry in the hotlist,\nselect it with a single mouse click\nand press the Go To button again." , "Error: Nothing Selected" );
                XtManageChild (Xmx_w);
        }                             
        return;                       
}

/* ---------------------------- hotlist_win_cb ---------------------------- */

static XmxCallback (hotlist_win_cb0)
{
	mo_window *win = (mo_window*)client_data;

	XtUnmanageChild (win->hotlist_win);
}
static XmxCallback (hotlist_win_cb1)
{
	mo_window *win = (mo_window*)client_data;

	mo_post_mailhot_win (win);
}
static XmxCallback (hotlist_win_cb2)
{
	mo_window *win = (mo_window*)client_data;

	mo_open_another_window (win, 
		mo_assemble_help_url ("help-on-hotlist-view.html"));
}
static XmxCallback (hotlist_win_cb3)
{
	mo_window *win = (mo_window*)client_data;

	if (win->current_node) {		 /* Add current. */
		mo_add_node_to_current_hotlist (win);
		mo_write_default_hotlist ();
	}
}
static XmxCallback (hotlist_win_cb4)		 /* Goto selected. */
{
	mo_window *win = (mo_window*)client_data;
	Boolean rv;
	int *pos_list;
	int pos_cnt;

	rv = XmListGetSelectedPos (win->hotlist_list, &pos_list, &pos_cnt);
	if (rv && pos_cnt) {
		mo_visit_hotlist_position (win, pos_list[0]);
	} else {
		XmxMakeErrorDialog (win->hotlist_win, "No entry in the hotlist is currently selected.\n\nTo go to an entry in the hotlist,\nselect it with a single mouse click\nand press the Go To button again." , "Error: Nothing Selected" );
		XtManageChild (Xmx_w);
	}
}

static XmxCallback (hotlist_win_cb5) 	/* Remove selected. */
{
	mo_window *win = (mo_window*)client_data;

	Boolean rv;
	int *pos_list;
	int pos_cnt;
	rv = XmListGetSelectedPos (win->hotlist_list, &pos_list, &pos_cnt);
	if (rv && pos_cnt) {
		mo_delete_position_from_current_hotlist (win, pos_list[0]);
		mo_write_default_hotlist ();
	} else {
		XmxMakeErrorDialog(win->hotlist_win, "No entry in the hotlist is currently selected.\n\nTo remove an entry in the hotlist,\nselect it with a single mouse click\nand press the Remove button again." , "Error: Nothing Selected" );
XtManageChild (Xmx_w);
	}
}

static XmxCallback (hotlist_win_cb6)	 /* Edit title of selected. */
{
	mo_window *win = (mo_window*)client_data;

	Boolean rv;
	int *pos_list;
	int pos_cnt;
	rv = XmListGetSelectedPos (win->hotlist_list, &pos_list, &pos_cnt);
	if (rv && pos_cnt) {
		mo_edit_title_in_current_hotlist (win, pos_list[0]);
		XtFree((char *)pos_list); /* DXP */
		/*Writing default hotlist should take place in the callback. */
		/* mo_write_default_hotlist (); */
	} else {
		XmxMakeErrorDialog(win->hotlist_win, "No entry in the hotlist is currently selected.\n\nTo edit an entry in the hotlist,\nselect it with a single mouse click\nand press the Edit button again." , "Error: Nothing Selected" );
		XtManageChild (Xmx_w);
	}
}
static XmxCallback (hotlist_win_cb7)	 /* Copy item to cut buffer */
{
	mo_window *win = (mo_window*)client_data;
	int *pos_list;
	int pos_cnt;

	if (XmListGetSelectedPos (win->hotlist_list, &pos_list, &pos_cnt) &&
	   pos_cnt) {
		mo_copy_hotlist_position(win, pos_list[0]);
		XtFree((char *)pos_list); /* DXP */
	} else {
		XmxMakeErrorDialog (win->hotlist_win, "No entry in the hotlist is currently selected.\n\nTo copy an entry in the hotlist,\nselect it with a single mouse click\nand press the Copy button again." , "Error: Nothing Selected" );
		XtManageChild (Xmx_w);
	}
}

static XmxCallback (hotlist_win_cb8) /* Insert an Item in the current hotlist */
{
	mo_window *win = (mo_window*)client_data;

	mo_insert_item_in_current_hotlist(win);
}

static XmxCallback (hotlist_win_cb9)	 /* Go Up one level */
{
	mo_window *win = (mo_window*)client_data;

	if (win->current_hotlist->parent != 0) {
		char *path = mo_compute_hot_path(win->current_hotlist->parent);
		XmListDeleteAllItems(win->hotlist_list);
		win->current_hotlist = win->current_hotlist->parent;
		XmxTextSetString(win->hotlist_label, path);
		free(path);
		mo_load_hotlist_list(win, win->hotlist_list);
	}
}
static XmxCallback (hotlist_win_cb10)		 /* Save in a file */
{
	mo_window *win = (mo_window*)client_data;

	if (!win->save_hotlist_win)
		win->save_hotlist_win = XmxMakeFileSBDialog(win->hotlist_win,
					 "NCSA Mosaic: Save Current hotlist" ,
				"Name for saved hotlist" , 
				save_hot_cb,(XtPointer) win);
	else
		XmFileSelectionDoSearch (win->save_hotlist_win, NULL);
	XmxManageRemanage (win->save_hotlist_win);
}
static XmxCallback (hotlist_win_cb11)	 /* Load a hotlist file */
{
	mo_window *win = (mo_window*)client_data;

	if (!win->load_hotlist_win) {
		Widget frame, workarea, tb;

		win->load_hotlist_win = XmxMakeFileSBDialog(win->hotlist_win, 
					"NCSA Mosaic: Load in Current hotlist" ,
					"Name of file to open", 
					load_hot_cb, (XtPointer)win);
/* This makes a frame as a work area for the dialog box. */
		XmxSetArg (XmNmarginWidth, 5);
		XmxSetArg (XmNmarginHeight, 5);
		frame = XmxMakeFrame (win->load_hotlist_win, XmxShadowEtchedIn);
		XmxSetArg (XmNorientation, XmHORIZONTAL);
		workarea = XmxMakeRadioBox (frame);
		tb = XtVaCreateManagedWidget("toggle", xmToggleButtonGadgetClass,
			workarea,
			XtVaTypedArg, XmNlabelString,
				XtRString,"Create new hotlist",
			 	strlen("Create new hotlist" )+1,
			XmNmarginHeight, 0,
			XmNset, True, NULL);
		XmxSetArg (XmNuserData, (XtArgVal)tb);
		XmxSetValues (win->load_hotlist_win);
		XtVaCreateManagedWidget("toggle", xmToggleButtonGadgetClass,
			workarea,
			XtVaTypedArg, XmNlabelString,
				XtRString, "Load in current hotlist" , 
				strlen("Load in current hotlist" )+1,
			XmNmarginHeight, 0,
			NULL);
	} else
		XmFileSelectionDoSearch (win->load_hotlist_win, NULL);
	XmxManageRemanage (win->load_hotlist_win);
}

static XmxCallback (hotlist_list_cb)
{
	mo_window *win = (mo_window*)client_data;
	XmListCallbackStruct *cs = (XmListCallbackStruct *)call_data;
  
	mo_visit_hotlist_position (win, cs->item_position);
	/* Don't unmanage the list. */
}

/* ------------------------- mo_post_hotlist_win -------------------------- */

/* Pop up a hotlist window for an mo_window.  */
mo_status mo_post_hotlist_win (mo_window *win)
{
  if (!win->hotlist_win) {
      Widget dialog_frame/*, toto*/;
      Widget dialog_sep, buttons_form, buttons1_form, buttons2_form;
      Widget hotlist_form/*, buttons1_frame*/;
      XtTranslations listTable;
      static char listTranslations[] =
	"~Shift ~Ctrl ~Meta ~Alt <Btn2Down>: ListBeginSelect() \n\
	  Button2<Motion>:		ListButtonMotion()\n\
	 ~Shift ~Ctrl ~Meta ~Alt <Btn2Up>:  ListBeginSelect() ListEndSelect()";

      listTable = XtParseTranslationTable(listTranslations);

      /* Create it for the first time. */
      XmxSetArg (XmNwidth, 475);
      XmxSetArg (XmNheight, 342);
      win->hotlist_win = XmxMakeFormDialog(win->base, "mMosaic: Hotlist View" );
      dialog_frame = XmxMakeFrame (win->hotlist_win, XmxShadowOut);
      
      /* Constraints for base. */
      XmxSetConstraints(dialog_frame, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      
      /* Main form. */
      hotlist_form = XmxMakeForm (dialog_frame);

      win->current_hotlist = (mo_hotlist *)mMosaicHotList;
      XmxSetArg (XmNcursorPositionVisible, False);
      XmxSetArg (XmNeditable, False);
      XmxSetArg (XmNvalue, (XtArgVal)"/");
      win->hotlist_label = XmxMakeTextField(hotlist_form);

      buttons1_form = XmxMakeFormAndFourButtons(hotlist_form, 
		"Add Current", "Goto URL", "Remove", "Edit", 
		hotlist_win_cb3, hotlist_win_cb4, hotlist_win_cb5, 
		hotlist_win_cb6, (XtPointer)win);
      buttons2_form = XmxMakeFormAndThreeButtons (hotlist_form, 
		"Copy", "Insert", "Up" , 
	 	hotlist_win_cb7, hotlist_win_cb8, hotlist_win_cb9,(XtPointer)win);
      XmxSetArg (XmNfractionBase, (XtArgVal)4);
      XmxSetArg (XmNverticalSpacing, (XtArgVal)0);
      XmxSetValues(buttons2_form);

      /* Hotlist list itself. */
      XmxSetArg (XmNresizable, False);
      XmxSetArg (XmNscrollBarDisplayPolicy, XmSTATIC);
      XmxSetArg (XmNlistSizePolicy, XmCONSTANT);
      win->hotlist_list = XmxMakeScrolledList(hotlist_form, 
			hotlist_list_cb, (XtPointer)win);
      XtAugmentTranslations (win->hotlist_list, listTable);
      XtAddCallback(win->hotlist_list,
                    XmNbrowseSelectionCallback, hotlist_rbm_toggle_cb,
                    win);
      dialog_sep = XmxMakeHorizontalSeparator (hotlist_form);
      
      buttons_form = XmxMakeFormAndFiveButtons(hotlist_form, 
		"Mail To..." ,"Save" ,"Load" , "Dismiss", "Help...",
		hotlist_win_cb1, hotlist_win_cb10, 
		hotlist_win_cb11,hotlist_win_cb0, hotlist_win_cb2,
		(XtPointer)win);
      
      /* Constraints for hotlist_form. */
      /* buttons1_form: top to nothing, bottom to hotlist_list,
         left to form, right to form. */
      XmxSetOffsets (win->hotlist_label,  4, 0, 2, 2);
      XmxSetConstraints(win->hotlist_label, XmATTACH_FORM,  XmATTACH_NONE,
	XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
      XmxSetOffsets (buttons1_form, 0, 0, 0, 0);
      XmxSetConstraints (buttons1_form, 
         XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         win->hotlist_label, NULL, NULL, NULL);
      XmxSetOffsets (buttons2_form, 0, 2, 0, 0);
      XmxSetConstraints (buttons2_form, 
         XmATTACH_WIDGET, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
	 buttons1_form, NULL, NULL, NULL);
/* list: top to form, bottom to rbm_toggle, etc... */
      XmxSetOffsets (XtParent (win->hotlist_list), 10, 10, 8, 8);
      XmxSetConstraints(XtParent (win->hotlist_list), 
         XmATTACH_WIDGET, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM, 
         buttons2_form, dialog_sep, NULL, NULL);

      XmxSetConstraints (dialog_sep, 
         XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_FORM, XmATTACH_FORM,
         NULL, buttons_form, NULL, NULL);
      XmxSetConstraints 
        (buttons_form, XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM, 
         XmATTACH_FORM,
         NULL, NULL, NULL, NULL);
      win->save_hotlist_win = win->load_hotlist_win = NULL;
      win->hot_cut_buffer = NULL;
      /* Go get the hotlist up to this point set up... */
      mo_load_hotlist_list (win, win->hotlist_list);
    }
  
  XmxManageRemanage (win->hotlist_win);
  
  return mo_succeed;
}

/* -------------------- mo_add_node_to_current_hotlist -------------------- */

mo_status mo_add_node_to_current_hotlist (mo_window *win)
{
  	return mo_add_item_to_hotlist (mMosaicHotList, mo_t_url,
				 win->current_node->htinfo->title,
				 win->current_node->aurl,
				 NULL, 0);
}
