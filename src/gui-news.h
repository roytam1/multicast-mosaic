/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __GUINEWS_H__
#define __GUINEWS_H__

mo_status mo_post_news_win (mo_window *);
mo_status mo_post_follow_win (mo_window *);
mo_status mo_post_generic_news_win (mo_window *, int follow);

void gui_news_post_subgroupwin (mo_window *win);
void gui_news_updateprefs (mo_window *win);
void gui_news_subgroup(mo_window *win);
void gui_news_unsubgroup(mo_window *win);
void gui_news_flush(mo_window *win);
void gui_news_flushgroup(mo_window *win);
void gui_news_list(mo_window *win);
void gui_news_showAllGroups (mo_window *win);
void gui_news_showGroups (mo_window *win);
void gui_news_showReadGroups (mo_window *win);
void gui_news_showAllArticles (mo_window *win);
void gui_news_showArticles (mo_window *win);
void gui_news_markGroupRead (mo_window *win);
void gui_news_markGroupUnread (mo_window *win);
void gui_news_markArticleUnread (mo_window *win);
void gui_news_initflush (mo_window *win);
void gui_news_index(mo_window *win);
void gui_news_prev(mo_window *win);
void gui_news_next(mo_window *win);
void gui_news_prevt(mo_window *win);
void gui_news_nextt(mo_window *win);
void news_status(char *url, int *prevt, int *nextt, int *prev, int *next, int *follow);

#endif

