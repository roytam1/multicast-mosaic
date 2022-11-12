/* HTMLtable.c
 * Author: Gilles Dauphin
 * Version 3.0.1 [Jan97]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :  dauphin@sig.enst.fr dax@inf.enst.fr
 */

#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <stdlib.h>
#include <ctype.h>

#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "HTML.h"
#include "list.h"

#define TBL_CELL_MARGIN_WIDTH	2

#define FONTHEIGHT(font) (font->max_bounds.ascent + font->max_bounds.descent)

void TableDump( TableInfo *t);

static void FreeColList(ColumnList * col_list)
{
#ifdef TO_DO
/* mjr: the ' confuses gcc-cpp */

	/* ####C'est a faire #### */
#endif
}
static void FreeRowlist( RowList * row_list)
{
#ifdef TO_DO
	/* ####C'est a faire #### */
#endif
}
static void FreeTableInfo(TableInfo * t)
{
#ifdef TO_DO
	/* ####C'est a faire #### */
#endif
}

/* #####Push the PhotoComposeContext (context) for M_TABLE ####
####            TablePlace(hw, mptr, x, y, #####a copy of chst ####);
###             get next mptr and look for caption or tr ###
###             if tr parse until (td or th) ####
###             if td or th alors FormatChunk (hw, mptr, x, y, copy of chst####
*/
void UpateColList( ColumnList ** col_list, int td_count,
		MarkType m_cell_type,
		struct mark_up * td_start_mark, struct mark_up * td_end_mark,
		int colspan,int rowspan)
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
		cells = (CellStruct *)malloc(sizeof(CellStruct)* cell_count );
	else
		cells = (CellStruct *)realloc(cl->cells, sizeof(CellStruct)* cell_count );

	cells[cur_cell_num].td_count = td_count;
	cells[cur_cell_num].colspan = colspan;
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

	nspan = colspan -1;
	cur_cell_num++;
	ns = nspan;
	for (i = 0; i< nspan; i++){
		cells[cur_cell_num].td_count = td_count;
		cells[cur_cell_num].colspan = ns;
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
		cur_cell_num++;
		ns--;
	}
	cl->cells=cells;
	cl->cell_count = cell_count;
	if (rowspan > cl->max_row_span)
		cl->max_row_span = rowspan;
	*col_list = cl;
}

void AddPadAtEndColList(ColumnList * cl, int toadd)
{
	int i;

	cl->cells = (CellStruct*)realloc(cl->cells,
		sizeof(CellStruct) * (cl->cell_count+toadd));
	for(i=cl->cell_count; i< (cl->cell_count+toadd); i++){
			cl->cells[i].td_count = 0;
			cl->cells[i].colspan = 1;
			cl->cells[i].rowspan = 1;
			cl->cells[i].back_cs = 0;
			cl->cells[i].back_rs = 0;
			cl->cells[i].td_start = NULL;
			cl->cells[i].td_end = NULL;
			cl->cells[i].height = 0;
			cl->cells[i].width = 0;
			cl->cells[i].is_colspan = 0;
			cl->cells[i].is_rowspan = 0;
			cl->cells[i].cell_type = M_TD_CELL_PAD;
	}
	cl->cell_count = cl->cell_count+toadd;
}
static void AddPadAtEndRowList(
	RowList * rl,
	int toadd,	/* # of cells to add at end of line */
	int from,	/*from = 0*/
	int to)		/*to= low_cur_line_num*/ /*exclu*/
			/* de low_cur_line_num inclu a row_count-1 mettre FREE */
{
	int i,j;

	for(i=0; i<rl->row_count;i++){ /* realloc more cell in each line */
		rl->cells_lines[i] = (CellStruct*)realloc(rl->cells_lines[i],
			sizeof(CellStruct)* (rl->max_cell_count_in_line + toadd));
	}
/* add PAD */
	for(i=0; i<rl->low_cur_line_num;i++){
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
			rl->cells_lines[i][j].cell_type = M_TD_CELL_PAD;
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
		}
	}
	rl->max_cell_count_in_line = rl->max_cell_count_in_line + toadd;
}

static void AddFreeLineToRow(RowList * rl, int toadd)
{
	CellStruct* ncl;
	int i,j;

	rl->cells_lines = (CellStruct**)realloc(
				rl->cells_lines,
				sizeof(CellStruct*) * (rl->row_count+toadd));
	for(j = 0; j< toadd; j++){
		ncl =(CellStruct*)malloc(
				sizeof(CellStruct) * rl->max_cell_count_in_line);
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
		}
		rl->cells_lines[rl->row_count] = ncl;
		rl->row_count++;
	}
}

static void UpdateRowList(RowList ** row_list, int tr_count,
		ColumnList * cl,
		struct mark_up * tr_start_mark,
		struct mark_up * tr_end_mark)
{
	RowList * rl;
	CellStruct work_cell;
	CellStruct ref_cell;
	CellStruct * rcl;
	CellStruct * this_line = NULL;
	int ncell_for_this_cl;
	int nrow_for_this_cl;
	int low_cur_line_num;
	int i,j;
	int jc, nr;
	int next_low_cur_line_num;
	int free_cell_found;
	int n_rl_free_cell;


	rl = *row_list;
	ncell_for_this_cl = cl->cell_count;
	nrow_for_this_cl = cl->max_row_span;
	if (rl == NULL){ /* Create one RowList */
		this_line = cl->cells;
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
					work_cell.back_rs++ ;
					work_cell.is_rowspan = 1;
					work_cell.td_start = NULL;
					work_cell.td_end = NULL;
					work_cell.cell_type = M_TD_CELL_PAD;
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
	if (n_rl_free_cell == 0){	/* add an empty line */
		AddFreeLineToRow(rl,1);
		low_cur_line_num++;
		rl->low_cur_line_num = low_cur_line_num;
		n_rl_free_cell = rl->max_cell_count_in_line;
	}
	if (ncell_for_this_cl< n_rl_free_cell){
		AddPadAtEndColList(cl,n_rl_free_cell - ncell_for_this_cl); 
	}
	if (ncell_for_this_cl > n_rl_free_cell){
		printf("BUG: number of TD/TH is false for this TABLE or span count is false.\n");
		printf("     padding the TABLE!!! Adjust....\n");
		AddPadAtEndRowList(rl,
				ncell_for_this_cl - n_rl_free_cell,
				/*from =*/ 0, /*to=*/ low_cur_line_num /*exclu*/);
		/* de low_cur_line_num inclu a row_count-1 mettre FREE */
	}
	this_line = cl->cells;
	rcl = rl->cells_lines[low_cur_line_num];
	nrow_for_this_cl = cl->max_row_span;

/*Si nrow_for_this_cl + low_cur_line_num > row_count */
/*etendre la table , augmenter le nombre de ligne avec partout FREE */
	if( (nrow_for_this_cl + low_cur_line_num) > rl->row_count){
		AddFreeLineToRow(rl,
			nrow_for_this_cl + low_cur_line_num - rl->row_count);
	}

/* maintenant cl->cell_count et n_rl_free_cell sont egaux */
/* le nombre de ligne dans rl est suffisant */
	jc = 0;
	for(i=0; i<rl->max_cell_count_in_line; i++){
		if ( rcl[i].cell_type == M_TD_CELL_FREE){
			ref_cell = this_line[jc];
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

TableInfo * FirstPasseTable(HTMLWidget hw, struct mark_up *mptr,
	PhotoComposeContext * pcc)
{
	char * val;
	char * tptr;
	TableInfo *t;
	struct mark_up * sm;
	struct mark_up * tb_start_mark;		/* save the marker <TABLE> */
	struct mark_up * tb_end_mark=NULL;	/* save the marker <TABLE> */
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

	tb_start_mark = mptr;
	psm = mptr;
	sm = mptr->next;

	t = (TableInfo *) malloc(sizeof(TableInfo));
	if (!t) {
		MEM_OVERFLOW;
	}
	td_count = 0;
	tr_count = 0;
	t->num_col = 0;
	t->num_row = 0;
	t->caption_start_mark = NULL;
	t->caption_end_mark = NULL;
	t->relative_width = 0;
	t->borders = 0;
	t->row_list = NULL;
	t->width = 0;
	t->height = 0;
	t->min_width =0;
	t->max_width =0;
	t->is_tint = 0;
	if (tptr=ParseMarkTag(mptr->start,MT_TABLE,"BORDER")){
		t->borders = atoi(tptr);
	}
	if (tptr=ParseMarkTag(mptr->start,MT_TABLE,"WIDTH")){
		t->relative_width = atoi(tptr);	/* the width wanted by user */
		if (strchr(tptr,'%') == NULL){ /* absolute value */
			t->relative_width = (t->relative_width*100)/pcc->cur_line_width;
		}
	}
	if (tptr=ParseMarkTag(mptr->start,MT_TABLE,"ALIGN"))
		printf("[MakeTable] %s not yet implemented\n","ALIGN");
	if (tptr=ParseMarkTag(mptr->start,MT_TABLE,"CELLSPACING"))
		printf("[MakeTable] %s not yet implemented\n","CELLSPACING");
	if (tptr=ParseMarkTag(mptr->start,MT_TABLE,"CELLPADDING"))
		printf("[MakeTable] %s not yet implemented\n","CELLPADDING");

	/* find the first TR or CAPTION */
	caption_found = 0;
	tr_found = 0;
	while ( sm ){
		if((sm->type== M_CAPTION) && (!sm->is_end)){
			t->captionAlignment = ALIGN_BOTTOM;
			if (ParseMarkTag(sm->start,MT_CAPTION,"top"))
				t->captionAlignment = ALIGN_TOP;
			caption_found = 1;
			caption_start_mark = sm;
			break;
		}
		if((sm->type == M_TABLE_ROW || sm->type == M_TABLE_DATA ||
		    sm->type == M_TABLE_HEADER) && (!sm->is_end)){
			tr_found = 1;
			tr_start_mark = sm;
			break;
		}
		sm = sm->next;
	}
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
			printf("</CAPTION> not found\n");
			return NULL;
		}
		t->caption_start_mark = caption_start_mark;
		t->caption_end_mark = caption_end_mark;
		printf("<CAPTION> not yet fully implemented\n");
	}
	/* now find the first <TR> */
	if ( ! tr_found ){
		tr_start_mark = caption_end_mark;
		while(tr_start_mark){
			if((tr_start_mark->type == M_TABLE_ROW) &&
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

	while (sm){
/* Traitement de <TD> ou <TH> */
		if( ((sm->type == M_TABLE_DATA) || (sm->type == M_TABLE_HEADER))&&
		    (!sm->is_end) ) { 		/* <TH> or <TD> */
			if (!tr_start_found){
				printf("A <TD> <TH> is outside a <TR>\n");
				psm = sm;
				sm = sm->next;
				continue;
			}
			if ( td_start_found) { /* this is the end of previous */
				td_count++;
				td_end_mark = psm;
				UpateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan);
			}
			if(sm->type == M_TABLE_DATA){
				m_cell_type = M_TABLE_DATA;
				mt_cell_type = MT_TABLE_DATA;
			} else {
				m_cell_type = M_TABLE_HEADER;
				mt_cell_type = MT_TABLE_HEADER;
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
			psm = sm;
			sm = sm->next;
			continue;
		}				/* <TH> or <TD> */
		if( ((sm->type == M_TABLE_DATA) || (sm->type == M_TABLE_HEADER))&&
		    (sm->is_end) ) { 		/* </TH> or </TD> */
			if (!tr_start_found){
                                printf("A <TD> <TH> is outside a <TR>\n");
				psm = sm;
				sm = sm->next;
                                continue;
                        }
			if (!td_start_found){
                                printf("A </TD> is without a <TD>\n");
				psm = sm;
				sm = sm->next;
                                continue;
                        }
			td_count++;
			td_end_mark = sm;
			UpateColList(&col_list,td_count,m_cell_type,
				td_start_mark,td_end_mark,
				colspan,rowspan);
			td_start_found = 0;
			psm = sm;
			sm = sm->next;
			continue;
		}
/* jusqu'a fin </TD> ou </TH> */

/* Traitement <TR> */
		if ((sm->type == M_TABLE_ROW) && (!sm->is_end)){
			if (td_start_found){
				td_count++;
				td_end_mark = psm;
				UpateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan);
				td_start_found = 0;
			}
			if (tr_start_found) {
				tr_end_mark = psm;
				tr_count++;
				if (col_list){
					UpdateRowList(&row_list,tr_count,col_list,
						tr_start_mark,tr_end_mark);
					col_list = NULL;
					td_count =0;
				}else{
					printf("<TR> without<TD>: empty line!\n");
				}
			}
			tr_start_found = 1;
			tr_start_mark = sm;
			psm = sm;
			sm = sm->next;
			continue;
		}
		if ((sm->type == M_TABLE_ROW) && (sm->is_end)){
			if (!tr_start_found){
				printf("A </TR> without <TR>\n");
				psm = sm;
				sm = sm->next;
				continue;
			}
			if (td_start_found){
				td_count++;
				td_end_mark = psm;
				UpateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan);
				td_start_found = 0;
			}
			tr_count++;
			tr_end_mark = sm;
			if(col_list){
				UpdateRowList(&row_list,tr_count,col_list,
					tr_start_mark,tr_end_mark);
				col_list = NULL;
				td_count =0;
			} else {
				printf("<TR> without<TD>: empty line!\n");
			}
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
				UpateColList(&col_list,td_count,m_cell_type,
					td_start_mark,td_end_mark,
					colspan,rowspan);
				td_start_found = 0;
			}
			if (tr_start_found) {
				tr_end_mark = psm;
				tr_count++;
				if(col_list){
					UpdateRowList(&row_list,tr_count,col_list,
						tr_start_mark,tr_end_mark);
					col_list = NULL;
					td_count =0;
				} else {
					printf("<TR> without<TD> and </TABLE> see!: Buggy TABLE !!!\n");
					break;
				}
			}
			if ( tr_count == 0)
				break;
			tb_end_mark = sm;
			/* UpdateTableInfo */
			t->num_col = row_list->max_cell_count_in_line ;
			t->num_row = row_list->row_count;
			t->tb_end_mark = tb_end_mark;
			t->tb_start_mark = tb_start_mark;
			t->row_list = row_list;
			row_list=NULL;
			tr_count =0;
			break;
		}

/* traitement d'une table dans une table par recursivite */
		if( (sm->type == M_TABLE) && (!sm->is_end)){ /*a table in table*/
			TableInfo *tt;

			tt = FirstPasseTable(hw, sm, pcc); /* be recursive */
			if (!tt){
				printf("table have no column or row!\n");
				return NULL;
			}
			tt->is_tint = 1;
			sm->table_info1= tt;
			psm = sm;
			sm = tt->tb_end_mark->next;
			continue;
		}
		psm = sm;
		sm=sm->next;
	}
	if(!tb_end_mark){
		printf("Probleme the end table </TABLE> is not see\n");
		if(col_list)
			FreeColList(col_list);
		if(row_list)
			FreeRowlist(row_list);
		FreeTableInfo(t);
		return NULL;
	}
	return t;
}

void EstimateMinMaxTable(HTMLWidget hw, TableInfo *t,PhotoComposeContext * orig_pcc)
{
	PhotoComposeContext deb_pcc;
	PhotoComposeContext fin_pcc;
	int i,j;
	CellStruct * line;
	CellStruct  cell;
	int line_min_w ;
	int line_max_w ;
	int estimate_height;
	int h_row=0;

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

	t->col_max_w = (int*) malloc(t->num_col* sizeof(int));
	t->col_min_w = (int*) malloc(t->num_col* sizeof(int));
	t->col_w = (int*) malloc(t->num_col* sizeof(int));
	for(i=0;i<t->num_col;i++){
		t->col_max_w[i] = 0;
		t->col_min_w[i] = 0;
		t->col_w[i] = 0;
	}

	for(i=0; i< t->num_row; i++){
		line = t->row_list->cells_lines[i];
		h_row = 0;
		for(j=0; j<t->num_col; j++){ /* prendre chaque element */
			fin_pcc = deb_pcc;
			cell = line[j];		/* un element */
			if(cell.cell_type == M_TABLE_HEADER){
				fin_pcc.cur_font = hw->html.bold_font;
			}
			FormatChunk(hw,cell.td_start,cell.td_end,&fin_pcc,0);
			line[j].min_width = 1+fin_pcc.computed_min_x/cell.colspan;
			line[j].max_width = 1+fin_pcc.computed_max_x/cell.colspan;
			if (t->col_min_w[j] < line[j].min_width)
				t->col_min_w[j] = line[j].min_width;
			if (t->col_max_w[j] < line[j].max_width)
				t->col_max_w[j] = line[j].max_width;
			if ( fin_pcc.cur_line_height > h_row)
				h_row = fin_pcc.cur_line_height;
		}
		for(j=0; j<t->num_col; j++)
			line[j].height = h_row;
		estimate_height += h_row;
	}
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

	t->min_width = line_min_w;
	t->max_width = line_max_w;
	t->estimate_height = estimate_height;
}

void TablePlace(HTMLWidget hw, struct mark_up **mptr, PhotoComposeContext * pcc)
{
	struct mark_up *sm;
	TableInfo *t;
	int w_cell;
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
	struct ele_rec * eptr;
	int max_line_number;
	int w_in_cell;
	int tbl_border;
	int line_som_bdw;
	int hbw;
	int to_add_col;
	int wanted_w;

        if ((*mptr)->is_end)            /* end of table */
                return;
	sm = *mptr;			/* save marker */

/* Faire une prepasse pour compter le nombre de colonne et de ligne */
/* capturer les markeurs de fin */

	if (!sm->table_info1){		/* do the prepasse */
        	if (htmlwTrace) {
                	fprintf(stderr,"[TablePlace] pcc->x=%d, pcc->y=%d\n",
				pcc->x,pcc->y);
        	}
		t = FirstPasseTable(hw,sm,pcc);	/* the First passe */
		if (!t){
			printf("table have no column or row!\n");
			return;
		}
		sm->table_info1= t;
	}
	t = sm->table_info1;

/* once we have a table, we compute the min and max size of each cell */
/* save the contexte for each cell, parse beetween marker , the return context */
/* give the size. When doing this NEVER create element */
/* save the img struct or aprog struct in the mark_up struct, NOT in */
/* ele_rec struct */

#ifdef TODO
	Faire un premier tour pour caption avec un pcc artificiel
	stocker le pcc_caption(min,max)
#endif

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
/*
	if (!t->is_tint){
*/
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
/*
/*	} else {	/* une table dans une table doit prendre la dim exacte*/
/*
		int gw;
		int mo;
		int cw;

		gw = pcc->parent_html_object_desc->inner_width;
		cw = gw / t->num_col;
		mo = gw % t->num_col; 

		if (cw == 0) cw = 1;
		for (i=0;i<t->num_col;i++){
			t->col_w[i] = cw;
		}
		t->col_w[t->num_col -1] += mo;
	}
*/

/* maintenant on peut calculer la largeur de la table */
	w_table = 0;
	for(i=0; i<t->num_col;i++){
		w_table += t->col_w[i]+ 2 * TBL_CELL_MARGIN_WIDTH;
	}
	w_table +=  (t->num_col -1) * t->borders;
	tbl_border = t->borders;
	hbw = t->borders/2;
	w_table = w_table + 2 * tbl_border- hbw;	/* Pour un TEST */

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


/*#######*/
/* on place le CAPTION toujours au debut */
	if(t->caption_start_mark){
		FormatChunk(hw,t->caption_start_mark,t->caption_end_mark,
					pcc, False);
		LineBreak(hw,t->caption_end_mark,pcc);
	}
/*#######*/

	tbl_pcc = *pcc;

	
/* now compute the real width of cell and create element in cell */
/*somme des bords dans une ligne */
	line_som_bdw = (t->num_col - 1) * tbl_border; 

/* set the default height of cell */
	h_def_height_cell = pcc->cur_font->ascent+pcc->cur_font->descent;
	line_pcc = tbl_pcc;
	line_pcc.y = tbl_pcc.y ;
	line_pcc.eoffsetx = tbl_pcc.left_margin+tbl_pcc.eoffsetx;
	line_pcc.line_number++;
	max_line_number = line_pcc.line_number;
	max_line_bot = line_pcc.y+ h_def_height_cell;
	for(i=0; i< t->num_row; i++){
		int cell_offset;

		line = t->row_list->cells_lines[i];
		cell_offset =0;
		for(j=0; j<t->num_col; j++){ /* prendre chaque element */
			int add_offset;
			ParentHTMLObjectDesc p_desc;

			w_cell = t->col_w[j]; /* set final width of cell */
					/* just for test: set equal width */
			w_in_cell = w_cell;
			cell = line[j];		/* un element */
			work_pcc = line_pcc;	/* en prendre un pour travailler*/
/* what is the type of this cell */
			add_offset = w_in_cell +
				2*(TBL_CELL_MARGIN_WIDTH + t->borders)-hbw;
			cell.width = w_in_cell +
				 2*(TBL_CELL_MARGIN_WIDTH + t->borders) - hbw;
			cell.y = line_pcc.y; 
			cell.height = work_pcc.cur_line_height;
			cell.line_bottom = line_pcc.y+
                                                line_pcc.cur_line_height;
			switch (cell.cell_type){
			case M_TD_CELL_PAD:
				cell.x = cell_offset + line_pcc.eoffsetx;
				break;
			case M_TD_CELL_FREE:
				cell.x = cell_offset + line_pcc.eoffsetx;
				break;
			case M_TABLE_DATA:
			case M_TABLE_HEADER:
				for(k=1; k< cell.colspan;k++){
					w_in_cell = w_in_cell + t->col_w[j+k];
				}
				w_in_cell = w_in_cell + (cell.colspan -1)*
				           (2*TBL_CELL_MARGIN_WIDTH + t->borders);
				work_pcc.left_margin = TBL_CELL_MARGIN_WIDTH +
							t->borders;
				work_pcc.right_margin = TBL_CELL_MARGIN_WIDTH;
				work_pcc.cur_line_width = w_in_cell;
				work_pcc.eoffsetx = line_pcc.eoffsetx+cell_offset;
				work_pcc.x = work_pcc.eoffsetx + 
					     work_pcc.left_margin;
				work_pcc.y = line_pcc.y;
				work_pcc.have_space_after = 0;
				if(cell.cell_type == M_TABLE_HEADER){
					work_pcc.cur_font = hw->html.bold_font;
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

				p_desc.type = cell.cell_type;
				p_desc.inner_width = w_in_cell;
				p_desc.inner_height = cell.height;
				p_desc.el = (void*) &cell;
				work_pcc.parent_html_object_desc = &p_desc;

				FormatChunk(hw,cell.td_start,cell.td_end,
					&work_pcc, False);
				work_pcc.parent_html_object_desc = NULL;
/* add little vertical space */
				LineBreak(hw,cell.td_end,&work_pcc);
				cell.end_elem = hw->html.last_formatted_elem;
/*difference des pcc pour determiner la hauteur*/
				cell.x = cell_offset + line_pcc.eoffsetx;
				cell.width = w_in_cell +
					   2*(TBL_CELL_MARGIN_WIDTH + t->borders)
					   - hbw;
				cell.y = line_pcc.y; 
				cell.height = work_pcc.y - line_pcc.y+
						line_pcc.cur_font->ascent/2;
				cell.line_bottom = work_pcc.y+
                                                line_pcc.cur_font->ascent/2;
				if(max_line_number < work_pcc.line_number)
					max_line_number = work_pcc.line_number;
				break;
			default:
				printf("BUG: Unknow cell type in TABLE\n");
			}
			cell_offset += add_offset - t->borders+hbw;
/* if (cellule_seule ou cellule_fin_rowspan) */
			if(cell.rowspan == 1){
				if (cell.line_bottom > max_line_bot)
					max_line_bot = cell.line_bottom;
			}
			line[j] = cell;
			line_pcc.widget_id = work_pcc.widget_id;
			line_pcc.element_id = work_pcc.element_id;
			line_pcc.internal_mc_eo = work_pcc.internal_mc_eo;
			line_pcc.aprog_id = work_pcc.aprog_id;
		}
/*Ajuster les hauteurs des 'cells'*/
/*Faire attention au span en ligne et colonne */
/* pour chaque cellule dans la ligne voir si c'est la fin d'un 'rowspan'*/
/* si oui (ou cellule seule) ajuster la hauteur pour que les lignes du bas */
/* soient alignees */
/* Si cellule seule ou fin d'un row span , alors */
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
		line_pcc.y = max_line_bot;
		max_line_number++;
		line_pcc.line_number = max_line_number;
	}

	pcc->widget_id = line_pcc.widget_id;
	pcc->element_id =line_pcc.element_id ;
	pcc->internal_mc_eo = line_pcc.internal_mc_eo ;
	pcc->aprog_id =	line_pcc.aprog_id ;

	t->width = w_table;
	t->height = h_table;
/* creer l'element graphique qui entoure la table */
	eptr = CreateElement(hw,E_TABLE, pcc->cur_font,
		pcc->x, pcc->y, w_table, h_table, h_table, pcc); 

	pcc->line_number = max_line_number;
	eptr->underline_number = 0; /* Table's can't be underlined */
	eptr->table_data = t;

/* adjust the mptr at end */
	*mptr = t->tb_end_mark;		/* advance mark pointer to end of table */

/* adjust pcc */

        pcc->x += eptr->width;
	pcc->y += eptr->height;
	pcc->div = DIV_ALIGN_LEFT;
/* do a linefeed */
	LinefeedPlace(hw,t->tb_end_mark,pcc);

	if (htmlwTrace)
		TableDump(t);
}


/* display table */
void TableRefresh( HTMLWidget hw, struct ele_rec *eptr)
{
	int x,y; 		/* table origin */
	TableInfo * t;
	CellStruct cell;
	CellStruct **cells_lines;
	int i,j;
	int hbw;			/* half border-width */

/* ### trace seulement les contours de la table */
	if(! eptr->table_data->borders )
		return;
	x = eptr->x;
	y = eptr->y;
	x = x - hw->html.scroll_x;
	y = y - hw->html.scroll_y;
	t = eptr->table_data;
	hbw = t->borders/2;
	XSetLineAttributes(XtDisplay(hw), hw->html.drawGC,
			   t->borders,
			   LineSolid, CapNotLast, JoinMiter);
	XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
	XSetBackground(XtDisplay(hw), hw->html.drawGC, eptr->bg);

	XDrawRectangle(XtDisplay(hw), XtWindow(hw->html.view),
		hw->html.drawGC, 
		x+hbw, y+hbw,
		eptr->width - t->borders+hbw,
		eptr->height - t->borders+hbw);

	cells_lines = t->row_list->cells_lines;
	for (i = 0; i < t->num_row; i++) {
		for (j = 0; j < t->num_col; j++) {
			cell = cells_lines[i][j];
			if( (cell.back_rs == 0) && (cell.back_cs == 0) ) {
				XDrawRectangle(XtDisplay(hw), 
					XtWindow(hw->html.view),
					hw->html.drawGC, 
					cell.x+hbw - hw->html.scroll_x,
					cell.y+hbw - hw->html.scroll_y,
					cell.width-t->borders+hbw, 
					cell.height-t->borders+hbw);
			}
		}
	}	
	XSetLineAttributes(XtDisplay(hw), hw->html.drawGC,
			   1, LineSolid, CapNotLast, JoinMiter);
}


/* print out the table to stdout */
void TableDump( TableInfo *t)
{
	register int x,y;

	if (htmlwTrace) {
		fprintf(stderr,"Table dump:\n");
		fprintf(stderr,"Border width is %d\n",t->borders);
		fprintf(stderr,"numColumns=%d, numRows=%d\n",
				t->num_col,t->num_row);
		fprintf(stderr,"----------------------------------------\n");
		for (y = 0; y < t->num_row; y++ ) {
			fprintf(stderr,"|");
			for (x = 0; x < t->num_col; x++ ) {
				fprintf(stderr,"colWidth=%d,rowHeight=%d | ",
					t->row_list->cells_lines[y][x].width,
				       t->row_list->cells_lines[y][x].height);
			}
			fprintf(stderr,"\n----------------------------------\n");
		}
	}
}
