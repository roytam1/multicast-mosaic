/* Some of code came from libwww2/HTTCP.c library. Don't know which
 * Copyright apply... */

/* rewrote by G.Dauphin 11 Oct 1997 */

/* Generic Unicast Connexion code. IPV4 and IPV6 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/time.h>
#include <assert.h>


/* From seanm@storm.ca  Fri Dec  8 04:44:43 2000 */
#if defined(SVR4) && !defined(SCO)
#include <sys/filio.h>
#include <unistd.h>
#endif

#if defined(linux)
#include <sys/ioctl.h>	/* mjr 12/Jan/1998 : for FIONBIO FNDELAY */
#include <sys/fcntl.h>
#include <stdlib.h>
#endif

#if defined(NETBSD) || defined(FreeBSD)
#include <sys/filio.h>
#endif

#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif

extern int errno;

#include "../libnut/mosaic-types.h"
#include "../libnut/system.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mime.h"
#include "paf.h"
#include "http-proto.h"
#include "file-proto.h"
#include "ftp-proto.h"
#include "URLParse.h"
#include "proxy.h"
#include "cache.h"
#include "www-con.h"

#ifdef DEBUG
#define DEBUG_HTTP
#endif

#ifdef IPV6
enum InIpV cipv = IN_IPV_UNKNOWN;
#endif

static void doc_connect_succes(PafDocDataStruct * pafd);

/*	Produce a string for an Internet address
**
** On exit,
**	returns	a pointer to a static string which must be copied if
**		it is to be kept.
*/

/*static const char * HTInetString (SockA *sin) */

#ifdef IPV6
static const char * HTInetString (void *s, enum InIpV type)
#else
static const char * HTInetString (SockA4 *sin)
#endif
{
    static char string[512];
#ifdef IPV6
	switch (type) {
	case IN_IPV_6 : {
		SockA6 *sin = (SockA6 *) s;

    		sprintf(string, "%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x.%x",
	    		(int)*((unsigned char *)(&sin->sin6_addr)+0),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+1),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+2),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+3),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+4),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+5),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+6),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+7),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+8),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+9),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+10),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+11),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+12),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+13),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+14),
	    		(int)*((unsigned char *)(&sin->sin6_addr)+15));
		}
		break;
	case IN_IPV_4 : {
		SockA4 *sin = (SockA4 *)s;

    		sprintf(string, "%d.%d.%d.%d",
	    		(int)*((unsigned char *)(&sin->sin_addr)+0),
	    		(int)*((unsigned char *)(&sin->sin_addr)+1),
	    		(int)*((unsigned char *)(&sin->sin_addr)+2),
	    		(int)*((unsigned char *)(&sin->sin_addr)+3));
		}
		break;
	default:
		assert(0);
	}
#else
    	sprintf(string, "%d.%d.%d.%d",
	    (int)*((unsigned char *)(&sin->sin_addr)+0),
	    (int)*((unsigned char *)(&sin->sin_addr)+1),
	    (int)*((unsigned char *)(&sin->sin_addr)+2),
	    (int)*((unsigned char *)(&sin->sin_addr)+3));
#endif
    	return string;
}


/*	Parse a network node address and port
**
** On entry,
**	str	points to a string with a node name or number,
**		with optional trailing colon and port number.
**
** On exit,
**	*sin4	is filled in. If no port is specified in str, that
**		field is left unchanged in *sin.
**	*sin6	is filled in. If no port is specified in str, that
**		field is left unchanged in *sin.
** return:
**	0	IN_IPV_UNKNOWN	(or fail)
**	1	IN_IPV_4	(or good)
**	2	IN_IPV_6
*/
#ifdef IPV6
static enum InIpV HTParseInet (SockA4 *sin4, SockA6 *sin6, const char *str)
#else
static enum InIpV HTParseInet (SockA4 *sin4, const char *str)
#endif
{
	char *port;
	char host[512];
	struct hostent  *phost;	/* Pointer to host - See netdb.h */
	int numeric_addr;
	char *tmp;
	enum InIpV cip_v;
  
#ifdef IPV6
	static char *cached6_host = NULL;
	static char *cached6_phost_h_addr = NULL;
	static int cached6_phost_h_length = 0;
	char * ph6addr, *ph6cl;
	int err_num;
#endif
	static char *cached4_host = NULL;
	static char *cached4_phost_h_addr = NULL;
	static int cached4_phost_h_length = 0;

	strcpy(host, str);		/* Take a copy we can mutilate */

	cip_v = IN_IPV_UNKNOWN;
  
#ifdef IPV6
	ph6addr = host;
	if (host[0] == '[' ) {	/* numeric addr ipv6 RFC2732 */
		int status;

		ph6addr++;
		ph6cl = strchr(ph6addr, ']');
		if (!ph6cl) {
			return IN_IPV_UNKNOWN;
		}
		*ph6cl = '\0';
		ph6cl++;		/* point on optional :port */
		if (*ph6cl == ':') {		/* ':' is here */
			ph6cl++;	/* point to number */
			if (*ph6cl>='0' && *ph6cl<='9') {
                        	sin6->sin6_port = (u_int16_t)htons(atol(ph6cl));
                	}
		}
		status = inet_pton(AF_INET6, ph6addr, &sin6->sin6_addr);
		if (status == 1) {
			cached6_host = NULL;
			cached6_phost_h_addr = NULL;
			return IN_IPV_6;
		}
		return IN_IPV_UNKNOWN;
        }
#endif
/* Parse port number if present */    
	port = strchr(host, ':');
	if (port) {
		*port++ = 0;		/* Chop off port */
		if (port[0]>='0' && port[0]<='9') {
#ifdef IPV6
			sin6->sin6_port = (u_int16_t)htons(atol(port));
			sin4->sin_port = (u_int16_t)htons(atol(port));
#else
			sin4->sin_port = (u_int16_t)htons(atol(port));
#endif
		}
	}

#ifdef IPV6
	if (cached6_host && (strcmp (cached6_host, host) == 0)) {
		memcpy(&sin6->sin6_addr,
			cached6_phost_h_addr, cached6_phost_h_length);
		return IN_IPV_6 ; /* OK */
	}
	if (cached4_host && (strcmp (cached4_host, host) == 0)) {
		memcpy(&sin4->sin_addr,
			cached4_phost_h_addr, cached4_phost_h_length);
		return IN_IPV_4 ; /* OK */
	}
/* manage only non numeric adresses for IPV6 */
/* RFC 2133 */
/*	phost = gethostbyname2 (host,AF_INET6); */
	phost = getipnodebyname( host, AF_INET6, AI_DEFAULT, &err_num);

	if (phost) {			/* try IPV6 */
#ifdef DEBUG_IPV6
		fprintf(stderr, "Talking to IPv6 host...\n");
#endif
		if(phost->h_length != 16 ){	
#ifdef DEBUG_IPV6
			fprintf(stderr,"IPv6 has h_lenght = %d. Returning ...\n",
				phost->h_length);
			assert(phost->h_length == 16);
#endif
			return IN_IPV_UNKNOWN; /* Fail */
		}
		cip_v = IN_IPV_6;
/* Cache new stuff. */
		if (cached6_host) free (cached6_host);
		if (cached6_phost_h_addr) free (cached6_phost_h_addr);

		cached6_host = strdup (host);
		cached6_phost_h_addr = (char*)calloc (phost->h_length + 1, 1);
		memcpy (cached6_phost_h_addr, phost->h_addr, phost->h_length);
		cached6_phost_h_length = phost->h_length;
		memcpy(&sin6->sin6_addr, phost->h_addr, phost->h_length);
		return IN_IPV_6 ; /* OK */
	} else {				/* try ipv4 */
#ifdef DEBUG_IPV6
		fprintf(stderr, "Trying IPV4 host...\n");
#endif
/* Parse host number if present. */  
		numeric_addr = 1;
		for (tmp = host; *tmp; tmp++) { /* If there's a non-numeric... */
			if ((*tmp < '0' || *tmp > '9') && *tmp != '.') {
				numeric_addr = 0;
				break;
			}
		}
		cip_v = IN_IPV_4;
 		if (numeric_addr) {   /* Numeric node address: */
			sin4->sin_addr.s_addr = inet_addr(host); /* See arpa/inet.h */
		} else {		    /* Alphanumeric node name: */
			if (cached4_host && (strcmp (cached4_host, host) == 0)){
				memcpy(&sin4->sin_addr, cached4_phost_h_addr, cached4_phost_h_length);
			} else {
/*				phost = gethostbyname2 (host,AF_INET); */
/*				phost = gethostbyname (host); */
				phost = getipnodebyname( host, AF_INET6, AI_DEFAULT, &err_num);
				if (!phost) {
					fprintf (stderr, "Can't find internet node name `%s'.\n",host);
					return IN_IPV_UNKNOWN;  /* Fail */
				}
/* Free previously cached strings. */
				if (cached4_host) {
					free (cached4_host);
					cached4_host=NULL;
				}
				if (cached4_phost_h_addr) {
					free (cached4_phost_h_addr);
					cached4_phost_h_addr=NULL;
				}
/* Cache new stuff. */
				cached4_host = strdup (host);
				cached4_phost_h_addr = (char*)calloc (phost->h_length + 1, 1);
				memcpy (cached4_phost_h_addr, phost->h_addr, phost->h_length);
				cached4_phost_h_length = phost->h_length;
				memcpy(&sin4->sin_addr, phost->h_addr, phost->h_length);
			}
		}
#ifdef DEBUG_HTTP
		fprintf(stderr, "TCP: Parsed address as port %d, IP address %s\n",
			(int)ntohs(sin4->sin_port),
			HTInetString(sin6,IN_IPV_4));
#endif
		return cip_v;	/* OK */
	}
#else		/* NOT compile with IPV6 */

/* Parse host number if present. */  
	numeric_addr = 1;
	for (tmp = host; *tmp; tmp++) { /* If there's a non-numeric... */
		if ((*tmp < '0' || *tmp > '9') && *tmp != '.') {
			numeric_addr = 0;
			break;
		}
	}
	cip_v = IN_IPV_4;
  
	if (numeric_addr) {   /* Numeric node address: */
		sin4->sin_addr.s_addr = inet_addr(host); /* See arpa/inet.h */
	} else {		    /* Alphanumeric node name: */
		if (cached4_host && (strcmp (cached4_host, host) == 0)) {
			memcpy(&sin4->sin_addr, cached4_phost_h_addr, cached4_phost_h_length);
		} else {
			phost = gethostbyname (host);
			if (!phost) {
				fprintf (stderr, "HTParseInet: Can't find internet node name `%s'.\n",host);
				return IN_IPV_UNKNOWN;  /* Fail */
			}
/* Free previously cached strings. */
			if (cached4_host) {
				free (cached4_host);
				cached4_host=NULL;
			}
			if (cached4_phost_h_addr) {
				free (cached4_phost_h_addr);
				cached4_phost_h_addr=NULL;
			}
/* Cache new stuff. */
			cached4_host = strdup (host);
			cached4_phost_h_addr = (char*)calloc (phost->h_length + 1, 1);
			memcpy (cached4_phost_h_addr, phost->h_addr, phost->h_length);
			cached4_phost_h_length = phost->h_length;
			memcpy(&sin4->sin_addr, phost->h_addr, phost->h_length);
		}
	}
#ifdef DEBUG_HTTP
		fprintf(stderr, "TCP: Parsed address as port %d, IP address %s\n",
			(int)ntohs(sin4->sin_port),
			HTInetString(sin6,cip_v));
#endif
#endif /* IPV6 */
	return cip_v;	/* OK */
}

/* ############################## */

/* we try to connect but timeout connection */
/* no need to remove the (cancel) timeout trigger, see Xt */
static void doc_cancel_connect_time_out_cb(XtPointer clid, XtIntervalId * id)
{
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;

/* remove trigger */
	pafd->www_con_type->call_me_on_stop_cb = NULL;
	XtRemoveTimeOut(pafd->loop_connect_time_out_id);
	pafd->loop_connect_time_out_id = (XtIntervalId) 0;	/* sanity */

/* close the unsucces socket */
	close(pafd->www_con_type->prim_fd);
	free(pafd->www_con_type);
	pafd->www_con_type = NULL;
/* call down to up procedure */
	(*pafd->call_me_on_error)(pafd, "Connection Error: Timeout");
}

/* we click on stop during the connection. two trigger is pending
 *	pafd->cancel_connect_time_out_id
 *	pafd->loop_connect_time_out_id
 * top->down procedure
 */
static void MMStopConnectPostRequestAndGetTypedData(PafDocDataStruct * pafd)
{
/* remove trigger */
	XtRemoveTimeOut(pafd->loop_connect_time_out_id);
	pafd->loop_connect_time_out_id = (XtIntervalId) 0;  /* sanity */
	XtRemoveTimeOut(pafd->cancel_connect_time_out_id);

/* close the pending connect socket */
	close(pafd->www_con_type->prim_fd);

	free(pafd->www_con_type);
	pafd->www_con_type = NULL;
}

static void loop_doc_connect_cb(XtPointer clid, XtIntervalId * id)
{
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	int status;
	unsigned long loop_connect_time = 100; /* second and next loop = 100 milli sec */
	struct sockaddr * psin;
	int socksize;

/* Try to connect again to make sure. If we try to connect again, and get
 * EISCONN, it means we have a successful connection.
 * If we don't get EISCONN, something has gone wrong.  Break out and report it.
 * Problem with Linux:
 * mjr: Linux: using the URL http://localhost/, i get status=0 and errno=115
 *  (apparently meaning EINPROGRESS  (Operation now in progress) 
 * gd: Proposal solution: if status is 0, the connect is a success. Because
 *      on a succes , the errno is not significative
 */
	switch(pafd->www_con_type->ipv){
	case IN_IPV_4:
		psin = (struct sockaddr *)&(pafd->www_con_type->sin4);
		socksize = sizeof(SockA4);
		break;
#ifdef IPV6
	case IN_IPV_6:
		psin = (struct sockaddr *)&(pafd->www_con_type->sin6);
		socksize = sizeof(SockA6);
		break;
#endif
	default:
		assert(0);
	}
/*	status =connect(pafd->www_con_type->prim_fd,
 *		(struct sockaddr*)&(pafd->www_con_type->sin), sizeof(SockA));
*/
	status =connect(pafd->www_con_type->prim_fd, psin, socksize);

	if ( (status == 0) || ((status < 0) && (errno == EISCONN))) { /* succes in connect */
		XtRemoveTimeOut(pafd->cancel_connect_time_out_id);
		pafd->loop_connect_time_out_id = (XtIntervalId) 0;  /* sanity */
		doc_connect_succes(pafd);
		return;
	}
/* If we get EALREADY: The socket is non-blocking and a previous connection
 * attempt has not yet been completed. Wait 100 milli-sec.
 * For some reason (bug:-( ) Solaris return EAGAIN some time. This is not
 * conform to the connect(3n) manual page of Solaris 2.5. Grrrrr.
 * We can loop until the cancel time out trigger.
 */
	if ( (status < 0) && (errno == EALREADY || errno == EAGAIN) ) {
		pafd->loop_connect_time_out_id =XtAppAddTimeOut(mMosaicAppContext,
			loop_connect_time, loop_doc_connect_cb, (XtPointer) pafd);
		return;
	}
/* others cases are error */
/* clear the connection and abort and popdown */
	pafd->www_con_type->call_me_on_stop_cb = NULL;
	close(pafd->www_con_type->prim_fd);
	free(pafd->www_con_type);
	pafd->www_con_type = NULL;
	XtRemoveTimeOut(pafd->cancel_connect_time_out_id);
	pafd->loop_connect_time_out_id = (XtIntervalId) 0;  /* sanity */
	(*pafd->call_me_on_error)(pafd,"Connection Error: Can't connect");
	return;
}

/* somethings goes wrong with linux. Il faut peut-etre bloquer la socket ICI */
/*         status = ioctl(pafd->www_con_type->prim_fd, FIONBIO, &zero); */
/* a voir: ####### */
static void doc_connect_succes(PafDocDataStruct * pafd)
{
	int status;
	int zero = 0;
	char * command = NULL;
	int lenc = 0;		/* len of command to send */

/* connect is a succes. No trigger is active. */

/* Make the socket blocking again on good connect */
	status = ioctl(pafd->www_con_type->prim_fd, FIONBIO, &zero);
	if (status == -1) {	/* for developpers */
		fprintf (stderr, "%s.%d:Could not restore socket to blocking.\n",
			__FILE__,__LINE__);
	}

/* send the command. Yes, We are able to do that because we are in
 * a select with writemask...   process per connexion type */
	XmxAdjustLabelText(pafd->win->tracker_widget, "Sending Request...");
	XFlush(mMosaicDisplay);

	switch ( pafd->con_type) {
	case HTTP_CON_TYPE:
/* ################# put and post */
/* int do_head = 0; int do_put = 0; int do_meta = 0;
 * FILE *put_fp;
 * if (do_put) {       
 * while (status>0) {     
 * n=fread(buf,1,BUFSIZ-1,put_fp);
 * upcnt+= status = write(s, buf, n);
 * if (feof(put_fp)) {  break; } }
 * if (status<0 || !feof(put_fp) || upcnt!=put_file_size) {
 * fprintf(stderr,"server does not supportthe PUT method, or error upload");
 * } * }
 * ############################################### */
		command = BuildDocHTTPCommand(pafd, &lenc);
/* write a command depending of protocol. Send the command to server */
		status = write(pafd->www_con_type->prim_fd, command, lenc);

#ifdef DEBUG_HTTP
               	fprintf (stderr, "Writing:\n%s--------\n", command);
#endif
		free(command);
 
		if (status != lenc) { /* something goes wrong */
			pafd->www_con_type->call_me_on_stop_cb = NULL;
			close(pafd->www_con_type->prim_fd);
			free(pafd->www_con_type);
			pafd->www_con_type = NULL;
			(*pafd->call_me_on_error)(pafd,"Network write error");
			return;
		}

/* read the response and process per protocol */
		pafd->www_con_type->call_me_on_stop_cb = MMStopHTTPReadDoc;
		pafd->www_con_type->call_me_on_error_cb = MMCancelHTTPReadDocOnError;
		pafd->iobs.iobuf = (char*) malloc(8192);
		pafd->iobs.size_iobuf = 8192;
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat =0;
		pafd->www_prim_fd_read_id = XtAppAddInput(mMosaicAppContext,
			pafd->www_con_type->prim_fd, (XtPointer) XtInputReadMask,
			read_http_doc_prim_fd_cb, (XtPointer) pafd);
		break;
	case FTP_CON_TYPE:
		pafd->www_con_type->call_me_on_stop_cb =MMCancelFTPReadDocOnStop;
		pafd->www_con_type->call_me_on_error_cb = MMCancelFTPReadDocOnError;
		pafd->iobs.iobuf = (char*) malloc(8192);
		pafd->iobs.size_iobuf = 8192;
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat =0;
		pafd->www_con_type->second_fd = -1;
		pafd->www_con_type->third_fd = -1;
		pafd->www_prim_fd_read_id = XtAppAddInput(mMosaicAppContext,
                        pafd->www_con_type->prim_fd, (XtPointer) XtInputReadMask,
                        read_ftp_doc_prim_fd_cb, (XtPointer) pafd);
		break;
	case NNTP_CON_TYPE:
		break;
	case GOPHER_CON_TYPE:
		break;
	}
}

void PostRequestAndGetTypedData( char * aurl, PafDocDataStruct * pafd)
{
	SockA4 sin4;
#ifdef IPV6
	SockA6 sin6;
#endif
	char * access_part;
	int def_port = 0;
	int con_type = FILE_CON_TYPE;
	char * hap = NULL;	/* Host And Port */
	enum InIpV cipv;
	int status;
	int one = 1;
	int soc= -1;		/* the socket */
	unsigned long cancel_time = 60 * 1000; /* 20 seconds for connect */
	unsigned long loop_connect_time = 1;    /* first loop = 1 milli sec */
	char * ptr;
	struct sockaddr * psin;
	int socksize;

	memset(&sin4, 0, sizeof(SockA4));
#ifdef IPV6
	memset(&sin6, 0, sizeof(SockA6));
#endif
	access_part = "file";
	if( !strncmp(aurl,"http",4) ){
		def_port = 80;
		con_type = HTTP_CON_TYPE;
		access_part = "http";
	} else if( !strncmp(aurl,"file",4) ){
		con_type = FILE_CON_TYPE;
		access_part = "file";
	} else if( !strncmp(aurl,"ftp",3) ){
		def_port = 21;
		con_type = FTP_CON_TYPE;
		access_part = "ftp";
	} else if( !strncmp(aurl,"news",4) || !strncmp(aurl,"nntp",4) ){
		def_port = 119;
		con_type = NNTP_CON_TYPE;
		access_part = "nntp";
	} else if( !strncmp(aurl,"gopher",6) ){
		def_port = 70;
		con_type = GOPHER_CON_TYPE;
		access_part = "gopher";
	}
/* remarque : the con_type is at least FILE_CON_TYPE */

	pafd->con_type = con_type;

/* don't look cache if reloading or post */
	if(! (pafd->pragma_no_cache || (pafd->post_ct && pafd->post_data)) ) {
		int found;
/* time to look at cache */
/* aurl_wa : to determine if url is cachable */
/* aurl : the reference to find in cache */
/* fd : the file where to write the data */
/* mhs: to build a mime struct like (return) */
		found = MMCacheFindData( pafd->aurl_wa, pafd->aurl,
				pafd->fd, pafd->mhs);
		if (found) {	/* we have a hit */
			pafd->http_status = HTTP_STATUS_INTERNAL_CACHE_HIT; /*OK*/
/* don't recache a cached data */
			pafd->mhs->cache_control = CACHE_CONTROL_NO_STORE;
			(*pafd->call_me_on_succes)(pafd);
			return;
		}
	}

	pafd->www_con_type = (WWWConType*) calloc(1, sizeof(WWWConType));
	pafd->www_con_type->prim_fd = -1;
	pafd->www_con_type->second_fd = -1;
	pafd->proxent = NULL;

/* if FILE_CON_TYPE :  easy */
	if( con_type == FILE_CON_TYPE) {
		read_file_local(pafd);
		return;
	}

	hap =  URLParse(aurl, "", PARSE_HOST);
	for (ptr=hap; ptr && *ptr; ptr++)
                *ptr=tolower(*ptr);
	if ( !GetNoProxy(access_part, hap)) { /* try to find proxy */
		struct Proxy *proxent = NULL;

		proxent = GetProxy(access_part);
		if (proxent) {
			pafd->proxent = proxent;
			pafd->con_type = HTTP_CON_TYPE;
			def_port = atoi(proxent->port);
			free(hap);
			hap = strdup(proxent->address);
		}
	}

/* Set up defaults network parameter */

#ifdef IPV6
	sin6.sin6_family = AF_INET6;
	sin6.sin6_flowinfo = 0;
	sin6.sin6_port = htons(def_port);
#endif
	sin4.sin_family = AF_INET;
	sin4.sin_port = htons(def_port);

	XmxAdjustLabelText(pafd->win->tracker_widget, "Connecting...");
	XFlush(mMosaicDisplay);

/* Get node name and optional port number: */
#ifdef IPV6
	cipv = HTParseInet(&sin4, &sin6, hap); 
#else
	cipv = HTParseInet(&sin4, hap);	/* return IPV4/6 on succes */
#endif
	switch (cipv){
	case IN_IPV_UNKNOWN: {
			char * info = (char*) malloc(strlen(hap) + 100);

			sprintf(info, "%s.%d:Unable to locate remote host :\n%s",
			__FILE__,__LINE__, hap);
			XmxMakeErrorDialog(pafd->win->base, info, "Net Error");
			free(pafd->www_con_type);
			pafd->www_con_type = NULL;
			(*pafd->call_me_on_error)(pafd,info);
			free(hap);
			free(info);
			return ;
		}
	case IN_IPV_4:
		pafd->www_con_type->ipv = IN_IPV_4;
		soc = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		break;
#ifdef IPV6
	case IN_IPV_6:
		pafd->www_con_type->ipv = IN_IPV_6;
		soc = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		break;
#endif
	}
	free(hap);

/* Now, let's get a socket set up from the server for the data: */      
/* ### remember : implement Keep-Alive or persistent connection on ftp */

	if ( soc < 0 ) {
		XmxMakeErrorDialog(pafd->win->base, "Can't create socket",
			"Net Error");
		free(pafd->www_con_type);
		pafd->www_con_type = NULL;
		(*pafd->call_me_on_error)(pafd,"Can't create socket");
		return ;
	}

/* Make the socket non-blocking, so the connect can be canceled. This means
 * that when we issue the connect we should NOT have to wait for the accept
 * on the other end.
 */
#if defined(linux)
	status = fcntl(soc,F_SETFL,O_NONBLOCK); /* set socket to non-blocking */
#else
	status = ioctl(soc, FIONBIO, &one);
#endif
	if (status == -1) { /* just a warn for developpers */
		fprintf(stderr, "Couldn't make connection non-blocking.");
	}

/* Issue the connect.  Since the server can't do an instantaneous accept
 * and we are non-blocking, this will almost certainly return a negative
 * status. According to the Sun Solaris 2.5 man page for connect:
 * 
 * EINPROGRESS	The socket is non-blocking and the connection cannot be completed
 *		immediately. It is possible to select(3C) for completion by
 *		selecting the socket for writing. However, this is only possible
 *		if the socket STREAMS module is the topmost module on the protocol
 *		stack with a write service procedure.This will be the normal case.
 */
	switch(cipv){
	case IN_IPV_4:
		psin = (struct sockaddr *)&(sin4);
		socksize = sizeof(SockA4);
		pafd->www_con_type->sin4 = sin4;
		break;
#ifdef IPV6
	case IN_IPV_6:
		psin = (struct sockaddr *)&(sin6);
		socksize = sizeof(SockA6);
		pafd->www_con_type->sin6 = sin6; 
		break;
#endif
	default:
		assert(0);
	}

	status= connect(soc, psin, socksize);

/* In most of case the connection is in progress */
/* activate a timeout to cancel connection or warn an error */
/* activate a XtAppAddInput with WriteMask to know if the socket is available */
/* then make a good blocking socket connection */

	pafd->www_con_type->prim_fd = soc;

/* don't forget: we can be interrupted by the stop button */
	pafd->www_con_type->call_me_on_stop_cb = MMStopConnectPostRequestAndGetTypedData;

	pafd->cancel_connect_time_out_id = XtAppAddTimeOut(mMosaicAppContext,
		cancel_time, doc_cancel_connect_time_out_cb, pafd);
	pafd->loop_connect_time_out_id = XtAppAddTimeOut(mMosaicAppContext,
		loop_connect_time, loop_doc_connect_cb, (XtPointer) pafd);

/* let the Mainloop of X working */
/* the next step of work will come on the timeout or the connect ready */
/* in most case the next step is loop_doc_connect_cb */

	if ( status == 0 )
		return;
/* status = -1 */
	if ( (errno == EISCONN) || (errno == EALREADY) || (errno == EAGAIN) ||
	     (errno == EINPROGRESS) )
		return;

/* others cases are error */
/* clear the connection and abort and popdown */
        pafd->www_con_type->call_me_on_stop_cb = NULL;
        close(pafd->www_con_type->prim_fd);
        free(pafd->www_con_type);
        pafd->www_con_type = NULL;
        XtRemoveTimeOut(pafd->cancel_connect_time_out_id);
	XtRemoveTimeOut(pafd->loop_connect_time_out_id);
	pafd->loop_connect_time_out_id = (XtIntervalId) 0;  /* sanity */
        (*pafd->call_me_on_error)(pafd,"Connection Error: Can't connect");
}
