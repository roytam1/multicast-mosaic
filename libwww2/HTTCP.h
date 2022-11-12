/*                               /Net/dxcern/userd/timbl/hypertext/WWW/Library/src/HTTCP.html
                               GENERIC TCP/IP COMMUNICATION
                                             
   This module has the common code for handling TCP/IP connections etc.
   
 */
#ifndef HTTCP_H
#define HTTCP_H

#include "HTUtils.h"

#include "tcp.h"


/*      Produce a string for an internet address
**      ---------------------------------------
**
** On exit:
**           returns a pointer to a static string which must be copied if
**                it is to be kept.
*/
#ifdef IPV6
        extern char * HTInetString(struct sockaddr_in6 * sin);
#else
        extern char * HTInetString(struct sockaddr_in* sin);
#endif


/*      Publicly accessible variables
*/
/* extern struct sockaddr_in HTHostAddress; */
                        /* The internet address of the host */
                        /* Valid after call to HTHostName() */


/*      Get Name of This Machine
**      ------------------------
**
*/

extern WWW_CONST char * HTHostName NOPARAMS;

extern int HTDoConnect (char *, char *, int, int *);

extern int HTDoRead (int, void *, unsigned);

#endif   /* HTTCP_H */
