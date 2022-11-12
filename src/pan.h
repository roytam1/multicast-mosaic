/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __PAN_H__
#define __PAN_H__

mo_status mo_is_editable_pan (char *text);
mo_status mo_setup_pan_list (void);
mo_status mo_write_pan_list (void);
mo_status mo_new_pan (char *url, char *title, char *author, char *text);
mo_status mo_delete_pan (int id);
mo_status mo_modify_pan (int id, char *title, char *author, char *text);
char *mo_fetch_personal_annotations (char *url);
char *mo_fetch_pan_links (char *url, int on_top);
mo_status mo_grok_pan_pieces (char *url, char *t,
                              char **title, char **author, char **text,
                              int *id, char **fn);
int mo_next_pan_id (void);

#endif
