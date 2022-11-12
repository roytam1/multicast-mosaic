/* aprog.c
 * Author: Gilles Dauphin
 * Version 2.7b4m9 [Sept96]
 *
 * Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :  dauphin@sig.enst.fr
 */

/* aprog.c  initialize the toolkit using windowId option.
 * It is usefull for mMosaic.
 */
#include <stdlib.h>
#include <X11/StringDefs.h>
#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include "aprog.h"

#ifdef DEBUG_APROG
static void printwattr(XWindowAttributes * a);
#endif

static Atom wm_protocols[2];
static Atom delete_atom;
static Atom wm_protocol_atom;
static Widget web_toplevel = NULL;

static AprogAppData aprog_ressource_data_instance;

#define offset(x) XtOffset (AprogAppDataPtr, x)
static XtResource resources[] = {
	{ "windowId","WindowId",XtRInt,sizeof (int),
	  offset (window_id), XtRString, "0" },
	{ ApNdataFromWebCB, ApCDataFromWebCB, XtRCallback,
	  sizeof(XtCallbackList), offset(data_from_web_cb),XtRImmediate, NULL},
	{ ApNresizeYourSelfCB, ApCResizeYourSelfCB, XtRCallback,
	  sizeof(XtCallbackList), offset(resize_your_self_cb),XtRImmediate, NULL},
};
#undef offset

static void (*saveRealize)(Widget, XtValueMask *, XSetWindowAttributes *);
static void ProtocolHCB(Widget, XtPointer, XEvent *, Boolean * );
static void DestroyCB(Widget w, XtPointer clid, XEvent *ev, Boolean *cont );

static void _WebRealize(Widget w, XtValueMask *vm, XSetWindowAttributes *attr)
{
	XWindowAttributes wattr;
	Window xw;
	ApplicationShellRec * brec;

	brec = (ApplicationShellRec *) w;
	xw = (Window) aprog_ressource_data_instance.window_id;
	XGetWindowAttributes(XtDisplay(w), xw, &wattr);
	XChangeWindowAttributes(XtDisplay(w), xw, *vm, attr);

	brec->core.window = xw;
	brec->core.width = wattr.width;
	brec->core.height = wattr.height;
#ifdef DEBUG_APROG
	printwattr(&wattr);
#endif
}

/* it *must* be the toplevel widget */
void WebRealizeWidget(Widget w)
{
	XWindowAttributes wattr;
	ApplicationShellClassRec* appClass;
	ApplicationShellRec * brec;
	Window xw;

	brec = (ApplicationShellRec *) w;
	xw = (Window) aprog_ressource_data_instance.window_id;
	XGetWindowAttributes(XtDisplay(w), xw, &wattr);
	XtVaSetValues(w, XtNheight,wattr.height, XtNwidth,wattr.width, NULL);

	appClass =(ApplicationShellClassRec*) XtClass(w);
	saveRealize = appClass->core_class.realize;
	appClass->core_class.realize = _WebRealize;
	XtRealizeWidget (w);
	appClass->core_class.realize = saveRealize;

	XtAddRawEventHandler(w,(EventMask)0, True, ProtocolHCB,w);
	XtAddRawEventHandler(w,StructureNotifyMask, False, DestroyCB,w);
#ifdef DEBUG_APROG
	printf("XtWindowToWidget(wid)= %d\n",XtWindowToWidget(XtDisplay(w),
		w->core.window));
#endif
}

Boolean AprogInitialize(Widget toplevel)
{
	XWindowAttributes wattr;
	Window xw = (Window) 0;
	ApplicationShellClassRec* appClass;
	ApplicationShellRec * brec;

	XtGetApplicationResources( toplevel,
		(XtPointer) &aprog_ressource_data_instance,
		resources, XtNumber (resources),
		NULL,0);
	xw = (Window) aprog_ressource_data_instance.window_id;
	if ( !xw )
		return False;

	wm_protocol_atom = XInternAtom (XtDisplay(toplevel), 
				"WM_PROTOCOLS", False);
	wm_protocols[0] = delete_atom = XInternAtom(XtDisplay(toplevel),
                               "WM_DELETE_WINDOW", False);
	XSetWMProtocols(XtDisplay(toplevel), xw, wm_protocols, 1);

	web_toplevel = toplevel;
	XGetWindowAttributes(XtDisplay(toplevel), xw, &wattr);

	return True;
}

static void ProtocolHCB(Widget w, XtPointer clid, XEvent *ev, Boolean *cont )
{
	XClientMessageEvent *p_ev ; 
	Atom proto_type ;
	Atom proto_data ;

#ifdef DEBUG_APROG
	printf("ProtocolHCB\n");
#endif
	switch (ev->type) {  
        case ClientMessage:
		p_ev = (XClientMessageEvent *) ev; 
		proto_type = p_ev->message_type;
		proto_data = (Atom) p_ev->data.l[0];

		if (proto_type == wm_protocol_atom){
			if (proto_data == delete_atom)
				exit(0);
		}
	}
#ifdef DEBUG_APROG
	printf("ProtocolHCB not exiting ######\n");
#endif
}

static void DestroyCB(Widget w, XtPointer clid, XEvent *ev, Boolean *cont )
{
#ifdef DEBUG_APROG
	printf("DestroyCB\n");
#endif
	switch (ev->type) {  
        case DestroyNotify:
		if (ev->xdestroywindow.window == XtWindow(web_toplevel))
			exit(0);
	}
}
#ifdef DEBUG_APROG
static void printwattr(XWindowAttributes * a)
{
	printf("x=%d y=%d\n", a->x,a->y);
	printf("width=%d height=%d\n",a->width,a->height);
	printf("border_width=%d\n",a->border_width);
	printf("depth=%d\n",a->depth);
	printf("visual=%08x\n",a->visual);     
	printf("Window root=%08x\n",a->root);       
	printf("class=%08x\n",a->class);
	printf("bit_gravity=%08x\n",a->bit_gravity); 
	printf("win_gravity=%08x\n",a->win_gravity);
	printf("backing_store=%08x\n",a->backing_store);
	printf("backing_planes=%08x\n",a->backing_planes);
	printf("backing_pixel=%08x\n",a->backing_pixel);
	printf("save_under=%08x\n",a->save_under);
	printf("colormap=%08x\n",a->colormap);
	printf("map_installed=%08x\n",a->map_installed);      
	printf("map_state=%08x\n",a->map_state);
	printf("all_event_masks=%08x\n",a->all_event_masks);  
	printf("your_event_mask=%08x\n",a->your_event_mask); 
	printf("do_not_propagate_mask=%08x\n",a->do_not_propagate_mask);
	printf("override_redirect=%08x\n",a->override_redirect);
	printf("screen=%08x\n",a->screen);
}
#endif
