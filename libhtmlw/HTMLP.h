/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#ifndef HTMLP_H
#define HTMLP_H

#include "HTML.h"

#include <Xm/XmP.h>
# ifdef MOTIF1_2
#  include <Xm/ManagerP.h>
# endif /* MOTIF1_2 */

#include <X11/Xatom.h>
#include <X11/Xmu/Atoms.h>

/*  New fields for the HTML widget class */
typedef struct _HTMLClassPart {
	int none;		/* no extra HTML class stuff */
} HTMLClassPart;

typedef struct _HTMLClassRec
{
	CoreClassPart		core_class;
	CompositeClassPart	composite_class;
	ConstraintClassPart	constraint_class;
	XmManagerClassPart	manager_class;
	HTMLClassPart		html_class;
} HTMLClassRec;


extern HTMLClassRec htmlClassRec;

#define IMAGE_DEFAULT_BORDER	2
#define DEF_IMAGE_HSPACE	2
#define DEF_IMAGE_VSPACE	2
#define D_INDENT_SPACES		40

#define D_NONE          0
#define D_TITLE         1
#define D_TEXT          2
#define D_OLIST         3
#define D_ULIST         4
#define D_DESC_LIST_START 5

/*
 * To allow arbitrary nesting of lists
 */
typedef struct dtype_rec {
        int type;               /* D_NONE, D_TITLE, D_TEXT, D_OLIST, D_ULIST */
        int count;
        int compact;
	int save_left_margin;
	int indent_margin;
	int save_cur_line_width;
	int cur_line_width;
        struct dtype_rec *next;
} DescRec;
 
/*
 * To allow arbitrary nesting of font changes
 */
typedef struct font_rec {
        XFontStruct *font;
        struct font_rec *next;
} FontRec;

/* a stack to maintain a html fifo stack */
typedef struct _PhotoComposeContext {
	int width_of_viewable_part;	/* never change */
				/* during computation */
	int right_margin;
	int left_margin;
	int eoffsetx;		/* The element offset relative to View */
	int eoffsety;
	int cur_line_width;    /* WidthOfViewablePart-right_margin-left_margin*/
	int x;			/* x,y relative to View, Where to put Next */
	int y;			/* Element */
	int ex;			/* x,y relative to object */
	int ey;

/* when cw_only we never create Element */
/* but we compute 2 values : computed_min_x, computed_max_x */
/* This part is for first pass of table. Pour pre-calculer la taille des 'cells' */
	Boolean cw_only;	/* compute width only if True */
	int computed_min_x;	/* the max of all min_x */
	int computed_max_x;	/* the max of all max_x */

	int margin_height;
	int cur_baseline;   /* all object in a line must have the same */
				/* baseline. If baseline change then adjust */
				/* y , ey & cur_line_height */
				/* and the y value in each element of line */
				/* y - cur_baseline donne la top line */
				/* de la boundingBox de la ligne */
				/* y - cur_baseline + cur_line_height*/
				/* donne la top line de la ligne suivante */
	int cur_line_height;
	int element_id;    	/* to get unique number */
	char is_bol;      	/* we are at begin of line if True */
	char have_space_after;  /* remember if a word have a space after*/
	XFontStruct *cur_font;
	struct mark_up * anchor_tag_ptr;     /* we are in anchor ?? */
	int max_width_return;	/* we compute the MaxWidth of hyper text to */
				/* adjust scrollbar */
				/* initial value is WidthOfViewablePart */
	int pf_lf_state; 	/* state for linefeed */
	int preformat;		/* is in <PRE> ? */
	DivAlignType div;	/* is in <CENTER> ? */
	int internal_mc_eo ;	/* internal multicast embedded object number */
	unsigned long	fg ;	/* the current foreground */
	unsigned long	bg ;	/* the current background */
	int		underline_number ;
	int		in_underlined ;
	Boolean		dashed_underlines ;
	FormInfo *	cur_form ; /* the CurrentForm */
	Boolean		in_form;	/* is in_form ? */
	int		widget_id ;
	int		aprog_id;
	int		applet_id;
	int		superscript ;
	int		subscript ;
	int		indent_level ;
	char *		text_area_buf ; /* buffer pour Form TextArea */
	int		ignore ;	/* ignore some tag when formating */
	SelectInfo *	current_select ; /* SELECT in FORM */
	Boolean		in_select;	/* is in_select ? */
/*#############################*/
	int		is_index ;
	int		Width ;
	Boolean		Strikeout ;
	DescRec		DescType ;
	int		InDocHead ;
	char *		TitleText ;
	FontRec		FontBase ;
	FontRec * 	FontStack;
	MapInfo *	cur_map;
} PhotoComposeContext;

/* New fields for the HTML widget */
typedef struct _HTMLPart {
	/* Resources */
	Dimension		margin_width;
	Dimension		margin_height;

	Widget			view;
	Widget			hbar;
	Widget			vbar;
	Widget                  frame;

	XtCallbackList		anchor_callback;
	XtCallbackList		link_callback;
	XtCallbackList		form_callback;

	char			*title;
	char			*raw_text;
/*
 * Without motif we have to define our own forground resource
 * instead of using the manager's
 */
	Pixel			anchor_fg;
	Pixel			visitedAnchor_fg;
	Pixel			activeAnchor_fg;
	Pixel			activeAnchor_bg;

        Boolean                 body_colors;
        Boolean                 body_images;

	int			max_colors_in_image;
	int			bg_image;

	Pixmap			bgmap_SAVE;
	Pixmap			bgclip_SAVE;
        int                     bg_height;
        int                     bg_width; 

        Pixel                   foreground_SAVE;
	Pixel			anchor_fg_SAVE;
	Pixel			visitedAnchor_fg_SAVE;
	Pixel			activeAnchor_fg_SAVE;
	Pixel			activeAnchor_bg_SAVE;
	Pixel			top_color_SAVE;
	Pixel			bottom_color_SAVE;    
        Pixel                   background_SAVE;
    
	int			num_anchor_underlines;
	int			num_visitedAnchor_underlines;
	Boolean			dashed_anchor_lines;
	Boolean			dashed_visitedAnchor_lines;
	Boolean			is_index;
	int			percent_vert_space;

	XFontStruct		*font;
	XFontStruct		*italic_font;
	XFontStruct		*bold_font;
	XFontStruct		*meter_font;
	XFontStruct		*toolbar_font;
	XFontStruct		*fixed_font;
	XFontStruct		*fixedbold_font;
	XFontStruct		*fixeditalic_font;
	XFontStruct		*header1_font;
	XFontStruct		*header2_font;
	XFontStruct		*header3_font;
	XFontStruct		*header4_font;
	XFontStruct		*header5_font;
	XFontStruct		*header6_font;
	XFontStruct		*address_font;
	XFontStruct		*plain_font;
	XFontStruct		*plainbold_font;
	XFontStruct		*plainitalic_font;
	XFontStruct		*listing_font;
/* amb */
        XFontStruct             *supsub_font;
/* end amb */

        XtPointer		previously_visited_test;
	XtCallbackList		image_callback;
	Boolean			delay_image_loads;
	XtCallbackList		get_url_data_cb;
        XtCallbackList		pointer_motion_callback;

	/* PRIVATE */
	Dimension		max_pre_width;
	Dimension		view_width;
	Dimension		view_height;
	int			doc_width;
	int			doc_height;
	int			scroll_x;
	int			scroll_y;
	Boolean			use_hbar;
	Boolean			use_vbar;
	struct ele_rec		*formatted_elements;
	struct ele_rec		*select_start;
	struct ele_rec		*select_end;
	int			sel_start_pos;
	int			sel_end_pos;
	struct ele_rec		*new_start;
	struct ele_rec		*new_end;
	int			new_start_pos;
	int			new_end_pos;
	struct ele_rec		*active_anchor;
	GC			drawGC;
	int			press_x;
	int			press_y;
	Time			but_press_time;
	Time			selection_time;
	struct mark_up		*html_objects;
	WidgetInfo		*widget_list;
	FormInfo		*form_list;
	MapInfo                 *map_list;
        Boolean                 obscured;
	struct ele_rec		*last_formatted_elem;
	struct ele_rec		*cur_elem_to_format;
#ifdef MULTICAST
	McMoWType 		mc_wtype;
#endif
} HTMLPart;


typedef struct _HTMLRec {
	CorePart		core;
	CompositePart		composite;
	ConstraintPart		constraint;
	XmManagerPart		manager;
	HTMLPart		html;
} HTMLRec;

/* to reduce the number of MOTIF/ATHENA ifdefs around the code
 * we use some generalized constants x */
#   define XxNx      XmNx
#   define XxNy      XmNy
#   define XxNwidth  XmNwidth
#   define XxNheight XmNheight
#   define XxNset    XmNset
#   define XxNvalue  XmNvalue

extern HTMLPart * McGetInternalHtmlPart( Widget w);
extern void ReformatWindow( HTMLWidget hw, Boolean save_obj);
extern int FormatAll(HTMLWidget hw, int *Fwidth, Boolean save_obj);
extern void FreeLineList( struct ele_rec *list, Widget w, Boolean save_obj);
extern void RefreshElement(HTMLWidget hw,struct ele_rec *eptr);
extern void LineBreak(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);

extern void _FreeAprogStruct(AprogInfo * aps);
extern void _FreeAppletStruct(AppletInfo * ats);
extern void _FreeTableStruct(TableInfo * t);

#endif /* HTMLP_H */
