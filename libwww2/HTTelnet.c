/*		Telnet Acees, Roligin, etc			HTAccess.c
**		==========================
**
** Authors
**	TBL	Tim Berners-Lee timbl@info.cern.ch
**	JFG	Jean-Francois Groff jgh@next.com
**	DD	Denis DeLaRoca (310) 825-4580  <CSP1DWD@mvs.oac.ucla.edu>
** History
**       8 Jun 92 Telnet hopping prohibited as telnet is not secure (TBL)
**	26 Jun 92 When over DECnet, suppressed FTP, Gopher and News. (JFG)
**	17 Dec 92 Tn3270 added, bug fix. (DD)
**	 2 Feb 93 Split from HTAccess.c. Registration.(TBL)
*/

/* Implements:
*/
#include <sys/types.h>
#include <unistd.h>
#include <errno.h>
#include <stdio.h>

#include "HText.h"
#include "HTTelnet.h"
#include "HTParse.h"
#include "HTUtils.h"
#include "HTAnchor.h"
#include "HTTP.h"
#include "HTFile.h"
#include "tcp.h"
#include "HTAccess.h"
#include "HTAlert.h"
#include "HTParams.h"

/*	make a string secure for passage to the
**	system() command.  Make it contain only alphanumneric
**	characters, or the characters '.', '-', '_', '+'.
**	Also remove leading '-' or '+'.
**	-----------------------------------------------------
*/
PRIVATE void make_system_secure ARGS1(char *, str)
{
	char *ptr1, *ptr2;

	if ((str == NULL)||(*str == '\0'))
		return;

	/*
	 * remove leading '-' or '+' by making it into whitespace that
	 * will be stripped later.
	 */
	if ((*str == '-')||(*str == '+'))
		*str = ' ';

	ptr1 = ptr2 = str;

	while (*ptr1 != '\0') {
		if ((!isalpha((int)*ptr1))&&(!isdigit((int)*ptr1))&&
		    (*ptr1 != '.')&&(*ptr1 != '_')&&
		    (*ptr1 != '+')&&(*ptr1 != '-')) {
			ptr1++;
		} else {
			*ptr2 = *ptr1;
			ptr2++;
			ptr1++;
		}
	}
	*ptr2 = *ptr1;
}


PRIVATE void run_a_command ARGS1(char *, command)
{
	char **argv;
	int argc;
	char *str;
	int alen;

	alen = 10;
	argv = (char **)malloc(10 * sizeof(char *));
	if (argv == NULL)
		return;
	argc = 0;

	str = strtok(command, " \t\n");
	while (str != NULL) {
		argv[argc] = strdup(str);
		argc++;
		if (argc >= alen) {
			int i;
			char **tmp_av;

			tmp_av = (char **)malloc((alen + 10) * sizeof(char *));
			if (tmp_av == NULL) {
				return;
			}
			for (i=0; i<alen; i++) {
				tmp_av[i] = argv[i];
			}
			alen += 10;
			free((char *)argv);
			argv = tmp_av;
		}
		str = strtok(NULL, " \t\n");
	}
	argv[argc] = NULL;

	if (fork() == 0) {
		execvp(argv[0], argv);
	} else {
		int i;

		/*
		 * The signal handler in main.c will clean this child
		 * up when it exits.
		 */

		for (i=0; i<argc; i++) {
			if (argv[i] != NULL) {
				free(argv[i]);
			}
		}
		free((char *)argv);
	}
}

/*	Telnet or "rlogin" access
**	-------------------------
*/
PRIVATE int remote_session (char * access, char * host, caddr_t appd)
{
  char *user, *hostname, *port;
  int portnum;
  char command[256];
  char *xterm_str;
  enum _login_protocol { telnet, rlogin, tn3270 } login_protocol;

  if (!access || !host) {
      HTapplication_user_feedback 
        ("Cannot open remote session, because\nURL is malformed.\0",appd);
      return HT_NO_DATA;
    }

  login_protocol =
    strcmp(access, "rlogin") == 0 ? rlogin :
      strcmp(access, "tn3270") == 0 ? tn3270 : 
        telnet;

  /* Make sure we won't overrun the size of command with a huge host string */
  if (strlen(host) > 200) {
	host[200] = '\0';
    }
  
  user = host;
  hostname = strchr(host, '@');
  port = strchr(host, ':');
  
  if (hostname) {
      *hostname++ = 0;	/* Split */
    } else {
      hostname = host;
      user = 0;		/* No user specified */
    }
  if (port) {
      *port++ = 0;	/* Split */
      portnum = atoi(port);
    }

  /*
   * Make user and hostname secure by removing leading '-' or '+'.
   * and allowing only alphanumeric, '.', '_', '+', and '-'.
   */
  make_system_secure(user);
  make_system_secure(hostname);
  
  xterm_str = wWWParams.global_xterm_str;
  
  if (login_protocol == rlogin) {
      /* For rlogin, we should use -l user. */
      if ((port)&&(portnum > 0)&&(portnum < 63336)) {
          sprintf(command, "%s -e %s %s %d %s %s", xterm_str, access,
              hostname,
              portnum,
              user ? "-l" : "",
              user ? user : "");
	} else {
          sprintf(command, "%s -e %s %s %s %s", xterm_str, access,
              hostname,
              user ? "-l" : "",
              user ? user : "");
	}
    } else {
      /* For telnet, -l isn't safe to use at all -- most platforms
         don't understand it. */
      if ((port)&&(portnum > 0)&&(portnum < 63336)) {
          sprintf(command, "%s -e %s %s %d", xterm_str, access,
              hostname, portnum);
	} else {
          sprintf(command, "%s -e %s %s", xterm_str, access,
              hostname);
	}
    }
  
  if (wWWParams.trace) fprintf(stderr, "HTaccess: Command is: %s\n", command);
  run_a_command(command);
  
  /* No need for application feedback if we're rlogging directly
     in... */
  if (user && login_protocol != rlogin) {
      char str[200];
      /* Sleep to let the xterm get up first.
         Otherwise, the popup will get buried. */
      sleep (2);
      sprintf (str, "When you are connected, log in as '%s'.", user);
      HTapplication_user_feedback (str,appd);
    }
  
  return HT_NO_DATA;		/* Ok - it was done but no data */
}

/*	"Load a document" -- establishes a session
**	------------------------------------------
**
** On entry,
**	addr		must point to the fully qualified hypertext reference.
**
** On exit,
**	returns		<0	Error has occured.
**			>=0	Value of file descriptor or socket to be used
**				 to read data.
**	*pFormat	Set to the format of the file, if known.
**			(See WWW.h)
**
*/
PRIVATE int HTLoadTelnet (WWW_CONST char *addr, HTParentAnchor *anchor,
 	HTFormat format_out,
 	HTStream * sink,		/* Ignored */
	caddr_t appd)
{
    char * access;
    
    char * host;
    int status;
    
    if (sink) 
      {
        HTAlert("Can't output a live session -- it has to be interactive",appd);
	return HT_NO_ACCESS;
      }
    access =  HTParse(addr, "file:", PARSE_ACCESS);
    
    host = HTParse(addr, "", PARSE_HOST);
    status = remote_session(access, host, appd);

    free(host);	
    free(access);
    return status;
}


PUBLIC HTProtocol HTTelnet = { "telnet", HTLoadTelnet };
PUBLIC HTProtocol HTRlogin = { "rlogin", HTLoadTelnet };
PUBLIC HTProtocol HTTn3270 = { "tn3270", HTLoadTelnet };


