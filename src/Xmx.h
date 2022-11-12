/* Please read copyright.tmpl. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#ifndef __XMX_H__
#define __XMX_H__

#if (XmVERSION == 1)&&(XmREVISION >= 2)
#ifndef MOTIF1_2
#define MOTIF1_2
#endif
#endif

/* --------------------------- INTERNAL DEFINES --------------------------- */

/* Maximum number of resource args usable --- should be large
   since applications also can add resources. */
#define XmxMaxNumArgs 40

/* ------------------------------ VARIABLES ------------------------------- */

/* These three will also be used from application code. */
extern int Xmx_n;
extern Arg Xmx_wargs[];
extern Widget Xmx_w;

/* ------------------------------ CONSTANTS ------------------------------- */

/* XmxFrame types. */
#define XmxShadowIn        0
#define XmxShadowOut       1
#define XmxShadowEtchedIn  2
#define XmxShadowEtchedOut 3

/* Null values for some arguments. */
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

/* ---------------------------- MENU TYPEDEFS ----------------------------- */

/* Struct used by app when loading option menu.  The 'set_state'
   parameter is used to indicate menu history for the option menu;
   the entry with 'XmxSet' is used. */
typedef struct _XmxOptionMenuStruct
{
	String namestr;
	XtCallbackProc data;
	int set_state;
	struct _mo_window * win;
} XmxOptionMenuStruct;


/* Menubar uses a recursive struct. */
typedef struct _XmxMenubarStruct
{
	String namestr;
	char mnemonic;
	void (*func)(Widget, XtPointer, XtPointer);
	XtPointer data;		/* menubar Struct Data */
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

/* ------------------------------ PROTOTYPES ------------------------------ */

/* Xmx.c */

extern void XmxSetArg (String, XtArgVal);
extern void XmxSetValues (Widget);
extern void XmxManageRemanage (Widget);
extern void XmxSetSensitive (Widget, int);

extern Widget XmxMakePushButton (Widget, String, XtCallbackProc, XtPointer);
extern Widget XmxMakeScrolledList (Widget, XtCallbackProc, XtPointer);
extern Widget XmxMakeRadioBox (Widget);
extern Widget XmxMakeToggleButton (Widget, String, XtCallbackProc, XtPointer);
extern void XmxSetToggleButton (Widget button, int set_state);
extern Widget XmxMakeFrame (Widget, int);
extern Widget XmxMakeForm (Widget);
extern void XmxSetPositions (Widget, int, int, int, int);
extern void XmxSetOffsets (Widget, int, int, int, int);
extern void XmxSetConstraints (Widget, int, int, int, int, Widget, Widget, 
                               Widget, Widget);
extern void XmxAdjustLabelText (Widget, String);
extern Widget XmxMakeLabel (Widget, String);
extern void  XmxMakeErrorDialog (Widget, char *, char *);
extern Widget XmxMakeInfoDialog (Widget, char *, char *);
extern Widget XmxMakeQuestionDialog (Widget, String, String, XtCallbackProc, 
                                     XtCallbackProc, XtPointer);
extern XmString XmxMakeXmstrFromString (String);
extern Widget XmxMakeFormDialog (Widget, String);
extern Widget XmxMakeFileSBDialog (Widget, String, String, XtCallbackProc, 
                                   XtPointer);
extern Widget XmxMakeHorizontalSeparator (Widget);
extern Widget XmxMakeScrolledText (Widget);
extern Widget XmxMakeText (Widget);
extern Widget XmxMakeTextField (Widget);
extern void XmxTextSetString (Widget, String);
extern void XmxTextInsertString (Widget, String);

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
extern void XmxRSetValues (XmxMenuRecord *, int);
extern Widget XmxRGetWidget (XmxMenuRecord *, int);

extern XmxMenuRecord *XmxRMakeOptionMenu (Widget, String, 
                                          XmxOptionMenuStruct *);
extern XmxMenuRecord *XmxRMakeMenubar (Widget, XmxMenubarStruct *);
extern XmxMenuRecord * _XmxMenuCreateRecord (Widget base);
extern void _XmxRCreateMenubar (Widget menu, XmxMenubarStruct *menulist,
                    XmxMenuRecord *rec, struct _mo_window * win);

extern void XmxMakeInfoDialogWait (Widget parent, XtAppContext app,
        char *infostr, char *titlestr, char *yesstr);
extern void XmxMakeErrorDialogWait (Widget parent, XtAppContext app,
        char *infostr, char *titlestr);
extern void _XmxRDestroyMenubar(XmxMenuRecord * rec);
#endif /* __XMX_H__ */
