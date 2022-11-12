/*	    ASSOCIATION LIST FOR STORING NAME-VALUE PAIRS.
**	    NAMES NOT CASE SENSITIVE, AND ONLY COMMON LENGTH
**	    IS CHECKED (allows abbreviations; well, length is
**	    taken from lookup-up name, so if table contains
**	    a shorter abbrev it is not found).
** AUTHORS:
**	AL	Ari Luotonen	luotonen@dxcern.cern.ch
*/

#include <stdio.h>
#include <string.h>

#include "HTAAUtil.h"
#include "HTAssoc.h"
#include "HTString.h"
#include "HTParams.h"		/* params from X resources */

void HTAssocList_delete(HTAssocList *alist)
{
    if (alist) {
	HTAssocList *cur = alist;
	HTAssoc *assoc;
	while (NULL != (assoc = (HTAssoc*)HTList_nextObject(cur))) {
	    if (assoc->name) free(assoc->name);
	    if (assoc->value) free(assoc->value);
	    free(assoc);
	}
	HTList_delete(alist);
    }
}

void HTAssocList_add(HTAssocList *alist, WWW_CONST char *name,
	 WWW_CONST char *value)
{
    HTAssoc *assoc;

    if (alist) {
	if (!(assoc = (HTAssoc*)malloc(sizeof(HTAssoc))))
	    outofmem(__FILE__, "HTAssoc_add");
	assoc->name = NULL;
	assoc->value = NULL;

	if (name) StrAllocCopy(assoc->name, name);
	if (value) StrAllocCopy(assoc->value, value);
	HTList_addObject(alist, (void*)assoc);
	return;
    }
    if(wWWParams.trace)
	fprintf(stderr,"HTAssoc_add: ERROR: assoc list NULL!\n");
}

char *HTAssocList_lookup(HTAssocList *alist, WWW_CONST char *name)
{
    HTAssocList *cur = alist;
    HTAssoc *assoc;

    while (NULL != (assoc = (HTAssoc*)HTList_nextObject(cur))) {
	if (!strncasecomp(assoc->name, name, strlen(name)))
	    return assoc->value;
    }
    return NULL;
}
