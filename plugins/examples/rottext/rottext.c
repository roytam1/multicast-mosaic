/*
 * Author:      Gilles Dauphin
 *              George Lise
 *              Hours Simon
 *              Deurveiller Gilles
 *              Ferlicot FrederiC
 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <assert.h>

#include <X11/StringDefs.h>
#include <Xm/Label.h>

#include "mplugin.h"
 
typedef struct {
	XtIntervalId id;
	XtAppContext app;
	int i;
	char *string;
	XtInputId *idI;
	int index;
	Widget label_frame;
}TimeClientData;


/* we cat the time to the text and rotate */

static void mise(XtPointer cl,XtIntervalId *id)
{
	char *tmp2; 
	char TOURNE[1000]="";
	char tmp1[1000]="";
	Widget myWin;
	HtmlObjectStruct *obs = (HtmlObjectStruct*) cl;
	TimeClientData *time1;
	XmString compose;
	int i;
	Arg args[10];
	char *chaine,*tmp;
	char thechaine[1000];
	time_t nsec;

	time(&nsec);
	chaine=ctime(&nsec);
   
	time1=(TimeClientData*)obs->plugin_data;
	i=time1->index; 
	tmp=time1->string;
	strcpy(thechaine,tmp);
	strcat(thechaine,chaine);
	if (i>=strlen(thechaine)) i=0;
	strncpy(tmp1,thechaine,i);
	tmp2=&(thechaine[i]);
	strncpy(TOURNE,tmp2,(strlen(thechaine)-i-1));
	strcat(TOURNE," ");
	strcat(TOURNE,tmp1);
	strcat(TOURNE,"\0");
	i++;

	time1->index=i;
	compose=XmStringCreateLtoR(TOURNE,XmFONTLIST_DEFAULT_TAG);
	myWin= time1->label_frame;
	(time1->i)++;
	XtSetArg(args[0],XmNlabelString,compose);
	XtSetValues(myWin /*### obs->frame*/,args,1);
	XmStringFree(compose);
	time1->id=XtAppAddTimeOut(time1->app,500, mise,obs );  
	XFlush(XtDisplay(myWin));
}

/* called by mMosaic after the parsing phase, mMosaic create a frame
 * if needed, works with it */

void MPP_New(HtmlObjectStruct *obs)
{
	char * text_rot = NULL;
	XtAppContext app; 
	Widget mosaic_frame;
	Widget label_frame;
	Arg args[10];
	TimeClientData *time;
	int i;

	assert(obs->width && obs->height);
	assert(obs->frame);

#ifdef DEBUG_PLUGIN
	printf("MPP_New\n");
#endif
  	mosaic_frame=(Widget)(obs->frame);	/* it's a Rowcolumn */
 
/* in <PARAM> find the value of TEXT="value" */

	for(i=0; i<obs->param_count; i++){
		if( !strcasecmp(obs->param_t[i]->name,"TEXT") ) {
			text_rot=strdup(obs->param_t[i]->value);
			break;
		}
	}
	if (!text_rot)
		text_rot = strdup("");

/* Create a widget in the mMosaic Rowcolumn Widget */
	i=0;
	XtSetArg(args[i], XmNwidth, obs->width); i++;
        XtSetArg(args[i], XmNheight, obs->height); i++;
	XtSetArg(args[i], XmNborderWidth, 0); i++;
	XtSetArg(args[i], XmNrecomputeSize, 0); i++;
	XtSetArg(args[i], XmNalignment, XmALIGNMENT_CENTER); i++;
  	label_frame=XmCreateLabel(mosaic_frame,text_rot,
		args, i);

  	app=XtWidgetToApplicationContext(mosaic_frame);

/* Create data for this object */
	time=(TimeClientData*)malloc(sizeof(TimeClientData));
	time->app=app;
	time->i=1;
/* Arm timer to rotate text */
	time->id=XtAppAddTimeOut(app ,1000L, mise, (XtPointer)obs); 
	time->string=text_rot;
	time->index=0;
	time->label_frame = label_frame;
	obs->plugin_data=(void*)time;	/* store into this object */

/* mMosaic do not map the widget, you MUST map the widget created by mMosaic */
	XtSetMappedWhenManaged(label_frame,True);
	XtSetMappedWhenManaged(mosaic_frame,True);
	XtManageChild(label_frame);
	XtManageChild(mosaic_frame);
	XtRealizeWidget(label_frame);
	XtRealizeWidget(mosaic_frame);
}

/* call by mMosaic in Pre-Parse html phase,  you can set 
 *	obs->load_data_method	(not really needed in this case)
 *	obs->mp_class->plugin_ext (if this is the first LOAD of plugins)
 *	obs->plugin_data	( could be set also in MPP_NEW)
 * in case of rotate text plugin, we need nothing , just return success 
 * NOTE: the mMosaic frame is NOT created yet
 */

int MPP_Initialize(HtmlObjectPtr obs)
{
#ifdef DEBUG_PLUGIN
	printf("Calling MPP_Initialize\n");
#endif
	if( obs->width == 0 || obs->height == 0) {
		return 0;	/* we are internal viewer only */
				/* we need a window */
	}
	obs->load_data_method = MPP_URI_TO_AS_IS;	/* in case of <object data="..."> */
	return 1;		/* Always succed */
}

/* mMosaic new page or exit */
/* stop the plugin */

void MPP_Destroy(HtmlObjectStruct *p)
{
	Widget label_frame;
	TimeClientData *time;

	if (p==NULL) return;

	time=(TimeClientData*)(p->plugin_data);
	XtRemoveTimeOut(time->id); 	/* remove timer callback */

	label_frame=time->label_frame; /*(Widget)MPP_GetValue(p,WINDOW,NULL);*/
	if (label_frame!=NULL) {
		XtSetMappedWhenManaged(label_frame,False);
		XFlush(XtDisplay(label_frame));
		XtDestroyWidget(label_frame);
	}
/* free what we malloc-ed */
	free(time->string);
	free(time);
}
