/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>
#include "../libmc/mc_defs.h"
#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "HTMLimages.h"

extern int htmlwTrace;

/*
 * Code to manage a linked list of parsed HTML objects generated
 * from a raw text file.
 * Also code to manage a linked list of formatted elements that
 * make up a page of a formatted document.
 */

/*
 * Free up the passed linked list of parsed elements, freeing
 * all memory associates with each element.
 */
void FreeObjList(struct mark_up *List)
{
	struct mark_up *current;
	struct mark_up *mptr;

	current = List;
	while (current != NULL) {
		mptr = current;
		current = current->next;
		mptr->next = NULL;
		if (mptr->start != NULL)
			free((char *)mptr->start);
		if (mptr->text != NULL)
			free((char *)mptr->text);
		if (mptr->end != NULL)
			free((char *)mptr->end);
		free((char *)mptr);
	}
}

/*
 * Add an object to the parsed object list.
 * return a pointer to the current (end) position in the list.
 * If the object is a normal text object containing nothing but
 * white space, throw it out, unless we have been told to keep
 * white space.
 */
struct mark_up * AddObj( struct mark_up **listp, struct mark_up *current,
	struct mark_up *mark, int keep_wsp)
{
	if (mark == NULL)
		return(current);
	/*
	 * Throw out normal text blocks that are only white space,
	 * unless keep_wsp is set.
	 */
	if ((mark->type == M_NONE)&&(!keep_wsp)) {
		char *ptr;

		ptr = mark->text;
		if (ptr == NULL) {
			free((char *)mark);
			return(current);
		}
/*
 * No longer throw out whitespace, it is important to keep
 * white space between tags.
 */
		if (*ptr == '\0') {
			free(mark->text);
			free((char *)mark);
			return(current);
		}
	}
	/*
	 * Add object to either the head of the list for a new list,
	 * or at the end after the current pointer.
	 */
	if (*listp == NULL) {
		*listp = mark;
		current = *listp;
	} else {
		current->next = mark;
		current = current->next;
	}
	current->next = NULL;
	return(current);
}

/*
 * Convert type number to a printed string for debug
 */
void PrintType( int type)
{
	switch(type) {
	case M_NONE:
		fprintf(stderr,"M_NONE");
		break;
	case M_TITLE:
		fprintf(stderr,"M_TITLE");
		break;
	case M_FIXED:
		fprintf(stderr,"M_FIXED");
		break;
	case M_BOLD:
		fprintf(stderr,"M_BOLD");
		break;
	case M_ITALIC:
		fprintf(stderr,"M_ITALIC");
		break;
	case M_EMPHASIZED:
		fprintf(stderr,"M_EMPHASIZED");
		break;
	case M_STRONG:
		fprintf(stderr,"M_STRONG");
		break;
	case M_CODE:
		fprintf(stderr,"M_CODE");
		break;
	case M_SAMPLE:
		fprintf(stderr,"M_SAMPLE");
		break;
	case M_KEYBOARD:
		fprintf(stderr,"M_KEYBOARD");
		break;
	case M_VARIABLE:
		fprintf(stderr,"M_VARIABLE");
		break;
	case M_CITATION:
		fprintf(stderr,"M_CITATION");
		break;
	case M_STRIKEOUT:
		fprintf(stderr,"M_STRIKEOUT");
		break;
	case M_HEADER_1:
		fprintf(stderr,"M_HEADER_1");
		break;
	case M_HEADER_2:
		fprintf(stderr,"M_HEADER_2");
		break;
	case M_HEADER_3:
		fprintf(stderr,"M_HEADER_3");
		break;
	case M_HEADER_4:
		fprintf(stderr,"M_HEADER_4");
		break;
	case M_HEADER_5:
		fprintf(stderr,"M_HEADER_5");
		break;
	case M_HEADER_6:
		fprintf(stderr,"M_HEADER_6");
		break;
	case M_ANCHOR:
		fprintf(stderr,"M_ANCHOR");
		break;
	case M_PARAGRAPH:
		fprintf(stderr,"M_PARAGRAPH");
		break;
	case M_ADDRESS:
		fprintf(stderr,"M_ADDRESS");
		break;
	case M_PLAIN_TEXT:
		fprintf(stderr,"M_PLAIN_TEXT");
		break;
	case M_LISTING_TEXT:
		fprintf(stderr,"M_LISTING_TEXT");
		break;
	case M_UNUM_LIST:
		fprintf(stderr,"M_UNUM_LIST");
		break;
	case M_NUM_LIST:
		fprintf(stderr,"M_NUM_LIST");
		break;
	case M_MENU:
		fprintf(stderr,"M_MENU");
		break;
	case M_DIRECTORY:
		fprintf(stderr,"M_DIRECTORY");
		break;
	case M_LIST_ITEM:
		fprintf(stderr,"M_LIST_ITEM");
		break;
	case M_DESC_LIST:
		fprintf(stderr,"M_DESC_LIST");
		break;
	case M_DESC_TITLE:
		fprintf(stderr,"M_DESC_TITLE");
		break;
	case M_DESC_TEXT:
		fprintf(stderr,"M_DESC_TEXT");
		break;
	case M_IMAGE:
		fprintf(stderr,"M_IMAGE");
		break;
	case M_SELECT:
		fprintf(stderr,"M_SELECT");
		break;
	case M_OPTION:
		fprintf(stderr,"M_OPTION");
		break;
	case M_INPUT:
		fprintf(stderr,"M_INPUT");
		break;
	case M_TEXTAREA:
		fprintf(stderr,"M_TEXTAREA");
		break;
	case M_FORM:
		fprintf(stderr,"M_FORM");
		break;
	case M_INDEX:
		fprintf(stderr,"M_INDEX");
		break;
	case M_HRULE:
		fprintf(stderr,"M_HRULE");
		break;
	case M_BASE:
		fprintf(stderr,"M_BASE");
		break;
	case M_LINEBREAK:
		fprintf(stderr,"M_LINEBREAK");
		break;
	case M_BLOCKQUOTE:
		fprintf(stderr,"M_BLOCKQUOTE");
		break;
	default:
		fprintf(stderr,"UNKNOWN %d", type);
		break;
	}
}

/* Print the contents of a parsed object list, for debug */
void PrintList( struct mark_up *list)
{
	struct mark_up *mptr;

	if (htmlwTrace) {
		mptr = list;
		while (mptr != NULL) {
			PrintType(mptr->type);
			if (mptr->is_end) {
				fprintf(stderr," END");
			} else {
				fprintf(stderr," START");
			}
			if (mptr->text != NULL) {
				fprintf(stderr,"\n{\n\t");
				fprintf(stderr,"%s", mptr->text);
				fprintf(stderr,"}\n");
			} else {
				fprintf(stderr,"\n");
			}
			mptr = mptr->next;
		}
	}
}

/*
 * Used to find the longest line (in characters) in a collection
 * of text blocks.  cnt is the running count of characters, and
 * txt is the pointer to the current text block.  Since we are
 * finding line widths, a newline resets the width count.
 */
char * MaxTextWidth(char *txt, int *cnt)
{
	char *start;
	char *end;
	int width;

	if (txt == NULL)
		return(NULL);
	width = *cnt;
	start = txt;
	/*
	 * If this blocks starts with a newline, reset the width
	 * count, and skip the newline.
	 */
	if (*start == '\n') {
		width = 0;
		start++;
	}
	end = start;
	/*
	 * count characters, stoping either at a newline, or at the
	 * end of this text block.  Expand tabs.
	 */
	while ((*end != '\0')&&(*end != '\n')) {
		if (*end == '\t') {
			width = ((width / 8) + 1) * 8;
		} else {
			width++;
		}
		end++;
	}
	*cnt = width;
	return(end);
}
static void send_clientmessage (Display * dpy, Window w, Atom p, Atom a, 
				Time timestamp)
{
    XClientMessageEvent ev;
 
    ev.type = ClientMessage;
    ev.window = w;
    ev.message_type = p;
    ev.format = 32;
    ev.data.l[0] = a;
    ev.data.l[1] = timestamp;
    XSendEvent (dpy, w, True, 0x00ffffff, (XEvent *) &ev);
}
 


/*
 * Free up the passed linked list of formatted elements, freeing
 * all memory associates with each element.
 */
void FreeLineList( struct ele_rec *list, Widget w, Boolean save_obj)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec *current;
	struct ele_rec *eptr;

	current = list;
	while (current != NULL) {
		eptr = current;
		current = current->next;
		if (eptr->edata != NULL) {
			free((char *)eptr->edata);
			eptr->edata = NULL;
		}
		/*############*/
		eptr->anchor_tag_ptr = NULL_ANCHOR_PTR;
		if((eptr->type == E_IMAGE) && eptr->pic_data->fetched ) {
                        /*
                         * Don't free the no_image default image
                         */
                         /*
                          * Don't free internal images
                          */
			if((eptr->pic_data->image != (Pixmap)NULL)&&
			   (eptr->pic_data->image != delayed_image.image)&&
			   (eptr->pic_data->image != anchored_image.image)&&
			   (eptr->pic_data->image != no_image.image)&&
			   (eptr->pic_data->internal != 1) ){
				XFreePixmap(XtDisplay(w), eptr->pic_data->image);
				eptr->pic_data->image = (Pixmap)NULL;
				if (eptr->pic_data->clip != (Pixmap)NULL) {
					XFreePixmap(XtDisplay(w),
						eptr->pic_data->clip);
					eptr->pic_data->clip = (Pixmap)NULL;
				}
				/* don't free because cache do it */
				eptr->pic_data->reds = NULL;
				eptr->pic_data->greens = NULL;
				eptr->pic_data->blues = NULL;
				eptr->pic_data->image_data = NULL;
				eptr->pic_data->clip_data=NULL;
				/* src is canonized by cache */
				eptr->pic_data->src = NULL;
			}
		}
		if((eptr->type == E_APROG) && eptr->aprog_struct && eptr->aprog_struct->frame){
			static Atom delete_atom = 0;
			static Atom proto_atom = 0;

			if (!delete_atom)
        			delete_atom = XInternAtom(XtDisplay(w),
						"WM_DELETE_WINDOW", False);
			if(!proto_atom)
        			proto_atom = XInternAtom(XtDisplay(w),
						"WM_PROTOCOLS", False);
			if (save_obj == False){
        			send_clientmessage(XtDisplay(w), 
					XtWindow(eptr->aprog_struct->frame),
					proto_atom, delete_atom, CurrentTime);
				XFlush(XtDisplay(w));
				XtSetMappedWhenManaged(eptr->aprog_struct->frame,
					False);
				XFlush(XtDisplay(w));
				XtDestroyWidget(eptr->aprog_struct->frame);
			}
/*##### need lot of other stuff to free the allocated struct ######### */

		}
#ifdef TODO
/*        	if(eptr->pic_data->fptr != NULL) {#########don't know what to do ##### */
/*			typedef struct form_rec {
*        			Widget hw;
*        			char *action;
*        			char *method;
*        			char *enctype;
*        			char *enc_entity;
*        			char *format;
*        			int start, end;
*			        Widget button_pressed;
*			        struct form_rec *next;
*			} FormInfo;
*		}
*/
#endif
        	if(eptr->widget_data != NULL) {
/*			typedef struct wid_rec {
*        			Widget w;
*        			int type;
*        			int id;
*        			int x, y;
*        			int width, height;
*        			char *name;
*        			char *value;
*        			char *password;
*        			char **mapping;
*        			Boolean checked;
*        			Boolean mapped;
*        			struct wid_rec *next;
*        			struct wid_rec *prev;
*			} WidgetInfo;
 */
		}
        	if(eptr->table_data != NULL) {
/*			typedef struct table_field {
*				int colSpan; 
*				int rowSpan;
*				Boolean contVert;
*				Boolean contHoriz;
*				int alignment;   
*				int maxWidth;
*				int minWidth;
*				int maxHeight;
*				int minHeight;
*				int colWidth;   
*				int rowHeight; 
*				Boolean header;
*				FieldType       type;
*				char            *text;
*				XFontStruct     *font;
*				char            **formattedText;
*				int             numLines;
*				ImageInfo       *image;
*				WidgetInfo      *winfo;
*			} TableField;
* 
*			typedef struct table_rec {
*				Boolean borders;
*				char    *caption;
*				int     captionAlignment;
*				struct  mark_up *mptr;
*				int     width,height;
*				int     bwidth,bheight;
*				int     numColumns;
*				int     numRows;
*				TableField *table;
*			} TableInfo;
 */
		}
        	if(eptr->font != NULL) {
/*			XFontStruct *font;
 */
		}
/*###################################*/
		eptr->next = NULL;
		eptr->prev = NULL;
		free((char *)eptr);
	}
        /* Now take care of the background image! -- SWP */
        if (hw->html.bgmap_SAVE!=None) {  
                XFreePixmap(XtDisplay(hw),
                            hw->html.bgmap_SAVE);
                hw->html.bgmap_SAVE=None; 
        }                            
        if (hw->html.bgclip_SAVE!=None) { 
                XFreePixmap(XtDisplay(hw),
                            hw->html.bgclip_SAVE);
                hw->html.bgclip_SAVE=None;
        }
}

/*
 * Contruct and return an array of pointers into the element list that
 * indexes the elements by line number.
 * Note, lines containing only while space will have NULL pointers
 * into the element list.
 */
struct ele_rec ** MakeLineList(struct ele_rec *elist, int max_line)
{
	int i;
	struct ele_rec *teptr;
	struct ele_rec *eptr;
	struct ele_rec **ll;

					/* malloc index array */
	ll = (struct ele_rec **)malloc(sizeof(struct ele_rec *) * max_line);
	if (ll == NULL) {
		fprintf(stderr, "cannot allocate space for line list\n");
		exit(1);
	}
	for (i=0; i<max_line; i++) 	/* zero the index array */
		ll[i] = NULL;
	eptr = elist;		/* fill in pointers to beginning of the lines */
	while (eptr != NULL) {
		if (eptr->line_number > max_line)
			break;
		if (ll[eptr->line_number - 1] == NULL){
			ll[eptr->line_number - 1] = eptr;
			ll[eptr->line_number - 1]->line_next = NULL;
		}else{
			teptr = ll[eptr->line_number - 1];
			eptr->line_next = teptr;
			ll[eptr->line_number - 1] = eptr;
		}
		eptr = eptr->next;
	}
	return(ll);
}

/*
 * Passed in 2 element pointers, and element positions.
 * Function should return 1 if if start occurs before end.
 * Otherwise return 0.
 */
int ElementLessThan( struct ele_rec *start, struct ele_rec *end,
	int start_pos, int end_pos)
{
	struct ele_rec *current;

	/*
	 * Deal with start or end being NULL
	 */
	if ((start == NULL)&&(end == NULL))
		return(0);
	if ((start == NULL)&&(end != NULL))
		return(1);
	if ((start != NULL)&&(end == NULL))
		return(0);
	/*
	 * Deal with easy identical case
	 */
	if (start == end) {
		if (start_pos < end_pos)
			return(1);
		return(0);
	}
	/* We know element Ids are always equal or increasing within a list.*/
	if (start->ele_id < end->ele_id)
		return(1);
	if (start->ele_id == end->ele_id) {
		current = start;
		while (current != NULL) {
			if (current->ele_id != start->ele_id)
				break;
			if (current == end)
				break;
			current = current->next;
		}
		if (current == end)
			return(1);
		return(0);
	}
	return(0);
}

/*
 * Passed in 2 element pointers, and element positions.
 * Function should return 1 if they need to be swapped in order for then
 * to proceed left to right and top to bottom in the text.
 * Otherwise return 0.
 */
int SwapElements( struct ele_rec *start, struct ele_rec *end, 
		int start_pos, int end_pos)
{
	struct ele_rec *current;

	/* Deal with start or end being NULL */
	if ((start == NULL)&&(end == NULL))
		return(0);
	if ((start == NULL)&&(end != NULL))
		return(1);
	if ((start != NULL)&&(end == NULL))
		return(0);
	/* Deal with easy identical case */
	if (start == end) {
		if (start_pos > end_pos)
			return(1);
		return(0);
	}

	/* We know element Ids are always equal or increasing within a list. */
	if (start->ele_id < end->ele_id)
		return(0);
	if (start->ele_id == end->ele_id) {
		current = start;
		while (current != NULL) {
			if (current->ele_id != start->ele_id)
				break;
			if (current == end)
				break;
			current = current->next;
		}
		if (current == end)
			return(0);
		return(1);
	}
	return(1);
}

/* Free up the allocated list of internal hrefs. */
void FreeHRefs(struct ref_rec *list)
{
	struct ref_rec *hptr;
	struct ref_rec *tptr;

	hptr = list;
	while (hptr != NULL) {
		tptr = hptr;
		hptr = hptr->next;
		if (tptr->anchorHRef != NULL)
			free((char *)tptr->anchorHRef);
		free((char *)tptr);
	}
}

/*
 * Find an element in the linked list of Internal HREFS.
 * return a pointer to the element, or NULL if not found.
 */
struct ref_rec * FindHRef( struct ref_rec *list, char *href)
{
	struct ref_rec *hptr;

	if (href == NULL)
		return(NULL);
	hptr = list;
	while (hptr != NULL) {
		if (strcmp(hptr->anchorHRef, href) == 0)
			break;
		hptr = hptr->next;
	}
	return(hptr);
}


/*
 * Add an element to the linked list of Internal HREFS we
 * have visited before.
 * return a pointer to the head of the new list.
 */
struct ref_rec * AddHRef(struct ref_rec *list, char *href)
{
	struct ref_rec *hptr;

	if (href == NULL)
		return(list);

	hptr = FindHRef(list, href);

	if (hptr == NULL) {
		hptr = (struct ref_rec *)malloc(sizeof(struct ref_rec));
		if (hptr == NULL) {
			fprintf(stderr, "cannot extend internal href list\n");
			return(list);
		}
		hptr->anchorHRef = (char *)malloc(strlen(href) + 1);
		if (hptr->anchorHRef == NULL) {
			free((char *)hptr);
			fprintf(stderr, "cannot extend internal href list\n");
			return(list);
		}
		strcpy(hptr->anchorHRef, href);
		hptr->next = list;
		list = hptr;
	}
	return(list);
}


/*
 * Free up the allocated list of visited delayed images
 */
void FreeDelayedImages(struct delay_rec *list)
{
	struct delay_rec *iptr;
	struct delay_rec *tptr;

	iptr = list;
	while (iptr != NULL) {
		tptr = iptr;
		iptr = iptr->next;
		if (tptr->src != NULL)
			free((char *)tptr->src);
		free((char *)tptr);
	}
}

/*
 * Find an element in the linked list of visited delayed images.
 * return a pointer to the element, or NULL if not found.
 */
struct delay_rec * FindDelayedImage(struct delay_rec *list, char *src)
{
	struct delay_rec *iptr;

	if (src == NULL)
		return(NULL);

	iptr = list;
	while (iptr != NULL) {
		if (strcmp(iptr->src, src) == 0)
			break;
		iptr = iptr->next;
	}
	return(iptr);
}

/*
 * Add an element to the linked list of visited delayed images.
 * return a pointer to the head of the new list.
 */
struct delay_rec * AddDelayedImage(struct delay_rec *list, char *src)
{
	struct delay_rec *iptr;

	if (src == NULL)
		return(list);
	iptr = FindDelayedImage(list, src);
	if (iptr == NULL) {
		iptr = (struct delay_rec *)malloc(sizeof(struct delay_rec));
		if (iptr == NULL) {
			fprintf(stderr,"cant extend visited delayed ima list\n");
			return(list);
		}
		iptr->src = (char *)malloc(strlen(src) + 1);
		if (iptr->src == NULL) {
			free((char *)iptr);
			fprintf(stderr,"cant extend visited delayed ima list\n");
			return(list);
		}
		strcpy(iptr->src, src);
		iptr->next = list;
		list = iptr;
	}
	return(list);
}
