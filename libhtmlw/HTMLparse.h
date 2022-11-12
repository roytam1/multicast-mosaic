/* HTMLparse.h
 * Author: Gilles Dauphin
 * Version 3.2.1 [May97]
 *
 * Copyright (C) 1997 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#ifndef HTML_PARSE_H
#define HTML_PARSE_H

typedef enum _MarkType {
	M_END_STATE = -3,
	M_INIT_STATE = -2,
	M_UNKNOWN = -1,		/* the first two must have this value */
	M_NONE = 0,		/* for compatibility		*/
	M_ABBR,
	M_ACRONYM,
	M_ANCHOR,
	M_ADDRESS,
	M_APPLET,
	M_AREA,
	M_BASE,
	M_BIG,
	M_BLOCKQUOTE,
	M_BODY,
	M_BOLD,
	M_BR,
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
	M_DOCTYPE,
	M_EMPHASIZED,
	M_FIXED,
	M_FONT,
	M_FORM,
	M_FRAME,
	M_FRAMESET,
	M_HEAD,
	M_HEADER_1,
	M_HEADER_2,
	M_HEADER_3,
	M_HEADER_4,
	M_HEADER_5,
	M_HEADER_6,
	M_HRULE,
	M_HTML,
	M_IMAGE,
	M_INPUT,
	M_ISINDEX,
	M_ITALIC,
	M_KEYBOARD,
	M_LINK,
	M_LIST_ITEM,
	M_MAP,
	M_MENU,
	M_META,
	M_NOFRAMES,
	M_NOSCRIPT,
	M_NUM_LIST,
	M_OBJECT,
	M_OPTION,
	M_PARAGRAPH,
	M_PARAM,
	M_PREFORMAT,
	M_BUGGY_TABLE,
	M_TABLE,
	M_TBODY,
	M_TD,
	M_TD_CELL_PAD,
	M_TD_CELL_FREE,
	M_TFOOT,
	M_TH,
	M_THEAD,
	M_TR,
	M_TEXTAREA,
	M_TITLE,
	M_SAMPLE,
	M_SCRIPT,
	M_SELECT,
	M_SMALL,
	M_STRIKEOUT,
	M_STRONG,
	M_STYLE,
	M_SUB,
	M_SUP,
	M_UNDERLINED,
	M_UNUM_LIST,
	M_VARIABLE
} MarkType;

				/* syntax of Mark types */

#define MT_ABBR		"abbr"
#define MT_ACRONYM	"acronym"
#define	MT_ANCHOR	"a"
#define	MT_ADDRESS	"address"
#define MT_APPLET	"applet"
#define MT_AREA		"area"
#define MT_BOLD		"b"
#define MT_BASE		"base"
#define MT_BIG		"big"
#define MT_BLOCKQUOTE	"blockquote"
#define MT_BODY		"body"
#define MT_BR		"br"
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
#define MT_FONT		"font"
#define MT_FORM		"form"
#define MT_FRAME	"frame"
#define MT_FRAMESET	"frameset"
#define	MT_HEADER_1	"h1"
#define	MT_HEADER_2	"h2"
#define	MT_HEADER_3	"h3"
#define	MT_HEADER_4	"h4"
#define	MT_HEADER_5	"h5"
#define	MT_HEADER_6	"h6"
#define MT_HEAD		"head"
#define MT_HRULE	"hr"
#define MT_HTML		"html"
#define MT_ITALIC	"i"
#define MT_IMAGE	"img"
#define MT_INPUT	"input"
#define MT_ISINDEX	"isindex"
#define MT_KEYBOARD	"kbd"
#define	MT_LIST_ITEM	"li"
#define MT_LINK		"link"
#define MT_MAP		"map"
#define MT_MENU		"menu"
#define MT_META		"meta"
#define MT_NOFRAMES	"noframes"
#define	MT_NUM_LIST	"ol"
#define MT_NOSCRIPT	"noscript"
#define MT_OBJECT	"object"
#define MT_OPTION	"option"
#define	MT_PARAGRAPH	"p"
#define	MT_PARAM	"param"
#define	MT_PREFORMAT	"pre"
#define MT_SAMPLE	"samp"
#define MT_SCRIPT	"script"
#define MT_SELECT	"select"
#define MT_SMALL	"small"
#define MT_STRIKEOUT	"strike"
#define MT_STRONG	"strong"
#define MT_STYLE	"style"
#define MT_SUB          "sub"
#define MT_SUP          "sup"
#define MT_TABLE	"table"
#define MT_TBODY	"tboby"
#define MT_TD		"td"
#define MT_TEXTAREA	"textarea"
#define MT_TFOOT	"tfoot"
#define MT_TH		"th"
#define MT_THEAD	"thead"
#define	MT_TITLE	"title"
#define MT_TR		"tr"
#define MT_FIXED	"tt"
#define MT_UNDERLINED   "u"
#define	MT_UNUM_LIST	"ul"
#define MT_VARIABLE	"var"


			/* non blank space character */
#define NBSP_CONST '\240'

typedef struct _AprogRec	*AprogPtr;
typedef struct _AppletRec	*AppletPtr;
typedef struct _TableRec	*TablePtr;
typedef struct image_rec	*ImageInfoPtr;

struct mark_up {
	MarkType type;
	int is_end;
	char *start;
	char *text;
	int   is_white_text;	/* is text only with 'white-space' chars ? */
	char *end;
	struct mark_up *next;
	int line;		/* line number of original html text */
	char * pcdata;		/* #pcdata for tag such <title> */

	TablePtr t_p1;		/* First pass table */

	ImageInfoPtr s_picd;	/* image saved */

	char * anc_name;	/* anchor tag */
	char * anc_href;
	char * anc_title;
	char * anc_target;

	struct mark_up * start_obj;	/* object saved */
	struct mark_up * end_obj;	/* the one that match start Object */
	struct mark_up * try_next_obj;	/* next obj to try if prev fail */

/*	AprogPtr s_aps;		/* aprog saved */
/*	AppletPtr s_ats;	/* applet saved */
};

/* frame type */

typedef enum {
	NOTFRAME_TYPE = 1,	/* Not a frame, this is a 'normal' html widget*/
	FRAME_TYPE,		/* this is a frame with html inside */
	FRAMESET_TYPE		/* html begin with frameset tag */
} FrameType;

/* What type of scrolling a frame should employ. */

typedef enum{
        FRAME_SCROLL_NONE = 1,
        FRAME_SCROLL_AUTO,
        FRAME_SCROLL_YES
}FrameScrollType;

/* Possible Frame layout policies */

typedef enum{
        FRAMESET_LAYOUT_ROWS = 1,  	/* rows only */
        FRAMESET_LAYOUT_COLS = 2,  	/* columns only */
        FRAMESET_LAYOUT_ROW_COLS = 4    /* left to right, top to bottom */
}FramesetLayout;                      
  

/* Possible types of frame sizes */

typedef enum{
        FRAME_SIZE_FIXED = 1,                 /* size specified in pixels    */
        FRAME_SIZE_RELATIVE,                  /* size is relative */
        FRAME_SIZE_OPTIONAL                   /* size is optional */
}FrameSize;


/* definition of a HTML frameset */

typedef struct _FrameSetInfo{
	FrameType	type;
        struct _FrameSetInfo *fs_parent_fs; /*parent frameset of this frameset*/
	union  _FrameChildInfo *fs_fc_next_child;	/* next child */
				/* list chaine. Le pointeur de depart est */
				/* fs_fc_children. il n'existe que dans un */
				/* frameset */
	union _FrameChildInfo *fs_fc_prev_child;	/* prev child */
	int	box_x;		/* position in parent window */
	int	box_y;
	int	box_width;
	int	box_height;
/* I am possibly also childs of FRAMESET_TYPE */
/* I get size from my parent FRAMESET */
	FrameSize fs_size_type;   /* frameset size specification for me.*/
	int	fs_size_s;        /* saved frameset size */


        FramesetLayout fs_layout_type; /* type of this set, either ROW or COL */

        int 	*fs_child_sizes;       /* array of child sizes */
        FrameSize *fs_child_size_types;/* array of size specifications */

        int 	fs_nchilds;            /* max no of childs */
        int 	fs_childs_done;        /* no of childs processed so far */
        int 	fs_insert_pos;         /* insertion position of current child */
        union  _FrameChildInfo *fs_childs;  /* array of childs, Frameset or frame */
        struct _FrameSetInfo *fs_next_frameset;    /* ptr to next frameSet */

	union _FrameChildInfo *fs_fc_children; /*list of childs. start pointer*/

}FrameSetInfo;

typedef struct _FrameInfo {
	FrameType 	type;
	FrameSetInfo	*frame_parent_fs;	/* my container */
	union _FrameChildInfo *frame_fc_next_child;	/* next child */
	union _FrameChildInfo *frame_fc_prev_child;	/* prev child */
	int	box_x;		/* position in parent window */
	int	box_y;
	int	box_width;
	int	box_height;
	FrameSize frame_size_type;
	int	frame_size_s;

	FrameScrollType	frame_scroll_type;
	char *		frame_name;		/* internal frame name */
	char *		frame_src;		/* the url to get */
	int		frame_margin_width;
	int		frame_margin_height;
	int		frame_resize;		/* may we resize this frame? */
        int 		frame_border;             /* frame border value */
} FrameInfo;

typedef struct _FrameAnyInfo {			/* common part Frame/Frameset */
	FrameType 	type;
	FrameSetInfo	*any_parent_fs;	/* my container */
	union _FrameChildInfo *any_fc_next_child;	/* next child */
	union _FrameChildInfo *any_fc_prev_child;	/* prev child */
	int	box_x;		/* position in parent window */
	int	box_y;
	int	box_width;
	int	box_height;
	FrameSize any_size_type;
	int	any_size_s;
} FrameAnyInfo;

typedef union _FrameChildInfo {
	FrameType 	type;
	FrameAnyInfo	fai;
	FrameSetInfo 	fsi;
	FrameInfo	fi;
} FrameChildInfo;

typedef struct _TopFrameSetInfo {
	int	n_allo_frames;		/* number of allocated frames */
	struct _FrameInfo * frames;	/* array of frames (final result)*/
	int 	nframes;		/* final result counter */
	int	def_margin_width;	/* init some default */
	int	def_margin_height;
	struct _FrameSetInfo *frameset_info;	/* pointer to info */
} TopFrameSetInfo;

typedef struct _HtmlTextInfo {
	char * title;
	char * base_url;
	char * base_target;
	struct mark_up *mlist;
	int    nframes;		/* if not null, it's a frameset */
	TopFrameSetInfo *frameset_info;	/* info if it is a frameset */
} HtmlTextInfo;


extern void 		clean_white_space(char *txt);
extern HtmlTextInfo 	*HTMLParseRepair(char *str);
extern struct mark_up 	*HTMLLexem( const char *str);
extern char * 		ParseMarkTag(char *text, char *mtext, char *mtag);

extern void		FreeHtmlTextInfo(HtmlTextInfo * htinfo);


#endif /* HTML_PARSE_H */

