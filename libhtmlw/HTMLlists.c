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
/*      	if(eptr->table_data)  
/*      		FreeTableInfo(eptr->table_data);
*/
		eptr->anchor_tag_ptr = NULL;
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
