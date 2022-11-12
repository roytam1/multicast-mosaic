/* Please read copyright.tmpl. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include <Xm/XmAll.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#define STRING XmString

#define X_NAME		"x"
#define Y_NAME		"y"

#define	W_TEXTFIELD	0
#define	W_CHECKBOX	1
#define	W_RADIOBOX	2
#define	W_PUSHBUTTON	3
#define	W_PASSWORD	4
#define	W_OPTIONMENU	5
#define	W_TEXTAREA	6
#define	W_LIST		7
#define	W_HIDDEN	9

extern char *ParseMarkTag();

char ** 	ParseCommaList( char *str, int *count);
void 		FreeCommaList( char **list, int cnt);
char * 		MapOptionReturn( char *val, char **mapping);


void AddNewForm( HTMLWidget hw, FormInfo *fptr)
{
	FormInfo *ptr;

	ptr = hw->html.form_list;
	if (ptr == NULL) {
		hw->html.form_list = fptr;
		fptr->next = NULL;
	} else {
		while (ptr->next != NULL)
			ptr = ptr->next;
		ptr->next = fptr;
		fptr->next = NULL;
	}
}

int CollectSubmitInfo( FormInfo *fptr, char ***name_list, char ***value_list)
{
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WbFormCallbackData cbdata;
	WidgetInfo *wptr;
	int cnt;

	if (fptr->end == -1)  /* unterminated FORM tag */ {
		wptr = hw->html.widget_list;
		cnt = 0;
		while (wptr != NULL) {
			cnt++;
			wptr = wptr->next;
		}
		cbdata.attribute_count = cnt;
	} else {
		cbdata.attribute_count = fptr->end - fptr->start;
	}
	cbdata.attribute_names = (char **)malloc(cbdata.attribute_count *
		sizeof(char *));
	cbdata.attribute_values = (char **)malloc(cbdata.attribute_count *
		sizeof(char *));

	if (fptr->start == 0) {
		wptr = hw->html.widget_list;
	} else {
		wptr = hw->html.widget_list;
		while (wptr != NULL) {
			if (wptr->id == fptr->start) {
				wptr = wptr->next;
				break;
			}
			wptr = wptr->next;
		}
	}
	cnt = 0;
	while ((wptr != NULL)&&(cnt < cbdata.attribute_count)) {
	/***   cvarela@ncsa.uiuc.edu:  August 17, 1994
               Adding multiple submit buttons support
         ***   changed to match widgets -- amb ***/
           if (wptr->name) {
		Widget child;
		STRING *str_list;
		int list_cnt;
		char *val;
		STRING label;
		Cardinal argcnt;
		Arg arg[5];

		cbdata.attribute_names[cnt] = wptr->name;
		switch(wptr->type) {
		case W_TEXTFIELD:
			cbdata.attribute_values[cnt] =
				XmTextFieldGetString(wptr->w);
			if ((cbdata.attribute_values[cnt] != NULL)&&
			    (cbdata.attribute_values[cnt][0] == '\0')) {
				cbdata.attribute_values[cnt] = NULL;
			}
			break;
		case W_TEXTAREA:
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNworkWindow, &child);
			argcnt++;
			XtGetValues(wptr->w, arg, argcnt);
			cbdata.attribute_values[cnt] = XmTextGetString(child);
			if ((cbdata.attribute_values[cnt] != NULL)&&
			    (cbdata.attribute_values[cnt][0] == '\0')) {
					cbdata.attribute_values[cnt] = NULL;
			}
			break;
		case W_PASSWORD:
			cbdata.attribute_values[cnt] = wptr->password;
			if ((cbdata.attribute_values[cnt] != NULL)&&
			    (cbdata.attribute_values[cnt][0] == '\0')) {
				cbdata.attribute_values[cnt] = NULL;
			}
			break;
		case W_LIST:
			/*
			 * First get the Widget ID of the proper
			 * list element
			 */
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNworkWindow, &child);
			argcnt++;
			XtGetValues(wptr->w, arg, argcnt);

			/*
			 * Now get the list of selected items.
			 */
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNselectedItemCount, &list_cnt);
			argcnt++;
			XtSetArg(arg[argcnt], XmNselectedItems, &str_list);
			argcnt++;
			XtGetValues(child, arg, argcnt);

			if (list_cnt == 0) {
				cnt--;
				cbdata.attribute_count--;
			} else /* list_cnt >= 1 */ {
				int j, new_cnt;
				char **names;
				char **values;

				if (list_cnt > 1) {
				    new_cnt = cbdata.attribute_count +list_cnt -1;
				    names =(char **)malloc(new_cnt*sizeof(char*));
				    values=(char **)malloc(new_cnt*sizeof(char*));
				    for (j=0; j<cnt; j++) {
					names[j] = cbdata.attribute_names[j];
					values[j] = cbdata.attribute_values[j];
				    }
				    free((char *) cbdata.attribute_names);
				    free((char *) cbdata.attribute_values);
				    cbdata.attribute_names = names;
				    cbdata.attribute_values = values;
				    cbdata.attribute_count = new_cnt;
				}

				for (j=0; j<list_cnt; j++) {
					cbdata.attribute_names[cnt + j]
						= wptr->name;
					XmStringGetLtoR(str_list[j],
					    XmSTRING_DEFAULT_CHARSET, &val);
					if ((val != NULL)&& (val[0] == '\0')) {
						val = NULL;
					} else if (val != NULL) {
						val = MapOptionReturn( val,
							wptr->mapping);
					}
					cbdata.attribute_values[cnt + j] = val;
				}
				cnt = cnt + list_cnt - 1;
			}
			break;
		/*
		 * For an option menu, first get the label gadget
		 * which holds the current value.
		 * Now get the text from that label as a character
		 * string.
		 */
		case W_OPTIONMENU:
			child = XmOptionButtonGadget(wptr->w);
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNlabelString, &label);
			argcnt++;
			XtGetValues(child, arg, argcnt);
			val = NULL;
			XmStringGetLtoR(label, XmSTRING_DEFAULT_CHARSET, &val);
			if ((val != NULL)&&(val[0] == '\0')) {
				val = NULL;
			} else if (val != NULL) {
				val = MapOptionReturn(val, wptr->mapping);
			}
			cbdata.attribute_values[cnt] = val;
			if ((cbdata.attribute_values[cnt] != NULL)&&
			    (cbdata.attribute_values[cnt][0] == '\0')) {
				cbdata.attribute_values[cnt] = NULL;
			}
			break;
		case W_CHECKBOX:
		case W_RADIOBOX:
			if (XmToggleButtonGetState(wptr->w) == True) {
			    cbdata.attribute_values[cnt] = wptr->value;
			} else {
			    cnt--;
			    cbdata.attribute_count--;
			}
			break;

	       /*** cvarela@ncsa.uiuc.edu:  August 17, 1994
	            Adding multiple submit buttons support ***/
			/* mods 3/11/95  -- amb */
                      case W_PUSHBUTTON:
                              if (fptr->button_pressed == wptr->w){
                                  cbdata.attribute_values[cnt] = wptr->value;
                              } else {
                                  cnt--;
                                  cbdata.attribute_count--;
                              }
                              break;
		/**/

		case W_HIDDEN:
			cbdata.attribute_values[cnt] = wptr->value;
			break;
		default:
			cbdata.attribute_values[cnt] = NULL;
			break;
		}
		cnt++;
	    } else {
		cbdata.attribute_count--;
	    }
	    wptr = wptr->next;
	}
	cbdata.attribute_count = cnt;

	*name_list = cbdata.attribute_names;
	*value_list = cbdata.attribute_values;
	return(cbdata.attribute_count);
}

void ImageSubmitForm( FormInfo *fptr, XEvent *event, int x, int y)
{
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WbFormCallbackData cbdata;
	int i, cnt;
	char **name_list;
	char **value_list;
	char valstr[100];

	cbdata.event = event;
	cbdata.href = fptr->action;
        cbdata.method = fptr->method;
        cbdata.enctype = fptr->enctype;

	name_list = NULL;
	value_list = NULL;
	cnt = CollectSubmitInfo(fptr, &name_list, &value_list);

	cbdata.attribute_count = cnt + 2;
	cbdata.attribute_names = (char **)malloc(cbdata.attribute_count *
		sizeof(char*));
	cbdata.attribute_values = (char **)malloc(cbdata.attribute_count *
		sizeof(char *));
	for (i=0; i<cnt; i++) {
		cbdata.attribute_names[i] = name_list[i];
		cbdata.attribute_values[i] = value_list[i];
	}
	if (name_list != NULL)
		free((char *)name_list);
	if (value_list != NULL)
		free((char *)value_list);

	cbdata.attribute_names[cnt] = (char *)malloc(strlen(X_NAME) +1);
	strcpy(cbdata.attribute_names[cnt], X_NAME);
	sprintf(valstr, "%d", x);
	cbdata.attribute_values[cnt] = (char *)malloc(strlen(valstr) + 1);
	strcpy(cbdata.attribute_values[cnt], valstr);

	cnt++;
	cbdata.attribute_names[cnt] = (char *)malloc(strlen(Y_NAME) +1);
	strcpy(cbdata.attribute_names[cnt], Y_NAME);
	sprintf(valstr, "%d", y);
	cbdata.attribute_values[cnt] = (char *)malloc(strlen(valstr) + 1);
	strcpy(cbdata.attribute_values[cnt], valstr);

	XtCallCallbackList ((Widget)hw, hw->html.form_callback,
		(XtPointer)&cbdata);
}

void CBSubmitForm( Widget w, caddr_t client_data, caddr_t call_data)
{
	FormInfo *fptr = (FormInfo *)client_data;
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WbFormCallbackData cbdata;
	XmPushButtonCallbackStruct *pb =
		(XmPushButtonCallbackStruct *)call_data;

	cbdata.event = pb->event;
	cbdata.href = fptr->action;
        cbdata.method = fptr->method;
        cbdata.enctype = fptr->enctype;
	fptr->button_pressed = w;

	cbdata.attribute_count = CollectSubmitInfo(fptr,
		&cbdata.attribute_names, &cbdata.attribute_values);

	XtCallCallbackList ((Widget)hw, hw->html.form_callback,
		(XtPointer)&cbdata);
}

/*
 * A radio buttom was toggled on in a form.
 * If there are other radios of the same name, turn them off.
 */
void CBChangeRadio( Widget w, caddr_t client_data, caddr_t call_data)
{
	FormInfo *fptr = (FormInfo *)client_data;
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WidgetInfo *wptr;
	WidgetInfo *wtmp;
	char *name;
	int cnt, count;
	XmToggleButtonCallbackStruct *tb =
		(XmToggleButtonCallbackStruct *)call_data;

	/*
	 * Bad button
	 */
	if (tb == NULL)
		return;

	/*
	 * Only do stuff when the button is turned on.
	 * Don't let the button be turned off, by clicking on
	 * it, as that would leave all buttons off.
	 */
	if ((tb == NULL)||(tb->set == False)) {
		XmToggleButtonSetState(w, True, False);
		return;
	}

	/*
	 * Terminate the form if it was never properly terminated.
	 */
	if (fptr->end == -1)  /* unterminated FORM tag */
	{
		wptr = hw->html.widget_list;
		cnt = 0;
		while (wptr != NULL) {
			cnt++;
			wptr = wptr->next;
		}
		count = cnt;
	} else {
		count = fptr->end - fptr->start;
	}

	/*
	 * Locate the start of the form.
	 */
	if (fptr->start == 0) {
		wptr = hw->html.widget_list;
	} else {
		wptr = hw->html.widget_list;
		while (wptr != NULL) {
			if (wptr->id == fptr->start) {
				wptr = wptr->next;
				break;
			}
			wptr = wptr->next;
		}
	}

	/*
	 * Find the name of the toggle button just pressed.
	 */
	name = NULL;
	wtmp = wptr;
	while (wtmp != NULL) {
		if (wtmp->w == w) {
			name = wtmp->name;
			break;
		}
		wtmp = wtmp->next;
	}

	/*
	 * Check for other checked radioboxes of the same name.
	 */
	cnt = 0;
	while ((wptr != NULL)&&(cnt < count)) {
		if ((wptr->type == W_RADIOBOX)&&
			(wptr->w != w)&&
			(XmToggleButtonGetState(wptr->w) == True)&&
			(wptr->name != NULL)&&
			(name != NULL)&&
			(strcmp(wptr->name, name) == 0))
		{
			XmToggleButtonSetState(wptr->w, False, False);
		}
		cnt++;
		wptr = wptr->next;
	}
}

/*
 * Catch all attempted modifications to the textfield for password
 * entry.  This is so we can prevent the password from showing
 * uponm the screen.
 * I would prefer that for all insereted characters a random 1-3 '*'s
 * were added, and any delete deleted the whole string, but due to
 * bugs in somve version of Motif 1.1 this won't work.
 */
void CBPasswordModify( Widget w, caddr_t client_data, caddr_t call_data)
{
	FormInfo *fptr = (FormInfo *)client_data;
	XmTextVerifyCallbackStruct *tv =(XmTextVerifyCallbackStruct *)call_data;
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WidgetInfo *wptr;
	int i, len;

	/* by default accept nothing
	tv->doit = False;
	 */

	/* only accept text modification of password fields */
	if (tv->reason != XmCR_MODIFYING_TEXT_VALUE)
		return;

	/* find the structure for this widget */
	wptr = hw->html.widget_list;
	while (wptr != NULL) {
		if (wptr->w == w)
			break;
		wptr = wptr->next;
	}
	if (wptr == NULL)
		return;

	/*  Deletion.  */
	if (tv->text->ptr == NULL) {
		tv->doit = True;

		/*
		 * Only can delete if we have stuff to delete.
		 */
		if ((wptr->password != NULL)&&(wptr->password[0] != '\0')) {
			int start;
			char *tptr;

			len = strlen(wptr->password);
			/*
			 * Find the start of the chunk of text to
			 * delete.
			 */
			if (tv->startPos < len) {
				start = tv->startPos;
			} else {
				start = len - 1;
			}

			/*
			 * might be more stuff after the end that we
			 * want to move up
			 */
			if (tv->endPos > len) {
				tptr = &(wptr->password[len]);
			} else {
				tptr = &(wptr->password[tv->endPos]);
			}
			wptr->password[start] = '\0';
			strcat(wptr->password, tptr);
		}
	}
	/*
	 * Else insert character.
	 */
	else if (tv->text->length >= 1) {
		int maxlength, plen;
		Cardinal argcnt;
		Arg arg[5];

		/*
		 * No insertion if it makes you exceed maxLength
		 */
		if (wptr->password == NULL) {
			plen = 0;
		} else {
			plen = strlen(wptr->password);
		}
		maxlength = 1000000;
		argcnt = 0;
		XtSetArg(arg[argcnt], XmNmaxLength, &maxlength); argcnt++;
		XtGetValues(w, arg, argcnt);
		if ((plen + tv->text->length) > maxlength) {
			return;
		}

		if (wptr->password == NULL) {
			wptr->password = (char *)malloc(tv->text->length + 1);
			for (i=0; i < tv->text->length; i++) {
				wptr->password[i] = tv->text->ptr[i];
			}
			wptr->password[tv->text->length] = '\0';
		}
		/*
		 * else insert a char somewhere.
		 * Make a new buffer.  Put everything from before the insert
		 * postion into it.  Now insert the character.
		 * Finally append any remaining text.
		 */
		else {
			char *buf;
			char *tptr;
			char tchar;
			int start;

			len = strlen(wptr->password);
			if (tv->startPos < len) {
				start = tv->startPos;
			} else {
				start = len;
			}
			tptr = &(wptr->password[start]);
			tchar = *tptr;
			*tptr = '\0';
			buf = (char *)malloc(len + tv->text->length + 1);
			strcpy(buf, wptr->password);
			for (i=0; i < tv->text->length; i++) {
				buf[start + i] = tv->text->ptr[i];
			}
			buf[start + tv->text->length] = '\0';
			*tptr = tchar;
			strcat(buf, tptr);
			free(wptr->password);
			wptr->password = buf;
		}
		tv->doit = True;
		/*
		 * make a '*' show up instead of what they typed
		 */
		for (i=0; i < tv->text->length; i++) {
			tv->text->ptr[i] = '*';
		}
	}
}

/*
 * RETURN was hit in a textfield in a form.
 * If this is the only textfield in this form, submit the form.
 */
void CBActivateField( Widget w, caddr_t client_data, caddr_t call_data)
{
	FormInfo *fptr = (FormInfo *)client_data;
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WidgetInfo *wptr;
	int cnt, count;
	XmAnyCallbackStruct *cb = (XmAnyCallbackStruct *)call_data;

	/*
	 * Terminate the form if it was never properly terminated.
	 */
	if (fptr->end == -1)  /* unterminated FORM tag */ {
		wptr = hw->html.widget_list;
		cnt = 0;
		while (wptr != NULL) {
			cnt++;
			wptr = wptr->next;
		}
		count = cnt;
	} else {
		count = fptr->end - fptr->start;
	}

	/*
	 * Locate the start of the form.
	 */
	if (fptr->start == 0) {
		wptr = hw->html.widget_list;
	} else {
		wptr = hw->html.widget_list;
		while (wptr != NULL) {
			if (wptr->id == fptr->start) {
				wptr = wptr->next;
				break;
			}
			wptr = wptr->next;
		}
	}

	/*
	 * Count the textfields in this form.
	 */
	cnt = 0;
	while ((wptr != NULL)&&(cnt < count)) {
		if ((wptr->type == W_TEXTFIELD)||(wptr->type == W_PASSWORD))
			cnt++;
		wptr = wptr->next;
	}

	/*
	 * If this is the only textfield in this form, submit the form.
	 */
	if (cnt == 1)
		CBSubmitForm(w, client_data, call_data);
}

void CBResetForm( Widget w, caddr_t client_data, caddr_t call_data)
{
	FormInfo *fptr = (FormInfo *)client_data;
	HTMLWidget hw = (HTMLWidget)(fptr->hw);
	WidgetInfo *wptr;
	int widget_count, cnt;
	XmPushButtonCallbackStruct *pb =
		(XmPushButtonCallbackStruct *)call_data;

	if (fptr->end == -1)  /* unterminated FORM tag */
	{
		wptr = hw->html.widget_list;
		cnt = 0;
		while (wptr != NULL) {
			cnt++;
			wptr = wptr->next;
		}
		widget_count = cnt;
	} else {
		widget_count = fptr->end - fptr->start;
	}

	if (fptr->start == 0) {
		wptr = hw->html.widget_list;
	} else {
		wptr = hw->html.widget_list;
		while (wptr != NULL) {
			if (wptr->id == fptr->start) {
				wptr = wptr->next;
				break;
			}
			wptr = wptr->next;
		}
	}

	cnt = 0;
	while ((wptr != NULL)&&(cnt < widget_count)) {
		Widget child;
		Cardinal argcnt;
		Arg arg[5];

		switch(wptr->type) {
		case W_TEXTFIELD:
			if (wptr->value == NULL) {
			    XmTextFieldSetString(wptr->w, "");
			} else {
			    XmTextFieldSetString(wptr->w, wptr->value);
			}
			break;
		case W_TEXTAREA:
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNworkWindow, &child);
			argcnt++;
			XtGetValues(wptr->w, arg, argcnt);
			if (wptr->value == NULL) {
			    XmTextSetString(child, "");
			} else {
			    XmTextSetString(child, wptr->value);
			}
			break;
		case W_PASSWORD:
			if (wptr->value == NULL) {
			    /*
			     * Due to errors in Motif1.1, I can't
			     * call XmTextFieldSetString() here.
			     * Because I have a modifyVerify callback
			     * registered for this widget.
			     * I don't know if this error exists
			     * in Motif1.2 or not.
			     */
			    argcnt = 0;
			    XtSetArg(arg[argcnt], XmNvalue, "");
			    argcnt++;
			    XtSetValues(wptr->w, arg, argcnt);
			    if (wptr->password != NULL) {
				free(wptr->password);
				wptr->password = NULL;
			    }
			} else {
			    int i, len;

			    if (wptr->password != NULL) {
				free(wptr->password);
				wptr->password = NULL;
			    }
			    len = strlen(wptr->value);
			    wptr->password = (char *)malloc(len + 1);
			    for (i=0; i<len; i++)
				wptr->password[i] = '*';
			    wptr->password[len] = '\0';
			    XmTextFieldSetString(wptr->w, wptr->password);
			    strcpy(wptr->password, wptr->value);
			}
			break;
		case W_LIST:
		    {
			char **vlist;
			int vlist_cnt;
			STRING *val_list;
			int i;

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNworkWindow, &child);
			argcnt++;
			XtGetValues(wptr->w, arg, argcnt);

			if (wptr->value != NULL) {
			    vlist = ParseCommaList(wptr->value, &vlist_cnt);
			    val_list = (STRING *)malloc(vlist_cnt *
				sizeof(STRING));
			    XmListDeselectAllItems(child);
			    for (i=0; i<vlist_cnt; i++) {
				val_list[i] = XmStringCreateSimple(vlist[i]);
			    }
			    FreeCommaList(vlist, vlist_cnt);
			    if (vlist_cnt > 0) {
				argcnt = 0;
				XtSetArg(arg[argcnt], XmNselectedItems, val_list);
				argcnt++;
				XtSetArg(arg[argcnt],
					XmNselectedItemCount, vlist_cnt);
				argcnt++;
				XtSetValues(child, arg, argcnt);
			    }
			    for (i=0; i<vlist_cnt; i++) {
				XmStringFree(val_list[i]);
			    }
			    if (val_list != NULL) {
				free((char *)val_list);
			    }
			} else {
				XmListDeselectAllItems(child);
			}
		    }
			break;
		/*
		 * gack, we saved the widget id of the starting default
		 * into the value character pointer, just so we could
		 * yank it out here, and restore the default.
		 */
		case W_OPTIONMENU:
			if (wptr->value != NULL) {
				Widget hist = (Widget)wptr->value;

				argcnt = 0;
				XtSetArg(arg[argcnt], XmNmenuHistory, hist);
				argcnt++;
				XtSetValues(wptr->w, arg, argcnt);
			}
			break;
		case W_CHECKBOX:
		case W_RADIOBOX:
			if (wptr->checked == True) {
			  XmToggleButtonSetState(wptr->w, True, False);
			} else {
			  XmToggleButtonSetState(wptr->w, False, False);
			}
			break;
		case W_HIDDEN:
			break;
		default:
			break;
		}
		cnt++;
		wptr = wptr->next;
	}
}

void PrepareFormEnd( HTMLWidget hw, Widget w, FormInfo *fptr)
{
	XtAddCallback(w, XmNactivateCallback, 
                      (XtCallbackProc)CBSubmitForm, (caddr_t)fptr);
}

void PrepareFormReset( HTMLWidget hw, Widget w, FormInfo *fptr)
{
	XtAddCallback(w, XmNactivateCallback, 
                      (XtCallbackProc)CBResetForm, (caddr_t)fptr);
}

void HideWidgets( HTMLWidget hw)
{
	WidgetInfo *wptr;
	XEvent event;

	/*
	 * Make sure all expose events have been dealt with first.
	 */
	XmUpdateDisplay((Widget)hw);
	wptr = hw->html.widget_list;
	while (wptr != NULL) {
		if ((wptr->w != NULL)&&(wptr->mapped == True)) {
			XtSetMappedWhenManaged(wptr->w, False);
			wptr->mapped = False;
		}
		wptr = wptr->next;
	}
				/* Force the exposure events into the queue */
	XSync(XtDisplay(hw), False);
				/* Remove all Expose events for the view window */
	while (XCheckWindowEvent(XtDisplay(hw->html.view),
		XtWindow(hw->html.view), ExposureMask, &event) == True)
	{
	}
}

void MapWidgets( HTMLWidget hw)
{
	WidgetInfo *wptr;

	wptr = hw->html.widget_list;
	while (wptr != NULL) {
		if ((wptr->w != NULL)&&(wptr->mapped == False)) {
			wptr->mapped = True;
			XtSetMappedWhenManaged(wptr->w, True);
		}
		wptr = wptr->next;
	}
}

Boolean AlreadyChecked( HTMLWidget hw, FormInfo *fptr, char *name)
{
	WidgetInfo *wptr;
	Boolean radio_checked;

	radio_checked = False;
	wptr = hw->html.widget_list;
	while (wptr != NULL) {
		if ((wptr->id >= fptr->start)&&
			(wptr->type == W_RADIOBOX)&&
			(wptr->checked == True)&&
			(wptr->name != NULL)&&
			(name != NULL)&&
			(strcmp(wptr->name, name) == 0))
		{
			radio_checked = True;
			break;
		}
		wptr = wptr->next;
	}
	return(radio_checked);
}

WidgetInfo * AddNewWidget( HTMLWidget hw, FormInfo *fptr, Widget w,
	int type, int id, int x, int y,
	int width, int height,
	char *name, char *value, char **mapping, Boolean checked)
{
	WidgetInfo *wptr, *lptr;

	wptr = hw->html.widget_list;
	if (wptr == NULL) {
		wptr = (WidgetInfo *)malloc(sizeof(WidgetInfo));
		wptr->w = w;
		wptr->type = type;
		wptr->id = id;
		wptr->x = x;
		wptr->y = y;
		wptr->width = width;
		wptr->height = height;
		wptr->seeable=0;
		wptr->name = name;
		wptr->value = value;
		wptr->password = NULL;
		wptr->mapping = mapping;
		wptr->checked = checked;
		wptr->mapped = False;
		wptr->next = NULL;
		wptr->prev = NULL;
		hw->html.widget_list = wptr;
	} else {
		while (wptr->next != NULL) {
			wptr = wptr->next;
		}
		wptr->next = (WidgetInfo *)malloc(sizeof(WidgetInfo));
		lptr = wptr; /* save this to fill in prev field */
		wptr = wptr->next;
		wptr->prev = lptr;
		wptr->w = w;
		wptr->type = type;
		wptr->id = id;
		wptr->x = x;
		wptr->y = y;
		wptr->width = width;
		wptr->height = height;
                wptr->seeable=0;
		wptr->name = name;
		wptr->value = value;
		wptr->password = NULL;
		wptr->mapping = mapping;
		wptr->checked = checked;
		wptr->mapped = False;
		wptr->next = NULL;
	}
	if ((wptr->type == W_PASSWORD)&&(wptr->value != NULL)) {
		wptr->password = (char *)malloc(strlen(wptr->value) + 1);
		strcpy(wptr->password, wptr->value);
	}
	return(wptr);
}

/*
 * Get the next value in a comma separated list.
 * Also unescape the '\' escaping done in ComposeCommaList
 * and convert the single ''' characters back to '"'
 * characters
 */
char * NextComma( char *string)
{
        char *tptr;

        tptr = string;
        while (*tptr != '\0') {
                if (*tptr == '\\') {
                        *tptr = '\0';
                        strcat(string, (char *)(tptr + 1));
                        tptr++;
                } else if (*tptr == '\'') {
                        *tptr = '\"';
                        tptr++;
                } else if (*tptr == ',') {
                        return(tptr);
                } else {
                        tptr++;
                }
        }
        return(tptr);
}

char ** ParseCommaList( char *str, int *count)
{
	char *str_copy;
	char **list;
	char **tlist;
	char *tptr;
	char *val;
	int i, cnt;
	int max_cnt;

	*count = 0;
	if ((str == NULL)||(*str == '\0'))
		return((char **)NULL);
	str_copy = (char *)malloc(strlen(str) + 1);
	CHECK_OUT_OF_MEM(str_copy);
	strcpy(str_copy, str);

	list = (char **)malloc(50 * sizeof(char *));
	CHECK_OUT_OF_MEM(list);
	max_cnt = 50;

	/*
	 * This loop counts the number of objects
	 * in this list.
	 * As a side effect, NextComma() unescapes in place so
	 * "\\" becomes '\' and "\," becomes ',' and "\"" becomes '"'
	 */
	cnt = 0;
	val = str_copy;
	tptr = NextComma(val);
	while (*tptr != '\0') {
		if ((cnt + 1) == max_cnt) {
			tlist = (char **)malloc((max_cnt +50) * sizeof(char *));
			CHECK_OUT_OF_MEM(tlist);
			for (i=0; i<cnt; i++) {
				tlist[i] = list[i];
			}
			free((char *)list);
			list = tlist;
			max_cnt += 50;
		}
		*tptr = '\0';
		list[cnt] = (char *)malloc(strlen(val) + 1);
		CHECK_OUT_OF_MEM(list[cnt]);
		strcpy(list[cnt], val);
		cnt++;

		val = (char *)(tptr + 1);
		tptr = NextComma(val);
	}
	list[cnt] = (char *)malloc(strlen(val) + 1);
	CHECK_OUT_OF_MEM(list[cnt]);
	strcpy(list[cnt], val);
	cnt++;

	free(str_copy);
	tlist = (char **)malloc(cnt * sizeof(char *));
	CHECK_OUT_OF_MEM(tlist);
	for (i=0; i<cnt; i++)
		tlist[i] = list[i];
	free((char *)list);
	list = tlist;

	*count = cnt;
	return(list);
}

/*
 * Compose a single string comma separated list from
 * an array of strings.  Any '\', or ',' in the
 * list are escaped with a prepending '\'.
 * So they become '\\' and '\,'
 * Also we want to allow '"' characters in the list, but
 * they would get eaten by the later parsing code, so we will
 * turn '"' into ''', and turn ''' into '\''
 */
char * ComposeCommaList( char **list, int cnt)
{
	int i;
	char *fail;
	char *buf;
	char *tbuf;
	int len, max_len;

	fail = (char *)malloc(1);
	*fail = '\0';

	if (cnt == 0)
		return(fail);

	buf = (char *)malloc(1024);
	if (buf == NULL)
		return(fail);
	max_len = 1024;
	len = 0;
	buf[0] = '\0';

	for (i=0; i<cnt; i++) {
		char *option;
		char *tptr;
		int olen;

		option = list[i];
		if (option == NULL) {
			olen = 0;
		} else {
			olen = strlen(option);
		}
		if ((len + (olen * 2)) >= (max_len-2)) /* amb 12/24/94 */ {
			tbuf = (char *)malloc(max_len + olen + 1024);
			if (tbuf == NULL)
				return(fail);
			strcpy(tbuf, buf);
			free(buf);
			buf = tbuf;
			max_len = max_len + olen + 1024;
		}
		tptr = (char *)(buf + len);
		while ((option != NULL)&&(*option != '\0')) {
			if ((*option == '\\')||(*option == ',')||
				(*option == '\'')) {
				*tptr++ = '\\';
				*tptr++ = *option++;
				len += 2;
			} else if (*option == '\"') {
				*tptr++ = '\'';
				option++;
				len++;
			} else {
				*tptr++ = *option++;
				len++;
			}
		}
		if (i != (cnt - 1)) {
			*tptr++ = ',';
			len++;
		}
		*tptr = '\0';
	}

	tbuf = (char *)malloc(len + 1);
	if (tbuf == NULL)
		return(fail);
	strcpy(tbuf, buf);
	free(buf);
	buf = tbuf;
	free(fail);
	return(buf);
}

void FreeCommaList( char **list, int cnt)
{
	int i;

	for (i=0; i<cnt; i++) {
		if (list[i] != NULL)
			free(list[i]);
	}
	if (list != NULL)
		free((char *)list);
}

/*
 * Clean up the mucked value field for a TEXTAREA.
 * Unescape the things with '\' in front of them, and transform
 * lone ' back to "
 */
void UnMuckTextAreaValue( char *value)
{
	char *tptr;

	if ((value == NULL)||(value[0] == '\0'))
		return;
	tptr = value;
        while (*tptr != '\0') {
                if (*tptr == '\\') {
                        *tptr = '\0';
                        strcat(value, (char *)(tptr + 1));
                        tptr++;
                } else if (*tptr == '\'') {
                        *tptr = '\"';
                        tptr++;
                } else {
                        tptr++;
                }
        }
}

char * MapOptionReturn( char *val, char **mapping)
{
	int cnt;

	if (mapping == NULL) 
		return(val);
	cnt = 0;
	while (mapping[cnt] != NULL) {
		if (strcmp(mapping[cnt], val) == 0)
			return(mapping[cnt + 1]);
		cnt += 2;
	}
	return(val);
}

char ** MakeOptionMappings( char **list1, char **list2, int list_cnt)
{
	int i, cnt;
	char **list;

	/*
	 * pass through to see how many mappings we have.
	 */
	cnt = 0;
	for (i=0; i<list_cnt; i++) {
		if ((list2[i] != NULL)&&(*list2[i] != '\0'))
			cnt++;
	}
	if (cnt == 0)
		return(NULL);
	list = (char **)malloc(((2 * cnt) + 1) * sizeof(char *));
	if (list == NULL)
		return(NULL);
	cnt = 0;
	for (i=0; i<list_cnt; i++) {
		if ((list2[i] != NULL)&&(*list2[i] != '\0')) {
			list[cnt] = (char *)malloc(strlen(list1[i]) + 1);
			list[cnt + 1] = (char *)malloc(strlen(list2[i]) + 1);
			if ((list[cnt] == NULL)||(list[cnt + 1] == NULL))
				return(NULL);
			strcpy(list[cnt], list1[i]);
			strcpy(list[cnt + 1], list2[i]);
			cnt += 2;
		}
	}
	list[cnt] = NULL;
	return(list);
}

/* Make the appropriate widget for this tag, and fill in an
 * WidgetInfo structure and return it.
 */
WidgetInfo * MakeWidget( HTMLWidget hw, char *text,
	PhotoComposeContext *pcc , int id)
{
	Arg arg[30];
	Cardinal argcnt;
	Widget w;
	WidgetInfo *wlist;
	WidgetInfo *wptr;
	Dimension width, height;
	int x=pcc->x;
	int y=pcc->y;
	FormInfo *fptr = pcc->cur_form;

	wlist = hw->html.widget_list;
	while (wlist != NULL) {
		if (wlist->id == id)
			break;
		wlist = wlist->next;
	}

	/* If this widget is not on the list, we have never
	 * used it before.  Create it now.
	 */
	if (wlist == NULL) {
		char widget_name[100];
		char **mapping;
		char *tptr;
		char *value;
		char *name;
		char *type_str;
		int type;
		short size;
		int maxlength;
		Boolean checked;

		mapping = NULL;
		checked = False;
		name = ParseMarkTag(text, MT_INPUT, "NAME");

		/*
		 * We may need to shorten the name for the widgets,
		 * which can't handle long names.
		 */
		if (name == NULL) {
			widget_name[0] = '\0';
		}
		else if (strlen(name) > 99) {
			strncpy(widget_name, name, 99);
			widget_name[99] = '\0';
		} else {
			strcpy(widget_name, name);
		}
		type_str = ParseMarkTag(text, MT_INPUT, "TYPE");
		if ((type_str != NULL)&&(strcasecmp(type_str, "checkbox") == 0)) {
			XmString label;

			type = W_CHECKBOX;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if (value == NULL) {
				value = (char *)malloc(strlen("on") + 1);
				strcpy(value, "on");
			}

			tptr = ParseMarkTag(text, MT_INPUT, "CHECKED");

			/* We want no text on our toggles */
			label = XmStringCreateSimple("");

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNlabelString, label); argcnt++;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE);
			   argcnt++;*/
			
			if (tptr != NULL) {
				XtSetArg(arg[argcnt], XmNset, True); argcnt++;
				checked = True;
				free(tptr);
			}
			w = XmCreateToggleButton(hw->html.view, widget_name,
				arg, argcnt);

			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			XmStringFree(label);
		}
		else if((type_str != NULL)&&(strcasecmp(type_str, "hidden")==0)) {
			type = W_HIDDEN;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if (value == NULL) {
				value = (char *)malloc(1);
				value[0] = '\0';
			}

			w = NULL;
		}
		else if ((type_str != NULL)&&(strcasecmp(type_str, "radio") == 0))
		{
			XmString label;

			type = W_RADIOBOX;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if (value == NULL) {
				value = (char *)malloc(strlen("on") + 1);
				strcpy(value, "on");
			}

			/*
			 * Only one checked radio button with the
			 * same name per form
			 */
			tptr = ParseMarkTag(text, MT_INPUT, "CHECKED");
			if ((tptr != NULL)&&
			   (AlreadyChecked(hw, fptr, name) == True)) {
				free(tptr);
				tptr = NULL;
			}

			/* We want no text on our toggles */
			label = XmStringCreateSimple("");

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNlabelString, label); argcnt++;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			XtSetArg(arg[argcnt], XmNindicatorType, XmONE_OF_MANY);
			argcnt++;
			/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE);
			argcnt++;*/
			if (tptr != NULL) {
				XtSetArg(arg[argcnt], XmNset, True); argcnt++;
				checked = True;
				free(tptr);
			}
			w = XmCreateToggleButton(hw->html.view, widget_name,
				arg, argcnt);
  
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			XtAddCallback(w, XmNvalueChangedCallback,
				(XtCallbackProc)CBChangeRadio, (caddr_t)fptr);

			XmStringFree(label);
		}
		else if((type_str != NULL)&&(strcasecmp(type_str, "submit") == 0))
		{
			XmString label;

			type = W_PUSHBUTTON;
			label = NULL;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if ((value == NULL)||(*value == '\0')) {
				value = (char *)malloc(strlen("Submit") +1);
				strcpy(value, "Submit");
			}
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE); argcnt++;*/
			label = XmStringCreateSimple(value);
			XtSetArg(arg[argcnt], XmNlabelString, label);
			argcnt++;
			w = XmCreatePushButton(hw->html.view, widget_name,
				arg, argcnt);
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			if (label != NULL) {
				XmStringFree(label);
			}
                        PrepareFormEnd(hw, w, fptr);

		}
		else if ((type_str != NULL)&&(strcasecmp(type_str, "reset") == 0))
		{
			XmString label;

			type = W_PUSHBUTTON;
			label = NULL;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if ((value == NULL)||(*value == '\0')) {
				value = (char *)malloc(strlen("Reset") + 1);
				strcpy(value, "Reset");
			}
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE);
			argcnt++;*/
			if (value != NULL) {
				label = XmStringCreateSimple(value);
				XtSetArg(arg[argcnt], XmNlabelString, label);
				argcnt++;
			}
			w = XmCreatePushButton(hw->html.view, widget_name,
				arg, argcnt);
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			if (label != NULL)
				XmStringFree(label);
			PrepareFormReset(hw, w, fptr);
		}
		else if((type_str != NULL)&&(strcasecmp(type_str, "button") == 0))
		{
			XmString label;

			type = W_PUSHBUTTON;
			label = NULL;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
                        /*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE);
                        argcnt++; */
			if (value != NULL) {
				label = XmStringCreateSimple(value);
				XtSetArg(arg[argcnt], XmNlabelString, label);
				argcnt++;
			}
			w = XmCreatePushButton(hw->html.view, widget_name,
				arg, argcnt);
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			if (label != NULL) {
				XmStringFree(label);
			}
		}
		else if((type_str != NULL)&&(strcasecmp(type_str, "select") == 0))
		{
			XmString label;
			Widget scroll;
			Widget pulldown, button, hist;
			char *options;
			char *returns;
			char **list;
			int list_cnt;
			char **ret_list;
			int return_cnt;
			char **vlist;
			int vlist_cnt;
			int i, mult;

			type = -1;
			tptr = ParseMarkTag(text, MT_INPUT, "HINT");
			if ((tptr != NULL)&&(strcasecmp(tptr, "list") == 0)) {
				type = W_LIST;
			}
			else if ((tptr != NULL)&&(strcasecmp(tptr, "menu") == 0))
			{
				type = W_OPTIONMENU;
			}
			if (tptr != NULL) {
				free(tptr);
			}
			size = 5;
			tptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if (tptr != NULL) {
				size = atoi(tptr);
				if ((size > 1)&&(type == -1))
					type = W_LIST;
				free(tptr);
			}

			mult = 0;
			tptr = ParseMarkTag(text, MT_INPUT, "MULTIPLE");
			if (tptr != NULL) {
				if (type == -1)
					type = W_LIST;
				mult = 1;
				free(tptr);
			}
			if (type == -1)
				type = W_OPTIONMENU;
			label = NULL;
			hist = NULL;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			options = ParseMarkTag(text, MT_INPUT, "OPTIONS");
			returns = ParseMarkTag(text, MT_INPUT, "RETURNS");
			list = ParseCommaList(options, &list_cnt);
			if (options != NULL) 
				free(options);

			ret_list = ParseCommaList(returns, &return_cnt);
			if (returns != NULL)
				free(returns);

			/*
			 * If return_cnt is less than list_cnt, the user made
			 * a serious error.  Try to recover by padding out
			 * ret_list with NULLs
			 */
			if (list_cnt > return_cnt) {
				int rcnt;
				char **rlist;

				rlist =(char **)malloc(list_cnt * sizeof(char *));
				for (rcnt = 0; rcnt < return_cnt; rcnt++)
					rlist[rcnt] = ret_list[rcnt];
				for (rcnt = return_cnt; rcnt < list_cnt; rcnt++)
					rlist[rcnt] = NULL;
				if (ret_list != NULL)
					free((char *)ret_list);
				ret_list = rlist;
			}

			vlist = ParseCommaList(value, &vlist_cnt);
			if (size > list_cnt)
				size = list_cnt;
			if (size < 1)
				size = 1;
			mapping = MakeOptionMappings(list, ret_list, list_cnt);

			if (type == W_OPTIONMENU) {
                                Widget child;
                                XmString xmstr;
				argcnt = 0;
				pulldown = XmCreatePulldownMenu(hw->html.view,
					widget_name, arg, argcnt);

				for (i=0; i<list_cnt; i++) {
					char bname[30];

					sprintf(bname, "Button%d", (i + 1));
					label = XmStringCreateSimple(list[i]);
					argcnt = 0;
					XtSetArg(arg[argcnt], XmNlabelString,
						label);
					argcnt++;
					button = XmCreatePushButton(pulldown,
						bname, arg, argcnt);
					XtManageChild(button);
					XmStringFree(label);
					if ((vlist_cnt > 0)&&
					    (vlist[0] != NULL)&&
					    (strcmp(vlist[0], list[i]) ==0)) {
						hist = button;
					}

					/*
					 * Start hist out as the first button
					 * so that if the user didn't set a
					 * default we always default to the
					 * first element.
					 */
					if ((i == 0)&&(hist == NULL)) {
						hist = button;
					}
				}

				FreeCommaList(list, list_cnt);
				FreeCommaList(ret_list, list_cnt);
				FreeCommaList(vlist, vlist_cnt);
				if (value != NULL)
					free(value);
				argcnt = 0;
				XtSetArg(arg[argcnt], XmNx, x); argcnt++;
				XtSetArg(arg[argcnt], XmNy, y); argcnt++;
                                /* kill margins */
                                XtSetArg(arg[argcnt], XmNmarginWidth, 0);
                                argcnt++;
                                XtSetArg(arg[argcnt], XmNmarginHeight, 0);
                                argcnt++;
				XtSetArg(arg[argcnt], XmNsubMenuId, pulldown);
					argcnt++;
				/*XtSetArg(arg[argcnt], XmNnavigationType, XmTAB_GROUP);
				argcnt++;*/
				if (hist != NULL) {
					XtSetArg(arg[argcnt], XmNmenuHistory,
						hist);
					argcnt++;
					/*
					 * A gaggage.  Value is used to later
					 * restore defaults.  For option menu
					 * this means we need to save a child
					 * widget id as opposed to the
					 * character string everyone else uses.
					 */
					value = (char *)hist;
				}
				w = XmCreateOptionMenu(hw->html.view,
					widget_name, arg, argcnt);
                                argcnt = 0;
                                xmstr = XmStringCreateSimple ("");
                                XtSetArg(arg[argcnt], XmNlabelString,
                                         (XtArgVal)xmstr);
                                argcnt++;
                                XtSetArg(arg[argcnt], XmNwidth, 0);
                                argcnt++;

                                child = XmOptionLabelGadget (w);
                                XtSetValues (child, arg, argcnt);
                                XmStringFree (xmstr);
                        } else /* type == W_LIST */ {
				XmString *string_list;
				XmString *val_list;

				if ((!mult)&&(vlist_cnt > 1)) {
					free(value);
					value = (char *)malloc(
						strlen(vlist[0]) + 1);
					strcpy(value, vlist[0]);
				}

				string_list = (XmString *)malloc(list_cnt *
					sizeof(XmString));
				val_list = (XmString *)malloc(vlist_cnt *
					sizeof(XmString));

				for (i=0; i<list_cnt; i++) {
					string_list[i] =
						XmStringCreateSimple(list[i]);
				}
				for (i=0; i<vlist_cnt; i++) {
					val_list[i] =
						XmStringCreateSimple(vlist[i]);
				}
				FreeCommaList(list, list_cnt);
				FreeCommaList(ret_list, list_cnt);
				FreeCommaList(vlist, vlist_cnt);

				argcnt = 0;
				XtSetArg(arg[argcnt], XmNx, x); argcnt++;
				XtSetArg(arg[argcnt], XmNy, y); argcnt++;
				scroll = XmCreateScrolledWindow(hw->html.view,
					"Scroll", arg, argcnt);

				argcnt = 0;
				XtSetArg(arg[argcnt], XmNitems, string_list);
				argcnt++;
				XtSetArg(arg[argcnt], XmNitemCount, list_cnt);
				argcnt++;
				XtSetArg(arg[argcnt], XmNvisibleItemCount,size);
				argcnt++;
				if (mult) {
					XtSetArg(arg[argcnt],XmNselectionPolicy,
						XmEXTENDED_SELECT);
					argcnt++;
				} else {
					XtSetArg(arg[argcnt],XmNselectionPolicy,
						XmBROWSE_SELECT);
					argcnt++;
				}
				if ((vlist_cnt > 0)&&(mult)) {
					XtSetArg(arg[argcnt], XmNselectedItems,
						val_list);
					argcnt++;
					XtSetArg(arg[argcnt],
						XmNselectedItemCount, vlist_cnt);
					argcnt++;
				}
				else if ((vlist_cnt > 0)&&(!mult)) {
					XtSetArg(arg[argcnt], XmNselectedItems,
						&val_list[0]);
					argcnt++;
					XtSetArg(arg[argcnt],
						XmNselectedItemCount, 1);
					argcnt++;
				}
				w = XmCreateList(scroll, widget_name,arg, argcnt);
                                XtManageChild(w);
				XtManageChild(w);
				w = scroll;
				for (i=0; i<list_cnt; i++)
					XmStringFree(string_list[i]);
				if (string_list != NULL)
					free((char *)string_list);
				for (i=0; i<vlist_cnt; i++)
					XmStringFree(val_list[i]);
				if (val_list != NULL)
					free((char *)val_list);
			}
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
		}
		else if((type_str!=NULL)&&(strcasecmp(type_str, "password") ==0))
		{
			type = W_PASSWORD;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");

			size = -1;
			maxlength = -1;
			tptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if (tptr != NULL) {
				size = atoi(tptr);
				free(tptr);
			}

			tptr = ParseMarkTag(text, MT_INPUT, "MAXLENGTH");
			if (tptr != NULL) {
				maxlength = atoi(tptr);
				free(tptr);
			}

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			if (size > 0) {
				XtSetArg(arg[argcnt], XmNcolumns, size);
				argcnt++;
			}
			if (maxlength > 0) {
				XtSetArg(arg[argcnt], XmNmaxLength, maxlength);
				argcnt++;
			}
			if (value != NULL) {
				int i, len;
				char *bval;

				len = strlen(value);
				bval = (char *)malloc(len + 1);
				for (i=0; i<len; i++)
					bval[i] = '*';
				bval[len] = '\0';
				XtSetArg(arg[argcnt], XmNvalue, bval);
				argcnt++;
			}
                        /*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE);
                        argcnt++;*/
			w = XmCreateTextField(hw->html.view, widget_name,
				arg, argcnt);

			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
			XtAddCallback(w, XmNactivateCallback,
				(XtCallbackProc)CBActivateField, (caddr_t)fptr);
			XtAddCallback(w, XmNmodifyVerifyCallback,
				(XtCallbackProc)CBPasswordModify, (caddr_t)fptr);
		}
		else if((type_str!=NULL)&&(strcasecmp(type_str, "textarea") ==0))
		{
			int rows, cols;
			Widget scroll;

			type = W_TEXTAREA;

			/* look for ROWS and COLS directly. */
			rows = 4;
			cols = 40;
			tptr = ParseMarkTag(text, MT_INPUT, "ROWS");
			if (tptr != NULL) {
				rows = atoi(tptr);
				free(tptr);
			}
			if ( rows <= 0)
				rows = 4;
			tptr = ParseMarkTag(text, MT_INPUT, "COLS");
			if (tptr != NULL) {
				cols = atoi(tptr);
				free(tptr);
			}
			if (cols <=0)
				cols = 40;

			/* Grab the starting value of the text here.
			 * NULL if none.
			 */
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			UnMuckTextAreaValue(value);

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			scroll = XmCreateScrolledWindow(hw->html.view,
				"Scroll", arg, argcnt);

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNeditMode, XmMULTI_LINE_EDIT);
			argcnt++;
			XtSetArg(arg[argcnt], XmNcolumns, cols); argcnt++;
			XtSetArg(arg[argcnt], XmNrows, rows); argcnt++;
			if (value != NULL) {
				XtSetArg(arg[argcnt], XmNvalue, value);
				argcnt++;
			}
/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE); argcnt++;*/ 
			w = XmCreateText(scroll, widget_name, arg, argcnt);
			XtManageChild(w);
			w = scroll;
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);
		}
		else /* if no type, assume type=text . Single line text field */
		{
			int cols=40;

			/* SIZE can be  COLUMNS, assume a TEXTFIELD */
			type = W_TEXTFIELD;
			tptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if (tptr != NULL){
				cols = atoi(tptr);
				free(tptr);
			}
			/* Grab the starting value of text here. NULL if none. */
			value = ParseMarkTag(text, MT_INPUT, "VALUE");

			/* parse maxlength and set up the widget. */
			maxlength = -1;
			tptr = ParseMarkTag(text, MT_INPUT,"MAXLENGTH");
			if (tptr != NULL) {
				maxlength = atoi(tptr);
				free(tptr);
			}
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			XtSetArg(arg[argcnt], XmNcolumns, cols); argcnt++;
			if (maxlength > 0) {
				XtSetArg(arg[argcnt], XmNmaxLength, maxlength);
				argcnt++;
			}
			if (value != NULL) {
				XtSetArg(arg[argcnt], XmNvalue, value); argcnt++;
			}
/*XtSetArg(arg[argcnt], XmNnavigationType, XmNONE); argcnt++;*/

			w = XmCreateTextField(hw->html.view,
					widget_name, arg, argcnt);
			XtSetMappedWhenManaged(w, False);
			XtManageChild(w);

			/* For textfields, a CR might be an activate */
			XtAddCallback(w, XmNactivateCallback,
				(XtCallbackProc)CBActivateField, (caddr_t)fptr);
		}
		if (type_str != NULL)
			free(type_str);

		/*
		 * Don't want to do GetValues if this is HIDDEN input
		 * tag with no widget.
		 */
		if (w != NULL) {
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNwidth, &width); argcnt++;
			XtSetArg(arg[argcnt], XmNheight, &height); argcnt++;
			XtGetValues(w, arg, argcnt);
			/* Set it to default so we don't lose it on "back"*/
                        XtVaSetValues(w,
                          XmNbackground, hw->html.background_SAVE,
                          XmNtopShadowColor, hw->html.top_color_SAVE,
                          XmNbottomShadowColor, hw->html.bottom_color_SAVE,
                          NULL);
		} else {
			width = 0;
			height = 0;
		}

		wptr = AddNewWidget(hw, fptr, w, type, id, x, y, width, 
			height, name, value, mapping, checked);
	} else {
	/*
	 * We found this widget on the list of already created widgets.
	 * Put it in place for reuse.
	 */
		wlist->x = x;
		wlist->y = y;

		/*
		 * Don't want to SetValues if type HIDDEN which
		 * has no widget.
		 */
		if (wlist->w != NULL) {
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, y); argcnt++;
			XtSetValues(wlist->w, arg, argcnt);
			/* Set it to default so we don't lose it on "back"*/
                        XtVaSetValues(wlist->w,
                          XmNbackground, hw->html.background_SAVE,
                          XmNtopShadowColor, hw->html.top_color_SAVE,
                          XmNbottomShadowColor, hw->html.bottom_color_SAVE,
                          NULL);
		}

		wptr = wlist;
	}
	return(wptr);
}

void WidgetRefresh( HTMLWidget hw, struct ele_rec *eptr)
{
/*
unsigned long wp=WhitePixel(XtDisplay(hw),DefaultScreen(XtDisplay(hw)));
unsigned long bp=BlackPixel(XtDisplay(hw),DefaultScreen(XtDisplay(hw)));
*/

	if ((eptr->widget_data != NULL)&&(eptr->widget_data->mapped == False)&&
	    (eptr->widget_data->w != NULL)) {
		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
		XSetBackground(XtDisplay(hw), hw->html.drawGC, eptr->bg);

		eptr->widget_data->mapped = True;
		XtSetMappedWhenManaged(eptr->widget_data->w, True);
	}
}

/* Place a Widget. Add an element record for it.  */
void WidgetPlace(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext *pcc)
{
        struct ele_rec *eptr;
	WidgetInfo * widget_data;
	int width,height,baseline;

        pcc->widget_id++;      /* get a unique element id */
	widget_data = MakeWidget(hw, mptr->start, pcc,
			pcc->widget_id );
	if(widget_data == NULL){
		printf("[WidgetPlace] ERROR in making widget_data\n");
		return;
	}
	width = widget_data->width;
	height = widget_data->height;
	baseline = height;
        if (!pcc->preformat) {
                if( (pcc->x + width ) >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width))
			LinefeedPlace(hw, mptr, pcc);
        }
	if(pcc->computed_min_x < (width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = width + pcc->eoffsetx + pcc->left_margin;
        }                              
        if (pcc->x + width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + width;

	if (!pcc->cw_only){ 
		eptr = CreateElement(hw, E_WIDGET, pcc->cur_font, pcc->x, pcc->y,
			width, height, baseline, pcc);
                eptr->underline_number = 0; /* Widgets can't be underlined! */
		AdjustBaseLine(hw,eptr,pcc);
		eptr->widget_data = widget_data;
	} else {                   
                     if (pcc->cur_line_height < height)           
                                pcc->cur_line_height = height;        
        } 
	pcc->have_space_after = 0;     
        pcc->x = pcc->x + width ;      
        pcc->is_bol = False;
}

void HTMLFreeWidgetInfo(void *ptr)
{
	WidgetInfo *wptr = (WidgetInfo *)ptr;
	WidgetInfo *tptr;

	while (wptr != NULL) {
		tptr = wptr;
		wptr = wptr->next;
		if (tptr->w != NULL) {
/* This is REALLY DUMB, but X generates an expose event
* for the destruction of the Widgte, even if it isn't
* mapped at the time it is destroyed.
* So I move the invisible widget to -1000,-1000
* before destroying it, to avoid a visible flash.
*/     
			XtMoveWidget(tptr->w, -1000, -1000);
			XtDestroyWidget(tptr->w);
		}
		if (tptr->name != NULL)
			free(tptr->name);
		if ((tptr->value != NULL)&&(tptr->type != W_OPTIONMENU))
			free(tptr->value);
		free((char *)tptr);
	}
}

