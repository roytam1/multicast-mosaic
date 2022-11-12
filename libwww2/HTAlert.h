/*      Displaying messages and getting input for WWW Library
**
**         May 92 Created By C.T. Barker
**         Feb 93 Portablized etc TBL
*/

#include "HTUtils.h"
#include "tcp.h"

/*      Display a message and get the input
**      On entry,
**              Msg is the message.
**      On exit,
**              Return value is malloc'd string which must be freed.
*/
extern char * HTPrompt (char *Msg, char *deflt, caddr_t appd);
extern char * HTPromptPassword(char *Msg, caddr_t appd);


/*      Display a message, don't wait for input
**      On entry,
**              The input is a list of parameters for printf.
*/
extern void HTAlert (char *Msg, caddr_t appd);

/*      Display a progress message for information (and diagnostics) only
**      On entry,
**              The input is a list of parameters for printf.
*/
extern void HTProgress (char *Msg, caddr_t appd);
extern int HTCheckActiveIcon (int twirl, caddr_t appd);
extern void HTClearActiveIcon (caddr_t appd);
extern void HTMeter (int level, char *text, caddr_t appd);
extern void HTDoneWithIcon(caddr_t appd);
extern void HTrename_binary_file( caddr_t appd, char * fnam);
extern void HTapplication_user_feedback (char * s, caddr_t appd);

/*      Display a message, then wait for 'yes' or 'no'.
**      On entry,
**              Takes a list of parameters for printf.
**      On exit,
**              If the user enters 'YES', returns TRUE, returns FALSE
**              otherwise.
*/
extern HT_BOOL HTConfirm (char *Msg, caddr_t appd);
