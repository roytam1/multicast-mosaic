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
#include "../src/gui.h"
#include "../src/gui-dialogs.h"
#include "../src/mo-www.h"

PUBLIC void HTAlert (char *Msg, caddr_t appd)
{
	mo_gui_notify_progress (Msg, (mo_window *)appd);
}

PUBLIC void HTProgress (char *Msg, caddr_t appd)
{
	mo_gui_notify_progress (Msg, (mo_window *)appd);
}

PUBLIC void HTMeter(int level, char *text, caddr_t appd)
{
	mo_gui_update_meter(level,text, (mo_window *)appd);
}

PUBLIC int HTCheckActiveIcon (int twirl, caddr_t appd)
{
	return ( mo_gui_check_icon (twirl, (mo_window *)appd));
}

PUBLIC void HTClearActiveIcon(caddr_t appd)
{
	mo_gui_clear_icon ((mo_window *)appd);
}

PUBLIC void HTDoneWithIcon(caddr_t appd)
{
	mo_gui_done_with_icon ((mo_window*)appd);
}

PUBLIC HT_BOOL HTConfirm (char *Msg, caddr_t appd)
{
	if (prompt_for_yes_or_no (Msg,(mo_window *)appd))
		return(YES);
	return(NO);
}

PUBLIC char * HTPrompt (char *Msg, char *deflt, caddr_t appd)
{
	char *Tmp = prompt_for_string (Msg,(mo_window *)appd);
	char *rep = 0;

	StrAllocCopy (rep, (Tmp && *Tmp) ? Tmp : deflt);
	return rep;
}

PUBLIC char * HTPromptPassword (char *Msg, caddr_t appd)
{
	return ( prompt_for_password (Msg,(mo_window *)appd));
}

PUBLIC void HTrename_binary_file( caddr_t appd, char * fnam)
{
	mo_rename_binary_file( (mo_window *) appd, fnam);
}
PUBLIC void HTapplication_user_feedback (char * s, caddr_t appd)
{
	application_user_feedback (s, (mo_window*) appd);
}
