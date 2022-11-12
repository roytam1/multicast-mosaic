/*          HTAccess:  Access manager  for libwww
                                      ACCESS MANAGER
                                             
   This module keeps a list of valid protocol (naming scheme)
   specifiers with associated access code.  It allows documents to be
   loaded given various combinations of parameters. New access
   protocols may be registered at any time.
   
   Part of the libwww library .
 */
#ifndef HTACCESS_H
#define HTACCESS_H
#include "HTUtils.h"
#include "tcp.h"
#include "HTAnchor.h"
#include "HTFormat.h"

/*      Return codes from load routines:
**
**      These codes may be returned by the protocol modules,
**      and by the HTLoad routines.
**      In general, positive codes are OK and negative ones are bad.
*/

#define HT_NO_DATA -9999        /* return code: OK but no data was loaded */
                                /* Typically, other app started or forked */

/* Load a document from absolute name

ON ENTRY,
  addr                    The absolute address of the document to be accessed.
  appd			  application data
ON EXIT,
  returns YES             Success in opening document
  NO                      Failure
*/
extern int HTLoadAbsolute (char * addr, caddr_t appd);


/* Register an access method */

typedef struct _HTProtocol {
        char * name;
        int (*load)PARAMS((
                WWW_CONST char *    full_address,
                HTParentAnchor * anchor,
                HTFormat        format_out,
                HTStream*       sink,
		caddr_t		appd));
} HTProtocol;

extern HT_BOOL HTRegisterProtocol PARAMS((HTProtocol * protocol));

#endif /* HTACCESS_H */
