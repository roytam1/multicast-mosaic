/* HTMLtable.c
 * Author: Gilles Dauphin
 * Version 3.0.1 [Jan97]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * 3D table from : malber@easynet.fr
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "HTML.h"
#include "list.h"

#define DEBUG_REFRESH 0

#define TBL_CELL_DEFAULT_PADDING 1
#define TBL_CELL_DEFAULT_SPACING 2

#define SHADOW_LINE_WIDTH 1

#if 0
static void TableDump( TableInfo *t);
#endif

static void FreeColList(ColumnList * col_list)
{
	ColumnList * cl;

	cl = col_list;
	if( cl->cells)
		free(cl->cells);
	free(cl);
}

static void FreeRowlist( RowList * row_list)
{
	RowList * rl;
	int i;

	rl = row_list;
	for(i=0; i< rl->row_count; i++)
		free(rl->cells_lines[i]);
	
	free(rl->cells_lines);
	free(rl);
}

void _FreeTableStruct(TableInfo * t)
{
	FreeRowlist(t->row_list);
	free(t->col_max_w);
	free(t->col_min_w);
	free(t->col_w);
	free(t);
}

/* Push the PhotoComposeContext (context) for M_TABLE
   TablePlace() get next mptr and look for caption or tr
   if tr parse until (td or th)
   if td or th then FormatChunk ()
*/
static void UpdateColList( ColumnList ** col_list, int td_count,
		MarkType m_cell_type,
		struct mark_up * td_start_mark, struct mark_up * td_end_mark,
		int colspan,int rowspan, int have_bgcolor, Pixel bgcolor)
{
	ColumnList * cl;
	int cell_count;
	int cur_cell_num;
	CellStruct * cells;
	int nspan;
	int i;
	int ns;

	cl = *col_list;

	if (cl == NULL ) { /* create one structure */
		cl = (ColumnList *) malloc (sizeof(ColumnList));
		cl->cell_count = 0;
		cl->cells = NULL;
		cl->max_row_span = 1;
	}
	cell_count = cl->cell_count;

	cur_cell_num = cell_count ;
	cell_count = cell_count + colspan;
	if (!cl->cells)	/* because a SunOS bug : GD 17 Dec 96 */
		cells = (CellStruct *)calloc(cell_count, sizeof(CellStruct));
	else
		cells = (CellStruct *)realloc(cl->cells, sizeof(CellStruct)* cell_count);

	cells[cur_cell_num].td_count = td_count;
	cells[cur_cell_num].colspan = colspan;
	assert(colspan >0);
	cells[cur_cell_num].rowspan = rowspan;
	cells[cur_cell_num].back_cs = 0;
	cells[cur_cell_num].back_rs = 0;
	cells[cur_cell_num].td_start = td_start_mark;
	cells[cur_cell_num].td_end = td_end_mark;
	cells[cur_cell_num].cell_type = m_cell_type;
	cells[cur_cell_num].height = 0;
	cells[cur_cell_num].width = 0;
	cells[cur_cell_num].is_colspan = 0;
	cells[cur_cell_num].is_rowspan = 0;
	cells[cur_cell_num].have_bgcolor = have_bgcolor;
	cells[cur_cell_num].bgcolor = bgcolor;

	nspan = colspan -1;
	cur_cell_num++;
	ns = nspan;
	for (i = 0; i< nspan; i++){
		cells[cur_cell_num].td_count = td_count;
		cells[cur_cell_num].colspan = ns;
		assert(ns >0);
		cells[cur_cell_num].back_cs = i+1;
		cells[cur_cell_num].back_rs = 0;
		cells[cur_cell_num].is_colspan = 1;
		cells[cur_cell_num].is_rowspan = 0;
		cells[cur_cell_num].rowspan = rowspan;
		cells[cur_cell_num].td_start = NULL;
		cells[cur_cell_num].td_end = NULL;
		cells[cur_cell_num].cell_type = M_TD_CELL_PAD;
		cells[cur_cell_num].height = 0;
		cells[cur_cell_num].width = 0;
		cells[cur_cell_num].line_bottom = 0;
		cells[cur_cell_num].have_bgcolor = False;
		cells[cur_cell_num].bgcolor = 0;
		cur_cell_num++;
		ns--;
	}
	cl->cells=cells;
	cl->cell_count = cell_count;
	if (rowspan > cl->max_row_span)
		cl->max_row_span = rowspan;
	*col_list = cl;
}

static void AddPadAtEndColList(ColumnList ** cl, int toadd)
{
	int i;

	assert((*cl)->cells); 	/* because Solaris Bug */
	assert(toadd >0);

	(*cl)->cells = (CellStruct*)realloc((*cl)->cells,
		sizeof(CellStruct) * ((*cl)->cell_count+toadd));

/* on pad le dernier TD dans la ligne (entre deux TR) */
/* on est ici parcequ'on a vu un /TR  ou le dedut d'un autre*/

	for(i=(*cl)->cell_count; i< (*cl)->cell_count+toadd; i++){
		(*cl)->cells[i].cell_type = M_TD_CELL_PROPAGATE;
	}
	(*cl)->cell_count = (*cl)->cell_count+toadd;
}

/* from = 0 to= low_cur_line_num (exclu)  mettre PAD */
/* de low_cur_line_num inclu a row_count-1 mettre FREE */

static void AddPadAtEndRowList(
	RowList * rl,
	int toadd)	/* # of cells to add at end of line */
{
	int i,j;

	for(i=0; i<rl->row_count;i++){ /* realloc more cell in each line */
		if ( !rl->cells_lines[i] ){
			rl->cells_lines[i] = (CellStruct*)calloc(
				rl->max_cell_count_in_line + toadd ,
				sizeof(CellStruct) );
		} else {
			rl->cells_lines[i] = (CellStruct*)realloc(rl->cells_lines[i],
				sizeof(CellStruct)* (rl->max_cell_count_in_line + toadd));
		}
	}
/* add PAD */
	for(i=0; i<rl->low_cur_line_num;i++){
		int bcs;

		bcs = rl->cells_lines[i][rl->max_cell_count_in_line-1].back_cs +1;
		for(j=rl->max_cell_count_in_line;
		    j<(rl->max_cell_count_in_line+toadd); j++){
			rl->cells_lines[i][j].td_count = 0;
			rl->cells_lines[i][j].colspan = 1;
			rl->cells_lines[i][j].rowspan = 1;
			rl->cells_lines[i][j].back_cs = bcs;
			bcs++;
			rl->cells_lines[i][j].back_rs = 0;
			rl->cells_lines[i][j].td_start = NULL;
			rl->cells_lines[i][j].td_end = NULL;
			rl->cells_lines[i][j].height = 0;
			rl->cells_lines[i][j].width = 0;
			rl->cells_lines[i][j].line_bottom = 0;
			rl->cells_lines[i][j].is_colspan = 0;
			rl->cells_lines[i][j].is_rowspan = 0;
			rl->cells_lines[i][j].cell_type = M_TD_CELL_PAD;
			rl->cells_lines[i][j].have_bgcolor = False;
			rl->cells_lines[i][j].bgcolor = 0;
		}
	}
/* add FREE */
	for(i=rl->low_cur_line_num; i<rl->row_count;i++){
		for(j=rl->max_cell_count_in_line;
		    j<(rl->max_cell_count_in_line+toadd); j++){
			rl->cells_lines[i][j].td_count = 0;
			rl->cells_lines[i][j].colspan = 1;
			rl->cells_lines[i][j].rowspan = 1;
			rl->cells_lines[i][j].back_cs = 0;
			rl->cells_lines[i][j].back_rs = 0;
			rl->cells_lines[i][j].td_start = NULL;
			rl->cells_lines[i][j].td_end = NULL;
			rl->cells_lines[i][j].height = 0;
			rl->cells_lines[i][j].width = 0;
			rl->cells_lines[i][j].is_colspan = 0;
			rl->cells_lines[i][j].is_rowspan = 0;
			rl->cells_lines[i][j].cell_type = M_TD_CELL_FREE;
			rl->cells_lines[i][j].have_bgcolor = False;
			rl->cells_lines[i][j].bgcolor = 0;
		}
	}
	rl->max_cell_count_in_line = rl->max_cell_count_in_line + toadd;
}

static void AddFreeLineToRow(RowList * rl, int toadd)
{
	CellStruct* ncl;
	int i,j;

	if ( !rl->cells_lines) {
		rl->cells_lines = (CellStruct**)malloc(
				sizeof(CellStruct*) * (rl->row_count+toadd));
	} else {
		rl->cells_lines = (CellStruct**)realloc(
				rl->cells_lines,
				sizeof(CellStruct*) * (rl->row_count+toadd));
	}
	for(j = 0; j< toadd; j++){
		ncl =(CellStruct*)calloc(rl->max_cell_count_in_line,
				sizeof(CellStruct) );
		for (i=0; i<rl->max_cell_count_in_line; i++){
			ncl[i].td_count = 0;
			ncl[i].colspan = 1;
			ncl[i].rowspan = 1;
			ncl[i].back_cs = 0;
			ncl[i].back_rs = 0;
			ncl[i].td_start = NULL;
			ncl[i].td_end = NULL;
			ncl[i].height = 0;
			ncl[i].width = 0;
			ncl[i].is_colspan = 0;
			ncl[i].is_rowspan = 0;
			ncl[i].cell_type = M_TD_CELL_FREE;
			ncl[i].have_bgcolor = False;
			ncl[i].bgcolor = 0;
		}
		rl->cells_lines[rl->row_count] = ncl;
		rl->row_count++;
	}
}

static void UpdateRowList(RowList ** row_list, int tr_count,
		ColumnList ** cl)
{
	RowList * rl;
	CellStruct work_cell;
	CellStruct ref_cell;
	CellStruct * rcl=NULL;
	CellStruct * this_line = NULL;
	int ncell_for_this_cl;
	int nrow_for_this_cl;
	int low_cur_line_num;
	int i,j;
	int jc, nr;
	int next_low_cur_line_num;
	int free_cell_found;
	int n_rl_free_cell;
	int first_free_cell;


	rl = *row_list;
	ncell_for_this_cl = (*cl)->cell_count;
	nrow_for_this_cl = (*cl)->max_row_span;
	if (rl == NULL){ /* Create one RowList */
		this_line = (*cl)->cells;
		rl = (RowList*) malloc( sizeof(RowList));
		rl->row_count = nrow_for_this_cl;
		rl->max_cell_count_in_line = ncell_for_this_cl;
		rl->low_cur_line_num = 0;
		rl->cells_lines = (CellStruct **)malloc(
					rl->row_count * sizeof(CellStruct*));
		for(i=0; i<rl->row_count; i++){ /* Create cell in rows */
			rl->cells_lines[i] = (CellStruct *) malloc(
				ncell_for_this_cl*sizeof(CellStruct));
		}
		for(j=0; j<ncell_for_this_cl; j++){ /* copy the first row */
			rl->cells_lines[0][j] = this_line[j];/* cells[line][col]*/
			rl->cells_lines[0][j].tr_count = tr_count;
		}
		/* now fill info for next lines */
		free_cell_found =0;
		for(i=1; i<rl->row_count; i++){
			for(j=0; j<ncell_for_this_cl; j++){
				ref_cell = rl->cells_lines[0][j];
				work_cell = ref_cell;
				if (ref_cell.rowspan > i ){
					work_cell.rowspan -= i;
					/* work_cell.back_rs++ ;*/
					work_cell.back_rs = i; /* winfried 15/06/00 */
					work_cell.is_rowspan = 1;
					work_cell.td_start = NULL;
					work_cell.td_end = NULL;
					work_cell.cell_type = M_TD_CELL_PAD;
					work_cell.have_bgcolor = False;
					work_cell.bgcolor = 0;
				} else {
					work_cell.rowspan = 1;
					work_cell.colspan = 1;
					work_cell.back_rs = 0;
					work_cell.back_cs = 0;	
					work_cell.is_rowspan = 0;
					work_cell.is_colspan = 0;
					work_cell.td_start = NULL;
					work_cell.td_end = NULL;
					work_cell.cell_type = M_TD_CELL_FREE;
					work_cell.have_bgcolor = False;
					work_cell.bgcolor = 0;
					if (!free_cell_found){
						free_cell_found = 1;
						rl->low_cur_line_num = i-1;
					}
				}
				rl->cells_lines[i][j] = work_cell;
			}
		}
		if (!free_cell_found){
			rl->low_cur_line_num = i-1;
		}
		*row_list = rl;
		return;
	}
	/* the next low_cur_line_num have a M_TD_CELL_FREE or an empty line */
	low_cur_line_num = rl->low_cur_line_num;
	n_rl_free_cell =0;
	rcl = rl->cells_lines[low_cur_line_num];
					/* count the number of free cells */
	for (i=0; i<rl->max_cell_count_in_line; i++){
		if (rcl[i].cell_type == M_TD_CELL_FREE)
			n_rl_free_cell++;
	}
	first_free_cell = 0;
	for (i=0; i<rl->max_cell_count_in_line; i++){
		if (rcl[i].cell_type == M_TD_CELL_FREE) {
			first_free_cell = i;
			break;
		}
	}
	if (n_rl_free_cell == 0){	/* add an empty line */
		AddFreeLineToRow(rl,1);
		low_cur_line_num++;
		rl->low_cur_line_num = low_cur_line_num;
		n_rl_free_cell = rl->max_cell_count_in_line;
		rcl = rl->cells_lines[low_cur_line_num];
		n_rl_free_cell = 0;
		for (i=0; i<rl->max_cell_count_in_line; i++){
			if (rcl[i].cell_type == M_TD_CELL_FREE)
				n_rl_free_cell++;
		}
		first_free_cell = 0;
		for (i=0; i<rl->max_cell_count_in_line; i++){
			if (rcl[i].cell_type == M_TD_CELL_FREE) {
				first_free_cell = i;
				break;
			}
		}
	}
	if (ncell_for_this_cl< n_rl_free_cell){
		AddPadAtEndColList(cl,n_rl_free_cell - ncell_for_this_cl); 
	}
	if (ncell_for_this_cl > n_rl_free_cell){
#ifdef HTMLTRACE
		printf("BUG: number of TD/TH is false for this TABLE or span count is false.\n");
		printf("     padding the TABLE!!! Adjust....\n");
#endif
		AddPadAtEndRowList(rl,
				ncell_for_this_cl - n_rl_free_cell );
		/* de low_cur_line_num inclu a row_count-1 mettre FREE */
	}
	this_line = (*cl)->cells;
	rcl = rl->cells_lines[low_cur_line_num];
	nrow_for_this_cl = (*cl)->max_row_span;

/*Si nrow_for_this_cl + low_cur_line_num > row_count */
/*etendre la table , augmenter le nombre de ligne avec partout FREE */
	if( (nrow_for_this_cl + low_cur_line_num) > rl->row_count){
		AddFreeLineToRow(rl,
			nrow_for_this_cl + low_cur_line_num - rl->row_count);
	}

/* maintenant (*cl)->cell_count et n_rl_free_cell sont egaux */
/* le nombre de ligne dans rl est suffisant */
	jc = 0;
	for(i=0; i<rl->max_cell_count_in_line; i++){
		if ( rcl[i].cell_type == M_TD_CELL_FREE){
			ref_cell = this_line[jc];
			if(ref_cell.cell_type == M_TD_CELL_PROPAGATE ){
				memset(&ref_cell, 0, sizeof(ref_cell));
				ref_cell.cell_type= M_TD_CELL_PROPAGATE;
				ref_cell.colspan=1;
				ref_cell.rowspan =1;
				rcl[i] = ref_cell;
				jc++;
				continue;
			}
			work_cell = ref_cell;
/* Faire gaffe au row spanning quand on ajoute une ligne on met des FREE */
/* sauf dans les colonnes[i..i+colspan] ou on met des PAD avec rowspan */
			for(nr = 1 ; nr < ref_cell.rowspan ; nr++){/*do rowspan */
				work_cell.rowspan = ref_cell.rowspan -nr;
				work_cell.back_rs++;
				work_cell.is_rowspan = 1;
				work_cell.td_start = NULL;
				work_cell.td_end = NULL;
				work_cell.cell_type = M_TD_CELL_PAD;
				rl->cells_lines[low_cur_line_num+nr][i] = work_cell;
			}
			rcl[i] = ref_cell;
			jc++;
		}
	}
/* compute the new low_cur_line_num */
	next_low_cur_line_num = low_cur_line_num+1;
	free_cell_found =0;
	for(i=next_low_cur_line_num; i<rl->row_count; i++){
		for(j=0; j<rl->max_cell_count_in_line; j++){
			if ( rl->cells_lines[i][j].cell_type == M_TD_CELL_FREE){
				free_cell_found = 1;
				rl->low_cur_line_num = i;
				break;
			}
		}
		if (free_cell_found)
			break;
	}
	*row_list = rl;
}

static TableInfo * FirstPasseTable(HTMLWidget hw, struct mark_up *mptr,
	PhotoComposeContext * pcc)
{
	char * val;
	char * tptr;
	TableInfo *t;
	TableInfo lt;
	struct mark_up * sm;
	struct mark_up * tb_start_mark;		/* save the marker <TABLE> */
	struct mark_up * tb_end_mark=NULL;	/* save the marker </TABLE> */
	struct mark_up * start_other_mark; /* is mark up between TABLE and TR */
					/* or CAPTION ?			   */
	struct mark_up * end_other_mark; /* is mark up between TABLE and TR */
	ColumnList * col_list;
	RowList * row_list;
	int td_count;
	int tr_count;
	int tr_start_found=0;
	int td_start_found=0;
	struct mark_up * caption_end_mark;
	struct mark_up * caption_start_mark;
	int caption_found;
	int end_caption_found;
	int tr_found;
	struct mark_up * tr_start_mark;
	struct mark_up * tr_end_mark;
	struct mark_up * td_start_mark;
	struct mark_up * td_end_mark;
	struct mark_up * psm;
	MarkType m_cell_type = M_TD_CELL_PAD;
	char * mt_cell_type = NULL;
	int colspan =0;
	int rowspan =0;
	Pixel thtd_bgcolor = 0;
	int thtd_have_bgcolor = False;

/* mptr is a pointer on <TABLE> */

	tb_start_mark = mptr;
	psm = mptr;
	sm = mptr->next;

/* 'sm' is a pointer just after <TABLE> */

	td_count = 0;
	tr_count = 0;
	lt.num_col = 0;
	lt.num_row = 0;
	lt.caption_start_mark = NULL;
	lt.caption_end_mark = NULL;
	lt.start_other_mark = NULL;
	lt.end_other_mark =NULL;
	lt.relative_width = 0;
	lt.borders = 0;
	lt.cellSpacing = TBL_CELL_DEFAULT_SPACING;
	lt.cellPadding = TBL_CELL_DEFAULT_PADDING;
	lt.row_list = NULL;
	lt.width = 0;
	lt.height = 0;
	lt.min_width =0;
	lt.max_width =0;
	lt.is_tint = 0;
	lt.have_bgcolor = False;
	lt.bgcolor = 0;

	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"BORDER")) ){
		lt.borders = atoi(tptr);
		free(tptr);
	}
	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"WIDTH")) ){
		lt.relative_width = atoi(tptr);	/* the width wanted by user */
		if (strchr(tptr,'%') == NULL){ /* absolute value */
			lt.relative_width = (lt.relative_width*100)/pcc->cur_line_width;
		}
		free(tptr);
	}
	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"ALIGN")) ) {
#ifdef HTMLTRACE
		printf("[MakeTable] %s not yet implemented\n","ALIGN");
#endif
/* ##########
           if(strcasecmp(tptr,"LEFT") == 0) halignment=HALIGN_LEFT_FLOAT;
               else
           if(strcasecmp(tptr,"CENTER") == 0) halignment=HALIGN_CENTER;
               else
           if(strcasecmp(tptr,"RIGHT") == 0)  halignment=HALIGN_RIGHT_FLOAT;
		free(tptr);
/* #######--- INHERITED value ? ---
	if(halignment == ALIGN_NONE)
	{if(pcc->div == HALIGN_LEFT) halignment = HALIGN_LEFT;
	else
	if(pcc->div == HALIGN_CENTER) halignment = HALIGN_CENTER;
	else
	if(pcc->div == HALIGN_RIGHT) halignment = HALIGN_RIGHT;
	}else
	{if(halignment == HALIGN_LEFT) pcc->div = HALIGN_LEFT;
	else
	if(halignment == HALIGN_CENTER) pcc->div = HALIGN_CENTER;
	else
	if(halignment == HALIGN_RIGHT) pcc->div = HALIGN_RIGHT;
	}
	lt.halignment = halignment;
####### */
	}
	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"CELLSPACING")) ) {
		lt.cellSpacing = atoi(tptr);
		free(tptr);
	}
	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"CELLPADDING")) ) {
		lt.cellPadding = atoi(tptr);
		free(tptr);
	}
	if ( (tptr=ParseMarkTag(mptr->start,MT_TABLE,"BGCOLOR")) ) {
		XColor c;
		int status;

		status = XParseColor(XtDisplay(hw),hw->core.colormap, tptr, &c);
        	c.flags = DoRed | DoGreen | DoBlue;
		if (status) {
			lt.bgcolor = HTMLXColorToPixel(&c);
			lt.have_bgcolor = True;
		}
		free(tptr);
	}
	/* find the first TR or CAPTION */
	caption_found = 0;
	tr_found = 0;
	start_other_mark = NULL;
	end_other_mark = NULL;
/* 'sm' is a pointer just after <TABLE> */
	if (sm->type== M_TABLE && sm->is_end) { /* empty table */
		return NULL;
	}
	while ( sm ){
		if((sm->type== M_CAPTION) && (!sm->is_end)){
			lt.captionAlignment = VALIGN_BOTTOM;
			if ( (tptr = ParseMarkTag(sm->start,MT_CAPTION,"top")) ){
				lt.captionAlignment = VALIGN_TOP;
				free(tptr);
			}
			caption_found = 1;
			caption_start_mark = sm;
			break;
		}
		if((sm->type == M_TR || sm->type == M_TD ||
		    sm->type == M_TH) && (!sm->is_end)){
			tr_found = 1;
			tr_start_mark = sm;
			break;
		}
		if ( (!sm->is_white_text) && (!start_other_mark) )
			start_other_mark = sm;
		if (start_other_mark)
			end_other_mark = sm;
		sm = sm->next;
	}
	lt.start_other_mark = start_other_mark;
	lt.end_other_mark = end_other_mark;

	if ( (caption_found == 0) && (tr_found == 0))
		return NULL;

	if (caption_found){ /* find the end CAPTION */
		end_caption_found =0;
		caption_end_mark = caption_start_mark;
		while(caption_end_mark){
			if((caption_end_mark->type == M_CAPTION) &&
			    caption_end_mark->is_end ){
				end_caption_found = 1;
				break;
			}
			caption_end_mark = caption_end_mark->next;
		}
		if (!end_caption_found){
#ifdef HTMLTRACE
			printf("</CAPTION> not found\n");
#endif
			return NULL;
		}
		lt.caption_start_mark = caption_start_mark;
		lt.caption_end_mark = caption_end_mark;
#ifdef HTMLTRACE
		printf("<CAPTION> not yet fully implemented\n");
#endif
	}

	/* now find the first <TR> */
	if ( ! tr_found ){
		tr_start_mark = caption_end_mark;
		while(tr_start_mark){
			if((tr_start_mark->type == M_TR) &&
			    !(tr_start_mark->is_end) ){
				tr_found = 1;
				break;
			}
			tr_start_mark = tr_start_mark->next;
		}
		if (!tr_found){
			return NULL;
		}
	}

				/*tr_start_mark	is the first TR */
	psm = NULL;		/* the previous mark because end is optional */
	sm = tr_start_mark;
	tr_end_mark = NULL;
	td_start_mark = NULL;
	td_end_mark = NULL;
	tb_end_mark = NULL;
	td_count = 0;
	tr_count =0;		/* we have stil a TR */
	row_list = NULL;
	col_list = NULL;
	tr_start_found = 0;
	td_start_found = 0;

/* ####### val=ParseMarkTag(sm->start,type_string,"VALIGN"); */
	while (sm){
/* Traitement de <TD> ou <TH> */
		if( ((sm->type == M_TD) || (sm->type == M_TH))&&
		    (!sm->is_end) ) { 		/* <TH> or <TD> */
			if (!tr_start_found){
#ifdef HTMLTRACE
				printf("A <TD> <TH> is outside a <TR>\n");
#endif
				psm = sm;
				sm = sm->next;
				continue;
			}
			if ( td_start_found) { /* this is the end of previous */
				td_count++;
				td_end_mark = psm;
				assert(colspan>0);
				UpdateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan,
					thtd_have_bgcolor, thtd_bgcolor);
			}
			if(sm->type == M_TD){
				m_cell_type = M_TD;
				mt_cell_type = MT_TD;
			} else {
				m_cell_type = M_TH;
				mt_cell_type = MT_TH;
			}
			td_start_found = 1;
			td_start_mark = sm;
			colspan =1;
			val = ParseMarkTag(sm->start,mt_cell_type, "colspan");
			if (val){
				colspan = atoi(val);
				free(val);
			}
			if (colspan <=0) 
				colspan = 1;
			rowspan = 1;
			val = ParseMarkTag(sm->start,mt_cell_type,"rowspan");
			if (val){
				rowspan = atoi(val);
				free(val);
			}
			if (rowspan <=0) 
				rowspan = 1;
			thtd_have_bgcolor = False;
			val=ParseMarkTag(sm->start,mt_cell_type,"BGCOLOR");
			if ( val) {
				XColor c;
				int status;

				status = XParseColor(XtDisplay(hw),
					hw->core.colormap, val, &c);
				c.flags = DoRed | DoGreen | DoBlue;
				if (status) {
					thtd_bgcolor = HTMLXColorToPixel(&c);
					thtd_have_bgcolor = True;
				}
				free(val);
			}
			psm = sm;
			sm = sm->next;
			continue;
		}				/* <TH> or <TD> */
		if( ((sm->type == M_TD) || (sm->type == M_TH))&&
		    (sm->is_end) ) { 		/* </TH> or </TD> */
			if (!tr_start_found){
#ifdef HTMLTRACE
                                printf("A <TD> <TH> is outside a <TR>\n");
#endif
				psm = sm;
				sm = sm->next;
                                continue;
                        }
			if (!td_start_found){
#ifdef HTMLTRACE
                                printf("A </TD> is without a <TD>\n");
#endif
				psm = sm;
				sm = sm->next;
                                continue;
                        }
			td_count++;
			td_end_mark = sm;
			assert(colspan>0);
			UpdateColList(&col_list,td_count,m_cell_type,
				td_start_mark,td_end_mark,
				colspan,rowspan,
				thtd_have_bgcolor, thtd_bgcolor);
			td_start_found = 0;
			psm = sm;
			sm = sm->next;
			continue;
		}
/* jusqu'a fin </TD> ou </TH> */

/* Traitement <TR> */
		if ((sm->type == M_TR) && (!sm->is_end)){
			if (td_start_found){
				td_count++;
				td_end_mark = psm;
				assert(colspan>0);
				UpdateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan,
					thtd_have_bgcolor, thtd_bgcolor);
				td_start_found = 0;
			}
			if (tr_start_found) {
				tr_end_mark = psm;
				tr_count++;
				if (col_list){
					UpdateRowList(&row_list,tr_count,&col_list);
					FreeColList(col_list);
					col_list = NULL;
					td_count =0;
				}
#ifdef HTMLTRACE
				else{
					printf("<TR> without<TD>: empty line!\n");
				}
#endif
			}
			tr_start_found = 1;
			tr_start_mark = sm;
			psm = sm;
			sm = sm->next;
			continue;
		}
		if ((sm->type == M_TR) && (sm->is_end)){
			if (!tr_start_found){
#ifdef HTMLTRACE
				printf("A </TR> without <TR>\n");
#endif
				psm = sm;
				sm = sm->next;
				continue;
			}
			if (td_start_found){
				td_count++;
				td_end_mark = psm;
				assert(colspan>0);
				UpdateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan,
					thtd_have_bgcolor, thtd_bgcolor);
				td_start_found = 0;
			}
			if(col_list){
				tr_count++;
				tr_end_mark = sm;
				UpdateRowList(&row_list,tr_count,&col_list);
				FreeColList(col_list);
				col_list = NULL;
				td_count =0;
			}
#ifdef HTMLTRACE
			 else {
				printf("<TR> without<TD>: empty line!\n");
			}
#endif
			tr_start_found = 0;
			psm = sm;
			sm = sm->next;
			continue;
		}
/* jusqu'a fin </TR> */

/* Traitement de la fin de la table */
		if((sm->type == M_TABLE) && (sm->is_end)){ /*the end table */
			if (td_start_found){
				td_count++;
				td_end_mark = psm;
				assert(colspan>0);
				UpdateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan,
					thtd_have_bgcolor, thtd_bgcolor);
				td_start_found = 0;
			}
			if (tr_start_found) {
				tr_end_mark = psm;
				tr_count++;
				if(col_list){
					UpdateRowList(&row_list,tr_count,&col_list);
					FreeColList(col_list);
					col_list = NULL;
					td_count =0;
				} else {
#ifdef HTMLTRACE
					printf("<TR> without<TD> and </TABLE> seen!: Buggy TABLE !!!\n");
#endif
					break;
				}
			}
			if ( tr_count == 0)
				break;
			tb_end_mark = sm;
			/* UpdateTableInfo */
			lt.num_col = row_list->max_cell_count_in_line ;
			lt.num_row = row_list->row_count;
			lt.tb_end_mark = tb_end_mark;
			lt.tb_start_mark = tb_start_mark;
			lt.row_list = row_list;
			row_list=NULL;
			tr_count =0;
			break;
		}

/* traitement d'une table dans une table par recursivite */
		if( (sm->type == M_TABLE) && (!sm->is_end)){ /*a table in table*/
			TableInfo *tt;

			tt = FirstPasseTable(hw, sm, pcc); /* be recursive */
			if (!tt){
#ifdef HTMLTRACE
				printf("Buggy Table in Table !\n");
#endif
				sm->type = M_BUGGY_TABLE; /* change type */
						/* don't rescan */
			} else {
				tt->is_tint = 1;
				sm->t_p1= tt;
				psm = sm;
				sm = tt->tb_end_mark->next;
				continue;
			}
		}
		psm = sm;
		sm=sm->next;
	}
	if(!tb_end_mark){
#ifdef HTMLTRACE
		printf("Probleme the end table </TABLE> is not see\n");
#endif
		if(col_list)
			FreeColList(col_list);
		if(row_list)
			FreeRowlist(row_list);
		return NULL;
	}
	t = (TableInfo *) calloc(1,sizeof(TableInfo));
	CHECK_OUT_OF_MEM(t);
	*t = lt;
	return t;
}

static void EstimateMinMaxTable(HTMLWidget hw, TableInfo *t,PhotoComposeContext * orig_pcc)
{
	PhotoComposeContext deb_pcc;
	PhotoComposeContext fin_pcc;
	int i,j,k;
	CellStruct * line;
	CellStruct  cell;
	int line_min_w ;
	int line_max_w ;
	int estimate_height;
	int h_row=0;
	int shadow_line_width = 0;

	deb_pcc = *orig_pcc;
	deb_pcc.cw_only = True;
	deb_pcc.computed_min_x = 0;
	deb_pcc.computed_max_x = 0;
	deb_pcc.x =0;
	estimate_height = deb_pcc.y= 0;
	deb_pcc.is_bol = 1;
	deb_pcc.eoffsetx = 0;
	deb_pcc.eoffsety = 0;
	deb_pcc.left_margin = 0;
	deb_pcc.right_margin = 0;
	deb_pcc.have_space_after =0;
	deb_pcc.cur_baseline = orig_pcc->cur_baseline;
	deb_pcc.cur_line_height = orig_pcc->cur_line_height;

	if(t->borders)
		shadow_line_width = SHADOW_LINE_WIDTH;

	t->col_max_w = (int*) calloc(t->num_col, sizeof(int));
	t->col_min_w = (int*) calloc(t->num_col, sizeof(int));
	t->col_w = (int*) calloc(t->num_col, sizeof(int));
	for(i=0;i<t->num_col;i++){
		t->col_max_w[i] = 0;
		t->col_min_w[i] = 0;
		t->col_w[i] = 0;
	}

	estimate_height = t->num_row * 2 * (t->cellPadding + shadow_line_width)
		+ (t->num_row + 1) * t->cellSpacing + 2 * t->borders;

	for(i=0; i< t->num_row; i++){
		line = t->row_list->cells_lines[i];
		h_row = 0;
		for(j=0; j<t->num_col; ){ /* prendre chaque element */
			fin_pcc = deb_pcc;
			cell = line[j];		/* un element */
			if(cell.cell_type == M_TH){
/* ########### faire un pushfont ...
				fin_pcc.cur_font = hw->html.bold_font;
########## faire un popfont ... mais ou? ### */
			}
			FormatChunk(hw,cell.td_start,cell.td_end,&fin_pcc,0);
			assert(cell.colspan >0);
			for(k = 0; k < cell.colspan; k++){
				line[j].min_width = 
					fin_pcc.computed_min_x/cell.colspan;
				line[j].max_width = 
					1+fin_pcc.computed_max_x/cell.colspan;
				if (t->col_min_w[j] < line[j].min_width)
					t->col_min_w[j] = line[j].min_width;
				if (t->col_max_w[j] < line[j].max_width)
					t->col_max_w[j] = line[j].max_width;
				j++;
			}
			if ( fin_pcc.cur_line_height > h_row)
				h_row = fin_pcc.cur_line_height;
			/* propagate in_form */
			deb_pcc.in_form = fin_pcc.in_form;
/*
12/07/2000 VOIR FONT color=#xxx -DTD page 271-
Quand 'attribut color n'est pas la il faut prendre le <body text=color> par defaut....
   size		CDATA 		#IMPLIED
   color	%Color;		#IMPLIED
   face		CDATA		#IMPLIED
*/
/* les fonts se propage a travers un stack, ainsi que les couleurs. */
			deb_pcc.fg_text = fin_pcc.fg_text;
			deb_pcc.cur_font = fin_pcc.cur_font;
		}
		for(j=0; j<t->num_col; j++)
			line[j].height = h_row;
		estimate_height += h_row;
	}
#if 0
	line_min_w = 0;
	line_max_w = 0;
	for(i=0;i<t->num_col;i++){
		line_min_w += t->col_min_w[i]+2 * TBL_CELL_MARGIN_WIDTH;
		line_max_w += t->col_max_w[i]+2 * TBL_CELL_MARGIN_WIDTH;
	}
	line_min_w += (t->num_col -1) * t->borders;
	line_max_w += (t->num_col -1) * t->borders;
	line_min_w += 2 * t->borders - t->borders/2;
	line_max_w += 2 * t->borders - t->borders/2;
#endif
/*                             
 *	at | we will draw a line with top ot bottom shadow color
 *		bbbb is the border in top or bottom shadow
 *	p the cellPadding
 *	cccc, the cell         
 *	s the cellSpacing      
 *
 *	bbbbs|pccccp|s|pccccp|s|pccccp|sbbbb
 *	bbbbs|pppppp|s|pppppp|
 *	bbbbs--------s--------
 *	bbbbssssssssssssssssss
*/ 
	line_min_w = t->num_col * 2 * (t->cellPadding + shadow_line_width)
		+ (t->num_col + 1) * t->cellSpacing + 2 * t->borders;
	line_max_w = line_min_w;
	for(i=0;i<t->num_col;i++){     
		line_min_w += t->col_min_w[i];
		line_max_w += t->col_max_w[i];
        }

	t->min_width = line_min_w;
	t->max_width = line_max_w;
	t->estimate_height = estimate_height;
}

void TablePlace(HTMLWidget hw, struct mark_up **mptr, PhotoComposeContext * pcc,
	Boolean save_obj)
{
	struct mark_up *sm;
	TableInfo *t;
	int h_def_height_cell;
	CellStruct * line;
	CellStruct cell;
	int i,j,k;
	PhotoComposeContext line_pcc, work_pcc,tbl_pcc;
	int w_table;
	int h_table;
	int max_line_bot;
	int delta;
	struct ele_rec * cr_eptr;
	struct ele_rec * table_eptr;
	int w_in_cell;
	int to_add_col;
	int wanted_w;
	int shadow_line_width = SHADOW_LINE_WIDTH;

        if ((*mptr)->is_end)            /* end of table */
                return;
	sm = *mptr;			/* save marker */

/* 'sm' is now a pointer on <TABLE> */

/* do a pre-pass to count the number of column and raw.
 * Capture all end markers
 * Faire une prepasse pour compter le nombre de colonne et de ligne
 * capturer les markeurs de fin */

	if (!sm->t_p1){		/* do the prepasse */
		t = FirstPasseTable(hw,sm,pcc);	/* the First passe */
		if (!t){
#ifdef HTMLTRACE
			printf("Invalid table structure !\n");
#endif
			return;
		}
		sm->t_p1= t;
	}
	t = sm->t_p1;

/* execute HTML marker beetween <TABLE> and first <TR> or <CAPTION> */

	if(t->start_other_mark){
		FormatChunk(hw, t->start_other_mark, t->end_other_mark,
			pcc, save_obj);
		LineBreak(hw,t->end_other_mark,pcc);
	}

/* on place le CAPTION toujours au debut */
	if(t->caption_start_mark){
		FormatChunk(hw,t->caption_start_mark,t->caption_end_mark,
					pcc, save_obj);
		LineBreak(hw,t->caption_end_mark,pcc);
	}

/* once we have a table, we compute the min and max size of each cell */
/* save the contexte for each cell, parse beetween marker , the return context */
/* give the size. When doing this NEVER create element */
/* save the img struct or aprog struct or applet struct in the mark_up struct, */
/* NOT in ele_rec struct */


/* faire un deuxieme tour pour chauque TD avec un pcc artificiel */
/* de td_start_mark a td_end_mark */
	if (!t->min_width) { /* on a pas estimer les dimensions de la table */
		EstimateMinMaxTable(hw,t,pcc);
	}
	if (pcc->cw_only) { /* mettre a jour la partie estimer du pcc */
		if(pcc->computed_min_x < pcc->x + t->min_width)
			pcc->computed_min_x = pcc->x + t->min_width;
		if(pcc->computed_max_x < pcc->x + t->max_width)
			pcc->computed_max_x = pcc->x + t->max_width;

/* adjust the mptr at end */
		*mptr = t->tb_end_mark;	/* advance mark pointer to end of table */
		return;
	}
/*
3 cas :

	1. t->min_width >= taille disponible ====>
	   allouer le mini pour chaque colonne et rendre le scroll possible

	2. t->max_width < taille disponible =====>
	   allouer la taille max pour chaque colonne.

	3. t->min_width < taille disponible < t->max_width ====>
	   gw = taille disponible - t->min_width;
	   gd = t->max_width - t->min_width;
	   pour chaque colonne 'i' faire:
		pd[i] = t->col_max_w[i] - t->col_min_w[i];
		col_width[i] = t->col_min_w[i] + pd[i] * gw / gd;
		En principe la somme des col_width[i] est
		egale a taille disponible. Attention au arrondi...

Caluler maintenant t->col_w[i] suivant ces trois cas.

*/
	if(t->borders == 0) 
		shadow_line_width = 0;
	if (t->min_width >= pcc->cur_line_width){	/* cas 1 */
		for(i=0; i<t->num_col;i++){
			t->col_w[i] = t->col_min_w[i];
		}
	} else if (t->max_width < pcc->cur_line_width){ /* cas 2 */
		for(i=0; i<t->num_col;i++){
			t->col_w[i] = t->col_max_w[i];
		}
	} else {				       /* cas 3 */
		int gw;
		int gd;
		int pd;

		gw = pcc->cur_line_width - t->min_width;
		gd = t->max_width - t->min_width;
		for (i=0;i<t->num_col;i++){
			pd = t->col_max_w[i] - t->col_min_w[i];
			t->col_w[i] = t->col_min_w[i] + (pd * gw) / gd;
		}
	}

/* maintenant on peut calculer la largeur de la table */
	w_table = t->num_col * 2 * (t->cellPadding + shadow_line_width)
		+ (t->num_col + 1) * t->cellSpacing + 2 * t->borders;
	for(i=0; i<t->num_col;i++){
		w_table += t->col_w[i];
	}

	wanted_w = (t->relative_width * pcc->cur_line_width) /100;
	if (wanted_w > w_table ) { /* add width to each col */
		to_add_col = (wanted_w - w_table) / t->num_col;
		for(i=0;i<t->num_col;i++){
			t->col_w[i] = t->col_w[i] + to_add_col;
			w_table = w_table + to_add_col;
		}
	}

	if (pcc->max_width_return < 
	    (w_table + pcc->left_margin + pcc->eoffsetx))
		pcc->max_width_return = w_table + pcc->left_margin +
					pcc->eoffsetx;

	/* ICI le pcc->cur_line_width doit etre correcte */
	/* le sauver , et au besoin mettre la table plus large */
	/* il faut alors ajuster pcc->max_width_return */
	/* calculer la dimension de chaque colonne avec des 'poids' */
	/* en fonction des min et max des 'cells' */
	/* calculer la dimension definitive de la table */
	/* calculer les dimension des cells pour que tout rentre */
	/* au besoin agrandir la table */

        LineBreak(hw, *mptr, pcc);

	if (t->have_bgcolor) {
		pcc->bgcolor = MMPushColorBg(hw, t->bgcolor);
	} else {
		pcc->bgcolor = MMPushColorBg(hw, pcc->bgcolor);
	}
	tbl_pcc = *pcc;
	
/* now compute the real width of cell and create element in cell */

/* set the default height of cell */
	h_def_height_cell = pcc->cur_font->ascent+pcc->cur_font->descent;
	line_pcc = tbl_pcc;
	line_pcc.y = tbl_pcc.y + t->borders + t->cellSpacing;
	line_pcc.eoffsetx = tbl_pcc.left_margin+tbl_pcc.eoffsetx;
	max_line_bot = line_pcc.y+ h_def_height_cell;
	table_eptr = CreateElement(hw,E_TABLE, pcc->cur_font,
		pcc->x, pcc->y,
		0 /* w_table */, 0 /*h_table*/,0 /* h_table*/, pcc);
	for(i=0; i< t->num_row; i++){
		int cell_offset;

		line = t->row_list->cells_lines[i];
		cell_offset =t->borders + t->cellSpacing;
		for(j=0; j<t->num_col; j++){ /* prendre chaque element */
			int add_offset;

			w_in_cell = t->col_w[j]; /* set final width of cell */
			cell = line[j];		/* un element */
			work_pcc = line_pcc;	/* en prendre un pour travailler*/
/* what is the type of this cell */
			add_offset = w_in_cell +
				t->cellSpacing  + 2 * (t->cellPadding+shadow_line_width);
			cell.width = w_in_cell +
				 2 * (t->cellPadding + shadow_line_width);
			cell.y = line_pcc.y; 
			cell.height = work_pcc.cur_line_height
				+ 2 * (t->cellPadding + shadow_line_width);
			cell.line_bottom = line_pcc.y+ t->cellPadding + shadow_line_width +
                                                line_pcc.cur_line_height;
			switch (cell.cell_type){
			case M_TD_CELL_PAD:
				cell.x = cell_offset + line_pcc.eoffsetx;
/* propagate the height, line_bottom, from starting cell span */
				cell.line_bottom = t->row_list->cells_lines[i-cell.back_rs][j-cell.back_cs].line_bottom;
				cell.height = t->row_list->cells_lines[i-cell.back_rs][j-cell.back_cs].height;
				break;
			case M_TD_CELL_PROPAGATE:
			case M_TD_CELL_FREE:
				cell.x = cell_offset + line_pcc.eoffsetx;
				break;
			case M_TD:
			case M_TH:
				for(k=1; k< cell.colspan;k++){
					w_in_cell = w_in_cell + t->col_w[j+k];
				}
				w_in_cell += (cell.colspan -1)*
				    (t->cellSpacing + 2 * (t->cellPadding+shadow_line_width));
				work_pcc.left_margin = t->cellPadding + shadow_line_width;
				work_pcc.right_margin = t->cellPadding + shadow_line_width;
				work_pcc.cur_line_width = w_in_cell;
				work_pcc.eoffsetx = line_pcc.eoffsetx+cell_offset;
				work_pcc.x = work_pcc.eoffsetx + 
					     work_pcc.left_margin;
				work_pcc.y = line_pcc.y + t->cellPadding + shadow_line_width;
				work_pcc.have_space_after = 0;
				if(cell.cell_type == M_TH){
/* ########### faire un push font 
					work_pcc.cur_font = hw->html.bold_font;
########## faire un popfont ... mais ou? ### */
/* comment faire pour centrer les titres ? */
				}
				work_pcc.cur_line_height = work_pcc.cur_font->ascent + line_pcc.cur_font->descent;
				work_pcc.cur_baseline = work_pcc.cur_font->ascent;
				work_pcc.is_bol = 1;
				cr_eptr = CreateElement(hw, E_CR,
					work_pcc.cur_font,
                			work_pcc.x, work_pcc.y,        
                			0, work_pcc.cur_line_height,
                			work_pcc.cur_baseline, &work_pcc);
				cell.start_elem = cr_eptr;

				if ( cell.have_bgcolor ) {
					work_pcc.bgcolor = MMPushColorBg(hw, cell.bgcolor);
				}

				FormatChunk(hw,cell.td_start,cell.td_end,
					&work_pcc, False);
/* add little vertical space */
				LineBreak(hw,cell.td_end,&work_pcc);
				if ( cell.have_bgcolor ) {
					work_pcc.bgcolor = MMPopColorBg(hw);
				}
				cell.end_elem = hw->html.last_formatted_elem;
/*difference des pcc pour determiner la hauteur*/
				cell.x = cell_offset + line_pcc.eoffsetx;
				cell.width = w_in_cell +
					   2 * (t->cellPadding + shadow_line_width);
				cell.y = line_pcc.y; 
				cell.height = work_pcc.y - line_pcc.y
					/* + line_pcc.cur_font->ascent/2 */
					+ t->cellPadding + shadow_line_width;
				cell.line_bottom = work_pcc.y
					/* + line_pcc.cur_font->ascent/2 */
					+ t->cellPadding + shadow_line_width;
				break;
			default:
				assert(0);
#ifdef HTMLTRACE
				printf("BUG: Unknow cell type in TABLE\n");
#endif
				break;
			}
			cell_offset += add_offset ;
/* if (cellule_seule ou cellule_fin_rowspan) */
			if(cell.rowspan == 1){
				if (cell.line_bottom > max_line_bot)
					max_line_bot = cell.line_bottom;
			}
			line[j] = cell;
/* #### FIXME we must propagate some attribute */
			line_pcc.widget_id = work_pcc.widget_id;
			line_pcc.element_id = work_pcc.element_id;
			line_pcc.aprog_id = work_pcc.aprog_id;
			line_pcc.applet_id = work_pcc.applet_id;
			line_pcc.in_form = work_pcc.in_form;
			line_pcc.cur_form = work_pcc.cur_form;
/* les font se propage a travers une pile */
			line_pcc.fg_text = work_pcc.fg_text;
			line_pcc.cur_font = work_pcc.cur_font;
		}
/*Ajuster les hauteurs des 'cells'.Faire attention au span en ligne et colonne */
/* pour chaque cellule dans la ligne voir si c'est la fin d'un 'rowspan'*/
/* si oui (ou cellule seule) ajuster la hauteur pour que les lignes du bas */
/* soient alignees. Si cellule seule ou fin d'un row span , alors */
/* Creer l'element graphique qui entoure la cellule */
		for(j=0; j<t->num_col; j++){
			cell = line[j];

/* if (cellule_seule ou cellule_fin_rowspan) */
			if( (cell.colspan == 1) && (cell.rowspan == 1) ){

/* ajuste la hauteur de la cellule en fonction de max_line_bot */
				delta = max_line_bot - t->row_list->cells_lines[i-cell.back_rs][j-cell.back_cs].line_bottom;
				if (delta > 0 ){
					t->row_list->cells_lines[i-cell.back_rs][j-cell.back_cs].height += delta;
					t->row_list->cells_lines[i-cell.back_rs][j-cell.back_cs].line_bottom = max_line_bot;
				}
/* Si cellule seule ou fin d'un row span , alors */
/* Creer l'element graphique qui entoure la cellule */
/*########### il faut centrer les ELMENENTS en hauteur dans la cellule */
			}
		}
/* stocker la hauteur de la table */
		h_table = max_line_bot - pcc->y;
		line_pcc.y = max_line_bot + t->cellSpacing;
	}

	pcc->widget_id = line_pcc.widget_id;
	pcc->element_id =line_pcc.element_id ;
	pcc->aprog_id =	line_pcc.aprog_id ;
	pcc->applet_id = line_pcc.applet_id ;
	pcc->in_form = line_pcc.in_form;
	pcc->cur_form = line_pcc.cur_form;

/* les font se propage a travers une pile */
	pcc->fg_text = line_pcc.fg_text;
	pcc->cur_font = line_pcc.cur_font;

	t->width = w_table;
	h_table += t->cellSpacing + t->borders;
	t->height = h_table;

	table_eptr->underline_number = 0; /* Table's can't be underlined */
	table_eptr->table_data = t;

	table_eptr->width = w_table;
	table_eptr->height = h_table;
	table_eptr->baseline = h_table;
	pcc->bgcolor = MMPopColorBg(hw);

/* adjust the mptr at end */
	*mptr = t->tb_end_mark;		/* advance mark pointer to end of table */

/* adjust pcc */

        pcc->x += table_eptr->width;
	pcc->y += table_eptr->height;
	pcc->div = HALIGN_LEFT;
/* do a linefeed */
	LinefeedPlace(hw,t->tb_end_mark,pcc);

	sm->t_p1=NULL;
}

/* put this in widget ! #### */
static GC ttopGC, tbotGC, colorGC;

#define shadowpm_width 2             
#define shadowpm_height 2            
static char shadowpm_bits[] = { 0x02, 0x01};

/* display table */
void TableRefresh( HTMLWidget hw, struct ele_rec *eptr,
	int win_x, int win_y, Dimension win_w, Dimension win_h)
{
	int x,y; 		/* table origin */
	TableInfo * t;
	CellStruct cell;
	CellStruct **cells_lines;
	int i,j;
	XPoint pt[6];                  
	Display *dsp = XtDisplay(hw);  
	Screen *scn = XtScreen(hw);    
	GC ltopGC, lbotGC;             
#define MAX_SEG 128                  
	XSegment segT[MAX_SEG], segB[MAX_SEG];
	int iseg=0;
	int rfr_x, rfr_y, rfr_w, rfr_h;
                                       
	t = eptr->table_data; 

/* position relative to left corner of top view */
	x = eptr->x - hw->html.scroll_x;
	y = eptr->y - hw->html.scroll_y;

	rfr_x = win_x;		/* refresh zone */
	rfr_y = win_y;
	rfr_w = win_w;
	rfr_h = win_h;
	if ( x > win_x ) {
		rfr_x = x;
		rfr_w = rfr_w - (x - win_x);
	}
	if ( y > win_y ) {
		rfr_y = y;
		rfr_h = rfr_h - (y - win_y);
	}
	if ( (rfr_w <= 0) || (rfr_h <= 0) )	/* object is not in rfr part */
		return;

	if ( (rfr_x + rfr_w) > (x + eptr->width) ) { /* reduce width */
		rfr_w = x + eptr->width - rfr_x;
	}
	if ( (rfr_y + rfr_h) > (y + eptr->height) ) { /* reduce height */
		rfr_h = y + eptr->height - rfr_y;
	}
	if ( (rfr_w <= 0) || (rfr_h <= 0) )	/* object is not in rfr part */
		return;
	
	if ( colorGC == NULL) {
		unsigned long valuemask;
		XGCValues values;

		valuemask = 0;
		colorGC = XCreateGC(dsp, RootWindow(dsp, DefaultScreen(dsp))
                        , valuemask, &values);
		XSetForeground(dsp, colorGC, WhitePixel(dsp, DefaultScreen(dsp)));
	}
	if (t->have_bgcolor) {
		XSetForeground(dsp, colorGC, t->bgcolor);
		XFillRectangle(dsp, XtWindow(hw->html.view), colorGC,
			rfr_x, rfr_y, rfr_w, rfr_h);
#if DEBUG_REFRESH
        fprintf(stderr,"[TableResfresh] Refresh <TABLE> x, y, w, h : %d, %d, %d, %d\n",              
                rfr_x, rfr_y, rfr_w, rfr_h);
#endif
	}

	cells_lines = t->row_list->cells_lines;

/* draw background of cell */
	for (i = 0; i < t->num_row; i++) {  
		for (j = 0; j < t->num_col; j++) {
			int cw1, ch1, cx, cy, r_cw1, r_ch1, r_cx, r_cy;

			cell = cells_lines[i][j];
			cw1 = cell.width - 1;  
			ch1 = cell.height - 1; 
			cx = cell.x - hw->html.scroll_x;
			cy = cell.y - hw->html.scroll_y;
			r_cx =	rfr_x;
			r_cy =	rfr_y;
			r_cw1 = rfr_w;
			r_ch1 = rfr_h;
			if ( cx > rfr_x ) {
				r_cx = cx;
				r_cw1 = r_cw1 - (cx - rfr_x);
			}
			if ( cy > rfr_y ) {
				r_cy = cy;
				r_ch1 = r_ch1 - (cy - rfr_y);
			}
			if ( (r_cw1 <= 0) || (r_ch1 <= 0) )	/* object is not in rfr part */
				continue;

			if ( (r_cx + r_cw1) > (cx + cell.width) ) { /* reduce width */
				r_cw1 = cx + cell.width - r_cx;
			}
			if ( (r_cy + r_ch1) > (cy + cell.height) ) {
				r_ch1 = cy + cell.height - r_cy;
			}
			if ( (r_cw1 <= 0) || (r_ch1 <= 0) )	/* object is not in rfr part */
				continue;
	
			if( (cell.back_rs == 0) && (cell.back_cs == 0) ) {
				if ( cell.have_bgcolor ) {
					XSetForeground(dsp, colorGC, cell.bgcolor);
					XFillRectangle(dsp, XtWindow(hw->html.view), colorGC,
						r_cx, r_cy, r_cw1, r_ch1);
#if DEBUG_REFRESH
        fprintf(stderr,"[TableResfresh] Refresh <TD/TH> x, y, w, h : %d, %d, %d, %d, type = %d\n",
                r_cx, r_cy, r_cw1, r_ch1, cell.cell_type);
#endif
				}
			}
		}
	}

/* ### trace seulement les contours de la table */
	if(! t->borders )
		return;

	if (ttopGC == NULL) {          
		char dash_list[2];         
		unsigned long valuemask;   
		XGCValues values;

		values.stipple = XCreateBitmapFromData(dsp,
			RootWindowOfScreen(scn),
			shadowpm_bits, shadowpm_width, shadowpm_height);
		values.fill_style = FillSolid;

		valuemask = GCFillStyle | GCStipple ;
		ttopGC = XCreateGC(dsp, RootWindow(dsp, DefaultScreen(dsp))
			, valuemask, &values);
		tbotGC = XCreateGC(dsp, RootWindow(dsp, DefaultScreen(dsp))
			, valuemask, &values);

		XSetLineAttributes(dsp, ttopGC, 1, LineOnOffDash,
			CapNotLast, JoinMiter); 
		XSetForeground(dsp, ttopGC, WhitePixel(dsp, DefaultScreen(dsp)));                  
		XSetLineAttributes(dsp, tbotGC, 0, LineOnOffDash,
			CapNotLast, JoinMiter); 
		XSetForeground(dsp, tbotGC, BlackPixel(dsp,
			DefaultScreen(dsp)));                  
		dash_list[0] = '\1';       
		dash_list[1] = '\1';       
		XSetDashes(dsp, ttopGC, 1, dash_list, 2);
		XSetDashes(dsp, tbotGC, 1, dash_list, 2);
	}                              
	XSetTSOrigin(dsp, ttopGC, hw->html.scroll_x % 2, hw->html.scroll_y % 2);                
	XSetTSOrigin(dsp, tbotGC, hw->html.scroll_x % 2, hw->html.scroll_y % 2);

	if (hw->html.cur_res.have_bgima) {       
		ltopGC = ttopGC;
		lbotGC = tbotGC;
	} else {                       
		ltopGC = hw->manager.top_shadow_GC;
		lbotGC = hw->manager.bottom_shadow_GC;
	}                              
	if (t->borders == 1) {         
		int y0, y1;                

/* top */                  
		if (y >= 0) {              
			XDrawLine(dsp, XtWindow(hw->html.view)
				, ltopGC, x, y, x + eptr->width - 1, y);
			y0 = y;                
		} else                     
			y0 = 0;                

		if (y + eptr->height <=  hw->html.view_height) {
			y1 = y + eptr->height - 1;  
			XDrawLine(dsp, XtWindow(hw->html.view)
				, lbotGC, x, y1, x + eptr->width - 1, y1);
		} else                     
			y1 = hw->html.view_height;  

/* draw left line */       
		XDrawLine(dsp, XtWindow(hw->html.view)
			, ltopGC, x, y0, x, y1);
		XDrawLine(dsp, XtWindow(hw->html.view)
			, lbotGC, x + eptr->width - 1, y0
			, x + eptr->width - 1, y1);
	} else {                       
		if (hw->html.cur_res.have_bgima) {   
			XSetFillStyle(dsp, ltopGC, FillStippled);
			XSetFillStyle(dsp, lbotGC, FillStippled);
		}                          
/* Draw shadows. Points are numbered as follows:
*                             
*     5 __________________________________________
*      |\                                        /4
*      | \                                      / |
*      |  \                                    /  |
*      |   2_________________________________ 3   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |__________________________________|   |
*      |   1                                   \  |
*      |  /                                     \ |
*      | /_______________________________________\|
*       0                     
*                             
*                             
*/                            

		pt[0].x = x;
		pt[0].y = y + eptr->height - 1; 
		pt[1].x = x + t->borders;  
		pt[1].y = y + eptr->height - t->borders - 1;
		pt[2].x = x + t->borders;  
		pt[2].y = y + t->borders;  
		pt[3].x = x + eptr->width - t->borders - 1;
		pt[3].y = y + t->borders;  
		pt[4].x = x + eptr->width - 1;  
		pt[4].y = y;               
		pt[5].x = x;               
		pt[5].y = y;               
		XFillPolygon(dsp, XtWindow(hw->html.view)
			, ltopGC, pt, 6, Complex, CoordModeOrigin);
/* Draw shadows. Points are numbered as follows:
*                             
*       __________________________________________
*      |\                                        /4
*      | \                                      / |
*      |  \                                    /  |
*      |   \_________________________________ 3   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |                                  |   |
*      |   |__________________________________2   |
*      |   1                                   \  |
*      |  /                                     \ |
*      | /_______________________________________\5
*       0                     
*                             
* only 2 and 5 changes        
*/                            
		pt[2].x = x + eptr->width - t->borders - 1;
		pt[2].y = y + eptr->height - t->borders - 1;
		pt[5].x = x + eptr->width - 1;  
		pt[5].y = y + eptr->height - 1; 
		XFillPolygon(dsp, XtWindow(hw->html.view)
			, lbotGC, pt, 6, Complex, CoordModeOrigin);

		if (hw->html.cur_res.have_bgima) {   
			XSetFillStyle(dsp, ltopGC, FillSolid);
			XSetFillStyle(dsp, lbotGC, FillSolid);
		}                          
	} 
	iseg = 0;                      

/*printf("scroll_x=%d, scroll_y=%d, view w=%d h=%d doc w=%d
h=%d\n"                                
, hw->html.scroll_x, hw->html.scroll_y
, hw->html.view_width, hw->html.view_height
, hw->html.doc_width, hw->html.doc_height);
*/

	for (i = 0; i < t->num_row; i++) {  
		for (j = 0; j < t->num_col; j++) {
			int cw1, ch1, cx, cy;  

			cell = cells_lines[i][j];
			cw1 = cell.width - 1;  
			ch1 = cell.height - 1; 
			cx = cell.x - hw->html.scroll_x;
			cy = cell.y - hw->html.scroll_y;
			if (cy + ch1 < 0)
				continue;   /* not visible : before */
			if (cy - 1 > hw->html.view_height)
				continue;   /* not visible : after */

			if( (cell.back_rs == 0) && (cell.back_cs == 0) ) {
				XSegment *pseg = segB + iseg;
/* top line */
				pseg->x1 = cx;
				pseg->y1 = cy;
				pseg->x2 = cx + cw1;
				pseg->y2 = cy;
				pseg++;
/* left line */
				pseg->x1 = cx;
				pseg->y1 = cy;
				pseg->x2 = cx;
				pseg->y2 = cy + ch1;

				pseg = segT + iseg;
/* bottom line */
				pseg->x1 = cx;
				pseg->y1 = cy + ch1;
				pseg->x2 = cx + cw1;
				pseg->y2 = cy + ch1;
				pseg++;
/* draw right line */
				pseg->x1 = cx + cw1;
				pseg->y1 = cy;
				pseg->x2 = cx + cw1;
				pseg->y2 = cy + ch1;
				iseg += 2;
				if (iseg > MAX_SEG - 1) {
					XDrawSegments(dsp, XtWindow(hw->html.view)
						, lbotGC, segB, iseg);
					XDrawSegments(dsp, XtWindow(hw->html.view)
						, ltopGC, segT, iseg);
					iseg = 0;
				}
			}
		}
	}
	if (iseg > 0) {                
		XDrawSegments(dsp, XtWindow(hw->html.view), lbotGC, segB, iseg);
		XDrawSegments(dsp, XtWindow(hw->html.view), ltopGC, segT, iseg);
	}                              
}

