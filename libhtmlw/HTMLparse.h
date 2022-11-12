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
	M_BUGGY_TABLE,
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
	AprogPtr s_aps;		/* aprog saved */
	AppletPtr s_ats;	/* applet saved */
	ImageInfoPtr s_picd;	/* image saved */
	TablePtr t_p1;		/* First pass table */
	char * anc_name;
	char * anc_href;
	char * anc_title;
};


extern int		caseless_equal(char *str1, char *str2);
extern void 		clean_white_space(char *txt);
extern struct mark_up * HTMLParse(char *str);
extern char * 		ParseMarkTag(char *text, char *mtext, char *mtag);


#endif /* HTML_PARSE_H */

