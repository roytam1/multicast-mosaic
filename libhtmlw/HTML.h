/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#ifndef HTML_H
#define HTML_H

#include <Xm/Xm.h>
#if (XmVERSION == 1)&&(XmREVISION >= 2)
#undef MOTIF1_2
#define MOTIF1_2
#endif

#include <X11/StringDefs.h>

/*
 * defines and structures used for the HTML parser, and the parsed object list.
 */

typedef enum _MarkType {
	M_INIT_STATE = -2,
	M_UNKNOWN = -1,		/* the first two must have this value */
	M_NONE = 0,		/* for compatibility		*/
	M_ANCHOR,
	M_ADDRESS,
	M_APPLET,
	M_APROG,
	M_AREA,
	M_BASE,
	M_BIG,
	M_BLOCKQUOTE,
	M_BOLD,
	M_CAPTION,
	M_CENTER,
	M_CITATION,
	M_COMMENT,
	M_CODE,
	M_DESC_LIST,
	M_DESC_TITLE,
	M_DESC_TEXT,
	M_DFN,
	M_DIRECTORY,
	M_DIV,
	M_DOC_BODY,
	M_DOC_HEAD,
	M_DOCTYPE,
	M_EMPHASIZED,
	M_FIGURE,
	M_FIXED,
	M_FONT,
	M_FORM,
	M_FRAME,
	M_HEADER_1,
	M_HEADER_2,
	M_HEADER_3,
	M_HEADER_4,
	M_HEADER_5,
	M_HEADER_6,
	M_HRULE,
	M_HTML,
	M_IMAGE,
	M_INDEX,
	M_INPUT,
	M_ITALIC,
	M_KEYBOARD,
	M_LINEBREAK,
	M_LINK,
	M_LIST_ITEM,
	M_LISTING_TEXT,
	M_MAP,
	M_MENU,
	M_META,
	M_NUM_LIST,
	M_OPTION,
	M_PARAGRAPH,
	M_PARAM,
	M_PLAIN_FILE,
	M_PLAIN_TEXT,
	M_PREFORMAT,
	M_TABLE,
	M_TABLE_DATA,
	M_TD_CELL_PAD,
	M_TD_CELL_FREE,
	M_TABLE_HEADER,
	M_TABLE_ROW,
	M_TEXTAREA,
	M_TITLE,
	M_SAMPLE,
	M_SELECT,
	M_SMALL,
	M_STRIKEOUT,
	M_STRONG,
	M_SUB,
	M_SUP,
	M_UNDERLINED,
	M_UNUM_LIST,
	M_VARIABLE
} MarkType;

				/* syntax of Mark types */

#define	MT_ANCHOR	"a"
#define	MT_ADDRESS	"address"
#define MT_APPLET	"applet"
#define MT_APROG	"aprog"
#define MT_AREA		"area"
#define MT_BOLD		"b"
#define MT_BASE		"base"
#define MT_BIG		"big"
#define MT_BLOCKQUOTE	"blockquote"
#define MT_DOC_BODY     "body"
#define MT_LINEBREAK	"br"
#define MT_CAPTION	"caption"
#define MT_CENTER	"center"
#define MT_CITATION	"cite"
#define MT_CODE		"code"
#define	MT_DESC_TEXT	"dd"
#define MT_DFN		"dfn"
#define MT_DIRECTORY	"dir"
#define MT_DIV		"div"
#define	MT_DESC_LIST	"dl"
#define	MT_DESC_TITLE	"dt"
#define MT_DOCTYPE	"!DOCTYPE"
#define MT_EMPHASIZED	"em"
#define MT_FIGURE	"fig"
#define MT_FONT		"font"
#define MT_FORM		"form"
#define MT_FRAME	"frame"
#define	MT_HEADER_1	"h1"
#define	MT_HEADER_2	"h2"
#define	MT_HEADER_3	"h3"
#define	MT_HEADER_4	"h4"
#define	MT_HEADER_5	"h5"
#define	MT_HEADER_6	"h6"
#define MT_DOC_HEAD     "head"
#define MT_HRULE	"hr"
#define MT_HTML		"html"
#define MT_ITALIC	"i"
#define MT_IMAGE	"img"
#define MT_INPUT	"input"
#define MT_INDEX	"isindex"
#define MT_KEYBOARD	"kbd"
#define	MT_LIST_ITEM	"li"
#define MT_LINK		"link"
#define MT_LISTING_TEXT	"listing"
#define MT_MAP		"map"
#define MT_MENU		"menu"
#define MT_META		"meta"
#define	MT_NUM_LIST	"ol"
#define MT_OPTION	"option"
#define	MT_PARAGRAPH	"p"
#define	MT_PARAM	"param"
#define	MT_PLAIN_FILE	"plaintext"
#define	MT_PREFORMAT	"pre"
#define MT_SAMPLE	"samp"
#define MT_SELECT	"select"
#define MT_SMALL	"small"
#define MT_STRIKEOUT	"strike"
#define MT_STRONG	"strong"
#define MT_SUB          "sub"
#define MT_SUP          "sup"
#define MT_TABLE	"table"
#define MT_TABLE_DATA	"td"
#define MT_TEXTAREA	"textarea"
#define MT_TABLE_HEADER	"th"
#define	MT_TITLE	"title"
#define MT_TABLE_ROW	"tr"
#define MT_FIXED	"tt"
#define MT_UNDERLINED   "u"
#define	MT_UNUM_LIST	"ul"
#define MT_VARIABLE	"var"
#define	MT_PLAIN_TEXT	"xmp"


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

struct mark_up {
	MarkType type;
	int is_end;
	char *start;
	char *text;
	char *end;
	struct mark_up *next;
	struct aprog_rec * saved_aps;
	struct table_rec * table_info1;	/* First pass table */
	char * anchor_name;
	char * anchor_href;
	char * anchor_title;
};

extern int htmlwTrace;

/*
 * Public functions
 */
extern char *HTMLGetText (Widget w, int pretty, char *url, char *time_str);
extern char *HTMLGetTextAndSelection (Widget w, char **startp, char **endp,
					char **insertp);
extern char **HTMLGetHRefs (Widget w, int *num_hrefs);
extern char **HTMLGetImageSrcs (Widget w, int *num_srcs);
extern void *HTMLGetWidgetInfo (Widget w);
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
extern void HTMLSetText (Widget w, char *text, char *header_text,
			char *footer_text, int element_id,
			char *target_anchor, void *ptr);
extern int HTMLSearchText (Widget w, char *pattern,
	ElementRef *m_start, ElementRef *m_end, int backward, int caseless);
extern int HTMLSearchNews(Widget w,ElementRef *m_start, ElementRef *m_end);
extern void HTMLSetFocusPolicy(Widget w, int to);
extern void HTMLSetAppInsensitive(Widget hw);
extern void HTMLSetAppSensitive(Widget hw);
extern void HTMLTraverseTabGroups(Widget w, int how);
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
        char *enc_entity;
	char *format;
	int attribute_count;
	char **attribute_names;
	char **attribute_values;
} WbFormCallbackData;

typedef struct form_rec {
	Widget hw;
	char *action;
        char *method;
        char *enctype;
        char *enc_entity;
	char *format;
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

typedef struct image_rec {
	char *usemap; 
        MapInfo *map;
	int ismap;
	FormInfo *fptr;
	int internal;
	int delayed;
	int fetched;
	int cached;
	int width;
	int height;
	int num_colors;
	int *reds;
	int *greens;
	int *blues;
        int bg_index;
	unsigned char *image_data;
	unsigned char *clip_data;
	int transparent;
	Pixmap image;
	Pixmap clip;
/*	char *text; */
        char *src;
	McMoWType wtype;
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


/* define alignment values */
typedef enum {
	ALIGN_BOTTOM,
	ALIGN_MIDDLE,
	ALIGN_TOP
} ValignType;

typedef enum {
	ALIGN_LEFT,
	ALIGN_CENTER,
	ALIGN_RIGHT
} HalignType;



typedef struct _CellStruct {
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
        MarkType cell_type;
	int x;
	int y;
        int height;
        int width;
        int max_width;
        int min_width;          
	int line_bottom;
        ValignType valignement ;
        HalignType halignement ;
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

typedef struct table_rec {
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
	E_MAP
} ElementType;

typedef enum {
	CODE_TYPE_UNKNOW,
	CODE_TYPE_BIN,
	CODE_TYPE_SRC
} CodeType;

typedef struct aprog_rec {
	CodeType ctype;
	char * src;
	char * name;
	int width;
	int height;
	int x;
	int y;
	int border_width;
	ValignType valignment;
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
	ImageInfo * pic_data;
	WidgetInfo *widget_data;
	TableInfo *table_data;
	AprogInfo * aprog_struct;
	XFontStruct *font;
	ValignType valignment;
	HalignType halignment;
/*	Boolean internal; */
	Boolean selected;
	int indent_level;
	int start_pos, end_pos;
	int x, y;		/* is the upper left corner of Bounding box */
	int baseline;		/* add baseline for XDrawString(text) */
	int bwidth;
	int width;
	int height;
	int line_number;
	int ele_id;
	int aprog_id;
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

typedef struct _ParentHTMLObjectDesc {
	MarkType type;
	int inner_width;
	int inner_height;
	void *el;
}ParentHTMLObjectDesc;

struct ref_rec {
	char *anchorHRef;
	struct ref_rec *next;
};

struct delay_rec {
	char *src;
	struct delay_rec *next;
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
#define	WbNheaderText		"headerText"
#define	WbNfooterText		"footerText"
#define	WbNtitleText		"titleText"
#define	WbNanchorUnderlines	"anchorUnderlines"
#define	WbNvisitedAnchorUnderlines	"visitedAnchorUnderlines"
#define	WbNdashedAnchorUnderlines	"dashedAnchorUnderlines"
#define	WbNdashedVisitedAnchorUnderlines	"dashedVisitedAnchorUnderlines"
#define	WbNanchorColor		"anchorColor"
#define	WbNvisitedAnchorColor	"visitedAnchorColor"
#define	WbNactiveAnchorFG	"activeAnchorFG"
#define	WbNactiveAnchorBG	"activeAnchorBG"
#define	WbNfancySelections	"fancySelections"
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
/*
#define	WbNresolveImageFunction "resolveImageFunction"
#define	WbNresolveDelayedImage "resolveDelayedImage"
*/
#define WbNimageCallback	"imageCallback"
#define WbNgetUrlDataCB		"getUrlDataCB"

#define	WbNpercentVerticalSpace "percentVerticalSpace"
#define WbNpointerMotionCallback "pointerMotionCallback"
#define WbNview			 "view"
#define WbNverticalScrollBar	 "verticalScrollBar"
#define WbNhorizontalScrollBar	 "horizontalScrollBar"
#define WbNsupSubFont            "supSubFont"    /* amb */
#define WbNbodyColors            "bodyColors"
#define WbNbodyImages            "bodyImages"
/*
 * New resource classes
 */
#define	WbCMarginWidth		"MarginWidth"
#define	WbCMarginHeight		"MarginHeight"
#define	WbCText			"Text"
#define	WbCHeaderText		"HeaderText"
#define	WbCFooterText		"FooterText"
#define	WbCTitleText		"TitleText"
#define	WbCAnchorUnderlines	"AnchorUnderlines"
#define	WbCVisitedAnchorUnderlines	"VisitedAnchorUnderlines"
#define	WbCDashedAnchorUnderlines	"DashedAnchorUnderlines"
#define	WbCDashedVisitedAnchorUnderlines	"DashedVisitedAnchorUnderlines"
#define	WbCAnchorColor		"AnchorColor"
#define	WbCVisitedAnchorColor	"VisitedAnchorColor"
#define	WbCActiveAnchorFG	"ActiveAnchorFG"
#define	WbCActiveAnchorBG	"ActiveAnchorBG"
#define	WbCFancySelections	"FancySelections"
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
/*
#define	WbCResolveImageFunction "ResolveImageFunction"
#define	WbCResolveDelayedImage "ResolveDelayedImage"
*/
#define WbCImageCallback	"ImageCallback"
#define WbCGetUrlDataCB		"GetUrlDataCB"

#define	WbCPercentVerticalSpace "PercentVerticalSpace"
#define WbCPointerMotionCallback "PointerMotionCallback"
#define WbCVerticalScrollOnRight "VerticalScrollOnRight"
#define WbCHorizontalScrollOnTop "HorizontalScrollOnTop"
#define WbCView			 "View"
#define WbCVerticalScrollBar	 "VerticalScrollBar"
#define WbCHorizontalScrollBar	 "HorizontalScrollBar"
#define WbCSupSubFont            "SupSubFont"  /* amb */
#define WbCBodyColors            "BodyColors"
#define WbCBodyImages            "BodyImages"


typedef struct _HTMLClassRec *HTMLWidgetClass;
typedef struct _HTMLRec      *HTMLWidget;

extern WidgetClass htmlWidgetClass;

#endif /* HTML_H */

