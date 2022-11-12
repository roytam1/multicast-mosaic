/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#ifndef NEWSRC_H
#define NEWSRC_H

#define naSUBSCRIBED 0x0001L
#define naUPDATE     0x0002L
#define naNEWGROUP   0x0004L
#define naSHOWME     0x0008L
#define naPOST       0x0010L
#define naSEQUENCED  0x0020L

typedef struct newsgroup_T {
  char *name;                    /* Group name */
  char *description;             /* Group description */
  long minart, maxart;           /* current article number information */
  long unread;                   /* current num of unread articles (0 if caught up) */
                                 /* may not be valid if group is not subscribed */
  long newsrcmin, newsrcmax;     /* most current newsrc information */
  char *read;                    /* Bitmask for read articles */
  long attribs;                  /* This group's attributes */

  struct newsgroup_T *next;      /* For the hash table */
  int h;
} newsgroup_t;


int isread (newsgroup_t *, long);
void markread (newsgroup_t *, long);
void markunread (newsgroup_t *, long);
void markrangeread (newsgroup_t *, long, long);
void markrangeunread (newsgroup_t *, long, long);
int initnewsrc (char *);
int killnewsrc ();
int flushnewsrc ();
newsgroup_t *issubscribed (char *);
newsgroup_t *subscribegroup (char *);
newsgroup_t *unsubscribegroup (char *);
newsgroup_t *findgroup (char *groupName);
newsgroup_t *addgroup (char *, long, long, int);
newsgroup_t *firstgroup (int mask);
newsgroup_t *nextgroup (newsgroup_t *);
void news_refreshprefs (void);
int newsrc_kill();
int newsrc_init (char *newshost);
void setminmax (newsgroup_t *ng, long min, long max);
void rereadseq (newsgroup_t *ng);
int newsrc_flush ();
int newsrc_flushgroup (newsgroup_t *n);
void newsrc_initflush ();

#endif
