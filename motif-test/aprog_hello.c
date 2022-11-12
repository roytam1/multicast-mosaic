/* aprog_hello.c  initialize the toolkit using windowId option.
 * It is usefull for mMosaic.
 */
#include <Xm/XmP.h>
#include <Xm/DisplayP.h>
#include <Xm/PushBP.h>
#include <Xm/Protocols.h>


/*
/*typedef struct { 
/*    int x, y;                   /* location of window */
/*    int width, height;          /* width and height of window */
/*    int border_width;           /* border width of window */
/*    int depth;                  /* depth of window */
/*    Visual *visual;             /* the associated visual structure */
/*    Window root;                /* root of screen containing window */
/*#if defined(__cplusplus) || defined(c_plusplus)
/*    int c_class;                /* C++ InputOutput, InputOnly*/
/*#else
/*    int class;                  /* InputOutput, InputOnly*/
/*#endif
/*    int bit_gravity;            /* one of bit gravity values */
/*    int win_gravity;            /* one of the window gravity values */
/*    int backing_store;          /* NotUseful, WhenMapped, Always */
/*    unsigned long backing_planes;/* planes to be preserved if possible */
/*    unsigned long backing_pixel;/* value to be used when restoring planes */
/*    Bool save_under;            /* boolean, should bits under be saved? */
/*    Colormap colormap;          /* color map to be associated with window */
/*    Bool map_installed;         /* boolean, is color map currently installed*/
/*    int map_state;              /* IsUnmapped, IsUnviewable, IsViewable */
/*    long all_event_masks;       /* set of events all people have interest in*/
/*    long your_event_mask;       /* my event mask */
/*    long do_not_propagate_mask; /* set of events that should not propagate */
/*    Bool override_redirect;     /* boolean value for override-redirect */
/*    Screen *screen;             /* back pointer to correct screen */
/*} XWindowAttributes;
*/

void printwattr(XWindowAttributes * a)
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
typedef struct {
	int window_id;
}AppData, *AppDataPtr;

AppData Rdata_instance;

#define offset(x) XtOffset (AppDataPtr, x)
XtResource resources[] = {
	{ "windowId","WindowId",XtRInt,sizeof (int),
	  offset (window_id), XtRString, "0" }
};
#undef offset

XrmOptionDescRec options[] = {
	{"-windowId",  ".windowId",  XrmoptionSepArg,  "0"}
};

Widget	toplevel, button;
XmDisplayRec * brec1;
XmDisplayRec * brec;
Window 	xw;
XWindowAttributes wattr;
XtAppContext  app;

void delete_window_handler(Widget w, XtPointer clid, XtPointer calld)
{
	exit(0);
}
void bcb(Widget w, XtPointer clid, XtPointer calld)
{
	printf ("#####Hello Yourself!from aprog_hello#####\n");
	XGetWindowAttributes(XtDisplay(toplevel), xw, &wattr);
	printwattr(&wattr);
}

static void (*saveRealize)(Widget wid, 
        XtValueMask *vmask, 
        XSetWindowAttributes *attr );
static void WebRealize(
        Widget wid,
        XtValueMask *vmask,
        XSetWindowAttributes *attr )
{
	printf ("in WebRealize window = %08x\n",Rdata_instance.window_id);
	wid->core.window = Rdata_instance.window_id;
}
void printEvent( XEvent *ev);
ApplicationShellClassRec* appClass;

Atom delete_atom;
extern int _Xdebug ;
void ProtocolHCB(Widget w, XtPointer clid, XEvent *ev, Boolean *cont )
{
	printf("ProtocolHCB\n");

	switch (ev->type) {  
        case ClientMessage:
	{
		XClientMessageEvent *p_ev = (XClientMessageEvent *) ev; 
		Atom proto_type = p_ev->message_type;
		Atom proto_data = (Atom) p_ev->data.l[0];
		
		if (proto_data == delete_atom)
			exit(0);
	}
	}

}
void DestroyCB(Widget w, XtPointer clid, XEvent *ev, Boolean *cont )
{
	printf("#########DestroyCB###########\n");

	switch (ev->type) {  
        case DestroyNotify:
		if (ev->xdestroywindow.window == XtWindow(toplevel))
			exit(0);
	}
	printf("#########End DestroyCB###########\n");
}
	

main( int argc, char *argv[])
{
	XmString 	  label;

	_Xdebug =1;

	XtSetLanguageProc (NULL, NULL, NULL);
	toplevel = XtAppInitialize (&app, "Hello", options, XtNumber (options),
		&argc, argv, NULL, NULL,0);
	XtGetApplicationResources( toplevel,
		(XtPointer) &Rdata_instance,
		resources, XtNumber (resources),
		NULL,0);
	xw = (Window) Rdata_instance.window_id;
	XGetWindowAttributes(XtDisplay(toplevel), xw, &wattr);
printf("XtWindow(toplevel) = %d , %08x apres INIT\n",XtWindow(toplevel),XtWindow(toplevel));

	appClass =(ApplicationShellClassRec*) XtClass(toplevel);
	saveRealize = appClass->core_class.realize;
	appClass->core_class.realize = WebRealize;
XtRealizeWidget (toplevel);
	appClass->core_class.realize = saveRealize;

	brec = (XmDisplayRec *) toplevel;
	brec->core.window = xw;
	brec->core.width = wattr.width;
	brec->core.height = wattr.height;


	brec1 = (XmDisplayRec *) XmGetXmDisplay(XtDisplay(toplevel));
/*	brec1->core.window = xw;*/
/*	brec1->core.width = wattr.width;
	brec1->core.height = wattr.height;
*/
printf("XtWindow(toplevel) = %d , %08x apres Realize\n",XtWindow(toplevel),XtWindow(toplevel));


	delete_atom = XmInternAtom(XtDisplay(toplevel),
                               "WM_DELETE_WINDOW",
                               FALSE);

/*
	XmAddWMProtocols(toplevel, &delete_atom, 1);

	XmSetWMProtocolHooks( toplevel,
                         delete_atom, NULL, NULL,
                         delete_window_handler, NULL);

*/

printwattr(&wattr);
printf("##########################################################\n");

	label = XmStringCreateLocalized("From aprog_hello: Push here say hello"); 
	button = XtVaCreateManagedWidget ("pushme",
		xmPushButtonWidgetClass, toplevel,
		XmNlabelString, label,
		XmNx,10,
		XmNy,10,
		NULL);
	XmStringFree (label);
	XtAddCallback (button, XmNactivateCallback, bcb, NULL);


	XtAddRawEventHandler(toplevel,(EventMask)0, True, 
		ProtocolHCB,toplevel);
	XtAddRawEventHandler(toplevel,StructureNotifyMask, False, 
		DestroyCB,toplevel);


	XtRealizeWidget (toplevel);
/*	XtAppMainLoop (app);*/
	{
    		XEvent event;
 
    		for (;;) {
        		XtAppNextEvent(app, &event);
			printEvent(&event);
        		XtDispatchEvent(&event);
    		}
	}      

}

void printEvent( XEvent *ev)
{
	XEvent e = *ev;
	Widget bid;

	printf("#############################################\n");
	printf("	type = %08x\n",e.xany.type);
	printf("	send_event = %08x\n",e.xany.send_event);
	printf("	Display = %08x\n", e.xany.display);
	printf("XtDisplay(toplevel)=%08x\n",XtDisplay(toplevel));
	printf("	Window = %08x\n",e.xany.window);
	printf(" Widget toplevel= %08x\n", toplevel);
	bid = XtWindowToWidget(e.xany.display, e.xany.window);
	printf("XtWindowtoWidget=%08x\n", bid);

	switch(e.type){
	case ButtonPress:
		printf("recoit l'evenement ButtonPress\n");
		printf("	subwindow = %d\n",e.xbutton.subwindow);
		break;
	case ButtonRelease:
		printf("recoit l'evenement ButtonRelease\n");
		printf("	subwindow = %d\n",e.xbutton.subwindow);
		break;
	case MotionNotify:
		printf("recoit l'evenement MotionNotify\n");
		printf("	subwindow = %d\n",e.xbutton.subwindow);
		break;
	case KeyPress:
		printf("recoit l'evenement KeyPress\n");
		break;
	case KeyRelease:
		printf("recoit l'evenement KeyRelease\n");
		break;
	case EnterNotify:
		printf("recoit l'evenement EnterNotify\n");
		break;
	case LeaveNotify:
		printf("recoit l'evenement LeaveNotify\n");
		break;
	case FocusIn:
		printf("recoit l'evenement FocusIn\n");
		break;
	case FocusOut:
		printf("recoit l'evenement FocusOut\n");
		break;
	case KeymapNotify:
		printf("recoit l'evenement KeymapNotify\n");
		break;
	case Expose:
		printf("recoit l'evenement Expose\n");
		break;
	case GraphicsExpose:
		printf("recoit l'evenement GraphicsExpose\n");
		break;
	case NoExpose:
		printf("recoit l'evenement NoExpose\n");
		break;
	case ConfigureNotify:
		printf("recoit l'evenement ConfigureNotify\n");
		break;
	case CirculateNotify:
		printf("recoit l'evenement CirculateNotify\n");
		break;
	case CreateNotify:
		printf("recoit l'evenement CreateNotify\n");
		break;
	case DestroyNotify:
		printf("recoit l'evenement DestroyNotify\n");
		printf(" Window event = %08x\n",e.xdestroywindow.event);
		printf(" Window window = %08x\n",e.xdestroywindow.window);
		break;
	case GravityNotify:
		printf("recoit l'evenement GravityNotify\n");
		break;
	case MapNotify:
		printf("recoit l'evenement MapNotify\n");
		break;
	case MappingNotify:
		printf("recoit l'evenement MappingNotify\n");
		break;
	case ReparentNotify:
		printf("recoit l'evenement ReparentNotify\n");
		break;
	case UnmapNotify:
		printf("recoit l'evenement UnmapNotify\n");
		break;
	case VisibilityNotify:
		printf("recoit l'evenement VisibilityNotify\n");
		break;
	case ClientMessage:
		printf("recoit l'evenement ClientMessage\n");
		printf("	message_type=%08x\n",e.xclient.message_type);
		break;
	case PropertyNotify:
		printf("recoit l'evenement PropertyNotify\n");
		break;
	case SelectionClear:
		printf("recoit l'evenement SelectionClear\n");
		break;
	case SelectionNotify:
		printf("recoit l'evenement SelectionNotify\n");
		break;
	case SelectionRequest:
		printf("recoit l'evenement SelectionRequest\n");
		break;
	default:
		printf("recoit un evenement INCONU %d \n",e.type);
		break;
	} /* switch */

	printf("#############################################\n");
}
