/* Most of code is rewrite from scratch. Lot of code come from NCSA Mosaic */

#include "copyright.ncsa"

 /* Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */


/* HTMLtext.c
 * Author: Gilles Dauphin
 * Version 3.0 [Sep96]
 *
 * Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :  dauphin@sig.enst.fr
 */

#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#define SKIPWHITE(s)    while( (((unsigned char)*s) < 128) && (isspace(*s)) ) s++
#define SKIPNONWHITE(s) while( (((unsigned char)*s) > 127) ||  \
			       ((!isspace(*s))&&(*s )) )s++

#define COMP_LINE_BUF_LEN 1024

void LineBreak(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc)
{
	if(pcc->is_bol)
		return;
	LinefeedPlace(hw,mptr,pcc);
}

/* We have encountered a line break.  Increment the line counter,
 * and move down some space.
 */
void LinefeedPlace(HTMLWidget hw, struct mark_up * mptr,
		PhotoComposeContext * pcc)
{
	struct ele_rec *eptr;
	struct ele_rec *septr;
	int adjx = 0;

	/* We need to center or right adjust the element here. */
	/* compute the total with and the remaining x offset to center. */

	if ((pcc->div == DIV_ALIGN_CENTER) || (pcc->div == DIV_ALIGN_RIGHT)) {
		adjx = pcc->cur_line_width - 
			(pcc->x - pcc->eoffsetx - pcc->left_margin) ;
		if(pcc->div == DIV_ALIGN_CENTER)
			adjx = adjx/2;
	}

	if (pcc->cw_only) {	/* compute width only , dont create Element*/
		if (pcc->x > pcc->computed_max_x)
			pcc->computed_max_x = pcc->x;
		pcc->y = pcc->y + pcc->cur_line_height;
		pcc->have_space_after = 0;
		pcc->x = pcc->left_margin + pcc->eoffsetx;
		pcc->cur_baseline = pcc->cur_font->ascent;
		pcc->cur_line_height=pcc->cur_font->ascent+pcc->cur_font->descent;
		pcc->is_bol = 1;
		return;
#ifdef REMEMBER
if (pcc->cw_only) {	/* compute width only , dont create Element*/
/* deb_pcc.computed_min_x = 0; /* celui-ci pour MOT IMAGE APROG APPLET*/
/* attention aussi au increment genre MENU*/
/* UL OL etc... */
}
#endif
	}

	/* linefeed is at end of line */
	eptr = CreateElement(hw, E_LINEFEED, pcc->cur_font,
		pcc->x, pcc->y, 0, pcc->cur_line_height, pcc->cur_baseline, pcc);
/*	eptr->anchor_tag_ptr = pcc->anchor_tag_ptr; */
		
	if(adjx > 0 ){
		/* adjust */
		/* Back to the list until CR and adjust each x with the adjx. */
		septr = eptr;
		while( septr && (septr->type != E_CR) ){
			septr->x = septr->x + adjx;
			septr = septr->prev;
		}
		pcc->x = pcc->x + adjx;
	}

	/* At the end of every line check if we have a new MaxWidth */
	if( pcc->x  > pcc->max_width_return )
		pcc->max_width_return = pcc->x ;

	pcc->y = pcc->y + pcc->cur_line_height;
	pcc->have_space_after = 0;
	pcc->x = pcc->left_margin + pcc->eoffsetx;
	pcc->cur_baseline = pcc->cur_font->ascent;
	pcc->cur_line_height = pcc->cur_font->ascent+pcc->cur_font->descent;
	pcc->is_bol = 1;
	/* CR is at begin of line */
	eptr = CreateElement(hw, E_CR, pcc->cur_font,
		pcc->x, pcc->y,
		0, pcc->cur_line_height,
		pcc->cur_baseline, pcc);
/*	eptr->anchor_tag_ptr = pcc->anchor_tag_ptr; */
}

/* Redraw a linefeed. Basically a filled rectangle at the end of a line. */

void LinefeedRefresh( HTMLWidget hw, struct ele_rec *eptr)
{
	if (eptr->selected == True) {
		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
	} else {
		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->bg);
	}
}
static void Translate_Nbsp( char * t)
{
	while(*t){
		if(*t == NBSP_CONST)
			*t = ' ';
		t++;
	}
}
		
/* remove blank, LF, CR, TAB */
static char ** split_in_word( char * text, unsigned int * nw,
		int * hsb4, int * hsa)
{
	unsigned int nword = 0;
	char * fin;
	char * deb;
	int len;
	char ** words;
	char * w_text;
	int i;
	char * pword;

	len = strlen(text);
	*hsb4 = 0;
	*hsa = 0;
	*nw = 0;
	deb = text;
	if( (((unsigned char)*text) < 128) && (isspace(*text)) )
		*hsb4 = 1;
	fin = text + len - 1 ;
	if( (((unsigned char)*fin) < 128) && (isspace(*fin)) )
		*hsa = 1;

				/* count the number of word */
	while(*deb != '\0'){
		SKIPWHITE(deb);
		if (*deb == '\0') break;
		SKIPNONWHITE(deb);
		nword++;
	}
	if (nword == 0 )
		return NULL;
	*nw = nword;
	words = (char **) malloc( nword * sizeof(char*));
	w_text = strdup(text);
	deb =w_text;
	for(i=0; i<nword; i++){
		SKIPWHITE(deb);
		pword = deb;
		SKIPNONWHITE(deb);
		*deb = '\0';
		deb++;
		words[i] = strdup(pword);
	}
	for(i=0; i<nword; i++){
		Translate_Nbsp(words[i]);	/* set nbsp to white */
	}
	free(w_text);
	return words;
}

void Set_E_TEXT_Element(HTMLWidget hw,
		struct ele_rec *eptr, char *text, PhotoComposeContext *pcc)
{
	int len;

	len = strlen(text) + 1;
	eptr->edata = strdup(text);
	CHECK_OUT_OF_MEM(eptr);
	eptr->edata_len = len;
			/* if this is an anchor, puts its href and name */
			/* values into the element.  */
	eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
}

 
/* Formate et place un morceau de text.
 * Le contexte est donne par pcc :
 *	pcc->x , pcc->y : c'est la qu'il faut placer le texte.
 *			  C'est le x,y dans view.
 *	pcc->width_of_viewable_part	: the width of viewable part (view)
 *
 *	pcc->ex , pcc->ey : C'est la qu'il faut placer le texte
 *			    Si le texte est a l'interieur d'un
 *			    element (comme un tableau par exemple).
 *			    ex , ey en coordonne relative de l'element.
 *
 *	pcc->cur_line_width	: # of pixel for the photo composed
 *				  line
 *	pcc->eoffsetx + pcc->left_margin : where to go when new line. relative
 *				  to view.
 *	pcc->right_margin : marge a droite
 * 	pcc->eoffsety	: offset par rapport a view
 *	...
 * Si le texte est dans un element on retouve facilement la geometry
 * de cet element :
 *	La position en haut a gauche par rapport a 'view' est:
 *		pcc->eoffsetx = offsetx de l'element (0 pour view)
 *		pcc->y - pcc->ey = offsety 
 *		pcc->cur_line_width + right_margin+left_margin = sa largeur
 *
 * Place le texte qui est dans mptr->text a partir de [pcc->x,pcc->y]
 * sachant que la partie visible est de largeur pcc->width
 *
 * Ca mets a jour des valeurs dans pcc: mais lesquelles ???
 */

void PartOfTextPlace(HTMLWidget hw, 	/* the widget */
	struct mark_up *mptr,		/* the texte markup */
	PhotoComposeContext * pcc)	/* le contexte pour cet element*/
{
	char 	*text;
	int	have_space_b4 = 0;
	char	**words;
	unsigned int	nword;
	int 	have_space_after = 0;
	char	is_bol;
	char * composed_line = NULL;
	int 	composed_line_width =0;
	char * the_word;
	int	word_width, font_height;
	int 	i;
	struct ele_rec *eptr;
	int 	baseline;

	text = strdup(mptr->text);	/* save text */
	is_bol = pcc->is_bol;

	words = split_in_word(text,&nword, &have_space_b4, &have_space_after);
	if (nword == 0 ){
		pcc->have_space_after = have_space_after;
		free(text);
		return;
	}
	if (pcc->have_space_after && !have_space_b4)
		have_space_b4 = 1;
	if (is_bol)		/* si on est en begin of line */
		have_space_b4 = 0;

				/* alloc enought space to compose a line */
	composed_line = (char*) malloc( strlen(text) + nword*3 + 1);
	composed_line[0]='\0';
	composed_line_width = 0;	/* en pixel */
	the_word = (char*) malloc( strlen(text) + nword*3 + 1);
	the_word[0]='\0';

	for(i = 0; i < nword; i++){	/* remplir la ligne de mots */
		if(have_space_b4){
			the_word[0]= ' ';
			the_word[1] = '\0';
		} else {
			the_word[0] = '\0';
		}
		strcat(the_word,words[i]);
		word_width =XTextWidth(pcc->cur_font,the_word,strlen(the_word));
		font_height = pcc->cur_font->ascent +pcc->cur_font->descent;
		baseline = pcc->cur_font->ascent;
		if(pcc->computed_min_x < 
		   (word_width+pcc->eoffsetx+pcc->left_margin)){
			pcc->computed_min_x = word_width + pcc->eoffsetx + 
					      pcc->left_margin;
		}
				/*plusieurs cas se presente */
		if (is_bol && (word_width >= pcc->cur_line_width)){
				/* le mot est + grand que la ligne */
			if(!pcc->cw_only){
				eptr = CreateElement(hw, E_TEXT,pcc->cur_font,
					pcc->x, pcc->y, 
					word_width, font_height,baseline,pcc);
				Set_E_TEXT_Element(hw,eptr,the_word,pcc);
				AdjustBaseLine(hw,eptr,pcc); 
						   /* aligne les elements sur */
						   /* leur baseline */
			} else {
        			if (pcc->cur_line_height < font_height)
                        		pcc->cur_line_height = font_height;
        		}

			pcc->pf_lf_state = 0;
			have_space_b4 =0;
			is_bol = 1;
			the_word[0]='\0';
			pcc->x = pcc->x + word_width;
			LinefeedPlace(hw, mptr,pcc);	 /* line feed change le contexte*/
			composed_line_width = 0;
			continue;
		}
		if((composed_line_width == 0) && 
		   (pcc->x - pcc->eoffsetx - pcc->left_margin + word_width > pcc-> cur_line_width)){
				/* position courante + le mot: c'est trop big*/
				/* alors qu'on a rien composer encore ! */
				/* ca arrive quand des tag de font sont */
				/* a interpreter */
			if(!pcc->cw_only){
				AdjustBaseLine(hw,hw->html.last_formatted_elem,pcc);
			}else { 
                                if (pcc->cur_line_height < font_height)
                                        pcc->cur_line_height = font_height;
                        }
				/* linefeed tout court*/
			pcc->pf_lf_state = 0;
			have_space_b4 =0;
			is_bol = 1;
			the_word[0]='\0';
			LinefeedPlace(hw, mptr,pcc); /* line feed change le contexte*/
			composed_line_width = 0;
			have_space_b4 = 0;
			i--;
			continue;
		}
		if(pcc->x - pcc->eoffsetx - pcc->left_margin + composed_line_width + word_width > 
		    pcc->cur_line_width){
				/* position courante +le mot + la ligne : c'est trop grand */
				/* on flush la ligne */
			if(!pcc->cw_only){
				eptr = CreateElement(hw, E_TEXT,pcc->cur_font,
                                	pcc->x, pcc->y,
                                	composed_line_width,
					font_height,baseline,pcc);
				Set_E_TEXT_Element(hw, eptr, composed_line,pcc);
				AdjustBaseLine(hw,eptr,pcc); 
			} else {              
                                if (pcc->cur_line_height < font_height)
                                        pcc->cur_line_height = font_height;
                        } 
			composed_line[0] = '\0';
                                /* linefeed tout court*/
                        pcc->pf_lf_state = 0;
                        have_space_b4 =0;
                        is_bol = 1;
                        the_word[0]='\0';
			pcc->x = pcc->x + composed_line_width;
			LinefeedPlace(hw, mptr,pcc); /* line feed change le contexte*/
			composed_line_width = 0;
                        have_space_b4 = 0;
                        i--;
                        continue;
		}
					/* le mot rentre */
		strcat(composed_line,the_word);
		have_space_b4 = 1;		/* pour le prochain mot */
		composed_line_width += word_width;
		is_bol = 0;
		the_word[0]='\0';
	}
	if (composed_line_width != 0) { /* ya des choses a flushher */
		if(!pcc->cw_only){
			eptr = CreateElement(hw, E_TEXT,pcc->cur_font, 
                          	pcc->x, pcc->y,
                          	composed_line_width, font_height,baseline,pcc);
			Set_E_TEXT_Element(hw, eptr, composed_line,pcc);
			AdjustBaseLine(hw,hw->html.last_formatted_elem,pcc);
		} else {              
                                if (pcc->cur_line_height < font_height)
                                        pcc->cur_line_height = font_height;
                        } 
		pcc->x = pcc->x + composed_line_width;
		is_bol =0;
	}
	if (pcc->x > pcc->computed_max_x)
		pcc->computed_max_x = pcc->x;
/* ajuster le contexte pcc */
	pcc->is_bol = is_bol;
	pcc->have_space_after = have_space_after;

	for(i=0; i<nword;i++)
		free(words[i]);
	free(words);

	free(the_word);
	free(composed_line);
	free(text);
}


/*
 * Place a piece of pre-formatted text. Add an element record for it.
 */

void PartOfPreTextPlace(HTMLWidget hw, 	/* the widget */
	struct mark_up *mptr,		/* the texte markup */
	PhotoComposeContext * pcc)	/* le contexte pour cet element*/
{
	char *end;
	struct ele_rec *eptr;
	int ntab, char_cnt;
	char *line;
	int tmp_cnt=0;
	int cur_char_in_line=0;
	int line_str_len;
	int i;
	int font_height;
	int line_width=0;

	font_height = pcc->cur_font->ascent+pcc->cur_font->descent;
	line = (char*) malloc(COMP_LINE_BUF_LEN);
	line_str_len = COMP_LINE_BUF_LEN;
	line[0] = '\0';
	end = mptr->text;
	char_cnt =0;
	while (*end != '\0') {
		if ((*end == '\r')||(*end == '\f')) { /* Throw out CR and FF */
			end++;
			continue;
		} 
		if(*end == '\n'){	/*line break */
			if (line[0] != '\0') {
				line_width = XTextWidth(pcc->cur_font,
					line, strlen(line));
			    if(pcc->computed_min_x < 
		              (line_width+pcc->eoffsetx+pcc->left_margin)){
			         pcc->computed_min_x= line_width + pcc->eoffsetx +
					      pcc->left_margin;
			    }
			    if(!pcc->cw_only){
				eptr = CreateElement(hw, E_TEXT,
					pcc->cur_font,
                               		pcc->x, pcc->y, line_width,
					pcc->cur_font->ascent+pcc->cur_font->descent,
					pcc->cur_font->ascent,pcc);
				Set_E_TEXT_Element(hw, eptr, line,pcc);
				AdjustBaseLine(hw,eptr,pcc);
			    } else {              
                                if (pcc->cur_line_height < font_height)
                                        pcc->cur_line_height = font_height;
                            } 
				pcc->pf_lf_state = 0;
			}
			end++;
			pcc->x = pcc->x + line_width;
			LinefeedPlace(hw, mptr,pcc); /* line feed change le contexte*/
			char_cnt =0;
			line[0] = '\0';
			cur_char_in_line=0;
			continue;
		}
		/*
		 * Should be only spaces and tabs here, so if it
		 * is not a tab, make it a space.
		 * Break on linefeeds, they must be done separately
		 */
		if (*end == '\t') {
			tmp_cnt= ((char_cnt/8) + 1) * 8;
			ntab = tmp_cnt - char_cnt;
			char_cnt += ntab;
			if (char_cnt +1 > line_str_len) {
				line_str_len += char_cnt+1;
				line = (char *)realloc(line,line_str_len);
			}
			/*
			 * If we have any tabs, expand them into spaces.
			 */
			for(i=0; i<ntab; i++)
				line[cur_char_in_line++]=' ';
			line[cur_char_in_line]='\0';
			end++;
			continue;
		}
		if (char_cnt +1 > line_str_len) {
			line_str_len += char_cnt+1;
			line = (char *)realloc(line,line_str_len);
		}
		line[cur_char_in_line++]=*end;
		line[cur_char_in_line]='\0';
		char_cnt++;
		end++;
	}
	if (line[0] != '\0') {
		line_width = XTextWidth(pcc->cur_font,
				line, strlen(line));
	    if(!pcc->cw_only){
		eptr = CreateElement(hw, E_TEXT,
			pcc->cur_font,
               		pcc->x, pcc->y, line_width,
			pcc->cur_font->ascent+pcc->cur_font->descent,
			pcc->cur_font->ascent,pcc);
		Set_E_TEXT_Element(hw, eptr, line,pcc);
		AdjustBaseLine(hw,eptr,pcc);
	    } else {
                     if (pcc->cur_line_height < font_height)
                                pcc->cur_line_height = font_height;
            }
		pcc->x = pcc->x + line_width;
		pcc->is_bol =0;
		pcc->pf_lf_state = 0;
	}
	if (pcc->x > pcc->computed_max_x)
		pcc->computed_max_x = pcc->x;
	pcc->have_space_after = 0;
	free(line);
}

/*
 * Redraw part of a formatted text element, in the passed fg and bg
 */
void PartialRefresh(HTMLWidget hw, struct ele_rec *eptr,
	int start_pos, int end_pos, unsigned long fg, unsigned long bg)
{
	int ascent;
	char *tdata;
	int tlen;
	int x, y, width;
	int partial, descent;
	int dir, nascent, ndescent;
	XCharStruct all;
	int height;

	XSetFont(XtDisplay(hw), hw->html.drawGC, eptr->font->fid);
	ascent = eptr->font->max_bounds.ascent;
	width = -1;
	partial = 0;

	x = eptr->x;
	tdata = (char *)eptr->edata;

	if (start_pos != 0) {
		XTextExtents(eptr->font, (char *)eptr->edata,
			start_pos, &dir, &nascent, &descent, &all);
		x = eptr->x + all.width;
		tdata = (char *)(eptr->edata + start_pos);
		partial = 1;
	}

	tlen = eptr->edata_len - start_pos - 1;
	if (end_pos != (eptr->edata_len - 2)) {
		tlen = end_pos - start_pos + 1;
		partial = 1;
	}

	y = eptr->y ;
	x = x - hw->html.scroll_x;
	y = y - hw->html.scroll_y;

	/*
	 * May be safe to used the cached full width of this
	 * string, and thus avoid a call to XTextExtents
	 */
	if ((!partial)&&(eptr->width != 0)) {
		all.width = eptr->width;
	} else {
		XTextExtents(eptr->font, (char *)tdata,
			tlen, &dir, &nascent, &ndescent, &all);
	}
	XSetForeground(XtDisplay(hw), hw->html.drawGC, bg);

	height = eptr->height;
	if (height > 0) {
		if(!hw->html.bg_image)
			XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
				hw->html.drawGC, x, y,
				(unsigned int)all.width, (unsigned int)height);
	}
	width = all.width;

	if(bg!=hw->html.view->core.background_pixel ||
	   !hw->html.body_images || !hw->html.bg_image) {
		XSetForeground(XtDisplay(hw), hw->html.drawGC, bg);
		XSetBackground(XtDisplay(hw), hw->html.drawGC, fg);

		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
			       hw->html.drawGC,
			       x, y,
			       width, height);
		XSetForeground(XtDisplay(hw), hw->html.drawGC, fg);
		XSetBackground(XtDisplay(hw), hw->html.drawGC, bg);
		XDrawString(XtDisplay(hw), XtWindow(hw->html.view),
			    hw->html.drawGC,
			    x, y + eptr->baseline,
			    (char *)tdata, tlen);
	} else {
		XSetForeground(XtDisplay(hw), hw->html.drawGC, bg);
		XSetBackground(XtDisplay(hw), hw->html.drawGC, fg);
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
			       hw->html.drawGC,
			       x, y,
			       width, height);
		XSetForeground(XtDisplay(hw), hw->html.drawGC, fg);
		XSetBackground(XtDisplay(hw), hw->html.drawGC, bg);
		HTMLDrawBackgroundImage((Widget)hw,
					(x<0 ? 0 : x), (y<0 ? 0 : y),
					(x<0 ? (width+x) : width),
					height );
		XDrawString(XtDisplay(hw), XtWindow(hw->html.view),
			    hw->html.drawGC,
			    x, y + eptr->baseline,
			    (char *)tdata, tlen);
	}
	if (eptr->underline_number) {
		int i, ly;

		if (eptr->dashed_underline) {
			XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
				LineOnOffDash, CapButt, JoinBevel);
		} else {
			XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
				LineSolid, CapButt, JoinBevel);
		}
		ly = (int)(y + (eptr->baseline + eptr->height)/2);
		for (i=0; i<eptr->underline_number; i++) {
			XDrawLine(XtDisplay(hw), XtWindow(hw->html.view), 
				hw->html.drawGC,
				x, ly, (int)(x + width), ly);
			ly -= 2;
		}
	}
	if (eptr->strikeout == True) {
		int ly;

		XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);
		ly = (int)(y + eptr->height/2);
		XDrawLine(XtDisplay(hw), XtWindow(hw->html.view),
			hw->html.drawGC,
			x, ly, (int)(x + width), ly);
	}
}


/* Redraw a formatted text element */

void TextRefresh( HTMLWidget hw, struct ele_rec *eptr,
        int start_pos, int end_pos)
{
	if (eptr->selected == False) {
		PartialRefresh(hw, eptr,
			start_pos, end_pos,
			eptr->fg, eptr->bg);
		return;
	}

/*eptr->selected == True */
	if ((start_pos >= eptr->start_pos)&&(end_pos <= eptr->end_pos)) {
		PartialRefresh(hw, eptr,
			start_pos, end_pos,
			eptr->bg, eptr->fg);
		return;
	}

	if (start_pos < eptr->start_pos) {
		PartialRefresh(hw, eptr,
			start_pos, eptr->start_pos - 1,
			eptr->fg, eptr->bg);
		start_pos = eptr->start_pos;
	}
	if (end_pos > eptr->end_pos) {
		PartialRefresh(hw, eptr,
			eptr->end_pos + 1, end_pos,
			eptr->fg, eptr->bg);
			end_pos = eptr->end_pos;
	}
	PartialRefresh(hw, eptr,
		start_pos, end_pos,
		eptr->bg, eptr->fg);
}

/* Place a horizontal rule across the page.
 * Create and add the element record for it.
 */

void HRulePlace( HTMLWidget hw, struct mark_up * mptr,
        PhotoComposeContext * pcc)
{
	struct ele_rec * eptr;

        pcc->have_space_after = 0;
        pcc->x = pcc->eoffsetx + pcc->left_margin;
	if(pcc->computed_min_x < (1+pcc->eoffsetx+pcc->left_margin)){
		pcc->computed_min_x = 1 + pcc->eoffsetx + pcc->left_margin;
	}
	if (pcc->x + pcc->cur_line_width > pcc->computed_max_x)
		pcc->computed_max_x = pcc->x + pcc->cur_line_width;
	if (!pcc->cw_only){
		eptr = CreateElement(hw, E_HRULE, pcc->cur_font, 
			pcc->x, pcc->y, pcc->cur_line_width, 
			pcc->cur_font->ascent+pcc->cur_font->descent,
			pcc->cur_font->ascent, pcc);
		eptr->underline_number = 0; /* Rules can't be underlined! */
	}
}

/*
 * Redraw a formatted horizontal rule element
 */
void HRuleRefresh( HTMLWidget hw, struct ele_rec *eptr)
{
	int width, height;
	int x1, y1;

	width = (int)hw->html.view_width - (int)(2 * hw->html.margin_width);
	width = eptr->width;
	if (width < 0)
		width = 0;
	x1 = eptr->x;
	y1 = eptr->y;
	x1 = x1 - hw->html.scroll_x;
	y1 = y1 - hw->html.scroll_y;
	height = eptr->height;

	/* blank out area */
	XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->bg);
	if(!hw->html.bg_image)
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
			hw->html.drawGC, x1, y1, width, height);
	y1 = y1 + (height / 2) - 1;

	XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
		LineSolid, CapButt, JoinBevel);
	XDrawLine(XtDisplay(hw), XtWindow(hw->html.view),
		hw->manager.bottom_shadow_GC,
		x1, y1, (int)(x1 + width), y1);
	XDrawLine(XtDisplay(hw), XtWindow(hw->html.view),
		hw->manager.top_shadow_GC,
		x1, y1 + 1, (int)(x1 + width), y1 + 1);
}

/* Place the bullet at the beginning of an unnumbered
 * list item. Create and add the element record for it.
 */
void BulletPlace( HTMLWidget hw, struct mark_up * mptr,
        PhotoComposeContext * pcc)
{
	struct ele_rec * eptr;

        pcc->have_space_after = 0;
	if(pcc->computed_min_x < (pcc->cur_font->max_bounds.width+pcc->eoffsetx+pcc->left_margin)){
		pcc->computed_min_x = pcc->cur_font->max_bounds.width + pcc->eoffsetx + pcc->left_margin;
	}
	if (pcc->x + pcc->cur_font->max_bounds.width > pcc->computed_max_x)
		pcc->computed_max_x = pcc->x + pcc->cur_font->max_bounds.width;
	if (!pcc->cw_only){
		eptr = CreateElement(hw, E_BULLET, pcc->cur_font, 
			pcc->x, pcc->y, pcc->cur_font->max_bounds.width, 
			pcc->cur_font->ascent+pcc->cur_font->descent,
			pcc->cur_font->ascent, pcc);
		eptr->underline_number = 0; /* Bullets can't be underlined! */
	}
/*	pcc->x = pcc->x + pcc->cur_font->max_bounds.width; ###*/
	pcc->x = pcc->x ;
	pcc->is_bol = 0;
}

/* Redraw a formatted bullet element */
void BulletRefresh( HTMLWidget hw, struct ele_rec *eptr)
{
	int width, line_height;
	int x1, y1;

	width = eptr->width;
	line_height = eptr->height;
	x1 = eptr->x;
	y1 = eptr->y + eptr->baseline - width/2;
	x1 = x1 - hw->html.scroll_x;
	y1 = y1 - hw->html.scroll_y;
	XSetFont(XtDisplay(hw), hw->html.drawGC, eptr->font->fid);
	XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
	XSetBackground(XtDisplay(hw), hw->html.drawGC, eptr->bg);
	if (eptr->indent_level < 2) {
		XFillArc(XtDisplay(hw), XtWindow(hw->html.view), hw->html.drawGC,
			(x1 - width), y1,
			(width / 2), (width / 2), 0, 23040);
	} else if (eptr->indent_level == 2) {
		XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);
		XDrawRectangle(XtDisplay(hw), XtWindow(hw->html.view), 
			hw->html.drawGC,
			(x1 - width), y1,
			(width / 2), (width / 2));
	} else if (eptr->indent_level > 2) {
		XSetLineAttributes(XtDisplay(hw), hw->html.drawGC, 1,
			LineSolid, CapButt, JoinBevel);
		XDrawArc(XtDisplay(hw), XtWindow(hw->html.view),
			hw->html.drawGC,
			(x1 - width), y1,
			(width / 2), (width / 2), 0, 23040);
	}
}

