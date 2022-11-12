/* Copyright G.Dauphin Sep 97 */

#include <stdlib.h>
#include <stdio.h>
/*#include <sys/varargs.h>  correction 1999-03-17 jolk@ap-pc513b.physik.uni-karlsruhe.de*/
#include <stdarg.h>
#include <assert.h>

#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#ifdef DEBUG_FRAME
#define _XmHTMLDebug(LEVEL,MSG) do {\
                { printf MSG;} \
         }while(0)
#else
#define _XmHTMLDebug(LEVEL,MSG)                         /* empty */
#endif

#define XmIsHTML(w)	XtIsSubclass(w, htmlWidgetClass)

static void adjustFrame(FrameChildInfo *parent, int *p_width, int *p_height);
 
Boolean parser_warnings = True;
void
#ifdef __STDC__
_XmHTMLWarning(Widget w, String module, int line, String routine,
        String fmt,...)
{
        va_list arg_list;
        char buf[1024];

        if(!parser_warnings)
                return;

        va_start(arg_list, fmt);

#else /* ! __STDC__ */
__XmHTMLWarning(w, module, line, routine, fmt, va_alist)
        Widget w;
        String module;
        String routine;
        String fmt;
        va_dcl
{
        va_list arg_list;
        char buf[1024];

        if(!parser_warnings)
                return;

        va_start(arg_list);

#endif /* __STDC__ */

        vsprintf(buf, fmt, arg_list);
        va_end(arg_list);
        if(w != NULL)
                fprintf(stderr, "Warning:\n    %s\n", buf);
        else
                fprintf(stderr, "Warning:    %s\n", buf);
        fprintf(stderr, "    (%s, %s, line %i)\n", module, routine, line);
}

/*##############################################*/
/* Come from XmHTML Widget :
 * Copyright (C) 1994-1997 by Ripley Software Development
 */

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/


/* usefull defines */
#define IS_FRAMESET(F) 		((F)->type == FRAMESET_TYPE)

#define IS_FRAME_SIZE_RELATIVE(F) \
				((F)->fai.any_size_type == FRAME_SIZE_RELATIVE)
#define IS_FRAME_SIZE_OPTIONAL(F) \
		((F)->fai.any_size_type == FRAME_SIZE_OPTIONAL)
#define IS_FRAME_SIZE_FIXED(F) \
		((F)->fai.any_size_type == FRAME_SIZE_FIXED)
#define IS_FRAMESET_LAYOUT_ROWS(F) \
		(IS_FRAMESET(F) && ((F)->fs_layout_type == FRAMESET_LAYOUT_ROWS))
#define IS_FRAMESET_LAYOUT_COLS(F) \
		(IS_FRAMESET(F) && ((F)->fs_layout_type == FRAMESET_LAYOUT_COLS))


/* length is :
	nn	: number of pixel
	nn%	: relative to lenght available
	n*	: option relative to remaining space
*/

static void adjustFramesetRows(FrameSetInfo *parent, int *p_width, int *p_height)
{
        FrameChildInfo *child = NULL ;
        int width, height ;
        int cum_fixed_size = 0, cum_rel_size = 0, cum_opt_size = 0 ;
        int nb_opt = 0, nb_rel =0, nb_fix =0;

/* Begin with fixed-sized children */
/* Then do relative-sized children */
/* Finally, end up with optional-sized children */
        cum_fixed_size = 0 ;	/* pixel */
        cum_rel_size = 0 ;	/* % */
        cum_opt_size = 0 ;	/* * */
/* IL FAUT AJUSTER LES POURCENT POUR QUE CA TIENNE... */

        for (child = parent->fs_fc_children ; child != NULL ; child = child->fai.any_fc_next_child) {
                if(IS_FRAME_SIZE_FIXED(child)) {
                        width = *p_width ;
                        height = child->fai.any_size_s ;
                        adjustFrame(child, &width, &height);
                        child->fai.box_width = width ;
                        child->fai.box_height = height ;
                        cum_fixed_size += height ;
			nb_fix++;
                } else if(IS_FRAME_SIZE_RELATIVE(child)) {

                        width = *p_width ;
                        height = child->fai.any_size_s * (*p_height) / 100 ;
                        adjustFrame(child, &width, &height);
                        child->fai.box_width = width ;
                        child->fai.box_height = height ;
                        cum_rel_size += height ;
			nb_rel++;
                } else if(IS_FRAME_SIZE_OPTIONAL(child)) {
/* count how many optional they are */
                        ++nb_opt;
			child->fai.box_height = 1 ;
			child->fai.box_width = *p_width;
		}
        }

/* tout l'espace n'est pas utilise */
	if ( cum_fixed_size + cum_rel_size < *p_height ) {
/* distribue le reste sur les espaces optionnel */
        	if(nb_opt > 0) {
                	int cum_size, remain_size, mean_opt_size ;

                	cum_size = cum_fixed_size + cum_rel_size ;
                	remain_size = *p_height - cum_size ;
                	if(remain_size <= nb_opt)
                        	remain_size = nb_opt ;
                	mean_opt_size = remain_size / nb_opt ;

                /* go adjust */
                	for(child = parent->fs_fc_children ; child != NULL ; child = child->fai.any_fc_next_child) {
                        	if(IS_FRAME_SIZE_OPTIONAL(child)) {
                                	width = *p_width ;
                                	height = mean_opt_size ;
                                	adjustFrame(child, &width, &height);
                                	child->fai.box_width = width ;
                                	child->fai.box_height = height ;
                        	}
                	}
        	} else if ( cum_rel_size >0 ) {
/* pas d'espace optionel, distribue sur les % */ /* go adjust */
			int to_add;

			to_add = (*p_height - cum_fixed_size - cum_rel_size) / nb_rel;
			for(child = parent->fs_fc_children; child != NULL; child = child->fai.any_fc_next_child) {
				if(IS_FRAME_SIZE_RELATIVE(child)){
					width = *p_width ;
					height = child->fai.box_height + to_add;
					adjustFrame(child, &width, &height);
					child->fai.box_width = width ;
					child->fai.box_height = height ;
				}
			}
		} else { /* distribue sur les pixels */
			int to_add;

			to_add = (*p_height - cum_fixed_size) / nb_fix;
			for(child = parent->fs_fc_children; child != NULL; child = child->fai.any_fc_next_child) { 

				width = *p_width ;
				height = child->fai.box_height + to_add;
				adjustFrame(child, &width, &height);
				child->fai.box_width = width ;
				child->fai.box_height = height ;
			}
		}
	} else if (cum_fixed_size + cum_rel_size > *p_height) { /* too much space is allocated */
				/* on reduit l'espace */
		if (cum_fixed_size <= *p_height ) { /* on reduit le % */
			int to_sub;

			to_sub =(*p_height - cum_fixed_size );
			for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) {
				if(IS_FRAME_SIZE_RELATIVE(child)){
					width = *p_width ;
					height = (child->fai.box_height * to_sub)/cum_rel_size;
					if ( height<1)
						height = 1;
					adjustFrame(child, &width, &height);
					child->fai.box_width = width ;
					child->fai.box_height = height ;
				}
			}
		} else { /* to much pixel */
			for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) { 
				width = *p_width;
				height = (child->fai.box_height* *p_height)/(cum_fixed_size+cum_rel_size);
				if ( height<1)
                                	height = 1;
				adjustFrame(child, &width, &height);
				child->fai.box_width = width ; 
                                child->fai.box_height = height ;
			}
		}
	}
}

static void adjustFramesetColumns(FrameSetInfo *parent, int *p_width, int *p_height)
{
        FrameChildInfo *child = NULL ;
        int width, height ;
        int cum_fixed_size = 0, cum_rel_size = 0, cum_opt_size = 0 ;
        int nb_opt = 0, nb_rel =0, nb_fix =0;

/* Begin with fixed-sized children */
/* Then do relative-sized children */
/* Finally, end up with optional-sized children */
        cum_fixed_size = 0 ;
        cum_rel_size = 0 ;
        cum_opt_size = 0 ;
        for(child = parent->fs_fc_children ; child != NULL ; child = child->fai.any_fc_next_child) {
                if(IS_FRAME_SIZE_FIXED(child)) {
                        width = child->fai.any_size_s ;
                        height = *p_height ;
                        adjustFrame(child, &width, &height);
                        child->fai.box_width = width ;
                        child->fai.box_height = height ;
                        cum_fixed_size += width ;
			nb_fix++;
                } else if(IS_FRAME_SIZE_RELATIVE(child)) {
                        width = child->fai.any_size_s * (*p_width) / 100 ;
                        height = *p_height ;
                        adjustFrame(child, &width, &height);
                        child->fai.box_width = width ;
                        child->fai.box_height = height ;
                        cum_rel_size += width ;
			nb_rel ++;
                } else if(IS_FRAME_SIZE_OPTIONAL(child)) {
                        ++nb_opt;
			child->fai.box_height = *p_height;
			child->fai.box_width = 1;
		}
	}

/* tout l'espace n'est pas utilise */  
        if ( cum_fixed_size + cum_rel_size < *p_width ) {

/* distribue le reste sur les espaces optionnel */
                if(nb_opt > 0) {       
                        int cum_size, remain_size, mean_opt_size ;
                                       
                        cum_size = cum_fixed_size + cum_rel_size ;
                        remain_size = *p_width - cum_size ;
                        if(remain_size <= nb_opt)
                                remain_size = nb_opt ;
                        mean_opt_size = remain_size / nb_opt ;
                                       
                /* go adjust */        
                        for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) {      
                                if(IS_FRAME_SIZE_OPTIONAL(child)) {
                                        width = mean_opt_size ;
                                        height = *p_height ;
                                        adjustFrame(child, &width, &height);
                                        child->fai.box_width = width ;
                                        child->fai.box_height = height ;
                                }      
                        }              
                } else if ( cum_rel_size >0 ) {
/* pas d'espace optionel, distribue sur les % */ /* go adjust */
                        int to_add;    
                                       
                        to_add = (*p_width - cum_fixed_size - cum_rel_size) / nb_rel;                                   
                        for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) {      
                                if(IS_FRAME_SIZE_RELATIVE(child)){
					height = *p_height;
					width = child->fai.box_width + to_add;
                                        adjustFrame(child, &width, &height);
                                        child->fai.box_width = width ;
                                        child->fai.box_height = height ;
                                }      
                        }              
                } else { /* distribue sur les pixels */
                        int to_add;    
                                       
                        to_add = (*p_width - cum_fixed_size) / nb_fix;
                        for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) {      
				height = *p_height;
				width = child->fai.box_width + to_add;
                                adjustFrame(child, &width, &height);
                                child->fai.box_width = width ;
                                child->fai.box_height = height ;
                        }              
                }                      
        } else if (cum_fixed_size + cum_rel_size > *p_width) { /* too much space
is allocated */                        
                                /* on reduit l'espace */
                if (cum_fixed_size <= *p_width ) { /* on reduit le % */
                        int to_sub;    
                                       
                        to_sub =(*p_width - cum_fixed_size );
                        for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) { 
                                if(IS_FRAME_SIZE_RELATIVE(child)){
                                        height = *p_height ;
                                        width = (child->fai.box_width * to_sub)/cum_rel_size;                       
                                        if ( width<1)
                                                width = 1;
                                        adjustFrame(child, &width, &height);
                                        child->fai.box_width = width ;
                                        child->fai.box_height = height ;
                                }      
                        }              
                } else { /* to much pixel */ 
                        for(child = parent->fs_fc_children ; child != NULL ;
child = child->fai.any_fc_next_child) {      
                                height = *p_height;
                                width = (child->fai.box_width* *p_width)/(cum_fixed_size+cum_rel_size);            
                                if ( width<1)
                                        width = 1;
                                adjustFrame(child, &width, &height);
                                child->fai.box_width = width ;
                                child->fai.box_height = height ;
                        }              
                }                      
        } 
}

 
static void adjustFrame(FrameChildInfo *parent, int *p_width, int *p_height)
{
        if(*p_width <= 0)
                *p_width = 1 ;
        if(*p_height <= 0)
                *p_height = 1 ;

        if(IS_FRAMESET(parent)) { /* do recursion if it is a frameset */
		FrameSetInfo *p = (FrameSetInfo *) parent;

                if(p->fs_layout_type == FRAMESET_LAYOUT_ROWS)
                        adjustFramesetRows(p, p_width, p_height);
                else if(p->fs_layout_type == FRAMESET_LAYOUT_COLS)
                        adjustFramesetColumns(p, p_width, p_height);
        }
}

static void locateFrame(FrameChildInfo *parent, int x, int y)
{
        parent->fai.box_x = x;
        parent->fai.box_y = y;

        if(IS_FRAMESET(parent)) { /* do recursion if it is a frameset */
                FrameChildInfo *frame ;
		FrameSetInfo *p = (FrameSetInfo *) parent;

                if(IS_FRAMESET_LAYOUT_ROWS(p)) {
                        for(frame = p->fs_fc_children ; frame != NULL ; frame = frame->fai.any_fc_next_child) {
                                locateFrame(frame, x, y);
                                y += frame->fai.box_height ;
                        }
                }

                if(IS_FRAMESET_LAYOUT_COLS(p)) {
                        for(frame = p->fs_fc_children ; frame != NULL ; frame = frame->fai.any_fc_next_child) {
                                locateFrame(frame, x, y);
                                x += frame->fai.box_width ;
                        }
                }
        }
}

void _XmHTMLFrameAdjustConstraints(
	HTMLWidget topw /* topframeset HTMLWidget */,
	TopFrameSetInfo * top_seti)
{
        int work_width, work_height;
	FrameSetInfo * ffsi;	/* first frameset info */

/* this uses the core dimensions */
        work_width = topw->core.width;
        work_height = topw->core.height;

/* adjust frames' dimensions */
	ffsi = top_seti->frameset_info;
        adjustFrame( (FrameChildInfo*)ffsi, &work_width, &work_height);

/* adjust frames' positions */
        locateFrame((FrameChildInfo*)ffsi, 0, 0);
}


/*****
* Description:  destroys the memory used by the framesets
* In:
*       set:            list of framesets to be destroyed
* Returns:
*       nothing
*****/
static void destroyFrameSets(FrameSetInfo *set)
{
        FrameSetInfo *tmp;

        while(set) {
                tmp = set->fs_next_frameset;
                if(set->fs_child_sizes)
                        free(set->fs_child_sizes);
                if(set->fs_child_size_types)
                        free(set->fs_child_size_types);
                if(set->fs_childs)
                        free(set->fs_childs);
                free(set);
                set = tmp;
        }
        set = NULL;
}

/*
* Description:  map's all HTML frame childs to screen
* In:
*       html:           XmHTMLWidget id
*/
void _XmHTMLMapFrames(HTMLWidget hw /* top XmHTMLWidget */)
{
        Widget frame;
        int i;

        /* map all HTML frame childs */
	
        for(i = 0; i < hw->html.nframe; i++) {
                frame = (Widget) hw->html.frames[i];
                XtSetMappedWhenManaged(frame, True); /* map to screen */
        }
/* resync */
        XSync(XtDisplay(hw), False);
}

/*****
* Description:  frame creation
* In:
*       hw:           XmHTMLWidget id (the frameset)
*       frame:        frame data	(the frame in frameset)
* Returns:
*       Widget id of frame to use, NULL otherwise
*****/
HTMLWidget _XmHTMLFrameCreate(HTMLWidget hw, FrameInfo * frame)
{
        Arg args[20];
        Dimension argc = 0;
        HTMLWidget html_widget;
        Widget widget;

        /* set constraints and other frame stuff */
        XtSetArg(args[argc], XmNx, frame->box_x); argc++;
        XtSetArg(args[argc], XmNy, frame->box_y); argc++;
        XtSetArg(args[argc], XmNwidth, frame->box_width - frame->frame_border); argc++;
        XtSetArg(args[argc], XmNheight, frame->box_height - frame->frame_border); argc++;
        XtSetArg(args[argc], XmNmarginWidth, frame->frame_margin_width); argc++;
        XtSetArg(args[argc], XmNmarginHeight, frame->frame_margin_height); argc++;
        XtSetArg(args[argc], XmNborderWidth, frame->frame_border); argc++;
        XtSetArg(args[argc], XmNborderColor, hw->manager.top_shadow_color);argc++;
        XtSetArg(args[argc], XmNmappedWhenManaged, False); argc++;

/* scrolling gets handled in the widget code itself, so don't set it 
* Create when we have to, the widget is NULL or the widget isn't a
* XmHTML widget.
*/
	widget = XtCreateManagedWidget(frame->frame_name, htmlWidgetClass,
			hw->html.view, args, argc);

        html_widget = (HTMLWidget)widget;
        html_widget->html.frame_type = FRAME_TYPE;
/*        html_widget->html.frame_border = frame->html.frame_border;	*/
/*        html_widget->html.frame_scroll_type = frame->html.frame_scroll_type; */

        /* manage it */
        XtManageChild(widget);
        return((HTMLWidget) widget);
}

/*****
* Name:                 _XmHTMLDestroyFrames
* Return Type:  void
* Description:  frame destroyer
* In:
*       hw:           XmHTMLWidget id
* Returns:
*       nothing, but the frames list of the widget is destroyed.
*****/
void  _XmHTMLDestroyFrames(HTMLWidget hw)
{
        int i = 0;

        /* unmap all XmHTML frame childs */
        for(i = 0; i < hw->html.nframe; i++)
               XtSetMappedWhenManaged((Widget)hw->html.frames[i], False);

	XmUpdateDisplay((Widget)hw);
        /* free them */

        for(i = 0; i < hw->html.nframe; i++) { /* destroy everything */
		HideWidgets(hw->html.frames[i]);
		HTMLFreeWidgetInfo(hw->html.frames[i]->html.widget_list);
        	hw->html.frames[i]->html.widget_list = NULL;  
        	hw->html.frames[i]->html.form_list = NULL;
               	XtDestroyWidget((Widget)hw->html.frames[i]);
                hw->html.frames[i] = NULL ;/* sanity */
        }
        free(hw->html.frames);
        hw->html.frames = NULL;
        hw->html.nframe = 0;
	hw->html.frame_type = NOTFRAME_TYPE;
}

/*
* Description:  resize method for XmHTML frame childs
* In:
*       hw:           XmHTMLWidget id
*/
void _XmHTMLReconfigureFrames(HTMLWidget hw, TopFrameSetInfo * top_seti)
{
        HTMLWidget frame;
 	FrameInfo *fi;
	int i;

/* compute new screen positions */ /* necessaire ????? */
/*####	_XmHTMLFrameAdjustConstraints(hw, top_seti); #### */

/* reconfigure all widgets */
	fi = top_seti->frames;
	for(i = 0; i < hw->html.nframe; i++) {
		frame = hw->html.frames[i];
		XtConfigureWidget((Widget)frame,
			fi[i].box_x, fi[i].box_y,
			fi[i].box_width - fi[i].frame_border,
			fi[i].box_height - fi[i].frame_border,
			fi[i].frame_border);
        }
}

/* doesn't work yet */
#if 0
void _XmHTMLDrawFrameBorder(XmHTMLWidget hw)
{
        int x = hw->core.x;
        int y = hw->core.y;
        int width = hw->html.frame_border;
        int height = hw->core.height;
        Display *dsp = XtDisplay((Widget)hw);
        GC gc;
        Window win = XtWindow((Widget)hw);

        gc = hw->manager.bottom_shadow_GC;
        XFillRectangle(dsp, win, gc, x, y, width, 1);
        XFillRectangle(dsp, win, gc, x, y, 1, height-1);

        gc = hw->manager.top_shadow_GC;
        XFillRectangle(dsp, win, gc, x+1, y + height-1, width-1, 1);
        XFillRectangle(dsp, win, gc, x + width - 1, y + 1, 1, height-2);
}
#endif

