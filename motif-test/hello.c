/* Written by Dan Heller and Paula Ferguson.  
 * Copyright 1994, O'Reilly & Associates, Inc.
 * Permission to use, copy, and modify this program without
 * restriction is hereby granted, as long as this copyright
 * notice appears in each copy of the program source code.
 * This program is freely distributable without licensing fees and
 * is provided without guarantee or warrantee expressed or implied.
 * This program is -not- in the public domain.
 */

/* hello.c -- initialize the toolkit using an application context and a 
 * toplevel shell widget, then create a pushbutton that says Hello using
 * the varargs interface.
 */
#include <Xm/RowColumn.h>
#include <Xm/PushB.h>

Widget        toplevel, button, button1;

main(argc, argv)
int argc;
char *argv[];
{
    XtAppContext  app;
    void          button_pushed();
    XmString 	  label;
    Widget rowcol;

    XtSetLanguageProc (NULL, NULL, NULL);

    toplevel = XtVaAppInitialize (&app, "Hello", NULL, 0,
        &argc, argv, NULL, NULL);
/*
    rowcol = XtVaCreateWidget("rowcol", xmRowColumnWidgetClass,toplevel,
	XmNorientation, XmHORIZONTAL,
	NULL);
*/
    label = XmStringCreateLocalized ("0Push0 to say hello"); 
    button = XtVaCreateManagedWidget ("pushme",
        xmPushButtonWidgetClass, toplevel,
        XmNlabelString, label,
        NULL);
/*
    label = XmStringCreateLocalized ("1Push1 to say hello"); 
    button1 = XtVaCreateManagedWidget ("pushme",
        xmPushButtonWidgetClass, rowcol,
        XmNlabelString, label,
        NULL);
    XtAddCallback (button1, XmNactivateCallback, button_pushed, NULL);
    XtManageChild(rowcol);
*/

    XmStringFree (label);
    XtAddCallback (button, XmNactivateCallback, button_pushed, NULL);

    XtRealizeWidget (toplevel);
    XtAppMainLoop (app);
}

Display *dpy;
int count =0;
Atom _XA_WM_PROTOCOLS;
static void send_clientmessage (w, a, timestamp)
    Window w;
    Atom a;
    Time timestamp;
{
    XClientMessageEvent ev;

    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = _XA_WM_PROTOCOLS;
    ev.format = 32;
    ev.data.l[0] = a;
    ev.data.l[1] = timestamp;
    XSendEvent (dpy, w, True, 0x00ffffff, (XEvent *) &ev);
}


void
button_pushed(widget, client_data, call_data)
Widget widget;
XtPointer client_data;
XtPointer call_data;
{
	Atom _XA_WM_DELETE_WINDOW;

	dpy = XtDisplay(toplevel);
	if (count == 0){
    		printf ("Hello Yourself! from Window %d\n", XtWindow(button));
		count++;
    		return;
	}
/*
	XDestroyWindow(XtDisplay(toplevel),XtWindow(toplevel));
	return;
*/

	_XA_WM_DELETE_WINDOW = XInternAtom (XtDisplay(toplevel), "WM_DELETE_WINDOW", False);
	_XA_WM_PROTOCOLS = XInternAtom (dpy, "WM_PROTOCOLS", False);
	send_clientmessage(XtWindow(button),_XA_WM_DELETE_WINDOW, CurrentTime);
	printf("sendind DELETE\n");
}

