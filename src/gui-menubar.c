/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui-popup.h" 			/* for callback struct definition */
#include "gui.h"
#include "gui-menubar.h"
#include "gui-dialogs.h"
#include "gui-documents.h"
#ifdef NEWS
#include "newsrc.h"
#include "gui-news.h"
#endif
#include "mime.h"
#include "navigate.h"
#include "globalhist.h"
#include "hotlist.h"
#include "cache.h"
#include "proxy.h"
#include "mailto.h"
#include "paf.h"

#include "../libnut/list.h"
#include "../libmc/mc_main.h"

extern void McPopdownMemberList(void);
extern void McPopupMemberList(void);

#define MAX_AGENTS		20
int	selectedAgent = 0;		/* SWP -- Spoof Agents Stuff */
int 	numAgents;	 /* SWP -- Agent Spoofing */
char 	**agent;

#ifdef MULTICAST
extern Widget mc_list_top_w;
int mc_show_participant_flag = 0;
#endif

static Widget exitbox = NULL;

static void mo_post_exitbox (void);
static XmxCallback (agent_menubar_cb);

/* --------------------------- mo_post_exitbox ---------------------------- */

static XmxCallback (exit_yes_cb)
{
	mo_exit ();
}
static XmxCallback (exit_no_cb)
{
	XtUnmanageChild (w);
}

static void mo_post_exitbox (void)
{
	if (mMosaicAppData.confirm_exit) {
		if (exitbox == NULL) {
			exitbox = XmxMakeQuestionDialog (mMosaicToplevelWidget, 
				"Are you sure you want to exit mMosaic?" ,
				"mMosaic: Exit Confirmation", 
				exit_yes_cb, exit_no_cb, 0);
			XtManageChild (exitbox);
		} else {
			XmxManageRemanage (exitbox);
		}
	} else { /* Don't confirm exit; just zap it. */
		mo_exit ();
	}
}

/* ---------------------------- mo_set_fonts ---------------------------- */

static long wrapFont (mo_window * win, char *name)
{
	char buf[BUFSIZ];
	XFontStruct *font = XLoadQueryFont (mMosaicDisplay, name);

	if (font == NULL) {
		sprintf(buf,"Could not open font '%s'. Using fixed." , name);
		XmxMakeErrorDialog(win->base,buf,"Load Font Error" );
		font = XLoadQueryFont (mMosaicDisplay, "fixed");
	}
	return ((long)font);
}

mo_status mo_set_fonts (mo_window *win, int size)
{
  switch (size) {
    case mo_large_fonts_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-times-medium-r-normal-*-20-*-*-*-*-*-*-*"));
	break;
    default:
	break;
/*##################
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-times-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-times-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-times-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-times-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-times-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 0;
      break;
    case mo_regular_fonts_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-times-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-times-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-times-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-times-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-times-medium-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetValues (win->scrolled_win);
      win->font_family = 0;
      break;
    case mo_small_fonts_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-times-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-times-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-times-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-times-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-times-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-times-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-times-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-times-medium-r-normal-*-8-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 0;
      break;
    case mo_large_helvetica_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->scrolled_win);
      win->font_family = 1;
      break;
    case mo_regular_helvetica_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetValues (win->scrolled_win);
      win->font_family = 1;
      break;
    case mo_small_helvetica_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-helvetica-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-helvetica-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-helvetica-medium-r-normal-*-8-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 1;
      break;
    case mo_large_newcentury_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetValues (win->scrolled_win);
      win->font_family = 2;
      break;
    case mo_small_newcentury_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-8-*-*-*-*-*-*-*"));
      XmxSetValues (win->scrolled_win);
      win->font_family = 2;
      break;
    case mo_regular_newcentury_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-adobe-new century schoolbook-bold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-adobe-new century schoolbook-medium-i-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-adobe-courier-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-adobe-new century schoolbook-medium-r-normal-*-10-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 2;
      break;
    case mo_large_lucidabright_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-25-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-20-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-14-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 3;
      break;
    case mo_regular_lucidabright_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-24-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-10-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 3;
      break;
    case mo_small_lucidabright_tkn:
      XmxSetArg (XtNfont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNitalicFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNboldFont, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixedboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNfixeditalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader1Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-18-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader2Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-17-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader3Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader4Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader5Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-11-*-*-*-*-*-*-*"));
      XmxSetArg (WbNheader6Font, wrapFont(win, "-b&h-lucidabright-demibold-r-normal-*-10-*-*-*-*-*-*-*"));
      XmxSetArg (WbNaddressFont, wrapFont(win, "-b&h-lucidabright-medium-i-normal-*-14-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainFont, wrapFont(win, "-b&h-lucidatypewriter-medium-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainboldFont, wrapFont(win, "-b&h-lucidatypewriter-bold-r-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNplainitalicFont, wrapFont(win, "-adobe-courier-medium-o-normal-*-12-*-*-*-*-*-*-*"));
      XmxSetArg (WbNsupSubFont, wrapFont(win, "-b&h-lucidabright-medium-r-normal-*-8-*-*-*-*-*-*-*"));

      XmxSetValues (win->scrolled_win);
      win->font_family = 3;
      break;
############## */
    }

  XmxRSetToggleState (win->menubar, (XtPointer)win->font_size, XmxNotSet); 
  XmxRSetToggleState (win->menubar, (XtPointer)size, XmxSet); 
  win->font_size = size;
  return mo_succeed;
}

/* -------------------------- mo_set_underlines --------------------------- */

mo_status mo_set_underlines (mo_window *win, int choice)
{
  if (!win->underlines_snarfed) {
      XtVaGetValues (win->scrolled_win,
                     WbNanchorUnderlines, &(win->underlines),
                     WbNvisitedAnchorUnderlines, &(win->visited_underlines),
                     WbNdashedAnchorUnderlines, &(win->dashed_underlines),
                     WbNdashedVisitedAnchorUnderlines, 
                     &(win->dashed_visited_underlines),
                     NULL);
      win->underlines_snarfed = 1;
  }

  switch (choice) {
    case mo_default_underlines_tkn:
      XmxSetArg (WbNanchorUnderlines, win->underlines);
      XmxSetArg (WbNvisitedAnchorUnderlines, win->visited_underlines);
      XmxSetArg (WbNdashedAnchorUnderlines, win->dashed_underlines);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, 
                 win->dashed_visited_underlines);
      XmxSetValues (win->scrolled_win);
      break;
    case mo_l1_underlines_tkn:
      XmxSetArg (WbNanchorUnderlines, 1);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, True);
      XmxSetValues (win->scrolled_win);
      break;
    case mo_l2_underlines_tkn:
      XmxSetArg (WbNanchorUnderlines, 1);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->scrolled_win);
      break;
    case mo_l3_underlines_tkn:
      XmxSetArg (WbNanchorUnderlines, 2);
      XmxSetArg (WbNvisitedAnchorUnderlines, 1);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->scrolled_win);
      break;
    case mo_no_underlines_tkn:
      XmxSetArg (WbNanchorUnderlines, 0);
      XmxSetArg (WbNvisitedAnchorUnderlines, 0);
      XmxSetArg (WbNdashedAnchorUnderlines, False);
      XmxSetArg (WbNdashedVisitedAnchorUnderlines, False);
      XmxSetValues (win->scrolled_win);
      break;
    }

  XmxRSetToggleState (win->menubar, (XtPointer)win->underlines_state, XmxNotSet);
  XmxRSetToggleState (win->menubar, (XtPointer)choice, XmxSet);
  win->underlines_state = choice;
  
  return mo_succeed;
}

static XmxCallback (clear_history_yes_cb)
{
	mo_window *win = (mo_window*)client_data;
  
	mo_window *ww = NULL;
	mo_wipe_global_history (win);
	while (ww = mo_main_next_window (ww))
		mo_redisplay_window (ww);
}
static XmxCallback (clear_history_no_cb)
{
	XtUnmanageChild (w);
}

/* --------------------------agent menubar_cb ------------------------------ */

void mo_set_agents(mo_window *win, int which)
{
	if (win->agent_state_pulldown){
		XmToggleButtonGadgetSetState(win->agspd_cbd[win->agent_state].w,
			False , False);
		XmToggleButtonGadgetSetState(win->agspd_cbd[which].w,
			True , False);
	}
	win->agent_state=which;
	selectedAgent=which;
}

static void agent_menubar_cb(Widget w, XtPointer clid, XtPointer calld)
{
	AgentSpoofCBStruct * cd = (AgentSpoofCBStruct*)(clid);
	mo_window *win = cd->win;
	int i = (int) cd->d;

	mo_set_agents(win,i);
}
static void mo_agent_spoofs(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window *win= (mo_window*)clid;
	Widget menu;
	int i;
	XmString xmstr;

	if (win->agent_state_pulldown)
		return;
	XtVaGetValues(w, XmNsubMenuId, &menu, NULL);
	win->agent_state_pulldown = menu;

	
	for(i=0; i<numAgents; i++){
		Xmx_n = 0;
		XmxSetArg (XmNindicatorType, (XtArgVal)XmONE_OF_MANY);
		xmstr = XmxMakeXmstrFromString( agent[i]);
		XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
		win->agspd_cbd[i].w = XtCreateManagedWidget("togglebutton",
				xmToggleButtonGadgetClass,menu, Xmx_wargs, Xmx_n);
		win->agspd_cbd[i].d= i;
		win->agspd_cbd[i].win = win;
		XmStringFree (xmstr);
		XtAddCallback(win->agspd_cbd[i].w, 
			XmNvalueChangedCallback,agent_menubar_cb,
			(XtPointer) &win->agspd_cbd[i]);
	}
	XmToggleButtonGadgetSetState(win->agspd_cbd[win->agent_state].w,
		True , False);
}
  
/* ------------------------------ menubar_cb ------------------------------ */

#ifdef NEWS
void mo_news_sub_anchor(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	if (NewsGroupS) {
		gui_news_subgroup (win);
		return;
	}
/*################################# voir ftp_rmbm_cb*/
	printf("[mo_news_sub_anchor] does not fully work\n");
/*
	if (!eptr)
		return;
	grp = &eptr->anchorHRef[5];
	subscribegroup (grp);
	sprintf (buf, "%s successfully subscribed", grp);
	mo_gui_notify_progress (buf,win);
*/
}

void mo_news_unsub_anchor(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	if (NewsGroupS) {
		gui_news_unsubgroup (win);
		return;
	}
/*################################# voir ftp_rmbm_cb*/
	printf("[mo_news_unsub_anchor] does not fully work\n");
/*
	if (!eptr)
		return;
	grp = &eptr->anchorHRef[5];
	unsubscribegroup (grp);
	sprintf (buf, "%s successfully unsubscribed", grp);
	mo_gui_notify_progress (buf,win);
*/
}

void mo_news_mread_anchor(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

/*################################# voir ftp_rmbm_cb*/
	printf("[mo_news_mread_anchor] does not fully work\n");
/*
	if (NewsGroupS)
		gui_news_markGroupRead (win);
	if (!eptr)
		break;
	grp = &eptr->anchorHRef[5];
	NewsGroupS = findgroup (grp);
	if (!NewsGroupS)
		break;
	markrangeread (NewsGroupS, NewsGroupS->minart, NewsGroupS->maxart);
	sprintf (buf, "All articles in %s marked read", NewsGroupS->name);
	mo_gui_notify_progress (buf,win);
	NewsGroupS = NULL;
*/
/* Return to newsgroup list */
/*
 *	sprintf (buf, "news:*");
 *	MMPafLoadHTMLDocInWin (win, buf);
*/
}
void mo_news_fmt0(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	HTSetNewsConfig (1,-1,-1,-1,-1,-1,-1,-1);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt1, XmxNotSet);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt0, XmxSet);
	mo_reload_document(w, (XtPointer) win, NULL);
}
void mo_news_fmt1(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	HTSetNewsConfig (0,-1,-1,-1,-1,-1,-1,-1);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt0, XmxNotSet);
	XmxRSetToggleState (win->menubar, (XtPointer)mo_news_fmt1, XmxSet);
	mo_reload_document(w, (XtPointer) win, NULL);
}

void mo_news_groups(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_list(win);
}
void mo_news_list(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_list(win);
}
void mo_news_index(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_index(win);
}
void mo_news_prevt(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_prevt(win);
}
void mo_news_prev(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_prev(win);
}
void mo_news_next(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_next(win);
}
void mo_news_nextt(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_nextt(win);
}
void mo_news_post(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_news_win(win);
}
void mo_news_follow(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_follow_win(win);
}
void mo_news_sub(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_subgroup(win);
}
void mo_news_unsub(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_unsubgroup(win);
}
void mo_news_grp0(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_showAllGroups (win);
}
void mo_news_grp1(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_showGroups (win);
}
void mo_news_grp2(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_showReadGroups (win);
}
void mo_news_art0(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_showAllArticles (win);
}
void mo_news_art1(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_showArticles (win);
}
void mo_news_mread(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_markGroupRead (win);
}
void mo_news_munread(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_markGroupUnread (win);
}
void mo_news_maunread(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_markArticleUnread (win);
}
void mo_news_flush(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_flush(win);
}
void mo_news_flushgroup(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	gui_news_flushgroup(win);
}
#endif /* NEWS */

void mo_clear_passwd_cache(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

        mo_flush_passwd_cache (win);
}

void mo_home_document(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	RequestDataStruct rds;

	rds.req_url = mMosaicAppData.home_document;
	rds.post_data = NULL;
	rds.ct = NULL;
	rds.is_reloading = False;
	rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (win, &rds);
}
static void mo_history_list_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_history_win (win);
}
void mo_links_window(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_links_window(win);
}
void mo_hotlist_postit(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_hotlist_win (win);
}
void mo_open_document(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_open_window (win);
}
void mo_open_local_document(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_open_local_window (win);
}
void mo_reload_document_and_object(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_reload_document(w, (XtPointer) win, NULL);
}

void mo_new_window(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * pwin = (mo_window*) clid;
	mo_window * neww;
	RequestDataStruct rds;

	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = mMosaicAppData.home_document;
	rds.gui_action = HTML_LOAD_CALLBACK;
	neww = mo_make_window( pwin,MC_MO_TYPE_UNICAST);
	neww->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (neww, &rds);
}

void mo_clone_window(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	mo_window * neww;
	RequestDataStruct rds;

	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = win->current_node->aurl;
	rds.gui_action = HTML_LOAD_CALLBACK;
	neww = mo_make_window(win,MC_MO_TYPE_UNICAST);
	neww->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (neww, &rds);
}

void mo_close_window(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_delete_window (win);
}
void mo_register_node_in_default_hotlist(Widget w,XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	if (win->current_node) {
		mo_add_node_to_current_hotlist (win);
		mo_write_default_hotlist ();
	}
}
void mo_network_starting_points(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	RequestDataStruct rds;

	rds.req_url = NETWORK_STARTING_POINTS_DEFAULT;
	rds.post_data = NULL;
	rds.ct = NULL;
	rds.is_reloading = False;
	rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (win, &rds);
}
void mo_internet_metaindex(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	RequestDataStruct rds;

	rds.req_url = INTERNET_METAINDEX_DEFAULT;
	rds.post_data = NULL;
	rds.ct = NULL;
	rds.is_reloading = False;
	rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (win, &rds);
}
void mo_help_about(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	mo_window * neww;
	RequestDataStruct rds;

	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = "http://www.enst.fr/~dauphin/mMosaic/index.html";
	rds.gui_action = HTML_LOAD_CALLBACK;
	neww = mo_make_window( win,MC_MO_TYPE_UNICAST);
	neww->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (neww, &rds);
}
void mo_mosaic_manual(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	mo_window * neww;
	RequestDataStruct rds;

	rds.ct = rds.post_data = NULL;
	rds.is_reloading = False;
	rds.req_url = mo_assemble_help_url("mosaic-docs.html");
	rds.gui_action = HTML_LOAD_CALLBACK;
	neww = mo_make_window( win,MC_MO_TYPE_UNICAST);
	neww->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (neww, &rds);
}

void mo_whats_new(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	mo_window * neww;
	RequestDataStruct rds;

	rds.ct = rds.post_data = NULL;
	rds.is_reloading = True;
	rds.req_url = WHATSNEW_PAGE_DEFAULT;
	rds.gui_action = HTML_LOAD_CALLBACK;
	neww = mo_make_window( win,MC_MO_TYPE_UNICAST);
	neww->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin (neww, &rds);
}

void mo_mosaic_demopage(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window (win, DEMO_PAGE_DEFAULT);
}
void mo_help_onversion(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window (win, MO_HELP_ON_VERSION_DOCUMENT);
}
void mo_help_onwindow(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window(win, 
		mo_assemble_help_url("help-on-docview-window.html"));
}
void mo_help_faq(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window(win, 
		mo_assemble_help_url ("mosaic-faq.html"));
}
void mo_help_html(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window (win, HTMLPRIMER_PAGE_DEFAULT);
}
void mo_help_url(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_open_another_window (win, URLPRIMER_PAGE_DEFAULT);
}
void mo_techsupport(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;
	char subj[128];

	sprintf (subj, "User Feedback -- NCSA Mosaic %s on %s.",
		MO_VERSION_STRING, MO_MACHINE_TYPE);
	mo_post_mailto_win(win,MO_DEVELOPER_ADDRESS,subj);
}

void mo_search(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_search_window (win);
}

void mo_document_edit(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_edit_source(win);
}
/*
void mo_document_date(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_source_date(win);
}
*/
void mo_print_document(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_print_window (win);
}
void mo_mail_document(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_post_mail_window (win);
}
#ifdef KRB5
void mo_kerberosv5_login(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	scheme_login(HTAA_KERBEROS_V5, win);
}
#endif
void mo_proxy(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	PopProxyDialog();
}
void mo_no_proxy(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	PopNoproxyDialog();
}
void mo_exit_program(Widget w, XtPointer clid, XtPointer calld)
{
/*	mo_window * win = (mo_window*) clid; */

	mo_post_exitbox ();
}
void mo_regular_fonts_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_regular_fonts_tkn);
}
void mo_small_fonts_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_small_fonts_tkn);
}
void mo_large_fonts_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_large_fonts_tkn);
}
void mo_regular_helvetica_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_regular_helvetica_tkn);
}
void mo_small_helvetica_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_small_helvetica_tkn);
}
void mo_large_helvetica_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_large_helvetica_tkn);
}
void mo_regular_newcentury_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_regular_newcentury_tkn);
}
void mo_small_newcentury_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_small_newcentury_tkn);
}
void mo_large_newcentury_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_large_newcentury_tkn);
}
void mo_regular_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_regular_lucidabright_tkn);
}
void mo_small_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_small_lucidabright_tkn);
}
void mo_large_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_fonts (win, mo_large_lucidabright_tkn);
}
void mo_default_underlines_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_underlines (win, mo_default_underlines_tkn);
}
void mo_l1_underlines_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_underlines (win, mo_l1_underlines_tkn);
}
void mo_l2_underlines_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_underlines (win, mo_l2_underlines_tkn);
}
void mo_l3_underlines_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_underlines (win, mo_l3_underlines_tkn);
}
void mo_no_underlines_cb(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	mo_set_underlines (win, mo_no_underlines_tkn);
}

void mo_body_color(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	win->body_color = (win->body_color ? 0 : 1);
	XtVaSetValues(win->scrolled_win,
		WbNbodyColors, win->body_color,
		NULL);
}

void mo_body_image(Widget w, XtPointer clid, XtPointer calld)
{                                    
       mo_window * win = (mo_window*) clid;
                                     
       win->body_image = (win->body_image ? 0 : 1);
       XtVaSetValues(win->scrolled_win,  
               WbNbodyImages, win->body_image,
               NULL);                
}                                    
  

void mo_expand_object_current(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	/* ########################### */
	/* mo_refresh_window_text (win); */
	/* ########################### */
}

void mo_delay_object_loads(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	/* win->delay_object_loads = (win->delay_object_loads ? 0 : 1); */
	win->delay_object_loads ^= 1;
	XmxRSetSensitive (win->menubar, (XtPointer)mo_expand_object_current,
		win->delay_object_loads ? XmxSensitive : XmxNotSensitive);
}
void mo_clear_cache(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	MMCacheClearCache ();
      /* Force a complete reload...nothing else we can do -- SWP */
	mo_reload_document(w, (XtPointer) win, NULL);
}
void mo_clear_global_history(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	XmxMakeQuestionDialog (win->base, 
		"Are you sure you want to clear the global history?",
		"mMosaic: Clear Global History",
		clear_history_yes_cb, clear_history_no_cb, 
		(XtPointer)win);
	XtManageChild (Xmx_w);
}

#ifdef MULTICAST
int mc_multicast_enable;
extern mo_window * mc_send_win;
void mo_multicast_send_tog(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	if(win->mc_type != MC_MO_TYPE_MAIN) {
		fprintf(stderr, "mo_multicast_send_tog: Bug, Please report...\n");
		assert(0);	/* something goes wrong */
	}
	if(mc_send_win){ /* j'emets. Faut que j'arrete */
		XmxRSetToggleState(mc_send_win->menubar, (XtPointer)mo_multicast_send_tog,
			XmxNotSet);
		McStopSendHyperText(win);
		mc_send_win = NULL;
	} else { 	/* je n'emets pas. Il faut */
		XmxRSetToggleState(win->menubar, (XtPointer)mo_multicast_send_tog,
			XmxSet);
		mc_send_win = win;
		McStartSender(win);
	}
}

void mo_multicast_show_participant(Widget w, XtPointer clid, XtPointer calld)
{
	mo_window * win = (mo_window*) clid;

	if(mc_show_participant_flag){
		XmxRSetToggleState(win->menubar,
			(XtPointer)mo_multicast_show_participant, XmxNotSet);
		McPopdownMemberList();
	} else {
		XmxRSetToggleState(win->menubar,
                         (XtPointer)mo_multicast_show_participant, XmxSet);
		McPopupMemberList();
	}
	mc_show_participant_flag = !mc_show_participant_flag;
}
#endif

/* --------------------------- Colleen menubar ---------------------------- */
/* typedef struct _XmxMenubarStruct {
 *        String namestr;
 *        char mnemonic;
 *        void (*func)(Widget, XtPointer, XtPointer);
 *        XtPointer data;
 *        struct _XmxMenubarStruct *sub_menu;
 *} XmxMenubarStruct;
 */

/* File Menu */
static XmxMenubarStruct file_menuspec[30] = {
	{ "New",	'N',	mo_new_window,	NULL, NULL },
        { "Clone",	'C',	mo_clone_window,NULL, NULL },
	{ "----", 	'\0', 	NULL, 		NULL, NULL }, 	/* spacer */

        { "Open URL...", 'O',	mo_open_document,NULL, NULL},
        { "Open Local...", 'L',	mo_open_local_document,NULL, NULL},
	{ "----", 	'\0', 	NULL, 		NULL, NULL },	/* spacer */

        { "Reload Current", 'R',mo_reload_document,NULL, NULL},
        { "Reload Images", 'a',	mo_reload_document_and_object, NULL, NULL},
	{ "----",	 '\0', 	NULL, 		NULL, NULL },	/* spacer */

        { "Find In Current", 'I', mo_search,	NULL, NULL},
        { "View Source...", 'V',mo_document_source,NULL, NULL},
        { "Edit Source...", 'E',mo_document_edit,NULL, NULL},
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

        { "Save As...",	'S',	mo_save_document,NULL, NULL},
        { "Print...",	'P',	mo_print_document,NULL, NULL},
        { "Mail To...", 'M',	mo_mail_document,NULL, NULL},
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

#ifdef KRB5
        { "Kerberos v5 Login...", '5', mo_kerberosv5_login,NULL, NULL},
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */
#endif  
        { "Proxy List...", '0',	mo_proxy,	NULL, NULL},
        { "No Proxy List...", '1', mo_no_proxy,	NULL, NULL},
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

        { "Close",	'W',	mo_close_window, NULL, NULL},
        { "Exit Program...", 'x', mo_exit_program, NULL, NULL},
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* Fonts Sub-Menu */
static XmxMenubarStruct fnts_menuspec[16] = {
	{ "<Times Regular", 'T',mo_regular_fonts_cb,NULL, NULL },
	{ "<Times Small", 'S',mo_small_fonts_cb,NULL, NULL },
	{ "<Times Large", 'L',mo_large_fonts_cb,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "<Helvetica Regular", 'H',mo_regular_helvetica_cb,NULL, NULL },
	{ "<Helvetica Small", 'e',mo_small_helvetica_cb,NULL, NULL },
	{ "<Helvetica Large", 'v',mo_large_helvetica_cb,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "<New Century Regular", 'N',mo_regular_newcentury_cb,NULL, NULL },
	{ "<New Century Small", 'w',mo_small_newcentury_cb,NULL, NULL },
	{ "<New Century Large", 'C',mo_large_newcentury_cb,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "<Lucida Regular", 'L',mo_regular_lucidabright_cb,NULL, NULL },
	{ "<Lucida Small", 'u',mo_small_lucidabright_cb,NULL, NULL },
	{ "<Lucida Large", 'i',mo_large_lucidabright_cb,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* Underline Sub-Menu */
static XmxMenubarStruct undr_menuspec[6] = {
	{ "<Default Underlines",'D',	mo_default_underlines_cb,NULL, NULL },
	{ "<Light Underlines",	'L',	mo_l1_underlines_cb,NULL, NULL },
	{ "<Medium Underlines",	'M',	mo_l2_underlines_cb,NULL, NULL },
	{ "<Heavy Underlines",	'H',	mo_l3_underlines_cb,NULL, NULL },
	{ "<No Underlines",	'N',	mo_no_underlines_cb,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* Options Menu */
static XmxMenubarStruct opts_menuspec[17] = {
	{ "#Body Color", 'y',	mo_body_color,	NULL, NULL },
	{ "#Body Image", 'y',   mo_body_image,  NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "#Delayed Image",'D',	mo_delay_object_loads,NULL, NULL },
	{ "Load Images",'L',	mo_expand_object_current,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Flush Cache",'I',	mo_clear_cache,NULL, NULL },
        { "Flush Password Cache",'P',mo_clear_passwd_cache,NULL, NULL },
	{ "Clear Global History...",'C',mo_clear_global_history,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "!Fonts",	'F',	NULL,		NULL, fnts_menuspec },
	{ "Anchor Underlines",'A',NULL,		NULL, undr_menuspec },
	{ "+Agent Spoofs",'g',	mo_agent_spoofs,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* Navigation Menu */
static XmxMenubarStruct navi_menuspec[15] = {
	{ "Back",	'B',	mo_back,	NULL, NULL },
	{ "Forward",	'F',	mo_forward,	NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Home Document",'D',	mo_home_document,NULL, NULL },
	{ "Window History...",'W',mo_history_list_cb,NULL, NULL },
	{ "Document Links...",'L',mo_links_window,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Hotlist...",'H',	mo_hotlist_postit,NULL, NULL },
	{ "Add To Hotlist",'A',	mo_register_node_in_default_hotlist,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Internet Starting Points",'I',mo_network_starting_points,NULL, NULL },
	{ "Internet Resource Meta-Index",'M',mo_internet_metaindex,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* Help Menu */
static XmxMenubarStruct help_menuspec[17] = {
	{ "About mMosaic ...",'A',mo_help_about,NULL, NULL },
	{ "Manual XMosaic...",'M',mo_mosaic_manual,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "What's New XMosaic...",'W',mo_whats_new,NULL, NULL },
	{ "Demo XMosaic...",'D',mo_mosaic_demopage,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Help on Version 2.7b5...",'V',mo_help_onversion,NULL, NULL },
	{ "On Window XMosaic...",'O',mo_help_onwindow,NULL, NULL },
	{ "On FAQ XMosaic...",'F',mo_help_faq,	NULL, NULL},
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "On HTML XMosaic...",'H',mo_help_html,NULL, NULL },
	{ "On URLS XMosaic...",'U',mo_help_url,	NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Mail Tech Support...",'M',mo_techsupport,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

#ifdef MULTICAST
/* Muticast Menu */
static XmxMenubarStruct multicast_menuspec[3] = {
	{ "#Send Enable",	'S', mo_multicast_send_tog,NULL, NULL },
	{ "#Show Particpants",	'P', mo_multicast_show_participant,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};
#endif /*MULTICAST */

#ifdef NEWS
/* News Format Sub-Menu */
static XmxMenubarStruct newsfmt_menuspec[3] = {
	{ "<Thread View",	'T', mo_news_fmt0,NULL, NULL },
	{ "<Article View",	'G', mo_news_fmt1,NULL, NULL },
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};

/* News Menu */
static XmxMenubarStruct news_menuspec[27] = {
	{ "Next",	'N',	mo_news_next,	NULL, NULL },
	{ "Prev",	'P',	mo_news_prev,	NULL, NULL },
	{ "Next Thread",'t',	mo_news_nextt,	NULL, NULL },
	{ "Prev Thread",'v',	mo_news_prevt,	NULL, NULL },
	{ "Article Index",'I',	mo_news_index,	NULL, NULL },
	{ "Group Index",'G',	mo_news_groups,	NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Post",	'o',	mo_news_post,	NULL, NULL },
	{ "Followup",	'F',	mo_news_follow,	NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Subscribe",	's',	mo_news_sub,	NULL, NULL },
	{ "Unsubscribe",'u',	mo_news_unsub,	NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "<Show All Groups",'A',mo_news_grp0,	NULL, NULL },
        { "<Show Unread Subscribed Groups",'S',mo_news_grp1,NULL, NULL },
        { "<Show All Subscribed Groups','R",mo_news_grp2,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "<Show All Articles",'l',mo_news_art0,NULL, NULL },
        { "<Show Only Unread Articles",'n',mo_news_art1,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

	{ "Mark Group Read",'e',mo_news_mread,	NULL, NULL },
	{ "Mark Group Unread",'d',mo_news_munread,NULL, NULL },
	{ "Mark Article Unread",'M',mo_news_maunread,NULL, NULL },
	{ "----",	'\0', 	NULL, 		NULL, NULL },	/* spacer */

/*	{ "Flush News Data",'F',mo_news_flush,	NULL)*/
	{ "Flush Group Data",'D',mo_news_flushgroup,NULL, NULL },
	{ "Thread Style",'T',	NULL,		NULL, newsfmt_menuspec},
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
#endif /* NEWS */


/* The Menubar */
static XmxMenubarStruct menuspec[9] = {
	{ "File",	'F',	NULL,		NULL, file_menuspec},
	{ "Options",	'O',	NULL,		NULL, opts_menuspec},
	{ "Navigate",	'N',	NULL,		NULL, navi_menuspec},
#ifdef NEWS
	{ "News",	'w',	NULL,		NULL, news_menuspec},
#endif
#ifdef MULTICAST
	{ "Multicast",	'M',	NULL,		NULL, multicast_menuspec},
#endif
	{ "Help",	'H',	NULL,		NULL, help_menuspec},
/* Dummy submenu. */
        { NULL,		'\0',	NULL,		NULL, NULL },	/* end */
        { NULL,		'\0',	NULL,		NULL, NULL }	/* end */
};


/* --------------------------- format options ----------------------------- */
/* typedef struct _XmxOptionMenuStruct {
 *        String namestr;
 *        XtCallbackProc data;
 *        int set_state;
 *        struct _mo_window * win; 
 * } XmxOptionMenuStruct;
*/
XmxOptionMenuStruct format_opts[5] = {
	{ "Plain Text",	mo_plaintext_cb,	XmxNotSet, NULL},
	{ "Format Text", mo_formatted_text_cb,	XmxNotSet, NULL},
	{ "PostScript",	mo_postscript_cb,	XmxNotSet, NULL},
	{ "HTML",	mo_html_cb,	XmxNotSet, NULL},
	{ NULL, 	NULL, 		XmxNotSet, NULL}
};

/* -------------------- mo_make_document_view_menubar --------------------- */


XmxMenuRecord *mo_make_document_view_menubar (Widget form, mo_window * win)
{
	XmxMenuRecord *toBeReturned;
	Widget _menubar;
	int i;

                	/* Preset resources applied to main menubar only. */
        _menubar = XmCreateMenuBar (form, "menubar", Xmx_wargs, Xmx_n);
        XtManageChild (_menubar);
        toBeReturned = _XmxMenuCreateRecord (_menubar); /*Create XmxMenuRecord.*/
        Xmx_n = 0;
        _XmxRCreateMenubar(_menubar, menuspec,  toBeReturned, (struct _mo_window *)win);
        Xmx_n = 0;
        Xmx_w = _menubar;

#ifdef MULTICAST
	XmxRSetSensitive (toBeReturned, (XtPointer) mo_multicast_send_tog,
                	XmxNotSensitive);
	XmxRSetSensitive (toBeReturned, (XtPointer) mo_multicast_show_participant,
                	XmxNotSensitive);
#endif
	win->agent_state_pulldown = NULL;
	win->agspd_cbd = (AgentSpoofCBStruct*)calloc(numAgents , sizeof(AgentSpoofCBStruct));
	for(i=0; i<numAgents; i++){
		win->agspd_cbd[i].w= NULL; /*widget is create in the first callback */
		win->agspd_cbd[i].d= 0;
		win->agspd_cbd[i].win= win;
	}
	return toBeReturned;
}

/* ---------------------------- Agent Spoofing ---------------------------- */

char * MMGetUserAgent()
{
	return agent[selectedAgent];
}

/* Agent Spoofing is simple. NCSA's real agent is always a member of the
 * menu. Any more than that, you can add to the file in your home directory
 * called ".mosaic-spoof-agents".
 */

void loadAgents(void) 
{
	FILE *fp;
	char fname[BUFSIZ],buf[512];
	char *ptr;
	char buf1[512];

	agent=(char **)calloc(MAX_AGENTS+1,sizeof(char *));
	sprintf(buf1,"mMosaic/%s",
		mMosaicAppVersion ? mMosaicAppVersion : "0.0");
	agent[0]=strdup(buf1);
	numAgents=1;

	sprintf(fname,"%s/agents",mMosaicRootDirName);

	if (!(fp=fopen(fname,"r")))
		return;

	while (!feof(fp)) {
		fgets(buf,511,fp);
		if (feof(fp))
			break;
		if (*buf && *buf!='#') {
			buf[strlen(buf)-1]='\0';
			for (ptr=buf; *ptr && isspace(*ptr); ptr++);
			if (*ptr=='+') { /* This is to be the default*/
				if (*(ptr+1)) {
					agent[numAgents]=strdup(ptr+1);
					selectedAgent=numAgents;
				} else
					continue;
			} else 	
				if (*ptr) {
					agent[numAgents]=strdup(ptr);
				} else
					continue;
			numAgents++;
		}
	}
	fclose(fp);
}
