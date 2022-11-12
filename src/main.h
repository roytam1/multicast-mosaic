/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* 
 * Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 *
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __MAIN_H__
#define __MAIN_H__

#ifdef __hpux
#define MO_SIGHANDLER_RETURNTYPE int
#else /* not __hpux */
#define MO_SIGHANDLER_RETURNTYPE void
#endif

void mo_exit (void);


#endif /* not  __MAIN_H__ */
