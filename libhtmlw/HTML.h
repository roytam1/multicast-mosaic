/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#ifndef HTML_H
#define HTML_H

#include <Xm/Xm.h>
#if (XmVERSION == 1)&&(XmREVISION >= 2)
#undef MOTIF1_2
#define MOTIF1_2
#endif

#include <X11/StringDefs.h>

#ifndef HTML_PARSE_H
#include "HTMLparse.h"
#endif

/* defines and structures used for the HTML parser, and the parsed object list. */

typedef enum {
        MC_MO_TYPE_UNICAST,
        MC_MO_TYPE_MAIN,         /* only the Main can send */
        MC_MO_TYPE_RCV_URL_ONLY,
        MC_MO_TYPE_RCV_ALL
} McMoWType;

typedef enum {
	DIV_ALIGN_LEFT,
	DIV_ALIGN_CENTER,
	DIV_ALIGN_RIGHT
} DivAlignType;

typedef int (*visitTestProc)(Widget, char*);
typedef void (*pointerTrackProc)();

typedef struct ele_ref_rec {
	int id, pos;
} ElementRef;

typedef struct link_rec {
	char *href;
	char *role;
} LinkInfo;

extern int htmlwTrace;

/*
 * Public functions
 */
extern char *HTMLGetText (Widget w, int pretty, char *url, char *time_str);
extern char *HTMLGetTextAndSelection (Widget w, char **startp, char **endp,
					char **insertp);
extern char **HTMLGetHRefs (Widget w, int *num_hrefs);
extern char **HTMLGetImageSrcs (Widget w, int *num_srcs);
extern void HTMLFreeWidgetInfo (void *ptr);
extern LinkInfo *HTMLGetLinks (Widget w, int *num_links);
extern int HTMLPositionToId(Widget w, int x, int y);
extern int HTMLIdToPosition(Widget w, int element_id, int *x, int *y);
extern int HTMLAnchorToPosition(Widget w, char *name, int *x, int *y);
extern int HTMLAnchorToId(Widget w, char *name);
extern void HTMLGotoId(Widget w, int element_idi,int correction);
extern int HTMLLastId(Widget w);
extern void HTMLRetestAnchors(Widget w, visitTestProc testFunc);
extern void HTMLClearSelection (Widget w);
extern void HTMLSetSelection (Widget w, ElementRef *start, ElementRef *end);
extern void HTMLSetText (Widget w, char *text, int element_id,
			char *target_anchor);
extern int HTMLSearchText (Widget w, char *pattern,
	ElementRef *m_start, ElementRef *m_end, int backward, int caseless);
extern int HTMLSearchNews(Widget w,ElementRef *m_start, ElementRef *m_end);
extern void HTMLSetAppInsensitive(Widget hw);
extern void HTMLSetAppSensitive(Widget hw);
extern void HTMLDrawBackgroundImage(Widget w, int x, int y, int width, 
				    int height);
extern void McUpdateWidgetObject(Widget w, int num_eo, char * data, int len_data);

/*
 * Public Structures
 */
typedef struct acall_rec {
	XEvent *event;
	int element_id;
	char *text;
	char *href;
} WbAnchorCallbackData;

typedef struct fcall_rec {
	XEvent *event;
	char *href;
        char *method;
        char *enctype;
	int attribute_count;
	char **attribute_names;
	char **attribute_values;
} WbFormCallbackData;

typedef struct form_rec {
	Widget hw;
	char *action;
        char *method;
        char *enctype;
	int start, end;
        Widget button_pressed; /* match button pressed to one of submits */
	struct form_rec *next;
} FormInfo;

/* Client-Side Ismap -- SWP */
typedef struct coord_rec {
      int coord;
      struct coord_rec *next;
} CoordInfo;


typedef struct area_rec {
      int shape;
      CoordInfo *coordList;
      CoordInfo *currentCoord;
      char *href;
      char *alt;
      struct area_rec *next;
} AreaInfo;


typedef struct map_rec {
      char *name;
      AreaInfo *areaList;
      AreaInfo *currentArea;
      struct map_rec *next;
} MapInfo;


/*##########*/

        
/*      
 * defines for client-side ismap -- SWP
 */     
#define AREA_RECT 0 
#define AREA_CIRCLE 1  
#define AREA_POLYGON 2 
/*##########*/


/* define alignment values */
typedef enum {
	ALIGN_NONE,
	VALIGN_BOTTOM,
	VALIGN_MIDDLE,
	VALIGN_TOP,
	HALIGN_LEFT,
	HALIGN_CENTER,
	HALIGN_RIGHT
} AlignType;

typedef struct image_rec {
        char *src;
	char *alt_text;		/* alternative text */
	AlignType align;
	int height;
	int req_height;		/* required height specified in HEIGHT=nnn */
	int width;
	int req_width;		/* required width specified in WIDTH=nnn */
	int border;
	int hspace;
	int vspace;
	char *usemap; 
        MapInfo *map;
	int ismap;
	FormInfo *fptr;
	int internal;
	int delayed;
	int fetched;
	int cached;
	int num_colors;
	XColor colrs[256];
        int bg_index;
	unsigned char *image_data;
	int len_image_data;
	unsigned char *clip_data;
	int transparent;
	Pixmap image;
	Pixmap clip;
/*	McMoWType wtype;	*/
	int look_only_cache;
	int internal_numeo;
	int cw_only;
} ImageInfo;

ImageInfo * McGetPicData(Widget w, char * buf, int len_buf);

typedef struct wid_rec {
	Widget w;
	int type;
	int id;
	int x, y;
	int width, height;
        int seeable;
	char *name;
	char *value;
	char *password;
	char **mapping;
	Boolean checked;
	Boolean mapped;
	struct wid_rec *next;
	struct wid_rec *prev;
} WidgetInfo;


typedef struct _CellStruct {
        MarkType cell_type;
        int td_count;
        int tr_count;
        int colspan;
        int rowspan;
        int is_colspan;
        int is_rowspan;
	int back_cs;
	int back_rs;
        struct mark_up * td_start;
        struct mark_up * td_end;
	struct ele_rec * start_elem;
	struct ele_rec * end_elem;
	int x;
	int y;
        int height;
        int width;
        int max_width;
        int min_width;          
	int line_bottom;
        AlignType valignement ;
        AlignType halignement ;
} CellStruct;
                                
typedef struct _ColumnList {
        CellStruct * cells;
        int cell_count;
        int max_row_span;
} ColumnList;
                        
typedef struct _RowList {
        CellStruct ** cells_lines;
        int row_count ; 
        int max_cell_count_in_line ;
        int low_cur_line_num ;
} RowList;

typedef struct _TableRec {
	int	borders;
	unsigned int relative_width; /*### for <table width=50%> */
				     /* it's relative to window width */
	int	num_col;
	int	num_row;
	struct mark_up * caption_start_mark;
	struct mark_up * caption_end_mark;
	int	captionAlignment;
	struct	mark_up *tb_start_mark;
	struct	mark_up *tb_end_mark;
	struct	mark_up *start_other_mark;
	struct	mark_up *end_other_mark;
	RowList * row_list;
	int	width,height;
	int	min_width,max_width;
	int * 	col_max_w;	/* merge de toutes les width */
	int * 	col_min_w;	/* pour les colonnes */
	int *	col_w;		/* taille definitive des colonnes */
	int	is_tint;	/* is table in table? */
	int 	estimate_height; /* comptuted estimated height */
} TableInfo;

typedef struct sel_rec {
	Widget hw;
	struct mark_up *mptr;
	int is_value;
	char *retval_buf;
	char *option_buf;
	char **returns;
	char **options;
	int option_cnt;
	char **value;
	int value_cnt;
} SelectInfo;


typedef ImageInfo *(*resolveImageProc)();


/*
 * defines and structures used for the formatted element list
 */

typedef enum {
	E_TEXT = 1,
	E_BULLET,
	E_CR,
	E_LINEFEED,
	E_IMAGE,
	E_ANCHOR,
	E_WIDGET,
	E_HRULE	,
	E_TABLE	,
	E_CELL_TABLE,
	E_APROG,
	E_APPLET,
	E_MAP
} ElementType;

typedef enum {
	CODE_TYPE_UNKNOW,
	CODE_TYPE_BIN,
	CODE_TYPE_SRC,
	CODE_TYPE_APPLET
} CodeType;

typedef struct _AppletRec {
	CodeType ctype;
	char * src;
	int width;
	int height;
	int x;
	int y;
	int border_width;
	AlignType valignment;
	int param_count;
	char **param_name_t;
	char **param_value_t;
	int url_arg_count;
	char **url_arg;
	McMoWType wtype;
	int *internal_numeos;
	char ** ret_filenames;
	Boolean cw_only;
	Widget w;
	Widget frame;
} AppletInfo;

typedef struct _AprogRec {
	CodeType ctype;
	char * src;
	char * name;
	int width;
	int height;
	int x;
	int y;
	int border_width;
	AlignType valignment;
	int param_count;
	char **param_name_t;
	char **param_value_t;
	int url_arg_count;
	char **url_arg;
	McMoWType wtype;
	int *internal_numeos;
	char ** ret_filenames;
	Boolean cw_only;
	Widget w;
	Widget frame;
} AprogInfo;

typedef struct _EODataStruct {
	char * src;
	char * ret_filename;
	int num_eo;
	McMoWType wtype;
	Boolean cw_only;
} EODataStruct;

struct ele_rec {
	ElementType type;
	ImageInfo 	* pic_data;
	WidgetInfo 	*widget_data;
	TableInfo 	*table_data;
	AprogInfo	*aps;
	AppletInfo 	*ats;
	XFontStruct 	*font;
	AlignType 	valignment;
	AlignType 	halignment;
/*	Boolean internal; */
	Boolean 	selected;
	Boolean 	is_in_form;
	int 		indent_level;
	int start_pos, end_pos;
	int x, y;		/* is the upper left corner of Bounding box */
	int baseline;		/* add baseline for XDrawString(text) */
	int bwidth;
	int width;
	int height;
	int ele_id;
	int aprog_id;
	int applet_id;
	int underline_number;
	Boolean dashed_underline;
	Boolean strikeout;
	unsigned long fg;
	unsigned long bg;
	struct mark_up *anchor_tag_ptr; /* put it in struct mark_up ######*/
	char *edata;
	int edata_len;
	struct ele_rec *next;
	struct ele_rec *prev;
	struct ele_rec *line_next;
	McMoWType wtype;
	int internal_numeo;
};

/* anchor tags */
#define	AT_NAME		"name"
#define	AT_HREF		"href"
#define	AT_TITLE	"title"


/*
 * New resource names
 */

#ifdef MULTICAST
#define WbNmctype		"mctype"
#define WbCMctype		"Mctype"
#endif

#define	WbNmarginWidth		"marginWidth"
#define	WbNmarginHeight		"marginHeight"
#define	WbNtext			"text"
#define	WbNfooterAnnoText	"footerAnnoText"
#define	WbNtitleText		"titleText"
#define	WbNanchorUnderlines	"anchorUnderlines"
#define	WbNvisitedAnchorUnderlines	"visitedAnchorUnderlines"
#define	WbNdashedAnchorUnderlines	"dashedAnchorUnderlines"
#define	WbNdashedVisitedAnchorUnderlines	"dashedVisitedAnchorUnderlines"
#define	WbNanchorColor		"anchorColor"
#define	WbNvisitedAnchorColor	"visitedAnchorColor"
#define	WbNactiveAnchorFG	"activeAnchorFG"
#define	WbNactiveAnchorBG	"activeAnchorBG"
#define	WbNimageBorders		"imageBorders"
#define	WbNdelayImageLoads	"delayImageLoads"
#define	WbNisIndex		"isIndex"
#define	WbNitalicFont		"italicFont"
#define	WbNboldFont		"boldFont"
#define	WbNfixedFont		"fixedFont"
#define	WbNmeterFont		"meterFont"
#define	WbNtoolbarFont		"toolbarFont"
#define	WbNfixedboldFont	"fixedboldFont"
#define	WbNfixeditalicFont	"fixeditalicFont"
#define	WbNheader1Font		"header1Font"
#define	WbNheader2Font		"header2Font"
#define	WbNheader3Font		"header3Font"
#define	WbNheader4Font		"header4Font"
#define	WbNheader5Font		"header5Font"
#define	WbNheader6Font		"header6Font"
#define	WbNaddressFont		"addressFont"
#define	WbNplainFont		"plainFont"
#define	WbNplainboldFont	"plainboldFont"
#define	WbNplainitalicFont	"plainitalicFont"
#define	WbNlistingFont		"listingFont"
#define	WbNanchorCallback	"anchorCallback"
#define	WbNlinkCallback		"linkCallback"
#define	WbNsubmitFormCallback	"submitFormCallback"
#define	WbNpreviouslyVisitedTestFunction "previouslyVisitedTestFunction"
#define WbNmaxColorsInImage	"maxColorsInImage"
#define WbNimageCallback	"imageCallback"
#define WbNgetUrlDataCB		"getUrlDataCB"

#define	WbNpercentVerticalSpace "percentVerticalSpace"
#define WbNpointerMotionCallback "pointerMotionCallback"
#define WbNview			 "view"
#define WbNsupSubFont            "supSubFont"    /* amb */
#define WbNbodyColors            "bodyColors"
#define WbNbodyImages            "bodyImages"
/*
 * New resource classes
 */
#define	WbCMarginWidth		"MarginWidth"
#define	WbCMarginHeight		"MarginHeight"
#define	WbCText			"Text"
#define	WbCFooterAnnoText	"FooterAnnoText"
#define	WbCTitleText		"TitleText"
#define	WbCAnchorUnderlines	"AnchorUnderlines"
#define	WbCVisitedAnchorUnderlines	"VisitedAnchorUnderlines"
#define	WbCDashedAnchorUnderlines	"DashedAnchorUnderlines"
#define	WbCDashedVisitedAnchorUnderlines	"DashedVisitedAnchorUnderlines"
#define	WbCAnchorColor		"AnchorColor"
#define	WbCVisitedAnchorColor	"VisitedAnchorColor"
#define	WbCActiveAnchorFG	"ActiveAnchorFG"
#define	WbCActiveAnchorBG	"ActiveAnchorBG"
#define	WbCImageBorders		"ImageBorders"
#define	WbCDelayImageLoads	"DelayImageLoads"
#define	WbCIsIndex		"IsIndex"
#define	WbCItalicFont		"ItalicFont"
#define	WbCBoldFont		"BoldFont"
#define	WbCFixedFont		"FixedFont"
#define	WbCMeterFont		"MeterFont"
#define	WbCToolbarFont		"ToolbarFont"
#define	WbCFixedboldFont	"FixedboldFont"
#define	WbCFixeditalicFont	"FixeditalicFont"
#define	WbCHeader1Font		"Header1Font"
#define	WbCHeader2Font		"Header2Font"
#define	WbCHeader3Font		"Header3Font"
#define	WbCHeader4Font		"Header4Font"
#define	WbCHeader5Font		"Header5Font"
#define	WbCHeader6Font		"Header6Font"
#define	WbCAddressFont		"AddressFont"
#define	WbCPlainFont		"PlainFont"
#define	WbCPlainboldFont	"PlainboldFont"
#define	WbCPlainitalicFont	"PlainitalicFont"
#define	WbCListingFont		"ListingFont"
#define	WbCPreviouslyVisitedTestFunction "PreviouslyVisitedTestFunction"
#define WbCMaxColorsInImage	"MaxColorsInImage"
#define WbCImageCallback	"ImageCallback"
#define WbCGetUrlDataCB		"GetUrlDataCB"

#define	WbCPercentVerticalSpace "PercentVerticalSpace"
#define WbCPointerMotionCallback "PointerMotionCallback"
#define WbCVerticalScrollOnRight "VerticalScrollOnRight"
#define WbCHorizontalScrollOnTop "HorizontalScrollOnTop"
#define WbCView			 "View"
#define WbCSupSubFont            "SupSubFont"  /* amb */
#define WbCBodyColors            "BodyColors"
#define WbCBodyImages            "BodyImages"


typedef struct _HTMLClassRec *HTMLWidgetClass;
typedef struct _HTMLRec      *HTMLWidget;

extern WidgetClass htmlWidgetClass;

#endif /* HTML_H */

