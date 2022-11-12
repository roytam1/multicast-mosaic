/* some part come from libwww2 */
/* FTP protocol */
/*  get the data from a requested url */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/stat.h>
#include <time.h>
#include <assert.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mime.h"
#include "paf.h"
#include "ftp-proto.h"
#include "URLParse.h"
#include "../libnut/system.h"

#ifdef DEBUG
#define DEBUG_FTP
#endif

#define FTP_READ_R_ON_CONNECT	0
#define FTP_READ_R_ON_USER	1
#define FTP_READ_R_ON_PASS	2
#define FTP_READ_R_ON_ACCT	3
#define FTP_READ_R_ON_PORT	4
#define FTP_READ_R_ON_TYPE	5
#define FTP_READ_R_ON_RETR_FILE	6
#define FTP_READ_R_ON_CWD_DIR	7
#define FTP_READING_DATASOC_FILE	8
#define FTP_READ_R_ON_END_OF_FILE 9
#define FTP_READ_R_ON_NLSTLLA_DIR 10
#define FTP_READ_R_ON_NLST_DIR 11
#define FTP_READING_DATASOC_DIR 12
#define FTP_READ_R_ON_END_OF_DIR 13

static void ftp_read_third_fd_file_cb(XtPointer clid, int * fd, XtInputId * id);
static void ftp_read_third_fd_dir_nlstlla_cb(XtPointer clid, int * fd, XtInputId * id);
static void ftp_read_third_fd_dir_nlst_cb(XtPointer clid, int * fd, XtInputId * id);
static int ParseFileSizeAndName(char *szBuffer, char *szFileName, char *szSize);
static int ParseDate(char *szBuffer, char *szFileInfo, char *szMonth, char *szDay, char *szYear, char *szTime);

/*      See the state machine illustrated in RFC959, p57. This implements
**      one command/reply sequence.  It also interprets lines which are to
**      be continued, which are marked with a "-" immediately after the
**      status code.
**      Continuation then goes on until a line with a matching reply code
**      an a space after it.
*/

static int parse_respons (PafDocDataStruct *pafd)
{
	char * buf = pafd->iobs.iobuf;
	int status;
	char * lf_ptr;
	char *deb_from;
	char *deb_to;
	int lcut;
	char * end_from;

/* on scan la reponse:
	- on a DDD texte <LF>
	- ou DDD- texte <LF>
	     DDD texte <LF>
	dans tous les cas il faut regarder pour
	    DDD texte <LF> c'est olbligatoire.
*/

	if (pafd->iobs.len_iobuf < 5){
		return 0; /* not enough data */
	}
	if ( !isdigit(buf[0]) || !isdigit(buf[1]) || !isdigit(buf[2])){
		return -1; /* protocol error */
	}
/* 3 digits code is found */
	if ( buf[3] == ' ') { /* found a Space => last message */
		lf_ptr = strchr(buf, '\012');
		if (lf_ptr == NULL) { /* continue to read */
			return 0;
		}
		sscanf(buf,"%d",&status);
/* shift the iobuf */
		deb_from = lf_ptr +1;
		deb_to = pafd->iobs.iobuf;
		lcut = deb_from  - pafd->iobs.iobuf ;
		end_from = pafd->iobs.iobuf + pafd->iobs.len_iobuf;
		while(deb_from <= end_from) 
			*deb_to++ = *deb_from++;
		pafd->iobs.len_iobuf = pafd->iobs.len_iobuf - lcut;
		return status/100;
	}
	if (buf[3] != '-' ) { /* proto error */
		return -1;
	}
/* Multiline responses start with a number and a hyphen; end with same number
 * and a space.*/              

	sscanf(buf,"%d",&status);
	if (pafd->iobs.iobuf[pafd->iobs.len_iobuf -1] != '\012'){
		return 0; /* continue to read */
	}
	pafd->iobs.iobuf[pafd->iobs.len_iobuf -1]= '\0';
	lf_ptr = strrchr(buf,'\012');
	pafd->iobs.iobuf[pafd->iobs.len_iobuf -1]= '\012';
	if (lf_ptr == NULL) {
		return 0; /* at least 2 LF */
	}
	if ( (lf_ptr[1] == buf[0]) &&
	     (lf_ptr[2] == buf[1]) &&
	     (lf_ptr[3] == buf[2]) &&
	     (lf_ptr[4] == ' ')) {	/* the same status . Goood */
/* shift the iobuf */
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		return status/100;
	}
	return 0;
}

#ifdef IPV6
static const struct in6_addr anyaddr = IN6ADDR_ANY_INIT;
#endif                                 

static int get_listen_socket_and_port_command(char *cmd, PafDocDataStruct *pafd )
{
#ifdef IPV6                            
/* 4.3 BSD ipv6 based */       
	int isv4;                      
	SockA6 soc6_address;
	SockA6 *sin6 = &soc6_address;
	int isv4mapped=0;      
#endif     
	SockA4 soc4_address;		/* Binary network address */
	SockA4 *sin4 = &soc4_address;
	int status;
	struct sockaddr * psin;
	int socksize;
	int s_socksize;
	enum InIpV cipv = IN_IPV_UNKNOWN;
	
 
	assert(pafd->www_con_type->second_fd == -1);
	memset(sin4, 0, sizeof(SockA4));
#ifdef IPV6
	memset(sin6, 0, sizeof(SockA6));
#endif

/* Create internet socket */           
	switch(pafd->www_con_type->ipv){
	case IN_IPV_UNKNOWN:
		assert(0);	/* impossible */
	case IN_IPV_4:
		pafd->www_con_type->second_fd = socket (AF_INET, SOCK_STREAM, IPPROTO_TCP);
		cipv = IN_IPV_4;
        	sin4->sin_family = AF_INET;   /* Family=internet, host order  */
        	sin4->sin_addr.s_addr = INADDR_ANY; /* Any peer address */
		psin = (struct sockaddr *)sin4;
		socksize = sizeof(SockA4);
		break;
#ifdef IPV6                            
	case IN_IPV_6:
		pafd->www_con_type->second_fd = socket (AF_INET6, SOCK_STREAM, IPPROTO_TCP);
		cipv = IN_IPV_6;
        	sin6->sin6_family = AF_INET6;   /* Family=internet, host order */
        	sin6->sin6_addr = anyaddr;      /* Any peer address */
		psin = (struct sockaddr *)sin6;
		socksize = sizeof(SockA6);
		break;
#endif
	default:
		assert(0);      /* impossible */
	}
	if (pafd->www_con_type->second_fd < 0) {
		fprintf(stderr,"%s.%d:socket() < 0. Returning ...\n",
		__FILE__,__LINE__);
		return -1;             
	}

/* Search for a free port. */
	s_socksize = socksize;
	status = getsockname(pafd->www_con_type->prim_fd, psin, &socksize);

	assert(s_socksize == socksize); /* socksize change. Why? */

        if (status<0) {
		fprintf(stderr,"%s.%d:getsockname() < 0. Returning ...\n",
		__FILE__,__LINE__);
		return -1;             
	}                                  
	switch (cipv){
	case IN_IPV_4:
		soc4_address.sin_port = 0; /* Unspecified: please allocate */
		break;
#ifdef IPV6
	case IN_IPV_6:
		soc6_address.sin6_port = 0; /* Unspecified: please allocate */
		break;
#endif
	}
/* Cast to generic sockaddr */
	status=bind(pafd->www_con_type->second_fd, psin, socksize);
	if (status<0) {
		fprintf(stderr,"%s.%d:bind() < 0. Returning ...\n",
		__FILE__,__LINE__);
		return -1;
	}

	s_socksize = socksize;
	status = getsockname(pafd->www_con_type->second_fd, psin, &socksize);

	assert(s_socksize == socksize); /* socksize change. Why? */

	if (status<0) {
		fprintf(stderr,"%s.%d:getsockname() < 0. Returning ...\n",
		__FILE__,__LINE__);
		return -1;
	}

#ifdef IPV6
	if(cipv == IN_IPV_6) {
		isv4mapped = IN6_IS_ADDR_V4MAPPED(&soc6_address.sin6_addr);
/* Now we must find out who we are to tell the other guy */
/* How the other guy see us (V6 or V4 ?)
 * ##### choose ipv4 or ipv6 addr for cmd PORT... BEUUUURK!
 */
		if (isv4mapped){                    /* our V4 addr is 4 last byte */
			sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\015\012",
				(int)*((unsigned char *)(&sin6->sin6_addr)+12),
				(int)*((unsigned char *)(&sin6->sin6_addr)+13),
				(int)*((unsigned char *)(&sin6->sin6_addr)+14),
				(int)*((unsigned char *)(&sin6->sin6_addr)+15),
				(int)*((unsigned char *)(&sin6->sin6_port)+0),
				(int)*((unsigned char *)(&sin6->sin6_port)+1));
		} else {                       
			sprintf(cmd, "LPRT %d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\015\012",
				6, 16,                                 
				(int)*((unsigned char *)(&sin6->sin6_addr)+0),
				(int)*((unsigned char *)(&sin6->sin6_addr)+1),
				(int)*((unsigned char *)(&sin6->sin6_addr)+2),
				(int)*((unsigned char *)(&sin6->sin6_addr)+3),
				(int)*((unsigned char *)(&sin6->sin6_addr)+4),
				(int)*((unsigned char *)(&sin6->sin6_addr)+5),
				(int)*((unsigned char *)(&sin6->sin6_addr)+6),
				(int)*((unsigned char *)(&sin6->sin6_addr)+7),
				(int)*((unsigned char *)(&sin6->sin6_addr)+8),
				(int)*((unsigned char *)(&sin6->sin6_addr)+9),
				(int)*((unsigned char *)(&sin6->sin6_addr)+10),
				(int)*((unsigned char *)(&sin6->sin6_addr)+11),
				(int)*((unsigned char *)(&sin6->sin6_addr)+12),
				(int)*((unsigned char *)(&sin6->sin6_addr)+13),
				(int)*((unsigned char *)(&sin6->sin6_addr)+14),
				(int)*((unsigned char *)(&sin6->sin6_addr)+15),
				2,                                     
				(int)*((unsigned char *)(&sin6->sin6_port)+0),
				(int)*((unsigned char *)(&sin6->sin6_port)+1));
		}
	} else
#endif
	if (cipv == IN_IPV_4) {
		sprintf(cmd, "PORT %d,%d,%d,%d,%d,%d\015\012",
			(int)*((unsigned char *)(&sin4->sin_addr)+0),
			(int)*((unsigned char *)(&sin4->sin_addr)+1),
			(int)*((unsigned char *)(&sin4->sin_addr)+2),
			(int)*((unsigned char *)(&sin4->sin_addr)+3),
			(int)*((unsigned char *)(&sin4->sin_port)+0),
			(int)*((unsigned char *)(&sin4->sin_port)+1));
	} else {
		assert(0);	/* impossible */
	}
#ifdef DEBUG_FTP
	fprintf(stderr, "FTP--> %s\n",cmd);
#endif
                                       
/* Inform TCP that we will accept connections */
	if (listen (pafd->www_con_type->second_fd, 1) < 0) { 
                return -1;             
	}                                        
	return 0;           /* Good */
}
  
void MMCancelFTPReadDocOnStop(PafDocDataStruct *pafd)
{
	free(pafd->iobs.iobuf);
	pafd->iobs.iobuf = NULL;
	pafd->iobs.size_iobuf = 0;
	pafd->iobs.len_iobuf = 0;
	pafd->read_stat =0;
	if (pafd->www_prim_fd_read_id) {
		XtRemoveInput(pafd->www_prim_fd_read_id);
		close(pafd->www_con_type->prim_fd);
	}
	close(pafd->www_con_type->second_fd);

	pafd->www_prim_fd_read_id = 0;		/* sanity */
	pafd->www_con_type->prim_fd = -1;
	pafd->www_con_type->second_fd = -1;

	if (pafd->www_con_type->third_fd >= 0 ) {
		XtRemoveInput(pafd->www_third_fd_read_id);
		close(pafd->www_con_type->third_fd);
		pafd->www_third_fd_read_id = 0;		/* sanity */
		pafd->www_con_type->third_fd = -1;		/* sanity */
	}

	free(pafd->www_con_type);
	pafd->www_con_type = NULL;

}

void MMCancelFTPReadDocOnError(PafDocDataStruct *pafd, char * reason)
{
        free(pafd->iobs.iobuf);        
        pafd->iobs.iobuf = NULL;       
        pafd->iobs.size_iobuf = 0;     
        pafd->iobs.len_iobuf = 0;      
        pafd->read_stat =0;            
/*      FreeMimeStruct(pafd->mhs); */  
        XmxMakeErrorDialog(pafd->win->base, reason, "Net Error");
	if (pafd->www_prim_fd_read_id) {	
        	XtRemoveInput(pafd->www_prim_fd_read_id);
        	close(pafd->www_con_type->prim_fd); 
	}
	pafd->www_prim_fd_read_id = 0;		/* sanity */
	pafd->www_con_type->prim_fd = -1;
	pafd->www_con_type->second_fd = -1;
                                       

	if (pafd->www_con_type->third_fd >= 0 ) {
		XtRemoveInput(pafd->www_third_fd_read_id);
		close(pafd->www_con_type->third_fd);

		pafd->www_third_fd_read_id = 0;		/* sanity */
		pafd->www_con_type->third_fd = -1;		/* sanity */
	}

        free(pafd->www_con_type);      
        pafd->www_con_type = NULL;     
        (*pafd->call_me_on_error)(pafd,reason);
}


#define IBUF_SIZE               8192    /* Start with input buffer this big */
static void IOBuffAppend(IOBStruct * iobs, char * ibuf, int len)
{
        int nsize;
        
        if ( iobs->len_iobuf + len + 1 > iobs->size_iobuf){
                /* the iobuf is too short */
                nsize = iobs->size_iobuf + len + 1 + IBUF_SIZE;
                iobs->iobuf = (char*) realloc(iobs->iobuf, nsize);
                iobs->size_iobuf = nsize;
        }
        memcpy( iobs->iobuf + iobs->len_iobuf, ibuf, len);
        iobs->iobuf[iobs->len_iobuf + len] = '\0';
        iobs->len_iobuf = iobs->len_iobuf + len;
}

/* si on est la c'est qu'il y a qqchose a lire */
void read_ftp_doc_prim_fd_cb( XtPointer clid, int * fd, XtInputId * id)
{
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	int status;
	int syserror;
	int len_read;
	char ibuf[IBUF_SIZE];
	int sw;

	/* lire les donnes */   
        len_read = read(pafd->www_con_type->prim_fd, ibuf, sizeof(ibuf));
        syserror = errno;
#ifdef DEBUG_FTP
        fprintf (stderr, "Read = %d syserror = %d\n", len_read, syserror);
#endif
/* on peut gerer un buffer d'entree/sortie reserver au I/O */
        if (len_read > 0) {     /* append to io buffer */
                IOBuffAppend(&(pafd->iobs), ibuf, len_read);
        }

/* on connect we are waiting for an answer */
	if (len_read < 0) {
		(*pafd->www_con_type->call_me_on_error_cb)(pafd,
			"Net Read Error;\naborting connection");
		return;
	}
	if (len_read == 0) {
		(*pafd->www_con_type->call_me_on_error_cb)(pafd,
			"EOF detected when reading");
		return; 
	}
/* len_read est positif => y a qqeschoses dans iobuf */

	pafd->iobs.iobuf[pafd->iobs.len_iobuf] = '\0'; /* iobuf est *toujours* assez grand pour faire ca */

	if (pafd->read_stat == FTP_READ_R_ON_CONNECT){
		char * username = "anonymous";
		char * cmd;

		status = parse_respons (pafd);  /* read respons on connect*/
		if (! status)		/* not enough data */
			return;
		if (status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
/* The connect(2) is a success. Time to send passwd parameters */

		cmd = (char*)malloc(20+strlen(username));
/* USER */
/* Now we log in; Look up username, prompt for pw. */
/* username is anonymous or ftp */
		sprintf(cmd, "USER %s\015\012", username);
		sw = write(pafd->www_con_type->prim_fd, cmd, (int)strlen(cmd));
		free(cmd);
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd,"FTP CMD USER Write error");
			return;
		}
		pafd->read_stat = FTP_READ_R_ON_USER;
/* reset iobuf */
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		return;
	}
	if (pafd->read_stat == FTP_READ_R_ON_USER){
		char cmd[40];

		status = parse_respons(pafd);
		if (! status)		/* not enought data */
			return;
		if (status != 3){
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
                        return;
		}
/* PASS */
		sprintf(cmd,"PASS mMosaic@\015\012"); /*@@*/
		sw = write(pafd->www_con_type->prim_fd, cmd, (int)strlen(cmd));
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd,"FTP CMD PASS Write error");
			return;
		}
		pafd->read_stat = FTP_READ_R_ON_PASS;
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		return;
	}
	if (pafd->read_stat == FTP_READ_R_ON_PASS){
		char port_cmd[1000];
		char acc_cmd[40];

		status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if (status == 3){
/* ACCT */
			sprintf (acc_cmd, "ACCT noaccount\015\012");
			sw = write(pafd->www_con_type->prim_fd,
				acc_cmd, (int)strlen(acc_cmd));
			if (sw < 0){
				MMCancelFTPReadDocOnError(pafd,
					"FTP CMD ACCT write error");
				return;
			}
			pafd->read_stat = FTP_READ_R_ON_ACCT;
			pafd->iobs.iobuf[0] = '\0';
			pafd->iobs.len_iobuf = 0;
			return;
		}
		if( status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
		status = get_listen_socket_and_port_command(port_cmd, pafd);
/* PORT */
		if ( status !=0 ){
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
		sw = write(pafd->www_con_type->prim_fd,port_cmd,strlen(port_cmd));
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd, "FTP CMD PORT write error");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READ_R_ON_PORT;
		return;
	}
	if (pafd->read_stat == FTP_READ_R_ON_ACCT){
		char port_cmd[1000];

		status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if( status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
		status = get_listen_socket_and_port_command(port_cmd, pafd);
/* PORT */
		if ( status !=0 ){
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
		sw = write(pafd->www_con_type->prim_fd,port_cmd,strlen(port_cmd));
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd, "FTP CMD PORT write error");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READ_R_ON_PORT;
		return;
	}
/* remarque: At this point we are logged and have a listenning socket for data */
	if ( pafd->read_stat == FTP_READ_R_ON_PORT){
		char cmd[100];

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if( status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
/* TYPE */
		sprintf(cmd, "TYPE I\015\012");
		sw = write(pafd->www_con_type->prim_fd,cmd,strlen(cmd));
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd, "FTP CMD TYPE write error");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READ_R_ON_TYPE;
		return;
	}
	if ( pafd->read_stat == FTP_READ_R_ON_TYPE){
		char * fname = URLParse(pafd->aurl,"", PARSE_PATH + PARSE_PUNCTUATION);
		char * cmd;

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if( status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
/* RETR */
		cmd = (char*) malloc(strlen(fname) + 20);
		sprintf(cmd, "RETR %s\015\012", fname);
		sw = write(pafd->www_con_type->prim_fd,cmd,strlen(cmd));
		free(cmd);
		free(fname);
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd, "FTP CMD RETR write error");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READ_R_ON_RETR_FILE;
		return;
	}

	if ( pafd->read_stat == FTP_READ_R_ON_RETR_FILE){
#ifdef IPV6
		SockA6 soc6_address;
#endif
		enum InIpV cipv = pafd->www_con_type->ipv;
		SockA4 soc4_address;
		struct sockaddr * psin=NULL;
		int socksize ;
		int s_socksize;
		char * fname = URLParse(pafd->aurl,"", PARSE_PATH + PARSE_PUNCTUATION);
		char * cmd;

		memset(&soc4_address, 0, sizeof(SockA4));
#ifdef IPV6
		memset(&soc6_address, 0, sizeof(SockA6));
#endif
		switch (cipv) {
		case IN_IPV_4:
			psin = (struct sockaddr *)&soc4_address;
			socksize = sizeof(SockA4);
			break;
#ifdef IPV6
		case IN_IPV_6:
			psin = (struct sockaddr *)&soc6_address; 
			socksize = sizeof(SockA6);
			break;
#endif
		}

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if (status != 1){ /* not a file. CWD to it and NLST */
/* try to cwd to a directory */
			cmd = (char*) malloc(strlen(fname) + 20);
			sprintf(cmd, "CWD %s\015\012", fname);
			sw = write(pafd->www_con_type->prim_fd,cmd,strlen(cmd));
			free(cmd);
			free(fname);
			if (sw < 0){
				MMCancelFTPReadDocOnError(pafd, "FTP CMD CWD write error");
				return;
			}
			pafd->iobs.iobuf[0] = '\0';
			pafd->iobs.len_iobuf = 0;
			pafd->read_stat = FTP_READ_R_ON_CWD_DIR;
			return;
		}
/* we get a file: Wait for the connection and get the data_soc */
		s_socksize = socksize;
		pafd->www_con_type->third_fd = accept(
			pafd->www_con_type->second_fd, psin, &socksize);

		assert(s_socksize == socksize); /* why it change */

		if (pafd->www_con_type->third_fd < 0) {
				MMCancelFTPReadDocOnError(pafd, "FTP Connection Failed trying to accept third_fd");
				return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READING_DATASOC_FILE;
		pafd->www_third_fd_read_id = XtAppAddInput(mMosaicAppContext,
				pafd->www_con_type->third_fd,
				(XtPointer) XtInputReadMask,
				ftp_read_third_fd_file_cb, pafd);
		XtRemoveInput(pafd->www_prim_fd_read_id);  
		pafd->www_prim_fd_read_id = 0;		/* sanity */
		return;
	}
	if (pafd->read_stat == FTP_READING_DATASOC_FILE){
		fprintf (stderr, " This is a Bug, Please report \n");
		fprintf(stderr, "read_ftp_doc_prim_fd_cb: pafd->read_stat == FTP_READING_DATASOC_FILE !!! \n");
		fprintf(stderr, "Aborting...\n");
		assert(0); /* incoherence: peux pas lire un status et des donnees*/
			/* en meme temps */
	}
	if (pafd->read_stat == FTP_READ_R_ON_END_OF_FILE) { /* end process file */
/* Unfortunately, picking up the final reply sometimes causes serious problems.
 * It *probably* isn't safe not to do this, as there is the possibility that   
 * FTP servers could die if they try to send it and we're not listening.     
 */
		char *ct;
		int comp;
		struct stat statbuf;

		status = parse_respons (pafd);  /* Pick up final reply */
		if (status != 2) {                
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
		} 
		pafd->www_con_type->call_me_on_stop_cb = NULL; 
		free(pafd->iobs.iobuf);
		pafd->iobs.iobuf = NULL;
		pafd->iobs.size_iobuf = 0;
		pafd->iobs.len_iobuf = 0; 
		XtRemoveInput(pafd->www_prim_fd_read_id);  
		close(pafd->www_con_type->prim_fd); 

		pafd->www_prim_fd_read_id = 0;		/* sanity */
		pafd->www_con_type->prim_fd = -1;

		free(pafd->www_con_type); 
		pafd->www_con_type = NULL; 
		ct = HTFileName2ct (pafd->aurl, "text/plain", &comp);
		pafd->mhs->content_type = ct;
		pafd->mhs->content_encoding = comp;
		stat(pafd->fname, &statbuf);
		pafd->mhs->content_length = statbuf.st_size;
		(*pafd->call_me_on_succes)(pafd);

/* voir le format du fichier en fonction de son type */
/* disarm the timer */
/* XtRemoveTimeOut(timer) */
		return;
/* end we get a file */
	}

	if (pafd->read_stat == FTP_READ_R_ON_CWD_DIR){
		char cmd[40];

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if( status != 2) {
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
		}
		sprintf(cmd, "NLST -Lla\015\012");
		sw = write(pafd->www_con_type->prim_fd,cmd,strlen(cmd));
		if (sw < 0){
			MMCancelFTPReadDocOnError(pafd, "FTP CMD NLST -Lla write error");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READ_R_ON_NLSTLLA_DIR;
		return;
	}

	if (pafd->read_stat == FTP_READ_R_ON_NLSTLLA_DIR){
#ifdef IPV6
		SockA6 soc6_address;
#endif
		enum InIpV cipv = pafd->www_con_type->ipv;
		SockA4 soc4_address;
		struct sockaddr * psin=NULL;
		int socksize ;
		int s_socksize;

		memset(&soc4_address, 0, sizeof(SockA4));
#ifdef IPV6
		memset(&soc6_address, 0, sizeof(SockA6));
#endif
		switch (cipv) {
		case IN_IPV_4:
			psin = (struct sockaddr *)&soc4_address;
			socksize = sizeof(SockA4);
			break;
#ifdef IPV6
		case IN_IPV_6:
			psin = (struct sockaddr *)&soc6_address; 
			socksize = sizeof(SockA6);
			break;
#endif
		}

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if(status==5) { /*unrecognized command or failed*/
			char cmd1[40];
			sprintf(cmd1, "NLST\015\012");
			sw = write(pafd->www_con_type->prim_fd,cmd1,strlen(cmd1));
			if (sw < 0){ 
				MMCancelFTPReadDocOnError(pafd, "FTP CMD NLST write error");
				return; 
			}
			pafd->iobs.iobuf[0] = '\0';
			pafd->iobs.len_iobuf = 0;
			pafd->read_stat = FTP_READ_R_ON_NLST_DIR;
			return;
		}
		if (status != 1){      
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
                }
/* status=2 succes. read the dir: Wait for the connection and get the data_soc */
		s_socksize = socksize;
		pafd->www_con_type->third_fd = accept(
			pafd->www_con_type->second_fd, psin, &socksize);

		assert(s_socksize == socksize); /* Why change */

		if (pafd->www_con_type->third_fd < 0) {
			MMCancelFTPReadDocOnError(pafd, "FTP unable to accept the data socket");
			return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READING_DATASOC_DIR;
		pafd->www_third_fd_read_id = XtAppAddInput(mMosaicAppContext,
				pafd->www_con_type->third_fd,
				(XtPointer) XtInputReadMask,
				ftp_read_third_fd_dir_nlstlla_cb, pafd);
/* while reading data soc, lock the master channel */
		XtRemoveInput(pafd->www_prim_fd_read_id);  
		pafd->www_prim_fd_read_id = 0; /* sanity */
		return;
	}

	if (pafd->read_stat == FTP_READ_R_ON_NLST_DIR){
#ifdef IPV6
		SockA6 soc6_address;
#endif
		enum InIpV cipv = pafd->www_con_type->ipv;
		SockA4 soc4_address;
		struct sockaddr * psin=NULL;
		int socksize ;
		int s_socksize;

		memset(&soc4_address, 0, sizeof(SockA4));
#ifdef IPV6
		memset(&soc6_address, 0, sizeof(SockA6));
#endif
		switch (cipv) {
		case IN_IPV_4:
			psin = (struct sockaddr *)&soc4_address;
			socksize = sizeof(SockA4);
			break;
#ifdef IPV6
		case IN_IPV_6:
			psin = (struct sockaddr *)&soc6_address; 
			socksize = sizeof(SockA6);
			break;
#endif
		}

                status = parse_respons (pafd);
		if (! status)		/* not enough data */
			return;
		if (status != 1){      
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
			return;
                }
/* status=2 succes. read the dir: Wait for the connection and get the data_soc */
		s_socksize = socksize;
		pafd->www_con_type->third_fd = accept(
			pafd->www_con_type->second_fd, psin, &socksize);

		assert( s_socksize == socksize);	/* why change */

		if (pafd->www_con_type->third_fd < 0) {
				MMCancelFTPReadDocOnError(pafd, "FTP unable to accept data sock");
				return;
		}
		pafd->iobs.iobuf[0] = '\0';
		pafd->iobs.len_iobuf = 0;
		pafd->read_stat = FTP_READING_DATASOC_DIR;
		pafd->www_third_fd_read_id = XtAppAddInput(mMosaicAppContext,
				pafd->www_con_type->third_fd,
				(XtPointer) XtInputReadMask,
				ftp_read_third_fd_dir_nlst_cb, pafd);
		return;
	}

	if (pafd->read_stat == FTP_READING_DATASOC_DIR){
		fprintf (stderr, " This is a Bug, Please report \n");
		fprintf(stderr, "read_ftp_doc_prim_fd_cb: pafd->read_stat == FTP_READING_DATASOC_DIR !!! \n");
		fprintf(stderr, "Aborting...\n");
		assert(0); /* incoherence: peux pas lire un status et des donnees*/
			/* en meme temps */
	}
	if (pafd->read_stat == FTP_READ_R_ON_END_OF_DIR) { /* end process dir */
		char * ct;
		int comp;
		struct stat statbuf;

		status = parse_respons (pafd);  /* Pick up final reply */
		if (status != 2) {    
			switch (status) {
			case -1:
				MMCancelFTPReadDocOnError(pafd,"FTP protocol error.");
				break;
			default:
				MMCancelFTPReadDocOnError(pafd,"FTP Connection Failed");
				break;
			}
		}
		pafd->www_con_type->call_me_on_stop_cb = NULL;
		free(pafd->iobs.iobuf);
                pafd->iobs.iobuf = NULL;
                pafd->iobs.size_iobuf = 0; 
                pafd->iobs.len_iobuf = 0;  
                XtRemoveInput(pafd->www_prim_fd_read_id);
                close(pafd->www_con_type->prim_fd);

		pafd->www_prim_fd_read_id = 0;       /* sanity */
		pafd->www_con_type->prim_fd = -1;

                free(pafd->www_con_type);  
                pafd->www_con_type = NULL; 
                                      
/* voir le format du fichier en fonction de son type */
                ct = HTFileName2ct (pafd->aurl, "text/html", &comp);
                pafd->mhs->content_type = ct;
                pafd->mhs->content_encoding = comp;
		stat(pafd->fname, &statbuf);
		pafd->mhs->content_length = statbuf.st_size;
                (*pafd->call_me_on_succes)(pafd);
/* disarm the timer */                
/* XtRemoveTimeOut(timer) */          
                return; 
	}
}


void ftp_read_third_fd_file_cb(XtPointer clid, int * fd, XtInputId * id)
{
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	int len_w, syserror, len_read;
	char info[256];         /* large enough... */
	char ibuf[IBUF_SIZE];

/* Set the meter....    */
/* lire les donnes */
        len_read = read(pafd->www_con_type->third_fd, ibuf, sizeof(ibuf));
        syserror = errno;
#ifdef DEBUG_FTP
        if (mMosaicAppData.wwwTrace)
                fprintf (stderr, "Read = %d syserror = %d\n",
                        len_read, syserror);
#endif
 
/* on peut gerer un buffer d'entree/sortie reserver au I/O */
        if (len_read > 0) {     /* append to io buffer */
                IOBuffAppend(&(pafd->iobs), ibuf, len_read);
		pafd->total_read_data = pafd->iobs.len_iobuf;
                sprintf(info,"Read %d",pafd->total_read_data);
                XmxAdjustLabelText(pafd->win->tracker_widget, info);
		XFlush(mMosaicDisplay);
                return;
        }
	if(len_read < 0) {
		fprintf (stderr, " This is a Bug, Please report \n");
		fprintf(stderr, "ftp_read_third_fd_file_cb: len_read < 0 !!! \n");
		fprintf(stderr, "Aborting...\n");
		assert(0);
	}
/* len_read = 0 => EOF */
/* copy iobuf to target */
	len_w = write(pafd->fd, pafd->iobs.iobuf, pafd->iobs.len_iobuf);
	if (len_w != pafd->iobs.len_iobuf){
		(*pafd->www_con_type->call_me_on_error_cb)(pafd,
			"I/O Write to disk error");
		return;
	}
/* return to the prim_fd processing */
	XtRemoveInput(pafd->www_third_fd_read_id);
	close(pafd->www_con_type->third_fd);
	pafd->www_con_type->third_fd = -1;
	close(pafd->www_con_type->second_fd);
/* rearm prim_fd for FTP to read final status */
	pafd->www_prim_fd_read_id = XtAppAddInput(mMosaicAppContext,
                        pafd->www_con_type->prim_fd, (XtPointer) XtInputReadMask,
                        read_ftp_doc_prim_fd_cb, (XtPointer) pafd);
	pafd->read_stat = FTP_READ_R_ON_END_OF_FILE;
	pafd->iobs.len_iobuf = 0;       /* purge the buffer */
}

/*      Read a directory into an hypertext object from the data socket
*/                                     
void ftp_read_third_fd_dir_nlstlla_cb(XtPointer clid, int * fd, XtInputId * id)
{
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	int syserror, len_read;
	char info[256];         /* large enough... */
	char ibuf[IBUF_SIZE];
	char *ct;
        char *filename ;
	FILE *fp;
	char *bl, *el;
	char * ptr;
	int count;

/* Set the meter....    */
/* lire les donnes */
        len_read = read(pafd->www_con_type->third_fd, ibuf, sizeof(ibuf));
        syserror = errno;
#ifdef DEBUG_FTP
        if (mMosaicAppData.wwwTrace)
                fprintf (stderr, "Read = %d syserror = %d\n",
                        len_read, syserror);
#endif
 
/* on peut gerer un buffer d'entree/sortie reserver au I/O */
        if (len_read > 0) {     /* append to io buffer */
                IOBuffAppend(&(pafd->iobs), ibuf, len_read);
		pafd->total_read_data = pafd->iobs.len_iobuf;
		sprintf(info,"Read %d",pafd->total_read_data);
		XmxAdjustLabelText(pafd->win->tracker_widget, info);
		XFlush(mMosaicDisplay);
		return;
        }
	if (len_read < 0) {
		fprintf (stderr, " This is a Bug, Please report \n");
		fprintf(stderr, "ftp_read_third_fd_dir_nlstlla_cb: len_read < 0 !!! \n");
		fprintf(stderr, "Aborting...\n");
		assert(0);
	}
/* len_read = 0 => EOF */
/* convert iobuf to html */
        filename = URLParse(pafd->aurl,"", PARSE_PATH + PARSE_PUNCTUATION);

	XtRemoveInput(pafd->www_third_fd_read_id);
	close(pafd->www_con_type->third_fd);
	pafd->www_con_type->third_fd = -1;
/* rearm prim_fd for FTP to read final status */
	pafd->www_prim_fd_read_id = XtAppAddInput(mMosaicAppContext,
                        pafd->www_con_type->prim_fd, (XtPointer)XtInputReadMask,
                        read_ftp_doc_prim_fd_cb, (XtPointer)pafd);
	pafd->read_stat = FTP_READ_R_ON_END_OF_DIR;
	fp = fopen(pafd->fname, "w");
        fprintf (fp, "<H1>FTP Directory ");
        fprintf (fp, filename);
        fprintf (fp, "</H1>\n<PRE><DL>\n");

/* If this isnt the root level, spit out a parent directory entry */
	if(strcmp(filename,"/") != 0) {
		char * buffer = strdup(filename);

		fprintf(fp,"<DD><A HREF=\"");
		strcpy(buffer,filename);
		ptr = strrchr(buffer,'/');
		if(ptr != NULL)
			 *ptr='\0';
		if(buffer[0] == '\0')  
			fprintf(fp,"/");
		else                   
			fprintf(fp, buffer);
		fprintf(fp,"\"><IMG SRC=\"");
		fprintf(fp, HTgeticonname(NULL, "directory"));
		fprintf(fp,"\"> Parent Directory</a>");
		free(buffer);
	}                              
                                       
        bl = pafd->iobs.iobuf;
	el = pafd->iobs.iobuf;
/* Loop until we hit EOF */            
	for(count = 0; count < pafd->iobs.len_iobuf; count++) {
		char c;
		int rs;
		char itemtype;                 
		char itemsize[BUFSIZ];         
		char * itemname;
		int nTime;                     
		char szFileInfo[32];           
		char szMonth[32];              
		char szDay[16];                
		char szYear[32];               
		char szTime[32];               
		char ellipsis_string[1024];
		int cmpr;
		int nStringLen;                
		int nSpaces;                   
		char szDate[256];              

		c = pafd->iobs.iobuf[count];
		if (c != '\n' && c != '\r')
			continue;
		el = pafd->iobs.iobuf + count;
		*el = '\0';
/* Parse the input buffer, extract the item type, and the item size */

		rs = sscanf(bl,"%c%*9s %*d %*s %*s %s", &itemtype, itemsize);
		if (rs != 2) { 	/* not a good format */
			bl = el+1;
			continue;
		}

/* Due to the various time stamp formats, its "safer" to retrieve the        */
/* filename by taking it from the right side of the string, we do that here. */
		ptr = strrchr(bl,' ');
		if(ptr == NULL) {
			bl = el+1;
			continue;
		}
		itemname= ptr+1;
		if (!strcmp(itemname,".") || !strcmp(itemname,"..")) {
			bl = el+1;
			continue;
		}
		nTime = ParseDate(bl, szFileInfo, szMonth, szDay, szYear, szTime);
		ParseFileSizeAndName(bl, itemname, itemsize);
		if (nTime == 3) {  /* ie a dos or NT server possibly */
			if (!ParseFileSizeAndName(bl, itemname, itemsize)) {
				itemtype = 'd';
			} else {
				itemtype = '-';
			}      
		}              
		fprintf (fp, "<DD>");
/* Spit out the anchor refrence, and continue on... */
		fprintf (fp, "<A HREF=\"");
/* Assuming it's a relative reference... */  
		if (itemname && itemname[0] != '/') {
			fprintf (fp, filename);
			if (filename[strlen(filename)-1] != '/')
				fprintf (fp, "/");
		}                      
		fprintf (fp, itemname);
		fprintf (fp, "\">");

/* There are 3 "types", directory, link and file.  If its a directory we     */
/* just spit out the name with a directory icon.  If its a link, we go       */
/* retrieve the proper name (i.e. the input looks like bob -> ../../../bob   */
/* so we want to hop past the -> and just grab bob.  The link case falls     */
/* through to the filetype case.  The filetype shows name and filesize, and  */
/* then attempts to select the correct icon based on file extension.         */
		switch(itemtype) {     
		case 'd':              
			if (compact_string(itemname,ellipsis_string,20,2,3)) {
				strcpy(itemname,ellipsis_string);
			}              
			fprintf(fp, "<IMG SRC=\"");
			fprintf(fp, HTgeticonname(NULL, "directory"));
			fprintf(fp, "\"> ");
			fprintf (fp, "%s", itemname);
			fprintf (fp, "</A>");
			break;         
		case 'l':              
			ptr = strrchr(bl,' ');
			if(ptr != NULL) {
				*ptr = '\0'; 
				ptr = strrchr(bl,' ');
			}              
			if(ptr != NULL) {
				*ptr = '\0'; 
				ptr = strrchr(bl,' ');
			}              
			if(ptr != NULL)
				strcpy(itemname,ptr+1);
			if (compact_string(itemname,ellipsis_string,20,2,3)) {
				strcpy(itemname,ellipsis_string);
			}              
		case '-':              
/* If this is a link type, and the bytes are small, its probably a directory
 * so lets not show the byte count */ 
			if (compact_string(itemname,ellipsis_string,20,2,3)) {
				strcpy(itemname,ellipsis_string);
			}              
			ct = HTFileName2ct(itemname, "text/html", &cmpr);
			fprintf(fp, "<IMG SRC=\"");
/* If this is a link, and we can't figure out what
 * kind of file it is by extension, throw up the unknown
 * icon; however, if it isn't a link and we can't figure
 * out what it is, throw up the text icon... 
 * Unless it's compressed. */          
			if(itemtype == 'l' && cmpr == NO_ENCODING) {
/* If it's unknown, let's call it a menu (since symlinks
 * are most commonly used on FTP servers to point to
 * directories, IMHO... -marc */       
				fprintf(fp, HTgeticonname(ct, "directory") );
			} else {
				fprintf(fp, HTgeticonname(ct, "text"));
			}
			fprintf(fp, "\"> ");
			fprintf (fp, "%s", itemname);
			fprintf (fp, "</A>");
			break;         
		default:               
			fprintf(fp, "<IMG SRC=\"");
			fprintf(fp, HTgeticonname(NULL, "unknown"));
			fprintf(fp, "\"> ");
			fprintf (fp, itemname);
			fprintf (fp, "</A>");
			break;         
		}                      
                nStringLen = strlen(itemname); 
                nSpaces = 20 - nStringLen;
                if (nTime == 1) {      
                        struct tm *ptr1;
                        time_t t;  
                                       
                        t=time(0);     
                        ptr1=localtime(&t);
                        sprintf(szYear,"%d",1900+ptr1->tm_year);
                        sprintf(szDate, "%*s%9s %s %s %s %2.2s, %s", nSpaces, " ", itemsize, szFileInfo, szTime, szMonth, szDay, szYear);
                } else if (nTime == 0) {
                        sprintf(szDate, "%*s%9s %s %s %s %2.2s, %s", nSpaces, " ", itemsize, szFileInfo, "     ", szMonth, szDay, szYear);
                } else {
			sprintf(szDate, "%*s  %9.9s  %s %s", nSpaces, " ", itemsize, szMonth, szTime);                   
                }
                fprintf (fp, szDate);
                fprintf (fp, "\n"); 
		bl = el+1;
        }                              
        fprintf (fp, "</DL>\n");
        fprintf (fp, "</PRE>\n");
	fclose(fp);
/* voir le format du fichier en fonction de son type */
	ct = "text/html";
	pafd->mhs->content_type = strdup("text/html");
	free(filename);
	pafd->iobs.len_iobuf = 0;       /* purge the buffer */
        return;
}

void ftp_read_third_fd_dir_nlst_cb(XtPointer clid, int * fd, XtInputId * id)
{
	/*only name*/
	PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	int syserror, len_read; 
	char info[256];         /* large enough... */
	char ibuf[IBUF_SIZE];          
	char *ct;                      
	char *filename ;               
	FILE *fp;                      
	char *bl, *el;                 
	char * ptr;                    
	int count; 

/* Set the meter....    */             
/* lire les donnes */                  
	len_read = read(pafd->www_con_type->third_fd, ibuf, sizeof(ibuf));
	syserror = errno;              
#ifdef DEBUG_FTP
	if (mMosaicAppData.wwwTrace)   
		fprintf (stderr, "Read = %d syserror = %d\n",
			len_read, syserror); 
#endif
/* on peut gerer un buffer d'entree/sortie reserver au I/O */
	if (len_read > 0) {     /* append to io buffer */
		IOBuffAppend(&(pafd->iobs), ibuf, len_read);
		pafd->total_read_data = pafd->iobs.len_iobuf;
		sprintf(info,"Read %d",pafd->total_read_data);
		XmxAdjustLabelText(pafd->win->tracker_widget, info);
		XFlush(mMosaicDisplay);
		return;                
	}                              
	if (len_read < 0) {            
		fprintf (stderr, " This is a Bug, Please report \n");
		fprintf(stderr, "ftp_read_third_fd_dir_nlstlla_cb: len_read < 0 !!! \n");
		fprintf(stderr, "Aborting...\n");
		assert(0);               
	}                              
/* len_read = 0 => EOF */              
/* convert iobuf to html */            
	filename = URLParse(pafd->aurl,"", PARSE_PATH + PARSE_PUNCTUATION);
 
        XtRemoveInput(pafd->www_third_fd_read_id);
        close(pafd->www_con_type->third_fd);
        pafd->www_con_type->third_fd = -1;  
/* rearm prim_fd for FTP to read final status */
        pafd->www_prim_fd_read_id = XtAppAddInput(mMosaicAppContext,
                        pafd->www_con_type->prim_fd, (XtPointer)XtInputReadMask,
                        read_ftp_doc_prim_fd_cb, (XtPointer)pafd);
        pafd->read_stat = FTP_READ_R_ON_END_OF_DIR;
        fp = fopen(pafd->fname, "w");  
        fprintf (fp, "<H1>FTP Directory "); 
        fprintf (fp, filename);        
        fprintf (fp, "</H1>\n<PRE><DL>\n");
/* If this isnt the root level, spit out a parent directory entry */
        if(strcmp(filename,"/") != 0) {
                char * buffer = strdup(filename);
                
                fprintf(fp,"<DD><A HREF=\"");
                strcpy(buffer,filename);
                ptr = strrchr(buffer,'/');
                if(ptr != NULL)
                         *ptr='\0';
                if(buffer[0] == '\0')
                        fprintf(fp,"/");
                else
                        fprintf(fp, buffer);
                fprintf(fp,"\"><IMG SRC=\"");
                fprintf(fp, HTgeticonname(NULL, "directory"));
                fprintf(fp,"\"> Parent Directory</a>");
                free(buffer);
        } 

        bl = pafd->iobs.iobuf;
        el = pafd->iobs.iobuf;
/* Loop until we hit EOF */
        for(count = 0; count < pafd->iobs.len_iobuf; count++) {
		char c;
		char * itemname;
		int cmpr;
		char ellipsis_string[1024];

                c = pafd->iobs.iobuf[count];
                if (c != '\n' && c != '\r')  
                        continue;      
                el = pafd->iobs.iobuf + count;
                *el = '\0';            

		itemname = bl;
                if (!strcmp(itemname,".") || !strcmp(itemname,"..")) {
                        bl = el+1;
                        continue;
                } 

                fprintf (fp, "<DD>");  
/* Spit out the anchor refrence, and continue on... */
                fprintf (fp, "<A HREF=\"");
                fprintf (fp, itemname);
                fprintf (fp, "\">");
                if (compact_string(itemname,ellipsis_string,20,2,3)) {
                	strcpy(itemname,ellipsis_string);
                } 
		ct = HTFileName2ct(itemname, "text/html", &cmpr);
		fprintf(fp, "<IMG SRC=\"");
		if(cmpr == NO_ENCODING) {
                        fprintf(fp, HTgeticonname(ct, "unknown") );
                }
		fprintf(fp, "\"> "); 
                fprintf (fp, "%s", itemname);
                fprintf (fp, "</A>");
		fprintf (fp, "\n");   
                bl = el+1; 
        }                             
        fprintf (fp, "</DL>\n");      
        fprintf (fp, "</PRE>\n");     
        fclose(fp);                   
/* voir le format du fichier en fonction de son type */
        ct = "text/html";             
        pafd->mhs->content_type = strdup("text/html");
        free(filename);               
        pafd->iobs.len_iobuf = 0;       /* purge the buffer */
        return;
}

/*
        char buffer[BUFSIZ];           
        char buf[BUFSIZ];              
        char itemname[BUFSIZ];         
        char *full_ftp_name, *ptr;     
        int count, ret, cmpr, c, rv;   
        int nOldSpaces;                
*/



static int ParseDate(char *szBuffer, char *szFileInfo, char *szMonth, char *szDay, char *szYear, char *szTime)
{
        char *szPtr,*szEndPtr;
        int nCount;
        char *tmpbuf=(char *)calloc(BUFSIZ,sizeof(char));
 
        if (!szBuffer) {
                free(tmpbuf);
                return(0);
        }
 
        if ( (*szBuffer != 'd') && (*szBuffer != 'l') && (*szBuffer != '-')) {
                /* Hopefully this is the dos format */
 
                szPtr = szBuffer;
                strncpy(szMonth, szBuffer, 8);
                szMonth[8] = '\0';
 
                szPtr = szPtr + 10;
                if (szPtr) {
                        strncpy(szTime, szPtr, 7);
                        szTime[7] = '\0';
                }
 
                szPtr = szPtr + 15;
                if (szPtr) {
                        if (*szPtr == 'D') {
                                *szDay = 'd';
                                szDay[1] = '\0';
                        } else {
                                *szDay = 'f';
                                szDay[1] = '\0';
                        }
                }
                                       
                free(tmpbuf);          
                return(3); /* ie the info is this dos way */
        } else {                       
                szPtr = NULL;          
                nCount = 0;            
                          
                /* alright, use this ugly loop to go to each of the month, day, year, whatever parts */                  
                while (szPtr || ((nCount == 0) && szBuffer)) {
                        switch (nCount) {
                        case 0:  /* file info */
                                strncpy(szFileInfo, szBuffer, 10);
                                szFileInfo[10] = '\0';
                                       
                                strcpy(tmpbuf,szBuffer);
                                /*filename*/
                                szPtr=strrchr(tmpbuf,' ');
                                while (szPtr && (*szPtr == ' ')) {
                                        szPtr--;
                                }
                                *(szPtr+1)='\0';
                                if (szPtr && *szPtr=='>') { /*deal with a
link*/                         
                                        if (szPtr) {
                                                szPtr=strrchr(tmpbuf,' ');                                                        while (szPtr && (*szPtr == ' ')) {                               
                                                        szPtr--;
                                                }
                                                *(szPtr+1)='\0';
                                        }
                                        if (szPtr) {
                                                szPtr=strrchr(tmpbuf,' ');                                                        while (szPtr && (*szPtr == ' ')) {                               
                                                        szPtr--;
                                                }
                                                *(szPtr+1)='\0';
                                        }
                                }
                                       
                                if (szPtr) { /*year/time*/
                                        szPtr=strrchr(tmpbuf,' ');
                                        while (szPtr && (*szPtr == ' ')) {                                                        szPtr--;
                                        }
                                }
                                *(szPtr+1)='\0';
                               
                                if (szPtr) { /*date*/
                                        szPtr=strrchr(tmpbuf,' ');
                                        while (szPtr && (*szPtr == ' ')) {                                                        szPtr--;
                                        }
                                }
                                *(szPtr+1)='\0';
                               
                                if (szPtr) { /*month*/
                                        szPtr=strrchr(tmpbuf,' ');
                                }
                                /*beginning of month*/
                                szPtr++;
                               
                                szPtr=szBuffer+(szPtr-tmpbuf);
                                break; 
                        case 1:        
                                szEndPtr = strchr(szPtr, ' ');
                                if (szEndPtr) {
                                        strncpy(szMonth, szPtr, szEndPtr - szPtr);                                        szMonth[szEndPtr - szPtr] = '\0';
                                        szPtr = szEndPtr+1;  /* go to next entry (day) */                                
                                        while (szPtr && (*szPtr == ' '))
                                                        szPtr++;
                                } else {
                                        strcpy(szMonth, " ");
                                }      
                                break; 
                                       
                        case 2:        
                                szEndPtr = strchr(szPtr, ' ');
                                if (szEndPtr) {
                                        strncpy(szDay, szPtr, szEndPtr - szPtr);
                                        szDay[szEndPtr - szPtr] = '\0';
                                        szPtr = szEndPtr+1;
                                        while (szPtr && (*szPtr == ' '))
                                                        szPtr++;
                                } else {
                                        strcpy(szDay, " ");
                                }      
                                break; 
                                       
                        case 3:
                                szEndPtr = strchr(szPtr, ' ');
                                if (szEndPtr) {
                                        strncpy(szYear, szPtr, szEndPtr - szPtr);
                                        szYear[szEndPtr - szPtr] = '\0';
                                        szPtr = szEndPtr+1;
                                } else if (szEndPtr) {
                                        strcpy(szYear, " ");
                                }      
                                break; 
                                       
                        case 4:        
                                szPtr = NULL;
                                       
                        }              
                        nCount++;      
                }                      
                szPtr = strchr(szYear, ':');
                if (!szPtr) {          
                        free(tmpbuf);  
                        return(0);  /* ie the info is month, day, year */
                }                      
                szPtr -= 2;  /* beginning of time; */
                                       
                strncpy(szTime, szPtr, 5);  
                szTime[5] = '\0';      
                                       
                free(tmpbuf);          
                return(1);  /* ie the info is month, day, time */
        }                              
}    
 
/* This code based off of Rick Vestal's FTP parse code for the NCSA Windows
 * Mosaic client.
 *
 * Modified for X by Scott Powers 9.27.95
 */
 
static int ParseFileSizeAndName(char *szBuffer, char *szFileName, char *szSize)
{
        char *szPtr;
        static char *tmpbuf=NULL;

        if (!szBuffer)
                return(0);

        if (!tmpbuf)
                tmpbuf=(char *)calloc(BUFSIZ,sizeof(char));

        strcpy(tmpbuf,szBuffer);

        /*filename*/
        szPtr=strrchr(tmpbuf,' ');
        while (szPtr && (*szPtr == ' '))
                szPtr--;
        *(szPtr+1)='\0';
        if (szPtr && *szPtr=='>') { /*deal with a link*/
                if (szPtr) {
                        szPtr=strrchr(tmpbuf,' ');
                        while (szPtr && (*szPtr == ' '))
                                szPtr--;
                        *(szPtr+1)='\0';
                }       
                if (szPtr) {
                        szPtr=strrchr(tmpbuf,' ');
                        while (szPtr && (*szPtr == ' '))
                                szPtr--;
                        *(szPtr+1)='\0';
                }
        }                      
        if (szPtr) { /*year/time*/
                szPtr=strrchr(tmpbuf,' ');
                while (szPtr && (*szPtr == ' '))
                        szPtr--;
        }                      
        *(szPtr+1)='\0';       
                               
        if (szPtr) { /*date*/  
                szPtr=strrchr(tmpbuf,' ');
                while (szPtr && (*szPtr == ' '))
                        szPtr--;
        }                      
        *(szPtr+1)='\0';       
                               
        if (szPtr) { /*month*/ 
                szPtr=strrchr(tmpbuf,' ');
                while (szPtr && (*szPtr == ' '))
                        szPtr--;
        }                      
        *(szPtr+1)='\0';       
                               
        if (szPtr) { /*filesize*/
                szPtr=strrchr(tmpbuf,' ');
        }
                               
        /*beginning of filesize*/
        szPtr++;               
                               
        strcpy(szSize,szPtr);  
        return(1); /* not a directory */
}

/*###### Remenber*/
/* Info for cached connection;  right now we only keep one around for a while */
/* ##########
XtIntervalId timer;
static struct ftpcache {
		int control;
		char p1[256];
		char host[256];
		char username[BUFSIZ];
		char password[BUFSIZ];
} ftpcache = { -1, "", "", "", "" };

void HTFTPClearCache (void)           
{                                     
	ftpcache.password[0] = '\0';        
	ftpcache.control = -1;              
	ftpcache.p1[0] = 0;                 
	ftpcache.host[0] = 0;               
	ftpcache.username[0] = 0;           
}                                     

*/

