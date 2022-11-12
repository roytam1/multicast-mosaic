/*                System dependencies in the W3 library

   System-system differences for TCP include files and macros. This
   file includes for each system the files necessary for network and
   file I/O.
   
  AUTHORS
  
  TBL                Tim Berners-Lee, W3 project, CERN, <timbl@info.cern.ch>
  EvA                     Eelco van Asperen <evas@cs.few.eur.nl>
  MA                      Marc Andreessen NCSA
  AT                      Aleksandar Totic <atotic@ncsa.uiuc.edu>
  SCW                     Susan C. Weber <sweber@kyle.eitech.com>
                         
  HISTORY:
  22 Feb 91               Written (TBL) as part of the WWW library.
  16 Jan 92               PC code from EvA
  22 Apr 93               Merged diffs bits from xmosaic release
  29 Apr 93               Windows/NT code from SCW

  Much of the cross-system portability stuff has been intentionally
  REMOVED from this version of the library by Marc A in order to
  discourage attempts to make "easy" ports of Mosaic for X to non-Unix
  platforms.  The library needs to be rewritten from the ground up; in
  the meantime, Unix is *all* we support or intend to support with
  this set of source code.
*/

#ifndef TCP_H
#define TCP_H

/* Default values

   These values may be reset and altered by system-specific sections
   later on.  there are also a bunch of defaults at the end .
*/

#ifdef unix
#define GOT_PIPE
#endif

#ifdef _AIX
#define AIX
#define unix
#endif

#ifdef _IBMR2
#define USE_DIRENT              /* sys V style directory open */
#endif

/* Solaris. */
#if defined(sun) && defined(__svr4__)
#define USE_DIRENT              /* sys V style directory open */
#endif

#if defined(__alpha)
#define USE_DIRENT
#endif

#ifndef USE_DIRENT
#ifdef SVR4
#define USE_DIRENT
#endif
#endif /* not USE_DIRENT */


/* SCO ODT unix version */
#ifdef sco
#include <sys/fcntl.h>
#define USE_DIRENT
#endif

/* MIPS unix */
/* Mips hack (bsd4.3/sysV mixture...) */

#ifdef mips
extern int errno;
#endif

#ifdef __QNX__
#include <sys/select.h>
#endif

/* Regular BSD unix versions
   These are a default unix where not already defined specifically.
 */
#include <sys/types.h>
#include <string.h>
#include <errno.h>          /* independent */
#include <sys/time.h>       /* independent */
#include <sys/stat.h>
#include <sys/param.h>
#include <sys/file.h>       /* For open() etc */

/*                      Directory reading stuff - BSD or SYS V */
#ifdef unix                    /* if this is to compile on a UNIX machine */
#ifdef USE_DIRENT             /* sys v version */
#include <dirent.h>
#define direct dirent
#else
#include <sys/dir.h>
#endif
#if defined(sun) && defined(__svr4__)
#include <sys/fcntl.h>
#include <limits.h>
#endif
#endif

/* Defaults INCLUDE FILES FOR TCP */

#include <sys/ioctl.h> /* EJB */
#include <sys/socket.h>
#include <netinet/in.h>
#ifndef __hpux /* this may or may not be good -marc */
#include <arpa/inet.h>      /* Must be after netinet/in.h */
#endif
#include <netdb.h>

/* MACROS FOR MANIPULATING MASKS FOR SELECT() */
#ifndef FD_SET
typedef unsigned int fd_set;
#define FD_SET(fd,pmask) (*(pmask)) |=  (1<<(fd))
#define FD_CLR(fd,pmask) (*(pmask)) &= ~(1<<(fd))
#define FD_ZERO(pmask)   (*(pmask))=0
#define FD_ISSET(fd,pmask) (*(pmask) & (1<<(fd)))
#endif  /* FD_SET */

#endif /* TCP_H */
