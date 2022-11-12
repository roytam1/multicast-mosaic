/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <X11/Intrinsic.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "cci.h"
#include "cciBindings2.h"

#include "cciServer.h"
#include "../libhtmlw/list.h"
#include "HText.h"
#include "HTFormat.h"
#include "accept.h"
#include "bla.h"

static Boolean cciAccepting = 0; 
static int listenPortNumber=0;
static XtInputId connectInputID;
static List listOfConnections;
static List listOfSendOutput;
static List listOfSendAnchorTo; /* client in list if should receive	*/
static List listOfSendBrowserView;
static List listOfSendEvent;
static List listOfFileToURLs;
static List listOfForm;		/* list of form submitted by application */

/* ADC ugly hack below  ZZZZZ */
extern int CCIprotocol_handler_found;
extern int MCCIanchorcached; 


/* from cciBindings.c */
extern int cci_event;

struct Connection {
	XtInputId inputId;
	MCCIPort client;
};

struct SendWhatToWhom {
	MCCIPort client;
	char	*contentType;
};
struct FileURL {
	char *fileName;
	char *url;
};

struct FormSubmit{
	MCCIPort client;
	char *actionID;
	int valid;
}; 

cciStat *cciStatListFindEntry(MCCIPort findMe)
{
/* this finds an entry in listOfSendAnchorTo based on MCCIPort client. */

	cciStat *current;
	char notDone = 1;

	current = (cciStat *) ListHead(listOfSendAnchorTo);
	while (notDone) {
		if ((current == NULL) || (current->client == findMe))
			notDone = 0;
		else
			current = (cciStat *) ListNext(listOfSendAnchorTo);
	}
	return current;
}
  
cciStat *cciStatListDeleteEntry(MCCIPort deleteMe)
{
	cciStat *current;
  
	if (current = cciStatListFindEntry(deleteMe)) {
		ListDeleteEntry(listOfSendAnchorTo, (char*)current);
		return current;
	}
	return NULL;
}

void cciStatPreventSendAnchor(MCCIPort client, char *url)
{
/* this sets the flag that is checked for in cciSafeToSend, but
* only if sendAnchor is on for this client. 
*/
  
	cciStat *current;

	if ((current = cciStatListFindEntry(client)) && (url != NULL))
		current->url = strdup(url);
}
  
int cciSafeToSend(cciStat *current, char *url)
{
/* ensures that Mosaic does not send an ANCHOR in response to a GET 
 * if SEND ANCHOR is turned on.
 *
 * INPUTS:
 * 		url	The url for which the flag is to be set.
 * RETURNS:
 * 		returns 0 if flag set for (client,url) pair (not safe to send)
 *                      1 if flag is not set for (client,url) pair
 *			(safe to send)
 */
	int rc = 1;
	if (current->url != NULL) {
		rc = strcmp(url,current->url);
		free(current->url);
		current->url = NULL;
	}
	return rc;
} 

void cciStatFree(cciStat *i)
{
	free(i);
}

cciStat *cciStatNew(MCCIPort client, int status)
{
	cciStat *nw;

	nw = (cciStat *) malloc( sizeof(cciStat) );
	if (nw) {
		nw->client = client;
		nw->status = status;
		nw->url = NULL;
	}
	return nw;
}
  
/* set the state of sending anchors */
void MoCCISendAnchor( MCCIPort client,
	int sendIt) /* 0, don't send, 1 send before browser gets document, */
		    /* 2 send after browser gets document */
            /* 3 send it before and then let the CCI app handle it  ADC ZZZ */
{
	cciStat *statElt;

	statElt = cciStatListDeleteEntry(client);
	switch (sendIt) {
	case 0:
		if (statElt)
			cciStatFree(statElt);
		break;
	case 1: /* send before browser gets document */
		if (statElt)
			statElt->status = 1;
		else
			statElt = cciStatNew(client,1);
		if (statElt)
			ListAddEntry(listOfSendAnchorTo,(char*)statElt);
		break;
	case 2: /* send after browser gets document */
		if (statElt)
			statElt->status = 2;
		else
			statElt = cciStatNew(client,2);
		if (statElt)
			ListAddEntry(listOfSendAnchorTo,(char*)statElt);
		break;
	case 3: /* send before browser gets document, then let it handle it  ADC ZZZ */                                
		if (statElt)
			statElt->status = 3;         
		else                           
			statElt = cciStatNew(client,3);
		if (statElt)                   
			ListAddEntry(listOfSendAnchorTo,(char*)statElt);
		break;                         
	}
}

void MoCCISendAnchorToCCI( char *url, int beforeAfter)
/***************************************************************
 * beforeAfter - 	0: send to all clients in list
 * 			1: send to all clients with status == 1;
 *			2: send to all clients with status == 2:
 *                      3: send to all clients with status == 3:  ADC ZZZ
 ***************************************************************/
{
	cciStat *client;
  
	client = (cciStat *) ListHead(listOfSendAnchorTo);
	while(client) {
		if (cciSafeToSend(client, url)) /* test flag */
			switch (beforeAfter) {
			case 0:
				if (client->status == 3) {         /* ADC ZZZ */
					MCCIanchorcached = 1;  
					MCCISendAnchorHistory(client->client,url);
					MCCIanchorcached = 0;  
				} else                     
					MCCISendAnchorHistory(client->client,url);
				break;
			case 1:
				if (client->status == 1)
					MCCISendAnchorHistory(client->client,url);
				break;
			case 2:
				if (client->status == 2)
					MCCISendAnchorHistory(client->client,url);
			case 3:                                  /* ADC ZZZ */
				if (client->status == 3) 
					MCCISendAnchorHistory(client->client,url);
				CCIprotocol_handler_found = 1; 
				break;
			}
		client = (cciStat *) ListNext(listOfSendAnchorTo);
	}
}

void MoCCISendEvent( MCCIPort client, int on)
{
	MCCIPort tmp;

	if (on) {
		ListAddEntry(listOfSendEvent,(char*)client);
	} else {
		ListDeleteEntry(listOfSendEvent,(char*)client);

		/* turn off cci_event only if list of clients is NULL */
		if (!(tmp = (MCCIPort) ListHead(listOfSendEvent)))
			cci_event = 0;
	}
}

void MoCCISendMouseAnchor( MCCIPort client, int on)
{
	MCCIPort tmp;

	if (on) {
		ListAddEntry(listOfSendEvent,(char*)client);
	} else {
		ListDeleteEntry(listOfSendEvent,(char*)client);

		/* turn off cci_event only if list of clients is NULL */
		if (!(tmp = (MCCIPort) ListHead(listOfSendEvent)))
			cci_event = 0;
	}
}

void MoCCISendBrowserView( MCCIPort client, int on)
{
	if (on) {
		ListAddEntry(listOfSendBrowserView,(char*)client);
	} else {
		ListDeleteEntry(listOfSendBrowserView,(char*)client);
	}
}

void MoCCIForm(MCCIPort client, char *actionID, int status, int close_connection)
{
	struct FormSubmit *sendForm;
	int registered = 0;

	if (status){
		sendForm = (struct FormSubmit *) ListHead(listOfForm);
		while (sendForm){
			if ((sendForm->client == client) &&
			   (!strcmp(sendForm->actionID,actionID))) {
				sendForm->valid = 1;
				registered = 1;
		   	}
			sendForm = (struct FormSubmit *)ListNext(listOfForm);
		}

		/* only register if not done so alreday */
		if (!registered){
			if (!(sendForm = (struct FormSubmit *)
				malloc(sizeof(struct FormSubmit)))) {
				return; /* mem problem */
			}

			sendForm->client = client;
			sendForm->actionID = strdup(actionID);
			sendForm->valid = 1;
			ListAddEntry(listOfForm, (char*)sendForm);
		}
	} else if (close_connection) {
		sendForm = (struct FormSubmit *) ListHead(listOfForm);
		while(sendForm){
			if (sendForm->client == client) {
				ListDeleteEntry(listOfForm, (char*)sendForm);
				free(sendForm->actionID);
				free(sendForm);
				sendForm = (struct FormSubmit *)
					ListCurrent(listOfForm);
#if 0
				sendForm->valid = 0; 
#endif
			} else{
				sendForm = (struct FormSubmit *)
					ListNext(listOfForm);
			}
		}
	} else{ /* remove entry from list, forget the valid bit, delete it!! */
		sendForm = (struct FormSubmit *) ListHead(listOfForm);
		while(sendForm){
			if ((sendForm->client == client) &&
			   (!strcmp(sendForm->actionID,actionID))){
				ListDeleteEntry(listOfForm, (char*)sendForm);
				free(sendForm->actionID);
				free(sendForm);
				sendForm = (struct FormSubmit *)
					ListCurrent(listOfForm);
#if 0
				sendForm->valid = 0;
#endif
			} else{
				sendForm = (struct FormSubmit *)
					ListNext(listOfForm);
			}
		}
	}
	if (mMosaicCCITrace) {
		sendForm = (struct FormSubmit *) ListHead(listOfForm);
		fprintf(stderr,"***** begin mosaic list *****\n");
		while(sendForm){
			fprintf(stderr,"actionID 	%s\n",sendForm->actionID);
			fprintf(stderr,"clientaddress %s\n",sendForm->client->serverAddress);
			sendForm = (struct FormSubmit *) ListNext(listOfForm);
		}
	}
}

void MoCCIPreInitialize()
{
	static int donePreInit = 0;

	if (donePreInit)
		return ;
	donePreInit = 1;

	listOfConnections = ListCreate();
	listOfSendOutput = ListCreate();
	listOfSendAnchorTo = ListCreate();
	listOfSendBrowserView = ListCreate();
	listOfSendEvent = ListCreate();
	listOfFileToURLs = ListCreate();
	listOfForm = ListCreate();
}

int MoCCIInitialize(int portNumber)
{
	int retVal;
    	FILE *fp;
	char * fnam;

	MoCCIPreInitialize();
	retVal = MCCIServerInitialize(portNumber);
	if (retVal) {		/* Write port number to .mosaiccciport */
    		fnam = (char *)malloc (strlen (mMosaicRootDirName) + 32);
    		sprintf (fnam, "%s/.mosaiccciport", mMosaicRootDirName);
    		fp = fopen (fnam, "w");
    		if (fp) {
			fprintf(fp,"%s:%d\n",mMosaicMachineWithDomain,portNumber);
        		fclose (fp);
      		}
    		free (fnam);
  	}
	return(retVal);
}

void MoCCITerminateAllConnections(void)
{
	struct Connection *con;
	struct SendWhatToWhom *sendOutput;

	con = (struct Connection *) ListHead(listOfConnections);
	while(con) {
		XtRemoveInput(con->inputId);
		NetCloseConnection(con->client);
		MoCCISendAnchor(con->client,0);
		MoCCISendBrowserView(con->client,0);
		MoCCISendEvent(con->client, 0);
		MoCCIForm(con->client, NULL, 0, 1);

		sendOutput=(struct SendWhatToWhom *)ListHead(listOfSendOutput);
		while(sendOutput){          /* remove sendOutputs */
			HTRemoveConversion(sendOutput->contentType,
				     "www/present",CCIPresent);
			ListDeleteEntry(listOfSendOutput,(char*)sendOutput);
			free(sendOutput->contentType);
			free(sendOutput);
		  
			sendOutput = (struct SendWhatToWhom *)
				ListCurrent(listOfSendOutput);
		}
		ListDeleteEntry(listOfConnections,(char*)con);
		con = (struct Connection *) ListHead(listOfConnections);
	}
}

void MoCCITerminateAConnection(MCCIPort client)
{
	struct Connection *con;
	struct SendWhatToWhom *sendOutput;

	con = (struct Connection *) ListHead(listOfConnections);
	while(con) {
		if (con->client == client) {
			XtRemoveInput(con->inputId);
			MoCCISendAnchor(con->client,0);
			MoCCISendBrowserView(con->client,0);
			MoCCISendEvent(con->client, 0);
			MoCCIForm(con->client, NULL, 0, 1);

			sendOutput=(struct SendWhatToWhom *)
			  	ListHead(listOfSendOutput);
			while(sendOutput){          /* remove sendOutputs */
			  	if(sendOutput->client == client){
			    		HTRemoveConversion(
						sendOutput->contentType,
					       "www/present",CCIPresent);
			    		ListDeleteEntry(listOfSendOutput,
						(char*)sendOutput);
			    		free(sendOutput->contentType);
			    		free(sendOutput);
			  	}
			  	sendOutput = (struct SendWhatToWhom *)
			    	ListNext(listOfSendOutput);
			}
			ListDeleteEntry(listOfConnections,(char*)con);
			con = (struct Connection *)
					ListCurrent(listOfConnections);
		} else {
			con = (struct Connection *) ListNext(listOfConnections);
		}
	}
}

void MoCCIHandleInput(MCCIPort client, int source)
{
	if (MCCIIsThereInput(client))
		MCCIHandleInput(client);
}

void MoCCINewConnection(XtPointer clid, int *source, XtInputId *inputID)
{
	MCCIPort client;
	struct Connection *con;

	if (mMosaicCCITrace) {
		fprintf(stderr,"MoCCINewConnection(): I've been called\n");
	}
	client = MCCICheckAndAcceptConnection();
	if (!client)  /* nothing here */
		return;
	if (mMosaicCCITrace) {
		fprintf(stderr,"Mosaic: got a CCI connection\n");
	}
/*	
	this determines wether there is one or many cci clients connecting 
	XtRemoveInput(*inputID);
*/

	MCCISendResponseLine(client,MCCIR_OK,"VERSION 01 mMosaic 3.2.3");

	con = (struct Connection *) malloc(sizeof(struct Connection));
	con->client = client;
	con->inputId = XtAppAddInput(mMosaicAppContext,
		MCCIGetSocketDescriptor(client),
		(XtPointer)  XtInputReadMask,
		(XtInputCallbackProc) MoCCIHandleInput, (XtPointer) client);
	ListAddEntry(listOfConnections,(char*)con);
}

static void MoCCIWindowCallBackDoit(Boolean newState, mo_window * win);
static XmxCallback (MoCCIWindowCallBack0) /*ok*/
{
	mo_window *win = (mo_window *)client_data;
	Boolean newState;

	newState = cciAccepting;
  	newState = XmToggleButtonGetState(win->cci_accept_toggle);
	XtUnmanageChild(win->cci_win);
	MoCCIWindowCallBackDoit(newState,  win);
}
static XmxCallback (MoCCIWindowCallBack1)  /*Cancel*/
{
	mo_window *win = (mo_window *)client_data;
	Boolean newState;

	newState = cciAccepting;
	if (cciAccepting != XmToggleButtonGetState (win->cci_accept_toggle)) {
		/* reset toggle state */
		XmxSetToggleButton(win->cci_accept_toggle, cciAccepting);
		XmxSetToggleButton(win->cci_off_toggle, !cciAccepting);
	}
	XtUnmanageChild(win->cci_win);
	MoCCIWindowCallBackDoit(newState,  win);
}
static XmxCallback (MoCCIWindowCallBack2)  /*Help*/
{
	mo_window *win = (mo_window *)client_data;
	Boolean newState;

	newState = cciAccepting;
	mo_open_another_window (win,
		        mo_assemble_help_url("docview-menubar-file.html#open"),
			NULL, NULL);

	MoCCIWindowCallBackDoit(newState, win);
}
static void MoCCIWindowCallBackDoit(Boolean newState, mo_window * win)
{
	int newPort;
	char *portString;
	char buff[256];

	if (newState != cciAccepting) {	/* toggle state has changed */
		cciAccepting = newState;
		if (cciAccepting) {
			newPort = 0;
			portString= XmTextGetString (win->cci_win_text);
			if (portString)
				newPort = atoi(portString);
			if ((newPort < 1024) || (newPort > 65535)) {
				XmxMakeErrorDialog (win->cci_win,
					"CCI port address must be between 1024 and 65535" ,
					"CCI port address error" );
				XtManageChild (Xmx_w);
				cciAccepting = False;
				listenPortNumber = 0;
				XmxSetToggleButton(win->cci_accept_toggle, 
					cciAccepting);
				XmxSetToggleButton(win->cci_off_toggle, 
					!cciAccepting);
				return;
			}
			if (MoCCIInitialize(newPort)) {
				listenPortNumber = newPort;
				connectInputID = XtAppAddInput(mMosaicAppContext,
					MCCIReturnListenPortSocketDescriptor(),
					(XtPointer) XtInputReadMask,
					(XtInputCallbackProc) MoCCINewConnection,
					NULL);
				sprintf(buff,"%s %d","CCI Now listening on port" ,newPort);
				XmxMakeInfoDialog(win->cci_win,buff,"CCI port status" );
				XtManageChild(Xmx_w);
			} else {
				cciAccepting = False;
				listenPortNumber = 0;
				XmxSetToggleButton(win->cci_accept_toggle, 
					cciAccepting);
				XmxSetToggleButton(win->cci_off_toggle, 
					!cciAccepting);
			/* Can't accept on this port...*/
				XmxMakeErrorDialog (win->cci_win,
					"CCI Can't accept requests on this port. May be in use already." ,
					"CCI port address error" );
				XtManageChild(Xmx_w);
			}
		} else {
			XtRemoveInput(connectInputID); /* stop accepting connections*/
			MCCICloseAcceptPort(); /* kill accept port */

/* terminate existing connections */
			MoCCITerminateAllConnections();
			listenPortNumber = 0;
			sprintf(buff,"CCI Is no longer listening");
			XmxMakeInfoDialog(win->cci_win, buff,"CCI port status");
			XtManageChild(Xmx_w);
		}
	}
}

mo_status MoDisplayCCIWindow( mo_window *win)
{
	Widget dialogFrame;
	Widget dialogSeparator;
	Widget cciForm,label;
	Widget toggleBox;
	Widget buttonBox;

	if (!win->cci_win) {
		win->cci_win = XmxMakeFormDialog(win->base, 
				 "NCSA Mosaic: Common Client Interface" );
		dialogFrame = XmxMakeFrame(win->cci_win, XmxShadowOut);
		XmxSetConstraints (dialogFrame, XmATTACH_FORM, XmATTACH_FORM,
		         XmATTACH_FORM, XmATTACH_FORM, NULL, NULL, NULL, NULL);
		/* Main form. */
		cciForm = XmxMakeForm(dialogFrame);

		label = XmxMakeLabel(cciForm, "CCI Port Address: " );
		XmxSetArg(XmNcolumns, 25);
		win->cci_win_text= XmxMakeText (cciForm);
		XtAddCallback(win->cci_win_text, XmNactivateCallback,
			MoCCIWindowCallBack0,(XtPointer)win);
		toggleBox = XmxMakeRadioBox(cciForm);
		win->cci_accept_toggle = XmxMakeToggleButton(toggleBox, 
				"Accept requests" , NULL, (XtPointer)win);

		win->cci_off_toggle = XmxMakeToggleButton(toggleBox, 
				"Interface off" , NULL, (XtPointer)win);
		dialogSeparator= XmxMakeHorizontalSeparator(cciForm);
		buttonBox = XmxMakeFormAndThreeButtons(cciForm,
			       "Ok" , "Dismiss" , "Help..." ,
			       MoCCIWindowCallBack0,MoCCIWindowCallBack1,
				MoCCIWindowCallBack2,(XtPointer)win);
		XmxSetOffsets (label, 13, 0, 10, 0);
		XmxSetConstraints (label, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_FORM, XmATTACH_NONE, NULL, NULL, NULL, NULL);

		XmxSetOffsets (win->cci_win_text, 10, 0, 5, 8);
		XmxSetConstraints
		        (win->cci_win_text, XmATTACH_FORM, XmATTACH_NONE, 
			XmATTACH_WIDGET,
			XmATTACH_FORM, NULL, NULL, label, NULL);

		XmxSetConstraints (toggleBox, XmATTACH_WIDGET, 
			XmATTACH_NONE, XmATTACH_WIDGET, XmATTACH_NONE,
			win->cci_win_text, NULL, label, NULL);
		XmxSetOffsets (toggleBox, 8, 0, 2, 0);

		XmxSetConstraints (dialogSeparator, XmATTACH_WIDGET, 
			XmATTACH_NONE, XmATTACH_FORM, XmATTACH_FORM,
			toggleBox, NULL, NULL, NULL);

		XmxSetConstraints (buttonBox, XmATTACH_WIDGET, 
			XmATTACH_FORM, XmATTACH_FORM, XmATTACH_FORM,
			dialogSeparator, NULL, NULL, NULL);
	}

	if (cciAccepting) {
		char buff[10];
		sprintf(buff,"%d",listenPortNumber);
		XmxTextSetString(win->cci_win_text,buff);
	} else {
		XmxTextSetString(win->cci_win_text,"");
	}
	XmxSetToggleButton(win->cci_accept_toggle, cciAccepting);
	XmxSetToggleButton(win->cci_off_toggle, !cciAccepting);

	XmxManageRemanage (win->cci_win);
	return mo_succeed;
}

void MoCCISendOutputToClient( char *contentType, /* string name of data type */
	char *fileName)				 /* data stored here */
{
	struct SendWhatToWhom *sendOutput;

	/* send data to all clients that registered for this content type*/
	sendOutput=(struct SendWhatToWhom *)ListHead(listOfSendOutput);
	while(sendOutput){
		if (!strcmp(sendOutput->contentType,contentType)) {
			if (mMosaicCCITrace) {
				fprintf(stderr,"Sending output through cci of type %s\n",
						sendOutput->contentType);
			}

			/* prep data for sending here */

			/* send it back to client */
			MCCISendOutputFile(sendOutput->client,
					contentType,fileName);
		}
		sendOutput=(struct SendWhatToWhom *)ListNext(listOfSendOutput);
	}
}

/* inorder to send content to application, mosaic must call this function
   twice, the first to set the variable send, the second to do the actual
   sending
*/
int MoCCIFormToClient( char *actionID, char *query, char *contentType,
	char *post_data, int status)
{
	static int send = 0;
	struct FormSubmit *sendForm;
	int found = 0;

	if (status && (send == 0)) {	/* just make a note that its a cciPOST */
		send = 1;
		return(0);
	}
	if (send){
	  	send = 0;
	  	sendForm=(struct FormSubmit *) ListHead(listOfForm);
	  	while(sendForm) {		/* found correct client */
			if ( !strcmp(actionID, sendForm->actionID)) {
				if (sendForm->valid)
					MCCIFormQueryToClient(sendForm->client, 
				  		actionID, query, 
						contentType, post_data);
				found = 1;
			}
			sendForm = (struct FormSubmit *)ListNext(listOfForm);
	  	}
	}
	return found;
}
/* this function is a callback from the libwww2 Converter */

void MoCCISendOutput( MCCIPort client, Boolean sendIt, char *contentType)
{
	struct SendWhatToWhom *sendOutput;

	if (sendIt) {
		if (!(sendOutput = (struct SendWhatToWhom *) 
		    malloc(sizeof(struct SendWhatToWhom)))) {
			return /*mem error*/;
		}
		sendOutput->client = client;
		sendOutput->contentType = strdup(contentType);
		ListAddEntry(listOfSendOutput,(char*)sendOutput);

		/* set up call back for this content Type */
		HTSetConversion(contentType,"www/present",CCIPresent,
				1.0, 0.0, 0.0);
/*If we use this, out put a message like....sent to cci */

/**		HTSetPresentation(contentType,MoCCISendOutputCB,
					1.0, 0.0, 0.0);
*/
	} else {
		sendOutput=(struct SendWhatToWhom *)ListHead(listOfSendOutput);
		while(sendOutput){
			if ((sendOutput->client == client) &&
			    (!strcmp(sendOutput->contentType,contentType))) {
				HTRemoveConversion(contentType,
						"www/present",CCIPresent);
				ListDeleteEntry(listOfSendOutput,(char*)sendOutput);
				free(sendOutput->contentType);
				free(sendOutput);

				sendOutput = (struct SendWhatToWhom *)
						ListCurrent(listOfSendOutput);
			} else {
				sendOutput = (struct SendWhatToWhom *)
						ListNext(listOfSendOutput);
			}
		}
	}
}

void MoCCIStartListening( Widget w, int port)
{
/*int listenPort;*/

        if (MoCCIInitialize(port)) {
		connectInputID = XtAppAddInput(mMosaicAppContext,
                        MCCIReturnListenPortSocketDescriptor(),
                        (XtPointer) XtInputReadMask,
                        (XtInputCallbackProc) MoCCINewConnection,
			NULL);
		cciAccepting = True; 
		listenPortNumber = port;
        } else {
		char buf[80];
		sprintf(buf,"%s %d","Could not listen on port" ,port);
		XmxMakeErrorDialog(w,buf, "CCI Error" );
		XtManageChild(Xmx_w);
	}
}

/* Handle SendEventOutput if needed */
/* send Event data to all the clients that have requested it */
void MoCCISendEventOutput( CCI_events event_type)
{
	MCCIPort client;

	client = (MCCIPort) ListHead(listOfSendEvent);
	while (client) {
		MCCISendEventOutput(client,event_type);
		client = (MCCIPort) ListNext(listOfSendEvent);
	}
}

/* Handle SendMouseAnchorOutput if needed */
/* send MouseAnchor data to all the clients that have requested it */
void MoCCISendMouseAnchorOutput( char *anchor)
{
	MCCIPort client;

	client = (MCCIPort) ListHead(listOfSendEvent);
	while (client) {
		MCCISendMouseAnchorOutput(client,anchor);
		client = (MCCIPort) ListNext(listOfSendEvent);
	}
}

/* Handle SendBrowserViewOutput if needed */
/* send BrowserView data to all the clients that have requested it */
void MoCCISendBrowserViewOutput(char *url, char *contentType, char *data, int  dataLength)
{
	MCCIPort client;

	if (dataLength == 0)
		return;
	if (!data)
		return;
	if ((!url) ||  (!strlen(url)))
		return;
	if ((!contentType) ||  (!strlen(contentType)))
		contentType = "unknown";
	client = (MCCIPort) ListHead(listOfSendBrowserView);
	while (client) {
		MCCISendBrowserViewOutput(client,url,contentType,
					data,dataLength);
		client = (MCCIPort) ListNext(listOfSendBrowserView);
	}
}

int MoCCISendBrowserViewFile( char *url, char *contentType, char *filename)
{
	struct stat fileInfo;
	FILE *fp;
	char *data;

	if (!ListHead(listOfSendBrowserView))
		return(MCCI_OK);
	if ((!filename) || (!strlen(filename)))
		return(MCCI_OK);
	if ((!url) || (!strlen(url)))
		return(MCCI_OK);
	if ((!contentType) || (!strlen(contentType)))
		contentType = "unknown";

        if (stat(filename,&fileInfo)) /* get the length of the file */
                return(MCCI_FAIL);
	if (!(fp = fopen(filename,"r")))
                return(MCCI_FAIL);
	if (!(data = (char *) malloc(fileInfo.st_size))) {
		fclose(fp);
                return(MCCI_OUTOFMEMORY);
	}
	if (fileInfo.st_size != fread(data,sizeof(char),fileInfo.st_size,fp)){
		fclose(fp);
		free(data);
                return(MCCI_FAIL);
	}
	MoCCISendBrowserViewOutput(url, contentType, data, fileInfo.st_size);
	free(data);
	fclose(fp);
	return(MCCI_OK);
}

int MoCCICurrentNumberOfConnections()
{
    return(ListCount(listOfConnections));
}

/* this routine should be called each time a url has been down loaded
   and stored as file.  This routine adds the fileName,url pair to the
   list for later query over cci by an external viewer (cci app).
*/
void MoCCIAddFileURLToList( char *fileName, char *url)
{
struct FileURL *fileURL;
	
	if (mMosaicCCITrace) {
		fprintf(stderr,"MoCCIAddFileURLToList():fileName=\"%s\", url=\"%s\"\n",(fileName?fileName:"NULL"),(url?url:"NULL"));
	}
	if ((!fileName) || (!url))
		return;

	if (!(fileURL = (struct FileURL *) malloc(sizeof (struct FileURL))))
		/* out of memory, just return */
		return;
	fileURL->fileName = strdup(fileName);
	fileURL->url = strdup(url);
	ListAddEntry(listOfFileToURLs,(char*)fileURL);
	return;
}

/* given filename, return associated URL in the list */
/* if not found, return NULL */
char *MoReturnURLFromFileName( char *fileName)
{
	struct FileURL *fileURL;

	fileURL = (struct FileURL *) ListHead(listOfFileToURLs);
	while (fileURL){ 
		if (!strncmp(fileURL->fileName,fileName,
			     strlen(fileURL->fileName))) {
			return(fileURL->url);
		}
		fileURL = (struct FileURL *) ListNext(listOfFileToURLs);
	}
	return(NULL);
}
/* this rplaces a vanilla url name with the name it was actually called with,
   which is what is needed by external apps.  It is indeed a huge hack, and 
   should be fixed
*/
void MoCCIAddAnchorToURL(char *url, char *urlAndAnchor)
{
	struct FileURL *fileURL;

	fileURL = (struct FileURL *) ListTail(listOfFileToURLs);
	if (fileURL && !strcmp(fileURL->url, url)) {
		free(fileURL->url);
		fileURL->url = strdup(urlAndAnchor);
	}
}
