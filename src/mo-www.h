
#ifndef MO_WWW_H
#define MO_WWW_H

extern void application_error(char *str, char *title);
extern int prompt_for_yes_or_no (char *questionstr, mo_window * win);
extern char *prompt_for_string (char *questionstr,mo_window * win);
extern char *prompt_for_password (char *questionstr,mo_window * win);
extern char *mo_post_pull_er_over (char *url, char *content_type, char *data,
	mo_window * win);
extern void application_user_feedback (char *str,mo_window * win);
extern char *mo_pull_er_over (char *url,mo_window * win);
extern mo_status mo_pull_er_over_virgin (char *url, char *fnam, mo_window *win);
#endif
