/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* 
 * Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 *
 */

extern mo_status mo_post_mailto_win (mo_window *win, char *to_address, char *subject);
extern mo_status mo_send_mail_message (char *text, char *to, char *subj,
                                char *content_type, char *url);
extern FILE *mo_start_sending_mail_message (char *to, char *subj,
                                     char *content_type, char *url);
extern mo_status mo_finish_sending_mail_message (void);
