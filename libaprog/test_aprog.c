/* test_aprog.c
 * Author: Gilles Dauphin
 * Version 2.7b4m9 [Sept96]
 *
 * Bug report :  dauphin@sig.enst.fr
 */

#include <stdio.h>
#include <Xm/PushB.h>
#include "aprog.h"

typedef struct {
        int supp_option;
}AppData, *AppDataPtr;

Widget		toplevel, button;
XtAppContext  	app;
Boolean		in_web = 0;
AppData 	res_data_instance;

#define offset(x) XtOffset (AppDataPtr, x)
static XtResource resources[] = {
        { "suppOption","suppOption",XtRInt,sizeof (int),
          offset (supp_option), XtRString, "0" }
};
#undef offset

XrmOptionDescRec options[] = {
	{"-suppOption",  ".suppOption",  XrmoptionSepArg,  "0"},
	APROG_OPTION_DESC_VALUES	/* Maaaaaagic Web */
};

void bcb(Widget w, XtPointer clid, XtPointer calld)
{
	if (in_web)
		printf("Hello: you are in WEB\n");
	else
		printf("Hello: you are on a simple Window Manager\n");
}

void main( int argc, char *argv[])
{
	XmString 	  label;

	XtSetLanguageProc (NULL, NULL, NULL);
	toplevel = XtAppInitialize (&app, "Hello", options, XtNumber (options),
		&argc, argv, NULL, NULL,0);
	XtGetApplicationResources( toplevel, (XtPointer) &res_data_instance,
		resources, XtNumber (resources), NULL,0);
	
	in_web = AprogInitialize(toplevel);	/* Maaaaaagic Web */

	label = XmStringCreateLocalized("From test_aprog: Where are You?"); 
	button = XtVaCreateManagedWidget ("pushme",
		xmPushButtonWidgetClass, toplevel,
		XmNlabelString, label,
		NULL);
	XmStringFree (label);
	XtAddCallback (button, XmNactivateCallback, bcb, NULL);
	if(in_web){
		WebRealizeWidget(toplevel);
	} else {
		XtRealizeWidget (toplevel);
	}
	XtAppMainLoop (app);
}
