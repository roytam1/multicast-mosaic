/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#ifndef __POPUP_H__
#define __POPUP_H__

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "hotlist.h"
#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/PushBG.h>
#include <Xm/SeparatoG.h>
#include <Xm/CascadeBG.h>
#include <Xm/ToggleBG.h>
#include <Xm/RowColumn.h>
#include <Xm/FileSB.h>
#include <X11/Xatom.h>

#define MAX_NUM_POPUP_ITEMS 50
#define ALL_TYPES (E_TEXT | E_BULLET | E_LINEFEED | E_IMAGE | E_WIDGET | E_HRULE | E_TABLE | E_ANCHOR)
#define TIGHT 0
#define LOOSE 1
#define COPY_URL_LABEL "Copy Link URL"

#define NEWS_NOANCHOR (E_TEXT | E_BULLET | E_LINEFEED | E_WIDGET | E_HRULE |E_TABLE)

typedef enum _w_class {
	LastItem=1, PushButton, Separator, CascadeButton,
			ToggleButton, Widgt
} w_class;

enum { I_Save, I_ViewExternal, I_ViewInternal, I_Reload,                    
        M_ImageData, M_LinkData, M_FileData };

typedef struct act_struct {
	XtCallbackProc act_code;
	struct ele_rec *eptr;
	void *str;
	mo_window * win;
} act_struct;

typedef struct _PopupItem {
/* the top half must be filled in if this is to appear in the popup */

  w_class              classw; /* this is a button, separator, label, cascade */
  unsigned long int    types; /* for which widget elements this button is to 
				 popup for (the list of elements is below) */
  int                  types_method; /* if TIGHT use == if LOOSE use & */
  unsigned long int    modes; /* news, http, ftp, etc. */
  int                  modes_method; /* if TIGHT use == if LOOSE use & */
  char *label;

/* these are needed for a button class */
  struct act_struct    acst; /* identifies the action */
  void                 (*cbfp)(Widget,XtPointer,XtPointer); /* callback function that takes act_struct
				     as client data */
/* theses are optional */
  char                 mnemonic;
  char                 *accel_text;
  char                 *accel;

/* this is needed for a cascade class */
  struct  _PopupItem   *sub_items; /* NULL if this isn't a pull_right */
  int 			sub_items_size;
/* this is for internal uses */
   Widget               _w;
   int                  startup; /* are we sensitive when we start */
   mo_window 		*win;     /* which mo_window call me ? */
} PopupItem;

XmxCallbackPrototype (rbm_ballonify);
extern void mo_make_popup(mo_window * win);

char *getFileName(char *file_src);
void mo_popup_set_something(char *what, int to, PopupItem *items);

extern void PopSaveLinkFsbDialog(char * url);
extern void mo_destroy_popup(mo_window * win);
#endif
