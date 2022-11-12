#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

#if defined(SOLARIS)
#include <stropts.h>
#endif

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mime.h"
#include "paf.h"
#include "http-proto.h"
#include "URLParse.h"

#ifdef DEBUG
#define DEBUG_HTTP
#endif

extern char * MMGetUserAgent();
extern char * HTSACat  (char **dest, const char *src);
#define StrAllocCat(dest, src)  HTSACat  (&(dest), src)


/* all we need for HyperText Tranfer Protocol */

#define HTTP_VERSION	"HTTP/1.0"

static char crlf[3] = { '\015', '\012', '\0'};	/* A CR LF equivalent string */

char * BuildDocHTTPCommand(PafDocDataStruct * pafd, int * len_ret)
{
	char *command;			/* The whole command */
	char line[10000];		/* #### */
	char * ua;


/* aurl looks like http://node.domain/path/doc.html  */
/* for a Simple request we don't use keep alive connection */

/* Ask that node for the document, omitting the host name & anchor */        
	char * abs_path = URLParse(pafd->aurl, "", PARSE_PATH|PARSE_PUNCTUATION);
	char * origin_server = URLParse(pafd->aurl, "", PARSE_HOST);

	command = (char*) malloc(5 + strlen(pafd->aurl)+ 2 + 31);
	*len_ret = 0;

/* make a command depending we ask a proxy or make a direct request
 * for a proxy:
 *	GET http://node/abspath HTTP/1.0
 * for a direct request:
 *	GET /abspath HTTP/1.0
*/

	if (pafd->post_data && pafd->post_ct) { /* posting data from a FORM */
 		strcpy(command, "POST ");
	} else {
		strcpy(command, "GET ");
	}

	if (pafd->proxent){ /*are we using an HTTP proxy */
		strcat(command, pafd->aurl);
	} else {
		strcat(command, abs_path);
	}
	free(abs_path);

	strcat(command, " ");
	strcat(command, HTTP_VERSION);
	strcat(command, crlf);	/* CR LF, as in rfc 977 */

/* make full HTTP/1.0 request header */
/* look in the accept parameter to form a Accept line */
/*	if (!HTPresentations)
		HTPresentationInit();
	n = HTList_count(HTPresentations);
*/

	sprintf(line, "Accept: */*\015\012");
	StrAllocCat(command, line);

/* no-cache ? */
	if (pafd->pragma_no_cache) {
		sprintf(line, "Cache-Control: no-cache\015\012");
		StrAllocCat(command, line);
	}
/* KeepAlive: ? */
/* if (useKeepAlive) {
 * sprintf(line, "Connection: keep-alive%c%c", CR, LF);
 * StrAllocCat(command, line);
 * }
*/

/* send User-Agent */
	ua = MMGetUserAgent();
	sprintf(line, "User-Agent: %s\015\012",ua);
	StrAllocCat(command, line);

/* the host:port field values of the original URL request. */
	sprintf(line, "Host: %s\015\012", origin_server);
	StrAllocCat(command, line);

	if(mMosaicAppData.acceptlanguage_str) {
		sprintf(line, "Accept-Language: %s\015\012",
			mMosaicAppData.acceptlanguage_str);
		StrAllocCat(command, line);
	}

/* Authorization: line */
/*	docname = URLParse(arg, "", PARSE_PATH);
 *	hostname = URLParse(arg, "", PARSE_HOST);
 *	if (hostname && NULL != (colon = strchr(hostname, ':'))) {
 *		*(colon++) = '\0';	*/ /* Chop off port number */
/*		portnumber = atoi(colon);
 *	} else 
 *		portnumber = 80;
 *	if (NULL!=(auth=HTAA_composeAuth(hostname, portnumber, docname,appd))) {
 *		sprintf(line, "%s%c%c", auth, CR, LF);
 *		StrAllocCat(command, line);
 *	}
 *	free(hostname);
 *	free(docname);
 */

/* POST somethings from a FORM */
	if (pafd->post_data && pafd->post_ct) { /* posting data from a FORM */
 		int post_content_len;

		post_content_len = strlen (pafd->post_data);
		sprintf(line, "Content-Type: %s\015\012", pafd->post_ct);
		StrAllocCat(command, line);
		sprintf(line, "Content-length: %d\015\012", post_content_len);
		StrAllocCat(command, line);
		StrAllocCat(command, crlf); /*Blank line: "end" (mimeheader) */
 		StrAllocCat(command, pafd->post_data); /* send post data */
		*len_ret = strlen(command);
		return command;
	}

/* The end of command */
	StrAllocCat(command, crlf);  /* Blank line means "end" */
	*len_ret = strlen(command);
	return command;
}

/* le read est par protocol specifique */
/*cette routine est 'pseudo interruptible'. On interrompt le transfert
 * en cliquant sur le bouton Stop */
/* mettre a jour la 'jauge' quand on recoit des donnes */
/* test pour voir si 0->EOF si 0< error */
/* sinon lire les donnes qui peuvent contenir en entete un message
 * comme par exemple le proto. HTTP. faut le depouiller de son entete
 * Analyser l'entete et stocker les donnes dans le fichier idoine */
/* si EOF alors on fait un popdown et on close la connexion */

#define IBUF_SIZE		8192	/* Start with input buffer this big */
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

/* ######################################## */

/* top->down proc. this is one at down level */
void MMStopHTTPReadDoc(PafDocDataStruct * pafd)
{
	free(pafd->iobs.iobuf);
	pafd->iobs.iobuf = NULL;
	pafd->iobs.size_iobuf = 0;
	pafd->iobs.len_iobuf = 0;
	pafd->read_stat =0;
	pafd->www_con_type->call_me_on_stop_cb = NULL;
	pafd->www_con_type->call_me_on_error_cb = NULL;
	XtRemoveInput(pafd->www_prim_fd_read_id);
	close(pafd->www_con_type->prim_fd);
	pafd->www_prim_fd_read_id = 0;		 /* sanity */
	pafd->www_con_type->prim_fd = -1;	 /* sanity */
	free(pafd->www_con_type);
	pafd->www_con_type = NULL;
}
void MMCancelHTTPReadDocOnError(PafDocDataStruct * pafd, char * msg)
{
	free(pafd->iobs.iobuf);
	pafd->iobs.iobuf = NULL;
	pafd->iobs.size_iobuf = 0;
	pafd->iobs.len_iobuf = 0;
	pafd->read_stat =0;
/*#####	FreeMimeStruct(pafd->mhs); #####*/
/*#####	pafd->mhs=NULL; 	don't do this because use by multiple pafc */
/*### one mhs for multiple embedded object */
/*	XmxMakeErrorDialog(pafd->win->base, msg, "Net Error"); */

	XtRemoveInput(pafd->www_prim_fd_read_id);

	close(pafd->www_con_type->prim_fd);
	pafd->www_prim_fd_read_id = 0; /* sanity */
	pafd->www_con_type->prim_fd = -1; /* sanity */

	free(pafd->www_con_type);
	pafd->www_con_type = NULL;
	(*pafd->call_me_on_error)(pafd,msg);
}

void read_http_doc_prim_fd_cb( XtPointer clid, int * fd, XtInputId * id)
{
#define HAVE_READ_STATUS 1
#define HAVE_READ_HEADER 2
        PafDocDataStruct * pafd = (PafDocDataStruct *) clid;
	MimeHeaderStruct mhs;
	char ibuf[IBUF_SIZE];
        int len_read;                       
	char server_version[30];
	int server_status;
	int syserror;
	int nfields;
	char * deb_from, *deb_to, *end_from;
	int lcut;
	char *lf_ptr;
	char *lfcrlf_ptr;
	int n_message = 0;
	int a_lire = 0;

/* La requete est parti. On a une reponse (qques choses a lire). On regarde ce
 * qu'on doit faire et ou on en est. Autrement dit : dans quel etat on est,
 * et suivant cet etat on fait des 'choses'
 */

	memset(&mhs,0,sizeof(MimeHeaderStruct));
	server_version[0] = '\0';

/* lire les donnes */
#if defined(SOLARIS) 
	n_message = ioctl(pafd->www_con_type->prim_fd, I_NREAD, &a_lire);
#else
	a_lire = sizeof(ibuf);
/*	n_message = ioctl(pafd->www_con_type->prim_fd, FIONREAD, &a_lire); */
#endif

/*#define DEBUG_READ 1 */

#ifdef DEBUG_READ
	printf("n_message = %d, a_lire = %d, sizeof(ibuf) = %d\n",
		n_message, a_lire, sizeof(ibuf));
#endif
	if (a_lire >0) {
		if (a_lire > sizeof(ibuf) ) a_lire = sizeof(ibuf);
		len_read = read(pafd->www_con_type->prim_fd, ibuf, a_lire);
		syserror = errno;
	} else if (a_lire == 0) {
		len_read =0;
	} else {
		len_read =-1;
		(*pafd->www_con_type->call_me_on_error_cb)(pafd,
			"Net Read Error;\naborting connection");
		return;
	}

#ifdef DEBUG_READ
	fprintf (stderr, "Read = %d syserror = %d\n", len_read, syserror);
#endif

/* on peut gerer un buffer d'entree/sortie reserver au I/O */
	if (len_read > 0) {	/* append to io buffer */
		IOBuffAppend(&(pafd->iobs), ibuf, len_read);
	}

	if( !pafd->read_stat ){
/* ###########	Read the first line of the response  #############   */
/* It is the status line respons header. The read_stat say: we are at
 * beginning. Read the status line. Get numeric status etc */
		memset(&mhs,0,sizeof(MimeHeaderStruct));
		ParseMimeHeader("", &mhs);  /* get a default mime header*/
/* ### 		FreeMimeStructData(pafd->mhs); ### */
		*(pafd->mhs) = mhs;
#ifdef DEBUG_HTTP
		fprintf (stderr, "Readind Status Line\n");
#endif
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
		pafd->iobs.iobuf[pafd->iobs.len_iobuf] = '\0'; /* iobuf est *toujours
					* assez grand pour faire ca */
		lf_ptr = strchr(pafd->iobs.iobuf, '\012'); /* RFC 2068 */
			/* say we must be tolerant with CR  section 19.2 */
/* pointer to LF or NULL */
		if (lf_ptr == NULL) { /* continue to read */
			/* sauf si on juge qu'on en a assez */
			if (pafd->iobs.len_iobuf > 100) { /* y a un problem */
				(*pafd->www_con_type->call_me_on_error_cb)(pafd,
					"HTTP server don't send status line");
				return;
			}
			return; /* continue to read */
		}
		pafd->lfcrlf_type = "\012\012";
		if (lf_ptr[-1] == '\015'){
			pafd->lfcrlf_type = "\012\015\012";
		}
/* on a trouve CRLF */
		nfields = sscanf(pafd->iobs.iobuf, "%20s %d", server_version, &server_status);
#ifdef DEBUG_HTTP
		if (mMosaicAppData.wwwTrace)
	 		fprintf (stderr, "server version %s status %d\n",
				server_version,server_status);
#endif
		if (nfields != 2 || strncmp(server_version, "HTTP/1.", 7)) {
			char * msg = (char*) malloc(strlen(server_version)+100);

			sprintf (msg, "Talking to bad release HTTP/1.x server.\nserver release is : %s", server_version);
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,msg);
			free(msg);
			return; 
		}
/* shift the iobuf and update len_iobuf */
		deb_from = lf_ptr ;
		deb_to = pafd->iobs.iobuf;
		lcut = lf_ptr  - pafd->iobs.iobuf ;
		end_from = pafd->iobs.iobuf + pafd->iobs.len_iobuf;
		while(deb_from <= end_from)
			*deb_to++ = *deb_from++;
		pafd->iobs.len_iobuf = pafd->iobs.len_iobuf - lcut;
/* remarque: pafd->iobuf point sur LF de fin de ligne (ou debut de la suivante) */		
/* ###########	END Read the first line of the response  #############   */
/* now Action on status. Decode full HTTP/1.0 response */

		pafd->format_in = strdup("text/html");
		pafd->http_status = server_status;
		switch (server_status) {
/* 1xx Informationnal */
		case 100:
		case 101:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Status Code 1xx not yet supported by mMosaic");
			return;
/* 2xx Success */
		case 200:	/* OK */
/* #################################################### */
/* if (do_head) {
 * if (!start_of_data || !*start_of_data) {
 * headData=NULL;
 * } else {
 * char *ptr;
 * headData=strdup(start_of_data);
 * ptr=strchr(headData,'\n');
 * *ptr='\0';
 * }       
 * }       
 * break;  
 */
/* #################################################### */
			break;
		case 201:	/* Created */
			break;
		case 202:	/* Accepted */
		case 203:	/* Non-Authoritative Information */
		case 204:	/* No-Content */
			break;
		case 205:	/* Reset Content */
		case 206:	/* Partial Content */
			break;
/* 3xx forms of redirection */
		case 300:
		case 301:
		case 302:
			break;
		case 303:
		case 304:
		case 305:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Status Code 3xx not yet supported by mMosaic");
			return;
/* 4xx Client error */
		case 400:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Bad Request");
			return;
		case 401:
/* #################################################### */
/* if (HTAA_shouldRetryWithAuth(start_of_data, length, s,appd)) {
 * (void)close(s);
 * lsocket = -1;
 * if (line_buffer)
 * free(line_buffer);
 * if (line_kept_clean)
 * free(line_kept_clean);
 * fprintf(stderr, "%s %d %s\n", "HTTP: close socket", s,
 * "to retry with Access Authorization");
 * HTProgress ("Retrying with access authorization information.",appd);
 * goto try_again;
 * } else {
 * statusError=1;
 * }   
 */
/* #################################################### */
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Unauthorized");
			return;
		case 402:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 402:\nPayment Required\nBad news :-(");
			return;
		case 403:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 403:\nForbidden");
			return;
		case 404:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 404:\nNot found");
			return;
		case 405:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 405:\nMethod Not Allowed");
			return;
		case 406:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 406:\nNot Acceptable");
			return;
		case 407:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 407:\nProxy Authentification Require");
			return;
		case 408:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 408:\nRequest Time Out");
			return;
		case 409:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 409:\nConflict");
			return;
		case 410:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 410:\nGone");
			return;
		case 411:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 411:\nLength required");
			return;
		case 412:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 412:\nPrecondition Failed");
			return;
		case 413:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 413:\nRequest Entity Too Large");
			return;
		case 414:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 414:\nRequest Uri Too Large");
			return;
		case 415:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 415:\nUnsupported Media Type");
			return;
/* Server Error */
		case 503:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Code 503:\nService Unavailable\nCheck address or proxy");
			return;
		case 500:
		case 501:
		case 502:
		case 504:
		case 505:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Status Code 5xx not yet supported by mMosaic");
			return;
/* bad number */
		default:
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Unknow Status Code from server !!!");
			return;
		} /* Switch on server_status */

		pafd->read_stat = HAVE_READ_STATUS;
		pafd->http_status = server_status;
	}
/* il faut traiter la partie MIME  - format_in = HTAtom_for("text/html");
 * dans iobuf on a laisse le LF de fin de status */

	if ( (pafd->read_stat & HAVE_READ_STATUS) &&
	    !(pafd->read_stat & HAVE_READ_HEADER)) { /* next read MIME header */

		pafd->iobs.iobuf[pafd->iobs.len_iobuf] = '\0'; /* iobuf est assez grand */

/* chercher dans iobuf la chaine LFCRLF (oui gasp !) */
/* la difference de pointeur donne l'entete MIME */

		lfcrlf_ptr = strstr(pafd->iobs.iobuf,pafd->lfcrlf_type);
		if (!lfcrlf_ptr) {
/* tester les different cas avec len_read et pafd->iobs.len_iobuf*/
/*si le buffer grossi trop pour une entete mime => y a un probleme */

			if ( pafd->iobs.len_iobuf > 16384){
				(*pafd->www_con_type->call_me_on_error_cb)(pafd,
					"No Mime Header !!!");
				return;
			}
			if (len_read > 0){ /* continue to read */
				return;
			} else {
				(*pafd->www_con_type->call_me_on_error_cb)(pafd,
					"I/O Read error in Mime Header");
				return;
			}
		}
		deb_to = pafd->iobs.iobuf;
		end_from = pafd->iobs.iobuf + pafd->iobs.len_iobuf;
/* set the last LF to '\0' */
		if ( lfcrlf_ptr[1] == '\015' ) {
			lfcrlf_ptr[2] = '\0';
			deb_from = lfcrlf_ptr + 3 ;
		} else {
			lfcrlf_ptr[1] = '\0';
			deb_from = lfcrlf_ptr + 2 ;
		}
		lcut = deb_from - pafd->iobs.iobuf ;
/* cas ou on trouve lfcrlf_ptr , on stocke le header */
/* en fait on lit le header, si il y en a un, jusqu'a une ligne vide
 * prendre le 'parseur' MIME dans HTMIME.c. */

		memset(&mhs,0,sizeof(MimeHeaderStruct));
		ParseMimeHeader(pafd->iobs.iobuf, &mhs); 
/* If we asked only to read the header or footer or we used a HEAD
 * method then we stop here as we don't expect any body part.
 *
**  If there is no content-length, no transfer encoding and no
**  content type then we assume that there is no body part in
**  the message and we can return HT_LOADED
*/

/* ##################################################### */
/* if(*p=='K' && !strncmp("Keep-Alive:",p,11)){
 * fprintf (stderr, "HTTP: Server Agrees to Keep-Alive\n");
 * lsocket = s; 
 * p+=10;
 * }
 */
/* ##################################################### */

/* FAIRE UN SHIFT SUR IOBUF POUR QUE LE 1ER CARA. SOIT CELUI DES DONNES.  */
		while(deb_from <= end_from) 
                        *deb_to++ = *deb_from++;
		pafd->iobs.len_iobuf = pafd->iobs.len_iobuf - lcut;
/*###		FreeMimeStructData(pafd->mhs); ### */
		*(pafd->mhs) = mhs;	/* it's a copy !!! */
/* remarque: pafd->iobuf pointe sur le 1er caractere des datas */
		pafd->read_stat |= HAVE_READ_HEADER;
		pafd->total_read_data = 0;
	}

/* Et maintenant on lit les donnees qu'on met dans le fichier cible */
/* on traite les donnees en fonction de l'entete MIME */
/*Tester ("Content-length: ",p,16) du header mime ) */
/*tester ("Keep-Alive:",p,11)) et le reste du header ... */
/* IOBUF POINTE SUR LE 1ER CARACTERE DE DONNE LIRE JUSQU'A EOF OU ERROR */
/* we need to read until pafd->total_read_data == pafd->mhs.content_length */

	if (pafd->read_stat & HAVE_READ_HEADER) { /* read data */
		int len_w;
		char info[256];		/* large enought... */

/* Set the meter....	*/
		if (len_read < 0) {
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"Net Read Error;\naborting connection");
			return;
		}
		pafd->total_read_data += pafd->iobs.len_iobuf;
		if (pafd->mhs->content_length > 0){
			sprintf(info,"Read %d/%d", pafd->total_read_data,
				pafd->mhs->content_length);
		} else {
			sprintf(info,"Read %d",pafd->total_read_data);
		}
		XmxAdjustLabelText(pafd->win->tracker_widget, info);
		XFlush(mMosaicDisplay);
/* copy iobuf to target */
		len_w = write(pafd->fd, pafd->iobs.iobuf, pafd->iobs.len_iobuf);
		if (len_w != pafd->iobs.len_iobuf){
			(*pafd->www_con_type->call_me_on_error_cb)(pafd,
				"I/O Write to disk error");
			return;
		}
		if((len_read == 0) || 		/* we are at EOF */
		   (pafd->mhs->content_length>0 && /* or enought data */
		    pafd->total_read_data >=pafd->mhs->content_length) ) {
/* that is the 'Stop' procedure */
			pafd->www_con_type->call_me_on_stop_cb = NULL;
			free(pafd->iobs.iobuf);
			pafd->iobs.iobuf = NULL;
			pafd->iobs.size_iobuf = 0;
			pafd->iobs.len_iobuf = 0;
			pafd->read_stat =0;
			XtRemoveInput(pafd->www_prim_fd_read_id);
			close(pafd->www_con_type->prim_fd);

			pafd->www_prim_fd_read_id = 0; /* sanity */
			pafd->www_con_type->prim_fd = -1; /* sanity */

			free(pafd->www_con_type);
			pafd->www_con_type = NULL;

			(*pafd->call_me_on_succes)(pafd);
			return;
		}
		pafd->iobs.len_iobuf = 0;	/* purge the buffer */
	}
/* next read */
}

#if 0
/* some code to send a command ####################### */
/* if (do_head)
 * strcpy(command, "HEAD ");
 * else if (do_meta)
 * strcpy(command, "META ");
 */

/* ################### some code to keepalive a socket */
/* addr looks like http://host.edu/blah/etc.html  lets crop it at the 3rd '/' */
/* for(p = arg,i=0;*p && i!=3;p++)
 * if(*p=='/') i++;
 * if(i==3)
/* i = p-arg; /* i = length not counting last '/' */
/* else
 * i = 0;                 
 * if((lsocket != -1) && i && addr && !strncmp(addr,arg,i)){
/* /* keepalive is active and addresses match -- try the old socket */
/* s = lsocket;           
/* keepingalive = 1;   /* flag network error due to server timeout*/
/* lsocket = -1;           /* prevent looping on failure */
/* } else {                       
 * if(addr)               
 * free(addr);    
/* /* save the address for next time around */  
/* addr = (char*) malloc(i+1);  
 * strncpy(addr,arg,i);   
 * *(addr+i)=0;           
/* keepingalive = 0; /* just normal opening of the socket */
/* if(lsocket != -1)      
/* close(lsocket); /* no socket leaks here */
/* lsocket = -1; /*dont assign until we know the server says okay */
/* }                              
 * if (!keepingalive) {           
 * status = HTDoConnect (arg, TCP_PORT, &s,appd);
 * }       
 */
/* ###################################### */
#endif
