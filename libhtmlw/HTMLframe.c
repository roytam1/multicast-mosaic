/* Copyright G.Dauphin Sep 97 */

#include "../libmc/mc_defs.h"
#include "HTML.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "HTMLframe.h"

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
