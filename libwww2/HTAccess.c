/*		Access Manager					HTAccess.c
**		==============
**
** Authors
**	TBL	Tim Berners-Lee timbl@info.cern.ch
**	JFG	Jean-Francois Groff jfg@dxcern.cern.ch
**	DD	Denis DeLaRoca (310) 825-4580  <CSP1DWD@mvs.oac.ucla.edu>
** History
**       8 Jun 92 Telnet hopping prohibited as telnet is not secure TBL
**	26 Jun 92 When over DECnet, suppressed FTP, Gopher and News. JFG
**	17 Dec 92 Tn3270 added, bug fix. DD
**	 4 Feb 93 Access registration, Search escapes bad chars TBL
**		  PARAMETERS TO HTSEARCH AND HTLOADRELATIVE CHANGED
**	28 May 93 WAIS gateway explicit if no WAIS library linked in.
**
** Bugs
**	This module assumes that that the graphic object is hypertext, as it
**	needs to select it when it has been loaded.  A superclass needs to be
**	defined which accepts select and select_anchor.
*/

#ifndef DEFAULT_WAIS_GATEWAY
#define DEFAULT_WAIS_GATEWAY "http://www.ncsa.uiuc.edu:8001/"
#endif

#include <stdio.h>
#include "HText.h"	/* See bugs above */
#include "HTAccess.h"

#include "HTParse.h"
#include "HTUtils.h"
#include "HTML.h"		/* SCW */


#include "HTList.h"
#include "HTAlert.h"
#include "HTParams.h"	/* params from X resources */

#include "../src/proxy.h"

char *currentURL=NULL;
int has_fallbacks(char *protocol);

/*	To generate other things, play with these: */

PUBLIC HT_BOOL using_gateway = NO; /* are we using a gateway? */
PUBLIC HT_BOOL using_proxy = NO; /* are we using a proxy gateway? */
PUBLIC char *proxy_host_fix=NULL; /* Host: header fix */

PRIVATE HTList * protocols = NULL;   /* List of registered protocol descriptors */

extern HTProtocol HTTP, HTFile, HTTelnet, HTTn3270, HTRlogin;
extern HTProtocol HTFTP, HTNews, HTGopher, HTMailto, HTNNTP;
#ifdef DIRECT_WAIS
extern HTProtocol HTWAIS;
#endif

extern int useKeepAlive;
extern char *redirecting_url;

/*	Register a Protocol				HTRegisterProtocol
*/

PUBLIC HT_BOOL HTRegisterProtocol( HTProtocol * protocol)
{
    if (!protocols) protocols = HTList_new();
    HTList_addObject(protocols, protocol);
    return YES;
}

/*	Register all known protocols
**
**	Add to or subtract from this list if you add or remove protocol modules.
**	This routine is called the first time the protocol list is needed,
**	unless any protocols are already registered, in which case it is not called.
**	Therefore the application can override this list.
*/
PRIVATE void HTAccessInit NOARGS			/* Call me once */
{
	HTRegisterProtocol(&HTFTP);
	HTRegisterProtocol(&HTNews);
	HTRegisterProtocol(&HTGopher);
#ifdef DIRECT_WAIS
	HTRegisterProtocol(&HTWAIS);
#endif
	HTRegisterProtocol(&HTTP);
	HTRegisterProtocol(&HTFile);
	HTRegisterProtocol(&HTTelnet);
	HTRegisterProtocol(&HTTn3270);
	HTRegisterProtocol(&HTRlogin);
	HTRegisterProtocol(&HTMailto);
	HTRegisterProtocol(&HTNNTP);
}


/*		Find physical name and access protocol
** On entry,
**	addr		must point to the fully qualified hypertext reference.
**	anchor		a pareent anchor with whose address is addr
**
** On exit,
**	returns		HT_NO_ACCESS		Error has occured.
**			HT_OK			Success
*/
PRIVATE int get_physical ARGS3(
	char *,		addr,
	HTParentAnchor *, anchor,
	int,		bong)
{
	char * access=NULL;	/* Name of access method */
	char * physical = NULL;
	struct Proxy *GetNoProxy(char *access, char *site);
	char *tmp_access,*tmp_host,*ptr;

	HTAnchor_setPhysical(anchor, addr);
	access =  HTParse(HTAnchor_physical(anchor), "file:", PARSE_ACCESS);
	tmp_host = HTParse(HTAnchor_physical(anchor), "", PARSE_HOST);
/*
** set useKeepAlive to the default here because the last pass might
** have turned this off, and the default might have been on.
*/

	useKeepAlive=1;
/*	Check whether gateway access has been set up for this */
/* make sure the using_proxy variable is false */
	using_proxy = NO;
	if (proxy_host_fix) {
		free(proxy_host_fix);
		proxy_host_fix=NULL;
	}

	for(ptr=tmp_access=strdup(access); ptr && *ptr; ptr++) 
		*ptr=tolower(*ptr);
	for (ptr=tmp_host; ptr && *ptr; ptr++) 
		*ptr=tolower(*ptr);
	if (!GetNoProxy(tmp_access, tmp_host)) {
		char *gateway_parameter, *gateway, *proxy;
		struct Proxy *proxent = NULL, 
			*GetProxy(char *proxy, char *access, int fMatchEnd);
		char *proxyentry = NULL;

		proxy_host_fix=strdup(tmp_host);
/* search for gateways */
		gateway_parameter = (char *)malloc(strlen(tmp_access)+20);
		if (gateway_parameter == NULL) outofmem(__FILE__, "HTLoad");
		strcpy(gateway_parameter, "WWW_");
		strcat(gateway_parameter, tmp_access);
		strcat(gateway_parameter, "_GATEWAY");
		gateway = (char *)getenv(gateway_parameter); /* coerce for decstation */

/* search for proxy servers */
		strcpy(gateway_parameter, tmp_access);
		strcat(gateway_parameter, "_proxy");
		proxy = (char *)getenv(gateway_parameter);
		free(gateway_parameter);

		/*
		 * Check the proxies list
		 */
		if ((proxy == NULL)||(proxy[0] == '\0')) {
			int fMatchEnd;
			char *scheme_info;

			scheme_info =HTParse(HTAnchor_physical(anchor), "", PARSE_HOST);
			fMatchEnd = 1; /* match hosts from the end */

			if ((scheme_info != NULL) && (*scheme_info == '\0')) {
				free(scheme_info);
				scheme_info = HTParse(HTAnchor_physical(anchor), "", PARSE_PATH);
				fMatchEnd = 0; /* match other scheme_info at beginning*/
			}

			if (bong) { /* this one is bad - disable! */
				proxent = GetProxy(tmp_access, scheme_info, fMatchEnd);
				if (proxent != NULL) proxent->alive = bong;
			}
			proxent = GetProxy(tmp_access, scheme_info, fMatchEnd);
			if (proxent != NULL) {
				useKeepAlive = 0; /* proxies don't keepalive */
				StrAllocCopy(proxyentry, proxent->transport);
				StrAllocCat(proxyentry, "://");
				StrAllocCat(proxyentry, proxent->address);
				StrAllocCat(proxyentry, ":");
				StrAllocCat(proxyentry, proxent->port);
				StrAllocCat(proxyentry, "/");
				proxy = proxyentry;
			}
		}
#ifndef DIRECT_WAIS
		if (!gateway && 0==strcmp(tmp_access, "wais")) {
			gateway = DEFAULT_WAIS_GATEWAY;
		}
#endif
/* proxy servers have precedence over gateway servers */
		if (proxy) {
			char * gatewayed;

			gatewayed = NULL;
			StrAllocCopy(gatewayed,proxy);
			StrAllocCat(gatewayed,addr);
			using_proxy = YES;
			HTAnchor_setPhysical(anchor, gatewayed);
			free(gatewayed);
			free(access);
			if (proxyentry)
				free(proxyentry);
			access =  HTParse(HTAnchor_physical(anchor),
						"http:", PARSE_ACCESS);
		} else if (gateway) {
			char * gatewayed;

			gatewayed = NULL;
			StrAllocCopy(gatewayed,gateway);
			StrAllocCat(gatewayed,addr);
			using_gateway = YES;
			HTAnchor_setPhysical(anchor, gatewayed);
			free(gatewayed);
			free(access);
			access =  HTParse(HTAnchor_physical(anchor),
						"http:", PARSE_ACCESS);
		} else {
			if (proxy_host_fix) {
				free(proxy_host_fix);
			}
			proxy_host_fix=NULL;
			using_proxy = NO;
			using_gateway = NO;
			ClearTempBongedProxies();
		}
	}
	free(tmp_access);
	free(tmp_host);

/*	Search registered protocols to find suitable one */

	{
		int i, n;

		if (!protocols) HTAccessInit();
		n = HTList_count(protocols);
		for (i=0; i<n; i++) {
			HTProtocol *p=(HTProtocol *)HTList_objectAt(protocols, i);

			if (strcmp(p->name, access)==0) {
				HTAnchor_setProtocol(anchor, p);
				free(access);
				return (HT_OK);
			}
		}
	}
	free(access);
	return HT_NO_ACCESS;
}

/*		Load a document
**		---------------
**	This is an internal routine, which has an address AND a matching
**	anchor.  (The public routines are called with one OR the other.)
**
** On entry,
**	addr		must point to the fully qualified hypertext reference.
**	anchor		a pareent anchor with whose address is addr
**
** On exit,
**	returns		<0		Error has occured.
**			HT_LOADED	Success
**			HT_NO_DATA	Success, but no document loaded.
**					(telnet sesssion started etc)
*/
PRIVATE int HTLoad(WWW_CONST char * addr, HTParentAnchor * anchor,
	HTFormat format_out, caddr_t appd)
{
	HTProtocol* p;
	int ret, status = get_physical(addr, anchor, 0);
	int retry=5;
	static char *buf1="Do you want to disable the proxy server:\n\n";
	static char *buf2="\n\nAlready attempted 5 contacts.";
	char *finbuf,*host;
	int fallbacks;

    /*
     * Yes...I know I'm only calling this to get the "name", but it is better
     *   than looping through all of the proxy list everytime a proxy fails!
     *   --SWP
     */
	p = (HTProtocol*)HTAnchor_protocol(anchor);
	if (!p) {
		return(HT_NOT_LOADED);
	}
	fallbacks=has_fallbacks(p->name);

	while (1) {
		if (status < 0) return status;	/* Can't resolve or forbidden */
		retry=5;

retry_proxy:
		p = (HTProtocol*)HTAnchor_protocol(anchor);
		ret = (*(p->load))(HTAnchor_physical(anchor),
					anchor, format_out, NULL,appd);

		if (ret==HT_INTERRUPTED || HTCheckActiveIcon(0,appd)==HT_INTERRUPTED) {
			if (using_proxy) {
				ClearTempBongedProxies();
			}
			return(HT_INTERRUPTED);
		}

/* HT_REDIRECTING supplied by Dan Riley -- dsr@lns598.lns.cornell.edu */
		if (!using_proxy || !fallbacks || ret == HT_LOADED 
		    || ret == HT_REDIRECTING 
		    || (ret == HT_NO_DATA && strncmp((char *)anchor, "telnet", 6) == 0)) {
			if (using_proxy) {
				ClearTempBongedProxies();
			}
			return(ret);
		}
		if (retry>0) {
			retry--;
			HTProgress("Retrying proxy server...",appd);
			goto retry_proxy;
		}

/* must be using proxy and have a problem to get here! */

		host=HTParse(HTAnchor_physical(anchor), "", PARSE_HOST);
		finbuf=(char *)calloc(strlen(host)+ strlen(buf1)+ strlen(buf2)+ 5,
					sizeof(char));
		sprintf(finbuf,"%s%s?%s",buf1,host,buf2);
		if (HTConfirm(finbuf,appd)) {
			free(finbuf);
			finbuf=(char *)calloc((strlen(host)+
				strlen("Disabling proxy server ")+
				strlen(" and trying again.")+ 5),
				sizeof(char));
			sprintf(finbuf,"Disabling proxy server %s and trying again.",
				host);
			HTProgress(finbuf,appd);
			HTapplication_user_feedback(finbuf,appd);
			free(finbuf);
			finbuf=NULL;
			status = get_physical(addr, anchor, 1); /* Perm disable */
		} else if (HTConfirm("Try next fallback proxy server?",appd)) {
			status = get_physical(addr, anchor, 2); /* Temp disable */
		}
/* else -- Try the same one again */
		if (finbuf) {
			free(finbuf);
		}
	}
}

/* This is exported all the way to gui-documents.c at the moment,
   to tell mo_load_window_text when to use a redirected URL instead. */

char *use_this_url_instead;

/*		Load a document from absolute name
**    On Entry,
**        addr     The absolute address of the document to be accessed.
**        filter   if YES, treat document as HTML
**    On Exit,
**        returns    1     Success in opening document
**                   0      Failure
**                  -1      Interrupted
*/

PUBLIC int HTLoadAbsolute ( char *addr, caddr_t appd)
{
	HTParentAnchor *anchor;
	HTFormat format_out;
	int	        status;

	if (currentURL)
		free(currentURL);
	currentURL=strdup(addr);

	anchor = HTAnchor_parent(HTAnchor_findAddress(addr));
	format_out = WWW_PRESENT;

	use_this_url_instead = NULL;
try_again:
	if (wWWParams.trace)
		fprintf(stderr,"HTAccess: loading doc. %s\n", addr);
	status = HTLoad(addr, anchor, format_out, appd);
	if (status == HT_LOADED) {
		if (wWWParams.trace) {
			fprintf(stderr, "HTAccess: `%s' has been accessed.\n",
				addr);
		}
		return 1;
	}
	if (status == HT_REDIRECTING) {
/* Exported from HTMIME.c, of all places. */
		if (wWWParams.trace) {
			fprintf(stderr,"HTAccess:'%s' is a redirection URL.\n",
					addr);
			fprintf(stderr,"HTAccess: Redirecting to '%s'\n",
				redirecting_url);
		}
		addr = redirecting_url;
		use_this_url_instead = addr;
		goto try_again;
	}
	if (status == HT_INTERRUPTED) {
		if (wWWParams.trace)
			fprintf (stderr, "HTAccess: We were interrupted.\n");
		return -1;
	}
	if (status == HT_NO_DATA) {
		if (wWWParams.trace)
			fprintf(stderr,
			   "HTAccess: `%s' has been accessed, No data left.\n",
				addr);
		return 0;
	}
	if (status<0) {		      /* Failure in accessing a document */
		if (wWWParams.trace) fprintf(stderr,
			"HTAccess: Can't access `%s'\n", addr);
		return 0;
	}
/* If you get this, then please find which routine is returning
       a positive unrecognised error code! */
	if (wWWParams.trace)
		fprintf(stderr,
			"**** HTAccess: socket or file number %d returned by obsolete load routine!\n", status);
	return 0;
}
