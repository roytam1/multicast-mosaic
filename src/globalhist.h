/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */


#ifndef __GLOBALHIST_H__
#define __GLOBALHIST_H__

extern void MMWriteHistory();
extern void MMInitHistory(char * rootdir);

mo_status mo_been_here_before_huh_dad (char *);
mo_status mo_here_we_are_son (char *);
mo_status mo_init_global_history (void);
mo_status mo_wipe_global_history (mo_window *);
mo_status mo_cache_data (char *, void *, int);

#endif
