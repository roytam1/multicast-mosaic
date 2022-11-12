/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#ifndef __XMX_H__
#define __XMX_H__

/* --------------------------- SYSTEM INCLUDES ---------------------------- */

/* Generic X11/Xt/Xm includes. */
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include <Xm/Xm.h>

#if (XmVERSION == 1)&&(XmREVISION >= 2)
#ifndef MOTIF1_2
#define MOTIF1_2
#endif
#endif

#if 0

#ifdef __sgi
/* Fast malloc. */
#include <malloc.h>
/* For GLXconfig type. */
#include <X11/Xirisw/GlxMDraw.h>
#endif

#ifdef _IBMR2
/* nothing that I know of */
#endif /* _IBMR2 */

#endif /* if 0 */

/* --------------------------- INTERNAL DEFINES --------------------------- */

/* Maximum number of resource args usable --- should be large
   since applications also can add resources. */
#define XmxMaxNumArgs 30

/* ------------------------------ VARIABLES ------------------------------- */

/* These three will also be used from application code. */
extern int Xmx_n;
extern Arg Xmx_wargs[];
extern Widget Xmx_w;

/* This probably won't be needed to be accessed
   directly by an application; if not, it should be
   moved to XmxP.h. */
extern int Xmx_uniqid;

/* ------------------------------ CONSTANTS ------------------------------- */

/* These probably shouldn't be necessary, since we use
   Gadgets whenever possible. */
#define XmxWidget 0
#define XmxGadget 1

/* XmxFrame types. */
#define XmxShadowIn        0
#define XmxShadowOut       1
#define XmxShadowEtchedIn  2
#define XmxShadowEtchedOut 3

/* Currently unused. */
#define XmxLeft   XmALIGNMENT_BEGINNING
#define XmxCenter XmALIGNMENT_CENTER
#define XmxRight  XmALIGNMENT_END

/* Null values for some arguments. */
#define XmxNotDisplayed -99999
#define XmxNoPosition   -99999
#define XmxNoOffset     -99999

/* States for togglebuttons. */
#define XmxUnset  0
#define XmxNotSet XmxUnset
#define XmxSet    1

/* Sensitivity states. */
#define XmxUnsensitive 0
#define XmxNotSensitive XmxUnsensitive
#define XmxSensitive 1

/* Types of togglebuttons. */
#define XmxOneOfMany 0
#define XmxNOfMany   1

/* Constraint possibilities. */
#define XmxCwidget XmATTACH_WIDGET
#define XmxCform   XmATTACH_FORM
#define XmxCnone   XmATTACH_NONE

/* ---------------------------- MENU TYPEDEFS ----------------------------- */

/* Struct used by app when loading option menu.  The 'set_state'
   parameter is used to indicate menu history for the option menu;
   the entry with 'XmxSet' is used. */
typedef struct _XmxOptionMenuStruct
{
	String namestr;
	XtCallbackProc data;
	int set_state;
	struct mo_window * win;
} XmxOptionMenuStruct;

/* Toggle menu and option menu accept same struct. */
typedef XmxOptionMenuStruct XmxToggleMenuStruct;

/* Menubar uses a recursive struct. */
typedef struct _XmxMenubarStruct
{
	String namestr;
	char mnemonic;
	void (*func)(Widget, XtPointer, XtPointer);
	XtPointer data;
	struct _XmxMenubarStruct *sub_menu;
} XmxMenubarStruct;

/* --------------------------- RECORD TYPEDEFS ---------------------------- */

/* These typedefs, while public, are not guaranteed to remain static
   and should not be actually used by an application. */

/* A single entry in a menu (menubar, toggle menu, or option menu),
   tagged by the integer token used as callback_data. */
typedef struct _XmxMenuEntry
{
  Widget w;
  XtPointer token;
  struct _XmxMenuEntry *next;
} XmxMenuEntry;

/* A menu (menubar, toggle menu, or option menu). */
typedef struct _XmxMenuRecord
{
  Widget base;
  XmxMenuEntry *first_entry;
} XmxMenuRecord;

/* -------------------------------- MACROS -------------------------------- */

/* Callback definitions and prototypes. */
#define XmxCallback(name)						      \
  void name (Widget w, XtPointer client_data, XtPointer call_data)
#define XmxCallbackPrototype(name)                                            \
  extern void name (Widget, XtPointer, XtPointer)

/* Event handler functions and prototypes. */
#define XmxEventHandler(name)						      \
  void name (Widget w, XtPointer client_data, XEvent *event, Boolean *cont)
#define XmxEventHandlerPrototype(name) 				              \
  extern void name (Widget, XtPointer, XEvent *, Boolean *)

/* Shortcut for XtAppInitialize --- of dubious value. */
#define XmxInit()							      \
  XtAppInitialize (&app_context, "XmxApplication", NULL, 0, &argc, argv,      \
                   NULL, Xmx_wargs, Xmx_n);

/* ------------------------------ PROTOTYPES ------------------------------ */

/* Xmx.c */
extern int XmxMakeNewUniqid (void);
extern void XmxSetUniqid (int);
extern void XmxZeroUniqid (void);
extern int XmxExtractUniqid (int);

extern void XmxAddCallback (Widget, String, XtCallbackProc, int);
extern void XmxAddEventHandler (Widget, EventMask, XtEventHandler, int);
extern void XmxRemoveEventHandler (Widget, EventMask, XtEventHandler, int);

extern void XmxStartup (void);
extern void XmxSetArg (String, XtArgVal);
extern void XmxSetValues (Widget);
extern void XmxManageRemanage (Widget);
extern void XmxSetSensitive (Widget, int);

extern Widget XmxMakePushButton (Widget, String, XtCallbackProc, XtPointer);
extern Widget XmxMakeNamedPushButton (Widget, String, String, 
				XtCallbackProc, XtPointer);
extern Widget XmxMakeBlankButton (Widget, XtCallbackProc, XtPointer);
extern Widget XmxMakeCommand (Widget, String, XtCallbackProc, XtPointer);
extern Widget XmxMakeScrolledList (Widget, XtCallbackProc, XtPointer);
extern Widget XmxMakeDrawingArea (Widget, int, int);
extern Widget XmxMakeRadioBox (Widget);
extern Widget XmxMakeOptionBox (Widget);
extern Widget XmxMakeToggleButton (Widget, String, XtCallbackProc, XtPointer);
extern void XmxSetToggleButton (Widget button, int set_state);
extern Widget XmxMakeScale (Widget, XtCallbackProc, XtPointer, String, 
                            int, int, int, int);
extern void XmxAdjustScale (Widget, int);
extern Widget XmxMakeFrame (Widget, int);
extern Widget XmxMakeForm (Widget);
extern void XmxSetPositions (Widget, int, int, int, int);
extern void XmxSetOffsets (Widget, int, int, int, int);
extern void XmxSetConstraints (Widget, int, int, int, int, Widget, Widget, 
                               Widget, Widget);
extern Widget XmxMakeVerticalRowColumn (Widget);
extern Widget XmxMakeHorizontalRowColumn (Widget);
extern Widget XmxMakeNColumnRowColumn (Widget, int);
extern Widget XmxMakeVerticalBboard (Widget);
extern Widget XmxMakeVerticalBboardWithFont (Widget, String);
extern Widget XmxMakeHorizontalBboard (Widget);
extern void XmxAdjustLabelText (Widget, String);
extern Widget XmxMakeLabel (Widget, String);
extern Widget XmxMakeNamedLabel (Widget, String, String);
extern Widget XmxMakeBlankLabel (Widget);
extern Widget XmxMakeErrorDialog (Widget, String, String);
extern Widget XmxMakeInfoDialog (Widget, String, String);
extern Widget XmxMakeQuestionDialog (Widget, String, String, XtCallbackProc, 
                                     XtCallbackProc, XtPointer);
extern XmString XmxMakeXmstrFromFile (String);
extern XmString XmxMakeXmstrFromString (String);
extern Widget XmxMakeBboardDialog (Widget, String);
extern Widget XmxMakeFormDialog (Widget, String);
extern Widget XmxMakeFileSBDialog (Widget, String, String, XtCallbackProc, 
                                   XtPointer);
extern Widget XmxMakeHelpDialog (Widget, XmString, String);
extern Widget XmxMakeHelpTextDialog (Widget, String, String, Widget *);
extern void XmxAdjustHelpDialogText (Widget, XmString, String);
extern void XmxAdjustDialogTitle (Widget, String);
extern Widget XmxMakeHorizontalSeparator (Widget);
extern Widget XmxMakeHorizontalSpacer (Widget);
extern Widget XmxMakeHorizontalBoundary (Widget);
extern Widget XmxMakeScrolledText (Widget);
extern Widget XmxMakeText (Widget);
extern Widget XmxMakeTextField (Widget);
extern void XmxTextSetString (Widget, String);
extern void XmxTextInsertString (Widget, String);
extern String XmxTextGetString (Widget);
extern void XmxAddCallbackToText (Widget, XtCallbackProc, XtPointer);

#if 0

#ifdef __sgi
extern Widget XmxMakeDrawingVolume 
  (Widget, int, int, GLXconfig *, XtCallbackProc, XtCallbackProc,
   XtCallbackProc);
extern void XmxInstallColormaps (Widget, Widget);
extern void XmxInstallColormapsWithOverlay (Widget, Widget);
extern void XmxWinset (Widget);
#endif

#ifdef _IBMR2
extern Widget XmxMakeDrawingVolume 
  (Widget, int, int, XtCallbackProc, XtCallbackProc,
   XtCallbackProc);
extern void XmxInstallColormaps (Widget, Widget);
extern void XmxWinset (Widget);
#endif

#endif /* if 0 */

extern void XmxApplyBitmapToLabelWidget (Widget, String, unsigned int, 
                                         unsigned int);
extern Pixmap XmxCreatePixmapFromBitmap (Widget, String, unsigned int,
                                         unsigned int);
extern void XmxApplyPixmapToLabelWidget (Widget, Pixmap);

extern Widget XmxMakeFormAndOneButton (Widget, XtCallbackProc, String, XtPointer);
extern Widget XmxMakeFormAndTwoButtons (Widget, String, String, 
		XtCallbackProc, XtCallbackProc,XtPointer);
extern Widget XmxMakeFormAndTwoButtonsSqueezed (Widget, String, String, 
		XtCallbackProc, XtCallbackProc, XtPointer);
extern Widget XmxMakeFormAndThreeButtons (Widget, String, String, String, 
		XtCallbackProc, XtCallbackProc, XtCallbackProc, XtPointer);
extern Widget XmxMakeFormAndThreeButtonsSqueezed (Widget, String, String, String,
		XtCallbackProc, XtCallbackProc, XtCallbackProc, XtPointer);
extern Widget XmxMakeFormAndFourButtons(Widget, 
		String, String, String, String,
		XtCallbackProc, XtCallbackProc, XtCallbackProc, XtCallbackProc,
		XtPointer);
extern Widget XmxMakeFormAndFiveButtons(Widget, String, String, String, String, 
		String,
		XtCallbackProc, XtCallbackProc, XtCallbackProc, XtCallbackProc, 
		XtCallbackProc, XtPointer);
extern int XmxModalYesOrNo (Widget parent, XtAppContext app,
                            char *questionstr, char *yesstr,
                            char *nostr);
extern char *XmxModalPromptForString (Widget parent, XtAppContext app, 
                                      char *questionstr, char *yesstr, 
                                      char *nostr);
extern char *XmxModalPromptForPassword (Widget parent, XtAppContext app, 
                                        char *questionstr, char *yesstr, 
                                        char *nostr);

/* Xmx2.c */
extern void XmxRSetSensitive (XmxMenuRecord *, XtPointer, int);
extern void XmxRSetToggleState (XmxMenuRecord *, XtPointer, int);
extern void XmxRUnsetAllToggles (XmxMenuRecord *);
extern void XmxRSetOptionMenuHistory (XmxMenuRecord *, XtPointer);
extern void XmxRSetValues (XmxMenuRecord *, int);
extern Widget XmxRGetWidget (XmxMenuRecord *, int);

extern XmxMenuRecord *XmxRMakeOptionMenu (Widget, String, 
                                          XmxOptionMenuStruct *);
extern XmxMenuRecord *XmxRMakeToggleMenu (Widget, int, XtCallbackProc, 
                                          XmxToggleMenuStruct *);
extern XmxMenuRecord *XmxRMakeMenubar (Widget, XmxMenubarStruct *);
extern XmxMenuRecord * _XmxMenuCreateRecord (Widget base);
extern void _XmxRCreateMenubar (Widget menu, XmxMenubarStruct *menulist,
                    XmxMenuRecord *rec, struct mo_window * win);

extern void XmxMakeInfoDialogWait (Widget parent, XtAppContext app,
        char *infostr, char *titlestr, char *yesstr);
extern void XmxMakeErrorDialogWait (Widget parent, XtAppContext app,
        char *infostr, char *titlestr, char *yesstr);
extern void _XmxRDestroyMenubar(XmxMenuRecord * rec);
#endif /* __XMX_H__ */
