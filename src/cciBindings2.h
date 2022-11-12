/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* 
 * Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */


#ifndef __CCIBINDINGS2_H__
#define __CCIBINDINGS2_H__

#include "cci.h"
#include "cciServer.h"

cciStat *cciStatListFindEntry(MCCIPort findMe);
cciStat *cciStatListDeleteEntry(MCCIPort deleteMe);
void cciStatPreventSendAnchor(MCCIPort client, char *url);
int cciSafeToSend(cciStat *current, char *url);
void cciStatFree(cciStat *i);
void MoCCISendAnchor(MCCIPort client, int sendIt);
void MoCCISendAnchorToCCI(char *url, int beforeAfter);
void MoCCISendEvent(MCCIPort client, int on);
void MoCCISendMouseAnchor(MCCIPort client, int on);
void MoCCISendBrowserView(MCCIPort client, int on);
void MoCCIForm(MCCIPort client, char *actionID, int status, int close_connection);
void MoCCIPreInitialize();
int MoCCIInitialize(int portNumber);
void MoCCITerminateAllConnections(void);
void MoCCIHandleInput(MCCIPort client,int source);
void MoCCINewConnection(XtPointer clid,int *source,XtInputId *inputID); /* mjr--*/
static XmxCallback (MoCCIWindowCallBack);
mo_status MoDisplayCCIWindow(mo_window *win);
void MoCCISendOutputToClient(char *contentType,char *fileName);
int MoCCIFormToClient(char *actionID, char *query, char *contentType, 
char *post_data, int status);
void MoCCISendOutput(MCCIPort client,Boolean sendIt,char *contentType);
void MoCCIStartListening(Widget w,int port);
void MoCCISendEventOutput(CCI_events event_type);
void MoCCISendMouseAnchorOutput(char *anchor);
void MoCCISendBrowserViewOutput(char *url, char *contentType, 
char *data, int dataLength);
int MoCCISendBrowserViewFile(char *url, char *contentType, char *filename);
int MoCCICurrentNumberOfConnections();
void MoCCIAddFileURLToList(char *fileName,char *url);
char *MoReturnURLFromFileName(char *fileName);
void MoCCIAddAnchorToURL(char *url, char *urlAndAnchor);

#endif
