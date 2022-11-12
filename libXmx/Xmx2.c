/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#include <stdlib.h>
#include "XmxP.h"

/* --------------------------- PRIVATE ROUTINES --------------------------- */

/* Create a new MenuEntry and add it to the head of a MenuRecord list. */
private void _XmxMenuAddEntryToRecord (XmxMenuRecord *rec, Widget w, 
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
private XmxMenuEntry * _XmxMenuGetEntryFromRecord (XmxMenuRecord *rec, 
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

public void XmxRSetSensitive (XmxMenuRecord *rec, XtPointer token, int state)
{
	XmxMenuEntry *_entry;

	assert (state == XmxSensitive || state == XmxUnsensitive);
	_entry = _XmxMenuGetEntryFromRecord (rec, token);
	/* XtSetSensitive propagates down Widget hierarchy. */
	if (_entry)
		XtSetSensitive(_entry->w, (state == XmxSensitive) ? True : False);
}

public void XmxRSetToggleState (XmxMenuRecord *rec, XtPointer token, int state)
{
	XmxMenuEntry *_entry;

	assert (state == XmxSet || state == XmxUnset);
	_entry = _XmxMenuGetEntryFromRecord (rec, token);
	if (_entry)
		XmToggleButtonGadgetSetState(_entry->w, 
			(state == XmxSet) ? True : False, False);
}

public void XmxRUnsetAllToggles (XmxMenuRecord *rec)
{
	XmxMenuEntry *_ent;

	for (_ent = rec->first_entry; _ent != NULL; _ent = _ent->next)
		XmToggleButtonGadgetSetState (_ent->w, False, False);
}

public void XmxRSetOptionMenuHistory (XmxMenuRecord *rec, XtPointer token)
{
	XmxMenuEntry *_entry;

	_entry = _XmxMenuGetEntryFromRecord (rec, token);
	if (_entry) {
		XmxSetArg (XmNmenuHistory, (XtArgVal)(_entry->w));
		XtSetValues (rec->base, Xmx_wargs, Xmx_n);
	}
	Xmx_n = 0;
}

/* args apply to pulldown menu */
public XmxMenuRecord * XmxRMakeOptionMenu(Widget parent, String name, 
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

/* -------------------------- XmxRMakeToggleMenu -------------------------- */

/* args apply to radiobox or optionbox */
public XmxMenuRecord *
XmxRMakeToggleMenu (Widget parent, int behavior, XtCallbackProc cb,
                    XmxToggleMenuStruct *opts)
{
	XmxMenuRecord *_rec;
	Widget _box;
	int _i;

	assert (behavior == XmxOneOfMany || behavior == XmxNOfMany);
	switch (behavior) {
	case XmxOneOfMany:
		_box = XmxMakeRadioBox (parent);
		break;
	case XmxNOfMany:
		_box = XmxMakeOptionBox (parent);
		break;
	}
	_rec = _XmxMenuCreateRecord (_box);
	_i = 0;
	while (opts[_i].namestr) {
		XmxMakeToggleButton (_box, opts[_i].namestr, cb, (XtPointer)opts[_i].win);
		XmxSetToggleButton (Xmx_w, opts[_i].set_state);
		_XmxMenuAddEntryToRecord (_rec, Xmx_w, (XtPointer)opts[_i].data);
		_i++;
	}
	Xmx_w = _box;
	Xmx_n = 0;
	return _rec;
}

/* Possible deficiency: will not be able to grey out a submenu (cascade button).*/
void _XmxRCreateMenubar (Widget menu, XmxMenubarStruct *menulist, 
                    XmxMenuRecord *rec, struct mo_window * win)
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
			menulist[_i].data = (char*) win;
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
