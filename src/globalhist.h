/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __GLOBALHIST_H__
#define __GLOBALHIST_H__

extern void MMWriteHistory();
extern void MMInitHistory(char * rootdir);
extern void MMUpdateGlobalHistory (char *aurl);

mo_status mo_been_here_before_huh_dad (char *);
mo_status mo_wipe_global_history (mo_window *);

#endif
