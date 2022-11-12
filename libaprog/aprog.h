/* aprog.h
 * Author: Gilles Dauphin
 * Version 2.7b4m9 [Sept96]
 *
 * Copyright (C) 1996 - G.Dauphin
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY 
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION 
 * 
 * Bug report :  dauphin@sig.enst.fr
 */ 

#ifndef LIBAPROG_APROG_H
#define LIBAPROG_APROG_H

#define ApNdataFromWebCB "dataFromWebCB"
#define ApCDataFromWebCB "DataFromWebCB"
#define ApNresizeYourSelfCB "resizeYourSelfCB"
#define ApCResizeYourSelfCB "ResizeYourSelfCB"

typedef struct { 
	int window_id;
	XtCallbackList data_from_web_cb;
	XtCallbackList resize_your_self_cb;
}AprogAppData, *AprogAppDataPtr;

#define APROG_OPTION_DESC_VALUES \
	{"-windowId",  ".windowId",  XrmoptionSepArg,  "0"}

extern AprogAppData * AprogGetAppData();
extern Boolean AprogInitialize(Widget);
extern void WebRealizeWidget(Widget w);

#endif /* LIBAPROG_APROG_H */
