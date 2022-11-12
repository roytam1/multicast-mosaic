/* Please read copyright.tmpl. Don't remove next line */
#include "copyright.ncsa"

#include <stdlib.h>
#include <sys/time.h>
#include <stdio.h>
#include <ctype.h>
#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "../src/mo-www.h"
#include "../src/mosaic.h"

#include <Xm/Frame.h>
#include <Xm/DrawingA.h>
#include <Xm/Label.h>
#include "../libmc/mc_dispatch.h"

extern unsigned int mc_global_eo_count; /*### from mc_dispatch ###*/

struct mark_up NULL_ANCHOR = {
	M_ANCHOR,		/* MarkType */
	1,			/* is_end */
	NULL,			/* start */
	NULL,			/* text */
	NULL,			/* end */
	NULL,			/* next */
	NULL,			/* saved_aps */
	NULL,			/* anchor_name */
	NULL,			/* anchor_href */
	NULL			/* anchor_title */
};

struct mark_up * NULL_ANCHOR_PTR = &NULL_ANCHOR ;

struct timeval Tv;
struct timezone Tz;

/* I need my own is ispunct function because I need a closing paren
 * immediately after a word to act like punctuation.
 */
#define	MY_ISPUNCT(val)	(ispunct((int)(val)) || ((val) == ')'))
#define INDENT_SPACES	2

/* ############## This is may be as global ####*/
static FontRec FontBase;
static FontRec *FontStack;
static SelectInfo *CurrentSelect;

/* ##############  This is maybe in a context stack ####*/
static DescRec BaseDesc;
static DescRec *DescType;
static int Ignore;
static Boolean Strikeout;
static char *TitleText;
static char *TextAreaBuf;
static XFontStruct *nonScriptFont;
static int InDocHead;
static MapInfo *CurrentMap=NULL; /* csi stuff -- swp */
/* ############ */

/*
 * Set the formatted element into the format list. 
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
	/*
	 * There is not pre-allocated format list, or we have reached
	 * the end of the pre-allocated list.  Create a new element, and
	 * add it.
	 */
	if (hw->html.formatted_elements == NULL){ /* the first element */
						/* create it */
		eptr = (struct ele_rec *) malloc( sizeof(struct ele_rec));
		if (eptr == NULL){
			MEM_OVERFLOW;
		}
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
		if (eptr == NULL){
			MEM_OVERFLOW;			
		}
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
	eptr->line_number = pcc->line_number;
	eptr->ele_id = ++(pcc->element_id);

	eptr->pic_data = NULL;
	eptr->widget_data = NULL;
	eptr->table_data = NULL;
	eptr->aprog_struct = NULL;

	eptr->line_next = NULL;

        eptr->wtype = MC_MO_TYPE_UNICAST;
        eptr->internal_numeo = 0;
	eptr->valignment = ALIGN_BOTTOM;
	eptr->halignment = ALIGN_LEFT;
	eptr->selected = False;
	eptr->indent_level = pcc->indent_level;
	eptr->start_pos = 0;
	eptr->end_pos = 0;
	eptr->bwidth = IMAGE_DEFAULT_BORDER;
	eptr->underline_number = pcc->underline_number;
	eptr->dashed_underline = pcc->dashed_underlines;
	eptr->strikeout = Strikeout;
	eptr->fg = pcc->fg;
	eptr->bg = pcc->bg;
        eptr->anchor_tag_ptr = NULL_ANCHOR_PTR;	/* putit in struct markup##### */
        eptr->edata = NULL;
        eptr->edata_len = 0;
        		/* eptr->next is set when allocate */
        		/* eptr->prev is set when allocate */
	return eptr;
}

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
		supsubBaseline = nonScriptFont->max_bounds.ascent;
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

/*
 * Place the number at the beginning of an numbered
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


static void PushFont( XFontStruct *font)
{
	FontRec *fptr;

	fptr = (FontRec *)malloc(sizeof(FontRec));
	if (fptr == NULL) {
		fprintf(stderr, "No memory to expand font stack!\n");
		return;
	}
	fptr->font = font;
	fptr->next = FontStack;
	FontStack = fptr;
}

static XFontStruct * PopFont()
{
	XFontStruct *font;
	FontRec *fptr;

	if (FontStack->next != NULL) {
		fptr = FontStack;
		FontStack = FontStack->next;
		font = fptr->font;
		free((char *)fptr);
	} else {
		if (htmlwTrace) 
			fprintf(stderr, "Warning, popping empty font stack!\n");
		font = FontStack->font;
	}
	return(font);
}

/*
 * We've just terminated the current OPTION.
 * Put it in the proper place in the SelectInfo structure.
 * Move option_buf into options, and maybe copy into
 * value if is_value is set.
 */
static void ProcessOption( SelectInfo *sptr)
{
	int i, cnt;
	char **tarray;

	clean_white_space(sptr->option_buf);
	tarray = sptr->options;
	cnt = sptr->option_cnt + 1;
	sptr->options = (char **)malloc(sizeof(char *) * cnt);
	for (i=0; i<(cnt - 1); i++)
		sptr->options[i] = tarray[i];
	if (tarray != NULL)
		free((char *)tarray);
	sptr->options[cnt - 1] = sptr->option_buf;
	sptr->option_cnt = cnt;
	tarray = sptr->returns;
	cnt = sptr->option_cnt;
	sptr->returns = (char **)malloc(sizeof(char *) * cnt);
	for (i=0; i<(cnt - 1); i++)
		sptr->returns[i] = tarray[i];
	if (tarray != NULL)
		free((char *)tarray);
	sptr->returns[cnt - 1] = sptr->retval_buf;
	if (sptr->is_value) {
		tarray = sptr->value;
		cnt = sptr->value_cnt + 1;
		sptr->value = (char **)malloc(sizeof(char *) * cnt);
		for (i=0; i<(cnt - 1); i++)
			sptr->value[i] = tarray[i];
		if (tarray != NULL)
			free((char *)tarray);
		sptr->value[cnt - 1] =(char *)malloc(strlen(sptr->option_buf) +1);
		strcpy(sptr->value[cnt - 1], sptr->option_buf);
		sptr->value_cnt = cnt;
	}
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
	if (buf == NULL)
		return(value);
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
/*static###*/ void TriggerMarkChanges(HTMLWidget hw, struct mark_up **mptr,
		PhotoComposeContext * pcc, Boolean save_obj)
{
	struct mark_up *mark;
	XFontStruct * tmp_font;
	int type;
	struct mark_up mark_tmp;
	char *tptr;

	mark = *mptr;
	type = mark->type;

	/* If we are not in a tag that belongs in the HEAD, end the HEAD
	   section  - amb */
	if (InDocHead) {
		if((type != M_TITLE)&&(type != M_NONE)&&(type != M_BASE)&&
		   (type != M_INDEX)&&(type != M_COMMENT)) {
			Ignore = 0;
			InDocHead = 0;
	  	}
	}
	/* If Ignore is set, we ignore all further elements until we get to the
	 * end of the Ignore
	 * Let text through so we can grab the title text.
	 * Let title through so we can hit the end title.
	 * Now also used for SELECT parseing
	 * Let SELECT through so we can hit the end SELECT.
	 * Let OPTION through so we can hit the OPTIONs.
	 * Let TEXTAREA through so we can hit the TEXTAREAs.
	 */
	if ((Ignore)&&(!InDocHead)&&(type != M_TITLE)&&(type != M_NONE)&&
		(type != M_SELECT)&&(type != M_OPTION)&&
		(type != M_TEXTAREA)&&(type != M_DOC_HEAD))
		        return;

	switch(type) {
	/*
	 * Place the text.  Different functions based on whether it
	 * is pre-formatted or not.
	 */
	case M_NONE:
		if ((Ignore)&&(CurrentSelect == NULL)&& (TextAreaBuf == NULL)) {
			if (TitleText == NULL) {
				TitleText = (char *)
					malloc(strlen((*mptr)->text) + 1);
				strcpy(TitleText, (*mptr)->text);
			} else {
				tptr = (char *) malloc(strlen(TitleText) +
					       strlen((*mptr)->text) + 1);
				strcpy(tptr, TitleText);
				strcat(tptr, (*mptr)->text);
				free(TitleText);
				TitleText = tptr;
			}
			break;
		}
		if ((Ignore)&&(CurrentSelect != NULL)) {
			if (CurrentSelect->option_buf != NULL) {
				tptr = (char *)malloc(strlen(
					CurrentSelect->option_buf) +
					       strlen((*mptr)->text) + 1);
				strcpy(tptr, CurrentSelect->option_buf);
				strcat(tptr, (*mptr)->text);
				free(CurrentSelect->option_buf);
				CurrentSelect->option_buf = tptr;
			}
			break;
		}
		if ((Ignore)&&(TextAreaBuf != NULL)) {
			TextAreaBuf = TextAreaAddValue(TextAreaBuf,(*mptr)->text);
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
			pcc->div = DIV_ALIGN_LEFT;
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			pcc->div = DIV_ALIGN_LEFT;
			tptr = ParseMarkTag(mark->start, MT_PARAGRAPH, "ALIGN");
			if (caseless_equal(tptr, "CENTER"))
				pcc->div = DIV_ALIGN_CENTER;
			if (caseless_equal(tptr, "RIGHT"))
				pcc->div = DIV_ALIGN_RIGHT;
			if(tptr)
				free(tptr);
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
		       if (!InDocHead)
			    Ignore = 0;
		       hw->html.title = TitleText;
		       TitleText = NULL;
		} else {
			Ignore = 1;
			TitleText = NULL;
		}
		break;
	/*
	 * Formatting commands just change the current font.
	 */
	case M_CODE:
	case M_SAMPLE:
	case M_KEYBOARD:
	case M_FIXED:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.fixed_font;
		}
		break;
	case M_STRONG:
	case M_BOLD:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
			if (pcc->cur_font == hw->html.fixed_font ||
			    pcc->cur_font == hw->html.fixeditalic_font)
				pcc->cur_font = hw->html.fixedbold_font;
			else if (pcc->cur_font == hw->html.plain_font ||
			    pcc->cur_font == hw->html.plainitalic_font)
				pcc->cur_font = hw->html.plainbold_font;
			else
				pcc->cur_font = hw->html.bold_font;
		}
		break;
	case M_EMPHASIZED:
	case M_VARIABLE:
	case M_CITATION:
	case M_ITALIC:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
			if (pcc->cur_font == hw->html.fixed_font ||
			    pcc->cur_font == hw->html.fixedbold_font)
				pcc->cur_font = hw->html.fixeditalic_font;
			else if (pcc->cur_font == hw->html.plain_font ||
			    pcc->cur_font == hw->html.plainbold_font)
				pcc->cur_font = hw->html.plainitalic_font;
			else
				pcc->cur_font = hw->html.italic_font;
		}
		break;
	/*
	 * Strikeout means draw a line through the text.
	 * Right now we just set a boolean flag which gets shoved
	 * in the element record for all elements in the
	 * strikeout zone.
	 */
	case M_STRIKEOUT:
		Strikeout = True;
		if (mark->is_end)
			Strikeout = False;
		break;
        case M_SUP:

                if (mark->is_end) {
                       pcc->superscript--;
                       if ((pcc->superscript==0) && (pcc->subscript==0))
                             pcc->cur_font = PopFont();
                 } else {
                       pcc->superscript++;
                       if ((pcc->superscript==1) && (pcc->subscript==0)) {
                              nonScriptFont=pcc->cur_font;
                              PushFont(pcc->cur_font);
                              pcc->cur_font = hw->html.supsub_font;
                        }
                }

                break;
	case M_SUB:
		if (mark->is_end) {
                        pcc->subscript--;
                        if ((pcc->subscript==0) && (pcc->superscript==0))
                                pcc->cur_font = PopFont();
                } else {
                        pcc->subscript++;
                        if ((pcc->subscript==1) && (pcc->superscript==0)) {
                        	nonScriptFont=pcc->cur_font;
                        	PushFont(pcc->cur_font);
                        	pcc->cur_font = hw->html.supsub_font;
                        }
                }
                break;
/* amb - ignore text inside a HEAD element */
	case M_DOC_HEAD:
		InDocHead = 1;
		Ignore = 1;
		if (mark->is_end) {
		        InDocHead = 0;
			Ignore = 0;
		}
		break;
	case M_DOC_BODY:
		if (!mark->is_end) {
		        InDocHead = 0;   /* end <head> section */
			Ignore = 0;
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
	 * Headers are preceeded and followed by a linefeed,
	 * and the change the font.
	 */
	case M_HEADER_1:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header1_font;
		}
		break;
	case M_HEADER_2:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header2_font;
		}
		break;
	case M_HEADER_3:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header3_font;
		}
		break;
	case M_HEADER_4:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header4_font;
		}
		break;
	case M_HEADER_5:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header5_font;
		}
		break;
	case M_HEADER_6:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
			LineBreak(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header6_font;
		}
		break;
	/*
	 * Anchors change the text color, and may set
	 * underlineing attributes.
	 * No linefeeds, so they can be imbedded anywhere.
	 */
	case M_FRAME:
		printf("Tag <FRAME> not implemented\n");
		break;
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
		tptr = (*mptr)->anchor_href = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_HREF);
		(*mptr)->anchor_name = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_NAME);
		(*mptr)->anchor_title = ParseMarkTag(mark->start, 
					MT_ANCHOR, AT_TITLE);

/*
########## faire une liste d'anchor et liberer a la fin #########
if(pcc->anchor_tag_ptr->anchor_href)
	free(pcc->anchor_tag_ptr->anchor_href);
if(pcc->anchor_tag_ptr->anchor_name)
	free(pcc->anchor_tag_ptr->anchor_name);
if(pcc->anchor_tag_ptr->anchor_title)
	free(pcc->anchor_tag_ptr->anchor_title);
###################################
*/

		if (tptr != NULL) { 
		    	pcc->fg = hw->html.anchor_fg;
		    	pcc->underline_number = hw->html.num_anchor_underlines;
		    	pcc->dashed_underlines = hw->html.dashed_anchor_lines;
/* we may want to send the href back somewhere else and
 * find out if we've visited it before */
		        if (hw->html.previously_visited_test != NULL) {
			    if((*(visitTestProc)
			      (hw->html.previously_visited_test)) ((Widget)hw, tptr)) {
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
		/* amb 2 */
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
		if (CurrentSelect != NULL) {
			if (CurrentSelect->option_buf != NULL)
				ProcessOption(CurrentSelect);
			CurrentSelect->option_buf = (char *)malloc(1);
			strcpy(CurrentSelect->option_buf, "");

			/*
			 * Check if this option starts selected
			 */
			tptr = ParseMarkTag(mark->start, MT_OPTION, "SELECTED");
			if (tptr != NULL) {
				CurrentSelect->is_value = 1;
				free(tptr);
			} else {
				CurrentSelect->is_value = 0;
			}

			/*
			 * Check if this option has an different
			 * return value field.
			 */
			tptr = ParseMarkTag(mark->start, MT_OPTION, "VALUE");
			if (tptr != NULL) {
			    if (*tptr != '\0') {
				CurrentSelect->retval_buf = tptr;
			    } else {
				CurrentSelect->retval_buf = NULL;
				free(tptr);
			    }
			} else {
				CurrentSelect->retval_buf = NULL;
			}
		}
		break;
/*
 * Special INPUT tag.  Allows an option menu or a scrolled list.
 * Due to a restriction in SGML, this can't just be a subset of 
 * the INPUT markup.  However, I can treat it that way to avoid duplicating
 * code. As a result I combine SELECT and OPTION into a faked up INPUT mark.
 */
	case M_SELECT:
		if (pcc->cur_form == NULL)
			break;
		if ((mark->is_end)&&(CurrentSelect != NULL)) {
			int len;
			char *buf;
			char *start;
			char *options, *returns, *value;

			if (CurrentSelect->option_buf != NULL)
				ProcessOption(CurrentSelect);

			options = ComposeCommaList( CurrentSelect->options,
				CurrentSelect->option_cnt);
			returns = ComposeCommaList( CurrentSelect->returns,
				CurrentSelect->option_cnt);
			value = ComposeCommaList( CurrentSelect->value,
				CurrentSelect->value_cnt);
			FreeCommaList( CurrentSelect->options,
				CurrentSelect->option_cnt);
			FreeCommaList( CurrentSelect->returns,
				CurrentSelect->option_cnt);
			FreeCommaList( CurrentSelect->value,
				CurrentSelect->value_cnt);
/*
 * Construct a fake INPUT tag.
 */
			len = strlen(MT_INPUT) + strlen(options) +
				strlen(returns) + strlen(value) + strlen(
			     " type=select options=\"\" returns=\"\" value=\"\"");
			buf = (char *)malloc(len +
			    strlen(CurrentSelect->mptr->start) + 1);
			strcpy(buf, MT_INPUT);
			strcat(buf, " type=select");
			strcat(buf, " options=\"");
			strcat(buf, options);
			strcat(buf, "\" returns=\"");
			strcat(buf, returns);
			strcat(buf, "\" value=\"");
			strcat(buf, value);
			strcat(buf, "\"");
			strcat(buf, (char *) (CurrentSelect->mptr->start +
				strlen(MT_SELECT)));
/*
 * stick the fake in, saving the real one.
 */
			start = CurrentSelect->mptr->start;
			CurrentSelect->mptr->start = buf;
			WidgetPlace(hw, CurrentSelect->mptr, pcc);
/* free the fake, put the original back */
			free(buf);
			free(options);
			free(returns);
			free(value);
			CurrentSelect->mptr->start = start;

			free((char *)CurrentSelect);
			CurrentSelect = NULL;
			Ignore = 0;
		}
		else if ((!mark->is_end)&&(CurrentSelect == NULL)) {
			CurrentSelect = (SelectInfo *)malloc( sizeof(SelectInfo));
			CurrentSelect->hw = (Widget)hw;
			CurrentSelect->mptr = *mptr;
			CurrentSelect->option_cnt = 0;
			CurrentSelect->returns = NULL;
			CurrentSelect->retval_buf = NULL;
			CurrentSelect->options = NULL;
			CurrentSelect->option_buf = NULL;
			CurrentSelect->value_cnt = 0;
			CurrentSelect->value = NULL;
			CurrentSelect->is_value = -1;
			Ignore = 1;
		}
		break;

/*
 * TEXTAREA is a replacement for INPUT type=text size=rows,cols
 * because SGML will not allow an arbitrary length value
 * field.
 */
	case M_TEXTAREA:
		if (pcc->cur_form == NULL)
			break;
		if ((mark->is_end)&&(TextAreaBuf != NULL)) {
			char *start;
			char *buf;


/* Finish a fake INPUT tag. */
			buf = (char *)malloc( strlen(TextAreaBuf) + 2);
			strcpy(buf, TextAreaBuf);
			strcat(buf, "\"");
/* stick the fake in, saving the real one. */
			start = mark->start;
			mark->start = buf;
			mark->is_end = 0;
			WidgetPlace(hw, mark, pcc);

/* free the fake, put the original back */
			free(buf);
			free(TextAreaBuf);
			mark->start = start;
			mark->is_end = 1;
			TextAreaBuf = NULL;
			Ignore = 0;
		}else if ((!mark->is_end)&&(TextAreaBuf == NULL)) {
			char *buf;
			int len;

/* Construct  the start of a fake INPUT tag. */
			len = strlen(MT_INPUT) +
				strlen( " type=textarea value=\"\"");
			buf = (char *)malloc(len + strlen(mark->start)+1);
			strcpy(buf, MT_INPUT);
			strcat(buf, (char *) (mark->start + strlen(MT_TEXTAREA)));
			strcat(buf, " type=textarea");
			strcat(buf, " value=\"");
			TextAreaBuf = buf;
			Ignore = 1;
		}
		break;
/*
 * Just insert the widget.
 * Can only inside a FORM tag.
 * Special case the type=image stuff to become a special
 * IMG tag.
 */
	case M_INPUT:
		if (pcc->cur_form != NULL) {
			char *tptr2;

			tptr = ParseMarkTag((*mptr)->start, MT_INPUT, "TYPE");
			if ((tptr != NULL)&& (caseless_equal(tptr, "image"))) {
				free(tptr);
				tptr = (char *)malloc(
					strlen((*mptr)->start) +
					strlen(" ISMAP") +
					strlen(MT_IMAGE) -
					strlen(MT_INPUT) + 1);
				strcpy(tptr, MT_IMAGE);
				strcat(tptr, (char *)
					((*mptr)->start + strlen(MT_INPUT))
					);
				strcat(tptr, " ISMAP");
				tptr2 = (*mptr)->start;
				(*mptr)->start = tptr;
				ImagePlace(hw, *mptr, pcc);
				(*mptr)->start = tptr2;
				free(tptr);
			}
/* hidden inputs have no element associated with them, just a widget record. */
			else if ((tptr != NULL)&& (caseless_equal(tptr, "hidden")))
			{
				free(tptr);
				pcc->widget_id++;
				(void)MakeWidget(hw, (*mptr)->start, pcc,
					pcc->widget_id);
			} else {
				if (tptr != NULL) {
					free(tptr);
				}
				WidgetPlace(hw, *mptr, pcc);
			}
		}
		break;

/*
 * Fillout forms.  Cannot be nested.
 */
	case M_FORM:
		if ((mark->is_end)&&(pcc->cur_form != NULL)) {
			pcc->cur_form->end = pcc->widget_id;
			AddNewForm(hw, pcc->cur_form);
			pcc->cur_form = NULL;
			break;
		}
		if ((!mark->is_end)&&(pcc->cur_form == NULL)) {
			pcc->cur_form = (FormInfo *)malloc(sizeof(FormInfo));
			pcc->cur_form->next = NULL;
			pcc->cur_form->hw = (Widget)hw;
			pcc->cur_form->action = ParseMarkTag(mark->start,
				MT_FORM, "ACTION");
			pcc->cur_form->format = ParseMarkTag(mark->start,
				MT_FORM, "FORMAT");
                        pcc->cur_form->method = ParseMarkTag(mark->start,
                                MT_FORM, "METHOD");
                        pcc->cur_form->enctype = ParseMarkTag(mark->start,
                                MT_FORM, "ENCTYPE");
                        pcc->cur_form->enc_entity = ParseMarkTag(mark->start,
				MT_FORM, "ENCENTITY");
			pcc->cur_form->start = pcc->widget_id;
			pcc->cur_form->end = -1;
                        pcc->cur_form->button_pressed=NULL;
		}
		break;

/*
 * Addresses are just like headers.  A linefeed before and
 * after, and change the font.
 */
	case M_ADDRESS:
		LinefeedPlace(hw,*mptr,pcc);
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.address_font;
		}
		break;
/*
 * Plain and listing text.  A single pre-formatted chunk of text
 * in its own font.
 */
	case M_PREFORMAT:
	case M_LISTING_TEXT:
	case M_PLAIN_TEXT:
	case M_PLAIN_FILE:
		tmp_font = hw->html.listing_font;
		if (type == M_PLAIN_TEXT)
			tmp_font = hw->html.plain_font;
		if (type == M_PREFORMAT)
			tmp_font = hw->html.plain_font;
		if (type == M_PLAIN_FILE)
			tmp_font = hw->html.plain_font;
		if (mark->is_end) {
			LineBreak(hw,*mptr,pcc);
			pcc->preformat = 0;
			pcc->cur_font = PopFont();
			LinefeedPlace(hw,*mptr,pcc);
		} else {
			LineBreak(hw,*mptr,pcc);
			LinefeedPlace(hw,*mptr,pcc);
			pcc->preformat = 1;
			PushFont(pcc->cur_font);
			pcc->cur_font = tmp_font;
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
				free((char *)dptr);
			}
		} else {
			DescRec *dptr;

			LineBreak(hw,*mptr,pcc);
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
 * Now with forms, <INDEX> is the same as:
 * <FORM>
 * <HR>
 * This is a searchable index.  Enter search keywords:
 * <INPUT NAME="isindex">
 * <HR>
 * </FORM>
 * Also, <INDEX> will take an ACTION tag to specify a
 * different URL to submit the query to.
 */
	case M_INDEX:
		hw->html.is_index = True;
		if (pcc->cur_form != NULL)
			break; /* No index inside a form */

/* Start the form */
		LinefeedPlace(hw,*mptr,pcc);
		pcc->cur_form = (FormInfo *)malloc( sizeof(FormInfo));
		pcc->cur_form->next = NULL;
		pcc->cur_form->hw = (Widget)hw;
		pcc->cur_form->action = NULL;
		pcc->cur_form->action = ParseMarkTag(mark->start,MT_INDEX,"ACTION");
		pcc->cur_form->format = ParseMarkTag(mark->start,MT_INDEX,"FORMAT");
		pcc->cur_form->method = ParseMarkTag(mark->start,MT_INDEX,"METHOD");
		pcc->cur_form->enctype=ParseMarkTag(mark->start,MT_INDEX,"ENCTYPE");
		pcc->cur_form->enc_entity = ParseMarkTag(mark->start, 
						MT_INDEX, "ENCENTITY");
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

	case M_LINEBREAK:
		LinefeedPlace(hw,*mptr,pcc); /* miss named break!!! */
		break;
	case M_TABLE:
		TablePlace(hw, mptr, pcc);
		break;
	case M_FIGURE:
	case M_IMAGE:		/* Just insert the image for now */
		if (mark->is_end)
			return;
		ImagePlace(hw, *mptr, pcc);
		break;
	case M_APROG:
		if ((*mptr)->is_end) 		/* end of aprog */
			return;
		AprogPlace(hw,mptr,pcc,save_obj);
		break;
	case M_SMALL:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
                        pcc->cur_font = hw->html.supsub_font;
		}
		break;
	case M_BIG:
		if (mark->is_end) {
			pcc->cur_font = PopFont();
		} else {
			PushFont(pcc->cur_font);
			pcc->cur_font = hw->html.header1_font;
		}
		break;
	case M_FONT:
		printf("Tag <FONT> not yet implemented\n");
		break;
	case M_MAP:
		printf("Tag <MAP> not yet implemented\n");
		break;
	case M_AREA:
		printf("Tag <AREA> not yet implemented\n");
		break;

	case M_HTML:			/* don't know what to do with */
	case M_COMMENT:
	case M_PARAM:			/* maybe seen in APROG */
	case M_CAPTION:
	case M_TABLE_HEADER:
	case M_TABLE_DATA:		/* <TD> peut reaparaitre dans une */
					/* analyse de <TABLE> */
	case M_DOCTYPE:			/* unused */
	case M_META:			/* unused */
		break;
	default:
		fprintf(stderr,"[TriggerMarkChanges] Unknow marker %d\n",
			mark->type);
		break;
	}
} /* TriggerMarkChanges() */

/*############# */
	/* copy and push the state */
	/* cur_hst = *(chst); */
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
/* ####################### */
/* ################# voir peut etre la reentrance ici en fonction des tags####*/
/* ####################### */
		TriggerMarkChanges(hw, &mptr, pcc , save_obj);
		if (mptr == end_mark){
			return;
		}
		if (mptr)
			mptr = mptr->next;
	}
}

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

	if (htmlwTrace) {
		gettimeofday(&Tv, &Tz);
		fprintf(stderr,"FormatAll enter (%d.%d)\n",Tv.tv_sec,Tv.tv_usec);
	}
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
	pcc.line_number = 1;  /* current line_number */
	pcc.is_bol = True;	/* we are at begin of line */
	pcc.have_space_after = False;	/* remember if a word have a space after*/
	pcc.cur_font = hw->html.font;
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
	pcc.widget_id = 0;
	pcc.aprog_id = 0;
        pcc.superscript = 0;
        pcc.subscript = 0;
	pcc.indent_level = 0;
	pcc.internal_mc_eo = 0;
	pcc.parent_html_object_desc = NULL;

/* Initialize local variables, some from the widget */
	Ignore = 0;
	Strikeout = False;
	DescType = &BaseDesc;
	DescType->type = D_NONE;
	DescType->count = 0;
	DescType->compact = 0;
	DescType->next = NULL;
	DescType->save_left_margin = pcc.left_margin;
	DescType->indent_margin = pcc.left_margin;
	DescType->save_cur_line_width = pcc.cur_line_width;
	DescType->cur_line_width = pcc.cur_line_width;
	CurrentSelect = NULL;
	TextAreaBuf = NULL;
	InDocHead = 0;
	if (hw->html.title != NULL) { /* Free the old title, if there is one. */
		free(hw->html.title);
		hw->html.title = NULL;
	}
/* #### memory leak ### */
	TitleText = NULL;
#ifdef MULTICAST
        if((hw->html.mc_wtype == MC_MO_TYPE_MAIN) && mc_send_enable)
                mc_global_eo_count = 0;
#endif
/* Free up previously formatted elements */
	FreeLineList(hw->html.formatted_elements,(Widget)hw, save_obj);
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
	FontStack = &FontBase;
	FontStack->font = hw->html.font;
/*If we have parsed special header text, fill it in now.*/
	if (hw->html.html_header_objects != NULL) {
		FormatChunk(hw, hw->html.html_header_objects,
			NULL, &pcc,save_obj);
		pcc.cur_font = hw->html.font;
		LinefeedPlace(hw,NULL,&pcc);
	}

 					/* Format all objects for width */
	FormatChunk(hw,hw->html.html_objects,NULL,&pcc,save_obj);

		/*If we have parsed special footer text, fill it in now.*/
	if (hw->html.html_footer_objects != NULL) {
		pcc.cur_font = hw->html.font;
		pcc.preformat = 0;
		pcc.have_space_after = 0;
		LinefeedPlace(hw,NULL,&pcc);
		FormatChunk(hw, hw->html.html_footer_objects,
			NULL, &pcc,save_obj);
	}
			/* Ensure a linefeed after the final element. */
	LinefeedPlace(hw,NULL,&pcc);
			/* Add the bottom margin to the max height. */
	pcc.y = pcc.y + hw->html.margin_height;

/* If the passed in MaxWidth was wrong, correct it.*/
	if (pcc.max_width_return > saved_width)
		*Fwidth = pcc.max_width_return;

	/* if height is too height tell the wiget to use the vbar */
	if ( pcc.y > hw->core.height - HbarHeight(hw) ){
		hw->html.use_vbar = True;
	} else {
		hw->html.use_vbar = False;
	}
	
	if (htmlwTrace) {
		gettimeofday(&Tv, &Tz);
		fprintf(stderr,"FormatAll exit (%d.%d)\n", Tv.tv_sec, Tv.tv_usec);
	}
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
	case E_APROG:
		AprogRefresh(hw,eptr);
		break;
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
			if (eptr->pic_data->fetched ) {
				if((x >= tx1)&&(x <= tx2)&&(y >= ty1)&&(y <=ty2)){
					rptr = eptr;
				}
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

/*
 * Used by ParseTextToPrettyString to let it be sloppy about its
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
		if (*str == NULL)
			return;
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
		if (buf == NULL)
			return;
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
char * ParseTextToString( struct ele_rec * elist,
	struct ele_rec *startp, struct ele_rec *endp,
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

/*
 * Parse all the formatted text elements from start to end
 * into an ascii text string, and return it.
 * Very like ParseTextToString() except the text is prettied up
 * to show headers and the like.
 * space_width and lmargin tell us how many spaces to indent lines.
 */
char * ParseTextToPrettyString( HTMLWidget hw, struct ele_rec * elist,
	struct ele_rec *startp, struct ele_rec *endp,
	int start_pos, int end_pos,
	int space_width, int lmargin)
{
	int line;
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

	/*
	 * We need to know if we should consider the indentation or bullet
	 * that might be just before the first selected element to also be
	 * selected.  This current hack looks to see if they selected the
	 * Whole line, and assumes if they did, they also wanted the beginning.
	 *
	 * If we are at the beginning of the list, or the beginning of
	 * a line, or just behind a bullett, assume this is the start of
	 * a line that we may want to include the indent for.
	 */
	if ((start_pos == 0)&&
	    ((start->prev == NULL)||(start->prev->type == E_BULLET)||
	    (start->prev->line_number != start->line_number))) {
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
	line = eptr->line_number;
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
			if (eptr->font == hw->html.header1_font) {
				lchar = '*';
			}
			else if (eptr->font == hw->html.header2_font) {
				lchar = '=';
			}
			else if (eptr->font == hw->html.header3_font) {
				lchar = '+';
			}
			else if (eptr->font == hw->html.header4_font) {
				lchar = '-';
			}
			else if (eptr->font == hw->html.header5_font) {
				lchar = '~';
			}
			else if (eptr->font == hw->html.header6_font) {
				lchar = '.';
			}
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
			if (eptr->font == hw->html.header1_font) {
				lchar = '*';
			}
			else if (eptr->font == hw->html.header2_font) {
				lchar = '=';
			}
			else if (eptr->font == hw->html.header3_font) {
				lchar = '+';
			}
			else if (eptr->font == hw->html.header4_font) {
				lchar = '-';
			}
			else if (eptr->font == hw->html.header5_font) {
				lchar = '~';
			}
			else if (eptr->font == hw->html.header6_font) {
				lchar = '.';
			}
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
		if (last->font == hw->html.header1_font) {
			lchar = '*';
		}
		else if (last->font == hw->html.header2_font) {
			lchar = '=';
		}
		else if (last->font == hw->html.header3_font) {
			lchar = '+';
		}
		else if (last->font == hw->html.header4_font) {
			lchar = '-';
		}
		else if (last->font == hw->html.header5_font) {
			lchar = '~';
		}
		else if (last->font == hw->html.header6_font) {
			lchar = '.';
		}
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
	int listing_text;
	int pcnt, lcnt, pwidth, lwidth;
	int width;
	char *ptr;

	/* Loop through object list looking at the plain, preformatted,
	 * and listing text
	 */
	width = 0;
	pwidth = 0;
	lwidth = 0;
	plain_text = 0;
	listing_text = 0;
	mptr = list;
	while (mptr != NULL) {
		/* All text blocks between the starting and ending
		 * plain and pre text markers are plain text blocks.
		 * Manipulate flags so we recognize these blocks.
		 */
		if((mptr->type == M_PLAIN_TEXT)|| (mptr->type == M_PLAIN_FILE)||
		   (mptr->type == M_PREFORMAT)) {
			if (mptr->is_end) {
				plain_text--;
				if (plain_text < 0)
					plain_text = 0;
			} else
				plain_text++;
			pcnt = 0;
			lcnt = 0;
		}
		/* All text blocks between the starting and ending
		 * listing markers are listing text blocks.
		 */
		else if (mptr->type == M_LISTING_TEXT) {
			if (mptr->is_end) {
				listing_text--;
				if (listing_text < 0)
					listing_text = 0;
			} else
				listing_text++;
			lcnt = 0;
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
		/*
		 * If this is a listing text block, add to line length.
		 * Find the Max of all line lengths.
		 */
		else if ((listing_text)&&(mptr->type == M_NONE)) {
			ptr = mptr->text;
			while ((ptr != NULL)&&(*ptr != '\0')) {
				ptr = MaxTextWidth(ptr, &lcnt);
				if (lcnt > lwidth)
					lwidth = lcnt;
			}
		}
		mptr = mptr->next;
	} /* while */
	width = pwidth * hw->html.plain_font->max_bounds.width;
	lwidth = lwidth * hw->html.listing_font->max_bounds.width;
	if (lwidth > width)
		width = lwidth;
	return(width);
}
