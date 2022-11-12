/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

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
#include "cci.h"

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
	Boolean delay_image_loads;
					/* mail */
	char *author_full_name;
	char *author_name;
	char *author_email;
	char *signature;
	char *mail_mode;
					/* MIME */
	char *print_command;
	char *uncompress_command;
	char *gunzip_command;
	Boolean use_default_extension_map;
	Boolean use_default_type_map;
	Boolean tweak_gopher_types;
					/* news */
					/* printing */
	char *print_mode;
	Boolean print_banners;
	Boolean print_footnotes;
	Boolean print_us;
					/* services */
	int cciPort;
	int max_num_of_cci_connections;
	int max_wais_responses;
	Boolean kiosk;
	Boolean kioskPrint;
	Boolean kioskNoExit;
	int ftp_timeout_val;
					/* window */
	int default_width;                    
	int default_height;                   
	Boolean initial_window_iconic;
	Boolean twirling_transfer_icon;
	Boolean securityIcon;
	int twirl_increment;
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
	char * meterForeground;
	char * meterBackground;
	char * meterFontForeground;
	char * meterFontBackground;
	char * acceptlanguage_str;
	int ftpRedial;
	int ftpFilenameLength;
	int ftpEllipsisLength;
	int ftpEllipsisMode;
	Boolean use_screen_gamma;
	float screen_gamma;
					/* newer in 2.7 */
	Boolean wwwTrace;
	Boolean htmlwTrace;
	Boolean cciTrace;
	Boolean srcTrace;
	Boolean install_colormap;
	Boolean imageViewInternal;
	int urlExpired;
	int popupCascadeMappingDelay;
				/* newest in 2.7 (ha top that) */
	int newsBackgroundFlushTime;
#ifdef MULTICAST
	Boolean         mc_debug;              /* debug or verbose */
	char           *mc_sess_name;
	char           *mc_media_name;
	int             mc_life_time;
	int             mc_ttl;
	char           *mc_alias_name;
	char	   *mc_dest;		/* multicast dest addr/port */
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

extern int	mMosaicCCITrace; 
extern int	mMosaicSrcTrace; 


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

#define MO_VERSION_STRING "3.2.3"
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
#if defined(NeXT)
#define MO_MACHINE_TYPE "NeXT"
#endif
#if defined (SCO)
#if defined (_SCO_DS)
#define MO_MACHINE_TYPE "SCO OpenServer 5"
#else /* _SCO_DS */
#define MO_MACHINE_TYPE "SCO Unix"
#endif /* _SCO_DS */
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
	struct mo_window * win;
	int d;
} AgentSpoofCBStruct;
/* ------------------------------ mo_window ------------------------------- */


#define moMODE_PLAIN  0x0001
#define moMODE_FTP    0x0002
#define moMODE_NEWS   0x0004
#define moMODE_ALL    0x0007

/* mo_window contains everything related to a single Document View
 * window, including subwindow details. */
typedef struct mo_window {
	Widget base;
	int mode;
    
/* Subwindows. */
	Widget source_win;
	Widget save_win;
	Widget upload_win;
	Widget savebinary_win;  /* for binary transfer mode */
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
	Widget cci_win;	     /* common client interface control window */
	Widget mailto_win;
	Widget mailto_form_win;
	Widget links_win;     /* window with list of links */
	Widget links_list; /* widget holding the list itself */
	XmString *links_items;
	int links_count;

	Widget ftpput_win, ftpremove_win, ftpremove_text, 
		ftpmkdir_win, ftpmkdir_text;
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

	Widget url_text;
	Widget scrolled_win, view;
	Widget tracker_label, logo, security;
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

	struct mo_node *history;
	struct mo_node *current_node;

	char *target_anchor;

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
	Widget print_header_toggle_save;
	Widget print_header_toggle_print;
	Widget print_header_toggle_mail;
	Widget print_footer_toggle_save;
	Widget print_footer_toggle_print;
	Widget print_footer_toggle_mail;
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

	char *cached_url;

	Widget search_win_text;
	Widget search_caseless_toggle;
	Widget search_backwards_toggle;
	void *search_start;
	void *search_end;

	Widget src_search_win_text;
	Widget src_search_caseless_toggle;
	Widget src_search_backwards_toggle;
	int src_search_pos;

	Widget cci_win_text;
	Widget cci_accept_toggle;
	Widget cci_off_toggle;

	int binary_transfer;
	int delay_image_loads;
/*SWP*/
	Boolean body_color;
	Boolean body_images;
	int image_view_internal;

/* PLB */
	Widget subgroup;
	Widget unsubgroup;

	struct mo_window *next;

	int agent_state;
	AgentSpoofCBStruct * agspd_cbd;
	Widget agent_state_pulldown;

#ifdef MULTICAST
	McMoWType mc_type;	/* MC_MO_TYPE_UNICAST   */
				/* MC_MO_TYPE_MAIN	*/
				/* only the Main can send */
				/* MC_MO_TYPE_RCV_URL_ONLY */
				/* MC_MO_TYPE_RCV_ALL */
	struct _mc_user *mc_user;
#endif
	int delete_position_from_current_hotlist;
} mo_window;

typedef struct {
        mo_window *win;
        char *fileName;
        char *url;
} EditFile;

#ifdef MULTICAST
#include "../libmc/mc_defs.h"
extern IPAddr 	mc_addr_ip_group;
extern unsigned short 	mc_port;
extern unsigned short 	mc_rtcp_port;
#endif

/* ------------------------------- mo_node -------------------------------- */

/* mo_node is a component of the linear history list.  A single
   mo_node will never be effective across multiple mo_window's;
   each window has its own linear history list. */
typedef struct mo_node
{
	char *title;
	char *url;
	char *last_modified;
	char *expires;
	char *ref;	/*how the node was referred to from a previous anchor,*/
			/*if such an anchor existed. */
	char *text;	/* a copy of html text , need to be freed */
			/* when the node is released */
	int position;	 /* Position in the list, starting at 1; last item is*/
			  /* effectively 0 (according to the XmList widget). */
	int docid; 	/* This is returned from HTMLPositionToId. */
	int authType; 	/* Type of authorization */
	struct mo_node *previous;
	struct mo_node *next;
} mo_node;

/* ------------------------------ MISC TYPES ------------------------------ */

typedef enum {
	mo_fail = 0, 
	mo_succeed
} mo_status;

/* ---------------------------- a few globals ----------------------------- */

extern Display *dsp;
extern XtAppContext app_context;
#ifdef MULTICAST
extern int 			mc_debug;
extern int 			mc_fdread; 
extern int 			mc_rtcp_fdread; 
extern int 			mc_fdwrite;
extern int 			mc_rtcp_fdwrite;
extern time_t 			mc_init_gmt;
extern unsigned short  		mc_my_pid;
extern unsigned char 		mc_len_alias ;
extern char *			mc_alias_name;
extern unsigned int		mc_local_url_id;
extern unsigned char		mc_len_local_url;
extern char                     mc_local_url[MC_MAX_URL_SIZE+1];
extern char * 			mc_sess_name;
extern char * 			mc_media_name;
extern time_t			mc_init_local_time;
extern unsigned char		mc_ttl;
#endif

/* ------------------------------- menubar -------------------------------- */

typedef enum
{
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
	mo_no_underlines_tkn,

  mo_ftp_remove,
  mo_ncsa_document_tkn,
  mo_cc_tkn,
  mo_checkout_tkn, mo_checkin_tkn

/*##############*/
   /*mo_news_sub_anchor, mo_news_unsub_anchor,*/
   /*mo_news_mread_anchor,*/
/* Password cash stuff */
   /*mo_clear_passwd_cache,*/

} mo_token;

/* ------------------------------ PROTOTYPES ------------------------------ */

/* gui.c */
extern mo_window *mo_fetch_window_by_id (int);
extern char *mo_assemble_help_url (char *);
extern mo_status mo_redisplay_window (mo_window *);
extern mo_status mo_set_dtm_menubar_functions (mo_window *);
extern mo_status mo_delete_window (mo_window *);
extern mo_window *mo_duplicate_window (mo_window *);
extern mo_window *mo_open_another_window (mo_window *, char *, char *, char *);
extern mo_status mo_open_initial_window (void);
extern int IconWidth, IconHeight, WindowWidth, WindowHeight;
extern Pixmap *IconPix,*IconPixSmall,*IconPixBig;
extern void MoCCISendEventOutput(CCI_events event_type);

extern int force_dump_to_file;
extern char *HTAppVersion;
char *MakeFilename();
long GetCardCount(char *fname);
extern int imageViewInternal;
extern int do_comment;
int anchor_visited_predicate (Widget, char *);
extern Pixmap securityKerberos4, securityBasic, securityMd5, securityNone,
	securityUnknown, securityKerberos5, securityDomain, securityLogin;
extern mo_status mo_post_access_document (mo_window *win, char *url,
                                          char *content_type,
                                          char *post_data);
void mo_assemble_controls(mo_window *win, int detach);

/* gui-extras.c */
extern mo_status mo_post_links_window(mo_window *);
extern mo_status mo_update_links_window(mo_window *);

/* gui-dialogs.c */
extern mo_status mo_post_save_window (mo_window *);
/* called from libwww */
extern mo_status mo_post_open_local_window (mo_window *);
extern mo_status mo_post_open_window (mo_window *);
extern mo_status mo_post_mail_window (mo_window *);
extern mo_status mo_post_print_window (mo_window *);
extern mo_status mo_post_source_window (mo_window *);
extern mo_status mo_post_search_window (mo_window *);
extern char *my_strerror(int);
extern mo_status mo_post_news_win (mo_window *);
extern mo_status mo_post_subscribe_win (mo_window *);
extern mo_status mo_post_follow_win (mo_window *);
extern mo_status mo_post_generic_news_win (mo_window *, int follow);

/* gui-menubar.c */
extern mo_status mo_set_fonts (mo_window *, int);
extern mo_status mo_set_underlines (mo_window *, int);

/* hotlist.c */
extern mo_status mo_write_default_hotlist (void);
extern mo_status mo_post_hotlist_win (mo_window *);
extern mo_status mo_add_node_to_current_hotlist (mo_window *);

/* main.c */
extern void mo_exit (void);

/* mo-www.c */
extern char *mo_tmpnam (char *);
extern char *mo_convert_newlines_to_spaces (char *);
extern mo_status mo_re_init_formats (void);

extern char *mo_url_prepend_protocol(char *);
extern char *mo_url_canonicalize (char *, char *);
extern char *mo_url_canonicalize_keep_anchor (char *, char *);
extern char *mo_url_canonicalize_local (char *);
extern char *mo_url_to_unique_document (char *);
extern char *mo_url_extract_anchor (char *);

extern void application_user_info_wait (char *str);
extern char *mo_escape_part (char *);
extern char *mo_unescape_part (char *);

/* pixmaps.c */
extern void AnimatePixmapInWidget(Widget, Pixmap);
extern void MakeAnimationPixmaps(Widget);

/* techsupport.c */
extern mo_status mo_send_mail_message (char *, char *, char *, char *, char *);
extern FILE *mo_start_sending_mail_message (char *, char *, char *, char *);
extern mo_status mo_finish_sending_mail_message (void);

/* HTNews.c -- this should be elsewhere */
extern void news_prev(char *url);
extern void news_next(char *url);
extern void news_prevt(char *url);
extern void news_nextt(char *url);
extern void news_index(char *url);
extern void gui_news_index(mo_window *win);
extern void gui_news_list(mo_window *win);
extern void gui_news_prev(mo_window *win);
extern void gui_news_next(mo_window *win);
extern void gui_news_prevt(mo_window *win);
extern void gui_news_nextt(mo_window *win);

/* gui-ftp.c */
extern mo_status mo_post_ftpput_window(mo_window *); 
extern mo_status mo_post_ftpremove_window(mo_window *); 
extern mo_status mo_post_ftpmkdir_window(mo_window *); 
extern mo_status mo_post_ftpbar_window(mo_window *); 

/* callback menubar_cb */
void mo_back(Widget w, XtPointer clid, XtPointer calld);
void mo_forward(Widget w, XtPointer clid, XtPointer calld);
void mo_reload_document(Widget w, XtPointer clid, XtPointer calld);
void mo_home_document(Widget w, XtPointer clid, XtPointer calld);
void mo_history_list(Widget w, XtPointer clid, XtPointer calld);
void mo_links_window(Widget w, XtPointer clid, XtPointer calld);
void mo_open_document(Widget w, XtPointer clid, XtPointer calld);
void mo_open_local_document(Widget w, XtPointer clid, XtPointer calld);
void mo_reload_document_and_images(Widget w, XtPointer clid, XtPointer calld);
void mo_refresh_document(Widget w, XtPointer clid, XtPointer calld);
void mo_save_document(Widget w, XtPointer clid, XtPointer calld);
void mo_new_window(Widget w, XtPointer clid, XtPointer calld);
void mo_clone_window(Widget w, XtPointer clid, XtPointer calld);
void mo_close_window(Widget w, XtPointer clid, XtPointer calld);
void mo_register_node_in_default_hotlist(Widget w,XtPointer clid, XtPointer calld);
void mo_network_starting_points(Widget w, XtPointer clid, XtPointer calld);
void mo_internet_metaindex(Widget w, XtPointer clid, XtPointer calld);
void mo_help_about(Widget w, XtPointer clid, XtPointer calld);
void mo_mosaic_manual(Widget w, XtPointer clid, XtPointer calld);
void mo_whats_new(Widget w, XtPointer clid, XtPointer calld);
void mo_mosaic_demopage(Widget w, XtPointer clid, XtPointer calld);
void mo_help_onversion(Widget w, XtPointer clid, XtPointer calld);
void mo_help_onwindow(Widget w, XtPointer clid, XtPointer calld);
void mo_help_faq(Widget w, XtPointer clid, XtPointer calld);
void mo_help_html(Widget w, XtPointer clid, XtPointer calld);
void mo_help_url(Widget w, XtPointer clid, XtPointer calld);
void mo_techsupport(Widget w, XtPointer clid, XtPointer calld);
void mo_news_fmt0(Widget w, XtPointer clid, XtPointer calld);
void mo_news_fmt1(Widget w, XtPointer clid, XtPointer calld);
void mo_search(Widget w, XtPointer clid, XtPointer calld);
void mo_document_source(Widget w, XtPointer clid, XtPointer calld);
void mo_document_edit(Widget w, XtPointer clid, XtPointer calld);
void mo_document_date(Widget w, XtPointer clid, XtPointer calld);
void mo_print_document(Widget w, XtPointer clid, XtPointer calld);
void mo_mail_document(Widget w, XtPointer clid, XtPointer calld);
void mo_cci(Widget w, XtPointer clid, XtPointer calld);
#ifdef KRB5
void mo_kerberosv5_login(Widget w, XtPointer clid, XtPointer calld);
#endif
void mo_proxy(Widget w, XtPointer clid, XtPointer calld);
void mo_no_proxy(Widget w, XtPointer clid, XtPointer calld);
void mo_exit_program(Widget w, XtPointer clid, XtPointer calld);
void mo_regular_fonts_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_small_fonts_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_large_fonts_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_regular_helvetica_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_small_helvetica_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_large_helvetica_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_regular_newcentury_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_small_newcentury_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_large_newcentury_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_regular_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_small_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_large_lucidabright_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_default_underlines_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_l1_underlines_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_l2_underlines_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_l3_underlines_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_no_underlines_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_binary_transfer(Widget w, XtPointer clid, XtPointer calld);
void mo_table_support(Widget w, XtPointer clid, XtPointer calld);
void mo_body_color(Widget w, XtPointer clid, XtPointer calld);
void mo_body_images(Widget w, XtPointer clid, XtPointer calld);
void mo_image_view_internal(Widget w, XtPointer clid, XtPointer calld);
void mo_delay_image_loads(Widget w, XtPointer clid, XtPointer calld);
void mo_expand_images_current(Widget w, XtPointer clid, XtPointer calld);
void mo_re_init(Widget w, XtPointer clid, XtPointer calld);
void mo_clear_global_history(Widget w, XtPointer clid, XtPointer calld);
void mo_news_groups(Widget w, XtPointer clid, XtPointer calld);
void mo_news_list(Widget w, XtPointer clid, XtPointer calld);
void mo_news_index(Widget w, XtPointer clid, XtPointer calld);
void mo_news_prevt(Widget w, XtPointer clid, XtPointer calld);
void mo_news_prev(Widget w, XtPointer clid, XtPointer calld);
void mo_news_next(Widget w, XtPointer clid, XtPointer calld);
void mo_news_nextt(Widget w, XtPointer clid, XtPointer calld);
void mo_news_post(Widget w, XtPointer clid, XtPointer calld);
void mo_news_follow(Widget w, XtPointer clid, XtPointer calld);
void mo_news_sub(Widget w, XtPointer clid, XtPointer calld);
void mo_news_unsub(Widget w, XtPointer clid, XtPointer calld);
void mo_news_grp0(Widget w, XtPointer clid, XtPointer calld);
void mo_news_grp1(Widget w, XtPointer clid, XtPointer calld);
void mo_news_grp2(Widget w, XtPointer clid, XtPointer calld);
void mo_news_art0(Widget w, XtPointer clid, XtPointer calld);
void mo_news_art1(Widget w, XtPointer clid, XtPointer calld);
void mo_news_mread(Widget w, XtPointer clid, XtPointer calld);
void mo_news_munread(Widget w, XtPointer clid, XtPointer calld);
void mo_news_maunread(Widget w, XtPointer clid, XtPointer calld);
void mo_news_flush(Widget w, XtPointer clid, XtPointer calld);
void mo_news_flushgroup(Widget w, XtPointer clid, XtPointer calld);
void mo_ftp_put(Widget w, XtPointer clid, XtPointer calld);
void mo_ftp_mkdir(Widget w, XtPointer clid, XtPointer calld);
#ifdef MULTICAST
void mo_multicast_send_tog(Widget w, XtPointer clid, XtPointer calld);
void mo_multicast_show_participant(Widget w, XtPointer clid, XtPointer calld);
#endif

void mo_plaintext_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_formatted_text_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_postscript_cb(Widget w, XtPointer clid, XtPointer calld);
void mo_html_cb(Widget w, XtPointer clid, XtPointer calld);

extern void InitChildProcessor();
extern int MoCCIFormToClient( char *, char *, char *, char *, int);
extern mo_status mo_edit_source(mo_window *win);
extern void CommentCard(mo_window *win);
extern void mo_init_menubar();
extern mo_window *mo_make_window ( mo_window *parent, McMoWType mc_t);

extern void GetUrlData(Widget w, XtPointer clid, XtPointer calld);

extern void System(char *cmd, char *title);
extern mo_window *mo_main_next_window (mo_window *win);
extern mo_status mo_source_date(mo_window *win);
extern mo_status MoDisplayCCIWindow( mo_window *win);
extern void ShowNoProxyDialog(mo_window *win);
extern void ShowProxyDialog(mo_window *win);
extern void gui_news_subgroup(mo_window *win);
extern void gui_news_unsubgroup(mo_window *win);
extern void gui_news_flush(mo_window *win);
extern void gui_news_flushgroup(mo_window *win);
extern void gui_news_showAllGroups (mo_window *win);
extern void gui_news_showGroups (mo_window *win);
extern void gui_news_showReadGroups (mo_window *win);
extern void gui_news_showAllArticles (mo_window *win);
extern void gui_news_showArticles (mo_window *win);
extern void gui_news_markGroupRead (mo_window *win);
extern void gui_news_markGroupUnread (mo_window *win);
extern void gui_news_markArticleUnread (mo_window *win);
extern mo_status mo_handle_ftpput(mo_window *win);
extern mo_status mo_handle_ftpmkdir(mo_window *win);
extern void mo_switch_mode(mo_window *win);

mo_status mo_save_window(mo_window *win, char *fname,
                                        mo_format_token save_format);
mo_status mo_print_window(mo_window *win,
                        mo_format_token print_format, char *lpr);

mo_status mo_search_window(mo_window *win,char *str,int backward, int caselessi,
	int news);

extern void loadAgents();

#endif /* not __MOSAIC_H__ */
