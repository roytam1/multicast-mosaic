/*              List object
**
**      The list object is a generic container for storing collections
**      of things in order.
*/
#ifndef HTLIST_H
#define HTLIST_H

typedef struct _HTList HTList;

struct _HTList {
	void * object;
	HTList * next;
	HTList * last;
};

extern HTList * HTList_new (void);

/*      Add object to START of list */
extern void     HTList_addObject (HTList *me, void *newObject);
extern void     HTList_addObjectAtEnd (HTList *me, void *newObject);

extern int      HTList_count (HTList *me);
extern void *   HTList_objectAt (HTList *me, int position);

#endif /* HTLIST_H */
