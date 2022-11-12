/* Please read copyright.tmpl. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>

#include "HTMLparse.h"
#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

/* Code to manage a linked list of parsed HTML objects generated from a
 * raw text file. Also code to manage a linked list of formatted elements that
 * make up a page of a formatted document.
 */

/* Free up the passed linked list of parsed elements, freeing
 * all memory associates with each element.
 */
void FreeMarkUpList(struct mark_up *List)
{
	struct mark_up *current;
	struct mark_up *mptr;

	current = List;
	while (current != NULL) {
		mptr = current;
		current = current->next;
		mptr->next = NULL;
		if (mptr->start != NULL)
			free((char *)mptr->start);
		if (mptr->text != NULL)
			free((char *)mptr->text);
		if (mptr->end != NULL)
			free((char *)mptr->end);
		if ( (!mptr->is_end) && (mptr->type == M_ANCHOR) ) {
			if (mptr->anc_name){
				free(mptr->anc_name);
			}
			if (mptr->anc_href){
				free(mptr->anc_href);
			}
			if (mptr->anc_title){
				free(mptr->anc_title);
			}
		}

#ifdef APROG
		if (mptr->s_aps){	/* aprog */
			_FreeAprogStruct(mptr->s_aps);
		}
#endif
#ifdef APPLET
		if (mptr->s_ats){	/* applet */
			_FreeAppletStruct(mptr->s_ats);
		}
#endif
		if (mptr->t_p1){	/* table */
			_FreeTableStruct(mptr->t_p1);
		}
		free((char *)mptr);
	}
}

/* Used to find the longest line (in characters) in a collection of text blocks.
 * cnt is the running count of characters, and txt is the pointer to the current
 * text block. Since we are finding line widths, a newline resets the width count.
 */
char * MaxTextWidth(char *txt, int *cnt)
{
	char *start;
	char *end;
	int width;

	if (txt == NULL)
		return(NULL);
	width = *cnt;
	start = txt;
	/* If this blocks starts with a newline, reset the width
	 * count, and skip the newline.
	 */
	if (*start == '\n') {
		width = 0;
		start++;
	}
	end = start;
	/* count characters, stoping either at a newline, or at the
	 * end of this text block.  Expand tabs.
	 */
	while ((*end != '\0')&&(*end != '\n')) {
		if (*end == '\t') {
			width = ((width / 8) + 1) * 8;
		} else {
			width++;
		}
		end++;
	}
	*cnt = width;
	return(end);
}

/* Free up the passed linked list of formatted elements, freeing
 * all memory associates with each element.
 */
void FreeLineList( struct ele_rec *list, HTMLWidget w)
{
	struct ele_rec *current;
	struct ele_rec *eptr;

	current = list;
	while (current != NULL) {
		eptr = current;
		current = current->next;
		if (eptr->edata != NULL) {
			free((char *)eptr->edata);
			eptr->edata = NULL;
		}
		eptr->anchor_tag_ptr = NULL;
/*######################################################################
 * free this somewhere in img.c ##########
 *		if((eptr->type == E_IMAGE) && eptr->pic_data->fetched ) {
 */                         /* Don't free internal images */
/*			if((eptr->pic_data->image != (Pixmap)NULL)&&
 *			   (eptr->pic_data->internal != 1) ){
 *				XFreePixmap(XtDisplay(w), eptr->pic_data->image);
 *				eptr->pic_data->image = (Pixmap)NULL;
 *				if (eptr->pic_data->clip != (Pixmap)NULL) {
 *					XFreePixmap(XtDisplay(w),
 *						eptr->pic_data->clip);
 *					eptr->pic_data->clip = (Pixmap)NULL;
 *				}
 *			}
 *			free(eptr->pic_data);
 *		}
 *		if(eptr->type == E_APPLET){
 * this is done in FreeMarkUpList */
/*		}
 *		if(eptr->type == E_APROG){
 * this is done in FreeMarkUpList */
/*		}
 *        	if(eptr->pic_data->fptr != NULL) {###don't know what to do #### */
/*			typedef struct form_rec {
 *			} FormInfo;
 *		}
 *
 *      	if(eptr->widget_data != NULL) {
 *			typedef struct wid_rec {
 *			} WidgetInfo;
 *
 *		}
 *      	if(eptr->table_data != NULL) {
 * do nothing . This is done in FreeMarkUpList  */
/*		}
 *      	if(eptr->font != NULL) {
 *			XFontStruct *font;
 *		}
 *###################################*/
		eptr->next = NULL;
		eptr->prev = NULL;
		free((char *)eptr);
	}
}

/* Passed in 2 element pointers, and element positions.
 * Function should return 1 if if start occurs before end.
 * Otherwise return 0.
 */
int ElementLessThan( struct ele_rec *start, struct ele_rec *end,
	int start_pos, int end_pos)
{
	struct ele_rec *current;

	/*
	 * Deal with start or end being NULL
	 */
	if ((start == NULL)&&(end == NULL))
		return(0);
	if ((start == NULL)&&(end != NULL))
		return(1);
	if ((start != NULL)&&(end == NULL))
		return(0);
	/*
	 * Deal with easy identical case
	 */
	if (start == end) {
		if (start_pos < end_pos)
			return(1);
		return(0);
	}
	/* We know element Ids are always equal or increasing within a list.*/
	if (start->ele_id < end->ele_id)
		return(1);
	if (start->ele_id == end->ele_id) {
		current = start;
		while (current != NULL) {
			if (current->ele_id != start->ele_id)
				break;
			if (current == end)
				break;
			current = current->next;
		}
		if (current == end)
			return(1);
		return(0);
	}
	return(0);
}

/*
 * Passed in 2 element pointers, and element positions.
 * Function should return 1 if they need to be swapped in order for then
 * to proceed left to right and top to bottom in the text.
 * Otherwise return 0.
 */
int SwapElements( struct ele_rec *start, struct ele_rec *end, 
		int start_pos, int end_pos)
{
	struct ele_rec *current;

	/* Deal with start or end being NULL */
	if ((start == NULL)&&(end == NULL))
		return(0);
	if ((start == NULL)&&(end != NULL))
		return(1);
	if ((start != NULL)&&(end == NULL))
		return(0);
	/* Deal with easy identical case */
	if (start == end) {
		if (start_pos > end_pos)
			return(1);
		return(0);
	}

	/* We know element Ids are always equal or increasing within a list. */
	if (start->ele_id < end->ele_id)
		return(0);
	if (start->ele_id == end->ele_id) {
		current = start;
		while (current != NULL) {
			if (current->ele_id != start->ele_id)
				break;
			if (current == end)
				break;
			current = current->next;
		}
		if (current == end)
			return(0);
		return(1);
	}
	return(1);
}
