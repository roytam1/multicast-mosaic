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

#ifndef __GUI_H__
#define __GUI_H__


void mo_process_external_directive (char *directive, char *url);

mo_window *mo_next_window (mo_window *);
mo_window *mo_fetch_window_by_id (int);
char *mo_assemble_help_url (char *);
mo_status mo_redisplay_window (mo_window *);
mo_status mo_set_dtm_menubar_functions (mo_window *);
mo_status mo_delete_window (mo_window *);
mo_window *mo_open_window (Widget, char *, mo_window *);
mo_window *mo_duplicate_window (mo_window *);
mo_window *mo_open_another_window (mo_window *, char *, char *, char *);
mo_status mo_open_initial_window (void);
void mo_gui_notify_progress (char *,mo_window * win);
int mo_gui_check_icon (int, mo_window *);
void mo_gui_done_with_icon ( mo_window *);
void mo_gui_clear_icon ( mo_window *);
void kill_splash();
void mo_gui_update_meter(int level,char *text,mo_window * win);
void stopBusyAnimation();
char *MakeFilename();
long GetCardCount(char *fname);
int anchor_visited_predicate (Widget, char *);
mo_status mo_post_access_document (mo_window *win, char *url,
                                          char *content_type,
                                          char *post_data);
XmxCallbackPrototype (menubar_cb);

void mo_gui_check_security_icon_in_win(int type, mo_window *win);

void mo_assemble_controls(mo_window *win, int detach);

extern void mo_do_gui();
void mo_gui_apply_default_icon(mo_window *);
void mo_flush_passwd_cache (mo_window *win);

#endif /* not __GUI_H__ */
