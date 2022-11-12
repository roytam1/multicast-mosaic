/*
 * Copyright (C) 1992, Board of Trustees of the University of Illinois.
 *
 * Permission is granted to copy and distribute source with out fee.
 * Commercialization of this product requires prior licensing
 * from the National Center for Supercomputing Applications of the
 * University of Illinois.  Commercialization includes the integration of this 
 * code in part or whole into a product for resale.  Free distribution of 
 * unmodified source and use of NCSA software is not considered 
 * commercialization.
 *
 */


typedef struct LISTSTRUCT  *List;

extern List ListCreate();
extern void ListDestroy(List theList);
extern int  ListAddEntry(List theList, char *v);
extern int  ListDeleteEntry(List theList, char *v);
extern int  ListMakeEntryCurrent(List theList, char *entry);
extern int  ListCount(List theList);

extern char *ListHead(List theList);
extern char *ListTail(List theList);
extern char *ListCurrent(List theList);
extern char *ListNext(List theList);
extern char *ListPrev(List theList);
extern char *ListGetIndexedEntry(List theList, int number);
