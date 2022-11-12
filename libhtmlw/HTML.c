/* Please read copyright.tmpl. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <assert.h>

#include <Xm/DrawingA.h>
#include <Xm/ScrollBar.h>
#include <Xm/DrawP.h>
#include <X11/cursorfont.h>
#include <X11/Xmu/StdSel.h>

#include "HTMLparse.h"

#include "../libnut/mipcf.h"
#include "../libnut/system.h"

#include "HTMLP.h"
#include "HTMLPutil.h"

#define DEBUG_REFRESH 0

#define	MARGIN_DEFAULT		10
#define	CLICK_TIME		500
#define	SELECT_THRESHOLD	3
#define	MAX_UNDERLINES		3
#define X_DEFAULT_INCREMENT       18
#define Y_DEFAULT_INCREMENT       18

#define VERT_SCROLL_WIDTH 14
#define HORIZ_SCROLL_HEIGHT 14

#ifndef ABS
#define ABS(x)  (((x) > 0) ? (x) : ((x) * -1))
#endif

static void		SelectStart(Widget w, XEvent *event,
				String * params, Cardinal * num_params);
static void		ExtendStart(Widget w, XEvent *event,
				String * params, Cardinal * num_params);
static void		ExtendAdjust(Widget w, XEvent *event,
				String * params, Cardinal * num_params);
static void		ExtendEnd(Widget w, XEvent *event,
				String * params, Cardinal * num_params);
static void 		TrackMotion( Widget w, XEvent *event,
				String * params, Cardinal * num_params);
static Boolean 		ConvertSelection( Widget w, Atom *selection,
				Atom *target, Atom *type, caddr_t *value,
				unsigned long *length, int *format);
static void 		LoseSelection( Widget w, Atom *selection);
static void 		SelectionDone( Widget w, Atom *selection, Atom *target);

static void		_HTMLInput(Widget w, struct ele_rec *eptr, XEvent *event,
				String *params, Cardinal *num_params);
static void             Initialize(HTMLWidget request, HTMLWidget nw);
static void             Redisplay(HTMLWidget hw, XEvent *event, Region region);
static void             Resize(HTMLWidget hw);
static Boolean          SetValues(HTMLWidget current, HTMLWidget request,
				HTMLWidget nw);
static XtGeometryResult GeometryManager(Widget w, XtWidgetGeometry *request,
				XtWidgetGeometry *reply);
static void		ViewRedisplay(HTMLWidget hw, int x, int y,
				int width, int height);
void             	ViewClearAndRefresh(HTMLWidget hw);
static Boolean 		html_accept_focus(Widget w, Time *t);
static void 		Realize(Widget ww, XtValueMask *valueMask, 
				XSetWindowAttributes *attrs); 

char CurrentURL[8096];	/*if url bigger than this...too bad*/

/* Default translations
 * Selection of text, and activate anchors.
 * If motif, add manager translations.
 */

static char defaultTranslations[] = " \
<Btn1Down>:	select-start() ManagerGadgetArm()\n\
<Btn1Motion>:	extend-adjust() ManagerGadgetButtonMotion()\n\
<Btn1Up>:	extend-end(PRIMARY, CUT_BUFFER0) ManagerGadgetActivate()\n\
<Btn2Down>:	select-start()\n\
<Btn2Motion>:	extend-adjust()\n\
<Btn2Up>:	extend-end(PRIMARY, CUT_BUFFER0)\n\
<Motion>:       track-motion()\n\
<Leave>:        track-motion()\n\
<Expose>:       track-motion()\
";

static XtActionsRec actionsList[] = {
   { "select-start",    (XtActionProc) SelectStart },
   { "extend-start",    (XtActionProc) ExtendStart },
   { "extend-adjust",   (XtActionProc) ExtendAdjust },
   { "extend-end",      (XtActionProc) ExtendEnd },
   { "track-motion",    (XtActionProc) TrackMotion },
};

/*
 * For some reason, in Motif1.2/X11R5 the actionsList above gets corrupted
 * When the parent HTML widget is created.  This means we can't use
 * it later with XtAppAddActions to add to the viewing area.
 * So, we make a spare copy here to use with XtAppAddActions.
 */
static XtActionsRec SpareActionsList[] = {
   { "select-start",    (XtActionProc) SelectStart },
   { "extend-start",    (XtActionProc) ExtendStart },
   { "extend-adjust",   (XtActionProc) ExtendAdjust },
   { "extend-end",      (XtActionProc) ExtendEnd },
   { "track-motion",    (XtActionProc) TrackMotion },
};

/*
 *  Resource definitions for HTML widget
 */

static XtResource resources[] = {
  /* Without Motif we need to override the borderWidth to 0 (from 1). */
	{ WbNmarginWidth, WbCMarginWidth, XtRDimension, sizeof (Dimension),
	  XtOffset (HTMLWidget, html.margin_width),
	  XtRImmediate, (caddr_t) MARGIN_DEFAULT
	},
	{ WbNmarginHeight, WbCMarginHeight, XtRDimension, sizeof (Dimension),
	  XtOffset (HTMLWidget, html.margin_height),
	  XtRImmediate, (caddr_t) MARGIN_DEFAULT
	},
	{ WbNanchorCallback, XtCCallback, XtRCallback, sizeof (XtCallbackList),
	  XtOffset (HTMLWidget, html.anchor_callback),
	  XtRImmediate, (caddr_t) NULL
	},
	{ WbNsubmitFormCallback, XtCCallback, XtRCallback, sizeof(XtCallbackList),
	  XtOffset (HTMLWidget, html.form_callback), XtRImmediate, (caddr_t) NULL
	},
	{ WbNbodyColors, WbCBodyColors, XtRBoolean, sizeof (Boolean),
	  XtOffset (HTMLWidget, html.body_colors), XtRString, "True"
	},
	{ WbNbodyImages, WbCBodyImages, XtRBoolean, sizeof (Boolean),
	  XtOffset (HTMLWidget, html.body_images), XtRString, "True"
	},
	{ WbNanchorUnderlines, WbCAnchorUnderlines, XtRInt, sizeof (int),
	  XtOffset (HTMLWidget, html.num_anchor_underlines), XtRString, "0"
	},
	{ WbNvisitedAnchorUnderlines, WbCVisitedAnchorUnderlines, 
	  XtRInt, sizeof (int),
	  XtOffset (HTMLWidget, html.num_visitedAnchor_underlines),
	  XtRString, "0"
	},
	{ WbNdashedAnchorUnderlines, WbCDashedAnchorUnderlines, 
	  XtRBoolean, sizeof (Boolean),
	  XtOffset (HTMLWidget, html.dashed_anchor_lines),
	  XtRString, "False"
	},
	{ WbNdashedVisitedAnchorUnderlines, WbCDashedVisitedAnchorUnderlines, 
	  XtRBoolean, sizeof (Boolean),
	  XtOffset (HTMLWidget, html.dashed_visitedAnchor_lines),
	  XtRString, "False"
	},
	{ WbNanchorColor, XtCForeground, XtRPixel, sizeof (Pixel),
	  XtOffset (HTMLWidget, html.def_res.fg_link), XtRString, "#0000ff"
	},
	{ WbNvisitedAnchorColor, XtCForeground, XtRPixel, sizeof (Pixel),
	  XtOffset (HTMLWidget, html.def_res.fg_vlink), XtRString, "#00FFFF"
	},
	{ WbNactiveAnchorFG, XtCBackground, XtRPixel, sizeof (Pixel),
	  XtOffset (HTMLWidget, html.def_res.fg_alink), XtRString, "#ff0000"
	},
	{ WbNisIndex, WbCIsIndex, XtRBoolean, sizeof (Boolean),
	  XtOffset (HTMLWidget, html.is_index), XtRString, "False"
	},
	{ WbNview, WbCView, XtRWidget, sizeof (Widget),
	  XtOffset (HTMLWidget, html.view), XtRImmediate, NULL
	},
	{ XtNfont, XtCFont, XtRFontStruct, sizeof (XFontStruct *),
	  XtOffset (HTMLWidget, html.cur_font),
	  XtRString, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"
	},
	{ WbNdefaultFont, WbCDefaultFont, XtRFontStruct, sizeof (XFontStruct *),
	  XtOffset (HTMLWidget, html.default_font),
	  XtRString, "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*"
	},
        { WbNpreviouslyVisitedTestFunction, WbCPreviouslyVisitedTestFunction, 
	  XtRPointer, sizeof (XtPointer),
          XtOffset (HTMLWidget, html.previously_visited_test),
          XtRImmediate, (caddr_t) NULL
        },
        { WbNpointerMotionCallback, WbCPointerMotionCallback, 
	  XtRCallback, sizeof(XtCallbackList),
          XtOffset (HTMLWidget, html.pointer_motion_callback),
          XtRImmediate, (caddr_t) NULL
        },
	{ WbNmaxColorsInImage, WbCMaxColorsInImage, XtRInt, sizeof (int),
	  XtOffset (HTMLWidget, html.max_colors_in_image),
	  XtRImmediate, (caddr_t) 50
	},
	{ XmNverticalScrollBar, XmCVerticalScrollBar, XtRWidget, sizeof (Widget),
	  XtOffset (HTMLWidget, html.vbar), XtRImmediate, (caddr_t) NULL
	},
	{ XmNhorizontalScrollBar, XmCHorizontalScrollBar, XtRWidget, sizeof (Widget),
	  XtOffset (HTMLWidget, html.hbar), XtRImmediate, NULL
	}
};


HTMLClassRec htmlClassRec = {
   {						/* core class fields  */
      (WidgetClass) &xmManagerClassRec,		/* superclass         */
      "HTML",					/* class_name         */
      sizeof(HTMLRec),				/* widget_size        */
      NULL,					/* class_initialize   */
      NULL,					/* class_part_init    */
      FALSE,					/* class_inited       */
      (XtInitProc) Initialize,			/* initialize         */
      NULL,					/* initialize_hook    */
      Realize,					/* realize            */
      actionsList,				/* actions	      */
      XtNumber(actionsList),			/* num_actions	      */
      resources,				/* resources          */
      XtNumber(resources),			/* num_resources      */
      NULLQUARK,				/* xrm_class          */
      TRUE,					/* compress_motion    */
      FALSE,					/* compress_exposure  */
      TRUE,					/* compress_enterlv   */
      FALSE,					/* visible_interest   */
      NULL,			                /* destroy            */
      (XtWidgetProc) Resize,			/* resize             */
      (XtExposeProc) Redisplay,			/* expose             */
      (XtSetValuesFunc) SetValues,		/* set_values         */
      NULL,					/* set_values_hook    */
      XtInheritSetValuesAlmost,			/* set_values_almost  */
      NULL,					/* get_values_hook    */
      html_accept_focus,			/* accept_focus       */
      XtVersion,				/* version            */
      NULL,					/* callback_private   */
      defaultTranslations,			/* tm_table           */
      XtInheritQueryGeometry,			/* query_geometry     */
      XtInheritDisplayAccelerator,              /* display_accelerator*/
      NULL,		                        /* extension          */
   },
   {		/* composite_class fields */
      (XtGeometryHandler) GeometryManager,   	/* geometry_manager   */
      NULL,					/* change_managed     */
      XtInheritInsertChild,			/* insert_child       */
      XtInheritDeleteChild,			/* delete_child       */
      NULL,                                     /* extension          */
   },
   {		/* constraint_class fields */
      NULL,					/* resource list        */   
      0,					/* num resources        */   
      0,					/* constraint size      */   
      NULL,					/* init proc            */   
      NULL,					/* destroy proc         */   
      NULL,					/* set values proc      */   
      NULL,                                     /* extension            */
   },
   {		/* manager_class fields */
      XtInheritTranslations,			/* translations           */
      NULL,					/* syn_resources      	  */
      0,					/* num_syn_resources 	  */
      NULL,					/* syn_cont_resources     */
      0,					/* num_syn_cont_resources */
      XmInheritParentProcess,                   /* parent_process         */
      NULL,					/* extension 	          */    
   },
   {		/* html_class fields */     
      0						/* none			  */
   }	
};

WidgetClass htmlWidgetClass = (WidgetClass)&htmlClassRec;

static Cursor in_anchor_cursor = (Cursor)NULL;


void hw_do_body_bgima(HTMLWidget hw, struct mark_up *mptr) 
{
	ImageInfo lpicd;

	if ( !mptr->s_picd)
		return;
	if ( !mptr->s_picd->fetched)
		return;
	lpicd = *(mptr->s_picd);

	hw->html.cur_res.have_bgima=1;
	hw->html.cur_res.bgima_height=lpicd.height;
	hw->html.cur_res.bgima_width=lpicd.width;
	hw->html.cur_res.bgimamap=lpicd.image;
	hw->html.view->core.background_pixmap = lpicd.image;
	XSetTile(XtDisplay(hw), hw->html.bgimgGC, hw->html.cur_res.bgimamap);
	XSetTSOrigin(XtDisplay(hw), hw->html.bgimgGC, 0, 0);
	XSetFillStyle(XtDisplay(hw), hw->html.bgimgGC, FillTiled);
}

void hw_do_body_color(HTMLWidget hw, const char *att, char *cname,
	PhotoComposeContext * pcc)
{
	XColor col ;
/*	XColor fg,sel,ts,bs; */
/*	XmColorProc calc; */
	Colormap cmap;
	int status ;

	if (!att || !*att || !cname || !*cname)
		return;

	cmap = hw->core.colormap;
	status = XParseColor(XtDisplay(hw),cmap, cname, &col);
	col.flags = DoRed | DoGreen | DoBlue;
	if (!status) {	/* pas d'allocation ou erreur on laisse telque */
			/* a cause de XFreeColors qui peut faire une erreur */
			/* BadAccess et ca plante mMosaic */
		fprintf(stderr,"Cannot alloc Body Color Name: %s\n", cname);
		return;
	}
	col.pixel= HTMLXColorToPixel(&col);


	if (!strcasecmp(att,"text")) {
		hw->html.cur_res.fg_text = col.pixel;	/* c'est pour le BODY */
		pcc->fg_text = hw->html.cur_res.fg_text; /* pour le courant*/
		pcc->fg_text = MMPushColorFg(hw,col.pixel);
	}

	if (!strcasecmp(att,"bgcolor")){ /* calculate shadow colors */
/*		calc = XmGetColorCalculation();		*/
/*		calc(&col, &fg, &sel, &ts, &bs);	*/
/*		ts.pixel = HTMLXColorToPixel(&ts);	*/
/*		hw->manager.top_shadow_color = ts.pixel;*/
/*		bs.pixel = HTMLXColorToPixel(&bs);	*/

		hw->html.view->core.background_pixel = col.pixel ;
		hw->html.cur_res.bgcolor = col.pixel;
		pcc->bgcolor = MMPushColorBg(hw, col.pixel);
		XSetWindowBackground(XtDisplay(hw), XtWindow(hw->html.view),
			hw->html.cur_res.bgcolor);
	}
	if (!strcasecmp(att,"link"))
		hw->html.cur_res.fg_link = col.pixel;
	if (!strcasecmp(att,"vlink"))
		hw->html.cur_res.fg_vlink = col.pixel;
	if (!strcasecmp(att,"alink"))
		hw->html.cur_res.fg_alink = col.pixel;
	return;
}

/* Process an expose event in the View (or drawing area).  This 
 * Can be a regular expose event, or perhaps a GraphicsExpose Event.
 */
static void DrawExpose(Widget w, caddr_t data, XEvent *event)
{
	XEvent NewEvent;
	HTMLWidget hw = (HTMLWidget)data;
	int x, y, x1, y1;
	int width, height;
	int nx, ny, nwidth, nheight;
	Display *dsp;        
	Window win;           

        if (!(event->xany.type==Expose || event->xany.type==GraphicsExpose)) {
                return;              
        } 

        if (event->xany.type==Expose) {
                x = event->xexpose.x;
                y = event->xexpose.y;
                width = event->xexpose.width;
                height = event->xexpose.height;
        } else {                       
                x = event->xgraphicsexpose.x;
                y = event->xgraphicsexpose.y;
                width = event->xgraphicsexpose.width;
                height = event->xgraphicsexpose.height;
        }  

	/* Get rid of any extra expose events.
	 * Be sure to get the entire area of exposure.
	 */
        dsp=XtDisplay(w);            
        win=XtWindow(w); 

	while(XCheckWindowEvent(dsp, win, ExposureMask, &NewEvent) == True) {
		if(NewEvent.xany.type==Expose ||
		   NewEvent.xany.type==GraphicsExpose) {
			if (NewEvent.xany.type==Expose) {
				nx = NewEvent.xexpose.x;
				ny = NewEvent.xexpose.y;
				nwidth = NewEvent.xexpose.width;
				nheight = NewEvent.xexpose.height;
			} else {      
				nx = NewEvent.xgraphicsexpose.x;
				ny = NewEvent.xgraphicsexpose.y;
				nwidth = NewEvent.xgraphicsexpose.width;
				nheight = NewEvent.xgraphicsexpose.height;
			}           
			x1 = x + width;
			y1 = y + height;
                                    
			if (x > nx)
				x = nx;
			if (y > ny)
				y = ny;
			if (x1 < (nx + nwidth))
				x1 = nx + nwidth;
			if (y1 < (ny + nheight))
				y1 = ny + nheight;
			width = x1 - x;
			height = y1 - y;
		}                   
	}    
	ViewRedisplay(hw, x, y, width, height);
}

/*##################*/
void ResetWidgetsOnResize( HTMLWidget hw)
{       
        WidgetInfo *wptr;
        
        wptr = hw->html.widget_list;
        while (wptr != NULL) {
                if (wptr->w != NULL) {                    
                        wptr->seeable=1;
                }
                wptr = wptr->next;   
        }
        return;                      
}

void ScrollWidgets(HTMLWidget hw)
{
	WidgetInfo *wptr;
	int xval, yval;

	xval = hw->html.scroll_x;
	yval = hw->html.scroll_y;
	wptr = hw->html.widget_list;
	while (wptr != NULL) {
		if (wptr->w != NULL) {
			Widget w;
			int x, y;

			w = wptr->w;
                        x = wptr->x-xval;
                        y = wptr->y-yval;

                        /* lower_right on screen?
                         * lower_left on screen?
                         * upper_right on screen?
                         * upper_left on screen?
                         *           
                         * if any of the above, move the widget, otherwise
                         *   it is not "seeable". Incredible speed for many
                         *   widget pages. 
                         *           
                         * SWP and TPR
                         */     
                        if (((y>0 && y<=hw->html.view_height) ||
                             ((y+wptr->height)>0 &&
                              (y+wptr->height)<=hw->html.view_height)) &&
                            ((x>0 && x<=hw->html.view_width) ||
                             ((x+wptr->width)>0 &&
                              (x+wptr->width)<=hw->html.view_width))) {
                                wptr->seeable=1;
                                XtMoveWidget(w, x, y);
                        } else if (wptr->seeable) {
                                wptr->seeable=0;
                                XtMoveWidget(w, x, y);
                        }  

		}
		wptr = wptr->next;
	}
#ifdef MULTICAST
/* faire un callback quand on scroll */
/* ############
	if (mc_send_win){
        	unsigned char code;     
        	unsigned long gmt_send_time;
        	unsigned int html_goto_id;

		if (mc_data_send_data_struct.is_send == 0)
			return; 
		code = MCR_HTML_GOTO_ID;
        	gmt_send_time = McDate();

        	html_goto_id = HTMLPositionToId(mc_send_win->scrolled_win, 0, 3);
        
        	McSendRtpGotoId( code, mc_my_pid, mc_local_url_id,
              			gmt_send_time, html_goto_id);
	}
####### */
#endif
}

/* Either the vertical or hortizontal scrollbar has been moved */

void ScrollToPos(Widget w, HTMLWidget hw, int value)
{
	/* Special code incase the scrollbar is "moved" before we have a window
	 * (if we have a GC we have a window)
	 */
	if (hw->html.drawGC == NULL) {
		if (w == hw->html.vbar) {
			hw->html.scroll_y = value;
		} else if (w == hw->html.hbar)
			hw->html.scroll_x = value;
		return;
	}
	/*
	 * If we've moved the vertical scrollbar
	 */
	if (w == hw->html.vbar) {
/* from malber */
          if (hw->html.scroll_y - value > 0
              && hw->html.scroll_y - value < hw->html.view_height)
          {
              unsigned int depl = hw->html.scroll_y - value;
              XCopyArea(XtDisplay(hw), XtWindow(hw->html.view)
                        , XtWindow(hw->html.view), hw->manager.background_GC
                        , 0, 0
                        , hw->html.view_width
                        , hw->html.view_height - depl
                        , 0, depl);
              hw->html.scroll_y = value;
              XClearArea(XtDisplay(hw), XtWindow(hw->html.view),
                      0, 0,
                      hw->html.view_width,
                      depl, False);
              ViewRedisplay(hw, 0, 0,
                      hw->html.view_width,
                      depl);

          } else if (value - hw->html.scroll_y > 0
              && value - hw->html.scroll_y < hw->html.view_height)
          {
              unsigned int depl = value - hw->html.scroll_y;
              XCopyArea(XtDisplay(hw), XtWindow(hw->html.view)
                        , XtWindow(hw->html.view), hw->manager.background_GC
                        , 0, depl
                        , hw->html.view_width
                        , hw->html.view_height - depl
                        , 0, 0);
              hw->html.scroll_y = value;
              XClearArea(XtDisplay(hw), XtWindow(hw->html.view),
                      0, hw->html.view_height - depl,
                      hw->html.view_width,
                      depl, False);  
              ViewRedisplay(hw, 0, hw->html.view_height - depl,
                      hw->html.view_width,
                      depl);         
                                     
            } else {

/* malber */
		hw->html.scroll_y = value;
		XClearArea(XtDisplay(hw), XtWindow(hw->html.view),
			0, 0,
			hw->html.view_width,
			hw->html.view_height, False);
		ViewRedisplay(hw, 0, 0,
			hw->html.view_width,
			hw->html.view_height);
	   }
	}
	/*
	 * Else we've moved the horizontal scrollbar
	 */
	if (w == hw->html.hbar) {
		hw->html.scroll_x = value;
		XClearArea(XtDisplay(hw), XtWindow(hw->html.view),
			0, 0,
			hw->html.view_width,
			hw->html.view_height, False);
		ViewRedisplay(hw, 0, 0,
			hw->html.view_width,
			hw->html.view_height);
	}
	ScrollWidgets(hw);
}

/*
 * Either the vertical or hortizontal scrollbar has been moved
 */
void ScrollMove(Widget w, caddr_t client_data, caddr_t call_data)
{
	XmScrollBarCallbackStruct *sc = (XmScrollBarCallbackStruct *)call_data;

	ScrollToPos(w, (HTMLWidget)client_data, sc->value);
}
static void Realize(Widget ww, XtValueMask *valueMask, XSetWindowAttributes *attrs)     
{       
        HTMLWidget w= (HTMLWidget) ww;
        
        XtCreateWindow(ww,InputOutput,(Visual *)CopyFromParent,
                *valueMask,attrs);      
        
        XtRealizeWidget(w->html.view);
        XtRealizeWidget(w->html.vbar);
        XtRealizeWidget(w->html.hbar);
} /* End Realize */ 

/* Return the width of the vertical scrollbar */
static Dimension VbarWidth( /*HTMLWidget hw*/)
{
	return(VERT_SCROLL_WIDTH);
}

/* Return the height of the horizontal scrollbar */

static Dimension HbarHeight(void)
{
	return(HORIZ_SCROLL_HEIGHT);
}

/* Resize and set the min and max values of the scrollbars.  Position viewing
 * area based on scrollbar locations.
 */
static void ConfigScrollBars( HTMLWidget hw)
{
	Arg arg[20];
	Cardinal argcnt;
	int vx, vy;

/* Move and size the viewing area */
	vx = hw->manager.shadow_thickness;
	vy = hw->manager.shadow_thickness;
	XtMoveWidget(hw->html.view, vx, vy);
	XtResizeWidget(hw->html.view, hw->html.view_width, hw->html.view_height,
		hw->html.view->core.border_width);
/* Set up vertical scrollbar */
	if (hw->html.use_vbar == True) {
		int maxv;
		int ss;

/* Size the vertical scrollbar to the height of the viewing area */
		XtResizeWidget(hw->html.vbar, hw->html.vbar->core.width,
		    hw->html.view_height + (2 * hw->manager.shadow_thickness),
		    hw->html.vbar->core.border_width);

/* Set the slider size to be the percentage of the viewing area that
 * the viewing area is of the document area. Or set it to 1 if that isn't
 * possible. */
		if (hw->html.doc_height == 0) {
			ss = 1;
		} else {
#ifdef HTMLTRACE
		      fprintf(stderr,"view_height %d, doc_height %d\n",
			    hw->html.view_height, hw->html.doc_height);
#endif
/* Added by marca: this produces results *very* close (~1 pixel)
 * to the original scrolled window behavior. */
                        ss = hw->html.view_height;
		}
		if (ss < 1)
			ss = 1;
#ifdef HTMLTRACE
		fprintf (stderr, "computed ss to be %d\n", ss);
#endif
/* If resizing of the document has made scroll_y
 * greater than the max, we want to hold it at the max.
 */
		maxv = hw->html.doc_height - (int)hw->html.view_height;
		if (maxv < 0)
			maxv = 0;
		if (hw->html.scroll_y > maxv)
			hw->html.scroll_y = maxv;
/* Prevent the Motif max value and slider size from going to zero, which is illegal */
		maxv = maxv + ss;
		if (maxv < 1)
			maxv = 1;
/* Motif will not allow the actual value to be equal to its max value. Adjust
 * accordingly. Since we might decrease scroll_y, cap it at zero. */
		if (hw->html.scroll_y >= maxv)
			hw->html.scroll_y = maxv - 1;
		if (hw->html.scroll_y < 0)
			hw->html.scroll_y = 0;
		argcnt = 0;
		XtSetArg(arg[argcnt], XmNminimum, 0); argcnt++;
		XtSetArg(arg[argcnt], XmNmaximum, maxv); argcnt++;
		XtSetArg(arg[argcnt], XmNvalue, hw->html.scroll_y); argcnt++;
		XtSetArg(arg[argcnt], XmNsliderSize, ss); argcnt++;
                XtSetArg(arg[argcnt], XmNincrement, Y_DEFAULT_INCREMENT); argcnt++;
                XtSetArg(arg[argcnt], XmNpageIncrement, 
                         hw->html.view_height > Y_DEFAULT_INCREMENT ? 
                           hw->html.view_height - Y_DEFAULT_INCREMENT :1); argcnt++;
		XtSetValues(hw->html.vbar, arg, argcnt);
#ifdef HTMLTRACE
		XtVaGetValues(hw->html.vbar, XmNsliderSize, &ss, NULL);
		fprintf (stderr, "real slider size %d\n", ss);
#endif
	}
/* Set up horizontal scrollbar */
	if (hw->html.use_hbar == True) {
		int maxv;
		int ss;

		/* Size the horizontal scrollbar to the width of
		 * the viewing area
		 */
		XtResizeWidget(hw->html.hbar,
		    hw->html.view_width + (2 * hw->manager.shadow_thickness),
		    hw->html.hbar->core.height, hw->html.hbar->core.border_width);
		/* Set the slider size to be the percentage of the
		 * viewing area that the viewing area is of the
		 * document area.  Or set it to 1 if that isn't possible.
		 */
		if (hw->html.doc_width == 0) {
			ss = 1;
		} else {
                        /* marca: this produces results *very* close (~1 pixel)
                         * to the original scrolled window behavior. */
                        ss = hw->html.view_width;
		}
		if (ss < 1)
			ss = 1;
		/* If resizing of the document has made scroll_x
		 * greater than the max, we want to hold it at the max.
		 */
		maxv = hw->html.doc_width - (int)hw->html.view_width;
		if (maxv < 0)
			maxv = 0;
		if (hw->html.scroll_x > maxv)
			hw->html.scroll_x = maxv;
		/* Prevent the Motif max value and slider size
		 * from going to zero, which is illegal
		 */
		maxv = maxv + ss;
		if (maxv < 1)
			maxv = 1;
		/*
		 * Motif will not allow the actual value to be equal to
		 * its max value.  Adjust accordingly.
		 * Since we might decrease scroll_x, cap it at zero.
		 */
		if (hw->html.scroll_x >= maxv)
			hw->html.scroll_x = maxv - 1;
		if (hw->html.scroll_x < 0)
			hw->html.scroll_x = 0;
		argcnt = 0;
		XtSetArg(arg[argcnt], XmNminimum, 0); argcnt++;
		XtSetArg(arg[argcnt], XmNmaximum, maxv); argcnt++;
		XtSetArg(arg[argcnt], XmNvalue, hw->html.scroll_x); argcnt++;
		XtSetArg(arg[argcnt], XmNsliderSize, ss); argcnt++;
                XtSetArg(arg[argcnt], XmNincrement, X_DEFAULT_INCREMENT); argcnt++;
                XtSetArg(arg[argcnt], XmNpageIncrement, 
                         hw->html.view_width > X_DEFAULT_INCREMENT ? 
                         hw->html.view_width - X_DEFAULT_INCREMENT : 1); argcnt++;
		XtSetValues(hw->html.hbar, arg, argcnt);
	}
#ifdef HTMLTRACE
	{
		int ss;
		XtVaGetValues(hw->html.vbar, XmNsliderSize, &ss, NULL);
		fprintf (stderr, "real slider size %d\n", ss);
        }
#endif
}

/* Reformat the window and scrollbars. May be called because of a changed document
 */
/* if save_obj == True alors ca vient d'un resize : c'est la meme page
 * il n'y a pas besoin de recreer certain elements, juste de changer leur taille*/

void ReformatWindow( HTMLWidget hw, Boolean save_obj)
{
	int temp;
	int new_width;
	Dimension swidth, sheight;
	Dimension st;

/* Find the current scrollbar sizes, and shadow thickness and format
 * the document to the current window width (assume a vertical scrollbar)
 */
	swidth = VbarWidth();
	sheight = HbarHeight();
	st = hw->manager.shadow_thickness;
	if (hw->core.width <= swidth+2*st)
		hw->core.width = swidth + 10 + 2*st;
	new_width = hw->core.width - swidth -2*st;
	temp = FormatAll(hw, new_width,save_obj);

/* if height is too height tell the wiget to use the vbar */
	if ( temp > hw->core.height - HbarHeight() ){
		hw->html.use_vbar = True;
	} else { 
		hw->html.use_vbar = False;
	}

/* If we need the vertical scrollbar, place and manage it, and store the
 * current viewing area width.
 */
	hw->html.view_width = hw->core.width;
	if (hw->html.use_vbar ) {
		hw->html.view_width = hw->core.width - swidth - (2 * st);
		XtMoveWidget(hw->html.vbar, hw->core.width - swidth, 0);
		XtManageChild(hw->html.vbar);
		if (XtIsRealized(hw->html.vbar) )
			XtMapWidget(hw->html.vbar);
	} else {
/* Else we were wrong to assume a vertical scrollbar. Remove it. */
		if (XtIsRealized(hw->html.vbar) )
			XtUnmapWidget(hw->html.vbar);
		hw->html.scroll_y = 0;
	}
/* Calculate the actual max width and height of the complete formatted document.
 * The max width may exceed the preformatted width due to special factors in
 * the formatting of the widget. Use the max of the 2 here, but leave
 * max_pre_width unchanged for future formatting calls.
 * new_width includes the margins, and hw->html.max_pre_width does not,
 * fix that here.
 */
	new_width = new_width - (2 * hw->html.margin_width);
	if (hw->html.max_pre_width > new_width)
		new_width = hw->html.max_pre_width;
/* If the maximum width derives from a formatted, as opposed to
 * unformatted piece of text, allow a 20% of margin width slop
 * over into the margin to cover up a minor glick with terminaing
 * punctuation after anchors at the end of the line.
 */
	else
		new_width = new_width - (10 * hw->html.margin_width / 100);

	hw->html.doc_height = temp;
	hw->html.doc_width = new_width + (2 * hw->html.margin_width);
	if (hw->html.view_width > hw->html.doc_width)
		hw->html.doc_width = hw->html.view_width;

/* If we need a horizontal scrollbar Place it and manage it. Save the height
 * of the current viewing area.
 */
	if (hw->html.doc_width > hw->html.view_width) {
		hw->html.use_hbar = True;
		XtMoveWidget(hw->html.hbar, 0,
					(hw->core.height - sheight));
		XtManageChild(hw->html.hbar);
		if (XtIsRealized(hw->html.hbar) )
			XtMapWidget(hw->html.hbar);
		hw->html.view_height = hw->core.height - sheight - (2 * st);
	} else {
/* Else we don't need a horizontal scrollbar.
 * Remove it and save the current viewing area height.
 */
		hw->html.use_hbar = False;
		XtUnmanageChild(hw->html.hbar);
		if (XtIsRealized(hw->html.hbar) )
			XtUnmapWidget(hw->html.hbar);
		hw->html.scroll_x = 0;
		hw->html.view_height = hw->core.height - (2 * st);
	}

/* Configure the scrollbar min, max, and slider sizes */
	ConfigScrollBars(hw);
}

/* We're a happy widget.  We let any child move or resize themselves
 * however they want, we don't care.
 */
static XtGeometryResult GeometryManager ( Widget w,
	XtWidgetGeometry * request, XtWidgetGeometry * reply)
{
	if (request->request_mode & CWX)
		reply->x = request->x;
	if (request->request_mode & CWY)
		reply->y = request->y;
	if (request->request_mode & CWWidth)
		reply->width = request->width;
	if (request->request_mode & CWHeight)
		reply->height = request->height;
	if (request->request_mode & CWBorderWidth)
		reply->border_width = request->border_width;
/*
	if (request->request_mode & CWSibling)
		reply->sibling = request->sibling;
*/
	reply->request_mode = request->request_mode;
	return (XtGeometryYes);
}

/* Initialize is called when the widget is first initialized (created).
 * Check to see that all the starting resources are valid.
 */
static void Initialize( HTMLWidget request, HTMLWidget nw)
{
	Arg arg[20];
	Cardinal argcnt;
	XtTranslations trans;
	unsigned long valuemask;
	XGCValues values;

/* Make sure height and width are not zero. */
	if (nw->core.width == 0)
		nw->core.width = nw->html.margin_width << 1 ;
	if (nw->core.width == 0)
		nw->core.width = MARGIN_DEFAULT ;
	if (nw->core.height == 0)
		nw->core.height = nw->html.margin_height << 1 ;
	if (nw->core.height == 0)
		nw->core.height = MARGIN_DEFAULT ;
/* Make sure the underline numbers are within bounds. */
	if (nw->html.num_anchor_underlines < 0)
		nw->html.num_anchor_underlines = 0;
	if (nw->html.num_anchor_underlines > MAX_UNDERLINES)
		nw->html.num_anchor_underlines = MAX_UNDERLINES;
	if (nw->html.num_visitedAnchor_underlines < 0)
		nw->html.num_visitedAnchor_underlines = 0;
	if (nw->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
		nw->html.num_visitedAnchor_underlines = MAX_UNDERLINES;

	nw->html.view_width = nw->core.width;
	nw->html.view_height = nw->core.height;

/* Parse the raw text with the HTML parser.  And set the formatted 
 * element list to NULL. */
	nw->html.html_objects = NULL;

	nw->html.formatted_elements = NULL;
	nw->html.widget_list = NULL;
	nw->html.form_list = NULL;

	nw->html.nframe = 0;
	nw->html.frame_type = NOTFRAME_TYPE;
	nw->html.frames = NULL;

/* Find the max width of a preformatted line in this document. */
	nw->html.max_pre_width = 0;

	nw->html.drawGC = NULL; 	/* Initialize private widget resources */
	nw->html.bgimgGC = NULL; 	/* Initialize private widget resources */
	nw->html.select_start = NULL;
	nw->html.select_end = NULL;
	nw->html.sel_start_pos = 0;
	nw->html.sel_end_pos = 0;
	nw->html.new_start = NULL;
	nw->html.new_end = NULL;
	nw->html.new_start_pos = 0;
	nw->html.new_end_pos = 0;
	nw->html.active_anchor = NULL;
	nw->html.press_x = 0;
	nw->html.press_y = 0;

/* color setting */
	nw->html.def_res.bgcolor = nw->core.background_pixel;
	nw->html.def_res.fgcolor = nw->manager.foreground;
	nw->html.def_res.top_color = nw->manager.top_shadow_color;
	nw->html.def_res.bottom_color = nw->manager.bottom_shadow_color;
	nw->html.def_res.fg_text = nw->manager.foreground;
/* def_res.fg_link; def_res.fg_vlink; def_res.fg_alink is set by ressources */
	nw->html.def_res.have_bgima = 0;	/* init with no bgimage */
	nw->html.def_res.bgimamap = None;
	nw->html.def_res.bgima_height = 0;
	nw->html.def_res.bgima_width = 0;
/* create and init color stack */
	MMInitWidgetColorStack(nw);

/* copy in current */
	nw->html.cur_res = nw->html.def_res;

/* Initialize cursor used when pointer is inside anchor. */
        if (in_anchor_cursor == (Cursor)NULL)
        	in_anchor_cursor = XCreateFontCursor (XtDisplay (nw), XC_hand2);

/*Make sure we have a valid GC to draw with.*/

	values.function = GXcopy;
	values.plane_mask = AllPlanes;
	values.foreground = nw->manager.foreground;
	values.background = nw->core.background_pixel;
	values.fill_style = FillSolid;
	valuemask = GCFunction|GCPlaneMask|GCForeground|GCBackground| GCFillStyle;
	nw->html.drawGC = XCreateGC(XtDisplay(nw),
				    DefaultRootWindow(XtDisplay(nw)),
				    valuemask, &values);
	nw->html.bgimgGC = XCreateGC(XtDisplay(nw),
				    DefaultRootWindow(XtDisplay(nw)),
				    valuemask, &values);

/* Create the scrollbars. Find their dimensions and then decide which
 * scrollbars you will need, and what the dimensions of the viewing area are.
 * Start assuming nomapping for a vertical scrollbar and a horizontal one.
 * map vertical if long enough, and map horizontal if wide enough.
 */
	nw->html.vbar = NULL;
	nw->html.hbar = NULL;
	nw->html.view = NULL;
	nw->html.use_vbar = False;
	nw->html.use_hbar = False;
	nw->html.scroll_x = 0;
	nw->html.scroll_y = 0;

/* Create the horizontal and vertical scroll bars. Size them later. */
/* Create the view also */
	argcnt = 0;
	XtSetArg(arg[argcnt], XxNwidth, 10); argcnt++;
	XtSetArg(arg[argcnt], XxNheight, 10); argcnt++;
	XtSetArg(arg[argcnt], XmNmarginWidth, 0); argcnt++;
	XtSetArg(arg[argcnt], XmNmarginHeight, 0); argcnt++;
	nw->html.view = XtCreateWidget("View", xmDrawingAreaWidgetClass,
		(Widget)nw, arg, argcnt);
	XtManageChild(nw->html.view);

/* For the view widget catch all Expose and GraphicsExpose
 * events.  Replace its translations with ours, and make
 * sure all the actions are in order.
 */
	XtAddEventHandler((Widget)nw->html.view,
		ExposureMask|VisibilityChangeMask, True,
		(XtEventHandler)DrawExpose, (caddr_t)nw);

/* As described previoisly, for some reason with Motif1.2/X11R5
 * the list actionsList is corrupted when we get here,
 * so we have to use the special copy SpareActionsList
 */
	XtAppAddActions(XtWidgetToApplicationContext(nw->html.view),
		SpareActionsList, XtNumber(SpareActionsList));
	trans = XtParseTranslationTable(defaultTranslations);
	argcnt = 0;
	XtSetArg(arg[argcnt], XtNtranslations, trans); argcnt++;
	XtSetValues(nw->html.view, arg, argcnt);

/* vert scrollbar . We manage 'manualy' the mapping of srollbar */
	argcnt = 0;
	XtSetArg(arg[argcnt], XmNorientation, XmVERTICAL); argcnt++;
	XtSetArg(arg[argcnt], XtNwidth, VERT_SCROLL_WIDTH); argcnt++;
	XtSetArg(arg[argcnt], XmNmappedWhenManaged, False); argcnt++;
	nw->html.vbar = XtCreateWidget("Vbar", xmScrollBarWidgetClass,
		(Widget)nw, arg, argcnt);
	XtManageChild(nw->html.vbar);
	XtAddCallback(nw->html.vbar, XmNvalueChangedCallback,
		(XtCallbackProc)ScrollMove, (caddr_t)nw);
	XtAddCallback(nw->html.vbar, XmNdragCallback,
		(XtCallbackProc)ScrollMove, (caddr_t)nw);

/* horiz scrollbar*/
	argcnt = 0;
	XtSetArg(arg[argcnt], XmNorientation, XmHORIZONTAL); argcnt++;
	XtSetArg(arg[argcnt], XtNheight, HORIZ_SCROLL_HEIGHT); argcnt++;
	XtSetArg(arg[argcnt], XmNmappedWhenManaged, False); argcnt++;
	nw->html.hbar = XtCreateWidget("Hbar", xmScrollBarWidgetClass,
		(Widget)nw, arg, argcnt);
	XtManageChild(nw->html.hbar);
	XtAddCallback(nw->html.hbar, XmNvalueChangedCallback,
		(XtCallbackProc)ScrollMove, (caddr_t)nw);
	XtAddCallback(nw->html.hbar, XmNdragCallback,
		(XtCallbackProc)ScrollMove, (caddr_t)nw);

/* next call will be Resize or HTMLSetmark or Realize */
}

/* This is called by redisplay.  It is passed a rectangle
 * in the viewing area, and it redisplays that portion of the
 * underlying document area.
 */
void ViewRedisplay( HTMLWidget hw, int x, int y, int width, int height)
{
	int sx, sy;
	int doc_x, doc_y;
	struct ele_rec * eptr;

	if( !XtIsRealized((Widget)hw))
		return;
	if(hw->html.cur_res.have_bgima)
		HTMLDrawBackgroundImage(hw, x, y, width, height);
/* Use scrollbar values to map from view space to document space. */
	sx = sy = 0;
	if (hw->html.use_vbar == True)
		sy += hw->html.scroll_y;
	if (hw->html.use_hbar == True)
		sx += hw->html.scroll_x;
	doc_x = x + sx;
	doc_y = y + sy;

/* Find Element to Refresh */
	eptr = hw->html.formatted_elements;
#if DEBUG_REFRESH
        fprintf(stderr,"[ViewRedisplay] x, y, w, h : %d, %d, %d, %d\n",     
                x, y, width, height);
#endif
	while(eptr){
		if( ((eptr->y + eptr->height) < doc_y) || 
		    (eptr->y > ( doc_y + height)) ){
			eptr = eptr->next;
			continue;
		}
		RefreshElement(hw,eptr, x, y, width, height );
		eptr = eptr->next;
	}
}

void ViewClearAndRefresh( HTMLWidget hw)
{

/* Only refresh if we have a window already. */
	if (!XtIsRealized((Widget)hw))
		return;

	XClearArea(XtDisplay(hw), XtWindow(hw->html.view),
		0, 0, 0, 0, False);
	ViewRedisplay(hw, 0, 0, 
			hw->html.view_width, hw->html.view_height);
/* This is a fake deal to make an Expose event tocall Redisplay
 * to redraw the shadow around the view area
*/
/*	XClearArea(XtDisplay(hw),XtWindow(hw->html.view),0,0,1,1,True); */
}

/* The Redisplay function is what you do with an expose event.
 * Right now we call user callbacks, and then call the CompositeWidget's
 * Redisplay routine.
 */
static void Redisplay( HTMLWidget hw, XEvent * event, Region region)
{
	int dx, dy;

	/* find out where the shadow is based on scrollbars */
	Dimension st = hw->manager.shadow_thickness;

	dx = dy = 0;
	/* Redraw the shadow around the scrolling area which may have been
	 * messed up.
	 */
	_XmDrawShadows(XtDisplay(hw), XtWindow(hw),
		      hw->manager.top_shadow_GC, hw->manager.bottom_shadow_GC,
		      dx, dy,
		      hw->html.view_width+(2*st),
		      hw->html.view_height+(2*st),
		      hw->manager.shadow_thickness,
			XmSHADOW_IN);
	_XmRedisplayGadgets ((Widget)hw, (XEvent*)event, region);
	return;
}

/* Resize is called when the widget changes size.
 * Mostly any resize causes a reformat, except for the special case
 * where the width doesn't change, and the height doesn't change
 * enought to affect the vertical scrollbar.
 * It is too complex to guess exactly what needs to be redrawn, so refresh the
 * whole window on any resize.
 */
static void Resize( HTMLWidget hw)
{
        if( hw->html.frame_type == FRAMESET_TYPE){ /* resize child */
		XtResizeWidget(hw->html.view, hw->core.width, hw->core.height,
			hw->html.view->core.border_width);
		hw->html.view_width= hw->core.width;
		hw->html.view_height = hw->core.height;
		/*ConfigScrollBars(hw);*/
		_XmHTMLReconfigureFrames(hw, hw->html.topframeset_info);
		return;
	}
	ResetWidgetsOnResize(hw);
	ReformatWindow(hw, True);
	ScrollWidgets(hw);
	ViewClearAndRefresh(hw);
}

/*
 * Find the complete text for this the anchor that aptr is a part of
 * and set it into the selection.
 */
static void FindSelectAnchor(HTMLWidget hw, struct ele_rec *aptr)
{
	struct ele_rec *eptr;

	eptr = aptr;
	while((eptr->prev != NULL)&& (eptr->prev->anchor_tag_ptr->anc_href != NULL)&&
	     (strcmp(eptr->prev->anchor_tag_ptr->anc_href, eptr->anchor_tag_ptr->anc_href) == 0))
		eptr = eptr->prev;
	hw->html.select_start = eptr;
	hw->html.sel_start_pos = 0;

	eptr = aptr;
	while((eptr->next != NULL)&& (eptr->next->anchor_tag_ptr->anc_href != NULL)&&
	     (strcmp(eptr->next->anchor_tag_ptr->anc_href, eptr->anchor_tag_ptr->anc_href) == 0))
		eptr = eptr->next;
	hw->html.select_end = eptr;
	hw->html.sel_end_pos = eptr->edata_len - 2;
}

/* Set as active all elements in the widget that are part of the anchor
 * in the widget's start ptr.
 */
static void SetAnchor(HTMLWidget hw)
{
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;
	unsigned long fg;
	unsigned long old_fg;

	eptr = hw->html.active_anchor;
	if ((eptr == NULL)||(eptr->anchor_tag_ptr->anc_href == NULL))
		return;
	fg = hw->html.cur_res.fg_alink;		/* alink is defined for all*/

	FindSelectAnchor(hw, eptr);
	start = hw->html.select_start;
	end = hw->html.select_end;
	eptr = start;
	while ((eptr != NULL)&&(eptr != end)) {
		if (eptr->type == E_TEXT) {
			old_fg = eptr->fg;
			eptr->fg = fg;
			TextRefresh(hw, eptr, 0, (eptr->edata_len - 2));
			eptr->fg = old_fg;
		} else 
			if (eptr->type == E_IMAGE) {
				old_fg = eptr->fg;
				eptr->fg = fg;
				ImageRefresh(hw, eptr);
				eptr->fg = old_fg;
			}
		eptr = eptr->next;
	}
	if (eptr != NULL) {
		if (eptr->type == E_TEXT) {
			old_fg = eptr->fg;
			eptr->fg = fg;
			TextRefresh(hw, eptr, 0, (eptr->edata_len - 2));
			eptr->fg = old_fg;
		} else 
			if (eptr->type == E_IMAGE) {
				old_fg = eptr->fg;
				eptr->fg = fg;
				ImageRefresh(hw, eptr);
				eptr->fg = old_fg;
			}
	}
}

/*
 * Draw selection for all elements in the widget
 * from start to end.
 */
static void DrawSelection( HTMLWidget hw, struct ele_rec *start,
	struct ele_rec *end, int start_pos, int end_pos)
{
	struct ele_rec *eptr;
	int epos;

	if ((start == NULL)||(end == NULL))
		return;
	/* Keep positions within bounds (allows us to be sloppy elsewhere) */
	if (start_pos < 0)
		start_pos = 0;
	if (start_pos >= start->edata_len - 1)
		start_pos = start->edata_len - 2;
	if (end_pos < 0)
		end_pos = 0;
	if (end_pos >= end->edata_len - 1)
		end_pos = end->edata_len - 2;
	if (SwapElements(start, end, start_pos, end_pos)) {
		eptr = start;
		start = end;
		end = eptr;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	}
	eptr = start;
	while ((eptr != NULL)&&(eptr != end)) {
		int p1, p2;

		if (eptr == start) {
			p1 = start_pos;
		} else {
			p1 = 0;
		}
		p2 = eptr->edata_len - 2;
		if (eptr->type == E_TEXT) {
			eptr->selected = True;
			eptr->start_pos = p1;
			eptr->end_pos = p2;
			TextRefresh(hw, eptr, p1, p2);
		} else if (eptr->type == E_LINEFEED) {
			eptr->selected = True;
			LinefeedRefresh(hw, eptr);
		}
		eptr = eptr->next;
	}
	if (eptr != NULL) {
		int p1, p2;

		if (eptr == start) {
			p1 = start_pos;
		} else {
			p1 = 0;
		}

		if (eptr == end) {
			p2 = end_pos;
		} else {
			p2 = eptr->edata_len - 2;
		}
		if (eptr->type == E_TEXT) {
			eptr->selected = True;
			eptr->start_pos = p1;
			eptr->end_pos = p2;
			TextRefresh(hw, eptr, p1, p2);
		} else if (eptr->type == E_LINEFEED) {
			eptr->selected = True;
			LinefeedRefresh(hw, eptr);
		}
	}
}

/*
 * Set selection for all elements in the widget's
 * start to end list.
 */
static void SetSelection( HTMLWidget hw)
{
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;

	start = hw->html.select_start;
	end = hw->html.select_end;
	start_pos = hw->html.sel_start_pos;
	end_pos = hw->html.sel_end_pos;
	DrawSelection(hw, start, end, start_pos, end_pos);
}

/*
 * Erase the selection from start to end
 */
static void EraseSelection( HTMLWidget hw,
	struct ele_rec *start, struct ele_rec *end,
	int start_pos, int end_pos)
{
	struct ele_rec *eptr;
	int epos;

	if ((start == NULL)||(end == NULL))
		return;
	/*
	 * Keep positoins within bounds (allows us to be sloppy elsewhere)
	 */
	if (start_pos < 0)
		start_pos = 0;
	if (start_pos >= start->edata_len - 1)
		start_pos = start->edata_len - 2;
	if (end_pos < 0)
		end_pos = 0;
	if (end_pos >= end->edata_len - 1)
		end_pos = end->edata_len - 2;

	if (SwapElements(start, end, start_pos, end_pos)) {
		eptr = start;
		start = end;
		end = eptr;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	}
	eptr = start;
	while ((eptr != NULL)&&(eptr != end)) {
		int p1, p2;

		p1 = 0;
		if (eptr == start)
			p1 = start_pos;
		p2 = eptr->edata_len - 2;
		if (eptr->type == E_TEXT) {
			eptr->selected = False;
			TextRefresh(hw, eptr, p1, p2);
		} else if (eptr->type == E_LINEFEED) {
			eptr->selected = False;
			LinefeedRefresh(hw, eptr);
		}
		eptr = eptr->next;
	}
	if (eptr != NULL) {
		int p1, p2;

		p1 = 0;
		if (eptr == start)
			p1 = start_pos;
		if (eptr == end) {
			p2 = end_pos;
		} else {
			p2 = eptr->edata_len - 2;
		}
		if (eptr->type == E_TEXT) {
			eptr->selected = False;
			TextRefresh(hw, eptr, p1, p2);
		} else if (eptr->type == E_LINEFEED) {
			eptr->selected = False;
			LinefeedRefresh(hw, eptr);
		}
	}
}

/* Clear the current selection (if there is one)
 */
static void ClearSelection(HTMLWidget hw)
{
	struct ele_rec *start;
	struct ele_rec *end;
	int start_pos, end_pos;

	start = hw->html.select_start;
	end = hw->html.select_end;
	start_pos = hw->html.sel_start_pos;
	end_pos = hw->html.sel_end_pos;
	EraseSelection(hw, start, end, start_pos, end_pos);

	if ((start == NULL)||(end == NULL)) {
		hw->html.select_start = NULL;
		hw->html.select_end = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.sel_end_pos = 0;
		hw->html.active_anchor = NULL;
		return;
	}
	hw->html.select_start = NULL;
	hw->html.select_end = NULL;
	hw->html.sel_start_pos = 0;
	hw->html.sel_end_pos = 0;
	hw->html.active_anchor = NULL;
}

/*
 * clear from active all elements in the widget that are part of the anchor.
 * (These have already been previously set into the start and end of the
 * selection.
 */
static void UnsetAnchor( HTMLWidget hw)
{
	struct ele_rec *eptr;

					/* Clear any activated images */
	eptr = hw->html.select_start;
	while ((eptr != NULL)&&(eptr != hw->html.select_end)) {
		if (eptr->type == E_IMAGE)
			ImageRefresh(hw, eptr);
		eptr = eptr->next;
	}
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
		ImageRefresh(hw, eptr);
	ClearSelection(hw); 		/* Clear the activated anchor */
}

/*
 * Erase the old selection, and draw the new one in such a way
 * that advantage is taken of overlap, and there is no obnoxious
 * flashing.
 */
static void ChangeSelection( HTMLWidget hw, struct ele_rec *start,
	struct ele_rec *end, int start_pos, int end_pos)
{
	struct ele_rec *old_start;
	struct ele_rec *old_end;
	struct ele_rec *new_start;
	struct ele_rec *new_end;
	struct ele_rec *eptr;
	int epos;
	int new_start_pos, new_end_pos;
	int old_start_pos, old_end_pos;

	old_start = hw->html.new_start;
	old_end = hw->html.new_end;
	old_start_pos = hw->html.new_start_pos;
	old_end_pos = hw->html.new_end_pos;
	new_start = start;
	new_end = end;
	new_start_pos = start_pos;
	new_end_pos = end_pos;

	if ((new_start == NULL)||(new_end == NULL))
		return;

	if ((old_start == NULL)||(old_end == NULL)) {
		DrawSelection(hw, new_start, new_end, new_start_pos, new_end_pos);
		return;
	}
	if (SwapElements(old_start, old_end, old_start_pos, old_end_pos)) {
		eptr = old_start;
		old_start = old_end;
		old_end = eptr;
		epos = old_start_pos;
		old_start_pos = old_end_pos;
		old_end_pos = epos;
	}
	if (SwapElements(new_start, new_end, new_start_pos, new_end_pos)) {
		eptr = new_start;
		new_start = new_end;
		new_end = eptr;
		epos = new_start_pos;
		new_start_pos = new_end_pos;
		new_end_pos = epos;
	}
	/*
	 * Deal with all possible intersections of the 2 selection sets.
	 *
	 ********************************************************
	 *			*				*
	 *      |--		*	     |--		*
	 * old--|		*	new--|			*
	 *      |--		*	     |--		*
	 *			*				*
	 *      |--		*	     |--		*
	 * new--|		*	old--|			*
	 *      |--		*	     |--		*
	 *			*				*
	 ********************************************************
	 *			*				*
	 *      |----		*	       |--		*
	 * old--|		*	  new--|		*
	 *      | |--		*	       |		*
	 *      |-+--		*	     |-+--		*
	 *        |		*	     | |--		*
	 *   new--|		*	old--|			*
	 *        |--		*	     |----		*
	 *			*				*
	 ********************************************************
	 *			*				*
	 *      |---------	*	     |---------		*
	 *      |		*	     |			*
	 *      |      |--	*	     |      |--		*
	 * new--| old--|	*	old--| new--|		*
	 *      |      |--	*	     |      |--		*
	 *      |		*	     |			*
	 *      |---------	*	     |---------		*
	 *			*				*
	 ********************************************************
	 */
	if((ElementLessThan(old_end, new_start, old_end_pos, new_start_pos))||
	   (ElementLessThan(new_end, old_start, new_end_pos, old_start_pos))) {
		EraseSelection(hw, old_start, old_end, old_start_pos,old_end_pos);
		DrawSelection(hw, new_start, new_end, new_start_pos, new_end_pos);
	} else if ((ElementLessThan(old_start, new_start,
			old_start_pos, new_start_pos))&&
		 (ElementLessThan(old_end, new_end, old_end_pos, new_end_pos))) {
		if (new_start_pos != 0) {
			EraseSelection(hw, old_start, new_start,
				old_start_pos, new_start_pos - 1);
		} else {
			EraseSelection(hw, old_start, new_start->prev,
				old_start_pos, new_start->prev->edata_len - 2);
		}
		if (old_end_pos < (old_end->edata_len - 2)) {
			DrawSelection(hw, old_end, new_end,
				old_end_pos + 1, new_end_pos);
		} else {
			DrawSelection(hw, old_end->next, new_end, 0, new_end_pos);
		}
	} else if ((ElementLessThan(new_start, old_start,
			new_start_pos, old_start_pos))&&
		 (ElementLessThan(new_end, old_end, new_end_pos, old_end_pos))) {
		if (old_start_pos != 0) {
			DrawSelection(hw, new_start, old_start,
				new_start_pos, old_start_pos - 1);
		} else {
			DrawSelection(hw, new_start, old_start->prev,
				new_start_pos, old_start->prev->edata_len - 2);
		}
		if (new_end_pos < (new_end->edata_len - 2)) {
			EraseSelection(hw, new_end, old_end,
				new_end_pos + 1, old_end_pos);
		} else {
			EraseSelection(hw, new_end->next, old_end,
				0, old_end_pos);
		}
	} else if ((ElementLessThan(new_start, old_start,
			new_start_pos, old_start_pos))||
		 (ElementLessThan(old_end, new_end, old_end_pos, new_end_pos))) {
		if ((new_start != old_start)||(new_start_pos != old_start_pos)) {
			if (old_start_pos != 0) {
				DrawSelection(hw, new_start, old_start,
					new_start_pos, old_start_pos - 1);
			} else {
				DrawSelection(hw, new_start, old_start->prev,
					new_start_pos,
					old_start->prev->edata_len - 2);
			}
		}
		if ((old_end != new_end)||(old_end_pos != new_end_pos)) {
			if (old_end_pos < (old_end->edata_len - 2)) {
				DrawSelection(hw, old_end, new_end,
					old_end_pos + 1, new_end_pos);
			} else {
				DrawSelection(hw, old_end->next, new_end,
					0, new_end_pos);
			}
		}
	} else {
		if ((old_start != new_start)||(old_start_pos != new_start_pos)) {
			if (new_start_pos != 0) {
				EraseSelection(hw, old_start, new_start,
					old_start_pos, new_start_pos - 1);
			} else {
				EraseSelection(hw, old_start, new_start->prev,
					old_start_pos,
					new_start->prev->edata_len - 2);
			}
		}
		if ((new_end != old_end)||(new_end_pos != old_end_pos)) {
			if (new_end_pos < (new_end->edata_len - 2)) {
				EraseSelection(hw, new_end, old_end,
					new_end_pos + 1, old_end_pos);
			} else {
				EraseSelection(hw, new_end->next, old_end,
					0, old_end_pos);
			}
		}
	}
}


static void SelectStart( Widget w, XEvent *event,
	String * params,         /* unused */
	Cardinal * num_params)   /* unused */
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *)event;
	struct ele_rec *eptr;
	int epos;

	if (XtClass(XtParent(w)) != htmlWidgetClass)
		return;
        XUndefineCursor(XtDisplay(hw), XtWindow(hw->html.view));
	/*
	 * Because X sucks, we can get the button pressed in the window, but
	 * released out of the window.  This will highlight some text, but
	 * never complete the selection.  Now on the next button press we
	 * have to clean up this mess.
	 */
	EraseSelection(hw, hw->html.new_start, hw->html.new_end,
		hw->html.new_start_pos, hw->html.new_end_pos);
	/*
	 * We want to erase the currently selected text, but still save the
	 * selection internally in case we don't create a new one.
	 */
	EraseSelection(hw, hw->html.select_start, hw->html.select_end,
		hw->html.sel_start_pos, hw->html.sel_end_pos);
	hw->html.new_start = hw->html.select_start;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = hw->html.sel_start_pos;
	hw->html.new_end_pos = 0;

	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);
	if (eptr != NULL) {
		/*
		 * If this is an anchor assume for now we are activating it
		 * and not selecting it.
		 */
		if (eptr->anchor_tag_ptr->anc_href != NULL) {
			hw->html.active_anchor = eptr;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			SetAnchor(hw);
		}
		/* INPUT TYPE=image ... is like an anchor */
		else if (eptr->type == E_INPUT_IMAGE) {
			hw->html.active_anchor = eptr;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			/*SetAnchor(hw);*/
		}
		/*
		 * Else if we are on an image we can't select text so
		 * pretend we got eptr==NULL, and exit here.
		 */
		else if (eptr->type == E_IMAGE) {
			hw->html.new_start = NULL;
			hw->html.new_end = NULL;
			hw->html.new_start_pos = 0;
			hw->html.new_end_pos = 0;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			hw->html.but_press_time = BuEvent->time;
			return;
		}
		/* Else if we used button2, we can't select text, so exit here. */
		else if (BuEvent->button == Button2) {
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
			hw->html.but_press_time = BuEvent->time;
			return;
		}
		/* Else a single click will not select a new object
		 * but it will prime that selection on the next mouse
		 * move.
		 */
		else {
			hw->html.new_start = eptr;
			hw->html.new_start_pos = epos;
			hw->html.new_end = NULL;
			hw->html.new_end_pos = 0;
			hw->html.press_x = BuEvent->x;
			hw->html.press_y = BuEvent->y;
		}
	} else {
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;
		hw->html.press_x = BuEvent->x;
		hw->html.press_y = BuEvent->y;
	}
	hw->html.but_press_time = BuEvent->time;
}

static void ExtendStart( Widget w, XEvent *event,
	String * params,         /* unused */
	Cardinal * num_params)   /* unused */
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	XButtonPressedEvent *BuEvent = (XButtonPressedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	struct ele_rec *old_start, *old_end;
	int old_start_pos, old_end_pos;
	int start_pos, end_pos;
	int epos;

	if (XtClass(XtParent(w)) != htmlWidgetClass)
		return;
	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);
	if ((eptr != NULL)&&(eptr->type == E_IMAGE)) /* Ignore IMAGE elements. */
		eptr = NULL;
	/*
	 * Ignore NULL elements.
	 */
	if (eptr != NULL) {
		old_start = hw->html.new_start;
		old_start_pos = hw->html.new_start_pos;
		old_end = hw->html.new_end;
		old_end_pos = hw->html.new_end_pos;
		if (hw->html.new_start == NULL) {
			hw->html.new_start = hw->html.select_start;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		} else {
			hw->html.new_end = eptr;
			hw->html.new_end_pos = epos;
		}
		if (SwapElements(hw->html.new_start, hw->html.new_end,
			hw->html.new_start_pos, hw->html.new_end_pos)) {
			if (SwapElements(eptr, hw->html.new_end,
				epos, hw->html.new_end_pos)) {
				start = hw->html.new_end;
				start_pos = hw->html.new_end_pos;
				end = eptr;
				end_pos = epos;
			} else {
				start = hw->html.new_start;
				start_pos = hw->html.new_start_pos;
				end = eptr;
				end_pos = epos;
			}
		} else {
			if (SwapElements(eptr, hw->html.new_start,
			    epos, hw->html.new_start_pos)) {
				start = hw->html.new_start;
				start_pos = hw->html.new_start_pos;
				end = eptr;
				end_pos = epos;
			} else {
				start = hw->html.new_end;
				start_pos = hw->html.new_end_pos;
				end = eptr;
				end_pos = epos;
			}
		}
		if (start == NULL) {
			start = eptr;
			start_pos = epos;
		}
		if (old_start == NULL) {
			hw->html.new_start = hw->html.select_start;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		} else {
			hw->html.new_start = old_start;
			hw->html.new_end = old_end;
			hw->html.new_start_pos = old_start_pos;
			hw->html.new_end_pos = old_end_pos;
		}
		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.new_start = start;
		hw->html.new_end = end;
		hw->html.new_start_pos = start_pos;
		hw->html.new_end_pos = end_pos;
	} else {
		if (hw->html.new_start == NULL) {
			hw->html.new_start = hw->html.select_start;
			hw->html.new_start_pos = hw->html.sel_start_pos;
			hw->html.new_end = hw->html.select_end;
			hw->html.new_end_pos = hw->html.sel_end_pos;
		}
	}
	hw->html.press_x = BuEvent->x;
	hw->html.press_y = BuEvent->y;
}

static void ExtendAdjust( Widget w, XEvent *event,
	String * params,         /* unused */
	Cardinal * num_params)   /* unused */
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	XPointerMovedEvent *MoEvent = (XPointerMovedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	int start_pos, end_pos;
	int epos;

	if (XtClass(XtParent(w)) != htmlWidgetClass)
		return;
	/*
	 * Very small mouse motion immediately after button press is ignored.
	 */
	if ((ABS((hw->html.press_x - MoEvent->x)) <= SELECT_THRESHOLD)&&
	    (ABS((hw->html.press_y - MoEvent->y)) <= SELECT_THRESHOLD))
		return;
	/*
	 * If we have an active anchor and we got here, we have moved the
	 * mouse too far.  Deactivate anchor, and prime a selection.
	 * If the anchor is internal text, don't
	 * prime a selection.
	 */
	if (hw->html.active_anchor != NULL) {
		eptr = hw->html.active_anchor;
		UnsetAnchor(hw);
		hw->html.new_start = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end = NULL;
		hw->html.new_end_pos = 0;
	}

	/*
	 * If we used button2, we can't select text, so
	 * clear selection and exit here.
	 */
	if ((MoEvent->state & Button2Mask) != 0) {
		hw->html.select_start = NULL;
		hw->html.select_end = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.sel_end_pos = 0;
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;
		return;
	}
	eptr = LocateElement(hw, MoEvent->x, MoEvent->y, &epos);

	/*
	 * If we are on an image pretend we are nowhere
	 * and just return;
	 */
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
		return;
	/*
	 * Ignore NULL items.
	 * Ignore if the same as last selected item and position.
	 * Ignore special internal text
	 */
	if ((eptr != NULL)&&
	    ((eptr != hw->html.new_end)||(epos != hw->html.new_end_pos))) {
		start = hw->html.new_start;
		start_pos = hw->html.new_start_pos;
		end = eptr;
		end_pos = epos;
		if (start == NULL) {
			start = eptr;
			start_pos = epos;
		}
		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.new_start = start;
		hw->html.new_end = end;
		hw->html.new_start_pos = start_pos;
		hw->html.new_end_pos = end_pos;
	}
}

static void ExtendEnd( Widget w, XEvent *event,
	String *params,
	Cardinal *num_params)
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	XButtonReleasedEvent *BuEvent = (XButtonReleasedEvent *)event;
	struct ele_rec *eptr;
	struct ele_rec *start, *end;
	Atom *atoms;
	int i, buffer;
	int start_pos, end_pos;
	int epos;
	char *text;

	if (XtClass(XtParent(w)) != htmlWidgetClass)
		return;
	eptr = LocateElement(hw, BuEvent->x, BuEvent->y, &epos);

	/* We first process the IMG usemap=#map */
	if (((BuEvent->button == Button1)||(BuEvent->button == Button2))&&
		(eptr) && (eptr->type == E_IMAGE )&&
		(eptr->pic_data) && (eptr->pic_data->usemap) &&
		((BuEvent->time - hw->html.but_press_time) < CLICK_TIME) ) 
	{
		_HTMLInput(w, eptr, event, params, num_params);
		return;
	}
		
	/*
	 * If we just released button one or two, and we are on an object,
	 * and we have an active anchor, and we are on the active anchor,
	 * and if we havn't waited too long.  Activate that anchor.
	 */
	if (((BuEvent->button == Button1)||(BuEvent->button == Button2))&&
		(eptr != NULL)&&
		(hw->html.active_anchor != NULL)&&
		(eptr == hw->html.active_anchor)&&
		((BuEvent->time - hw->html.but_press_time) < CLICK_TIME))
	{
		_HTMLInput(w, eptr, event, params, num_params);
		return;
	}
	if (hw->html.active_anchor != NULL) {
		start = hw->html.active_anchor;
		UnsetAnchor(hw);
		hw->html.new_start = eptr;
		hw->html.new_start_pos = epos;
		hw->html.new_end = NULL;
		hw->html.new_end_pos = 0;
	}
	/*
	 * If we used button2, we can't select text, so clear
	 * selection and exit here.
	 */
	if (BuEvent->button == Button2) {
		hw->html.new_start = hw->html.select_start;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = hw->html.sel_start_pos;
		hw->html.new_end_pos = 0;
		return;
	}
	/*
	 * If we are on an image, pretend we are nowhere
	 * and NULL out the eptr
	 */
	if ((eptr != NULL)&&(eptr->type == E_IMAGE))
		eptr = NULL;

	/*
	 * If button released on a NULL item, take the last non-NULL
	 * item that we highlighted.
	 */
	if ((eptr == NULL)&&(hw->html.new_end != NULL)) {
		eptr = hw->html.new_end;
		epos = hw->html.new_end_pos;
	}

	if ((eptr != NULL)&& (hw->html.new_end != NULL)) {
		start = hw->html.new_start;
		start_pos = hw->html.new_start_pos;
		end = eptr;
		end_pos = epos;
		if (start == NULL) {
			start = eptr;
			start_pos = epos;
		}
		ChangeSelection(hw, start, end, start_pos, end_pos);
		hw->html.select_start = start;
		hw->html.sel_start_pos = start_pos;
		hw->html.select_end = end;
		hw->html.sel_end_pos = end_pos;
		SetSelection(hw);
		hw->html.new_start = NULL;
		hw->html.new_end = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end_pos = 0;

		atoms = (Atom *)malloc(*num_params * sizeof(Atom));
		if (atoms == NULL) {
			fprintf(stderr, "Out of memory\n");
			assert(0);
		}
		XmuInternStrings(XtDisplay((Widget)hw),params, *num_params,atoms);
		hw->html.selection_time = BuEvent->time;
		for (i=0; i< *num_params; i++) {
			switch (atoms[i]) {
			case XA_CUT_BUFFER0: buffer = 0; break;
			case XA_CUT_BUFFER1: buffer = 1; break;
			case XA_CUT_BUFFER2: buffer = 2; break;
			case XA_CUT_BUFFER3: buffer = 3; break;
			case XA_CUT_BUFFER4: buffer = 4; break;
			case XA_CUT_BUFFER5: buffer = 5; break;
			case XA_CUT_BUFFER6: buffer = 6; break;
			case XA_CUT_BUFFER7: buffer = 7; break;
			default: buffer = -1; break;
			}
			if (buffer >= 0) {
				text = ParseTextToString(
					hw->html.select_start,
					hw->html.select_end,
					hw->html.sel_start_pos,
					hw->html.sel_end_pos,
					hw->html.cur_font->max_bounds.width,
					hw->html.margin_width);
				XStoreBuffer(XtDisplay((Widget)hw),
					text, strlen(text), buffer);
				if (text != NULL)
					free(text);
			} else {
				XtOwnSelection((Widget)hw, atoms[i],
				       BuEvent->time,
				       (XtConvertSelectionProc )ConvertSelection,
				       (XtLoseSelectionProc )LoseSelection,
				       (XtSelectionDoneProc )SelectionDone);
			}
		}
		free((char *)atoms);
	} else if (eptr == NULL) {
		hw->html.select_start = NULL;
		hw->html.sel_start_pos = 0;
		hw->html.select_end = NULL;
		hw->html.sel_end_pos = 0;
		hw->html.new_start = NULL;
		hw->html.new_start_pos = 0;
		hw->html.new_end = NULL;
		hw->html.new_end_pos = 0;
	}
}

#define LEAVING_ANCHOR(hw) \
  XtCallCallbackList ((Widget)hw,hw->html.pointer_motion_callback,&pmcbs);\
  XUndefineCursor (XtDisplay (hw), XtWindow (hw->html.view));

/* KNOWN PROBLEM: We never get LeaveNotify or FocusOut events,
 * despite the fact we've requested them.  Bummer. */

static void TrackMotion( Widget w, XEvent *event,
	String * params,         /* unused */
	Cardinal * num_params)   /* unused */
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	struct ele_rec *eptr;
	int epos, x, y;
	PointerMotionCBStruct pmcbs;

	pmcbs.href = "";
	pmcbs.ev = event;

	if (XtClass(hw) != htmlWidgetClass)
		return;

	if (event->type == MotionNotify) {
		x = ((XMotionEvent *)event)->x;
		y = ((XMotionEvent *)event)->y;
	} else {
		if (event->type == LeaveNotify || event->type == FocusOut ||
		    event->type == Expose) 	 /* Wipe out. */
			LEAVING_ANCHOR (hw);
		return;
	}
	eptr = LocateElement(hw, x, y, &epos);
	if (eptr == NULL){
		LEAVING_ANCHOR (hw);
		return;
	}

/* We're hitting a new anchor if eptr exists and
 * eptr != cached tracked element and anchor_tag_ptr != NULL. */

	if ( eptr->anchor_tag_ptr->anc_href != NULL ||
	     eptr->type == E_INPUT_IMAGE ||
	     (eptr->type == E_IMAGE && eptr->pic_data->usemap )) {
		pmcbs.href = eptr->anchor_tag_ptr->anc_href;
		if (eptr->type == E_INPUT_IMAGE) {
			pmcbs.href = "Send Request";
		}
		if (eptr->type == E_IMAGE && eptr->pic_data->usemap) {
			char * href;

			if( !MapAreaFound(hw, eptr, x, y, &href)) {
				 LEAVING_ANCHOR (hw);
				 return;
			}
			pmcbs.href = href;
		}

		XtCallCallbackList((Widget)hw,
			hw->html.pointer_motion_callback, &pmcbs );
		XDefineCursor (XtDisplay (hw), XtWindow (hw->html.view), 
			in_anchor_cursor);
	} else {
		if (eptr->anchor_tag_ptr->anc_href == NULL && 
		    eptr->type !=E_INPUT_IMAGE &&
		    !(eptr->type ==E_IMAGE && eptr->pic_data->usemap)) {
/* We're leaving an anchor if eptr exists and
 * a cached ele exists and we're not entering a new anchor. */
			LEAVING_ANCHOR (hw);
		}
	}
	return;
}

/* Process mouse input to the HTML widget
 * Currently only processes an anchor-activate when Button1 is pressed
 */
static void _HTMLInput( Widget w, struct ele_rec *eptr, XEvent *event,
	String * params,	/* unused */
	Cardinal * num_params)	/* unused */
{
	HTMLWidget hw = (HTMLWidget)XtParent(w);
	char *tptr, *ptr;
	WbAnchorCallbackData cbdata;
	Boolean on_gadget;
	char *buf = NULL;

	if (XtClass(XtParent(w)) != htmlWidgetClass)
		return;
	/* If motif is defined, we don't want to process this button press
	 * if it is on a gadget */
	on_gadget = (_XmInputInGadget((Widget)hw,
				event->xbutton.x, event->xbutton.y) != False);
	if (on_gadget)
		return;
	if (event->type != ButtonRelease)
		return; 

	assert(eptr != NULL);

	if (!( (eptr->type == E_IMAGE) && eptr->pic_data->usemap ) ) {
		if (eptr->anchor_tag_ptr->anc_href == NULL && eptr->type!=E_INPUT_IMAGE)
			return;
	}
		   /* Save the anchor text, replace newlines with * spaces. */
	tptr = ParseTextToString(
		hw->html.select_start, hw->html.select_end,
		hw->html.sel_start_pos, hw->html.sel_end_pos,
		hw->html.cur_font->max_bounds.width,
		hw->html.margin_width);
	ptr = tptr;
	while ((ptr != NULL)&&(*ptr != '\0')) {
		if (*ptr == '\n')
			*ptr = ' ';
		ptr++;
	}
	UnsetAnchor(hw);	/* Clear the activated anchor */
#ifdef EXTRA_FLUSH
	XFlush(XtDisplay(hw));
#endif

/* An INPUT TYPE=image in Form */
	if((eptr->type == E_INPUT_IMAGE) && eptr->fptr) {      
		InputImageSubmitForm(eptr->fptr, event, hw);
		return;
	} 
/* Send the selection location along with the HRef
 * for images. Allows you to point at a location on a map and have
 * the server send you the related document.
 */
	if (eptr->type == E_IMAGE && eptr->pic_data->image_data && 
	    eptr->pic_data->ismap) {
 		buf = (char *) malloc(strlen(eptr->anchor_tag_ptr->anc_href) + 256);
    		sprintf(buf, "%s?%d,%d", eptr->anchor_tag_ptr->anc_href,
			event->xbutton.x + hw->html.scroll_x - eptr->x,
			event->xbutton.y + hw->html.scroll_y - eptr->y);
       	} else if (eptr->type == E_IMAGE && eptr->pic_data->usemap) {
		char * href;

		if( !MapAreaFound(hw, eptr, event->xbutton.x, event->xbutton.y, &href) ) {
			return;
		}
		buf = strdup(href);
	} else {
		buf = strdup(eptr->anchor_tag_ptr->anc_href);
	}
/* XXX: should call a media dependent function that decides how to munge the
 * HRef.  For example mpeg data will want to know on what frame the event occured.
 *
 * cddata.href = *(eptr->eventf)(eptr, event);
 */
	cbdata.event = event;
	cbdata.element_id = eptr->ele_id;
	cbdata.href = buf;
	cbdata.text = tptr;
	cbdata.title = eptr->anchor_tag_ptr->anc_title;
	cbdata.target = eptr->anchor_tag_ptr->anc_target;
	XtCallCallbackList ((Widget)hw, hw->html.anchor_callback,
			(XtPointer)&cbdata);
       	if (buf) free(buf);
       	if (tptr != NULL) free(tptr);
}

/* SetValues is called when XtSetValues is used to change resources in this
 * widget.
 */
static Boolean SetValues( HTMLWidget current, HTMLWidget request, HTMLWidget nw)
{
	/*	Make sure the underline numbers are within bounds.
	 */
	if (request->html.num_anchor_underlines < 0)
		nw->html.num_anchor_underlines = 0;
	if (request->html.num_anchor_underlines > MAX_UNDERLINES)
		nw->html.num_anchor_underlines = MAX_UNDERLINES;
	if (request->html.num_visitedAnchor_underlines < 0)
		nw->html.num_visitedAnchor_underlines = 0;
	if (request->html.num_visitedAnchor_underlines > MAX_UNDERLINES)
		nw->html.num_visitedAnchor_underlines = MAX_UNDERLINES;

	if ((request->html.cur_font != current->html.cur_font)||
	         (request->html.def_res.fg_alink != current->html.def_res.fg_alink)||
	         (request->html.def_res.fg_link != current->html.def_res.fg_link)||
	         (request->html.def_res.fg_vlink != current->html.def_res.fg_vlink)||
	         (request->html.dashed_anchor_lines != current->html.dashed_anchor_lines)||
	         (request->html.dashed_visitedAnchor_lines != current->html.dashed_visitedAnchor_lines)||
	         (request->html.num_anchor_underlines != current->html.num_anchor_underlines)||
	         (request->html.num_visitedAnchor_underlines != current->html.num_visitedAnchor_underlines))
	{
		nw->html.max_pre_width = 0;
		ReformatWindow(nw,True);
		ScrollWidgets(nw);
		ViewClearAndRefresh(nw);
	}

	/*
	 * vertical space has been changed
	 */
	if(request->html.percent_vert_space != current->html.percent_vert_space) {
		ReformatWindow(nw,True);
		ScrollWidgets(nw);
		ViewClearAndRefresh(nw);
	}
	return(False);
}

static Boolean ConvertSelection( Widget w,
	Atom *selection, Atom *target, Atom *type,
	caddr_t *value, unsigned long *length, int *format)
{
	Display *d = XtDisplay(w);
	HTMLWidget hw = (HTMLWidget)w;
	char *text;

	if (hw->html.select_start == NULL)
		return False;

	if (*target == XA_TARGETS(d)) {
		Atom *targetP;
		Atom *std_targets;
		unsigned long std_length;
		XmuConvertStandardSelection( w, hw->html.selection_time,
			selection, target, type, (caddr_t*)&std_targets,
			&std_length, format);

		*length = std_length + 5;
		*value = (caddr_t)XtMalloc(sizeof(Atom)*(*length));
		targetP = *(Atom**)value;
		*targetP++ = XA_STRING;
		*targetP++ = XA_TEXT(d);
		*targetP++ = XA_COMPOUND_TEXT(d);
		*targetP++ = XA_LENGTH(d);
		*targetP++ = XA_LIST_LENGTH(d);

		memcpy((char*)targetP, (char*)std_targets, 
			sizeof(Atom)*std_length);
		XtFree((char*)std_targets);
		*type = XA_ATOM;
		*format = 32;
		return True;
	}

	if (*target == XA_STRING || *target == XA_TEXT(d) ||
		*target == XA_COMPOUND_TEXT(d))
	{
		if (*target == XA_COMPOUND_TEXT(d)) {
			*type = *target;
		} else {
			*type = XA_STRING;
		}
		text = ParseTextToString(
			hw->html.select_start, hw->html.select_end,
			hw->html.sel_start_pos, hw->html.sel_end_pos,
			hw->html.cur_font->max_bounds.width,
			hw->html.margin_width);
		*value = text;
		*length = strlen(*value);
		*format = 8;
		return True;
	}

	if (*target == XA_LIST_LENGTH(d)) {
		*value = XtMalloc(4);
		if (sizeof(long) == 4) {
			*(long*)*value = 1;
		} else {
			long temp = 1;
			memcpy( (char*)*value, ((char*)&temp)+sizeof(long)-4, 4);
		}
		*type = XA_INTEGER;
		*length = 1;
		*format = 32;
		return True;
	}

	if (*target == XA_LENGTH(d)) {
		text = ParseTextToString(
			hw->html.select_start, hw->html.select_end,
			hw->html.sel_start_pos, hw->html.sel_end_pos,
			hw->html.cur_font->max_bounds.width,
			hw->html.margin_width);
		*value = XtMalloc(4);
		if (sizeof(long) == 4) {
			*(long*)*value = strlen(text);
		} else {
			long temp = strlen(text);
			memcpy( (char*)*value, ((char*)&temp)+sizeof(long)-4, 4);
		}
		free(text);
		*type = XA_INTEGER;
		*length = 1;
		*format = 32;
		return True;
	}

	if (XmuConvertStandardSelection(w, hw->html.selection_time, selection,
				    target, type, value, length, format))
		return True;
	return False;
}

static void LoseSelection( Widget w, Atom * selection)
{
	HTMLWidget hw = (HTMLWidget)w;

	ClearSelection(hw);
}

static void SelectionDone( Widget w, Atom * selection, Atom * target)
{
	/* empty proc so Intrinsics know we want to keep storage */
}


/******************************* PUBLIC FUNCTIONS *************************/
/*
 * Convenience function to return the text of the HTML document as a plain
 * ascii text string. This function allocates memory for the returned string,
 * that it is up to the user to free.
 * Extra option flags "pretty" text to be returned.
 * when pretty is two or larger, Postscript is returned. The font used is
 * encoded in the pretty parameter:
 * pretty = 2: Times
 * pretty = 3: Helvetica
 * pretty = 4: New century schoolbook
 * pretty = 5: Lucida Bright
 */
char * HTMLGetText(Widget w, int pretty, char *url, char *time_str)
{
	HTMLWidget hw = (HTMLWidget)w;
	char *text;
	char *tptr, *buf;
	struct ele_rec *start;
	struct ele_rec *end;

	if (url && *url) {
		strcpy(CurrentURL,url);
	} else {
		sprintf(CurrentURL,"UNKNOWN");
	}
	text = NULL;
	start = hw->html.formatted_elements;
	end = start;
	while (end != NULL)
		end = end->next;

	if (pretty >= 2) {
		tptr = ParseTextToPSString(hw, start, end, 0, 0,
				hw->html.margin_width,
				pretty-2, url, time_str);
	} else if (pretty) {
		tptr = ParseTextToPrettyString(start, end, 0, 0,
				hw->html.cur_font->max_bounds.width,
				hw->html.margin_width);
	} else {
		tptr = ParseTextToString(start, end, 0, 0,
				hw->html.cur_font->max_bounds.width,
				hw->html.margin_width);
	}
	if (tptr != NULL) {
		if (text == NULL) {
			text = tptr;
		} else {
			buf = (char *)malloc(strlen(text) + strlen(tptr) + 1);
			strcpy(buf, text);
			strcat(buf, tptr);
			free(text);
			free(tptr);
			text = buf;
		}
	}
	return(text);
}

/* Convenience function to return the element id of the element
 * nearest to the x,y coordinates passed in.
 * If there is no element there, return the first element in the
 * line we are on.  If there we are on no line, either return the
 * beginning, or the end of the document.
 */
int HTMLPositionToId(Widget w, int x, int y)
{
	HTMLWidget hw = (HTMLWidget)w;
	int epos;
	struct ele_rec *eptr;

	eptr = LocateElement(hw, x, y, &epos);
	if (eptr == NULL) {
		x = x + hw->html.scroll_x;
		y = y + hw->html.scroll_y;
				/* trouver l'element le plus proche en y */
		eptr = hw->html.formatted_elements;
		while (eptr){
			if (eptr->y <= y) {
				eptr = eptr->next;
				continue;
			} 
			break;
		}
	}

	/* 0 means the very top of the document.  We put you there for
	 * unfound elements.
	 * We also special case for when the scrollbar is at the
	 * absolute top.
	 */
	if ((eptr == NULL)||(hw->html.scroll_y == 0)) 
		return(0);
	return(eptr->ele_id);
}

/* Convenience function to return the position of the element
 * based on the element id passed in.
 * Function returns 1 on success and fills in x,y pixel values.
 * If there is no such element, x=0, y=0 and -1 is returned.
 */
int HTMLIdToPosition(Widget w, int element_id, int *x, int *y)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;
	struct ele_rec *eptr;

	eptr = NULL;
	start = hw->html.formatted_elements;
	while (start != NULL) {
		if (start->ele_id == element_id) {
			eptr = start;
			break;
		}
		start = start->next;
	}

	if (eptr == NULL){
		*x = 0;
		*y = 0;
		return(-1);
	}
	*x = eptr->x;
	*y = eptr->y;
	return(1);
}

/*
 * Convenience function to position the element
 * based on the element id passed at the top of the viewing area.
 * A passed in id of 0 means goto the top.
 */
/* 
 * "correction" is either -1, 0, or 1. These values determine if we are
 *   to set the pointer a 1/2 page in the negative or positive direction...or
 *   simply leave it alone.
 * --SWP
 */ 

void HTMLGotoId(Widget w, int element_id, int correction)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;
	struct ele_rec *eptr;
	int newy;
	int val, size, inc, pageinc;

/* If we have no scrollbar, just return. */
	if (hw->html.use_vbar == False)
		return;
/* Find the element corrsponding to the id passed in. */
	eptr = NULL;
	start = hw->html.formatted_elements;
	while (start != NULL) {
		if (start->ele_id == element_id) {
			eptr = start;
			break;
		}
		start = start->next;
	}

/* No such element, do nothing. */
	if ((element_id != 0)&&(eptr == NULL))
		return;
	if (element_id == 0)
		newy = 0;
	else {
/*#################*/
                if (!correction) {    
                        newy = eptr->y - 2; 
                }                     
                else if (correction<0) { /*"up" a 1/2 page*/
                        newy = eptr->y - 2 - ((int)(hw->html.view_height)/2);
                }                     
                else { /*"down" a 1/2 page*/
                        newy = eptr->y - 2 + ((int)(hw->html.view_height)/2);
                }                     
        }   
	if (newy < 0)
		newy = 0;
	if (newy > (hw->html.doc_height - (int)hw->html.view_height))
		newy = hw->html.doc_height - (int)hw->html.view_height;
	if (newy < 0)
		newy = 0;
	XmScrollBarGetValues(hw->html.vbar, &val, &size, &inc, &pageinc);
	XmScrollBarSetValues(hw->html.vbar, newy, size, inc, pageinc, True);
	XmScrollBarGetValues(hw->html.hbar, &val, &size, &inc, &pageinc);
	XmScrollBarSetValues(hw->html.hbar, 0, size, inc, pageinc, True);
}

void HTMLGotoAnchor(Widget w, char * target_anchor)
{
	HTMLWidget hw = (HTMLWidget)w;
	int x, y;
	int newy;

	HTMLAnchorToPosition(w, target_anchor, &x, &y);
        newy = y - 2;
        if (newy < 0)
                newy = 0;
        if (newy > (hw->html.doc_height - (int)hw->html.view_height))
                newy = hw->html.doc_height - (int)hw->html.view_height;
        if (newy < 0)
                newy = 0;
        hw->html.scroll_x = 0; 
        hw->html.scroll_y = newy;

        ConfigScrollBars(hw);          
        ScrollWidgets(hw);            
        ViewClearAndRefresh(hw);                /* Display the new text */
}

/*
 * Convenience function to return the position of the anchor
 * based on the anchor NAME passed.
 * Function returns 1 on success and fills in x,y pixel values.
 * If there is no such element, x=0, y=0 and -1 is returned.
 */
int HTMLAnchorToPosition(Widget w, char *name, int *x, int *y)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;
	struct ele_rec *eptr;

	eptr = NULL;
	start = hw->html.formatted_elements;
	while (start != NULL) {
		if ((start->anchor_tag_ptr->anc_name)&&
		    (strcmp(start->anchor_tag_ptr->anc_name, name) == 0)) {
			eptr = start;
			break;
		}
		start = start->next;
	}
	if (eptr == NULL) {
		*x = 0;
		*y = 0;
		return(-1);
	}
	*x = eptr->x;
	*y = eptr->y;
	return(1);
}

/*
 * Convenience function to return the element id of the anchor
 * based on the anchor NAME passed.
 * Function returns id on success.
 * If there is no such element, 0 is returned.
 */
int HTMLAnchorToId(Widget w, char *name)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;
	struct ele_rec *eptr;

/* Find the passed anchor name */
	eptr = NULL;
	start = hw->html.formatted_elements;
	while (start != NULL) {
		if (start->anchor_tag_ptr && start->anchor_tag_ptr->anc_name &&
		    (strcmp(start->anchor_tag_ptr->anc_name, name) == 0)) {
			eptr = start;
			break;
		}
		start = start->next;
	}

	if (eptr == NULL)
		return(0);
	return(eptr->ele_id);
}

/*
 * Convenience function to return the HREFs of all active anchors in the
 * document.
 * Function returns an array of strings and fills num_hrefs passed.
 * If there are no HREFs NULL returned.
 */
char ** HTMLGetHRefs(Widget w, int *num_hrefs)
{
	HTMLWidget hw = (HTMLWidget)w;
	int cnt;
	struct ele_rec *start;
	struct ele_rec *list;
	struct ele_rec *eptr;
	char **harray;

	list = NULL;
	cnt = 0;
	/*
	 * Construct a linked list of all the diffeent hrefs, counting
	 * then as we go.
	 */
	start = hw->html.formatted_elements;
	while (start != NULL) {
		/*
		 * This one has an HREF
		 */
		if (start->anchor_tag_ptr->anc_href != NULL) {
			/*
			 * Check to see if we already have
			 * this HREF in our list.
			 */
			eptr = list;
			while (eptr != NULL) {
				if(strcmp(eptr->anchor_tag_ptr->anc_href,start->anchor_tag_ptr->anc_href)==0)
					break;
				eptr = eptr->next;
			}
			/*
			 * This HREF is not, in our list.  Add it.
			 * That is, if it's not an internal reference.
			 */
			if (eptr == NULL) {
				eptr = (struct ele_rec *)
					malloc(sizeof(struct ele_rec));
				eptr->anchor_tag_ptr = start->anchor_tag_ptr;
				eptr->next = list;
				list = eptr;
				cnt++;
			}
		}
		start = start->next;
	}

	if (cnt == 0) {
		*num_hrefs = 0;
		return(NULL);
	} 
	*num_hrefs = cnt;
	harray = (char **)malloc(sizeof(char *) * cnt);
	eptr = list;
	cnt--;
	while (eptr != NULL) {
		harray[cnt] = (char *) malloc(strlen(eptr->anchor_tag_ptr->anc_href) + 1);
		strcpy(harray[cnt], eptr->anchor_tag_ptr->anc_href);
		start = eptr;
		eptr = eptr->next;
		free((char *)start);
		cnt--;
	}
	return(harray);
}

/*
 * Convenience function to return the SRCs of all images in the
 * document.
 * Function returns an array of strings and fills num_srcs passed.
 * If there are no SRCs NULL returned.
 */
char ** HTMLGetImageSrcs(Widget w, int *num_srcs)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct mark_up *mptr;
	int cnt;
	char *tptr;
	char **harray;

	cnt = 0;
	mptr = hw->html.html_objects;
	while (mptr != NULL) {
		if (mptr->type == M_IMAGE) {
			tptr = ParseMarkTag(mptr->start, MT_IMAGE, "SRC");
			if ((tptr != NULL)&&(*tptr != '\0')) {
				cnt++;
				free(tptr);
			}
		}
		mptr = mptr->next;
	}
	if (cnt == 0) {
		*num_srcs = 0;
		return(NULL);
	} 
	*num_srcs = cnt;
	harray = (char **)malloc(sizeof(char *) * cnt);
	mptr = hw->html.html_objects;
	cnt = 0;
	while (mptr != NULL) {
		if (mptr->type == M_IMAGE) {
			tptr = ParseMarkTag(mptr->start,MT_IMAGE,"SRC");
			if ((tptr != NULL)&&(*tptr != '\0')) {
				harray[cnt] = tptr;
				cnt++;
			}
		}
		mptr = mptr->next;
	}
	return(harray);
}

/*
 * Convenience function to return the link information
 * for all the <LINK> tags in the document.
 * Function returns an array of LinkInfo structures and fills
 * num_links passed.
 * If there are no LINKs NULL returned.
 */
LinkInfo * HTMLGetLinks(Widget w, int *num_links)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct mark_up *mptr;
	int cnt;
	char *tptr;
	LinkInfo *larray;

	cnt = 0;
	mptr = hw->html.html_objects;
	while (mptr != NULL) {
		if (mptr->type == M_BASE)
			cnt++;
		mptr = mptr->next;
	}
	if (cnt == 0){
		*num_links = 0;
		return(NULL);
	}
	*num_links = cnt;
	larray = (LinkInfo *)malloc(sizeof(LinkInfo) * cnt);
	mptr = hw->html.html_objects;
	cnt = 0;
	while (mptr != NULL) {
		if (mptr->type == M_BASE) {
			tptr = ParseMarkTag(mptr->start, MT_BASE, "HREF");
			larray[cnt].href = tptr;
			cnt++;
		}
		mptr = mptr->next;
	}
	return(larray);
}

/* Convenience function to redraw all active anchors in the document.
 * Can also pass a new predicate function to check visited
 * anchors.  If NULL passed for function, uses default predicate function.
 */
void HTMLRetestAnchors(Widget w, visitTestProc testFunc, char * base_url)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;

	if (testFunc == NULL)
		testFunc = (visitTestProc)hw->html.previously_visited_test;
	/*
	 * Search all elements
	 */
	start = hw->html.formatted_elements;
	while (start != NULL) {
		if (start->anchor_tag_ptr->anc_href == NULL) {
			start = start->next;
			continue;
		}
		if (testFunc != NULL) {
			if ((*testFunc)((Widget)hw, start->anchor_tag_ptr->anc_href, base_url)) {
			    start->fg = hw->html.cur_res.fg_vlink;
			    start->underline_number =
				hw->html.num_visitedAnchor_underlines;
			    start->dashed_underline =
				hw->html.dashed_visitedAnchor_lines;
			} else {
			    start->fg = hw->html.cur_res.fg_link;
			    start->underline_number =
				hw->html.num_anchor_underlines;
			    start->dashed_underline =
				hw->html.dashed_anchor_lines;
			}
		} else {
			start->fg = hw->html.cur_res.fg_link;
			start->underline_number =
				hw->html.num_anchor_underlines;
			start->dashed_underline =
				hw->html.dashed_anchor_lines;
		}
		/*
		 * Since the element may have changed, redraw it
		 */
		switch(start->type) {
		case E_TEXT:
			TextRefresh(hw, start, 0, (start->edata_len - 2));
			break;
		case E_IMAGE:
			ImageRefresh(hw, start);
			break;
		case E_BULLET:
			BulletRefresh(hw, start);
			break;
		case E_LINEFEED:
			LinefeedRefresh(hw, start);
			break;
		case E_CR:
		case E_ANCHOR:
		case E_WIDGET:
		case E_HRULE:
		case E_TABLE:
		case E_CELL_TABLE:
		case E_OBJECT:
		case E_APPLET:
		case E_INPUT_IMAGE:
			break;
		}
		start = start->next;
	}
}

void HTMLClearSelection(Widget w)
{
	LoseSelection (w, NULL);
}

/*
 * Set the current selection based on the ElementRefs passed in.
 * Both refs must be valid.
 */
void HTMLSetSelection(Widget w, ElementRef *start, ElementRef *end)
{
	HTMLWidget hw = (HTMLWidget)w;
	int found;
	struct ele_rec *eptr;
	struct ele_rec *e_start=NULL;
	struct ele_rec *e_end=NULL;
	int start_pos=0, end_pos=0;
	Atom *atoms;
	int i, buffer;
	char *text;
	char *params[2];

	/*
	 * If the starting position is not valid, fail the selection
	 */
	if ((start->id > 0)&&(start->pos >= 0)) {
		found = 0;
		eptr = hw->html.formatted_elements;

		while (eptr != NULL) {
			if (eptr->ele_id == start->id) {
				e_start = eptr;
				start_pos = start->pos;
				found = 1;
				break;
			}
			eptr = eptr->next;
		}
		if (!found)
			return;
	}

	/*
	 * If the ending position is not valid, fail the selection
	 */
	if ((end->id > 0)&&(end->pos >= 0)) {
		found = 0;
		eptr = hw->html.formatted_elements;

		while (eptr != NULL) {
			if (eptr->ele_id == end->id) {
				e_end = eptr;
				end_pos = end->pos;
				found = 1;
				break;
			}
			eptr = eptr->next;
		}
		if (!found)
			return;
	}

	LoseSelection (w, NULL);

	/*
	 * We expect the ElementRefs came from HTMLSearchText, so we know
	 * that the end_pos is one past what we want to select.
	 */
	end_pos = end_pos - 1;

	/*
	 * Sanify the position data
	 */
	if ((start_pos > 0)&&(start_pos >= e_start->edata_len - 1))
		start_pos = e_start->edata_len - 2;
	if ((end_pos > 0)&&(end_pos >= e_end->edata_len - 1))
		end_pos = e_end->edata_len - 2;

	hw->html.select_start = e_start;
	hw->html.sel_start_pos = start_pos;
	hw->html.select_end = e_end;
	hw->html.sel_end_pos = end_pos;
	SetSelection(hw);
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = 0;
	hw->html.new_end_pos = 0;

	/*
	 * Do all the gunk from the end of the ExtendEnd function
	 */
	params[0] = "PRIMARY";
	params[1] = "CUT_BUFFER0";
	atoms = (Atom *)malloc(2 * sizeof(Atom));
	if (atoms == NULL) {
		fprintf(stderr, "Out of memory\n");
		assert(0);
	}
	XmuInternStrings(XtDisplay((Widget)hw), params, 2, atoms);
	hw->html.selection_time = CurrentTime;
	for (i=0; i< 2; i++) {
		switch (atoms[i]) {
		case XA_CUT_BUFFER0: buffer = 0; break;
		case XA_CUT_BUFFER1: buffer = 1; break;
		case XA_CUT_BUFFER2: buffer = 2; break;
		case XA_CUT_BUFFER3: buffer = 3; break;
		case XA_CUT_BUFFER4: buffer = 4; break;
		case XA_CUT_BUFFER5: buffer = 5; break;
		case XA_CUT_BUFFER6: buffer = 6; break;
		case XA_CUT_BUFFER7: buffer = 7; break;
		default: buffer = -1; break;
		}
		if (buffer >= 0) {
			text = ParseTextToString(
				hw->html.select_start,
				hw->html.select_end,
				hw->html.sel_start_pos,
				hw->html.sel_end_pos,
				hw->html.cur_font->max_bounds.width,
				hw->html.margin_width);
			XStoreBuffer(XtDisplay((Widget)hw),
				text, strlen(text), buffer);
			free(text);
		} else {
			XtOwnSelection((Widget)hw, atoms[i], CurrentTime,
				       (XtConvertSelectionProc )ConvertSelection,
				       (XtLoseSelectionProc )LoseSelection,
				       (XtSelectionDoneProc )SelectionDone);
		}
	}
	free((char *)atoms);
}

/*
 * Convenience function to return the text of the HTML document as a single
 * white space separated string, with pointers to the various start and
 * end points of selections.
 * This function allocates memory for the returned string, that it is up
 * to the user to free.
 */
char * HTMLGetTextAndSelection(Widget w,char **startp,char **endp,char **insertp)
{
	HTMLWidget hw = (HTMLWidget)w;
	int length;
	char *text;
	char *tptr;
	struct ele_rec *eptr;
	struct ele_rec *sel_start;
	struct ele_rec *sel_end;
	struct ele_rec *insert_start;
	int start_pos, end_pos, insert_pos;

	if (SwapElements(hw->html.select_start, hw->html.select_end,
		hw->html.sel_start_pos, hw->html.sel_end_pos)) {
		sel_end = hw->html.select_start;
		end_pos = hw->html.sel_start_pos;
		sel_start = hw->html.select_end;
		start_pos = hw->html.sel_end_pos;
	} else {
		sel_start = hw->html.select_start;
		start_pos = hw->html.sel_start_pos;
		sel_end = hw->html.select_end;
		end_pos = hw->html.sel_end_pos;
	}

	insert_start = hw->html.new_start;
	insert_pos = hw->html.new_start_pos;
	*startp = NULL;
	*endp = NULL;
	*insertp = NULL;

	length = 0;
	eptr = hw->html.formatted_elements;
	while (eptr != NULL) {
		if (eptr->type == E_TEXT) {
			length = length + eptr->edata_len - 1;
		} else if (eptr->type == E_LINEFEED) {
			length = length + 1;
		}
		eptr = eptr->next;
	}
	text = (char *)malloc(length + 1);
	if (text == NULL) {
		fprintf(stderr, "Out of memory\n");
		assert(0);
	}
	strcpy(text, "");
	tptr = text;

	eptr = hw->html.formatted_elements;
	while (eptr != NULL) {
		if (eptr->type == E_TEXT) {
			if (eptr == sel_start)
				*startp = (char *)(tptr + start_pos);
			if (eptr == sel_end)
				*endp = (char *)(tptr + end_pos);
			if (eptr == insert_start)
				*insertp = (char *)(tptr + insert_pos);
			strcat(text, (char *)eptr->edata);
			tptr = tptr + eptr->edata_len - 1;
		} else if (eptr->type == E_LINEFEED) {
			if (eptr == sel_start)
				*startp = tptr;
			if (eptr == sel_end)
				*endp = tptr;
			if (eptr == insert_start)
				*insertp = tptr;
			strcat(text, " ");
			tptr = tptr + 1;
		}
		eptr = eptr->next;
	}
	return(text);
}

static void ResetBody(HTMLWidget hw) 
{
	unsigned long pixels[30];
	int np = 0;

/* release old color and reset to default */
        if (hw->html.def_res.bgcolor != hw->html.cur_res.bgcolor) {
		pixels[np] = hw->html.cur_res.bgcolor; np++;
		hw->html.view->core.background_pixel = hw->html.def_res.bgcolor;
		XSetWindowBackground(XtDisplay(hw),XtWindow(hw->html.view),
			hw->html.def_res.bgcolor);
	}
	if (hw->html.def_res.have_bgima != hw->html.cur_res.have_bgima) {
		hw->html.view->core.background_pixmap = XtUnspecifiedPixmap;
		XSetWindowBackground(XtDisplay(hw),XtWindow(hw->html.view),
                        hw->html.def_res.bgcolor);
	}
/* we SET THE background in the drawing area . reset it
 * we don't want to scroll the background (like netscape or NCSA mosaic)
 * if it is a backgorund pixel. Else for a background image we MUST scroll
 * because of XCopyArea...
 * A background is FIXED like the screen background 
 * RESET THE BACKGROUND OF THE DRAWING AREA ONLY 
 * A background does not have clipmask.... 
 * A background image don't need to bee freed here. It is allocated by
 * application and free by her
 *       have_bgima = 0;         init with no bgimage 
 *       bgimamap = None;
 *       bgima_height = 0;
 *       bgima_width = 0;
 */
	XClearWindow(XtDisplay(hw), XtWindow(hw->html.view));
	hw->html.cur_res = hw->html.def_res;
	return;
}

/* Convenience function to set the makup list into the widget.
 * Forces a reformat.
 * If any pointer is passed in as NULL that text is unchanged,
 * Also pass an element ID to set the view area to that section of the new
 * text.  Finally pass an anchor NAME to set position of the new text
 * to that anchor.
 */
void HTMLSetHTMLmark(Widget w, struct mark_up *mlist, int element_id, 
	char *target_anchor, char * base_url)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *start;
	struct ele_rec *eptr;
	int newy;

	hw->html.base_url = base_url;
	if (mlist == NULL)
		return;
	ResetBody(hw); /*init body stuff. in case they have no body tag*/

	HideWidgets(hw); 		/* Hide any old widgets */
	HTMLFreeWidgetInfo(hw->html.widget_list);
	hw->html.widget_list = NULL;
	hw->html.form_list = NULL;
	if( hw->html.frame_type == FRAMESET_TYPE){
		_XmHTMLDestroyFrames(hw);
	}

/* see: FreeHtmlTextInfo():
 * when we free a node, we free the mark_up list of this node.
 * Thus we must not free the hw->html.html_objects here.
 */

	hw->html.html_objects = mlist;
				/* Reformat the new text */
	hw->html.max_pre_width = 0;
	hw->html.formatted_elements = NULL;
	ReformatWindow(hw,False); /* here we rescan all tag and make all */

	/* If a target anchor is passed, override the element id
	 * with the id of that anchor.
	 */
	if (target_anchor != NULL) {
		int id;

		id = HTMLAnchorToId(w, target_anchor);
		if (id != 0)
			element_id = id;
	}

	/* Position text at id specified, or at top if no position
	 * specified.
	 * Find the element corrsponding to the id passed in.
	 */
	eptr = NULL;
	if (element_id != 0) {
		start = hw->html.formatted_elements;
		while (start != NULL) {
			if (start->ele_id == element_id) {
				eptr = start;
				break;
			}
			start = start->next;
		}
	}
	if (eptr == NULL)
		newy = 0;
	else
		newy = eptr->y - 2;
	if (newy < 0)
		newy = 0;
	if (newy > (hw->html.doc_height - (int)hw->html.view_height))
		newy = hw->html.doc_height - (int)hw->html.view_height;
	if (newy < 0)
		newy = 0;
	hw->html.scroll_x = 0;
	hw->html.scroll_y = newy;
#ifdef HTMLTRACE
	fprintf (stderr, "calling in HTMLSetText\n");
#endif
	ConfigScrollBars(hw);
	ScrollWidgets(hw);
	ViewClearAndRefresh(hw); 		/* Display the new text */

	hw->html.select_start = NULL;	 /* Clear any previous selection */
	hw->html.select_end = NULL;
	hw->html.sel_start_pos = 0;
	hw->html.sel_end_pos = 0;
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
	hw->html.new_start_pos = 0;
	hw->html.new_end_pos = 0;
	hw->html.active_anchor = NULL;
}

void HTMLUnsetFrameSet (Widget w)
{
	HTMLWidget hw = (HTMLWidget) w;

	assert(hw->html.frame_type == FRAMESET_TYPE);

	_XmHTMLDestroyFrames(hw);
        hw->html.frames = NULL;
	hw->html.nframe = 0;
	hw->html.frame_type = NOTFRAME_TYPE;
	hw->html.html_objects = NULL;
}

/* Convenience function to create container for frame.
 * We change the type of current HTMLwidget to be a frameset, and we create
 * enought HTMLwidget for nframes.
 *
 * Returns:
 *	tset_ret	Allocated vector of HTMLWidget
 */
void HTMLSetFrameSet(Widget w, struct mark_up *mlist,
	char * base_url, int nframes, TopFrameSetInfo *tset_info,
	Widget ** thw_ret)
{
	HTMLWidget hw = (HTMLWidget)w;
	HTMLWidget *thw;
	int i, vx, vy;

	hw->html.base_url = base_url;
	if (mlist == NULL)
		assert(0);
	ResetBody(hw); /*init body stuff. in case they have no body tag*/

	HideWidgets(hw); 		/* Hide any old widgets */
	HTMLFreeWidgetInfo(hw->html.widget_list);
	hw->html.widget_list = NULL;
	hw->html.form_list = NULL;

	if( hw->html.frame_type == FRAMESET_TYPE)
		assert(0);	/* call HTMLUnsetFrameSet first */

	hw->html.frame_type = FRAMESET_TYPE;
	thw = (HTMLWidget *) calloc(nframes, sizeof(HTMLWidget));
        hw->html.frames = thw;
	hw->html.nframe = nframes;
	hw->html.topframeset_info = tset_info;
	hw->html.html_objects = mlist;
	hw->html.formatted_elements = NULL;

	hw->html.use_vbar = False;
	hw->html.use_hbar = False;
	hw->html.view_height = hw->core.height;
	hw->html.view_width = hw->core.width;
	XtUnmapWidget(hw->html.vbar);
	XtUnmapWidget(hw->html.hbar);

/* Move and size the viewing area */
        vx = hw->manager.shadow_thickness;
        vy = hw->manager.shadow_thickness;
	XtMoveWidget(hw->html.view, vx, vy);
	XtResizeWidget(hw->html.view, hw->html.view_width, hw->html.view_height,
                hw->html.view->core.border_width);

	_XmHTMLFrameAdjustConstraints(hw, tset_info);

/* and now create all frames */
	for(i = 0; i < nframes; i++) {
                thw[i] = _XmHTMLFrameCreate(hw, &tset_info->frames[i]);
        }
/* erase a few glitches by calling adjustConstraints again */
        _XmHTMLReconfigureFrames(hw,tset_info);

        _XmHTMLMapFrames(hw);			 /* and now map them to screen */
	*thw_ret = (Widget *)thw;

}

/* Allows us to jump to the bottom of a document (or very close).  */
int HTMLLastId(Widget w)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *eptr;
	int Id;

	if (!w)
		return(0);
	eptr=hw->html.formatted_elements;
	Id=eptr->ele_id;
	while (eptr->next != NULL) {
		Id=eptr->ele_id;
		eptr=eptr->next;
	}
	return(Id);
}

/* News hack of searching function for HTML widget...only looks for an edata
 *   of ">>>" as it will be by itself because the one we are looking for
 *   will be enclosed in a <b></b>. This will have to be rewritten when we
 *   start using the new parser! --SWP                 
 */                   
int HTMLSearchNews(Widget w, ElementRef *m_start, ElementRef *m_end)
{                     
      HTMLWidget hw = (HTMLWidget)w;
      struct ele_rec *eptr;
                      
      /*                             
       * If bad parameters are passed, just fail the search
       */
      if ((m_start == NULL)||(m_end == NULL)) {
              return(-1);
      }

      eptr = hw->html.formatted_elements;

      while (eptr != NULL) {
              if (eptr->type==E_TEXT) {
                      if (eptr->edata && !strcmp(eptr->edata,">>>")) {
                              m_start->id = eptr->ele_id;
                              m_start->pos = 0;
                              m_end->id = eptr->ele_id;
                              m_end->pos = 3;

                              return(1);
                      }
              }
              eptr=eptr->next;
      }

      return(-1);
}

#define TOLOWER(x)      (tolower(x))

/* Convenience function to search the text of the HTML document as a single
 * white space separated string. Linefeeds are converted into spaces.
 *
 * Takes a pattern, pointers to the start and end blocks to store the
 * start and end of the match into.  Start is also used as the location to
 * start the search from for incremental searching.  If start is an invalid
 * position (id = 0).  Default start is the beginning of the document for
 * forward searching, and the end of the document for backwards searching.
 * The backward and caseless parameters I hope are self-explanatory.
 *
 * returns 1 on success
 *      (and the start and end positions of the match).
 * returns -1 otherwise (and start and end are unchanged).
 */
int HTMLSearchText(Widget w, char *pattern,
		ElementRef *m_start, ElementRef *m_end,
		int backward, int caseless)
{
	HTMLWidget hw = (HTMLWidget)w;
	int found, equal;
	char *match;
	char *tptr;
	char *mptr;
	char cval;
	struct ele_rec *eptr;
	int s_pos;
	struct ele_rec *s_eptr=NULL;
	ElementRef s_ref, e_ref;
	ElementRef *start, *end;

/* If bad parameters are passed, just fail the search
 */
	if ((pattern == NULL)||(*pattern == '\0')||
		(m_start == NULL)||(m_end == NULL)) {
		return(-1);
	}

/* If we are caseless, make a lower case copy of the pattern to
 * match to use in compares.
 * remember to free this before returning
 */
	if (caseless) {
		match = (char *)malloc(strlen(pattern) + 1);
		tptr = pattern;
		mptr = match;
		while (*tptr != '\0') {
			*mptr = (char)TOLOWER((int)*tptr);
			mptr++;
			tptr++;
		}
		*mptr = '\0';
	} else {
		match = pattern;
	}

/* Slimy coding.  I later decided I didn't want to change start and
 * end if the search failed.  Rather than changing all the code,
 * I just copy it into locals here, and copy it out again if a match is found.
 */
	start = &s_ref;
	end = &e_ref;
	start->id = m_start->id;
	start->pos = m_start->pos;
	end->id = m_end->id;
	end->pos = m_end->pos;

	/*
	 * Find the user specified start position.
	 */
	if (start->id > 0) {
		found = 0;
		eptr = hw->html.formatted_elements;

		while (eptr != NULL) {
			if (eptr->ele_id == start->id) {
				s_eptr = eptr;
				found = 1;
				break;
			}
			eptr = eptr->next;
		}
		/*
		 * Bad start position, fail them out.
		 */
		if (!found) {
			if (caseless)
				free(match);
			return(-1);
		}
		/*
		 * Sanify the start position
		 */
		s_pos = start->pos;
		if (s_pos >= s_eptr->edata_len - 1)
			s_pos = s_eptr->edata_len - 2;
		if (s_pos < 0)
			s_pos = 0;
	} else {
		/*
		 * Default search starts at end for backward, and
		 * beginning for forwards.
		 */
		if (backward) {
			s_eptr = hw->html.formatted_elements;
			while (s_eptr->next != NULL)
				s_eptr = s_eptr->next;
			s_pos = s_eptr->edata_len - 2;
		} else {
			s_eptr = hw->html.formatted_elements;
			s_pos = 0;
		}
	}
	if (backward) {
		char *mend;

		/*
		 * Save the end of match here for easy end to start searching
		 */
		mend = match;
		while (*mend != '\0')
			mend++;
		if (mend > match)
			mend--;
		found = 0;
		equal = 0;
		mptr = mend;

		if (s_eptr != NULL) {
			eptr = s_eptr;
		} else {
			eptr = hw->html.formatted_elements;
			while (eptr->next != NULL)
				eptr = eptr->next;
		}

		while (eptr != NULL) {
			if (eptr->type == E_TEXT) {
			    tptr = (char *)(eptr->edata + eptr->edata_len - 2);
			    if (eptr == s_eptr) {
				tptr = (char *)(eptr->edata + s_pos);
			    }
			    while (tptr >= eptr->edata) {
				if (equal) {
					if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
					} else {
						cval = *tptr;
					}
					while ((mptr >= match)&&
						(tptr >= eptr->edata)&&
						(cval == *mptr)) {
						tptr--;
						mptr--;
					    if (tptr >= eptr->edata) {
						if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
						} else {
							cval = *tptr;
						}
					    }
					}
					if (mptr < match) {
						found = 1;
						start->id = eptr->ele_id;
						start->pos = (int)
						    (tptr - eptr->edata + 1);
						break;
					} else if (tptr < eptr->edata) {
						break;
					} else {
						equal = 0;
					}
				} else {
					mptr = mend;
					if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
					} else {
						cval = *tptr;
					}
					while ((tptr >= eptr->edata)&&
						(cval != *mptr)) {
						tptr--;
					    if (tptr >= eptr->edata) {
						if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
						} else {
							cval = *tptr;
						}
					    }
					}
					if ((tptr >= eptr->edata)&&
						(cval == *mptr)) {
						equal = 1;
						end->id = eptr->ele_id;
						end->pos = (int)
						    (tptr - eptr->edata + 1);
					}
				}
			    }
			}
			/*
			 * Linefeeds match to single space characters.
			 */
			else if (eptr->type == E_LINEFEED) {
				if (equal) {
					if (*mptr == ' ') {
						mptr--;
						if (mptr < match) {
							found = 1;
							start->id =eptr->ele_id;
							start->pos = 0;
						}
					} else {
						equal = 0;
					}
				} else {
					mptr = mend;
					if (*mptr == ' ') {
						equal = 1;
						end->id = eptr->ele_id;
						end->pos = 0;
						mptr--;
						if (mptr < match) {
							found = 1;
							start->id =eptr->ele_id;
							start->pos = 0;
						}
					}
				}
			}
			if (found)
				break;
			eptr = eptr->prev;
		}
	} else /* forward */ {
		found = 0;
		equal = 0;
		mptr = match;

		if (s_eptr != NULL) {
			eptr = s_eptr;
		} else {
			eptr = hw->html.formatted_elements;
		}

		while (eptr != NULL) {
			if (eptr->type == E_TEXT) {
			    tptr = eptr->edata;
			    if (eptr == s_eptr)
				tptr = (char *)(tptr + s_pos);
			    while (*tptr ) {
				if (equal) {
					if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
					} else {
						cval = *tptr;
					}
					while ( *mptr && (cval == *mptr)) {
						tptr++;
						mptr++;
						if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
						} else {
							cval = *tptr;
						}
					}
					if (*mptr == '\0') {
						found = 1;
						end->id = eptr->ele_id;
						end->pos = (int)
							(tptr - eptr->edata);
						break;
					} else if (*tptr == '\0') {
						break;
					} else {
						equal = 0;
					}
				} else {
					mptr = match;
					if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
					} else {
						cval = *tptr;
					}
					while ( *tptr && (cval != *mptr)) {
						tptr++;
						if (caseless) {
						cval =(char)TOLOWER((int)*tptr);
						} else {
							cval = *tptr;
						}
					}
					if (cval == *mptr) {
						equal = 1;
						start->id = eptr->ele_id;
						start->pos = (int)
							(tptr - eptr->edata);
					}
				}
			    }
			} else if (eptr->type == E_LINEFEED) {
				if (equal) {
					if (*mptr == ' ') {
						mptr++;
						if (*mptr == '\0') {
							found = 1;
							end->id = eptr->ele_id;
							end->pos = 0;
						}
					} else {
						equal = 0;
					}
				} else {
					mptr = match;
					if (*mptr == ' ') {
						equal = 1;
						start->id = eptr->ele_id;
						start->pos = 0;
						mptr++;
						if (*mptr == '\0') {
							found = 1;
							end->id = eptr->ele_id;
							end->pos = 0;
						}
					}
				}
			}
			if (found)
				break;
			eptr = eptr->next;
		}
	}
	if (found) {
		m_start->id = start->id;
		m_start->pos = start->pos;
		m_end->id = end->id;
		m_end->pos = end->pos;
	}
	if (caseless)
		free(match);
	if (found)
		return(1);
	return(-1);
}

void HTMLDrawBackgroundImage(HTMLWidget hw, int x, int y, int width, int height) 
{
	int tile_x_origin, tile_y_origin;
/*
 *	tile_x_origin = (x+hw->html.scroll_x) % hw->html.cur_res.bgima_width;  
 *	tile_y_origin = (y+hw->html.scroll_y) % hw->html.cur_res.bgima_height;
 *	XSetTSOrigin(XtDisplay(hw), hw->html.bgimgGC, tile_x_origin,
 *		tile_y_origin);
*/

/* Winfried 8 Nov 2000 */
	tile_x_origin = hw->html.scroll_x % hw->html.cur_res.bgima_width;  
	tile_y_origin = hw->html.scroll_y % hw->html.cur_res.bgima_height;
	XSetTSOrigin(XtDisplay(hw), hw->html.bgimgGC, -tile_x_origin,
		-tile_y_origin);

	XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
                hw->html.bgimgGC, x, y, width, height);
}

 
static Boolean html_accept_focus(Widget w, Time *t)
{
	return True;
}
