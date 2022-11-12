/*	Hypertext "Anchor" Object
**
** An anchor represents a region of a hypertext document which is linked to
** another anchor in the same or a different document.
**
** History
**         Nov 1990  Written in Objective-C for the NeXT browser (TBL)
**	24-Oct-1991 (JFG), written in C, browser-independant 
**	21-Nov-1991 (JFG), first complete version
**
**	(c) Copyright CERN 1991 - See Copyright.html
*/

#define HASH_SIZE 101		/* Arbitrary prime. Memory/speed tradeoff */

#include <ctype.h>
#include <stdio.h>

#include "tcp.h"
#include "HText.h"
#include "HTAnchor.h"
#include "HTUtils.h"
#include "HTParse.h"
#include "HTParams.h"	/* Params from X res. */

PRIVATE HTList **adult_table=0;  /* Point to table of lists of all parents */

/*				Creation Methods
**
**	Do not use "new" by itself outside this module. In order to enforce
**	consistency, we insist that you furnish more information about the
**	anchor you are creating : use newWithParent or newWithAddress.
*/

PRIVATE HTParentAnchor * HTParentAnchor_new (void)
{
	HTParentAnchor *newAnchor = 
		(HTParentAnchor *)calloc(1,sizeof(HTParentAnchor));/*zero-fill*/
	newAnchor->parent = newAnchor;
	return newAnchor;
}

/*	Case insensitive string comparison
** On entry,
**	s	Points to one string, null terminated
**	t	points to the other.
** On exit,
**	returns	YES if the strings are equivalent ignoring case
**		NO if they differ in more than  their case.
*/

PRIVATE HT_BOOL equivalent(WWW_CONST char *s, WWW_CONST char *t)
{
	if (s && t) {  /* Make sure they point to something */
		for ( ; *s && *t ; s++, t++) {
			if (TOUPPER(*s) != TOUPPER(*t))
				return NO;
		}
		return TOUPPER(*s) == TOUPPER(*t);
	} else
		return s == t;  /* Two NULLs are equivalent, aren't they ? */
}

/*	Create new or find old sub-anchor
**
**	Me one is for a new anchor being edited into an existing
**	document. The parent anchor must already exist.
*/

static HTChildAnchor * HTAnchor_findChild(HTParentAnchor *parent, WWW_CONST char *tag)
{
	HTChildAnchor *child;
	HTList *kids;

	if (! parent) {
		if (wWWParams.trace)
			 printf("HTAnchor_findChild called with NULL parent.\n");
		return NULL;
	}
	if (kids = parent->children) {  /* parent has children : search them */
		if (tag && *tag) {		/* TBL */
			while(child = (HTChildAnchor *)HTList_nextObject (kids)) {
				if (equivalent(child->tag, tag)) {
					 /* Case sensitive 920226 */
		if (wWWParams.trace) fprintf (stderr,
	       "Child anchor %p of parent %p with name `%s' already exists.\n",
		    (void*)child, (void*)parent, tag);
return child;
				}
			}
		}  /*  end if tag is void */
	} else  /* parent doesn't have any children yet : create family */
		parent->children = HTList_new ();
	child=(HTChildAnchor *)calloc(1,sizeof(HTChildAnchor));/* zero-filled */
  if (wWWParams.trace) fprintf(stderr, "new Anchor %p named `%s' is child of %p\n",
       (void*)child, (int)tag ? tag : (WWW_CONST char *)"" , (void*)parent); /* int for apollo */
	HTList_addObject (parent->children, child);
	child->parent = parent;
	StrAllocCopy(child->tag, tag);
	return child;
}

/*	Create new or find old named anchor
**
**	Me one is for a reference which is found in a document, and might
**	not be already loaded.
**	Note: You are not guaranteed a new anchor -- you might get an old one,
**	like with fonts.
*/

HTAnchor * HTAnchor_findAddress (WWW_CONST char *address)
{
	char *tag = HTParse(address, "", PARSE_ANCHOR); /* Anchor tag specified? */

/* If the address represents a sub-anchor, we recursively load its parent,
 * then we create a child anchor within that document. */
  if (tag && *tag) {
      char *docAddress = HTParse(address, "", PARSE_ACCESS | PARSE_HOST |
                                 PARSE_PATH | PARSE_PUNCTUATION);
      HTParentAnchor * foundParent =
        (HTParentAnchor *) HTAnchor_findAddress (docAddress);
      HTChildAnchor * foundAnchor = HTAnchor_findChild (foundParent, tag);
      free (docAddress);
      free (tag);
      return (HTAnchor *) foundAnchor;
    } else { /* If the address has no anchor tag, 
	    check whether we have this node */
    int hash;
    WWW_CONST char *p;
    HTList * adults;
    HTList *grownups;
    HTParentAnchor * foundAnchor;

    free (tag);
    
    /* Select list from hash table */
    for(p=address, hash=0; *p; p++)
    	hash = (hash * 3 + (*(unsigned char*)p))
    	 % HASH_SIZE;
    if (!adult_table)
        adult_table = (HTList**) calloc(HASH_SIZE, sizeof(HTList*));
    if (!adult_table[hash]) adult_table[hash] = HTList_new();
    adults = adult_table[hash];

    /* Search list for anchor */
    grownups = adults;
    while (foundAnchor = (HTParentAnchor*)HTList_nextObject (grownups)) {
       if (equivalent(foundAnchor->address, address)) {
	if (wWWParams.trace) fprintf(stderr, "Anchor %p with address `%s' already exists.\n",
			  (void*) foundAnchor, address);
	return (HTAnchor *) foundAnchor;
      }
    }
    
    /* Node not found : create new anchor */
    foundAnchor = HTParentAnchor_new ();
    if (wWWParams.trace) fprintf(stderr, "New anchor %p has hash %d and address `%s'\n",
    	(void*)foundAnchor, hash, address);
    StrAllocCopy(foundAnchor->address, address);
    HTList_addObject (adults, foundAnchor);
    return (HTAnchor *) foundAnchor;
  }
}
