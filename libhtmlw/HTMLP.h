/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

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

/*****  
* Possible types of frame sizes
*****/               
typedef enum{   
        FRAME_SIZE_FIXED = 1,                 /* size specified in pixels    */
        FRAME_SIZE_RELATIVE,                  /* size is relative */
        FRAME_SIZE_OPTIONAL                   /* size is optional */
}FrameSize;             
                                
/***** 
* What type of scrolling a frame should employ.
*****/                                
typedef enum{                         
        FRAME_SCROLL_NONE = 1,        
        FRAME_SCROLL_AUTO,            
        FRAME_SCROLL_YES              
}FrameScrolling; 

/*****  
* Possible Frame layout policies
*****/  
typedef enum{
        FRAMESET_LAYOUT_ROWS = 1,  /* rows only */      
        FRAMESET_LAYOUT_COLS = 2,  /* columns only */      
        FRAMESET_LAYOUT_ROW_COLS = 4    /* left to right, top to bottom */
}FramesetLayout;

#define	NOTFRAME_TYPE 0	/* Not a frame, this is a 'normal' html widget*/
#define	FRAME_TYPE    1	/* this is a frame with html inside */
#define	FRAMESET_TYPE 2	/* html begin with frameset tag */
/* remarque:
	A frameset may have the FRAME_TYPE because son of frameset
	The upper level frameset does not set the FRAME_TYPE
*/

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
        XFontStruct	*xfont;
	char		*fndry;		/* adobe */
	char		*family;	/* times courier helvetica */
	char		*weight;	/* medium bold */
	char		*slant;		/* i r */
/* style width always normal */
	char		*pixelsize;	/* pixelsize (default = 10) */
	char		*pointsize;	/* pointsize (default = 100 decip */
	char		*xres;
	char		*yres;		/* -*- 0 72 75 100 */
	char		*charset;	/* iso8859-1 */
	char		*cache_name;
} FontRec;

typedef struct _FontStack {
	FontRec 	*font;		/* pointeur to currrent font */
	struct _FontStack *next;	/* to be stacked */
} FontStack;

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
	Boolean		is_in_paragraph; /* am I in paragraph block ? */
        XFontStruct 	*cur_font;
	Boolean		strikeout ;
/*#############################*/
	int		is_index ;
	int		Width ;
	DescRec		DescType ;
	int		InDocHead ;
	char *		TitleText ;
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

	XtCallbackList		anchor_callback;
	XtCallbackList		form_callback;

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


        XtPointer		previously_visited_test;
	char *			base_url;
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

/* frame ressource */
	int		frame_type;	/* FRAMESET_TYPE, FRAME_TYPE,
						   NOTFRAME_TYPE */
	HTMLWidget	*frames;	/* a frame is a HTMLWidget */
					/* FRAMESET_TYPE is a container for FRAME */
	int		nframe;		/* number of frame in FRAMESET_TYPE */

/* if i am a frame , i have attribute. i am also childs of FRAMESET_TYPE */
	FrameScrolling	frame_scroll_type;    /* frame scrolling */
	int             frame_border;   /* add a border to the frames? */
        int		frame_x;        /* computed frame x-position */
        int		frame_y;        /* computed frame y-position */
        Dimension	frame_width;    /* computed frame width */
        Dimension	frame_height;   /* computed frame height */
	Dimension       frame_size_s;		/* saved frame size */
	FrameSize       frame_size_type;	/* horizontal frame size specification */
        String   	frame_src;      /* source document */
        String   	frame_name;     /* internal frame name */
        Dimension	frame_margin_width;   /* frame margin width */
        Dimension	frame_margin_height;  /* frame margin height */
        Boolean  	frame_resize;   /* may we resize this frame? */
        Widget		frame_wid;      /* Widget id for this frame */

/* Frame resizing */
        int            frame_drag_x;      /* Amount dragged in x-direction */
        int            frame_drag_y;      /* Amount dragged in y-direction */
     
	HTMLWidget	frame_parent_frameset; /* parent frameset, if any */
        HTMLWidget 	frame_next;  /* next frame child, if any  */
        HTMLWidget 	frame_prev;  /* prev. frame child, if any    */
        HTMLWidget 	frame_children;    /* list of frames */
        FramesetLayout  frame_layout; /* frameset layout policy */
	XtCallbackList  frame_callback;

	FontStack	*font_stack;	/* Widget have font stack */
	XFontStruct	*default_font;	/* start with this font */
	XFontStruct 	*cur_font;	/* current font drawing */
/*#############*/
/* amb */
/*        XFontStruct             *supsub_font; */
/* end amb */
/*#############*/
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
extern int FormatAll(HTMLWidget hw, int Fwidth, Boolean save_obj);
extern void RefreshElement(HTMLWidget hw,struct ele_rec *eptr);
extern void LineBreak(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void HtmlGetImage(HTMLWidget hw, ImageInfo *picd,
	PhotoComposeContext *pcc, int force_load);

extern void _FreeAprogStruct(AprogInfo * aps);
extern void _FreeAppletStruct(AppletInfo * ats);
extern void _FreeTableStruct(TableInfo * t);

extern void MMPopFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void MMPushFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void MMInitWidgetFont(HTMLWidget hw);



#endif /* HTMLP_H */
