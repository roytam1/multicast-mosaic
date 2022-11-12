/* Please read copyright.tmpl. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdlib.h>
#include <assert.h>
#include <Xm/XmAll.h>

#include "Xmx.h"

/* ---------------------------- FILE VARIABLES ---------------------------- */

/* Variables accessed through Xmx.h as extern. */
int    Xmx_n = 0;
Arg    Xmx_wargs[XmxMaxNumArgs];
Widget Xmx_w;

/* -------------------------- INTERNAL CALLBACKS -------------------------- */

/* Internal routine to unmanage file selection box on Cancel. */
static XmxCallback (_XmxCancelCallback)
{
	XtUnmanageChild (w);
}

/* -------------------------- UTILITY FUNCTIONS --------------------------- */

/* sets an arg */
void XmxSetArg (String arg, XtArgVal val)
{
  	XtSetArg (Xmx_wargs[Xmx_n], arg, val);
  	Xmx_n++;
}

void XmxSetValues (Widget w)
{
  	XtSetValues (w, Xmx_wargs, Xmx_n);
  	Xmx_n = 0;
}

void XmxManageRemanage (Widget w)
{
	if (XtIsManaged (w))
		if (XtIsShell(w))
			XRaiseWindow (XtDisplay (w), XtWindow (w));
		else
			XMapRaised (XtDisplay (w), XtWindow (XtParent(w)));
	else
		XtManageChild (w);
}

void XmxSetSensitive (Widget w, int state)
{
	assert (state == XmxSensitive || state == XmxUnsensitive);
	XtSetSensitive (w, (state == XmxSensitive) ? True : False);
}

/* ---------------- WIDGET CREATION AND HANDLING ROUTINES ----------------- */
/* ----------------------------- PUSHBUTTONS ------------------------------ */

/* args work */
Widget XmxMakePushButton (Widget parent, String name, XtCallbackProc cb,
                          XtPointer cb_data)
{
	XmString xmstr = NULL;

	if (name) {
		xmstr = XmStringCreateLtoR (name, XmSTRING_DEFAULT_CHARSET);
		XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
	}
	Xmx_w = XtCreateManagedWidget ("pushbutton", xmPushButtonWidgetClass,
			parent, Xmx_wargs, Xmx_n);
	XtAddCallback (Xmx_w, XmNactivateCallback, cb, (XtPointer)(cb_data));
	if (xmstr)
		XmStringFree (xmstr);
	Xmx_n = 0;
	return Xmx_w;
}

/* ---------------------------- SCROLLED LIST ----------------------------- */

/* args work */
Widget XmxMakeScrolledList (Widget parent, XtCallbackProc cb, XtPointer cb_data)
{
  Xmx_w = XmCreateScrolledList (parent, "scrolled_list", Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);
/* defaultAction gets triggered on double click and sends item along with it... */
  XtAddCallback (Xmx_w, XmNdefaultActionCallback, cb, cb_data);
  Xmx_n = 0;
  return Xmx_w;
}

/* ------------------------ TOGGLE BUTTONS & BOXES ------------------------ */

/* args work */
Widget XmxMakeRadioBox (Widget parent)
{
  /* Could set XmxNspacing here to avoid having to play with
     margins for each togglebutton. */
  XmxSetArg (XmNspacing, (XtArgVal)0);
  XmxSetArg (XmNentryClass, (XtArgVal)xmToggleButtonGadgetClass);
  Xmx_w = XmCreateRadioBox (parent, "radiobox", Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);
  Xmx_n = 0;
  return Xmx_w;
}

/* args work */
Widget XmxMakeToggleButton (Widget parent, String name, XtCallbackProc cb,
                            XtPointer cb_data)
{
	XmString label = XmStringCreateLtoR (name, XmSTRING_DEFAULT_CHARSET);
	XmxSetArg (XmNlabelString, (XtArgVal)label);
	XmxSetArg (XmNmarginHeight, (XtArgVal)0);
	Xmx_w = XtCreateManagedWidget("togglebutton", 
		xmToggleButtonWidgetClass, parent, Xmx_wargs, Xmx_n);
/* Used to be XmNarmCallback --- probably not right. */
	if (cb)
		XtAddCallback (Xmx_w, XmNvalueChangedCallback, cb, cb_data);
	XmStringFree (label);
	Xmx_n = 0;
	return Xmx_w;
}

/* args work */
void XmxSetToggleButton (Widget button, int set_state)
{
  assert (set_state == XmxSet || set_state == XmxUnset);
  XmToggleButtonSetState (button, (set_state == XmxSet) ? True : False, False);
  Xmx_n = 0;
  return;
}

/* -------------------------------- SCALES -------------------------------- */

/* args work */
Widget XmxMakeFrame (Widget parent, int shadow)
{
  assert (shadow == XmxShadowIn || shadow == XmxShadowOut || shadow == XmxShadowEtchedIn || shadow == XmxShadowEtchedOut);
  switch (shadow) {
    case XmxShadowIn:
      XmxSetArg (XmNshadowType, (XtArgVal)XmSHADOW_IN);  break;
    case XmxShadowOut:
      XmxSetArg (XmNshadowType, (XtArgVal)XmSHADOW_OUT);  break;
    case XmxShadowEtchedIn:
      XmxSetArg (XmNshadowType, (XtArgVal)XmSHADOW_ETCHED_IN);  break;
    case XmxShadowEtchedOut:
      XmxSetArg (XmNshadowType, (XtArgVal)XmSHADOW_ETCHED_OUT);  break;
    }
  Xmx_w = XtCreateManagedWidget ("frame", xmFrameWidgetClass,
                                 parent, Xmx_wargs, Xmx_n);
  Xmx_n = 0;
  return Xmx_w;
}

/* -------------------------------- FORMS --------------------------------- */

/* args work */
Widget XmxMakeForm (Widget parent)
{
  Xmx_w = XtCreateManagedWidget ("form", xmFormWidgetClass, parent,
				 Xmx_wargs, Xmx_n);
  Xmx_n = 0;
  return Xmx_w;
}

/* args sent to w */
void XmxSetPositions (Widget w, int top, int bottom, int left, int right)
{
  if (top != XmxNoPosition) {
      XmxSetArg (XmNtopAttachment, (XtArgVal)XmATTACH_POSITION);
      XmxSetArg (XmNtopPosition, (XtArgVal)top);
    }
  if (bottom != XmxNoPosition) {
      XmxSetArg (XmNbottomAttachment, (XtArgVal)XmATTACH_POSITION);
      XmxSetArg (XmNbottomPosition, (XtArgVal)bottom);
    }
  if (left != XmxNoPosition) {
      XmxSetArg (XmNleftAttachment, (XtArgVal)XmATTACH_POSITION);
      XmxSetArg (XmNleftPosition, (XtArgVal)left);
    }
  if (right != XmxNoPosition) {
      XmxSetArg (XmNrightAttachment, (XtArgVal)XmATTACH_POSITION);
      XmxSetArg (XmNrightPosition, (XtArgVal)right);
    }
  XmxSetValues (w);
  Xmx_n = 0;
  return;
}

/* args sent to w */
void XmxSetOffsets (Widget w, int top, int bottom, int left, int right)
{
  if (top != XmxNoOffset)
    XmxSetArg (XmNtopOffset, (XtArgVal)top);
  if (bottom != XmxNoOffset)
    XmxSetArg (XmNbottomOffset, (XtArgVal)bottom);
  if (left != XmxNoOffset)
    XmxSetArg (XmNleftOffset, (XtArgVal)left);
  if (right != XmxNoOffset)
    XmxSetArg (XmNrightOffset, (XtArgVal)right);
  XmxSetValues (w);
  Xmx_n = 0;
}

/* args sent to w */
void XmxSetConstraints (Widget w, int top, int bottom, int left, int right,
			Widget topw, Widget botw, Widget lefw, Widget rigw)
{
  if (top != XmATTACH_NONE) {
      XmxSetArg (XmNtopAttachment, (XtArgVal)top);
      if (topw)
        XmxSetArg (XmNtopWidget, (XtArgVal)topw);
    }
  
  if (bottom != XmATTACH_NONE) {
      XmxSetArg (XmNbottomAttachment, (XtArgVal)bottom);
      if (botw)
        XmxSetArg (XmNbottomWidget, (XtArgVal)botw);
    }

  if (left != XmATTACH_NONE) {
      XmxSetArg (XmNleftAttachment, (XtArgVal)left);
      if (lefw)
        XmxSetArg (XmNleftWidget, (XtArgVal)lefw);
    }

  if (right != XmATTACH_NONE) {
      XmxSetArg (XmNrightAttachment, (XtArgVal)right);
      if (rigw)
        XmxSetArg (XmNrightWidget, (XtArgVal)rigw);
    }
  XmxSetValues (w);
  Xmx_n = 0;
  return;
}

/* -------------------------------- LABELS -------------------------------- */

/* args work */
void XmxAdjustLabelText (Widget label, String text)
{
  XmString xmstr = XmStringCreateLtoR (text, XmSTRING_DEFAULT_CHARSET);
  XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
  XtSetValues (label, Xmx_wargs, Xmx_n);
  XmStringFree (xmstr);
  Xmx_n = 0;
}

/* args work */
Widget XmxMakeLabel (Widget parent, String name)
{
  XmString xmstr = XmStringCreateLtoR (name, XmSTRING_DEFAULT_CHARSET);
  XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
  Xmx_w = XtCreateManagedWidget ("label", xmLabelWidgetClass,
                                 parent, Xmx_wargs, Xmx_n);
  XmStringFree (xmstr);
  Xmx_n = 0;
  return Xmx_w;
}

/* ------------------------------- DIALOGS -------------------------------- */

/* args work */
Widget XmxMakeInfoDialog (Widget parent, String name, String title)
{
  XmString message = XmStringCreateLtoR (name, XmSTRING_DEFAULT_CHARSET);
  XmString dialog = XmStringCreateLtoR (title, XmSTRING_DEFAULT_CHARSET);
  XmxSetArg (XmNmessageString, (XtArgVal)message);
  XmxSetArg (XmNdialogTitle, (XtArgVal)dialog);

  Xmx_w = XmCreateInformationDialog (parent, "infozoid", Xmx_wargs, Xmx_n);
  XtUnmanageChild (XmMessageBoxGetChild (Xmx_w, XmDIALOG_CANCEL_BUTTON));
  XtUnmanageChild (XmMessageBoxGetChild (Xmx_w, XmDIALOG_HELP_BUTTON));
  XmStringFree (message);
  XmStringFree (dialog);
  Xmx_n = 0;
  return Xmx_w;
}

/* args work */
Widget XmxMakeQuestionDialog (Widget parent, String question, String title,
			      XtCallbackProc cb_yes, 
			      XtCallbackProc cb_no, XtPointer cb_data)
{
  XmString message = XmStringCreateLtoR (question, XmSTRING_DEFAULT_CHARSET);
  XmString dialog = XmStringCreateLtoR (title, XmSTRING_DEFAULT_CHARSET);
  XmString ok = XmStringCreateLtoR ("Yes", XmSTRING_DEFAULT_CHARSET);
  XmString cancel = XmStringCreateLtoR ("No", XmSTRING_DEFAULT_CHARSET);
  XmxSetArg (XmNmessageString, (XtArgVal)message);
  XmxSetArg (XmNdialogTitle, (XtArgVal)dialog);
  XmxSetArg (XmNokLabelString, (XtArgVal)ok);
  XmxSetArg (XmNcancelLabelString, (XtArgVal)cancel);

  Xmx_w = XmCreateQuestionDialog (parent, "question", Xmx_wargs, Xmx_n);
  XtUnmanageChild (XmMessageBoxGetChild (Xmx_w, XmDIALOG_HELP_BUTTON));

  XtAddCallback (Xmx_w, XmNcancelCallback, cb_no, cb_data);
  XtAddCallback (Xmx_w, XmNokCallback, cb_yes, cb_data);

  XmStringFree (message);
  XmStringFree (dialog);
  XmStringFree (ok);
  XmStringFree (cancel);
  Xmx_n = 0;
  return Xmx_w;
}

/* ----------------------------- STRING UTILS ----------------------------- */

/* args do nothing */
XmString XmxMakeXmstrFromString (String mstr)
{
  XmString _xmstr;

  _xmstr = XmStringCreateLtoR (mstr, XmSTRING_DEFAULT_CHARSET);
  return _xmstr;
}

/* args work */
Widget XmxMakeFormDialog (Widget parent, String title)
{
  XmString xmstr = XmStringCreateLtoR (title, XmSTRING_DEFAULT_CHARSET);
  XmxSetArg (XmNdialogTitle, (XtArgVal)xmstr);
  XmxSetArg (XmNautoUnmanage, (XtArgVal)False);

  Xmx_w = XmCreateFormDialog (parent, "formdialog", Xmx_wargs, Xmx_n);
  XmStringFree (xmstr);
  Xmx_n = 0;
  return Xmx_w;
}

/* args work */
Widget XmxMakeFileSBDialog (Widget parent, String title, String selection_txt,
	XtCallbackProc cb, XtPointer cb_data)
{
	Widget _selection_label;
	XmString dialog_title = XmStringCreateLtoR (title, 
					XmSTRING_DEFAULT_CHARSET);
	XmString label = XmStringCreateLtoR (selection_txt, 
					XmSTRING_DEFAULT_CHARSET);

	XmxSetArg (XmNdialogTitle, (XtArgVal)dialog_title);
/* Can't set width of box with XmNwidth here... why not? */
/* this will cause the dialog to only resize if needed. That 
 *   way it won't be growing and shrinking all the time... very annoying. - DXP */
	XmxSetArg (XmNresizePolicy, (XtArgVal)XmRESIZE_GROW);

/* Create the FileSelectionBox with OK and Cancel buttons. */
	Xmx_w = XmCreateFileSelectionDialog (parent, "fsb", Xmx_wargs, Xmx_n);
	XtUnmanageChild(XmFileSelectionBoxGetChild(Xmx_w,XmDIALOG_HELP_BUTTON));
	XtAddCallback (Xmx_w, XmNokCallback, cb, cb_data);
	XtAddCallback (Xmx_w, XmNcancelCallback, 
			(XtCallbackProc)_XmxCancelCallback, 0);

/* Set selection label to specified selection_txt. */
	Xmx_n = 0;
	_selection_label = XmFileSelectionBoxGetChild (Xmx_w, 
					XmDIALOG_SELECTION_LABEL);
	XmxSetArg (XmNlabelString, (XtArgVal)label);
	XtSetValues (_selection_label, Xmx_wargs, Xmx_n);
	XmStringFree (dialog_title);
	XmStringFree (label);
	Xmx_n = 0;
	return Xmx_w;
}

/* Boy, this is a hack. */
static XmxCallback(_XmxHelpTextCancelCallback)
{
	/* This is highly dependent on the button being four layers
	 * below the dialog shell... what a ridiculous hack. */
	XtUnmanageChild (XtParent (XtParent (XtParent (XtParent (w)))));
}

/* ------------------------------ SEPARATORS ------------------------------ */

/* args work */
Widget XmxMakeHorizontalSeparator (Widget parent)
{
  Xmx_w = XmCreateSeparatorGadget (parent, "separator", Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);
  Xmx_n = 0;
  return Xmx_w;
}

/* ------------------------- TEXT & SCROLLED TEXT ------------------------- */

/* args work */
Widget XmxMakeScrolledText (Widget parent)
{
  Xmx_w = XmCreateScrolledText (parent, "scrolledtext",
				Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);

  /* Remember this returns the Text Widget, NOT the ScrolledWindow Widget, 
     which is what needs to be tied into a form.  Use XtParent to get the
     actual ScrolledWindow. */
  Xmx_n = 0;
  return Xmx_w;
}

/* args work */
Widget XmxMakeText (Widget parent)
{
  Xmx_w = XmCreateText (parent, "text", Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);
  Xmx_n = 0;
  return Xmx_w;
}

/* args work */
Widget XmxMakeTextField (Widget parent)
{
  Xmx_w = XmCreateTextField (parent, "textfield", Xmx_wargs, Xmx_n);
  XtManageChild (Xmx_w);
  Xmx_n = 0;
  return Xmx_w;
}

/* args do nothing */
void XmxTextSetString (Widget text, String str)
{
  XmTextSetString (text, str);
  XmTextShowPosition (text, 0);
}

/* Insert a sting into a text widget -- BJS */
void XmxTextInsertString (Widget text, String str)
{
  XmTextInsert(text, XmTextGetInsertionPosition(text), str);
  XmTextShowPosition (text, 0);
}

/* ----------------------------- BITMAP UTILS ----------------------------- */

/* args ignored and reset */
void XmxApplyBitmapToLabelWidget
  (Widget label, String data, unsigned int width, unsigned int height)
{
  Display *_disp;
  Pixel _fg, _bg;
  Pixmap _pix;

  _disp = XtDisplay (label);

  Xmx_n = 0;
  XmxSetArg (XmNforeground, (XtArgVal)(&_fg));
  XmxSetArg (XmNbackground, (XtArgVal)(&_bg));
  XtGetValues (label, Xmx_wargs, Xmx_n);
  Xmx_n = 0;

  _pix = XCreatePixmapFromBitmapData
    (_disp, DefaultRootWindow (_disp), data, width, height, _fg, _bg,
     DefaultDepthOfScreen (DefaultScreenOfDisplay (_disp)));
  XmxSetArg (XmNlabelPixmap, (XtArgVal)_pix);
  XmxSetArg (XmNlabelType, (XtArgVal)XmPIXMAP);
  XmxSetValues (label);

  Xmx_n = 0;
  return;
}

/* args ignored and reset */
Pixmap XmxCreatePixmapFromBitmap
  (Widget label, String data, unsigned int width, unsigned int height)
{
  Display *_disp;
  Pixel _fg, _bg;
  Pixmap _pix;

  _disp = XtDisplay (label);

  Xmx_n = 0;
  XmxSetArg (XmNforeground, (XtArgVal)(&_fg));
  XmxSetArg (XmNbackground, (XtArgVal)(&_bg));
  XtGetValues (label, Xmx_wargs, Xmx_n);
  Xmx_n = 0;

  _pix = XCreatePixmapFromBitmapData
    (_disp, DefaultRootWindow (_disp), data, width, height, _fg, _bg,
     DefaultDepthOfScreen (DefaultScreenOfDisplay (_disp)));

  return _pix;
}

/* args used */
void XmxApplyPixmapToLabelWidget
  (Widget label, Pixmap pix)
{
  XmxSetArg (XmNlabelPixmap, (XtArgVal)pix);
  XmxSetArg (XmNlabelType, (XtArgVal)XmPIXMAP);
  XmxSetValues (label);

  Xmx_n = 0;
  return;
}

/* ------------------------ DIALOG CONTROL BUTTONS ------------------------ */

/* args apply to form */
Widget XmxMakeFormAndOneButton (Widget parent, XtCallbackProc cb, 
                                String name1, XtPointer cb_data1)
{
	Widget _form, _button1;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)3);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb, cb_data1);
	XmxSetConstraints(_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

Widget XmxMakeFormAndTwoButtons(Widget parent, String name1, String name2,
   XtCallbackProc cb0, XtCallbackProc cb1, XtPointer cb_data)
{
	Widget _form, _button1, _button2;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)2);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
	_button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
	XmxSetConstraints(_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button2, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 0, 1);
	XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
	XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

Widget XmxMakeFormAndTwoButtonsSqueezed (Widget parent,
   String name1, String name2,
   XtCallbackProc cb0, XtCallbackProc cb1, XtPointer cb_data)
{
  Widget _form, _button1, _button2;

  XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
  XmxSetArg (XmNfractionBase, (XtArgVal)5);
  _form = XmxMakeForm (parent);
  _button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
  _button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
  XmxSetConstraints (_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE, 
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
  XmxSetConstraints (_button2, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE, 
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
  XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 1, 2);
  XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 3, 4);
  XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
  XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 8);
  Xmx_n = 0;
  Xmx_w = _form;
  return Xmx_w;
}

/* args apply to form */
Widget XmxMakeFormAndThreeButtonsSqueezed (Widget parent,
   String name1, String name2, String name3,
   XtCallbackProc cb0, XtCallbackProc cb1, XtCallbackProc cb2,
   XtPointer cb_data)
{
	Widget _form, _button1, _button2, _button3;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)7);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
	_button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
	_button3 = XmxMakePushButton (_form, name3, cb2, cb_data);
	XmxSetConstraints (_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button2, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button3, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 3, 4);
	XmxSetPositions (_button3, XmxNoPosition, XmxNoPosition, 5, 6);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
	XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button3, XmxNoOffset, XmxNoOffset, 4, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

/* args apply to form */
Widget XmxMakeFormAndThreeButtons (Widget parent, 
   String name1, String name2, String name3,
   XtCallbackProc cb0, XtCallbackProc cb1, XtCallbackProc cb2,
	XtPointer cb_data)
{
	Widget _form, _button1, _button2, _button3;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)3);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
	_button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
	_button3 = XmxMakePushButton (_form, name3, cb2, cb_data);
	XmxSetConstraints(_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button2, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button3, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
			XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 0, 1);
	XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetPositions (_button3, XmxNoPosition, XmxNoPosition, 2, 3);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
	XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button3, XmxNoOffset, XmxNoOffset, 4, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

/* args apply to form */
Widget XmxMakeFormAndFourButtons 
  (Widget parent, String name1, String name2, String name3, String name4,
   XtCallbackProc cb0, XtCallbackProc cb1, XtCallbackProc cb2, XtCallbackProc cb3,
   XtPointer cb_data)
{
	Widget _form, _button1, _button2, _button3, _button4;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)4);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
	_button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
	_button3 = XmxMakePushButton (_form, name3, cb2, cb_data);
	_button4 = XmxMakePushButton (_form, name4, cb3, cb_data);

	XmxSetConstraints(_button1, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button2, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button3, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints(_button4, XmATTACH_FORM, XmATTACH_FORM, XmATTACH_NONE,
		XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 0, 1);
	XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetPositions (_button3, XmxNoPosition, XmxNoPosition, 2, 3);
	XmxSetPositions (_button4, XmxNoPosition, XmxNoPosition, 3, 4);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
	XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button3, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button4, XmxNoOffset, XmxNoOffset, 4, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

/* args apply to form */
Widget XmxMakeFormAndFiveButtons (Widget parent, 
	String name1, String name2, String name3, String name4, String name5,
	XtCallbackProc cb0, XtCallbackProc cb1, XtCallbackProc cb2, 
	XtCallbackProc cb3, XtCallbackProc cb4, XtPointer cb_data)
{
	Widget _form, _button1, _button2, _button3, _button4, _button5;

	XmxSetArg (XmNverticalSpacing, (XtArgVal)8);
	XmxSetArg (XmNfractionBase, (XtArgVal)5);
	_form = XmxMakeForm (parent);
	_button1 = XmxMakePushButton (_form, name1, cb0, cb_data);
	_button2 = XmxMakePushButton (_form, name2, cb1, cb_data);
	_button3 = XmxMakePushButton (_form, name3, cb2, cb_data);
	_button4 = XmxMakePushButton (_form, name4, cb3, cb_data);
	_button5 = XmxMakePushButton (_form, name5, cb4, cb_data);
	XmxSetConstraints (_button1, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_NONE, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button2, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_NONE, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button3, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_NONE, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button4, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_NONE, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetConstraints (_button5, XmATTACH_FORM, XmATTACH_FORM, 
		XmATTACH_NONE, XmATTACH_NONE, NULL, NULL, NULL, NULL);
	XmxSetPositions (_button1, XmxNoPosition, XmxNoPosition, 0, 1);
	XmxSetPositions (_button2, XmxNoPosition, XmxNoPosition, 1, 2);
	XmxSetPositions (_button3, XmxNoPosition, XmxNoPosition, 2, 3);
	XmxSetPositions (_button4, XmxNoPosition, XmxNoPosition, 3, 4);
	XmxSetPositions (_button5, XmxNoPosition, XmxNoPosition, 4, 5);
	XmxSetOffsets (_button1, XmxNoOffset, XmxNoOffset, 8, 4);
	XmxSetOffsets (_button2, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button3, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button4, XmxNoOffset, XmxNoOffset, 4, 4);
	XmxSetOffsets (_button5, XmxNoOffset, XmxNoOffset, 4, 8);
	Xmx_n = 0;
	Xmx_w = _form;
	return Xmx_w;
}

static void _XmxYesOrNoResponse (Widget w, int *answer, XmAnyCallbackStruct *cbs)
{
	if (cbs->reason == XmCR_OK)
		*answer = 1;
	else 
		if (cbs->reason == XmCR_CANCEL)
			*answer = 0;
}

int XmxModalYesOrNo (Widget parent, XtAppContext app, 
	char *questionstr, char *yesstr, char *nostr)
{
	Widget dialog;
	XmString question, yes, no, title;
	int answer = -1;

	question = XmStringCreateLtoR (questionstr, XmSTRING_DEFAULT_CHARSET);
	yes = XmStringCreateLtoR (yesstr, XmSTRING_DEFAULT_CHARSET);
	no = XmStringCreateLtoR (nostr, XmSTRING_DEFAULT_CHARSET);
	title = XmStringCreateLtoR ("Prompt", XmSTRING_DEFAULT_CHARSET);
  
	XmxSetArg (XmNdialogTitle, (XtArgVal)title);
	XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XmxSetArg (XmNmessageString, (XtArgVal)question);
	XmxSetArg (XmNokLabelString, (XtArgVal)yes);
	XmxSetArg (XmNcancelLabelString, (XtArgVal)no);
	dialog = XmCreateQuestionDialog(parent,"question_dialog",Xmx_wargs,Xmx_n);
	Xmx_n = 0;
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	XtAddCallback (dialog, XmNokCallback, 
			(XtCallbackProc)_XmxYesOrNoResponse, &answer);
	XtAddCallback (dialog, XmNcancelCallback, 
			(XtCallbackProc)_XmxYesOrNoResponse, &answer);
	XtManageChild (dialog);
	while (answer == -1) {
		XtAppProcessEvent (app, XtIMAll);
		XSync (XtDisplay (dialog), 0);
    	}
	XtUnmanageChild (dialog);
	XSync (XtDisplay (dialog), 0);
	XmUpdateDisplay (dialog);

	XmStringFree (question);
	XmStringFree (yes);
	XmStringFree (no);
	XmStringFree (title);
	XtDestroyWidget (dialog);
	return answer;
}

static char * XMX_OK_ANSWER = "OK";

/*SWP -- 7/6/95*/
static void _XmxActivate (Widget w, char **answer, 
	XmSelectionBoxCallbackStruct *cbs)
{
	*answer = XMX_OK_ANSWER;
}

static void _XmxPromptForStringResponse (Widget w, char **answer, 
	XmSelectionBoxCallbackStruct *cbs)
{
	if (!XmStringGetLtoR (cbs->value, XmSTRING_DEFAULT_CHARSET, answer))
		*answer = XMX_OK_ANSWER;
}

static void _XmxPromptForStringCancel (Widget w, char **answer, 
	XmSelectionBoxCallbackStruct *cbs)
{
	*answer = XMX_OK_ANSWER;
}

/*SWP -- 7/4/95*/
void XmxMakeInfoDialogWait (Widget parent, XtAppContext app, 
	char *infostr, char *titlestr, char *yesstr)
{
	Widget dialog;
	XmString info, yes, title;
	char *answer = NULL;

	info = XmStringCreateLtoR (infostr, XmSTRING_DEFAULT_CHARSET);
	yes = XmStringCreateLtoR (yesstr, XmSTRING_DEFAULT_CHARSET);
	title = XmStringCreateLtoR (titlestr, XmSTRING_DEFAULT_CHARSET);
  
	XmxSetArg (XmNdialogTitle, (XtArgVal)title);
	XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XmxSetArg (XmNmessageString, (XtArgVal)info);
	XmxSetArg (XmNokLabelString, (XtArgVal)yes);

	dialog = XmCreateInformationDialog(parent, "information_dialog", 
			Xmx_wargs, Xmx_n);
	Xmx_n = 0;
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
	XtAddCallback (dialog, XmNokCallback, 
			(XtCallbackProc)_XmxActivate, &answer);
	XtManageChild (dialog);
	while (answer == NULL) {
		XtAppProcessEvent (app, XtIMAll);
		XSync (XtDisplay (dialog), 0);
	}

	XtUnmanageChild (dialog);
	XSync (XtDisplay (dialog), 0);
	XmUpdateDisplay (dialog);
	XmStringFree (info);
	XmStringFree (yes);
	XmStringFree (title);
	XtDestroyWidget (dialog);
}

/*GD 24 Oct 97 */
static void ErrorDialogDestroyCB(Widget w, XtPointer clid, XtPointer calld)
{
	XtUnmanageChild (w);         
	XtDestroyWidget (w);         
}

/*GD 24 Oct 97 */
/* An error dialog. non blocking version */
void XmxMakeErrorDialog(Widget parent, char *istr, char *tstr)
{     
	Widget dialog;
	XmString info, title;
      
	info = XmStringCreateLtoR (istr, XmSTRING_DEFAULT_CHARSET);
	title = XmStringCreateLtoR (tstr, XmSTRING_DEFAULT_CHARSET);
      
	XmxSetArg (XmNdialogTitle, (XtArgVal)title);
	XmxSetArg (XmNdialogStyle, XmDIALOG_MODELESS);
	XmxSetArg (XmNmessageString, (XtArgVal)info);
	XmxSetArg (XmNmessageAlignment, (XtArgVal)XmALIGNMENT_CENTER);

	dialog = XmCreateErrorDialog(parent, "error_dialog", Xmx_wargs, Xmx_n);
	Xmx_n = 0;

	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
	XmStringFree (info);              
	XmStringFree (title);             

	XtAddCallback(dialog,XmNokCallback, ErrorDialogDestroyCB, NULL);
	XtManageChild (dialog);           
}

 /*SWP -- 4/15/96*/
void XmxMakeErrorDialogWait (Widget parent, XtAppContext app,
	char *infostr, char *titlestr)
{     
	Widget dialog;
	XmString info, title;
	char *answer = NULL;
      
	info = XmStringCreateLtoR (infostr, XmSTRING_DEFAULT_CHARSET);
	title = XmStringCreateLtoR (titlestr, XmSTRING_DEFAULT_CHARSET);
      
	XmxSetArg (XmNdialogTitle, (XtArgVal)title);
	XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
	XmxSetArg (XmNmessageString, (XtArgVal)info);
	XmxSetArg (XmNmessageAlignment, (XtArgVal)XmALIGNMENT_CENTER);

	dialog = XmCreateErrorDialog(parent, "error_dialog", Xmx_wargs, Xmx_n);                              
	Xmx_n = 0;

	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
	XtUnmanageChild (XmMessageBoxGetChild (dialog, XmDIALOG_CANCEL_BUTTON));
	XmStringFree (info);              
	XmStringFree (title);             

	XtAddCallback(dialog,XmNokCallback,(XtCallbackProc)_XmxActivate, &answer);
	XtManageChild (dialog);           
	while (answer == NULL) {                               
		XtAppProcessEvent (app, XtIMAll);  
		XSync (XtDisplay (dialog), 0);
	}                               
	XtUnmanageChild (dialog);         
	XSync (XtDisplay (dialog), 0);    
	XmUpdateDisplay (dialog);         
	XtDestroyWidget (dialog);         
}

char *XmxModalPromptForString (Widget parent, XtAppContext app, 
	char *questionstr, char *yesstr, char *nostr)
{
  Widget dialog;
  XmString question, yes, no, title;
  char *answer = NULL;

  question = XmStringCreateLtoR (questionstr, XmSTRING_DEFAULT_CHARSET);
  yes = XmStringCreateLtoR (yesstr, XmSTRING_DEFAULT_CHARSET);
  no = XmStringCreateLtoR (nostr, XmSTRING_DEFAULT_CHARSET);
  title = XmStringCreateLtoR ("Prompt", XmSTRING_DEFAULT_CHARSET);
  
  XmxSetArg (XmNdialogTitle, (XtArgVal)title);
  XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
  XmxSetArg (XmNselectionLabelString, (XtArgVal)question);
  XmxSetArg (XmNokLabelString, (XtArgVal)yes);
  XmxSetArg (XmNcancelLabelString, (XtArgVal)no);
  dialog = XmCreatePromptDialog (parent, "question_dialog", Xmx_wargs, Xmx_n);
  Xmx_n = 0;
  
  XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback (dialog, XmNokCallback, 
                 (XtCallbackProc)_XmxPromptForStringResponse, &answer);
  XtAddCallback (dialog, XmNcancelCallback, 
                 (XtCallbackProc)_XmxPromptForStringCancel, &answer);

  XtManageChild (dialog);

  while (answer == NULL) {
      XtAppProcessEvent (app, XtIMAll);
      XSync (XtDisplay (dialog), 0);
    }

  XtUnmanageChild (dialog);
  XSync (XtDisplay (dialog), 0);
  XmUpdateDisplay (dialog);

  XmStringFree (question);
  XmStringFree (yes);
  XmStringFree (no);
  XmStringFree (title);

  XtDestroyWidget (dialog);

  if (!answer || strcmp (answer, XMX_OK_ANSWER) == 0)
    return NULL;
  else
    return answer;
}


static char *_passwd = NULL;

static void _XmxPromptForPasswordResponse (Widget w, char **answer, 
                                           XmSelectionBoxCallbackStruct *cbs)
{
  if (!XmStringGetLtoR (cbs->value, XmSTRING_DEFAULT_CHARSET, answer))
    *answer = XMX_OK_ANSWER;
}

static void _XmxPromptForPasswordCancel (Widget w, char **answer, 
                                         XmSelectionBoxCallbackStruct *cbs)
{
  *answer = XMX_OK_ANSWER;
}

static void _XmxPromptForPasswordVerify (Widget text_w, XtPointer unused, 
                                         XmTextVerifyCallbackStruct *cbs)
{
	char *nw;
	int len;
  
	if (cbs->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;
	if (cbs->text->ptr == NULL) { /* backspace */
		cbs->doit = True;
		if (_passwd && *_passwd) {
			int start;
			char *tptr;
	
			len = strlen(_passwd);
					/* Find the start of the delete */
			if (cbs->startPos < len) {
				start = cbs->startPos;
			} else {
				start = len - 1;
			}
					/* Move up stuff after the delete */
			if (cbs->endPos > len) {
				tptr = &(_passwd[len]);
			} else {
				tptr = &(_passwd[cbs->endPos]);
			}
			_passwd[start] ='\0';
			strcat(_passwd, tptr);
		}
	} else if (cbs->text->length >= 1) {
		int i;

		if (_passwd == NULL) {
			_passwd = XtMalloc (cbs->text->length + 1);
			strncpy(_passwd, cbs->text->ptr, cbs->text->length);
			_passwd[cbs->text->length] = '\0';
		} else {
			char *tptr;
			char tchar;
			int start;

			len = strlen(_passwd);
				/* Find the start of the delete */
			if (cbs->startPos < len) {
				start = cbs->startPos;
			} else {
				start = len;
			}
			tptr = &(_passwd[start]);
			tchar = *tptr;
			*tptr = '\0';
			nw = XtMalloc (len + cbs->text->length + 1);
			strcpy(nw, _passwd);
			strncat(nw, cbs->text->ptr, cbs->text->length);
			nw[start + cbs->text->length] = '\0';
			*tptr = tchar;
			strcat(nw, tptr);
			XtFree(_passwd);
			_passwd = nw;
		}
		cbs->doit = True;
			/*  make a '*' show up instead of what they typed */
		for (i=0; i < cbs->text->length; i++) {
			cbs->text->ptr[i] = '*';
		}
	}
}

char *XmxModalPromptForPassword (Widget parent, XtAppContext app, 
                               char *questionstr, char *yesstr, char *nostr)
{
  Widget dialog;
  XmString question, yes, no, title;
  char *answer = NULL;

  _passwd = NULL;
  question = XmStringCreateLtoR (questionstr, XmSTRING_DEFAULT_CHARSET);
  yes = XmStringCreateLtoR (yesstr, XmSTRING_DEFAULT_CHARSET);
  no = XmStringCreateLtoR (nostr, XmSTRING_DEFAULT_CHARSET);
  title = XmStringCreateLtoR ("Prompt", XmSTRING_DEFAULT_CHARSET);
  
  XmxSetArg (XmNdialogTitle, (XtArgVal)title);
  XmxSetArg (XmNdialogStyle, XmDIALOG_FULL_APPLICATION_MODAL);
  XmxSetArg (XmNselectionLabelString, (XtArgVal)question);
  XmxSetArg (XmNokLabelString, (XtArgVal)yes);
  XmxSetArg (XmNcancelLabelString, (XtArgVal)no);
  dialog = XmCreatePromptDialog (parent, "question_dialog", Xmx_wargs, Xmx_n);
  Xmx_n = 0;
  
  XtUnmanageChild (XmSelectionBoxGetChild (dialog, XmDIALOG_HELP_BUTTON));
  XtAddCallback
    (XmSelectionBoxGetChild (dialog, XmDIALOG_TEXT),
     XmNmodifyVerifyCallback, (XtCallbackProc)_XmxPromptForPasswordVerify, 0);
  XtAddCallback (dialog, XmNokCallback, 
                 (XtCallbackProc)_XmxPromptForPasswordResponse, &answer);
  XtAddCallback (dialog, XmNcancelCallback, 
                 (XtCallbackProc)_XmxPromptForPasswordCancel, &answer);

  XtManageChild (dialog);

  while (answer == NULL) {
      XtAppProcessEvent (app, XtIMAll);
      XSync (XtDisplay (dialog), 0);
    }

  XtUnmanageChild (dialog);
  XSync (XtDisplay (dialog), 0);
  XmUpdateDisplay (dialog);

  XmStringFree (question);
  XmStringFree (yes);
  XmStringFree (no);
  XmStringFree (title);

  XtDestroyWidget (dialog);

  if (!answer || strcmp (answer, XMX_OK_ANSWER) == 0 || !_passwd || !(*_passwd))
    return NULL;
  else
    return strdup (_passwd);
}


/* --------------------------- PRIVATE ROUTINES --------------------------- */

/* Create a new MenuEntry and add it to the head of a MenuRecord list. */
static void _XmxMenuAddEntryToRecord (XmxMenuRecord *rec, Widget w, 
	XtPointer token)
{
	XmxMenuEntry *_ent;

	_ent = (XmxMenuEntry *)malloc (sizeof (XmxMenuEntry));
	_ent->w = w;
	_ent->token = token;
	_ent->next = rec->first_entry; /*Add rest of list to tail of this entry.*/
	rec->first_entry = _ent;	/* Make this entry head of list. */
}


/* Given token, fetch the corresponding entry. */
static XmxMenuEntry * _XmxMenuGetEntryFromRecord (XmxMenuRecord *rec, 
	XtPointer token)
{
	XmxMenuEntry *_ent = NULL;
	int _done;

			/* Search the linked list. */
	_ent = rec->first_entry;
	_done = 0;
	while (_ent != NULL && !_done) {
		if (_ent->token == token)
			_done = 1;
		else
			_ent = _ent->next;
	}
	/* Punish the application for asking for a nonexistent entry. */
	/* assert (_done); */
	return _ent;
}

void _XmxRDestroyMenubar(XmxMenuRecord * rec)
{
	XmxMenuEntry *_ent = NULL;
	XmxMenuEntry *_next = NULL;

	_ent = rec->first_entry;
	while (_ent != NULL){
		_next = _ent->next;
		free(_ent);
		_ent = _next;
	}
}

/* ------------------------- _XmxMenuCreateRecord ------------------------- */

/* Create a new MenuRecord and clear out its list. */
XmxMenuRecord * _XmxMenuCreateRecord (Widget base)
{
  XmxMenuRecord *_rec;

  /* Create the new XmxMenuRecord. */
  _rec = (XmxMenuRecord *)malloc (sizeof (XmxMenuRecord));
  _rec->base = base;
  _rec->first_entry = NULL;
  return _rec;
}

/* --------------------------- PUBLIC ROUTINES ---------------------------- */

void XmxRSetSensitive (XmxMenuRecord *rec, XtPointer token, int state)
{
	XmxMenuEntry *_entry;

	assert (state == XmxSensitive || state == XmxUnsensitive);
	_entry = _XmxMenuGetEntryFromRecord (rec, token);
	/* XtSetSensitive propagates down Widget hierarchy. */
	if (_entry)
		XtSetSensitive(_entry->w, (state == XmxSensitive) ? True : False);
}

void XmxRSetToggleState (XmxMenuRecord *rec, XtPointer token, int state)
{
	XmxMenuEntry *_entry;

	assert (state == XmxSet || state == XmxUnset);
	_entry = _XmxMenuGetEntryFromRecord (rec, token);
	if (_entry)
		XmToggleButtonGadgetSetState(_entry->w, 
			(state == XmxSet) ? True : False, False);
}

/* args apply to pulldown menu */
XmxMenuRecord * XmxRMakeOptionMenu(Widget parent, String name, 
	XmxOptionMenuStruct *opts)
{
	XmxMenuRecord *_rec;
	Widget _pulldown, _button, _menuhist = (Widget)NULL;
	int _i;

/* Create a pulldown menupane to attach to the option menu;
 * preloaded wargs affect this. */
	_pulldown = XmCreatePulldownMenu(parent,"pulldownmenu",Xmx_wargs, Xmx_n);

/* menuHistory will not be applied to _pulldown, so we'll modify
 * _rec directly after creating the option menu. */
	_rec = _XmxMenuCreateRecord (_pulldown);

/* Create pushbutton gadgets as childen of the pulldown menu. */
	_i = 0;
	while (opts[_i].namestr) {
		Xmx_n = 0;
		XmxSetArg (XmNlabelString,
			(XtArgVal)XmStringCreateLtoR (opts[_i].namestr,
			XmSTRING_DEFAULT_CHARSET));
		_button = XmCreatePushButtonGadget (_pulldown, "pushbutton",
			Xmx_wargs, Xmx_n);
		XtManageChild (_button);
		XtAddCallback (_button, XmNactivateCallback, opts[_i].data ,
			(XtPointer)opts[_i].win);
		if (opts[_i].set_state == XmxSet)
			_menuhist = _button;
		_XmxMenuAddEntryToRecord (_rec, _button, (XtPointer)opts[_i].data);
		_i++;
	}
/* Create the option menu itself; tie in the pulldown menu. */
	Xmx_n = 0;
	XmxSetArg (XmNsubMenuId, (XtArgVal)_pulldown);
	if (_menuhist != (Widget)NULL)
		XmxSetArg (XmNmenuHistory, (XtArgVal)_menuhist);
	Xmx_w = XmCreateOptionMenu (parent, "optionmenu", Xmx_wargs, Xmx_n);
	XtManageChild (Xmx_w);

	XmxSetArg (XmNalignment, (XtArgVal)XmALIGNMENT_BEGINNING);
	XmxSetValues (XmOptionButtonGadget (Xmx_w));
	if (name) {
		XmxSetArg (XmNlabelString,
			(XtArgVal)XmStringCreateLtoR(name, 
					XmSTRING_DEFAULT_CHARSET));
		XmxSetValues (XmOptionLabelGadget (Xmx_w));
	} else {
		XmxSetArg (XmNspacing, (XtArgVal)0);
		XmxSetArg (XmNmarginWidth, (XtArgVal)0);
		XmxSetValues (Xmx_w);
		XmxSetArg (XmNlabelString, (XtArgVal)NULL);
		XmxSetValues (XmOptionLabelGadget (Xmx_w));
	}
	_rec->base = Xmx_w;	/* Explicitly set base Widget of record. */
	Xmx_n = 0;
	return _rec;
}

/* Possible deficiency: will not be able to grey out a submenu (cascade button).*/
void _XmxRCreateMenubar (Widget menu, XmxMenubarStruct *menulist, 
                    XmxMenuRecord *rec, struct _mo_window * win)
{
	int _i;
	Widget *_buttons;
	int _separators = 0, _nitems;
	Widget _sub_menu;
	XmString xmstr;

	_nitems = 0;
	while (menulist[_nitems].namestr)
		_nitems++;
	_buttons = (Widget *)XtMalloc (_nitems * sizeof (Widget));

	for (_i = 0; _i < _nitems; _i++) {
		if (strcmp(menulist[_i].namestr, "----") == 0) {
/* Name of "----" means make a separator. */
			XtCreateManagedWidget("separator",
				xmSeparatorGadgetClass, menu, NULL, 0);
			_separators++;
			continue;
		}
		if (menulist[_i].func) {
/* A function means it's an ordinary entry with callback. */
			Xmx_n = 0;
			if (menulist[_i].mnemonic)
				XmxSetArg(XmNmnemonic, 
					(XtArgVal)(menulist[_i].mnemonic));
			menulist[_i].data =  (XtPointer)win;
			if (menulist[_i].namestr[0] == '#' ||
			    menulist[_i].namestr[0] == '<') {
					/* option/toggle button */
              				/* A toggle button is diamond-shaped. */
				if (menulist[_i].namestr[0] == '<')
					XmxSetArg (XmNindicatorType, 
						(XtArgVal)XmONE_OF_MANY);
/* Make sure the button shows up even when toggled off. */
				if (menulist[_i].namestr[0] == '#')
					XmxSetArg(XmNvisibleWhenOff, 
						(XtArgVal)True);
              /* Ignore first character of label. */
				xmstr = XmxMakeXmstrFromString(
						&(menulist[_i].namestr[1]));
				XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
				_buttons[_i - _separators] = XtCreateManagedWidget
					("togglebutton",xmToggleButtonGadgetClass,
					menu, Xmx_wargs, Xmx_n);
				XmStringFree (xmstr);
				XtAddCallback(_buttons[_i - _separators], 
					XmNvalueChangedCallback,menulist[_i].func,
					(XtPointer) menulist[_i].data);
			} else if (menulist[_i].namestr[0] == '+') { /* cascade for a pulldown menu*/
				/* the pulldown is fulled later by the callback func */
				_sub_menu = XmCreatePulldownMenu(menu, "pulldownmenu",
                                        NULL, 0);
				Xmx_n = 0;
				XmxSetArg (XmNsubMenuId, (XtArgVal)_sub_menu);
				if (menulist[_i].mnemonic)
					XmxSetArg(XmNmnemonic, (XtArgVal)(menulist[_i].mnemonic));
				xmstr = XmStringCreateLtoR (&(menulist[_i].namestr[1]),
						XmSTRING_DEFAULT_CHARSET);
				XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
				_buttons[_i - _separators] = XtCreateWidget(
				      "cascadebutton", xmCascadeButtonGadgetClass,
					menu, Xmx_wargs, Xmx_n);
				XmStringFree (xmstr);
				XtAddCallback(_buttons[_i - _separators],
					XmNcascadingCallback,menulist[_i].func,
					(XtPointer)menulist[_i].data);
				continue;
			} else { /* regular button */
				XmString xmstr = XmStringCreateLtoR
					(menulist[_i].namestr, 
					XmSTRING_DEFAULT_CHARSET);
				XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
				_buttons[_i - _separators] = XtCreateManagedWidget
					("pushbutton", xmPushButtonGadgetClass,
					menu, Xmx_wargs, Xmx_n);
				XmStringFree (xmstr);
				XtAddCallback(_buttons[_i - _separators], 
					XmNactivateCallback, menulist[_i].func, 
					(XtPointer)menulist[_i].data);
			}
/* Add thie button to the menu record. */
			_XmxMenuAddEntryToRecord(rec, _buttons[_i - _separators], 
					(XtPointer)menulist[_i].func);
			continue;
		}
		if (menulist[_i].sub_menu == (XmxMenubarStruct *)NULL) {
/* No function and no submenu entry means it's just a label. */
			Xmx_n = 0;
			XmxSetArg (XmNlabelString, (XtArgVal)XmStringCreateLtoR
				(menulist[_i].namestr, XmSTRING_DEFAULT_CHARSET));
			_buttons[_i - _separators] = XtCreateManagedWidget
				("label", xmLabelGadgetClass, menu, 
				Xmx_wargs, Xmx_n);
			continue;
		}
/* If all if fails, it's a submenu. */
		_sub_menu = XmCreatePulldownMenu(menu, "pulldownmenu", 
					NULL, 0);
		Xmx_n = 0;
		XmxSetArg (XmNsubMenuId, (XtArgVal)_sub_menu);
		if (menulist[_i].mnemonic)
			XmxSetArg(XmNmnemonic, (XtArgVal)(menulist[_i].mnemonic));
		xmstr = XmStringCreateLtoR
				(menulist[_i].namestr, XmSTRING_DEFAULT_CHARSET);
		XmxSetArg (XmNlabelString, (XtArgVal)xmstr);
		_buttons[_i - _separators] = XtCreateWidget("cascadebutton", 
				xmCascadeButtonGadgetClass,
				menu, Xmx_wargs, Xmx_n);
		XmStringFree (xmstr);
				/* If name is "Help", put on far right. */
		if (strcmp (menulist[_i].namestr, "Help") == 0) {
			Xmx_n = 0;
			XmxSetArg (XmNmenuHelpWidget, 
					(XtArgVal)_buttons[_i - _separators]);
			XtSetValues (menu, Xmx_wargs, Xmx_n);
		}
/* Recursively create new submenu. */
		_XmxRCreateMenubar(_sub_menu, menulist[_i].sub_menu, rec,win);
	}
	XtManageChildren (_buttons, _nitems - _separators);
	XtFree ((char *)_buttons);
}
