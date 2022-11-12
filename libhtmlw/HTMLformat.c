/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Some part of this file is Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <ctype.h>

#include <Xm/Frame.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "../src/mosaic.h"

#include "HTMLform.h"
#include "HTMLframe.h"


static struct mark_up NULL_ANCHOR = {
	M_ANCHOR,		/* MarkType */
	1,			/* is_end */
	NULL,			/* start */
	NULL,			/* text */
	0,			/* is_white_text */
	NULL,			/* end */
	NULL,			/* next */
	NULL,			/* s_aps */
	NULL,			/* s_ats */
	NULL,			/* s_picd */
	NULL,			/* t_p1 */
	NULL,			/* anchor_name */
	NULL,			/* anchor_href */
	NULL			/* anchor_title */
};

struct mark_up * NULL_ANCHOR_PTR = &NULL_ANCHOR ;

/* ############## This is may be as global ####*/
static int InDocHead;
static int in_title;

/* ##############  This is maybe in a context stack ####*/
static DescRec BaseDesc;
static DescRec *DescType;
static MapInfo *CurrentMap=NULL; /* csi stuff -- swp */
/* ############ */

/* GD ########## */
/* Set the formatted element into the format list. 
 * If hw->html.cur_elem_to_format is initialised to NULL each time
 * we reformat the widget, CreateElement alway create and elem. at end.
 */
struct ele_rec * CreateElement( HTMLWidget hw, ElementType type, XFontStruct *fp,
	int x, int y, int width, int height, int baseline,
	PhotoComposeContext * pcc)
{
	struct ele_rec *eptr;

	if (pcc->cw_only)
		return NULL;	/* just for check ######## */
/* There is not pre-allocated format list, or we have reached
 * the end of the pre-allocated list.  Create a new element, and add it.
 */
	if (hw->html.formatted_elements == NULL){ /* the first element */
						/* create it */
		eptr = (struct ele_rec *) malloc( sizeof(struct ele_rec));
		CHECK_OUT_OF_MEM(eptr);
		hw->html.formatted_elements = eptr;
		memset(eptr, 0, sizeof(struct ele_rec)); /* initialize */
		eptr->next =NULL;
		eptr->prev =NULL;
		hw->html.cur_elem_to_format = eptr;
		hw->html.last_formatted_elem = eptr;
	} 
	/* work now with the current element to format */
	eptr = hw->html.cur_elem_to_format;
	if ( eptr == NULL) { /* no current element , add one at end */
		eptr = (struct ele_rec *) malloc( sizeof(struct ele_rec));
		CHECK_OUT_OF_MEM(eptr);
		eptr->next = NULL;
		eptr->prev = hw->html.last_formatted_elem;
		hw->html.last_formatted_elem->next = eptr;
		hw->html.last_formatted_elem = eptr;
	}
	hw->html.cur_elem_to_format = NULL;

	/* Now we work with 'eptr' and start the stuff */

	eptr->type = type;
	eptr->font = fp;
	eptr->x = x;
	eptr->y = y;
	eptr->width = width;
	eptr->height = height;
	eptr->baseline = baseline;
	eptr->ele_id = ++(pcc->element_id);

	eptr->pic_data = NULL;
	eptr->aps = NULL;
	eptr->widget_data = NULL;
	eptr->table_data = NULL;
	eptr->ats = NULL;

	eptr->line_next = NULL;

        eptr->internal_numeo = 0;
	eptr->valignment = VALIGN_BOTTOM;
	eptr->halignment = HALIGN_LEFT;
	eptr->selected = False;
	eptr->indent_level = pcc->indent_level;
	eptr->start_pos = 0;
	eptr->end_pos = 0;
	eptr->bwidth = IMAGE_DEFAULT_BORDER;
	eptr->underline_number = pcc->underline_number;
	eptr->dashed_underline = pcc->dashed_underlines;
	eptr->strikeout = pcc->strikeout;
	eptr->fg = pcc->fg;
	eptr->bg = pcc->bg;
        eptr->anchor_tag_ptr = NULL_ANCHOR_PTR;	/* putit in struct markup##### */
        eptr->edata = NULL;
        eptr->edata_len = 0;
        		/* eptr->next is set when allocate */
        		/* eptr->prev is set when allocate */

	eptr->is_in_form = 0;
	return eptr;
}
/* end GD ###### */

void AdjustBaseLine(HTMLWidget hw,struct ele_rec *eptr,
		PhotoComposeContext * pcc)
{
	int cur_baseline;
	int ele_baseline;
	int ele_height;
	int cur_line_height;
	int dbasel;
	struct ele_rec *septr;
	int supsubBaseline;

	cur_baseline = pcc->cur_baseline;
	ele_baseline = eptr->baseline;
	cur_line_height = pcc->cur_line_height;
	ele_height = eptr->height;

        if ((pcc->superscript>0) || (pcc->subscript>0)) {
		supsubBaseline = hw->html.cur_font->max_bounds.ascent;
		cur_baseline -= ((supsubBaseline * .4) * pcc->superscript);
		cur_baseline += ((supsubBaseline * .4) * pcc->subscript);
		cur_baseline += 2;
        }

	if ( ele_baseline <= cur_baseline ) { /* adjust baseline element */
					     /* compute the new height element */ 
					     /* of BB */
		dbasel = cur_baseline - ele_baseline;
		ele_height = ele_height + dbasel;
		eptr->baseline = cur_baseline;
		if(ele_height <= cur_line_height){ /* adjust the ele */
			eptr->height = cur_line_height;
			return;
		} else {		/* adjust previous ele height */
			eptr->height = ele_height;
			pcc->cur_line_height = ele_height;
			while(eptr->prev != NULL){
				eptr = eptr->prev;
				eptr->height = ele_height;
				if( eptr->type == E_CR)
					break;
			}
			return;
		}
	}

	/* ele_baseline > cur_baseline */
	eptr->baseline = ele_baseline;
	pcc->cur_baseline = ele_baseline;
	dbasel = ele_baseline - cur_baseline;
	cur_line_height = cur_line_height + dbasel;
	septr = eptr;
	while(septr->prev != NULL){	/* adjust all ele base line */
		septr = septr->prev;
		septr->baseline = ele_baseline;  
		septr->height = cur_line_height;  
		if( septr->type == E_CR)
			break;
	}
	if(cur_line_height < ele_height){	/* adjust prev ele height */
		eptr->height = ele_height;
		pcc->cur_line_height = ele_height; 
		septr = eptr;
		while(septr->prev != NULL){
			septr = septr->prev;
			septr->height = ele_height;  
			if( septr->type == E_CR) 
				break;
		}
		return; 
	} else {
		eptr->height = cur_line_height;
		pcc->cur_line_height = cur_line_height;
	}
}

/* Place the number at the beginning of an numbered
 * list item. Create and add the element record for it.
 */
void ListNumberPlace( HTMLWidget hw, PhotoComposeContext * pcc, int val)
{
	int width;
	int dir, ascent, descent;
	XCharStruct all;
	char buf[20];
	int x= pcc->x;
	int y = pcc->y;
	struct ele_rec * eptr;
	int font_height;
	int baseline;

	sprintf(buf, "%d.", val);
	x = x - D_INDENT_SPACES + D_INDENT_SPACES /3 ;
	XTextExtents(pcc->cur_font, buf, strlen(buf), &dir,
		&ascent, &descent, &all);
	width = all.width;
	font_height = pcc->cur_font->ascent +pcc->cur_font->descent;
	baseline = pcc->cur_font->ascent;

	if (!pcc->cw_only){
		eptr = CreateElement(hw, E_TEXT,pcc->cur_font,
                                x, pcc->y,
                                width, font_height,baseline,pcc);
        	Set_E_TEXT_Element(hw, eptr, buf,pcc);
		AdjustBaseLine(hw,eptr,pcc);
	} else {
	        if (pcc->cur_line_height < font_height)
                        pcc->cur_line_height = font_height;
        }
	pcc->have_space_after = 0;
	pcc->is_bol =1;
	pcc->x = pcc->x ;
}

/* Horrible code for the TEXTAREA element.  Escape '\' and ''' by
 * putting a '\' in front of them, then replace all '"' with '''.
 * This lets us safely put the resultant value between double quotes.
 */
char * TextAreaAddValue( char *value, char *text)
{
	int extra;
	char *buf;
	char *bptr;
	char *tptr;

	if ((text == NULL)||(text[0] == '\0'))
		return(value);

	extra = 0;
	tptr = text;
	while (*tptr != '\0') {
		if ( (*tptr == '\\') || (*tptr == '\'') )
			extra++;
		tptr++;
	}

	buf = (char *)malloc(strlen(value) + strlen(text) + extra + 1);
	CHECK_OUT_OF_MEM(buf);
	strcpy(buf, value);

	tptr = text;
	bptr = (char *)(buf + strlen(value));
	while (*tptr != '\0') {
		if ((*tptr == '\\')||(*tptr == '\'')) {
			*bptr++ = '\\';
			*bptr++ = *tptr++;
		}
		else if (*tptr == '\"')
		{
			*bptr++ = '\'';
			tptr++;
		} else {
			*bptr++ = *tptr++;
		}
	}
	*bptr = '\0';
	free(value);
	return(buf);
}

/*
 * Make necessary changes to formatting, based on the type of the
 * parsed HTML text we are formatting.
 * Some calls create elements that are added to the formatted element list.
 */
static void TriggerMarkChanges(HTMLWidget hw, struct mark_up **mptr,
		PhotoComposeContext * pcc, Boolean save_obj)
{
	struct mark_up *mark;
	int type;
	struct mark_up mark_tmp;
	char *tptr;

	mark = *mptr;
	type = mark->type;

	/* If we are not in a tag that belongs in the HEAD, end the HEAD
	   section  - amb */
	if (in_title && (type != M_TITLE)) {
		return;
	}
/* some marker other than M_NONE M_BASE M_ISINDEX M_COMMENT finish the HEAD */
	if (InDocHead) {
		if((type != M_NONE)&&(type != M_BASE)&&
		   (type != M_ISINDEX)&&(type != M_COMMENT)) {
			pcc->ignore = 0;
			InDocHead = 0;
	  	}
	}
	/* If pcc->ignore is set, we ignore all further elements until we get to the
	 * end of the pcc->ignore
	 * Let text through so we can grab the title text.
	 * Let title through so we can hit the end title.
	 * Now also used for SELECT parseing
	 * Let SELECT through so we can hit the end SELECT.
	 * Let OPTION through so we can hit the OPTIONs.
	 * Let TEXTAREA through so we can hit the TEXTAREAs.
	 */
	if ((pcc->ignore)&&(!InDocHead)&&(type != M_NONE)&&
		(type != M_SELECT)&&(type != M_OPTION)&&
		(type != M_TEXTAREA)&&(type != M_HEAD))
		        return;

	switch(type) {
	/*
	 * Place the text.  Different functions based on whether it
	 * is pre-formatted or not.
	 */
	case M_NONE:
		if ((pcc->ignore)&&(pcc->current_select != NULL)) {
			if (pcc->current_select->option_buf != NULL) {
				tptr = (char *)malloc(strlen(
					pcc->current_select->option_buf) +
					       strlen((*mptr)->text) + 1);
				strcpy(tptr, pcc->current_select->option_buf);
				strcat(tptr, (*mptr)->text);
				free(pcc->current_select->option_buf);
				pcc->current_select->option_buf = tptr;
			}
			break;
		}
		if ((pcc->ignore)&&(pcc->text_area_buf != NULL)) {
			pcc->text_area_buf = TextAreaAddValue(pcc->text_area_buf,(*mptr)->text);
			break;
		}
		if (pcc->preformat) {
			PartOfPreTextPlace(hw, *mptr, pcc);
			break;
		} 
		PartOfTextPlace(hw, *mptr, pcc);
		break;
	case M_CENTER:
		if (mark->is_end) {
			LineBreak(hw,*mptr,pcc);
			pcc->div = DIV_ALIGN_LEFT;
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			pcc->div = DIV_ALIGN_CENTER;
		}
		break;
/*
 * Just insert a linefeed, or ignore if this is prefomatted
 * text because the <P> will be followed be a linefeed.
 */
	case M_PARAGRAPH:
		if (mark->is_end) {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			pcc->div = DIV_ALIGN_LEFT;
			pcc->is_in_paragraph = False;
		} else {
			if (pcc->is_in_paragraph){ /* end the paragraph */
				LineBreak(hw,*mptr,pcc);
				LinefeedPlace(hw,*mptr,pcc);
				pcc->div = DIV_ALIGN_LEFT;
				pcc->is_in_paragraph = False;
			}
			LineBreak(hw,*mptr,pcc);
			pcc->div = DIV_ALIGN_LEFT;
			tptr = ParseMarkTag(mark->start, MT_PARAGRAPH, "ALIGN");
			if ( tptr && !strcasecmp(tptr, "CENTER"))
				pcc->div = DIV_ALIGN_CENTER;
			if ( tptr && !strcasecmp(tptr, "RIGHT"))
				pcc->div = DIV_ALIGN_RIGHT;
			if(tptr)
				free(tptr);
			pcc->is_in_paragraph = True;
		}
		break;

	case M_HRULE:
		LineBreak(hw,*mptr,pcc);
		LinefeedPlace(hw,*mptr,pcc);
		HRulePlace(hw, *mptr, pcc);
		LinefeedPlace(hw,*mptr,pcc);
		break;
	/*
	 * Titles are just set into the widget for retrieval by
	 * XtGetValues().
	 */
	case M_TITLE:
		if (mark->is_end) {
			in_title = 0;
		} else {
			in_title =1;
		}
		break;
/* Strikeout means draw a line through the text.
 * Right now we just set a boolean flag which gets shoved
 * in the element record for all elements in the
 * strikeout zone.
 */
	case M_STRIKEOUT:
		pcc->strikeout = True;
		if (mark->is_end)
			pcc->strikeout = False;
		break;
/* Formatting commands just change the current font. */
/*from XmHTML :
* <font> is a big performance hit. We always need to push & pop 
* the font *even* if only the font color has been changed as we
* can't keep track of what has actually been changed.
*****/
	case M_FONT:
	case M_CODE:
	case M_SAMPLE:
	case M_KEYBOARD:
	case M_FIXED:
	case M_STRONG:
	case M_BOLD:
	case M_EMPHASIZED:
	case M_VARIABLE:
	case M_CITATION:
	case M_ITALIC:
	case M_SMALL:
	case M_BIG:
		if (mark->is_end) {
			MMPopFont(hw, *mptr, pcc);
		} else {
			MMPushFont(hw, *mptr, pcc);
		}
		break;

/* Addresses are just like headers.  A linefeed before and
 * after, and change the font.
 */
	case M_ADDRESS:
		LinefeedPlace(hw,*mptr,pcc);
		if (mark->is_end) {
			MMPopFont(hw, *mptr, pcc);
		} else {
			MMPushFont(hw, *mptr, pcc);
		}
		break;
/* Plain and listing text. A single pre-formatted chunk of text in its own font.*/
	case M_PREFORMAT:
		if (mark->is_end) {
			LineBreak(hw,*mptr,pcc);
			pcc->preformat = 0;
			MMPopFont(hw, *mptr, pcc);
			LinefeedPlace(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			pcc->preformat = 1;
			MMPushFont(hw, *mptr, pcc);
		}
		break;
/* Headers are preceeded by a line feed if text before.
 * and followed by a linefeed.
 */
	case M_HEADER_1:
	case M_HEADER_2:
	case M_HEADER_3:
	case M_HEADER_4:
	case M_HEADER_5:
	case M_HEADER_6:
		if (mark->is_end) {
			MMPopFont(hw, *mptr, pcc);
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
		} else {
			if (pcc->is_in_paragraph){ /* end the paragraph */
				LineBreak(hw,*mptr,pcc);
				LinefeedPlace(hw,*mptr,pcc);
				pcc->div = DIV_ALIGN_LEFT;
				pcc->is_in_paragraph = False;
			}

			LineBreak(hw,*mptr,pcc);
			MMPushFont(hw, *mptr, pcc);
		}
		break;

/*#############
        case M_SUP:

                if (mark->is_end) {
                       pcc->superscript--;
                       if ((pcc->superscript==0) && (pcc->subscript==0))
                             pcc->cur_font = MMPopFont();
                 } else {
                       pcc->superscript++;
                       if ((pcc->superscript==1) && (pcc->subscript==0)) {
                              nonScriptFont=pcc->cur_font;
                              MMPushFont(pcc->cur_font);
                              pcc->cur_font = hw->html.supsub_font;
                        }
                }

                break;
	case M_SUB:
		if (mark->is_end) {
                        pcc->subscript--;
                        if ((pcc->subscript==0) && (pcc->superscript==0))
                                pcc->cur_font = MMPopFont();
                } else {
                        pcc->subscript++;
                        if ((pcc->subscript==1) && (pcc->superscript==0)) {
                        	nonScriptFont=pcc->cur_font;
                        	MMPushFont(pcc->cur_font);
                        	pcc->cur_font = hw->html.supsub_font;
                        }
                }
                break;
##############*/
/* amb - ignore text inside a HEAD element */
	case M_HEAD:
		InDocHead = 1;
		pcc->ignore = 1;
		if (mark->is_end) {
		        InDocHead = 0;
			pcc->ignore = 0;
		}
		break;
	case M_FRAMESET:
		_XmHTMLCreateFrameSet(hw, hw, mptr, pcc);
		break;
	case M_BODY:
		if (!mark->is_end) {
/*##### */
			static char *atts[]={"text","bgcolor","alink","vlink","link",NULL};
			char *tmp=NULL;
			int i;

			if (hw->html.body_colors) {
				for(i=0;atts[i];i++) {
					tmp=ParseMarkTag(mark->start,
						MT_BODY,atts[i]);
					if (tmp) {
						hw_do_color(hw,atts[i],tmp,pcc);
						free(tmp);
						tmp=NULL;
					}
				}
			}
			if (mark->s_picd) {
				hw_do_bg(hw,pcc,mark);
			}
/*##### */
		        InDocHead = 0;   /* end <head> section */
			pcc->ignore = 0;
		}
		break;
	case M_UNDERLINED:
		pcc->underline_number = 1;
		pcc->in_underlined = 1;
		if (mark->is_end) {
		    pcc->underline_number = 0;
		    pcc->in_underlined = 0;
		}
		break;

	/*
	 * Anchors change the text color, and may set
	 * underlineing attributes.
	 * No linefeeds, so they can be imbedded anywhere.
	 */
	case M_ANCHOR:
		if (mark->is_end) {
			pcc->fg = hw->manager.foreground;
			pcc->underline_number = pcc->in_underlined;
			pcc->dashed_underlines = False;
			pcc->anchor_tag_ptr = NULL_ANCHOR_PTR;
			break;
		}

/* Only change the color of anchors with HREF tags, 
 * because other anchors are not active.
 */
		pcc->anchor_tag_ptr = *mptr;
		tptr = (*mptr)->anc_href = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_HREF);
		(*mptr)->anc_name = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_NAME);
		(*mptr)->anc_title = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_TITLE);
		(*mptr)->anc_target = ParseMarkTag(mark->start,
					MT_ANCHOR, AT_TARGET);

		if (tptr != NULL) { 
		    	pcc->fg = hw->html.anchor_fg;
		    	pcc->underline_number = hw->html.num_anchor_underlines;
		    	pcc->dashed_underlines = hw->html.dashed_anchor_lines;
/* we may want to send the href back somewhere else and
 * find out if we've visited it before */
		        if (hw->html.previously_visited_test != NULL) {
			    if((*(visitTestProc)
			      (hw->html.previously_visited_test)) ((Widget)hw, tptr,hw->html.base_url)) {
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
		break;
/* Blockquotes increase the margin width. They cannot be nested. */
	case M_BLOCKQUOTE:
		if (mark->is_end) {
			pcc->left_margin = pcc->left_margin - hw->html.margin_width;
			LineBreak(hw,*mptr,pcc);
			pcc->x = pcc->left_margin;
			LinefeedPlace(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			pcc->left_margin = pcc->left_margin + hw->html.margin_width;
			LinefeedPlace(hw,*mptr,pcc);
			pcc->x = pcc->left_margin;
		}
		break;

/* Can only be inside a SELECT tag. */
	case M_OPTION:
		if (mark->is_end)	/* no end mark on <option> */
			return;
		FormSelectOptionField( hw, mptr, pcc);
		break;

/* Special INPUT tag. */
	case M_SELECT:
		if (mark->is_end) {
			FormSelectEnd(hw,mptr,pcc);
		} else {
			FormSelectBegin(hw,mptr,pcc);
		}
		break;

	case M_TEXTAREA:
		if (mark->is_end) {
			FormTextAreaEnd(hw, mptr, pcc);
		}else {
			FormTextAreaBegin(hw, mptr, pcc);
		}
		break;

/* Just insert the widget. */
	case M_INPUT:
		if (mark->is_end)	/* no end mark on <input> */
			return;
		FormInputField( hw, mptr, pcc);
		break;

/* Fillout forms.  Cannot be nested. */
	case M_FORM:
		if (mark->is_end) {
			EndForm  (hw, mptr, pcc);
		} else {
			BeginForm(hw, mptr, pcc);
		}
		break;

	/*
	 * Numbered lists, Unnumbered lists, Menus.
	 * Currently also lump directory listings into this.
	 * Save state for each indent level.
	 * Change the value of the TxtIndent (can be nested)
	 * Linefeed at the end of the list.
	 */
	case M_NUM_LIST:
	case M_UNUM_LIST:
	case M_MENU:
	case M_DIRECTORY:
		if (mark->is_end) {
/* restore the old state if there is one */
			if (DescType->next != NULL) {
				DescRec *dptr;

				dptr = DescType;
				DescType = DescType->next;
				pcc->left_margin = dptr->save_left_margin;
				pcc->cur_line_width = dptr->save_cur_line_width;
				pcc->indent_level--;
				LineBreak(hw,*mptr,pcc);
				LinefeedPlace(hw,*mptr,pcc);
				free((char *)dptr);
			}
		} else {
			DescRec *dptr;

			LineBreak(hw,*mptr,pcc);
			if (pcc->is_in_paragraph){ /* end the paragraph */
				LinefeedPlace(hw,*mptr,pcc);
				pcc->div = DIV_ALIGN_LEFT;
				pcc->is_in_paragraph = False;
			}
			dptr = (DescRec *)malloc(sizeof(DescRec));
/* Save the old state, and start a new */
			dptr->compact = 0;
			pcc->indent_level++;
			if (type == M_NUM_LIST) {
				dptr->type = D_OLIST;
				dptr->count = 1;
			} else {
				dptr->type = D_ULIST;
				dptr->count = 0;
			}
			if(DescType->next != NULL){
				pcc->left_margin =DescType->indent_margin;
				pcc->cur_line_width =DescType->cur_line_width;
			}
			dptr->save_left_margin = pcc->left_margin;
			dptr->indent_margin = dptr->save_left_margin +
					D_INDENT_SPACES ;
			dptr->save_cur_line_width = pcc->cur_line_width;
			dptr->cur_line_width = dptr->save_cur_line_width -
					D_INDENT_SPACES ;
			if (dptr->cur_line_width <=0){
				dptr->cur_line_width = dptr->save_cur_line_width;
			}
			dptr->next = DescType;
			DescType = dptr;
		}
		pcc->x = pcc->eoffsetx + pcc->left_margin;
		pcc->is_bol =1;
		break;
/* Place the bullet element at the beginning of this item. */
	case M_LIST_ITEM:
		if (!mark->is_end) {
			LineBreak(hw,*mptr,pcc);
/* for ordered/numbered lists put numbers in place of bullets. */
			pcc->left_margin= DescType->indent_margin;
			pcc->cur_line_width= DescType->cur_line_width;
			pcc->x = pcc->eoffsetx + pcc->left_margin;
			pcc->is_bol =1;
			if (DescType->type == D_OLIST) {
				ListNumberPlace(hw,  pcc, DescType->count);
				DescType->count++;
			} else {
				BulletPlace(hw, *mptr, pcc);
			}	
		}
		break;
/* Description lists */
	case M_DESC_LIST:
		if (mark->is_end) {
/* restore the old state if there is one */
			if (DescType->next != NULL) {
				DescRec *dptr;

				dptr = DescType;
				DescType = DescType->next;
				pcc->left_margin = dptr->save_left_margin;
				pcc->cur_line_width = dptr->save_cur_line_width;
				LineBreak(hw,*mptr,pcc);
				pcc->indent_level--;
				free((char *)dptr);
			}
		} else {
			DescRec *dptr;

			LineBreak(hw,*mptr,pcc);
			pcc->indent_level++;
			dptr = (DescRec *)malloc(sizeof(DescRec));
/* Check is this is a compact list */
			tptr = ParseMarkTag(mark->start, MT_DESC_LIST, "COMPACT");
			if (tptr != NULL) {
				free(tptr);
				dptr->compact = 1;
			} else {
				dptr->compact = 0;
			}
			if(DescType->next != NULL){
				pcc->left_margin =DescType->indent_margin;
				pcc->cur_line_width =DescType->cur_line_width;
			}
			dptr->save_left_margin = pcc->left_margin;
			dptr->indent_margin = dptr->save_left_margin +
					D_INDENT_SPACES ;
			dptr->save_cur_line_width = pcc->cur_line_width;
			dptr->cur_line_width = dptr->save_cur_line_width -
					D_INDENT_SPACES ;
			if (dptr->cur_line_width <=0){
				dptr->cur_line_width = dptr->save_cur_line_width;
			}
/* Save the old state, and start a new */
			dptr->type = D_DESC_LIST_START;
			dptr->next = DescType;
			DescType = dptr;
		}
		pcc->x = pcc->eoffsetx + pcc->left_margin;
		pcc->is_bol =1;
		break;
	case M_DESC_TITLE:
		if (mark->is_end)
			break;
		LineBreak(hw,*mptr,pcc);
		pcc->left_margin= DescType->save_left_margin;
		pcc->cur_line_width= DescType->save_cur_line_width;
		pcc->x = pcc->eoffsetx + pcc->left_margin;
		pcc->is_bol =1;
		break;
	case M_DESC_TEXT:
		if (mark->is_end){
			pcc->left_margin= DescType->save_left_margin;
			pcc->cur_line_width= DescType->save_cur_line_width;
			break;
		}
		/* For a compact list we want to stay on the same
		 * line if there is room and we are the first line
		 * after a title.
		 */
		if (!DescType->compact) {
			LinefeedPlace(hw,*mptr,pcc);
			pcc->have_space_after = 0;
			pcc->left_margin= DescType->indent_margin;
			pcc->cur_line_width= DescType->cur_line_width;
			pcc->x = pcc->eoffsetx + pcc->left_margin;
			pcc->is_bol =1;
			break;
		} 
		pcc->left_margin= DescType->indent_margin;
		pcc->cur_line_width= DescType->cur_line_width;
		break;

/*
 * Now with forms, <ISINDEX> is the same as:
 * <FORM>
 * <HR>
 * This is a searchable index.  Enter search keywords:
 * <INPUT NAME="isindex">
 * <HR>
 * </FORM>
 * Also, <ISINDEX> will take an ACTION tag to specify a
 * different URL to submit the query to.
 */
	case M_ISINDEX:
		hw->html.is_index = True;
		if (pcc->cur_form != NULL)
			break; /* No index inside a form */

/* Start the form */
		LinefeedPlace(hw,*mptr,pcc);
		pcc->cur_form = (FormInfo *)malloc( sizeof(FormInfo));
		pcc->cur_form->next = NULL;
		pcc->cur_form->hw = (Widget)hw;
		pcc->cur_form->action = NULL;
		pcc->cur_form->action = ParseMarkTag(mark->start,MT_ISINDEX,"ACTION");
		pcc->cur_form->method = ParseMarkTag(mark->start,MT_ISINDEX,"METHOD");
		pcc->cur_form->enctype=ParseMarkTag(mark->start,MT_ISINDEX,"ENCTYPE");
		pcc->cur_form->start = pcc->widget_id;
		pcc->cur_form->end = -1;
/* Horizontal rule */
		LinefeedPlace(hw,*mptr,pcc);
		HRulePlace(hw, *mptr ,pcc);
		LinefeedPlace(hw,*mptr,pcc);
		/*
		 * Text: "This is a searchable index.
		 *  Enter search keywords: "
		 */
		mark_tmp.text = (char *)malloc(strlen(
			"This is a searchable index.  Enter search keywords: ")
			 + 1);
		strcpy(mark_tmp.text,
			"This is a searchable index.  Enter search keywords: ");
		PartOfTextPlace(hw, &mark_tmp, pcc);
/* Fake up the text INPUT tag.  */
		mark_tmp.start = (char *)malloc( strlen(
					"input SIZE=25 NAME=\"isindex\"") + 1);
		strcpy(mark_tmp.start,"input SIZE=25 NAME=\"isindex\"");
		WidgetPlace(hw, &mark_tmp,  pcc);
/*  Horizontal rule */
		LinefeedPlace(hw,*mptr,pcc);
		HRulePlace(hw, *mptr, pcc);
		LinefeedPlace(hw,*mptr,pcc);
/* Close the form  */
		LinefeedPlace(hw,*mptr,pcc);
		pcc->cur_form->end = pcc->widget_id;
		LinefeedPlace(hw,*mptr,pcc);
		AddNewForm(hw, pcc->cur_form);
		pcc->cur_form = NULL;
		break;

	case M_BR:
		LinefeedPlace(hw,*mptr,pcc); /* miss named break!!! */
		break;
	case M_BUGGY_TABLE:
		break;
	case M_TABLE:
		TablePlace(hw, mptr, pcc, save_obj);
		break;
	case M_IMAGE:		/* Just insert the image for now */
		if (mark->is_end)
			return;
		ImagePlace(hw, *mptr, pcc);
		break;
#ifdef APPLET
	case M_APPLET:
		if ((*mptr)->is_end) 		/* end of applet */
			return;
		AppletPlace(hw,mptr,pcc,save_obj);
		break;
#endif
#ifdef APROG
	case M_APROG:
		if ((*mptr)->is_end) 		/* end of aprog */
			return;
		AprogPlace(hw,mptr,pcc,save_obj);
		break;
#endif
	case M_MAP:
		printf("Tag <MAP> not yet implemented\n");
		break;
	case M_AREA:
		printf("Tag <AREA> not yet implemented\n");
		break;

	case M_HTML:			/* don't know what to do with */
	case M_COMMENT:
	case M_PARAM:			/* maybe seen in APROG/APPLET */
	case M_CAPTION:
	case M_TH:
	case M_TD:		/* <TD> peut reaparaitre dans une */
					/* analyse de <TABLE> */
	case M_DOCTYPE:			/* unused */
	case M_META:			/* unused */
	case M_LINK:			/* unused */

	case M_FRAME:		/* process by frameset */
	case M_ACRONYM:		/* not used */
	case M_ABBR:
	case M_TBODY:
	case M_TFOOT:
	case M_THEAD:
		break;
	default:
		fprintf(stderr,"[TriggerMarkChanges] Unknow marker %d\n",
			mark->type);
		break;
	}
} /* TriggerMarkChanges() */

/* GD ######### */
	/* copy and push the state */
	/* now pop PhotoComposeContext */
/*#############*/
/* Format all the objects in the passed Widget's parsed object list to fit
 * the locally global Width. Passes in the x,y coords of where to start 
 * placing the formatted text. Returns the ending x,y in same variables.
 * Title objects are ignored, and not formatted.
 * The locally global variables are assumed to have been initialized
 * before this function was called.
 * FormatChunk build also an internal 'struct ele_rec' that contain
 * information for later X-Window code and Placement. So it need to
 * be recursive . By example a table can contain an other table.
 */
void FormatChunk( HTMLWidget hw, struct mark_up * start_mark,
	struct mark_up * end_mark,
	PhotoComposeContext * pcc, Boolean save_obj)
{
	struct mark_up *mptr;

	mptr = start_mark;
	while (mptr != NULL) {
/* ################# voir peut etre la reentrance ici en fonction des tags####*/
		TriggerMarkChanges(hw, &mptr, pcc , save_obj);
		if (mptr == end_mark){
			return;
		}
		if (mptr)
			mptr = mptr->next;
	}
}
/* end GD ####### */

/* GD: add PhotoComposeContext struct ### */
/*
 * Called by the widget to format all the objects in the
 * parsed object list to fit its current window size.
 * Returns the max_height of the entire document.
 * Title objects are ignored, and not formatted.
 */
int FormatAll(HTMLWidget hw, int *Fwidth, Boolean save_obj)
{
	int saved_width;
	PhotoComposeContext pcc;
	int WidthOfViewablePart;

	saved_width = *Fwidth;
	hw->html.is_index = False;	 /* Clear the is_index flag */

	WidthOfViewablePart = *Fwidth;	/* taille visible de la fenetre */
					/* hw->core.width - swidth - (2 * st)*/
					/* on suppose qu'on a toujours vbar */

	pcc.width_of_viewable_part = WidthOfViewablePart; /* never change */
					/* during computation */
	pcc.right_margin = hw->html.margin_width; /* initial margin */
	pcc.left_margin = hw->html.margin_width;
	pcc.eoffsetx = 0;	/* I am the master element */
	pcc.eoffsety = 0;
	pcc.cur_line_width = WidthOfViewablePart - 
				pcc.right_margin - pcc.left_margin;
	pcc.ex = pcc.x = pcc.left_margin;
	pcc.ey = pcc.y = hw->html.margin_height;
	pcc.margin_height = hw->html.margin_height;
	pcc.cur_baseline = 0;	/* all object in a line must have the same */
				/* baseline. If baseline change then adjust */
				/* pcc.y & pcc.ey & pcc.cur_line_height */
				/* and the y value in each element of line */
				/* pcc.y - pcc.cur_baseline donne la top line */
				/* de la boundingBox de la ligne */
				/* pcc.y - pcc.cur_baseline +pcc.cur_line_height*/
				/* donne la top line de la ligne suivante */
	pcc.cur_line_height = 0;
	pcc.element_id = 0;	/* to get unique number */
	pcc.is_bol = True;	/* we are at begin of line */
	pcc.have_space_after = False;	/* remember if a word have a space after*/
	pcc.cur_font = hw->html.cur_font;
	pcc.anchor_tag_ptr = NULL_ANCHOR_PTR;		/* we are in anchor ?? */
	pcc.max_width_return = 0;
				/* we compute the MaxWidth of hyper text to */
				/* adjust scrollbar */
				/* initial value is WidthOfViewablePart */
	pcc.pf_lf_state = 0;	/* linefeed state. Hack for preformat */
	pcc.preformat = 0;
	pcc.div = DIV_ALIGN_LEFT;
	pcc.fg = hw->manager.foreground;
	pcc.bg = hw->core.background_pixel;
	pcc.underline_number = 0;
	pcc.in_underlined = 0;
	pcc.dashed_underlines = False;
	pcc.cw_only = False;
	pcc.computed_min_x = 0;
	pcc.computed_max_x = 0;
	pcc.cur_form = NULL;
	pcc.in_form = False;
	pcc.widget_id = 0;
	pcc.aprog_id = 0;
	pcc.applet_id = 0;
        pcc.superscript = 0;
        pcc.subscript = 0;
	pcc.indent_level = 0;
	pcc.internal_mc_eo = 0;
	pcc.text_area_buf = NULL;
	pcc.ignore = 0;
	pcc.current_select = NULL;
	pcc.in_select = False;
	pcc.is_in_paragraph = False;
	pcc.strikeout = False;

/* Initialize local variables, some from the widget */
	DescType = &BaseDesc;
	DescType->type = D_NONE;
	DescType->count = 0;
	DescType->compact = 0;
	DescType->next = NULL;
	DescType->save_left_margin = pcc.left_margin;
	DescType->indent_margin = pcc.left_margin;
	DescType->save_cur_line_width = pcc.cur_line_width;
	DescType->cur_line_width = pcc.cur_line_width;
	InDocHead = 0;
	in_title = 0;
/* Free up previously formatted elements */
	FreeLineList(hw->html.formatted_elements,(Widget)hw);
/* Start a null element list, to be filled in as we go. */
	hw->html.cur_elem_to_format = NULL;
        hw->html.last_formatted_elem = NULL;
	hw->html.formatted_elements = NULL;
				/* Clear any previous selections */
	hw->html.select_start = NULL;
	hw->html.select_end = NULL;
	hw->html.new_start = NULL;
	hw->html.new_end = NULL;
/* Set up a starting font, and starting x, y, position */
	hw->html.font_stack = NULL;
/*	FontStack->font = hw->html.font; */
	MMInitWidgetFont(hw);

 					/* Format all objects for width */
	FormatChunk(hw,hw->html.html_objects,NULL,&pcc,save_obj);

/* Ensure a linefeed after the final element. */
	LinefeedPlace(hw,NULL,&pcc);
			/* Add the bottom margin to the max height. */
	pcc.y = pcc.y + hw->html.margin_height;

/* If the passed in MaxWidth was wrong, correct it.*/
	if (pcc.max_width_return > saved_width)
		*Fwidth = pcc.max_width_return;

	return(pcc.y);
}

/*
 * Refresh all elements on a single line into the widget's window
 * C'est la dedans qu'on fait des actions X pour les objets calcules
 * et Parses au paravant. Tout est deja precalcule, Y a pu Qu'a...
 * line_array[i] est la ligne a faire . C'est un tableau de struct ele_rec *
 * line_array[i] est le premier element d'une liste chainee (de eptr).
 */
/*void PlaceLine( HTMLWidget hw, int line)
*/
void RefreshElement(HTMLWidget hw,struct ele_rec *eptr)
{
/*
	printf("call PlaceLine at line # %d for %d\n",line, eptr->type);
	printf("eptr->x = %d, eptr->y = %d\n", eptr->x, eptr->y);
	printf("eptr->baseline = %d, scroll_x = %d, scroll_y = %d\n",
		eptr->baseline, hw->html.scroll_x, hw->html.scroll_y);
*/
	switch(eptr->type) {
	case E_TEXT:
	        TextRefresh(hw, eptr, 0, (eptr->edata_len - 2));
		break;
	case E_BULLET:
		BulletRefresh(hw, eptr);
		break;
	case E_HRULE:
		HRuleRefresh(hw, eptr);
		break;
	case E_CR:
/*		printf("Refresh E_CR\n"); */
		break;
	case E_LINEFEED:
	        if(!hw->html.bg_image)
		  LinefeedRefresh(hw, eptr); 
		break;
	case E_IMAGE:
		ImageRefresh(hw, eptr);
		break;
	case E_WIDGET:
		WidgetRefresh(hw, eptr);
		break;
	case E_TABLE:
		TableRefresh(hw, eptr);
		break;
	case E_CELL_TABLE:
		printf("Refresh E_CELL_TABLE\n");
		break;
#ifdef APROG
	case E_APROG:
		AprogRefresh(hw,eptr);
		break;
#endif
#ifdef APPLET
	case E_APPLET:
		AppletRefresh(hw,eptr);
		break;
#endif
	default:
		fprintf(stderr,"[PlaceLine] Unknow Element %d\n",eptr->type);
		break;
	}
}

/*
 * Locate the element (if any) that is at the passed location
 * in the widget.  If there is no corresponding element, return
 * NULL.  If an element is found return the position of the character
 * you are at in the pos pointer passed.
 */
struct ele_rec * LocateElement( HTMLWidget hw, int x, int y, int *pos)
{
	struct ele_rec *eptr;
	struct ele_rec *rptr;
	int tx1, tx2, ty1, ty2;

	x = x + hw->html.scroll_x;
	y = y + hw->html.scroll_y;


	/* Search element by element, for now we only search
	 * text elements, images, and linefeeds.
	 */
	eptr = hw->html.formatted_elements;

	rptr = NULL;
	while (eptr != NULL) {
		ty1 = eptr->y;
		ty2 = eptr->y + eptr->height;
		tx1 = eptr->x;
		tx2 = eptr->x + eptr->width;
		switch(eptr->type){
		case E_TEXT:
			if ((x >= tx1)&&(x <= tx2)&&(y >= ty1)&&(y <= ty2)) {
				rptr = eptr;
			}
			break;
		case E_IMAGE:
			if((x >= tx1)&&(x <= tx2)&&(y >= ty1)&&(y <=ty2)){
				rptr = eptr;
			}
			break;
		case E_CR:
		case E_LINEFEED:
/*########################
			if ((x >= tx1)&&(y >= ty1)&&(y <= ty2)) {
				rptr = eptr;
				break;
			}
			 else if (eptr->next == NULL) {
				rptr = eptr;
				break;
			} else if (eptr->next != NULL) {
				int tmpy;

				tmpy = eptr->next->y + eptr->next->height;
				tx2 = eptr->next->x;
				if ((x < tx2)&&(y >= ty2)&&(y <= tmpy)) {
					rptr = eptr;
					break;
				}
			}
########*/
			break;
		}
		if (rptr)
			break;
		eptr = eptr->next;
	} /* while */

	/*
	 * If we found an element, locate the exact character position within
	 * that element.
	 */
	if ( (rptr != NULL) && rptr->type == E_TEXT) {
		int dir, ascent, descent;
		XCharStruct all;
		int epos;

		/*
		 * Start assuming fixed width font.  The real position should
		 * always be <= to this, but just in case, start at the end
		 * of the string if it is not.
		 */
		epos = ((x - rptr->x) / rptr->font->max_bounds.width) + 1;
		if (epos >= rptr->edata_len - 1)
			epos = rptr->edata_len - 2;
		XTextExtents(rptr->font, (char *)rptr->edata,
				(epos + 1), &dir, &ascent, &descent, &all);
		if (x > (int)(rptr->x + all.width)) {
			epos = rptr->edata_len - 3;
		} else {
			epos--;
		}

		while (epos >= 0) {
			XTextExtents(rptr->font, (char *)rptr->edata,
				(epos + 1), &dir, &ascent, &descent, &all);
			if ((int)(rptr->x + all.width) <= x)
				break;
			epos--;
		}
		epos++;
		*pos = epos;
	}
	return(rptr);
}

/* Used by ParseTextToPrettyString to let it be sloppy about its
 * string creation, and never overflow the buffer.
 * It concatonates the passed string to the current string, managing
 * both the current string length, and the total buffer length.
 */
void strcpy_or_grow( char **str, int *slen, int *blen, char *add)
{
	int newlen;
	int addlen;
	char *buf;

	/*
	 * If necessary, initialize this string buffer
	 */
	if (*str == NULL) {
		*str = (char *)malloc(1024 * sizeof(char));
		CHECK_OUT_OF_MEM(*str);
		*blen = 1024;
		strcpy(*str, "");
		*slen = 0;
	}
	buf = *str;
	if ((buf == NULL)||(add == NULL))
		return;
	addlen = strlen(add);
	newlen = *slen + addlen;
	if (newlen >= *blen) {
		newlen = ((newlen / 1024) + 1) * 1024;
		buf = (char *)malloc(newlen * sizeof(char));
		CHECK_OUT_OF_MEM(buf);
		memcpy(buf, *str, *blen);
		free((char *)*str);
		*str = buf;
		*blen = newlen;
	}
	memcpy((char *)(buf + *slen), add, addlen + 1);
	*slen = *slen + addlen;
}

/*
 * Parse all the formatted text elements from start to end
 * into an ascii text string, and return it.
 * space_width and lmargin tell us how many spaces
 * to indent lines.
 */
char * ParseTextToString( struct ele_rec *startp, struct ele_rec *endp,
	int start_pos, int end_pos,
	int space_width, int lmargin)
{
	int newline;
	int epos;
	char *text;
	int t_slen, t_blen;
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;

	if (startp == NULL)
		return(NULL);

	if (SwapElements(startp, endp, start_pos, end_pos)) {
		start = endp;
		end = startp;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	} else {
		start = startp;
		end = endp;
	}
	text = NULL;
	newline = 0;
	eptr = start;
	while ((eptr != NULL)&&(eptr != end)) { 
		if (eptr->type == E_TEXT) {
			int i, spaces;
			char *tptr;

			if (eptr == start) {
				tptr = (char *)(eptr->edata + start_pos);
			} else {
				tptr = (char *)eptr->edata;
			}

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				if (spaces < 0)
					spaces = 0;
				for (i=0; i<spaces; i++) {
					strcpy_or_grow(&text, &t_slen, &t_blen,
						" ");
				}
			}
			strcpy_or_grow(&text, &t_slen, &t_blen, tptr);
			newline = 0;
		}
		else if (eptr->type == E_LINEFEED) {
			strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			newline = 1;
		}
		eptr = eptr->next;
	}
	if (eptr != NULL) {
		if (eptr->type == E_TEXT) {
			int i, spaces;
			char *tptr;
			char *tend, tchar;

			if (eptr == start) {
				tptr = (char *)(eptr->edata + start_pos);
			} else {
				tptr = (char *)eptr->edata;
			}

			if (eptr == end) {
				tend = (char *)(eptr->edata + end_pos + 1);
				tchar = *tend;
				*tend = '\0';
			}

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				if (spaces < 0)
					spaces = 0;
				for (i=0; i<spaces; i++) 
					strcpy_or_grow(&text,&t_slen,&t_blen," ");
			}
			strcpy_or_grow(&text, &t_slen, &t_blen, tptr);
			newline = 0;
			if (eptr == end)
				*tend = tchar;
		} else if (eptr->type == E_LINEFEED) {
			strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			newline = 1;
		}
	}
	return(text);
}

/* Parse all the formatted text elements from start to end
 * into an ascii text string, and return it.
 * Very like ParseTextToString() except the text is prettied up
 * to show headers and the like.
 * space_width and lmargin tell us how many spaces to indent lines.
 */
char * ParseTextToPrettyString( HTMLWidget hw,
	struct ele_rec *startp, struct ele_rec *endp,
	int start_pos, int end_pos,
	int space_width, int lmargin)
{
	int newline;
	int lead_spaces;
	int epos;
	char *text;
	int t_slen, t_blen;
	char *line_buf;
	int l_slen, l_blen;
	char lchar;
	struct ele_rec *eptr;
	struct ele_rec *start;
	struct ele_rec *end;
	struct ele_rec *last;

	if (startp == NULL)
		return(NULL);

	if (SwapElements(startp, endp, start_pos, end_pos)) {
		start = endp;
		end = startp;
		epos = start_pos;
		start_pos = end_pos;
		end_pos = epos;
	} else {
		start = startp;
		end = endp;
	}

	text = NULL;
	line_buf = NULL;

/* We need to know if we should consider the indentation or bullet
 * that might be just before the first selected element to also be
 * selected.  This current hack looks to see if they selected the
 * Whole line, and assumes if they did, they also wanted the beginning.
 *
 * If we are at the beginning of the list, or the beginning of
 * a line, or just behind a bullett, assume this is the start of
 * a line that we may want to include the indent for.
 */
	if( (start_pos == 0) && 
	    ((start->prev == NULL) || (start->prev->type == E_BULLET)) ) {
		eptr = start;
		while((eptr != NULL)&&(eptr != end)&&(eptr->type != E_LINEFEED))
			eptr = eptr->next;
		if ((eptr != NULL)&&(eptr->type == E_LINEFEED)) {
			newline = 1;
			if((start->prev != NULL)&&(start->prev->type == E_BULLET))
				start = start->prev;
		} else {
			newline = 0;
		}
	} else {
		newline = 0;
	}
	lead_spaces = 0;
	last = start;
	eptr = start;
	while ((eptr != NULL)&&(eptr != end)) {
		if (eptr->type == E_BULLET) {
			int i, spaces;

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				spaces -= 2;
				if (spaces < 0)
					spaces = 0;
				lead_spaces = spaces;
				for (i=0; i<spaces; i++) {
					strcpy_or_grow(&line_buf, &l_slen, 
							&l_blen, " ");
				}
			}
			newline = 0;
			strcpy_or_grow(&line_buf, &l_slen, &l_blen, "o ");
			lead_spaces += 2;
		}
		else if (eptr->type == E_TEXT) {
			int i, spaces;
			char *tptr;

			if (eptr == start) {
				tptr = (char *)(eptr->edata + start_pos);
			} else {
				tptr = (char *)eptr->edata;
			}

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				if (spaces < 0)
					spaces = 0;
				lead_spaces = spaces;
				for (i=0; i<spaces; i++) {
					strcpy_or_grow(&line_buf,
						&l_slen, &l_blen, " ");
				}
			}
			strcpy_or_grow(&line_buf, &l_slen, &l_blen, tptr);
			newline = 0;
		}
		else if (eptr->type == E_LINEFEED) {
			strcpy_or_grow(&text, &t_slen, &t_blen, line_buf);
			strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			newline = 1;
			lchar = '\0';
			/*### lchar = '*'; in case of header #### */
			if (lchar != '\0') {
				char *ptr;
				int cnt;

				cnt = 0;
				ptr = line_buf;
				while ((ptr != NULL)&&(*ptr != '\0')) {
					cnt++;
					if (cnt > lead_spaces)
						*ptr = lchar;
					ptr++;
				}
				strcpy_or_grow(&text,&t_slen,&t_blen, line_buf);
				strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			}
			if (line_buf != NULL) {
				free(line_buf);
				line_buf = NULL;
			}
		}
		last = eptr;
		eptr = eptr->next;
	} /* while */
	if (eptr != NULL) {
		if (eptr->type == E_BULLET) {
			int i, spaces;

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				spaces -= 2;
				if (spaces < 0)
					spaces = 0;
				lead_spaces = spaces;
				for (i=0; i<spaces; i++) {
					strcpy_or_grow(&line_buf,
						&l_slen, &l_blen, " ");
				}
			}
			newline = 0;

			strcpy_or_grow(&line_buf, &l_slen, &l_blen, "o ");
			lead_spaces += 2;
		}
		else if (eptr->type == E_TEXT) {
			int i, spaces;
			char *tptr;
			char *tend, tchar;

			if (eptr == start) {
				tptr = (char *)(eptr->edata + start_pos);
			} else {
				tptr = (char *)eptr->edata;
			}

			if (eptr == end) {
				tend = (char *)(eptr->edata + end_pos + 1);
				tchar = *tend;
				*tend = '\0';
			}

			if (newline) {
				spaces = (eptr->x - lmargin) / space_width;
				if (spaces < 0)
					spaces = 0;
				lead_spaces = spaces;
				for (i=0; i<spaces; i++) {
					strcpy_or_grow(&line_buf,
						&l_slen, &l_blen, " ");
				}
			}
			strcpy_or_grow(&line_buf, &l_slen, &l_blen, tptr);
			newline = 0;
			if (eptr == end)
				*tend = tchar;
		}
		else if (eptr->type == E_LINEFEED) {
			strcpy_or_grow(&text, &t_slen, &t_blen, line_buf);
			strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			newline = 1;
			lchar = '\0';
			/* #### lchar = '*'; in case of header */
			if (lchar != '\0') {
				char *ptr;
				int cnt;

				cnt = 0;
				ptr = line_buf;
				while ((ptr != NULL)&&(*ptr != '\0')) {
					cnt++;
					if (cnt > lead_spaces) {
						*ptr = lchar;
					}
					ptr++;
				}
				strcpy_or_grow(&text,&t_slen,&t_blen, line_buf);
				strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			}
			if (line_buf != NULL) {
				free(line_buf);
				line_buf = NULL;
			}
		}
		last = eptr;
	}
	if (line_buf != NULL) {
		strcpy_or_grow(&text, &t_slen, &t_blen, line_buf);
		lchar = '\0';
		/* #### lchar = '*'; in case of header */
		if (lchar != '\0') {
			char *ptr;
			int cnt;

			cnt = 0;
			ptr = line_buf;
			while ((ptr != NULL)&&(*ptr != '\0')) {
				cnt++;
				if (cnt > lead_spaces)
					*ptr = lchar;
				ptr++;
			}
			strcpy_or_grow(&text, &t_slen, &t_blen, "\n");
			strcpy_or_grow(&text, &t_slen, &t_blen, line_buf);
		}
	}
	if (line_buf != NULL) {
		free(line_buf);
		line_buf = NULL;
	}
	return(text);
}

/* Find the preferred width of a parsed HTML document
 * Currently unformatted plain text, unformatted listing text, plain files
 * and preformatted text require special width.
 * Preferred width = (width of longest plain text line in document) *
 * 	(width of that text's font)
 */
int DocumentWidth(HTMLWidget hw, struct mark_up *list)
{
	struct mark_up *mptr;
	int plain_text;
	int pcnt, pwidth;
	int width;
	char *ptr;

	/* Loop through object list looking at the plain, preformatted text
	 */
	width = 0;
	pwidth = 0;
	plain_text = 0;
	mptr = list;
	while (mptr != NULL) {
		/* All text blocks between the starting and ending
		 * plain and pre text markers are plain text blocks.
		 * Manipulate flags so we recognize these blocks.
		 */
		if( mptr->type == M_PREFORMAT) {
			if (mptr->is_end) {
				plain_text--;
				if (plain_text < 0)
					plain_text = 0;
			} else
				plain_text++;
			pcnt = 0;
		}
		/* If this is a plain text block, add to line length.
		 * Find the Max of all line lengths.
		 */
		else if ((plain_text)&&(mptr->type == M_NONE)) {
			ptr = mptr->text;
			while ((ptr != NULL)&&(*ptr != '\0')) {
				ptr = MaxTextWidth(ptr, &pcnt);
				if (pcnt > pwidth)
					pwidth = pcnt;
			}
		}
		mptr = mptr->next;
	} /* while */
/*	width = pwidth * hw->html.plain_font->max_bounds.width; */
	width = pwidth * hw->html.cur_font->max_bounds.width;
	return(width);
}
