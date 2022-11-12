/*             Network News Transfer protocol module for the WWW library
                                       HTNEWS
 */
/* History:
**      26 Sep 90   Written TBL in Objective-C
**      29 Nov 91   Downgraded to C, for portable implementation.
**         Mar 96   Moved NewsArt here.  Upgraded back to C from Objectionable-C
*/

#ifndef HTNEWS_H
#define HTNEWS_H

#include "HTAccess.h"
#include "HTAnchor.h"
#include "../src/newsrc.h"
extern HTProtocol HTNews;

void news_status(char *url, int *prevt, int *nextt, int *prev, int *next, int *follow);

extern char * HTNewsHost;
extern int newsShowAllGroups;
extern int newsShowReadGroups;
extern int ConfigView;
extern int newsGotList;
extern char *NewsGroup;
extern newsgroup_t *NewsGroupS;
extern int newsShowAllArticles;

#define NO_CHANGE -1

void HTSetNewsConfig (int, int, int, int, int, int, int, int );

/* Thread Chain Structure */
typedef struct NEWSART {
    struct NEWSART *prev, *next, *prevt, *nextt;    /* Article List pointers */
    char *FirstRef, *LastRef;                       /* Thread List pointers */
    long num;                                       /* Article Header Info */
    char *ID;
    char *SUBJ;
    char *FROM;
} NewsArt;

extern NewsArt *CurrentArt;

#endif /* HTNEWS_H */
