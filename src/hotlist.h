/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#ifndef NCSA_HOTLIST_H
#define NCSA_HOTLIST_H

#define NCSA_HOTLIST_FORMAT_COOKIE_ONE "ncsa-xmosaic-hotlist-format-1"
#define NCSA_HOTLIST_FORMAT_COOKIE_TWO "<!-- ncsa-xmosaic-hotlist-format-2 -->"
#define NCSA_HOTLIST_FORMAT_COOKIE_THREE "<!-- ncsa-xmosaic-hotlist-format-3 -->"


typedef enum {
	mo_t_url, mo_t_list
} mo_item_type;


/* mo_any_item is any item in a mo_hotlist. */
typedef struct _mo_any_item {
	mo_item_type type;
	char *name; 		/* title for an URL, name for a hotlist */
	int position;		 /* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;
} mo_any_item;


/* mo_hotnode is a single item in a mo_hotlist. */
typedef struct _mo_hotnode {
	mo_item_type type;
	char *title;		 /* title for an URL */
	int position;		 /* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;
	char *url;
	char *lastdate;
	int rbm; /* Is this list on the RBM? */
} mo_hotnode;


/* mo_hotlist is a list of URL's and (cached) titles that can be
 *  added to and deleted from freely, and stored and maintained across
 *  sessions. */

typedef struct _mo_hotlist {
	mo_item_type type;
	char *name; 		/* name for a hotlist */
	int position;		/* Position in the list; starting at 1... */
	union _mo_hot_item *previous;
	union _mo_hot_item *next;

  		/* specific to mo_hotlist */
	struct _mo_hotlist *parent;
  		/* Point to last element in nodelist for fast appends. */
	union _mo_hot_item *nodelist;
	union _mo_hot_item *nodelist_last;
	int rbm; /* Is this list on the RBM? */
} mo_hotlist;


/* mo_root_hotlist is the root hotlist */
typedef struct _mo_root_hotlist {
	mo_item_type type;
	char *name; /* name for a hotlist */
  /* Position in the list; starting at 1... */
	int position;
	union _mo_hot_item *previous;
	union _mo_hot_item *next;

  /* specific to mo_hotlist */
	struct _mo_hotlist *parent;

	union _mo_hot_item *nodelist;
  /* Point to last element in nodelist for fast appends. */
	union _mo_hot_item *nodelist_last;
  /* Filename for storing this hotlist to local disk; example is
     $HOME/.mosaic-hotlist-default. */
	char *filename;

  /* Flag set to indicate whether this hotlist has to be written
     back out to disk at some point. */
	int modified;
} mo_root_hotlist;


/* mo_hot_item is the union of all item type */
typedef union _mo_hot_item {
	mo_item_type type;
	mo_any_item any;
	mo_hotnode hot;
	mo_hotlist list;
	mo_root_hotlist root;
} mo_hot_item;

extern void mo_append_item_to_hotlist (mo_hotlist *list, mo_hot_item *node);
extern char * mo_read_new_hotlist (mo_hotlist *list, FILE *fp);
extern mo_status mo_write_hotlist (mo_hotlist *list, FILE *fp);
extern mo_status mo_add_item_to_hotlist (mo_hotlist *list, mo_item_type type,
           char *title, char *url, int position,int rbm);

extern void mo_init_hotmenu();
void mo_rbm_myself_to_death(mo_window *win, int val);
#endif
