/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __GUIFTP_H__
#define __GUIFTP_H__

mo_status mo_handle_ftpput(mo_window *win);
mo_status mo_handle_ftpmkdir(mo_window *win);
mo_status mo_post_ftpput_window(mo_window *); 
mo_status mo_post_ftpremove_window(mo_window *); 
mo_status mo_post_ftpmkdir_window(mo_window *); 
mo_status mo_post_ftpbar_window(mo_window *); 

#endif
