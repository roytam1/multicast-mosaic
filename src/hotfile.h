/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __HOTFILE_H__
#define __HOTFILE_H__

char * mo_read_new_hotlist(mo_hotlist *list, FILE *fp);
mo_status mo_write_hotlist (mo_hotlist *list, FILE *fp);

#endif
