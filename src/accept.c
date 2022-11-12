/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#ifdef linux
#define SCREWY_BLOCKING
#endif

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/ioctl.h>
#include <memory.h>

#ifdef SCREWY_BLOCKING
#include <sys/fcntl.h>
#endif


#ifdef SVR4
# ifndef linux /* mjr++ */
#  include <sys/filio.h>
# endif
#endif

#ifdef __QNX__
#include <sys/select.h>
#endif

#include "port.h"
#include "accept.h"

/* return -1 on error */
ListenAddress NetServerInitSocket(int portNumber)
{
	ListenAddress socketFD;
	struct sockaddr_in serverAddress;
	struct protoent *protocolEntry;

	protocolEntry = getprotobyname("tcp");
	if (protocolEntry)
		socketFD = socket(AF_INET, SOCK_STREAM,protocolEntry->p_proto);
	else
		socketFD = socket(AF_INET, SOCK_STREAM,0);
	
	if (socketFD < 0) {
		fprintf(stderr,"Can't create socket.\n");
		return(-1);
	}
	memset((char *) &serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;
	serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);
	serverAddress.sin_port = htons(portNumber);

	if (bind(socketFD, (struct sockaddr *) &serverAddress, 
	    sizeof(serverAddress))<0){
		fprintf(stderr,"Can't bind to address.\n");
		return(-1);
	}
#ifdef SCREWY_BLOCKING
        fcntl(socketFD,FNDELAY,0); /* set socket to non-blocking for linux */
#endif
	if (listen(socketFD,5) == -1) {
		fprintf(stderr,"Can't listen.\n");
		return(-1);
	}
#ifndef SCREWY_BLOCKING
	ioctl(socketFD,FIONBIO,0); /* set socket to non-blocking */
#endif
	return(socketFD);
}

/* accept a connection off of a base socket. Do not block! */
/* return NULL if no connection else return PortDescriptor*  */
PortDescriptor *NetServerAccept( ListenAddress socketFD)
{
	int newSocketFD;
	struct sockaddr_in clientAddress;
	int clientAddressLength;
	PortDescriptor *c;

	/* it's assumed that the socketFD has already been set to non block*/
	clientAddressLength = sizeof(clientAddress);
	newSocketFD = accept(socketFD,(struct sockaddr *) &clientAddress,
				&clientAddressLength);
	if (newSocketFD < 0)
		return(NULL);
	/* we have connection */
	if (!(c =(PortDescriptor *)malloc(sizeof(PortDescriptor))))
		return(0);
	c->socketFD = newSocketFD;
	c->numInBuffer = 0;
	return(c);
}

/* read input from port, return number of bytes read */
int NetRead(PortDescriptor *c,char *buffer,int bufferSize)
{
	int length;

	length = read(c->socketFD, buffer, bufferSize);
	return(length);
}

/* send buffer, return number of bytes sent */
int NetServerWrite(PortDescriptor *c,char *buffer,int bufferSize)
{
	int length;

	length = write(c->socketFD,buffer,bufferSize);
	return(length);
}

/* close the connection */
void NetCloseConnection(PortDescriptor *c)
{
	close(c->socketFD);
}

void NetCloseAcceptPort(int s)
{
	close(s);
}

/* FD_SETSIZE is define in <sys/select.h> */
#ifndef FD_SETSIZE
#define FD_SETSIZE      32
#endif

/* Do a non block check on socket for input and return 1 for yes, 0 for no */
int NetIsThereInput(PortDescriptor *p)
{
	static struct  timeval timeout = { 0L , 0L };
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(p->socketFD,&readfds);
	if (0 < select(FD_SETSIZE, &readfds, 0, 0, &timeout))
		return(1);
	else
		return(0);
}

/* Do a non block check on socket for input and return 1 for yes, 0 for no */
int NetIsThereAConnection(int socketFD)
{
	static struct  timeval timeout = { 0L , 0L };
	fd_set readfds;

	FD_ZERO(&readfds);
	FD_SET(socketFD,&readfds);
	if (0 < select(FD_SETSIZE, &readfds, 0, 0, &timeout))
		return(1);
	else
		return(0);
}
