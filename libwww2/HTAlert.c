/*	Displaying messages and getting input for LineMode Browser
**	==========================================================
**
**	REPLACE THIS MODULE with a GUI version in a GUI environment!
**
** History:
**	   Jun 92 Created May 1992 By C.T. Barker
**	   Feb 93 Simplified, portablised (ha!) TBL
**
*/

#include "HTAlert.h"

#include "tcp.h"		/* for TOUPPER */
#include <ctype.h> 		/* for toupper - should be in tcp.h */
#include "../libhtmlw/HTML.h"
#include "../src/mosaic.h"

extern void mo_gui_notify_progress (char *);
extern int mo_gui_check_icon (int);
extern void mo_gui_clear_icon (void);
extern void mo_gui_update_meter(int,char *);
extern int prompt_for_yes_or_no (char *);
extern char *prompt_for_string (char *);
extern char *prompt_for_password (char *);

PUBLIC void HTAlert ARGS1(WWW_CONST char *, Msg)
{
	mo_gui_notify_progress (Msg);
}

PUBLIC void HTProgress ARGS1(WWW_CONST char *, Msg)
{
	mo_gui_notify_progress (Msg);
}

PUBLIC void HTMeter ARGS2(WWW_CONST int, level, WWW_CONST char *, text)
{
	mo_gui_update_meter(level,text);
}

PUBLIC int HTCheckActiveIcon ARGS1(int, twirl)
{
	return ( mo_gui_check_icon (twirl));
}

PUBLIC void HTClearActiveIcon NOARGS
{
	mo_gui_clear_icon ();
}

PUBLIC void HTDoneWithIcon NOARGS
{
	mo_gui_done_with_icon (NULL);
}

PUBLIC HT_BOOL HTConfirm ARGS1(WWW_CONST char *, Msg)
{
	if (prompt_for_yes_or_no (Msg))
		return(YES);
	else
		return(NO);
}

PUBLIC char * HTPrompt ARGS2(WWW_CONST char *, Msg, WWW_CONST char *, deflt)
{
	char *Tmp = prompt_for_string (Msg);
	char *rep = 0;

	StrAllocCopy (rep, (Tmp && *Tmp) ? Tmp : deflt);
	return rep;
}

PUBLIC char * HTPromptPassword ARGS1(WWW_CONST char *, Msg)
{
	return ( prompt_for_password (Msg));
}
