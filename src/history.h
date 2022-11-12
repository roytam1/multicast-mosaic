/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __HISTORY_H__
#define __HISTORY_H__

mo_status mo_free_node_data (mo_node *);
mo_status mo_kill_node (mo_window *, mo_node *);
mo_status mo_kill_node_descendents (mo_window *, mo_node *);
mo_status mo_add_node_to_history (mo_window *, mo_node *);
char *mo_grok_title (mo_window *, char *, char *);
mo_status mo_record_visit (mo_window *, char *, char *, char *, char *, char *);
mo_status mo_back_node (mo_window *);
mo_status mo_forward_node (mo_window *);
mo_status mo_visit_position (mo_window *, int);
mo_status mo_dump_history (mo_window *);
mo_status mo_post_history_win (mo_window *);

#endif
