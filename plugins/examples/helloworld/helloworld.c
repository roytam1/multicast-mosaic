#include <stdio.h>

#include <X11/StringDefs.h>
#include <Xm/Label.h>

#include "mplugin.h"

/* call by mMosaic in Pre-Parse html phase,  you can set 
 *	obs->load_data_method	(not really needed in this case)
 *	obs->mp_class->plugin_ext (if this is the first LOAD of plugins)
 *	obs->plugin_data	( could be set also in MPP_NEW)
 * NOTE: the mMosaic window is NOT created yet
 */

int MPP_Initialize(HtmlObjectPtr obs)
{
	printf("MPP_Initialize\n");
	if (obs->mp_class->release < MP_PLUGIN_RELEASE ) {
		/* ou afficher un message de chargement */
		printf("wrong mMosaic plugin release\n");
		return 0;
	}
	if( obs->width == 0 || obs->height == 0) {
		/* if you set width and height, then create a window
                 * else the plugin is  'external' */
                obs->width=100;		/* pixel */
                obs->height = 20;  
	}
	obs->load_data_method = MPP_URI_TO_AS_IS;	/* in case of <object data="..."> */
	return 1;		/* Always succed */
}
 
/* called by mMosaic after the parsing phase, mMosaic create a frame
 * if needed, works with it */

void MPP_New(HtmlObjectStruct *obs)
{
	Widget mosaic_rowcol; 
	Arg args[10];         
	Widget label_frame;   
	int i;                

	printf("MPP_New\n");
	/* the mMosaic widget is a  Motif RowColumn */
	mosaic_rowcol=(Widget)(obs->frame); /* XmRowColumn */

	/* Add a label in RowColumn */
	i=0;                  
	XtSetArg(args[i], XmNwidth, obs->width); i++;
	XtSetArg(args[i], XmNheight, obs->height); i++;
	XtSetArg(args[i], XmNborderWidth, 0); i++;
	XtSetArg(args[i], XmNrecomputeSize, 0); i++;
	XtSetArg(args[i], XmNalignment, XmALIGNMENT_CENTER); i++;
	label_frame=XmCreateLabel(mosaic_rowcol,"Hello World", args, i);                                      
	/* mMosaic let the widget unmapped, the plugin must map it */

	XtRealizeWidget(mosaic_rowcol); /* au cas ou... */
	XtRealizeWidget(label_frame);
	XtManageChild(mosaic_rowcol);
	XtSetMappedWhenManaged(label_frame,True);
	XtSetMappedWhenManaged(mosaic_rowcol,True);
	XtMapWidget(label_frame);

	/* store data for this object */
	obs->plugin_data=(void*)label_frame;
}

/* mMosaic new page or exit */
/* stop the plugin */

void MPP_Destroy(HtmlObjectStruct *p)
{
	Widget label_frame;

	label_frame=(Widget) p->plugin_data;
	XtDestroyWidget(label_frame);
}
