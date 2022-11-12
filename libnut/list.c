/*	A small List class
**
**	A list is represented as a sequence of linked nodes of type HTList.
**	The first node is a header which contains no object.
**	New nodes are inserted between the header and the rest of the list.
*/

#include "list.h"

#include <stdio.h>				/* joe@athena, TBL 921019 */
#include <stdlib.h>
#include <malloc.h>

#define outofmem(file, func) \
 { fprintf(stderr, "%s %s: out of memory.\nProgram aborted.\n", file, func); \
  exit(1);}


HTList * HTList_new (void)
{
	HTList *newList = (HTList *)calloc (1, sizeof (HTList));
 
	if (newList == NULL)
		outofmem(__FILE__, "HTList_new");
	return newList;
}

void HTList_addObject (HTList *me, void *newObject)
{
	if (me) {
		HTList *newNode = (HTList *)calloc(1, sizeof (HTList));

		if (newNode == NULL) outofmem(__FILE__, "HTList_addObject");
		newNode->object = newObject;
		newNode->next = me->next;
		me->next = newNode;
		return;
	}
	fprintf(stderr, "HTList: Trying to add object %p to a nonexisting list\n", newObject);
}

void HTList_addObjectAtEnd(HTList *me, void *newObject)
{
	if (me) {
		HTList *newNode = (HTList *)calloc(1, sizeof (HTList));

		if (newNode == NULL) outofmem(__FILE__, "HTList_addObject");
		newNode->object = newObject;
		newNode->next = NULL;
		while (me->next) 
			me = me->next;
		me->next = newNode;
	} else
		fprintf(stderr, "HTList: Trying to add object %p to a nonexisting list\n", newObject);
}

int HTList_count (HTList *me)
{
	int count = 0;

	if (me)
		while (me = me->next)
			count++;
	return count;
}

void * HTList_objectAt (HTList *me, int position)
{
	if (position < 0)
		return NULL;
	if (me) {
		while (me = me->next) {
			if (position == 0)
				return me->object;
			position--;
		}
	}
	return NULL;  /* Reached the end of the list */
}
