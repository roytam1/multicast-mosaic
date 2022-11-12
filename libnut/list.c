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
	HTList *newList = (HTList *)malloc (sizeof (HTList));
 
	if (newList == NULL) outofmem(__FILE__, "HTList_new");
	newList->object = NULL;
	newList->next = NULL;
	return newList;
}

void HTList_delete(HTList *me)
{
	HTList *current;

	while (current = me) {
		me = me->next;
		free (current);
	}
}

void HTList_addObject (HTList *me, void *newObject)
{
	if (me) {
		HTList *newNode = (HTList *)malloc (sizeof (HTList));

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
		HTList *newNode = (HTList *)malloc (sizeof (HTList));

		if (newNode == NULL) outofmem(__FILE__, "HTList_addObject");
		newNode->object = newObject;
		newNode->next = NULL;
		while (me->next) 
			me = me->next;
		me->next = newNode;
	} else
		fprintf(stderr, "HTList: Trying to add object %p to a nonexisting list\n", newObject);
}

int HTList_removeObject (HTList *me, void *oldObject)
{
	if (me) {
		HTList *previous;

		while (me->next) {
			previous = me;
			me = me->next;
			if (me->object == oldObject) {
				previous->next = me->next;
				free (me);
				return 1;  /* Success */
			}
		}
	}
	return 0;  /* object not found or NULL list */
}

void * HTList_removeLastObject(HTList *me)
{
	if (me && me->next) {
		HTList *lastNode = me->next;
		void * lastObject = lastNode->object;

		me->next = lastNode->next;
		free (lastNode);
		return lastObject;
	} else  /* Empty list */
		return NULL;
}

void * HTList_removeFirstObject (HTList *me)
{
	if (me && me->next) {
		HTList * prevNode;
		void *firstObject;

		while (me->next) {
			prevNode = me;
			me = me->next;
		}
		firstObject = me->object;
		prevNode->next = NULL;
		free (me);
		return firstObject;
	} else  /* Empty list */
		return NULL;
}

int HTList_count (HTList *me)
{
	int count = 0;

	if (me)
		while (me = me->next)
			count++;
	return count;
}

int HTList_indexOf (HTList *me, void *object)
{
	if (me) {
		int position = 0;

		while (me = me->next) {
			if (me->object == object)
				return position;
			position++;
		}
	}
	return -1;  /* Object not in the list */
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
