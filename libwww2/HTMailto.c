/*			MAILTO WINDOW				HTMailTo.c
 **			=============
 ** Authors:
 **  Mike Peter Bretz (bretz@zdv.uni-tuebingen.de)
 **  Alan Braverman (alanb@ncsa.uiuc.edu)
 **
 ** History:
 **	07 Jul 94   First version  (MPB)
 **     07 Mar 95   Stuck it in NCSA Mosaic for X 2.6 (AMB)
 */

#include <stdio.h>

#include "HText.h"
#include "HTAccess.h"
#include "HTUtils.h"
#include "tcp.h"
#include "HTML.h"
#include "HTParse.h"
#include "HTFormat.h"
#include "HTAlert.h"
#include "HTParams.h"

#include "../libhtmlw/HTML.h"
#include "../src/mosaic.h"

extern void GetMailtoKludgeInfo(char **url, char **subject);
extern mo_status mo_post_mailto_win (mo_window *win, char *to_address, char *subject);

struct _HTStructured 
{
  WWW_CONST HTStructuredClass *	isa;
  /* ... */
};


/*	Module-wide variables */
PRIVATE int s;                                  /* Socket for FingerHost */
PRIVATE HTStructured * target;			/* The output sink */
PRIVATE HTStructuredClass targetClass;		/* Copy of fn addresses */


/*	Initialisation for this module */
PRIVATE HT_BOOL initialized = NO;
PRIVATE HT_BOOL initialize NOARGS
{
	s = -1;			/* Disconnected */
	return YES;
}

PUBLIC int HTSendMailTo (
      WWW_CONST char *     arg,
      HTParentAnchor *	anAnchor,
      HTFormat		format_out,
      HTStream*		stream,
	caddr_t 	appd)
{
	char *mailtoURL;
	char *mailtoSubject;
	WWW_CONST char * p1;

if (wWWParams.trace) fprintf(stderr, "HTMailto: Mailing to %s\n", arg);
  
	if (!initialized) 
		initialized = initialize();
		if (!initialized) {
			HTProgress ((char *) 0,appd);
			return HT_NOT_LOADED;
		}

	p1=arg;

/* We will ask for the document, omitting the host name & anchor.
**
** Syntax of address is
** 	xxx@yyy			User xxx at site yyy (xxx is optional).
*/        
	if (!strncasecomp (arg, "mailto:", 7))
		p1 = arg + 7;	/* Skip "mailto:" prefix */

	if (!*arg) {
		HTProgress ("Could not find email address",appd);
		return HT_NOT_LOADED;	/* Ignore if no name */
	}
	GetMailtoKludgeInfo(&mailtoURL,&mailtoSubject);
	(void) mo_post_mailto_win((mo_window*)appd,p1,mailtoSubject);
	return HT_LOADED;
}

PUBLIC HTProtocol HTMailto = { "mailto", HTSendMailTo };
