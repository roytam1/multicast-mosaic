#ifndef __CCISERVER_H__              
#define __CCISERVER_H__  

extern int MCCIServerInitialize(int portNumber);
extern MCCIPort MCCICheckAndAcceptConnection();
extern int MCCISendResponseLine(MCCIPort client, int code, char *text);
extern int MCCIIsThereInput(MCCIPort client);

int MCCISendAnchorHistory( MCCIPort client, char *url);
int MCCIHandleInput( MCCIPort client);
int MCCIGetSocketDescriptor( MCCIPort clientPort);
int MCCIReturnListenPortSocketDescriptor();
void MCCICloseAcceptPort();
int MCCISendOutputFile( MCCIPort client, char *contentType, char *fileName);
int MCCIFormQueryToClient( MCCIPort client, char *actionID, char *query,
        char *contentType, char *post_data);
int MCCISendEventOutput( MCCIPort client, CCI_events event_type);
int MCCISendMouseAnchorOutput( MCCIPort client, char *anchor);
int MCCISendBrowserViewOutput( MCCIPort client, char *url,
        char *contentType, char *data, int dataLength);

typedef struct {
	MCCIPort client;
	int status;
	char *url;
} cciStat;			/* ejb 03/09/95 */
#endif
