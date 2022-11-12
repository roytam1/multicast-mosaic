/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#ifndef HOTLIST_H
#define HOTLIST_H

extern void MMHotlistInit( char * rootdir);

/*############*/
typedef enum {
	mo_t_url, mo_t_list, mo_t_none
} mo_item_type;

/* mo_any_item is any item in a mo_hotlist. */
typedef struct _mo_any_item {
	mo_item_type type;
	char *title; 		/* title for an URL, name for a hotlist */
	char * desc;		/* user comment */
	int position;		 /* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;
} mo_any_item;


/* mo_hotnode is a single item in a mo_hotlist. */
typedef struct _mo_hotnode {
	mo_item_type type;
	char *title;		 /* title for an URL */
	char * desc;		/* user comment */
	int position;		 /* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;

	char *url;
	char *lastdate;
} mo_hotnode;


/* mo_hotlist is a list of URL's and (cached) titles that can be
 *  added to and deleted from freely, and stored and maintained across
 *  sessions. */

typedef struct _mo_hotlist {
	mo_item_type type;
	char * title; 		/* name for a hotlist */
	char * desc;		/* user comment */
	int position;		/* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;

/* specific to mo_hotlist */
	struct _mo_hotlist *parent;
	union _mo_hot_item *nodelist;
/* Point to last element in nodelist for fast appends. */
	union _mo_hot_item *nodelist_last;
/* Flag set to indicate whether this hotlist has to be written
 * back out to disk at some point. */
	int modified;
} mo_hotlist;

/* mo_hot_item is the union of all item type */
typedef union _mo_hot_item {
	mo_item_type type;
	mo_any_item any;
	mo_hotnode hot;
	mo_hotlist list;
} mo_hot_item;

extern mo_hotlist * mMosaicHotList;
extern void mo_append_item_to_hotlist (mo_hotlist *list, mo_hot_item *node);
extern mo_status mo_write_hotlist (mo_hotlist *list, FILE *fp);
extern mo_status mo_add_item_to_hotlist (mo_hotlist *list, mo_item_type type,
           char *title, char *url, char * desc, int position);

#endif
