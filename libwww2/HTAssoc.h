/*                       ASSOCIATION LIST FOR STORING NAME-VALUE PAIRS
                                             
   Lookups from assosiation list are not case-sensitive.
   
 */

#ifndef HTASSOC_H
#define HTASSOC_H

#include "HTUtils.h"
#include "HTList.h"


typedef HTList HTAssocList;

typedef struct {
    char * name;
    char * value;
} HTAssoc;


PUBLIC HTAssocList *HTAssocList_new NOPARAMS;
PUBLIC void HTAssocList_delete PARAMS((HTAssocList * alist));

PUBLIC void HTAssocList_add PARAMS((HTAssocList *       alist,
                                    WWW_CONST char *        name,
                                    WWW_CONST char *        value));

PUBLIC char *HTAssocList_lookup PARAMS((HTAssocList *   alist,
                                        WWW_CONST char *    name));

#endif /* not HTASSOC_H */
