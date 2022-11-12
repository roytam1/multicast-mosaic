#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#define DEBUG_IMAGE_PLACE 1

/* Place an image. Add an element record for it. */
void ImagePlace(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext *pcc)
{
	struct ele_rec *eptr;
	int width=0;
	int height = 0;
	int baseline =0;
	ImageInfo * picd;
	AlignType valignment,halignment;

	valignment = VALIGN_BOTTOM;	/* default vert. align for image */
	halignment = HALIGN_NONE;	/* default horiz. align for image */

/* saved work:  got an image in picd */
	picd = mptr->s_picd;

	if (picd->req_border == -1){
		if (pcc->anchor_tag_ptr->anc_href == NULL)
			picd->req_border = picd->border = 0;
		else
			picd->req_border = picd->border = IMAGE_DEFAULT_BORDER;
	} else {
		picd->border = picd->req_border;
	}
/* remarque : remaining field to update : 
 *      fptr = NULL; 
 *      internal_numeo = -1; cw_only = -1;
 */

	picd->fptr = NULL; 
	picd->cw_only = pcc->cw_only;

/* now we have an image. It is :
	- an internal-gopher	(internal=1,fetched=1, delayed =0)
	- a delayed image	(internal=1, fetched=0, delayed=1)
	- a no image		(internal=1, fetched=0, delayed=0)
	- the requested image	(internal=0, fetched=1, delayed=0)
*/

	if( mptr->type == M_INPUT && (pcc->cur_form != NULL) ){
			/* <INPUT type="image" src=url ...> */
		picd->fptr = pcc->cur_form;
	}
	baseline = height = picd->height + 2*picd->border;
	width = picd->width + 2*picd->border;

	valignment = VALIGN_BOTTOM;
	halignment = HALIGN_NONE;
/* Check if this image will be top aligned */
	if (picd->align == VALIGN_TOP) {
		valignment = VALIGN_TOP;
		baseline =0;
	} else if ( picd->align == VALIGN_MIDDLE) {
		valignment = VALIGN_MIDDLE;
		baseline = baseline/2;
	} else if ( picd->align == VALIGN_BOTTOM){
		valignment = VALIGN_BOTTOM;
		/* baseline unchanged */
	} else if ( picd->align == HALIGN_LEFT_FLOAT) {
		halignment = HALIGN_LEFT_FLOAT;
		/* baseline unchanged ######???  */
		baseline =0;
	} else if ( picd->align == HALIGN_RIGHT_FLOAT) {
		halignment = HALIGN_RIGHT_FLOAT;
		baseline =0;
		/* baseline unchanged ######???  */
	}
		
/* Now look if the image is too wide, if so insert a linebreak. */
	if (!pcc->preformat) {
		if ( (pcc->x + width) >
		     (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)) 
			LinefeedPlace(hw, mptr, pcc);
	}
	if (halignment == HALIGN_LEFT_FLOAT) {
		LineBreak( hw, mptr, pcc);
	}

        if(pcc->computed_min_x < (width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = width + pcc->eoffsetx + pcc->left_margin;
        }               
        if (pcc->x + width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + width;

	if (!pcc->cw_only){
		if( mptr->type == M_INPUT && (pcc->cur_form != NULL) ){
			eptr = CreateElement(hw, E_INPUT_IMAGE, pcc->cur_font,
				pcc->x, pcc->y, width, height, baseline, pcc);
			eptr->fptr = picd->fptr;
		} else {
			
			eptr = CreateElement(hw, E_IMAGE, pcc->cur_font,
				pcc->x, pcc->y, width, height, baseline, pcc);
		}
		eptr->underline_number = 0; /* Images can't be underlined! */
		eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
		/* check the max line height. */
		AdjustBaseLine(hw,eptr,pcc);
		eptr->pic_data=picd;
		eptr->bwidth=picd->border ;
		if (halignment == HALIGN_RIGHT_FLOAT ) {
			eptr->x = pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width - width;
		}
	} else {
		if (pcc->cur_line_height < height)
			pcc->cur_line_height = height;
	}

	if (halignment == HALIGN_LEFT_FLOAT ){
		SetFloatAlignLeft(pcc, width, height);
		pcc->have_space_after = 0;
		pcc->is_bol = True;
		return;
	}
	if (halignment == HALIGN_RIGHT_FLOAT ) {
		SetFloatAlignRight(pcc, width, height);
		return;
	}
/* update pcc */
	pcc->have_space_after = 0;
#ifdef DEBUG_IMAGE_PLACE
	fprintf(stderr,"Place Image: pcc->x=%d, width=%d\n",
		pcc->x, width);
#endif
	pcc->x = pcc->x + width ;
	pcc->is_bol = False;
	if (!pcc->preformat) {
		if (pcc->x >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)) 
			LinefeedPlace(hw, mptr, pcc);
	}
}

/*
 * Redraw a formatted image element.
 * The color of the image border reflects whether it is an active anchor or not.
 * Actual Pixmap creation was put off until now to make sure we
 * had a window.  If it hasn't been already created, make the Pixmap now.
 */
void ImageRefresh(HTMLWidget hw, struct ele_rec *eptr)
{
	unsigned long valuemask;
	XGCValues values;
	int x, y, extra,baseline;

	x = eptr->x;
	y = eptr->y;
	baseline = eptr->baseline;
	extra = eptr->bwidth;

	x = x - hw->html.scroll_x;
	y = y - hw->html.scroll_y;
	XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
	XSetBackground(XtDisplay(hw), hw->html.drawGC, eptr->bg);
	if (extra) {
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view), 
				hw->html.drawGC,
				x, y,
				eptr->width, extra);
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view), 
				hw->html.drawGC,
				x, (y + eptr->pic_data->height + extra),
				eptr->width, extra);
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
				hw->html.drawGC,
				x, y,
				extra, (eptr->pic_data->height + (2 * extra)));
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
				hw->html.drawGC,
				(x + eptr->width - extra), y,
				extra, (eptr->pic_data->height + (2 * extra)));
	}

	values.clip_mask=None;
	values.clip_x_origin=0;
	values.clip_y_origin=0;
	valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
	XChangeGC(XtDisplay(hw), hw->html.drawGC, valuemask, &values);

	if (eptr->pic_data->transparent) {
		values.clip_mask=eptr->pic_data->clip;
		values.clip_x_origin=x+extra;
		values.clip_y_origin=y+extra;
		valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
		XChangeGC(XtDisplay(hw), hw->html.drawGC,
			  valuemask, &values);
	} 
	XCopyArea(XtDisplay(hw), eptr->pic_data->image,
		  XtWindow(hw->html.view), hw->html.drawGC,
		  0, 0,
		  eptr->pic_data->width, eptr->pic_data->height,
		  (x + extra), (y + extra));
#ifdef DEBUG_IMAGE_PLACE
	fprintf( stderr, "corner x=%d, y=%d, width=%d, height=%d (extra=%d)\n",
		(x + extra), (y + extra), eptr->pic_data->width,eptr->pic_data->height,extra);
#endif
	values.clip_mask=None;
	values.clip_x_origin=0;
	values.clip_y_origin=0;
	valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
	XChangeGC(XtDisplay(hw), hw->html.drawGC, valuemask, &values);

/*	if((eptr->pic_data.delayed)&&
 *	   (eptr->anchor_tag_ptr->anc_href != NULL)&&
 *	   (!IsIsMapForm(hw, eptr->anchor_tag_ptr->anc_href))) {
 *		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
 *		XFillRectangle(XtDisplay(hw->html.view),
 *			       XtWindow(hw->html.view),
 *			       hw->html.drawGC,
 *			       x, (y + AnchoredImage_height + IMAGE_DEFAULT_BORDER),
 *			       (eptr->width + (2 * extra)), extra);
 *	}
*/
}
