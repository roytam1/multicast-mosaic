/* Copyright G.Dauphin Sep 97 */

#include <stdlib.h>
#include <stdio.h>
/*#include <sys/varargs.h>  correction 1999-03-17 jolk@ap-pc513b.physik.uni-karlsruhe.de*/
#include <stdarg.h>

#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "HTMLframe.h"

#define __WFUNC__(WIDGET_ID, FUNC)      (Widget)WIDGET_ID, __FILE__, \
         __LINE__, FUNC

#define DEBUG_FRAME 1

#ifdef DEBUG_FRAME
#define _XmHTMLDebug(LEVEL,MSG) do {\
                { printf MSG;} \
         }while(0)
#else
#define _XmHTMLDebug(LEVEL,MSG)                         /* empty */
#endif

#define XmIsHTML(w)	XtIsSubclass(w, htmlWidgetClass)

static void adjustFrame(HTMLWidget parent, int *p_width, int *p_height);
 
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


extern struct mark_up * NULL_ANCHOR_PTR  ;

/* Basically we show the urls that appear within the frameset tag
   as urls and add some text explaining that these are the urls they
   were supposed to see as frames. We also show the NOFRAMES stuff. */

/* just now do the same things as anchor */

void FramePlace(HTMLWidget hw, struct mark_up *mptr,PhotoComposeContext * pcc)
{
	char * tptr;

/* Change color of anchors with SRC, because other anchors are not active. */
	pcc->anchor_tag_ptr = mptr;
	tptr = mptr->anc_href = ParseMarkTag(mptr->start, MT_FRAME, "SRC");
	if (!tptr)
		return;
	if (tptr != NULL) {
		pcc->fg = hw->html.anchor_fg;
		pcc->underline_number = hw->html.num_anchor_underlines;
		pcc->dashed_underlines = hw->html.dashed_anchor_lines;
/* we may want to send the href back somewhere else and
 * find out if we've visited it before */
		if (hw->html.previously_visited_test != NULL) {
			if((*(visitTestProc)
				(hw->html.previously_visited_test)) ((Widget)hw, tptr, hw->html.base_url)) {
				pcc->fg = hw->html.visitedAnchor_fg;
				pcc->underline_number = hw->html.num_visitedAnchor_underlines;
				pcc->dashed_underlines= hw->html.dashed_visitedAnchor_lines;
			} else {
				pcc->fg = hw->html.anchor_fg;
				pcc->underline_number = hw->html.num_anchor_underlines;
				pcc->dashed_underlines = hw->html.dashed_anchor_lines;
			}
		} else {
			pcc->fg = hw->html.anchor_fg;
			pcc->underline_number =hw->html.num_anchor_underlines;
			pcc->dashed_underlines = hw->html.dashed_anchor_lines;
		}
	}
	if (pcc->in_underlined) {
		pcc->dashed_underlines = False;
		if (!pcc->underline_number)
			pcc->underline_number = 1;
	}

	LineBreak(hw,mptr,pcc);
	mptr->text = tptr;
	PartOfTextPlace(hw, mptr, pcc);
	mptr->text = NULL;

	pcc->fg = hw->manager.foreground;
	pcc->underline_number = pcc->in_underlined;
	pcc->dashed_underlines = False;
	pcc->anchor_tag_ptr = NULL_ANCHOR_PTR;
}

/*##############################################*/
/* Come from XmHTML Widget :
/* Copyright (C) 1994-1997 by Ripley Software Development */

/*** External Function Prototype Declarations ***/

/*** Public Variable Declarations ***/
/* how many times may we retry frame alignment? */
#define MAX_FRAME_ITERATIONS    100
#define ROW     1
#define COL     2
#define ROW_COL	4

/* usefull defines */
#define IS_FRAMESET(F) \
		((F)->html.frame_type & FRAMESET_TYPE)
#define IS_FRAME_SIZE_RELATIVE(F) \
		((F)->html.frame_size_type == FRAME_SIZE_RELATIVE)
#define IS_FRAME_SIZE_OPTIONAL(F) \
		((F)->html.frame_size_type == FRAME_SIZE_OPTIONAL)
#define IS_FRAME_SIZE_FIXED(F) \
		((F)->html.frame_size_type == FRAME_SIZE_FIXED)
#define IS_FRAMESET_LAYOUT_ROWS(F) \
		(IS_FRAMESET(F) && ((F)->html.frame_layout == FRAMESET_LAYOUT_ROWS))
#define IS_FRAMESET_LAYOUT_COLS(F) \
		(IS_FRAMESET(F) && ((F)->html.frame_layout == FRAMESET_LAYOUT_COLS))
#define IS_FRAMESET_LAYOUT_ROW_COLS(F) \
		(IS_FRAMESET(F) && ((F)->html.frame_layout == FRAMESET_LAYOUT_ROW_COLS))


/* definition of a HTML frameset */

typedef struct _frameSet{
        int type;	/* type of this set, either ROW or COL */
        int border;     /* frame border value */
        int *sizes;     /* array of child sizes */
        FrameSize *size_types;	/* array of possible size specifications */
        int nchilds;    /* max no of childs */
        int childs_done;/* no of childs processed so far */
        int insert_pos; /* insertion position of current child */
        struct _frameSet *parent;       /* parent frameset of this frameset */
        struct _frameSet *childs;       /* list of childs */
        struct _frameSet *next;         /* ptr to next frameSet */
	HTMLWidget actualFrameSet; /* ptr to saved FrameSet */
}frameSet;

/* stack of framesets */

typedef struct _frameStack{
        frameSet *frame_set;
        struct _frameStack *next;
}frameStack;

static int current_frame;       /* running frame counter */
static frameSet *frame_sets;    /* list of all framesets processed */
static frameStack frame_base, *frame_stack;


/*****
* Name:                 pushFrameSet
* Return Type:  void
* Description:  pushes a frameset on the stack
* In:
*       frame_set:      frameset to push
* Returns:
*       nothing
*****/
static void pushFrameSet(frameSet *frame_set)
{
        frameStack *tmp;

        tmp = (frameStack*)malloc(sizeof(frameStack));
        tmp->frame_set = frame_set;
        tmp->next = frame_stack;
        frame_stack = tmp;
}

/*****
* Name:                 popFrameSet
* Return Type:  frameSet*
* Description:  pops a frameset of the stack
* In:
*       nothing
* Returns:
*       the next frameset on the stack, or NULL when stack is empty
*****/
static frameSet* popFrameSet(void)
{
        frameStack *tmp;
        frameSet *frame_set;

        if(frame_stack->next) {
                tmp = frame_stack;
                frame_stack = frame_stack->next;
                frame_set = tmp->frame_set;
                free(tmp);
                return(frame_set);
        }
        return(NULL);
}

/*****
* Name:                 doFrameSet
* Return Type:  frameSet*
* Description:  creates and fills a frameSet structure with the info in it's
*                               attributes
* In:
*       attributes:     attributes for this frameset
* Returns:
*       a newly created frameset.
* Note:
*       this routine inserts each frameset it creates in a linked list which
*       is used for stack purposes.
*****/
static frameSet* doFrameSet(String attributes)
{
        frameSet *list, *tmp;
        String chPtr, tmpPtr, ptr;
        int i;

        /* nothing to do if no attributes */
        if(attributes == NULL)
                return(frame_sets);

        /* create new entry */
        list = (frameSet*)malloc(sizeof(frameSet));
        (void)memset(list, 0, sizeof(frameSet));

        list->type = ROW;

        if((chPtr = ParseMarkTag(attributes, MT_FRAMESET,  "rows")) == NULL) {
                if((chPtr = ParseMarkTag(attributes, MT_FRAMESET, "cols")) == NULL) {
                        /* useless sanity, should be catched upon entry */
/*                        free(list);*/
/*                        return(frame_sets);*/
                } else
                        list->type = COL;
        }

        /*
        * count how many childs this frameset has: the no of childs is given by
        * the no of entries within the COLS or ROWS tag
        * Note that childs can be frames and/or framesets as well.
        */
        for(tmpPtr = chPtr; tmpPtr && *tmpPtr != '\0'; tmpPtr++)
                if(*tmpPtr == ',')
                        list->nchilds++;
        list->nchilds++;

        list->sizes = (int*)calloc(list->nchilds, sizeof(int));
        list->size_types = (FrameSize*)calloc(list->nchilds, sizeof(FrameSize));
        list->childs = (frameSet*)calloc(list->nchilds, sizeof(frameSet));

        /*
        * get dimensions: when we encounter a ``*'' in a size definition it
        * means we are free to choose any size we want. When its a number
        * followed by a ``%'' we must choose the size relative against the total
        * width of the render area. When it's a number not followed by anything
        * we have an absolute size.
        */
        tmpPtr = ptr = chPtr;
        i = 0;
	list->size_types[i] = FRAME_SIZE_OPTIONAL;
        while(tmpPtr){
                if(*tmpPtr == ',' || *tmpPtr == '\0') {
                        if(*(tmpPtr-1) == '*')
                                list->size_types[i] = FRAME_SIZE_OPTIONAL;
                        else if(*(tmpPtr-1) == '%')
                                list->size_types[i] = FRAME_SIZE_RELATIVE;
                        else
                                list->size_types[i] = FRAME_SIZE_FIXED;

                        list->sizes[i++] = atoi(ptr);

                        if(*tmpPtr == '\0')
                                break;
                        ptr = tmpPtr+1;
                }
                tmpPtr++;
                /* sanity */
                if(i == list->nchilds)
                        break;
        }
        if (chPtr) free(chPtr);

/* Frame borders can be specified by both frameborder or border, they
 * are equal.
 */
        list->border = 2;
        if((chPtr = ParseMarkTag(attributes, MT_FRAMESET, "frameborder")) != NULL){

/* Sigh, stupid Netscape frameset definition allows a tag to have
* a textvalue or a number.
*/
                if(!(strcasecmp(chPtr, "no")) || *chPtr == '0')
                        list->border = 0;
                else
                        list->border = atoi(chPtr);
                free(chPtr);
        }

/* insert this new frame in the overal frameset list. */
        if(frame_sets == NULL)
                frame_sets = list;
        else {
                for(tmp = frame_sets; tmp != NULL && tmp->next != NULL;
                        tmp = tmp->next);
                tmp->next = list;
        }

/* create actual representation of frameset */
        {
          HTMLWidget actualFrameSet = NULL ;
          actualFrameSet = (HTMLWidget)calloc(1, sizeof(HTMLRec));
          actualFrameSet->html.frame_type |= FRAMESET_TYPE ;
          actualFrameSet->html.frame_layout =
                     (list->type == ROW ? FRAMESET_LAYOUT_ROWS : FRAMESET_LAYOUT_COLS);
          list->actualFrameSet = actualFrameSet ;
        }
        return(list);
}

/*****
* Name:                 doFrame
* Return Type:  XmHTMLFrameWidget*
* Description:  fills a HTML frame structure with data from it's attributes
* In:
*       html:           XmHTMLWidget id;
*       attributes:     frame attributes
* Returns:
*       updated frame
* Note:
*       this routine takes the frame to update from an already allocated list
*       of frames and increments the running frame counter when it returns.
*****/

static HTMLWidget doFrame(HTMLWidget html, String attributes)
{
/*        XmHTMLFrameWidget *frame; */
        HTMLWidget frame;
        String chPtr;

        frame = html->html.frames[current_frame];

/* default frame sizing & scrolling */
	frame->html.frame_size_type = FRAME_SIZE_FIXED;
        frame->html.frame_scroll_type = FRAME_SCROLL_AUTO;

/* get frame name, default to _frame if not present */
        if(!attributes || (frame->html.frame_name = ParseMarkTag(attributes, MT_FRAME, "name")) == NULL) {
                char buf[24];
                sprintf(buf, "_frame%i", current_frame);
                sprintf(buf, "_frame%i", current_frame);
                frame->html.frame_name = strdup(buf);
        }

        /* pick up all remaining frame attributes */
        if(attributes) {
                frame->html.frame_src = ParseMarkTag(attributes,MT_FRAME,"src");
		frame->html.frame_margin_width = 5;
		if (chPtr = ParseMarkTag(attributes, MT_FRAME, "marginwidth")){
			frame->html.frame_margin_width = atoi(chPtr);
			free(chPtr);
		}
		frame->html.frame_margin_height = 5;
		if (chPtr = ParseMarkTag(attributes, MT_FRAME, "marginheight")){
			frame->html.frame_margin_height = atoi(chPtr);
			free(chPtr);
		}

/* inherit margins from parent if we'd gotten an invalid spec */
		if(!frame->html.frame_margin_width)
                        frame->html.frame_margin_width = html->html.margin_width;
                if(!frame->html.frame_margin_height)
                        frame->html.frame_margin_height =html->html.margin_height;

/*
 * This is useless as we don't support frame resizing. I think this is
 * a thing the caller must be able to do. A possible way could be to
 * overlay the render area with a PanedWidget and store these HTML
 * widgets as childs of this paned widget.
 */
		if (chPtr = ParseMarkTag(attributes, MT_FRAME, "noresize")){
			frame->html.frame_resize = False;
			free(chPtr);
		}
/* what about scrolling? */
                if(chPtr = ParseMarkTag(attributes, MT_FRAME,"scrolling")){
                        if(!(strcasecmp(chPtr, "yes")))
                                frame->html.frame_scroll_type = FRAME_SCROLL_YES;
                        else if(!(strcasecmp(chPtr, "no")))
                                frame->html.frame_scroll_type = FRAME_SCROLL_NONE;
                        free(chPtr);
                }
        } else {
                frame->html.frame_src           = NULL;
                frame->html.frame_margin_width  = 5;
                frame->html.frame_margin_height = 5;
                frame->html.frame_resize        = True;
        }

        _XmHTMLDebug(11, ("frames.c: doFrame, frame %i created\n"
                "\tname: %s\n"
                "\tsrc : %s\n"
                "\tmargin width : %i\n"
                "\tmargin height: %i\n"
                "\tresize       : %s\n"
                "\tscrolling    : %s\n",
		current_frame,
		frame->html.frame_name,
                frame->html.frame_src ? frame->html.frame_src : "<none>",
		frame->html.frame_margin_width,
                frame->html.frame_margin_height,
		frame->html.frame_resize ? "yes" : "no",
                frame->html.frame_scroll_type == FRAME_SCROLL_AUTO ? "auto" :
                (frame->html.frame_scroll_type == FRAME_SCROLL_YES ? "always" : "none")));

        /*
        * Actual widget creation is postponed until the very last moment
        * of _XmHTMLCreateFrames
        */

        /* increment running frame counter */
        current_frame++;
        return(frame);
}

/*****
* Name:                 insertFrameSetChild
* Return Type:  void
* Description:  inserts a child frameset in it's parent list
* In:
*       parent:         parent of this frameset
*       child:          obvious
* Returns:
*       nothing
*****/
static void insertFrameSetChild(frameSet *parent, frameSet *child)
{
        if(parent && parent->childs_done < parent->nchilds) {
                int idx = parent->childs_done;
		HTMLWidget c, dad, son;

                child->parent = parent;
                child->insert_pos = idx;

                dad = parent->actualFrameSet;
                son = child->actualFrameSet;

                son->html.frame_size_s = parent->sizes[child->insert_pos];
                son->html.frame_size_type = parent->size_types[child->insert_pos];

                if(son->html.frame_size_s == 0)
                        son->html.frame_size_type = FRAME_SIZE_OPTIONAL;

                /* set additional constraints for this frame */
                son->html.frame_border = parent->border;

                /* disable resizing if we don't have a border */
                if(!son->html.frame_border)
                        son->html.frame_resize = False;

                for(c = dad->html.frame_children ; c != NULL ; c = c->html.frame_next)
                        if(!c->html.frame_next)
                                break;
                if(c)
                        c->html.frame_next = son;
                else
                        dad->html.frame_children = son ;
                son->html.frame_prev = c ;
                son->html.frame_parent_frameset = dad ;

                parent->childs[parent->childs_done] = *child;
                parent->childs_done++;
        }
}


/*****
* Name:                 insertFrameChild
* Return Type:  void
* Description:  sets the geometry constraints on a HTML frame
* In:
*       frame_set:      frameset parent of this frame;
*       frame:          frame for which to set the constraints
* Returns:
*       nothing, but frame is updated.
*****/
static void insertFrameChild(frameSet *frame_set, HTMLWidget frame)
{
	HTMLWidget c, dad;
        int insert_pos = frame_set->childs_done;

	frame->html.frame_size_s = frame_set->sizes[insert_pos];
        frame->html.frame_size_type = frame_set->size_types[insert_pos];

        if(frame->html.frame_size_s == 0)
                frame->html.frame_size_type = FRAME_SIZE_OPTIONAL;

        /* set additional constraints for this frame */
        frame->html.frame_border = frame_set->border;
        /*frame->html.frame_border = frame_set->border; */

        /* disable resizing if we don't have a border */
        if(!frame->html.frame_border)
          frame->html.frame_resize = False;
        /*if(!frame->html.frame_border)
                frame->html.frame_resize = False;*/

        dad = frame_set->actualFrameSet;
        for(c = dad->html.frame_children ; c != NULL ; c = c->html.frame_next)
                if(!c->html.frame_next)
                        break;
        if(c)
                c->html.frame_next = frame;
        else
                dad->html.frame_children = frame;
        frame->html.frame_prev = c;
        frame->html.frame_parent_frameset = dad;

        frame_set->childs_done++;
}

/* *mptr is a pointer to frameset */
void makeFrameset(HTMLWidget hw, struct mark_up **mptr, PhotoComposeContext *pcc)
{
	struct mark_up *tmp;
	frameSet *current_set = NULL;
	frameSet *parent_set = NULL;
	HTMLWidget frame;
	int idx = 0;

	for(tmp = *mptr; tmp != NULL; tmp = tmp->next) {
		switch(tmp->type) {
		case M_FRAMESET:
			if(tmp->is_end) { /* end frameset  pop stack*/
				current_set = popFrameSet();
/* no more sets on the stack : we've reached end of outermost frameset tag */
				if(current_set == NULL)
					return;
			} else { /* A new frameset, push the current frameset on the stack */
				pushFrameSet(current_set);
				parent_set = frame_stack->frame_set;
				/* Check if we still have room for this thing.*/
				if(!parent_set || parent_set->childs_done < parent_set->nchilds) {      /* create a new frameset */
					current_set = doFrameSet(tmp->start);
					insertFrameSetChild(parent_set, current_set);
					idx = 0;
				} else {
/* No more room available, this is an unspecified
* frameset, kill it and all childs it might have.
*/
					int depth = 1;
					int start_line = tmp->line;
					for(tmp = tmp->next; tmp != NULL; tmp = tmp->next) {
						if(tmp->type == M_FRAMESET) {
							if(tmp->is_end) {
								if(--depth == 0)
									break;
							} else /*child frameset*/
								depth++;
						}
					}
					_XmHTMLWarning(__WFUNC__(hw, "doFrameSets"),
						"Bad <FRAMESET> tag: missing COLS or ROWS attribute on parent set\n    Skipped all frame declarations between line %i and %i in input.", start_line, tmp ? tmp->line : -1);
				}
			}
			break;
		case M_FRAME:
			/* check if we have room left */
			if(current_set->childs_done < current_set->nchilds) {
				/* insert child in current frameset */
				frame = doFrame(hw, tmp->start);
				insertFrameChild(current_set, frame);
				idx++;
			} else {
				_XmHTMLWarning(__WFUNC__(hw, "doFrameSets"), "Bad <FRAME> tag on line %i of input: missing COLS or ROWS attribute\n    on parent set, skipped.", tmp->line);
/*				frame->html.frame_parent_frameset = NULL; */
			}
/* Note: </FRAME> doesn't exist. The parser is smart enough
 * to kick these out.
 *****/
		default:
			break;
		}
		if(idx == hw->html.nframe)
			return;
	}
}

static HTMLWidget getRootFrameset(HTMLWidget hw)
{
        HTMLWidget frame;

        for (frame = hw->html.frames[0];
                frame != NULL && frame->html.frame_parent_frameset != NULL; frame = frame->html.frame_parent_frameset);

        return(frame);
}


static void adjustFramesetRows(HTMLWidget parent, int *p_width, int *p_height)
{
        HTMLWidget child = NULL ;
        int width, height ;
        int cum_fixed_size = 0, cum_rel_size = 0, cum_opt_size = 0 ;
        int nb_opt = 0;

        /* Begin with fixed-sized children */
        cum_fixed_size = 0 ;
        for (child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                if(IS_FRAME_SIZE_FIXED(child)) {
                        width = *p_width ;
                        height = child->html.frame_size_s ;
                        adjustFrame(child, &width, &height);
                        child->html.frame_width = width ;
                        child->html.frame_height = height ;
                        cum_fixed_size += height ;
                }
        }

        /* Then do relative-sized children */
        cum_rel_size = 0 ;
        for (child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                if(IS_FRAME_SIZE_RELATIVE(child)) {
                        width = *p_width ;
                        height = child->html.frame_size_s * (*p_height) / 100 ;
                        adjustFrame(child, &width, &height);
                        child->html.frame_width = width ;
                        child->html.frame_height = height ;
                        cum_rel_size += height ;
                }
        }

/* Finally, end up with optional-sized children */
        cum_opt_size = 0 ;

/* count how many optional they are */
        for (child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next)
                if(IS_FRAME_SIZE_OPTIONAL(child))
                        ++nb_opt;

        if(nb_opt > 0) {
                int cum_size, remain_size, mean_opt_size ;

/*****
 * stupid hack : equal sizes for all optional fields.
 * FIXME! find sth smarter than that!
 *****/
                cum_size = cum_fixed_size + cum_rel_size ;
                remain_size = *p_height - cum_size ;
                if(remain_size <= nb_opt)
                        remain_size = nb_opt ;
                mean_opt_size = remain_size / nb_opt ;

                /* go adjust */
                for(child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                        if(IS_FRAME_SIZE_OPTIONAL(child)) {
                                width = *p_width ;
                                height = mean_opt_size ;
                                adjustFrame(child, &width, &height);
                                child->html.frame_width = width ;
                                child->html.frame_height = height ;
                                cum_opt_size += height ;
                        }
                }
        }
/* end of optional-sized children mgt */

#ifdef FEEDBACK_SIZES
        *p_height = cum_fixed_size + cum_rel_size + cum_opt_size ;
        if(*p_height <= 0)
                *p_height = 1 ;
#endif
}

static void adjustFramesetColumns(HTMLWidget parent, int *p_width, int *p_height)
{
        HTMLWidget child = NULL ;
        int width, height ;
        int cum_fixed_size = 0, cum_rel_size = 0, cum_opt_size = 0 ;
        int nb_opt = 0;

        /* Begin with fixed-sized children */
        cum_fixed_size = 0 ;
        for(child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                if(IS_FRAME_SIZE_FIXED(child)) {
                        width = child->html.frame_size_s ;
                        height = *p_height ;
                        adjustFrame(child, &width, &height);
                        child->html.frame_width = width ;
                        child->html.frame_height = height ;
                        cum_fixed_size += width ;
                }
        }

        /* Then do relative-sized children */
        cum_rel_size = 0 ;
        for (child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                if(IS_FRAME_SIZE_RELATIVE(child)) {
                        width = child->html.frame_size_s * (*p_width) / 100 ;
                        height = *p_height ;
                        adjustFrame(child, &width, &height);
                        child->html.frame_width = width ;
                        child->html.frame_height = height ;
                        cum_rel_size += width ;
                }
        }

        /* Finally, end up with optional-sized children */
        cum_opt_size = 0 ;

        /* count how many optional they are */
        for (child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next)
                if(IS_FRAME_SIZE_OPTIONAL(child))
                        ++nb_opt;

        if(nb_opt > 0) {
                int cum_size, remain_size, mean_opt_size ;

/*****
 * stupid hack : equal sizes for all optional fields.
 * FIXME! find sth smarter than that!
 *****/
                cum_size = cum_fixed_size + cum_rel_size ;
                remain_size = *p_width - cum_size ;
                if(remain_size <= nb_opt)
                        remain_size = nb_opt ;
                mean_opt_size = remain_size / nb_opt ;

                /* go adjust */
                for(child = parent->html.frame_children ; child != NULL ; child = child->html.frame_next) {
                        if(IS_FRAME_SIZE_OPTIONAL(child)) {
                                width = mean_opt_size ;
                                height = *p_height ;

                                adjustFrame(child, &width, &height);
                                child->html.frame_width = width ;
                                child->html.frame_height = height ;
                                cum_opt_size += width ;
                        }
                }
        }
/* end of optional-sized children mgt */

#ifdef FEEDBACK_SIZES
        *p_width = cum_fixed_size + cum_rel_size + cum_opt_size ;
        if(*p_width <= 0)
                *p_width = 1 ;
#endif
}

static void adjustFrame(HTMLWidget parent, int *p_width, int *p_height)
{
        if(*p_width <= 0)
                *p_width = 1 ;
        if(*p_height <= 0)
                *p_height = 1 ;

        if(IS_FRAMESET(parent)) /* do recursion only if it is a frameset */
        {
                if(parent->html.frame_layout == FRAMESET_LAYOUT_ROWS)
                        adjustFramesetRows(parent, p_width, p_height);
                else if(parent->html.frame_layout == FRAMESET_LAYOUT_COLS)
                        adjustFramesetColumns(parent, p_width, p_height);
        }
}

static void locateFrame(HTMLWidget parent, int x, int y)
{
        parent->html.frame_x = x;
        parent->html.frame_y = y;

        if(IS_FRAMESET(parent)) /* do recursion only if it is a frameset */
        {
                HTMLWidget frame ;

                if(IS_FRAMESET_LAYOUT_ROWS(parent)) {
                        for(frame = parent->html.frame_children ; frame != NULL ; frame = frame->html.frame_next) {
                                locateFrame(frame, x, y);
                                y += frame->html.frame_height ;
                        }
                }

                if(IS_FRAMESET_LAYOUT_COLS(parent)) {
                        for(frame = parent->html.frame_children ; frame != NULL ; frame = frame->html.frame_next) {
                                locateFrame(frame, x, y);
                                x += frame->html.frame_width ;
                        }
                }
        }
}

static void adjustConstraints(HTMLWidget hw /* top XmHTMLWidget */)
{
        HTMLWidget root_frame;
        int work_width, work_height;

        /* this uses the core dimensions */
        work_width = hw->core.width;
        work_height = hw->core.height;

        /* get the root frame */
        root_frame = getRootFrameset(hw);

        /* adjust frames' dimensions */
        adjustFrame(root_frame, &work_width, &work_height);

        /* adjust frames' positions */
        locateFrame(root_frame, 0, 0);
}


/*****
* Name:                 destroyFrameSets
* Return Type:  void
* Description:  destroys the memory used by the framesets
* In:
*       set:            list of framesets to be destroyed
* Returns:
*       nothing
*****/
static void destroyFrameSets(frameSet *set)
{
        frameSet *tmp;

        while(set) {
                tmp = set->next;
                if(set->sizes)
                        free(set->sizes);
                if(set->size_types)
                        free(set->size_types);
                if(set->childs)
                        free(set->childs);
                free(set);
                set = tmp;
        }
        set = NULL;
}

/*
* Name:                 mapFrames
* Description:  map's all XmHTML frame childs to screen
* In:
*       html:           XmHTMLWidget id
* Return :  void
*/
static void mapFrames(HTMLWidget hw /* top XmHTMLWidget */)
{
        HTMLWidget frame;
        XmHTMLFrameCallbackStruct cbs;
        int i;

        if(!hw->html.frame_callback)
                return;
        /* map all XmHTML frame childs */
        for(i = 0; i < hw->html.nframe; i++) {
                frame = hw->html.frames[i];
/* map to screen */
                XtSetMappedWhenManaged(frame->html.frame_wid, True);
/* inform user that this frame is finished . call notifier */
        	cbs.reason = XmCR_HTML_FRAMEDONE;
        	cbs.event = NULL;
        	cbs.src = frame->html.frame_src;
        	cbs.name = frame->html.frame_name;
        	cbs.html = frame->html.frame_wid;
        	cbs.doit = False;
/* call the callback list */
        	XtCallCallbackList((Widget)hw, hw->html.frame_callback, &cbs);
        }
/* resync */
        if(hw->html.drawGC)
                XSync(XtDisplay(hw), False);
}

/*****
* Name:                 _XmHTMLCheckForFrames
* Return Type:  int
* Description:  checks if the given list of objects contains HTML frames
* In:
*       html:           XmHTMLWidget id;
*       objects:        parser output to check
* Returns:
*       no of frames found in the current document.
*****/
int _XmHTMLCheckForFrames(HTMLWidget hw, struct mark_up **mptr,PhotoComposeContext * pcc )
{
        struct mark_up *tmp;
        int nframes = 0;

        /* we only support frames if user has attached a frame callback */
        if(!hw->html.frame_callback)
                return(0);

        /*
        * frames are not allowed to appear inside the BODY tag.
        * So we never have to walk the entire contents of the current document
        * but simply break out of the loop once we encounter the <BODY> tag.
        * This is a fairly huge performance increase.
        */
        for(tmp = *mptr; tmp != NULL && tmp->type != M_BODY; tmp = tmp->next)
                if(tmp->type == M_FRAME)
                        nframes++;

        return(nframes);
}

/*****
* Name:                 _XmHTMLFrameCreateCallback
* Return Type:  void
* Description:  frame creation notifier
* In:
*       hw:           XmHTMLWidget id (the frameset)
*       frame:          frame data	(the frame in frameset)
* Returns:
*       Widget id of frame to use, NULL otherwise
*****/
Widget _XmHTMLFrameCreateCallback(HTMLWidget hw, HTMLWidget frame)
{
        XmHTMLFrameCallbackStruct cbs;
        Arg args[20];
        Dimension argc = 0;
        HTMLWidget html_widget;
        static Widget widget;

        /* inform user that this frame is about to be created */
        if(!hw->html.frame_callback)
                return(NULL);

        cbs.reason = XmCR_HTML_FRAMECREATE;
        cbs.event = NULL;
        cbs.src = frame->html.frame_src;
        cbs.name = frame->html.frame_name;
        cbs.html = NULL;
        cbs.doit = True;

        /* call the callback list */
        XtCallCallbackList((Widget)hw, hw->html.frame_callback, &cbs);


        /* set constraints and other frame stuff */
        XtSetArg(args[argc], XmNx, frame->html.frame_x); argc++;
        XtSetArg(args[argc], XmNy, frame->html.frame_y); argc++;
        XtSetArg(args[argc], XmNwidth, frame->html.frame_width - frame->html.frame_border); argc++;
        XtSetArg(args[argc], XmNheight, frame->html.frame_height - frame->html.frame_border); argc++;
        XtSetArg(args[argc], XmNmarginWidth, frame->html.frame_margin_width); argc++;
        XtSetArg(args[argc], XmNmarginHeight, frame->html.frame_margin_height); argc++;
        XtSetArg(args[argc], XmNborderWidth, frame->html.frame_border); argc++;
        XtSetArg(args[argc], XmNborderColor, hw->manager.top_shadow_color);argc++;
        XtSetArg(args[argc], XmNmappedWhenManaged, False); argc++;

/* scrolling gets handled in the widget code itself, so don't set it 
* Create when we have to, the widget is NULL or the widget isn't a
* XmHTML widget.
*/
        if(cbs.doit == True || cbs.html == NULL)
		widget = XtCreateManagedWidget(cbs.name, htmlWidgetClass,
			hw->html.view, args, argc);
        else if(!XmIsHTML(cbs.html)) {
                /* not HTMLwidget, spit out a warning and create one ourselves */
                _XmHTMLWarning(__WFUNC__(cbs.html, "_XmHTMLFrameCreateCallback"),
                        "Bad HTML frame widget: not a subclass of xmHTMLWidgetClas");
		widget = XtCreateManagedWidget(cbs.name, htmlWidgetClass,
			hw->html.view, args, argc);
        } else {
                widget = cbs.html;

                /* first unmanage if it's still up */
                if(XtIsManaged(widget))
                        XtUnmanageChild(widget);

                /* check if we need to clear any existing source */
                if(((HTMLWidget)widget)->html.html_objects != NULL) {
			HTMLSetHTMLmark(widget, NULL, 0, NULL, NULL );
                }

                /* reconfigure this widget so it'll fit our purposes */
                XtSetValues(widget, args, argc);

                /* unmanage scrollbars as well */
                ((HTMLWidget)widget)->html.use_vbar = False;
                ((HTMLWidget)widget)->html.use_hbar = False;
                XtUnmanageChild(((HTMLWidget)widget)->html.hbar);
                XtUnmanageChild(((HTMLWidget)widget)->html.vbar);
        }

        html_widget = (HTMLWidget)widget;
        html_widget->html.frame_type = FRAME_TYPE;  /* maybe |= FRAME_TYPE?? */
        html_widget->html.frame_border = frame->html.frame_border;
        html_widget->html.frame_scroll_type = frame->html.frame_scroll_type;

        /* manage it */
        XtManageChild(widget);
        return(widget);
}

static void recursiveDestroyFrameset(HTMLWidget frame)
{
        if (!frame) /* sanity */
    		return ;

        if (IS_FRAMESET(frame)) {
                HTMLWidget child, tmp ;
                for(child = frame->html.frame_children ; child != NULL ; ) {
                        tmp = child->html.frame_next;
                        recursiveDestroyFrameset(child);
                        child = tmp ;
                }
                frame->html.frame_children = NULL ;

                if(frame->html.frame_src) {
                        free(frame->html.frame_src);
                        frame->html.frame_src = NULL; /* sanity */
                }
                if(frame->html.frame_name) {
                        free(frame->html.frame_name);
                        frame->html.frame_name = NULL; /* sanity */
                }
                frame->html.frame_parent_frameset = NULL; /* sanity */

                free(frame);
                frame = NULL ;
        }
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
 	HTMLWidget root_frame = NULL;

        /* unmap all XmHTML frame childs */
        for(i = 0; i < hw->html.nframe; i++)
               XtSetMappedWhenManaged(hw->html.frames[i]->html.frame_wid, False);

        /* free them */
        root_frame = getRootFrameset(hw);
        recursiveDestroyFrameset(root_frame);

        for(i = 0; i < hw->html.nframe; i++) {
	        int ret_val;
        	XmHTMLFrameCallbackStruct cbs;
		HTMLWidget frame;

        	if(!hw->html.frame_callback){
                	hw->html.frames[i] = NULL; /* sanity */
                	continue;
		}

/* inform user that this frame is about to be destroyed */
		frame = hw->html.frames[i];
        	cbs.reason = XmCR_HTML_FRAMEDESTROY;
        	cbs.event = NULL;
        	cbs.src = frame->html.frame_src;
        	cbs.name = frame->html.frame_name;
        	cbs.html = frame->html.frame_wid;
        	cbs.doit = True;
        	XtCallCallbackList((Widget)hw, hw->html.frame_callback, &cbs);

/* cbs.doit = 0: clear frame data but don't destroy the widget;
 * cbs.doit = 1: clear and destroy;
 */
		ret_val = cbs.doit;

/* always destroy this */
        	if(frame->html.frame_src) {
          		free(frame->html.frame_src);
          		frame->html.frame_src = NULL; /* sanity */
        	}
        	if(frame->html.frame_name) {
          		free(frame->html.frame_name);
          		frame->html.frame_name = NULL; /* sanity */
        	}
        	frame->html.frame_parent_frameset = NULL; /* sanity */

/* return if we may not destroy this frame */
        	if(ret_val == 0) { /*destroy frame data, but keep widget alive*/
                	free(frame);
                	frame = NULL;
                	return;
        	}
/* destroy everything */
        	if(frame->html.frame_wid)
                	XtDestroyWidget(frame->html.frame_wid);
        	free(frame);
        	frame = NULL;
                hw->html.frames[i] = NULL ;/* sanity */
        }
        free(hw->html.frames);
        hw->html.frames = NULL;
        hw->html.nframe = 0;
	hw->html.frame_type = NOTFRAME_TYPE;
}

/*****
* Name:                 _XmHTMLReconfigureFrames
* Return Type:  void
* Description:  resize method for XmHTML frame childs
* In:
*       hw:           XmHTMLWidget id
* Returns:
*       nothing
*****/
void _XmHTMLReconfigureFrames(HTMLWidget hw)
{
        HTMLWidget frame;
        int i;

        _XmHTMLDebug(11, ("frames.c: _XmHTMLReconfigureFrames Start\n"));
        /* compute new screen positions */
        adjustConstraints(hw);

        /* reconfigure all widgets */
        for(i = 0; i < hw->html.nframe; i++) {
                frame = hw->html.frames[i];

                _XmHTMLDebug(11,("frames.c: _XmHTMLReconfigureFrames doing frame "
                        "%s.\n", frame->html.frame_name));

                XtConfigureWidget(frame->html.frame_wid, frame->html.frame_x, frame->html.frame_y,
                        frame->html.frame_width - frame->html.frame_border,
                        frame->html.frame_height - frame->html.frame_border, frame->html.frame_border);
/*		HTML_ATTR(tka)->ConfigureWidget(frame->frame, frame->x, frame->y,
                        frame->width - frame->border,
                        frame->height - frame->border, frame->border);
*/
        }
        _XmHTMLDebug(11, ("frames.c: _XmHTMLReconfigureFrames End.\n"));
}

/*****
* Name:                 _XmHTMLCreateFramecwSet
* Return Type:  Boolean
* Description:  main frame creator
* In:
*       html_old:       previous XmHTMLWidget id;
*       hw:           XmHTMLWidget id;
* Returns:
*       True when all frames could be created, False otherwise.
*****/
Boolean _XmHTMLCreateFrameSet(HTMLWidget old, HTMLWidget hw, struct mark_up **mptr, PhotoComposeContext *pcc)
{
        int i;
        struct mark_up **tmp;
        static Widget frame;

        frame_stack = &frame_base;
        frame_stack->next = NULL;
        frame_stack->frame_set = NULL;

        /* first destroy all previous frames of this widget */
        if(old && old->html.nframe && IS_FRAMESET(old))
                _XmHTMLDestroyFrames(old);

        if(frame_sets)
                destroyFrameSets(frame_sets);
        frame_sets = NULL;

	hw->html.nframe = _XmHTMLCheckForFrames(hw, mptr, pcc);
	hw->html.frame_type |= FRAMESET_TYPE;
/*
* Don't do a thing if we are destroying the previous list, we don't have
* a frame callback or the new widget doesn't have any frames at all
*/
        if(hw == NULL || !hw->html.frame_callback || hw->html.nframe == 0) {	
		hw->html.frames = NULL;
                return(False);
	}
        frame = NULL;

        /* create the list of HTML frame childs */
        hw->html.frames = (HTMLWidget*)calloc(hw->html.nframe,
                sizeof(HTMLWidget));

        /* create individual HTML frame child ptrs */
        for(i = 0; i < hw->html.nframe; i++) {
                HTMLWidget frame_w;
                frame_w = (HTMLWidget)malloc(sizeof(HTMLRec));
                (void)memset(frame_w, 0, sizeof(HTMLRec));
                hw->html.frames[i] = frame_w;
        }

        /* move to the first frameset declaration */
	tmp = mptr;
/*        for(tmp = hw->html.html_objects; tmp != NULL && tmp->type != M_FRAMESET;
                tmp = tmp->next); */
	tmp = mptr;

        current_frame = 0;
        /* create all frames (and possibly nested framesets also) */
	makeFrameset( hw, tmp, NULL);
	*tmp = NULL;	/* This ends the scanning of Mark loop */
			/* no more HTML is computed after the outermost framset
			   tag */

        /* adjust framecount, makeFrameSets might have found some invalid sets */
        hw->html.nframe = current_frame;

#ifdef DEBUG
        _XmHTMLDebug(11, ("frames.c: _XmHTMLCreateFrames, raw frame listing\n"));
        for(i = 0; i < hw->html.nframe; i++) {
                _XmHTMLDebug(11, ("frame %i\n"
                        "\tname           : %s\n"
                        "\tsrc            : %s\n"
                        "\tsize           : %i\n",
                        i, hw->html.frames[i]->src, hw->html.frames[i]->name,
                        hw->html.frames[i]->size_s));
        }
#endif

        adjustConstraints(hw);

#ifdef DEBUG
        _XmHTMLDebug(11, ("frames.c: _XmHTMLCreateFrames, adjusted frame "
                "listing\n"));
        for(i = 0; i < hw->html.nframe; i++) {
                _XmHTMLDebug(11, ("frame %i\n"
                        "\tname           : %s\n"
                        "\tsrc            : %s\n"
                        "\twidth by height: %ix%i\n"
                        "\tx offset       : %i\n"
                        "\ty offset       : %i\n",
                        i, hw->html.frames[i]->src, hw->html.frames[i]->name,
                        hw->html.frames[i]->width, hw->html.frames[i]->height,
                        hw->html.frames[i]->x, hw->html.frames[i]->y));
        }
#endif
        /* and now create all frames */
        for(i = 0; i < hw->html.nframe; i++) {
                hw->html.frames[i]->html.frame_wid = _XmHTMLFrameCreateCallback(
			hw, hw->html.frames[i]);
        }
        /* erase a few glitches by calling adjustConstraints again */
        _XmHTMLReconfigureFrames(hw);

        /* and now map them to screen */
        mapFrames(hw);
        return(True);
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


/*****
* Name:                 XmHTMLFrameGetChild
* Return Type:  Widget
* Description:  returns the Widget id of a frame child given it's name.
* In:
*       w:                      XmHTMLWidget
*       name:           name of frame to locate.
* Returns:
*       If found, the widget id of the requested frame, NULL otherwise.
*****/
Widget XmHTMLFrameGetChild(Widget w, String name)
{
        HTMLWidget html;
        int i;

        /* sanity check */
        if(!w || !XmIsHTML(w) || name == NULL) {
                String func = "FrameGetChild";
                if(name == NULL)
                        _XmHTMLWarning(__WFUNC__(w, func),
                                "%s passed to %s", "NULL frame name", func);
                else
			_XmHTMLWarning(__WFUNC__(w, func), "%s parent passed to %s.",
				(w ? "Invalid parent" : "NULL parent"), func);
                return(NULL);
        }
        html = (HTMLWidget)w;
        for(i = 0; i < html->html.nframe; i++) {
                if(!(strcmp(html->html.frames[i]->html.frame_name, name)))
                        return(html->html.frames[i]->html.frame_wid);
        }
        return(NULL);
}

static Boolean areAllSizesOptional(HTMLWidget frameset)
{
        Boolean all_opt = True ;
        if (IS_FRAMESET(frameset))  {
                HTMLWidget frame;

                for(frame = frameset->html.frame_children ; frame != NULL ; frame = frame->html.frame_next) {
                        if(IS_FRAME_SIZE_OPTIONAL(frame)) {
                                all_opt = False;
                                break;
                        }
                }
        }
        return(all_opt);
}

static Boolean areAllSizesRelative(HTMLWidget frameset)
{
        Boolean all_rel = False ;
        if(IS_FRAMESET(frameset)) {
                HTMLWidget frame;

                all_rel = True ;
                for(frame = frameset->html.frame_children ; frame != NULL ; frame = frame->html.frame_next) {
                        if (IS_FRAME_SIZE_RELATIVE(frame)) {
                                all_rel = False;
                                break;
                        }
                }
        }
        return(all_rel);
}

static int relativeSizesSum(HTMLWidget frameset)
{
        int rel_sum = 0 ;
        if(IS_FRAMESET(frameset)) {
                HTMLWidget frame;
                for(frame = frameset->html.frame_children ; frame != NULL ; frame = frame->html.frame_next) {
                        if (IS_FRAME_SIZE_RELATIVE(frame)) {
                                rel_sum += frame->html.frame_size_s ;
                        }
                }
        }
        return(rel_sum);
}
