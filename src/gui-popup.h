/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

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

typedef enum _w_class { LastItem=1, PushButton, Separator, CascadeButton,
			ToggleButton, Widgt } w_class;
enum { I_Save, I_ViewExternal, I_ViewInternal, I_Reload,                    
        M_ImageData, M_LinkData, M_FileData };

typedef struct act_struct {
  XtCallbackProc act_code;
  struct ele_rec *eptr;
  void *str;
  mo_window * win;
} act_struct;

typedef struct PopupItem {
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
  struct               PopupItem *sub_items; /* NULL if this isn't a 
						    pull_right */
  /* this is for internal uses */
    Widget               _w;
    int                  startup; /* are we sensitive when we start */
} PopupItem;

XmxCallbackPrototype (image_cb);
XmxCallbackPrototype (metadata_cb);
XmxCallbackPrototype (user_defs_cb);
XmxCallbackPrototype (ftp_rmbm_cb);
XmxCallbackPrototype (fsb_OKCallback);
XmxCallbackPrototype (fsb_CancelCallback);
XmxCallbackPrototype (copy_link_cb);
XmxCallbackPrototype (session_cb);
XmxCallbackPrototype (rbm_ballonify);

char *getFileName(char *file_src);
Widget  _PopupMenuBuilder(Widget parent, int type, char *title, 
				 char mnem, PopupItem *items);
void _set_eptr_field();
void mo_popup_set_something(char *what, int to, PopupItem *items);
PopupItem *popup_build_user_defs();
extern mo_window *current_win;
char *my_chop(char *str);
char *my_strndup(char *str, int num);
extern char *mo_escape_part();

void mo_make_popup();
void mo_init_hotmenu();
void mo_add_to_rbm_history(mo_window *win, char *url, char *title);

static Boolean convert_selection(Widget, Atom *, Atom *, Atom *, XtPointer *,
				 unsigned long *, int *);

extern Boolean have_popup;
extern Widget popup ;

extern PopupItem image_menu[];
extern PopupItem pan_menu[];
extern PopupItem photo_cd_sub_menu[];
extern PopupItem file_menu[];
extern PopupItem popup_items[];

extern void mo_init_hotlist_menu(mo_hotlist *list);
extern void mo_reinit_hotlist_menu(mo_hotlist *list);
extern void mo_add_to_rbm_history(mo_window *win, char *url, char *title);
extern void mo_delete_rbm_history_win(mo_window *win);
#endif
