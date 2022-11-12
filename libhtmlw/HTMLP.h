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

/* To allow arbitrary nesting of lists */

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

typedef struct _ColorStack {
	Pixel	pixel;
	struct _ColorStack *next;	/* to be stacked */
} ColorStack;

typedef struct _FloatAlignRec {	/* For float align object and indent */
				/* level stack (left and right) */ 
	int	offsetx;	/* how many indent (left or right) */
				/* that's the width of object */
	int	ret_topy;	/* where is the top to return */
	int	trap_boty;	/* where is the end of indent */
}FloatAlignRec;

typedef struct _FloatAlignStackRec {
	FloatAlignRec fa;
	int float_align_sum;	/* indent @ this level */
	struct _FloatAlignStackRec * next;
} FloatAlignStackRec;
	
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

	FloatAlignStackRec *fasl; /* Float left */
	int float_align_left;		/* result of all align left */
	FloatAlignStackRec *fasr; /* Float right */
	int float_align_right;		/* result of all align right */

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
	AlignType div;		/* is in <CENTER> ? */
	unsigned long	fg_text;	/* the current text foreground */
	unsigned long	bgcolor;	/* the current background */
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
	Boolean		processing_text;
        XFontStruct 	*cur_font;
	Boolean		strikeout ;
/*#############################*/
	int		is_index ;
	int		Width ;
	DescRec		DescType ;
	int		InDocHead ;
	MapRec *	cur_map;
	int nowrap; /* inherited from CellStruct: <NOWRAP> or <NOBR> */
} PhotoComposeContext;

typedef struct _HTMLSubRessources {	
/* color of background is set by BODY, TABLE , TR, TD TH (and style...) */
/* Part of photocomposecontext */
/* For BODY, a background image superseeded a background pixel.... */
	Pixel	bgcolor;	/* set by <body bgcolor = > */

        Pixel   fgcolor;	/* foreground top and bottom shadow color */
	Pixel	top_color;	/* is computed from bg color */
	Pixel	bottom_color;    

/* color of text is set by : BODY, BASEFONT and FONT (and style...) */
/* fg_text is part of PhotoComposeContext */
	Pixel	fg_text;	/* set by <body text = > */

/* color of link alink and vlink is only set by BODY (and style...) */
/* Not part of PhotoComposeContext */
	Pixel	fg_link;	/* unvisited color link */
	Pixel	fg_vlink;	/* visited link color */
	Pixel	fg_alink;	/* active link color (when clic on a link */

/* background image is set by BODY */
	int	have_bgima;	/* <body background=uri */
	Pixmap	bgimamap;	/* set to NULL at init */
        int     bgima_height;
        int     bgima_width; 
/*	int		bg_image;		*/
/*	Pixmap		bgmap_SAVE;		*/
/*      int             bg_height;		*/
/*      int             bg_width; 		*/
} HTMLSubRessources;

	/* default ressource of widget. to be use when reset */
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

/* Ces ressources sont defini a la creation et constitue le defaut */
	HTMLSubRessources	def_res;
/* Ceci est 'reseter' par copie de def_res lors d'un chargement de page html */
	HTMLSubRessources	cur_res;

        Boolean                 body_colors; 	/* did we enable body colors ?? */
	Boolean                 body_images;    /* enable body image ? */

	int			max_colors_in_image;
    
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
	Dimension	max_pre_width;
	Dimension	view_width;
	Dimension	view_height;
	int		doc_width;
	int		doc_height;
	int		scroll_x;
	int		scroll_y;
	Boolean		use_hbar;
	Boolean		use_vbar;
	struct ele_rec	*formatted_elements;
	struct ele_rec	*select_start;
	struct ele_rec	*select_end;
	int		sel_start_pos;
	int		sel_end_pos;
	struct ele_rec	*new_start;
	struct ele_rec	*new_end;
	int		new_start_pos;
	int		new_end_pos;
	struct ele_rec	*active_anchor;
	GC		drawGC;
	GC		bgimgGC;
	int		press_x;
	int		press_y;
	Time		but_press_time;
	Time		selection_time;
	struct mark_up	*html_objects;
	WidgetInfo	*widget_list;
	FormInfo	*form_list;
	MapRec		*map_list;
        Boolean         obscured;
	struct ele_rec	*last_formatted_elem;
	struct ele_rec	*cur_elem_to_format;

/* frame ressource */
	int		frame_type;	/* FRAMESET_TYPE, FRAME_TYPE, NOTFRAME_TYPE */
					/* FRAMESET_TYPE is a container for FRAME */
	HTMLWidget	*frames;	/* a frame is a HTMLWidget */
	int		nframe;		/* number of frame in FRAMESET_TYPE */
	TopFrameSetInfo *topframeset_info; /* when i am the top frameset */

/*      String   	frame_src;    */  /* source document */
/*      String   	frame_name;   */  /* internal frame name */

	FontStack	*font_stack;	/* Widget have font stack */
	XFontStruct	*default_font;	/* start with this font */
	XFontStruct 	*cur_font;	/* current font drawing */

	ColorStack	*color_stack_bg;
	ColorStack	*color_stack_fg;
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
extern void RefreshElement(HTMLWidget hw, struct ele_rec *eptr,
		int win_x, int win_y, Dimension win_w, Dimension win_h);
extern void LineBreak(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void HtmlGetImage(HTMLWidget hw, ImageInfo *picd,
	PhotoComposeContext *pcc, int force_load);


extern void MMPopFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void MMPushFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc);
extern void MMInitWidgetFont(HTMLWidget hw);
extern Pixel MMPopColorBg(HTMLWidget hw);
extern Pixel MMPopColorFg(HTMLWidget hw);
extern Pixel MMPushColorBg(HTMLWidget hw, Pixel new_pix);
extern Pixel MMPushColorFg(HTMLWidget hw, Pixel new_pix);
extern void MMInitWidgetColorStack(HTMLWidget hw);
extern void MMResetWidgetColorStack(HTMLWidget hw);

extern void HTMLDrawBackgroundImage(HTMLWidget w, int x, int y, int width, 
				    int height);
extern void _XmHTMLFrameAdjustConstraints( HTMLWidget w, TopFrameSetInfo *tfsi);
extern HTMLWidget _XmHTMLFrameCreate(HTMLWidget w, FrameInfo * fi);
extern void _XmHTMLReconfigureFrames(HTMLWidget w, TopFrameSetInfo *tfsi);
extern void _XmHTMLMapFrames(HTMLWidget w);
extern void _XmHTMLDestroyFrames(HTMLWidget w);


extern void ObjectPlace(HTMLWidget hw, struct mark_up **mptr,
	PhotoComposeContext * pcc);

extern void ObjectRefresh(HTMLWidget hw, struct ele_rec *eptr);

extern void SkipMap(struct mark_up **map_start);
extern int MapAreaFound(HTMLWidget hw,struct ele_rec * eptr,
	Dimension x,Dimension y, char** href);

extern void UnsetFloatAlignRight(PhotoComposeContext * pcc);
extern void UnsetFloatAlignLeft(PhotoComposeContext * pcc);
extern void SetFloatAlignRight(PhotoComposeContext *pcc, int width, int height);
extern void SetFloatAlignLeft(PhotoComposeContext *pcc, int width, int height);

#endif /* HTMLP_H */
