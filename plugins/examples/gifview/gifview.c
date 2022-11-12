/* G.Dauphin gifview plugin */
/* 28 Sept 2000 */

#include <stdio.h>
#include <string.h>
#include <time.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <assert.h>

#include <X11/StringDefs.h>
#include <Xm/PushB.h>
#include <Xm/Label.h>

#include "mplugin.h"
 
#define GIFV_PATH "/usr/local/bin/gifview"

typedef struct {
	Widget mosaic_frame;
	Window mosaic_window;
	char * fname;
	int pid;
	int internal;
}gifvData;

/* gifview need a file, set load data to MPP_URI_TO_FILE,
 * mMosaic will get this file for you, you will find it in obs->fname_for_data
 * when mMosaic call your MPP_NEW
 */

int MPP_Initialize(HtmlObjectStruct *obs)
{
	obs->load_data_method = MPP_URI_TO_FILE;
	return 1;
}

void MPP_New(HtmlObjectStruct *obs)
{
	gifvData *gifv_data;
	char *argv[10];
	char bufw[100];
	char bufbg[100];
	Arg xarg[1];
	Pixel bg;
	XColor bg_color;
	Colormap cmap;

	gifv_data = (gifvData*) calloc(1,sizeof(gifvData));
  	gifv_data->mosaic_frame=(Widget)(obs->frame);
	gifv_data->mosaic_window = XtWindow(gifv_data->mosaic_frame);
	gifv_data->fname = strdup(obs->fname_for_data);
	gifv_data->internal = 1;

        if(obs->width == 0 || obs->height == 0) {       /* play external */
                gifv_data->internal = 0;  
        }

	argv[0] = GIFV_PATH;
	if (gifv_data->internal) {
		XtVaGetValues(gifv_data->mosaic_frame, 
			XtNbackground, &bg,
			NULL);
		XtVaGetValues(gifv_data->mosaic_frame, 
			XtNcolormap, &cmap,
			NULL);
		bg_color.pixel = bg;
		XQueryColor(XtDisplay(gifv_data->mosaic_frame), cmap , &bg_color);

		sprintf( bufw, "-w%d", gifv_data->mosaic_window);
		argv[1] = bufw;
		argv[2] = "-a";
		argv[3] = "+e";
		argv[4] = "--bg";
		sprintf( bufbg, "#%04x%04x%04x",
			bg_color.red, bg_color.green, bg_color.blue);
		argv[5] = bufbg;
		argv[6] = gifv_data->fname;
		argv[7] = NULL;

		XtSetMappedWhenManaged(gifv_data->mosaic_frame,True);
		XtManageChild(gifv_data->mosaic_frame);
		XtRealizeWidget(gifv_data->mosaic_frame);
		XFlush(XtDisplay(gifv_data->mosaic_frame));
	} else {
		argv[1] = "-a";
		argv[2] = gifv_data->fname;
		argv[3] = NULL;
	}
	gifv_data->pid = fork();
	if(gifv_data->pid == -1)
		assert(0);

	if(gifv_data->pid) {	/* pere */
		obs->plugin_data = gifv_data;
	} else {
		execvp(argv[0], argv);
		assert(0);
	}
}


void MPP_Destroy(HtmlObjectStruct *p)
{
	gifvData* gifv_data;

  	gifv_data=(gifvData*)(p->plugin_data);

	if (gifv_data->internal) {
		kill(gifv_data->pid,9);
		free(gifv_data);
	} else {
		free(gifv_data);
	}
}
