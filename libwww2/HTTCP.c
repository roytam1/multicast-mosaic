/*			Generic Communication Code		HTTCP.c
**			==========================
**
**	This code is in common between client and server sides.
**
**	16 Jan 92  TBL	Fix strtol() undefined on CMU Mach.
**	25 Jun 92  JFG  Added DECNET option through TCP socket emulation.
*/

#include <stdio.h>

#ifdef __STDC__
#include <stdlib.h>
#endif
/* Apparently needed for AIX 3.2. */
#ifndef FD_SETSIZE
#define FD_SETSIZE 256
#endif
#if defined(SVR4) && !defined(SCO) && !defined(linux)
#include <sys/filio.h>
#include <unistd.h>
#endif

#ifdef SOLARIS
#ifdef  __cplusplus
extern "C" {
#endif

int gethostname(char *name, int namelen); /* because solaris 2.5 include bug */

#ifdef  __cplusplus
}
#endif
#endif

#include "HText.h"
#include "HTTCP.h"
#include "HTUtils.h"
#include "HTParse.h"
#include "HTAlert.h"
#include "HTAccess.h"
#include "tcp.h"
#include "HTParams.h"		/* params from X resources */


/*	Module-Wide variables */

PRIVATE char *hostname=0;		/* The name of this host */


/*	PUBLIC VARIABLES */

/* PUBLIC SockA HTHostAddress; */	/* The internet address of the host */
					/* Valid after call to HTHostName() */

/*	Encode INET status (as in sys/errno.h)			  inet_status()
**	------------------
**
** On entry,
**	where		gives a description of what caused the error
**	global errno	gives the error number in the unix way.
**
** On return,
**	returns		a negative status in the unix way.
*/
#ifndef errno
extern int errno;
#endif /* errno */

extern char *sys_errlist[];		/* see man perror on cernvax */
extern int sys_nerr;

/*	Produce a string for an Internet address
**	----------------------------------------
**
** On exit,
**	returns	a pointer to a static string which must be copied if
**		it is to be kept.
*/

PUBLIC WWW_CONST char * HTInetString ARGS1(SockA*,sin)
{
#ifdef IPV6
    static char string[512];
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
#else
    static char string[16];
    sprintf(string, "%d.%d.%d.%d",
	    (int)*((unsigned char *)(&sin->sin_addr)+0),
	    (int)*((unsigned char *)(&sin->sin_addr)+1),
	    (int)*((unsigned char *)(&sin->sin_addr)+2),
	    (int)*((unsigned char *)(&sin->sin_addr)+3));
#endif
    return string;
}


/*	Parse a network node address and port
**	-------------------------------------
**
** On entry,
**	str	points to a string with a node name or number,
**		with optional trailing colon and port number.
**	sin	points to the binary internet or decnet address field.
**
** On exit,
**	*sin	is filled in. If no port is specified in str, that
**		field is left unchanged in *sin.
*/
PUBLIC int HTParseInet ARGS2(SockA *,sin, WWW_CONST char *,str)
{
  char *port;
  char host[256];
  struct hostent  *phost;	/* Pointer to host - See netdb.h */
  int numeric_addr;
  char *tmp;
  
  static char *cached_host = NULL;
  static char *cached_phost_h_addr = NULL;
  static int cached_phost_h_length = 0;

  strcpy(host, str);		/* Take a copy we can mutilate */
  
  /* Parse port number if present */    
  if (port=strchr(host, ':')) {
      *port++ = 0;		/* Chop off port */
      if (port[0]>='0' && port[0]<='9') {
#ifdef IPV6
          sin->sin6_port = htons(atol(port));
#else
          sin->sin_port = htons(atol(port));
#endif
	}
    }
#ifdef IPV6
/* manage only non numeric adresses */
/* RFC 2133 */
/*          phost = gethostbyname2 (host,AF_INET6);
 * not yet on solaris */

	if (cached_host && (strcmp (cached_host, host) == 0)) {
		memcpy(&sin->sin6_addr,
			cached_phost_h_addr, cached_phost_h_length);
		return 0; /* OK */
	}
	phost = hostname2addr(host, AF_INET6);
	if (!phost) {
		fprintf(stderr,"IPV6 gasp no hosts \n");
		return -1;
	}
	if(phost->h_length != 16 ){
		fprintf(stderr,"IPV6 gasp h_lenght = %d\n", phost->h_length);
		return -1; /* Fail? */
	}
	if (cached_host) free (cached_host);
	if (cached_phost_h_addr) free (cached_phost_h_addr);

	/* Cache new stuff. */
	cached_host = strdup (host);
	cached_phost_h_addr = (char*)calloc (phost->h_length + 1, 1);
	memcpy (cached_phost_h_addr, phost->h_addr, phost->h_length);
	cached_phost_h_length = phost->h_length;
	memcpy(&sin->sin6_addr, phost->h_addr, phost->h_length);
#else
  
  /* Parse host number if present. */  
  numeric_addr = 1;
  for (tmp = host; *tmp; tmp++) { /* If there's a non-numeric... */
      if ((*tmp < '0' || *tmp > '9') && *tmp != '.') {
          numeric_addr = 0;
          goto found_non_numeric_or_done;
        }
    }
  
 found_non_numeric_or_done:
  if (numeric_addr) {   /* Numeric node address: */
      sin->sin_addr.s_addr = inet_addr(host); /* See arpa/inet.h */
    } else {		    /* Alphanumeric node name: */
      if (cached_host && (strcmp (cached_host, host) == 0)) {
          memcpy(&sin->sin_addr, cached_phost_h_addr, cached_phost_h_length);
        } else {
          phost = gethostbyname (host);
          if (!phost) {
if (wWWParams.trace) fprintf (stderr, 
"HTTPAccess: Can't find internet node name `%s'.\n",host);
              return -1;  /* Fail? */
            }

          /* Free previously cached strings. */
          if (cached_host) {
            free (cached_host);
	    cached_host=NULL;
	  }
          if (cached_phost_h_addr) {
            free (cached_phost_h_addr);
	    cached_phost_h_addr=NULL;
	  }

          /* Cache new stuff. */
          cached_host = strdup (host);
          cached_phost_h_addr = (char*)calloc (phost->h_length + 1, 1);
          memcpy (cached_phost_h_addr, phost->h_addr, phost->h_length);
          cached_phost_h_length = phost->h_length;
          memcpy(&sin->sin_addr, phost->h_addr, phost->h_length);
        }
    }
  
if (wWWParams.trace) fprintf(stderr,  
            "TCP: Parsed address as port %d, IP address %d.%d.%d.%d\n",
            (int)ntohs(sin->sin_port),
            (int)*((unsigned char *)(&sin->sin_addr)+0),
            (int)*((unsigned char *)(&sin->sin_addr)+1),
            (int)*((unsigned char *)(&sin->sin_addr)+2),
            (int)*((unsigned char *)(&sin->sin_addr)+3));

#endif /* IPV6 */
  return 0;	/* OK */
}

/*	Derive the name of the host on which we are
**	-------------------------------------------
**
*/
#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 64		/* Arbitrary limit */
#endif
PRIVATE void get_host_details(void)
{
    char name[MAXHOSTNAMELEN+1];	/* The name of this host */
#ifdef NEED_HOST_ADDRESS		/* no -- needs name server! */
    struct hostent * phost;		/* Pointer to host -- See netdb.h */
#endif
    int namelength = sizeof(name);
    
    if (hostname) return;		/* Already done */
    gethostname(name, namelength);	/* Without domain */

    if (wWWParams.trace) {
	fprintf(stderr, "TCP: Local host name is %s\n", name);
    }

    StrAllocCopy(hostname, name);

#ifdef NEED_HOST_ADDRESS		/* no -- needs name server! */
    phost=gethostbyname(name);		/* See netdb.h */
    if (!phost) {
	if (wWWParams.trace) fprintf(stderr, 
		"TCP: Can't find my own internet node address for `%s'!!\n",
		name);
	return;  /* Fail! */
    }
    StrAllocCopy(hostname, phost->h_name);
    memcpy(&HTHostAddress, &phost->h_addr, phost->h_length);
    if (wWWParams.trace) fprintf(stderr, "     Name server says that I am `%s' = %s\n",
	    hostname, HTInetString(&HTHostAddress));
#endif
}

PUBLIC char * HTHostName(void)
{
    get_host_details();
    return hostname;
}

PUBLIC int HTDoConnect (char *url, char *protocol, int default_port, int *s,
	caddr_t appd)
{
#ifdef IPV6
/* 4.3 BSD ipv6 based */
  struct sockaddr_in6 soc_address;
  struct sockaddr_in6 *sin6 = &soc_address;
#else
  struct sockaddr_in soc_address;
  struct sockaddr_in *sin = &soc_address;
#endif
  int status;

  /* Set up defaults: */
#ifdef IPV6
  sin6->sin6_family = AF_INET6;
  sin6->sin6_flowinfo = 0;
  sin6->sin6_port = htons(default_port);
#else
  sin->sin_family = AF_INET;
  sin->sin_port = htons(default_port);
#endif
  
  /* Get node name and optional port number: */
  {
    char line[256];
    char *p1 = HTParse(url, "", PARSE_HOST);

    sprintf (line, "Looking up %s.", p1);
    HTProgress (line,appd);

#ifdef IPV6
    status = HTParseInet(sin6, p1);
#else
    status = HTParseInet(sin, p1);
#endif
    if (status) {
        sprintf (line, "Unable to locate remote host %s.", p1);
        HTProgress(line,appd);
        free (p1);
        return HT_NO_DATA;
    }

    sprintf (line, "Making %s connection to %s.", protocol, p1);
    HTProgress (line,appd);
    free (p1);
  }

  /* Now, let's get a socket set up from the server for the data: */      
#ifdef IPV6
  *s = socket(AF_INET6, SOCK_STREAM, IPPROTO_TCP);
#else
  *s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
#endif

  /*
   * Make the socket non-blocking, so the connect can be canceled.
   * This means that when we issue the connect we should NOT
   * have to wait for the accept on the other end.
   */
  {
    int ret;
    int val = 1;
    char line[256];
    
    ret = ioctl(*s, FIONBIO, &val);
    if (ret == -1) {
        sprintf (line, "Could not make connection non-blocking.");
        HTProgress(line,appd);
    }
  }
  HTClearActiveIcon(appd);

  /*
   * Issue the connect.  Since the server can't do an instantaneous accept
   * and we are non-blocking, this will almost certainly return a negative
   * status.
   */
  status = connect(*s, (struct sockaddr*)&soc_address, sizeof(soc_address));

  /*
   * According to the Sun man page for connect:
   *     EINPROGRESS         The socket is non-blocking and the  con-
   *                         nection cannot be completed immediately.
   *                         It is possible to select(2) for  comple-
   *                         tion  by  selecting the socket for writ-
   *                         ing.
   * According to the Motorola SVR4 man page for connect:
   *     EAGAIN              The socket is non-blocking and the  con-
   *                         nection cannot be completed immediately.
   *                         It is possible to select for  completion
   *                         by  selecting  the  socket  for writing.
   *                         However, this is only  possible  if  the
   *                         socket  STREAMS  module  is  the topmost
   *                         module on  the  protocol  stack  with  a
   *                         write  service  procedure.  This will be
   *                         the normal case.
   */
#ifdef SVR4
  if ((status < 0) && ((errno == EINPROGRESS)||(errno == EAGAIN)))
#else
  if ((status < 0) && (errno == EINPROGRESS))
#endif /* SVR4 */
    {
      struct timeval timeout;
      int ret;

      ret = 0;
      while (ret <= 0) {
          fd_set writefds;
          int intr;
          
          FD_ZERO(&writefds);
          FD_SET(*s, &writefds);

	  /* linux (and some other os's, I think) clear timeout... 
	     let's reset it every time. bjs */
	  timeout.tv_sec = 0;
	  timeout.tv_usec = 100000;

#ifdef __hpux
          ret = select(FD_SETSIZE, NULL, (int *)&writefds, NULL, &timeout);
#else
          ret = select(FD_SETSIZE, NULL, &writefds, NULL, &timeout);
#endif
	  /*
	   * Again according to the Sun and Motorola man pagse for connect:
           *     EALREADY            The socket is non-blocking and a  previ-
           *                         ous  connection attempt has not yet been
           *                         completed.
           * Thus if the errno is NOT EALREADY we have a real error, and
	   * should break out here and return that error.
           * Otherwise if it is EALREADY keep on trying to complete the
	   * connection.
	   */
          if ((ret < 0)&&(errno != EALREADY)) {
              status = ret;
              break;
          } else if (ret > 0) {
	      /*
	       * Extra check here for connection success, if we try to connect
	       * again, and get EISCONN, it means we have a successful
	       * connection.
	       */
              status = connect(*s, (struct sockaddr*)&soc_address,
                               sizeof(soc_address));
              if ((status < 0)&&(errno == EISCONN))
                  status = 0;
              break;
          }
	  /*
	   * The select says we aren't ready yet.
	   * Try to connect again to make sure.  If we don't get EALREADY
	   * or EISCONN, something has gone wrong.  Break out and report it.
	   * For some reason SVR4 returns EAGAIN here instead of EALREADY,
	   * even though the man page says it should be EALREADY.
	   */
          else {
              status = connect(*s, (struct sockaddr*)&soc_address,
                               sizeof(soc_address));
#ifdef SVR4
              if ((status < 0)&&(errno != EALREADY)&&(errno != EAGAIN)&&
			(errno != EISCONN))
#else
              if ((status < 0)&&(errno != EALREADY)&&(errno != EISCONN))
#endif /* SVR4 */
                {
                  break;
                }
            }
          intr = HTCheckActiveIcon(1,appd);
          if (intr)
            {
              if (wWWParams.trace)
                fprintf (stderr, "*** INTERRUPTED in middle of connect.\n");
              status = HT_INTERRUPTED;
              errno = EINTR;
              break;
            }
	}
    }

  /*
   * Make the socket blocking again on good connect
   */
  if (status >= 0) {
      int ret;
      int val = 0;
      char line[256];
      
      ret = ioctl(*s, FIONBIO, &val);
      if (ret == -1) {
          sprintf (line, "Could not restore socket to blocking.");
          HTProgress(line,appd);
	}
  }
  /*
   * Else the connect attempt failed or was interrupted.
   * so close up the socket.
   */
  else
	close(*s);
  return status;
}

/* This is so interruptible reads can be implemented cleanly. */
int HTDoRead (int fildes, void *buf, unsigned nbyte, caddr_t appd)
{
  int ready, ret, intr;
  fd_set readfds;
  struct timeval timeout;
  char *adtestbuf;

  ready = 0;
  while (!ready) {
        FD_ZERO(&readfds);
        FD_SET(fildes, &readfds);

	  /* linux (and some other os's, I think) clear timeout... 
	     let's reset it every time. bjs */
	timeout.tv_sec = 0;
	timeout.tv_usec = 100000;
#ifdef __hpux
        ret = select(FD_SETSIZE, (int *)&readfds, NULL, NULL, &timeout);
#else
        ret = select(FD_SETSIZE, &readfds, NULL, NULL, &timeout);
#endif
        if (ret < 0)
                return -1;
        if (ret > 0) {
                ready = 1;
        } else {
                intr = HTCheckActiveIcon(1,appd);
                if (intr)
                        return HT_INTERRUPTED;
        }
  }
  ret = read (fildes, buf, nbyte);

  if (wWWParams.trace) {
	adtestbuf = (char*) buf;
	for (intr = 0; intr < ret; fprintf(stderr,"%c",adtestbuf[intr++]) ) ;
  }
  return ret;
}
