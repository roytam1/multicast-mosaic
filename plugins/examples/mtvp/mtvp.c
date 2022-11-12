/* G.Dauphin mtvp plugin */
/* 28 Sept 2000 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <X11/StringDefs.h>
#include <Xm/Label.h>

#include "HTMLparse.h"
#include "mplugin.h"

/* #define MTVP_PATH "/usr/local/bin/mtvp" */
#define MTVP_PATH "mtvp"
 
typedef struct {
	Widget mosaic_frame;
	Window mosaic_window;
	char * url;
	int pid;
	int internal;
}mtvpData;

/* mtvp don't need file, but an absolute URL */
/* convert <OBJECT DATA="uri"> to an absolute uri relative to html page*/

int MPP_Initialize(HtmlObjectPtr obs)
{
	obs->load_data_method = MPP_URI_TO_AURI; 
	return 1;
}


void MPP_New(HtmlObjectStruct *obs)
{
	mtvpData *mtvp_data;
	char *argv[10];
	char buf[100];

	mtvp_data = (mtvpData*) calloc(1,sizeof(mtvpData));
  	mtvp_data->mosaic_frame=(Widget)(obs->frame);
	mtvp_data->mosaic_window = XtWindow(mtvp_data->mosaic_frame);
	mtvp_data->url = obs->data_url; /* i sayed i want an absolute url */
	mtvp_data->internal = 1;

	if(obs->width == 0 || obs->height == 0) {	/* play external */
		mtvp_data->internal = 0;
	}

	argv[0] = MTVP_PATH;
	if (mtvp_data->internal) {
		sprintf( buf, "-W%d", mtvp_data->mosaic_window);
		argv[1] = buf;
		argv[2] = mtvp_data->url;
		argv[3] = NULL;

		XtSetMappedWhenManaged(mtvp_data->mosaic_frame,True);
		XtManageChild(mtvp_data->mosaic_frame);
		XtRealizeWidget(mtvp_data->mosaic_frame);
		XFlush(XtDisplay(mtvp_data->mosaic_frame));
	} else {
		argv[1] = mtvp_data->url;
		argv[2] = NULL;
	}

	mtvp_data->pid = fork();
	if(mtvp_data->pid == -1)
		assert(0);

	if(mtvp_data->pid) {	/* pere */
		obs->plugin_data = mtvp_data;
	} else {
		execvp(argv[0], argv);
		assert(0);
	}
}

void MPP_Destroy(HtmlObjectStruct *p)
{
	mtvpData* mtvp_data;

  	mtvp_data=(mtvpData*)(p->plugin_data);

	if (mtvp_data->internal) {
		kill(mtvp_data->pid,9);
		free(mtvp_data);
	} else {
		free(mtvp_data); /* let me play external */
	}
}
