/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#ifndef __MOSAIC_H__
#define __MOSAIC_H__

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#if !defined(VMS) && !defined(NeXT)
#include <unistd.h>
#endif
#include <stdlib.h>
#include <sys/types.h>
#include <errno.h>

#ifdef __sgi
#include <malloc.h>
#endif

#include <Xm/XmAll.h>
#include <X11/StringDefs.h>
#include <X11/Xutil.h>
#include <X11/cursorfont.h>
#include "Xmx.h"
#include "toolbar.h"

typedef struct _MimeHeaderStruct *MimeHeaderStructPtr;

typedef enum {
	HTML_LOAD_CALLBACK,
	FRAME_CALLBACK
} GuiActionType;

typedef enum {
        MC_MO_TYPE_UNICAST,
        MC_MO_TYPE_MAIN,         /* only the Main can send */
        MC_MO_TYPE_RCV_URL_ONLY,
        MC_MO_TYPE_RCV_ALL
} McMoWType;


typedef struct _RequestDataStruct {
	char *req_url;
	char *post_data;
	char *ct;	/* Content-Type of post_data */
	int is_reloading;
	GuiActionType gui_action;
} RequestDataStruct;

typedef struct _TwirlStruct {
	Widget logo_widget;
	int logo_count;
	XtIntervalId time_id;
} TwirlStruct;

typedef struct {
					/* anchors */
	Boolean track_visited_anchors;
	Boolean track_pointer_motion;
	Boolean track_full_url_names;
					/* cache */
	Boolean reload_pragma_no_cache;
					/* strings (command name, file name) */
	char *sendmail_command;
	char *edit_command;
	char *xterm_command;
	char *mail_filter_command;
					/* document */
	char *home_document;                  
	char *docs_directory;
					/* fonts */
	char *default_font_choice;
					/* images */
	int colors_per_inlined_image;
	Boolean delay_object_loads;
					/* mail */
	char *author_full_name;
	char *author_name;
	char *author_email;
	char *signature;
	char *mail_mode;
					/* MIME */
	char *print_command;
	char *gunzip_command;
	Boolean tweak_gopher_types;
					/* news */
					/* printing */
	char *print_mode;
	Boolean print_us;
					/* services */
	int max_wais_responses;
	Boolean kiosk;
	Boolean kioskPrint;
	Boolean kioskNoExit;
	int ftp_timeout_val;
					/* window */
	int default_width;                    
	int default_height;                   
	Boolean initial_window_iconic;
	Boolean securityIcon;
					/* Save file stuff */
	char *save_mode;
					/* miscellaneous */
	char *full_hostname;
	int  load_local_file;
	Boolean edit_command_use_xterm;
	Boolean confirm_exit;
	Boolean catch_prior_and_next; 
	Boolean protect_me_from_myself;      
					/* new in 2.7 */
	Pixel meterForeground;
	Pixel meterBackground;
	Pixel meterFontForeground;
	Pixel meterFontBackground;
	char * acceptlanguage_str;
	int ftpRedial;
	Boolean use_screen_gamma;
	float screen_gamma;
					/* newer in 2.7 */
	Boolean wwwTrace;
	Boolean srcTrace;
	Boolean install_colormap;
	int urlExpired;
	int popupCascadeMappingDelay;
				/* newest in 2.7 (ha top that) */
	int newsBackgroundFlushTime;
#ifdef MULTICAST
	char           *mc_sess_name;
	char           *mc_media_name;
	int             mc_life_time;
	int             mc_ttl;
	char           *mc_alias_name;
	char		*mc_dest;		/* multicast dest addr/port */
#endif
					/* newest in 2.7b5 double haha; */
	int numberOfItemsInRBMHistory;
} AppData, *AppDataPtr;

extern char	*mMosaicRootDirName;	/* ~/.mMosaic is a directory */
					/* allocated at init. */
extern char	*mMosaicTmpDir;		/* working tmp dir */
extern char 	*mMosaicStartupDocument; /* the argv of command line */
extern char	*mMosaicMachineWithDomain;

extern AppData	mMosaicAppData;		/* Application resources of mMosaic */
extern XtAppContext	mMosaicAppContext;
extern int	mMosaicVisualClass;	/* visual class for 24bit support hack */
extern Widget	mMosaicToplevelWidget;	/* the toplevel widget */
extern Display *mMosaicDisplay;		/* the display */
extern Colormap mMosaicColormap;	/* the colormap */
extern Window	mMosaicRootWindow;	/* DefaultRootWindow */

extern Pixmap	mMosaicWinIconPixmap;
extern Pixmap	mMosaicWinIconMaskPixmap;
extern Cursor	mMosaicBusyCursor;

extern int	mMosaicSrcTrace; 

extern char	*mMosaicBalloonTranslationTable;

extern char 	*mMosaicAppVersion;

extern char	*mMosaicPersonalTypeMap;
extern char	*mMosaicPersonalExtensionMap;

/*###############################*/
typedef enum {
	mo_plaintext = 0, mo_formatted_text, mo_html, mo_latex, mo_postscript,
	mo_mif
} mo_format_token;

/* -------------------------------- ICONS --------------------------------- */
#define NUMBER_OF_FRAMES	25
#define ANIMATION_PIXMAPS	0
#define SECURITY_PIXMAPS	1
#define DIALOG_PIXMAPS		2

/* -------------------------------- MACROS -------------------------------- */

#define MO_VERSION_STRING "3.4.9"
#define MO_HELP_ON_VERSION_DOCUMENT \
	mo_assemble_help_url ("help-on-version-2.7b5.html")
#define MO_DEVELOPER_ADDRESS "mMosaic-dev@sig.enst.fr"

#ifndef DOCS_DIRECTORY_DEFAULT
#define DOCS_DIRECTORY_DEFAULT \
	"http://www.ncsa.uiuc.edu/SDG/Software/XMosaic"
#endif

/* This must be a straight string as it is included into a struct; no tricks. */
#ifndef HOME_PAGE_DEFAULT
#define HOME_PAGE_DEFAULT "http://www-sig.enst.fr/~dauphin/mMosaic/index.html"
#endif /* not HOME_PAGE_DEFAULT */

#ifndef WHATSNEW_PAGE_DEFAULT
#define WHATSNEW_PAGE_DEFAULT \
	"http://www-sig.enst.fr/~dauphin/mMosaic/change.html"
#endif /* not WHATSNEW_PAGE_DEFAULT */

#ifndef DEMO_PAGE_DEFAULT
#define DEMO_PAGE_DEFAULT \
	"http://www.ncsa.uiuc.edu/demoweb/demo.html"
#endif /* not DEMO_PAGE_DEFAULT */

#ifndef HTMLPRIMER_PAGE_DEFAULT
#define HTMLPRIMER_PAGE_DEFAULT \
	"http://www.ncsa.uiuc.edu/General/Internet/WWW/HTMLPrimer.html"
#endif /* not HTMLPRIMER_PAGE_DEFAULT */

#ifndef URLPRIMER_PAGE_DEFAULT
#define URLPRIMER_PAGE_DEFAULT \
	"http://www.ncsa.uiuc.edu/demoweb/url-primer.html"
#endif /* not URLPRIMER_PAGE_DEFAULT */

#ifndef NETWORK_STARTING_POINTS_DEFAULT
#define NETWORK_STARTING_POINTS_DEFAULT \
	"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/StartingPoints/NetworkStartingPoints.html"
#endif /* not NETWORK_STARTING_POINTS_DEFAULT */

#ifndef INTERNET_METAINDEX_DEFAULT
#define INTERNET_METAINDEX_DEFAULT \
	"http://www.ncsa.uiuc.edu/SDG/Software/Mosaic/MetaIndex.html"
#endif /* not INTERNET_METAINDEX_DEFAULT */


#if defined(bsdi)
#define MO_MACHINE_TYPE "BSD/OS"
#endif
#if defined(__hpux)
#define MO_MACHINE_TYPE "HP-UX"
#endif
#if defined(__sgi)
#define MO_MACHINE_TYPE "Silicon Graphics"
#endif
#if defined(ultrix)
#define MO_MACHINE_TYPE "DEC Ultrix"
#endif
#if defined(linux)
#define MO_MACHINE_TYPE "Linux"
#endif
#if defined(_IBMR2)
#define MO_MACHINE_TYPE "RS/6000 AIX"
#endif
#if defined(sun) && !defined(SOLARIS)
#define MO_MACHINE_TYPE "Sun"
#else
#if defined(SOLARIS)
#define MO_MACHINE_TYPE "Solaris"
#endif
#endif
#if defined(__alpha)
#define MO_MACHINE_TYPE "DEC Alpha"
#endif
#if defined(NEXT)
#define MO_MACHINE_TYPE "NeXT BSD"
#endif
#if defined(cray)
#define MO_MACHINE_TYPE "Cray"
#endif
#if defined(VMS)
#define MO_MACHINE_TYPE "VMS"
#endif
#if defined (SCO)
#define MO_MACHINE_TYPE "SCO Unix"
#endif /* SCO */

#ifndef MO_MACHINE_TYPE
#define MO_MACHINE_TYPE "Unknown Platform"
#endif

#ifdef __hpux
#define MO_SIGHANDLER_RETURNTYPE int
#else /* not __hpux */
#define MO_SIGHANDLER_RETURNTYPE void
#endif

#ifdef SVR4
#define MO_SIGHANDLER_ARGS int sig
#else /* not ultrix */
#define MO_SIGHANDLER_ARGS void
#endif

/* Be safe... some URL's get very long. */
#define MO_LINE_LENGTH 2048

#define MO_MAX(x,y) ((x) > (y) ? (x) : (y))
#define MO_MIN(x,y) ((x) > (y) ? (y) : (x))

#define public
#define private static

/*String #defs for Print/Mail/Save*/
#ifndef MODE_HTML
#define MODE_HTML "html"
#endif

#ifndef MODE_POSTSCRIPT
#define MODE_POSTSCRIPT "postscript"
#endif

#ifndef MODE_FORMATTED
#define MODE_FORMATTED "formatted"
#endif

#ifndef MODE_PLAIN
#define MODE_PLAIN "plain"
#endif

/* ------------------------------ MAIN TYPES ------------------------------ */

typedef struct _AgentSpoofCBStruct {
	Widget w;
	struct _mo_window * win;
	int d;
} AgentSpoofCBStruct;
/* ------------------------------ mo_window ------------------------------- */


typedef int * DependObjectTab;	/* tab of moid */

#define moMODE_PLAIN  0x0001
#define moMODE_FTP    0x0002
#define moMODE_NEWS   0x0004
#define moMODE_ALL    0x0007

typedef enum _NavigationType {NAVIGATE_NEW, NAVIGATE_OVERWRITE,
	NAVIGATE_BACK, NAVIGATE_FORWARD } NavigationType;

/* mo_window contains everything related to a single Document View
 * window, including subwindow details. */
typedef struct _mo_window {
	Widget base;
	int mode;
    
/* frame */
	char * frame_name;
	struct _mo_window * frame_parent;
	struct _mo_window ** frame_sons;
	int frame_sons_nbre;
	int frame_dot_index;
	int number_of_frame_loaded;

	NavigationType navigation_action;	/* how we navigate */
/* Subwindows. */
	Widget source_win;
	Widget save_win;
/*######*/
	Widget open_win;
	Widget mail_fsb_win;
	Widget mail_win;
	Widget mailhot_win;
	Widget edithot_win;
	Widget inserthot_win;
	Widget mailhist_win;
	Widget print_win;
	Widget history_win;
	Widget open_local_win;
	Widget hotlist_win;
	Widget techsupport_win;
	Widget news_win;           /* News Post/Followup*/
	Widget news_fsb_win;
	Widget news_sub_win;       /* News Subscribe Window */
	Widget src_search_win;         /* source window document search */
	Widget search_win;         /* internal document search */
	Widget mailto_win;
	Widget mailto_form_win;
	Widget links_win;     /* window with list of links */
	Widget links_list; /* widget holding the list itself */
	XmString *links_items;
	int links_count;

	char *ftp_site;

	Widget session_menu;
	Widget *session_items;  
	int num_session_items;
  

/* USER INTERFACE BITS 'n PIECES */
	struct toolbar tools[BTN_COUNT];
	Widget slab[7];
	int slabpart[8];
	int biglogo,smalllogo,texttools;

	XmxMenuRecord *menubar;

	Widget url_widget;
	Widget dest_widget;		/* for a load to file */

	Widget scrolled_win, view;
	Widget tracker_widget, logo, security;
	Widget button_rc, button2_rc, encrypt;
	Widget toolbarwin, topform;
	int toolset;
	int toolbardetached;
	int toolbarorientation;

	Widget meter, meter_frame;
	int meter_level,meter_width,meter_height;
	int meter_notext;
	Pixel meter_fg, meter_bg, meter_font_fg, meter_font_bg;
	int meter_fontW, meter_fontH;
	char *meter_text;
	XFontStruct *meter_font;

/* navigation data structure */
	struct mo_node *first_node;
	struct mo_node *current_node;

/* Document source window. */
	Widget source_text;
	Widget source_url_text;
	Widget source_date_text;
	XmxMenuRecord *format_optmenu;
	mo_format_token save_format; /* starts at 0 */

	Widget open_text;

	Widget mail_to_text;
	Widget mail_subj_text;
	XmxMenuRecord *mail_fmtmenu;
	int mail_format;

	Widget mailhot_to_text;
	Widget mailhot_subj_text;
	Widget mailhist_to_text;
	Widget mailhist_subj_text;

	Widget print_text;
	XmxMenuRecord *print_fmtmenu;
	mo_format_token print_format;
/*swp*/
	Widget print_a4_toggle_save;
	Widget print_a4_toggle_print;
	Widget print_a4_toggle_mail;
	Widget print_us_toggle_save;
	Widget print_us_toggle_print;
	Widget print_us_toggle_mail;
	Widget print_url_only;
	Widget print_doc_only;
 
	Widget history_list;

	Widget hotlist_list;
	Widget hotlist_label;
	Widget save_hotlist_win;
	Widget load_hotlist_win;
	struct _mo_hotlist *current_hotlist;
	union _mo_hot_item *hot_cut_buffer;

	Widget techsupport_text;

	Widget news_text;
	Widget news_text_from, news_text_subj, news_text_group;
/* news followup storage */
	char *newsfollow_artid;
	char *newsfollow_grp, *newsfollow_subj, *newsfollow_ref, *newsfollow_from;

	Widget mailto_text;
	Widget mailto_fromfield;
	Widget mailto_tofield;
	Widget mailto_subfield;
  
	Widget mailto_form_text;
	Widget mailto_form_fromfield;
	Widget mailto_form_tofield;
	Widget mailto_form_subfield;

	char *post_data;

	int font_size;
	int font_family;

	int underlines_snarfed;
	int underlines_state;
/* Default values only, mind you. */
	int underlines;
	int visited_underlines;
	Boolean dashed_underlines;
	Boolean dashed_visited_underlines;

	Widget delete_button;
	Widget include_fsb;
	int editing_id;

	Widget search_win_text;
	Widget search_caseless_toggle;
	Widget search_backwards_toggle;
	void *search_start;
	void *search_end;

	Widget src_search_win_text;
	Widget src_search_caseless_toggle;
	Widget src_search_backwards_toggle;
	int src_search_pos;

	int delay_object_loads;
/*SWP*/
	Boolean body_color;
	Boolean body_image;
	int image_view_internal;

/* PLB */
	Widget subgroup;
	Widget unsubgroup;

	struct _mo_window *next;

	int agent_state;
	AgentSpoofCBStruct * agspd_cbd;
	Widget agent_state_pulldown;

	struct _PafDocDataStruct * pafd;
#ifdef MULTICAST
	McMoWType mc_type;	/* MC_MO_TYPE_UNICAST   */
				/* MC_MO_TYPE_MAIN	*/
				/* only the Main can send */
				/* MC_MO_TYPE_RCV_URL_ONLY */
				/* MC_MO_TYPE_RCV_ALL */
	void (*mc_callme_on_new_object)(char *fname, char *aurl_wa, 
		MimeHeaderStructPtr mhs, DependObjectTab dot,
		int n_do, int *moid_ret);
	void (*mc_callme_on_error_object)(char *aurl, int status_code,
		int * moid_ret);
	void (*mc_callme_on_new_state)(struct _mo_window * win, int moid_ref, DependObjectTab dot, int ndo);
	struct _Source *source;

	int	moid_ref;	/* current moid at sender side */
	int	n_do;		/* number of depend object */
	DependObjectTab dot;	/* alldepend object */
#endif
	int delete_position_from_current_hotlist;
} mo_window;


typedef struct _BalloonInfoData {
        mo_window * win;
        char * msg;
} BalloonInfoData;

typedef struct {
        mo_window *win;
        char *fileName;
        char *url;
} EditFile;

/* ------------------------------- mo_node -------------------------------- */

/* mo_node is a component of the linear history list.  A single
   mo_node will never be effective across multiple mo_window's;
   each window has its own linear history list. */
typedef struct mo_node {
	char *title;
	char *aurl_wa;	/* the full absolute url with anchor */
	char *aurl;	/* THE absolute url of this document */
	char *base_url;	/* a tag <BASE> is dectect, else aurl */
	char *goto_anchor;	/* #target in url */
	char *base_target;	/* <BASE target="..." */
	char *last_modified;
	char *expires;
	char *text;	/* a copy of html text , need to be freed */
			/* when the node is released */
	struct mark_up * m_list;
	int position;	 /* Position in the list, starting at 1; last item is*/
			  /* effectively 0 (according to the XmList widget). */
	int docid; 	/* This is returned from HTMLPositionToId. */
	struct _MimeHeaderStruct * mhs;

	int authType; 	/* Type of authorization */
	struct mo_node *previous;
	struct mo_node *next;
} mo_node;

/* ------------------------------ MISC TYPES ------------------------------ */

typedef enum {
	mo_fail = 0, 
	mo_succeed
} mo_status;

/* ------------------------------- menubar -------------------------------- */

typedef enum {
	mo_regular_fonts_tkn,
	mo_small_fonts_tkn,
	mo_large_fonts_tkn,
	mo_large_helvetica_tkn,
	mo_small_helvetica_tkn,
	mo_regular_helvetica_tkn,
	mo_large_newcentury_tkn,
	mo_small_newcentury_tkn,
	mo_regular_newcentury_tkn,
	mo_large_lucidabright_tkn,
	mo_regular_lucidabright_tkn,
	mo_small_lucidabright_tkn,
	mo_default_underlines_tkn,
	mo_l1_underlines_tkn,
	mo_l2_underlines_tkn,
	mo_l3_underlines_tkn,
	mo_no_underlines_tkn
} mo_token;

/* ------------------------------ PROTOTYPES ------------------------------ */

/* gui.c */
extern int IconWidth, IconHeight, WindowWidth, WindowHeight;
extern Pixmap *IconPix,*IconPixSmall,*IconPixBig;

extern Pixmap securityKerberos4, securityBasic, securityMd5, securityNone,
	securityUnknown, securityKerberos5, securityDomain, securityLogin;

/* gui-extras.c */
extern mo_status mo_post_links_window(mo_window *);
extern mo_status mo_update_links_window(mo_window *);

/* called from libwww */
extern mo_status mo_post_open_local_window (mo_window *);
extern mo_status mo_post_open_window (mo_window *);
extern mo_status mo_post_mail_window (mo_window *);
extern mo_status mo_post_print_window (mo_window *);
extern mo_status mo_post_search_window (mo_window *);
extern mo_status mo_post_subscribe_win (mo_window *);

/* gui-menubar.c */
extern mo_status mo_set_fonts (mo_window *, int);
extern mo_status mo_set_underlines (mo_window *, int);
extern void	loadAgents(void);
/* hotlist.c */
extern mo_status mo_write_default_hotlist (void);
extern mo_status mo_post_hotlist_win (mo_window *);
extern mo_status mo_add_node_to_current_hotlist (mo_window *);

/* pixmaps.c */
extern void AnimatePixmapInWidget(Widget, Pixmap);
extern void MakeAnimationPixmaps(Widget);

/* callback menubar_cb */
#ifdef KRB5
void mo_kerberosv5_login(Widget w, XtPointer clid, XtPointer calld);
#endif
#ifdef MULTICAST
void mo_multicast_send_tog(Widget w, XtPointer clid, XtPointer calld);
void mo_multicast_show_participant(Widget w, XtPointer clid, XtPointer calld);
#endif

void mo_plaintext_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_formatted_text_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_postscript_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_html_cb(Widget w, XtPointer clid, XtPointer calld);

extern void InitChildProcessor();
extern mo_status mo_edit_source(mo_window *win);
extern mo_window *mo_make_window ( mo_window *parent, McMoWType mc_t);
extern mo_window *MMMakeSubWindow( mo_window *parent, Widget htmlw,
                        char *url, char *frame_name);

extern void System(char *cmd, char *title);
extern mo_window *mo_main_next_window (mo_window *win);
extern mo_status mo_source_date(mo_window *win);
extern void ShowNoProxyDialog(mo_window *win);
extern void ShowProxyDialog(mo_window *win);

mo_status mo_save_window(mo_window *win, char *fname,
                                        mo_format_token save_format);
mo_status mo_print_window(mo_window *win,
                        mo_format_token print_format, char *lpr);

mo_status mo_search_window(mo_window *win,char *str,int backward, int caselessi,
	int news);

#if 0
/* ACCES AUTHORIZATION part : extract from libwww2 */
/* The enumeration HTAAScheme represents the possible authentication schemes
 * used by the WWW Access Authorization. */
typedef enum {
    HTAA_UNKNOWN,
    HTAA_NONE,                        
    HTAA_BASIC,                       
    HTAA_PUBKEY,
    HTAA_KERBEROS_V4,
    HTAA_KERBEROS_V5,                 
    HTAA_MD5,                         
    HTAA_DOMAIN,
    HTAA_MAX_SCHEMES, /* THIS MUST ALWAYS BE LAST! Number of schemes */
    HTAA_LOGIN /*No...this must always be last because it is a FTP hack*/
} HTAAScheme;
#endif
#endif /* not __MOSAIC_H__ */
