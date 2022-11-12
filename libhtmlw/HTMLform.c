/* Some part of this file is Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */ 

#include "HTMLP.h"
#include "HTMLPutil.h"
#include "../src/mosaic.h"
#include "HTMLform.h"


typedef enum { 
	InputTypeText,
	InputTypePassword,
	InputTypeCheckbox,
	InputTypeRadio,
	InputTypeSubmit,
	InputTypeReset,
	InputTypeFile,
	InputTypeHidden,
	InputTypeImage,
	InputTypeTextArea
} FormInputType;

static void ProcessOption( SelectInfo *sptr);

/* Fillout forms.  Cannot be nested. */

/*</FORM> */
void EndForm(HTMLWidget hw, struct mark_up ** mptr,
	PhotoComposeContext * pcc)
{
	if (pcc->cw_only) {	/* just compute the size of a form */
				/* but the tag by itself have no size */
		pcc->in_form = False; /* always put False on end Form */
				/* because form canont be nested */
		return;		/* just return */
	}
/* Here we go to create it really */
	if ( !pcc->in_form && !pcc->cur_form){ /* its an error */
		return;
	}

	pcc->cur_form->end = pcc->widget_id;
	AddNewForm(hw, pcc->cur_form);
	pcc->cur_form = NULL;
	pcc->in_form = False; /* always put False on end Form */
}

/* <FORM> */
void BeginForm(HTMLWidget hw, struct mark_up ** mptr, 
        PhotoComposeContext * pcc)
{
	struct mark_up * mark = *mptr;

	if(pcc->cw_only){	/* just compute the size of a form */
		if(pcc->in_form) { /* we are still in a form. Error!!! */
#ifdef HTMLTRACE
			fprintf(stderr,"Warning: A Form in Form !!!\n");
#endif
			return;
		}
		pcc->in_form = True;
		return;		/* just return */
	}

	if ( pcc->in_form || pcc->cur_form){ /* its an error */
#ifdef HTMLTRACE
		fprintf(stderr,"Warning: A Form in Form !!!\n");
#endif
		return;
	}

/* create a Form structure */
	pcc->cur_form = (FormInfo *)malloc(sizeof(FormInfo));
	pcc->cur_form->next = NULL;
	pcc->cur_form->hw = (Widget)hw;
	pcc->cur_form->action = ParseMarkTag(mark->start,
			MT_FORM, "ACTION");
	pcc->cur_form->method = ParseMarkTag(mark->start,
			MT_FORM, "METHOD");
	pcc->cur_form->enctype = ParseMarkTag(mark->start,
			MT_FORM, "ENCTYPE");
	pcc->cur_form->start = pcc->widget_id;
	pcc->cur_form->end = -1;
	pcc->cur_form->button_pressed=NULL;
	pcc->in_form = True;
}


/* Just insert the widget. Can only be inside a FORM tag.
 * Special case the type=image stuff to become a special IMG tag.
 */
void FormInputField(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	char *tptr2, * tptr, *sptr;
	int dir, ascent, descent;
	XCharStruct all;
	char * text;
	int m_w, def_w, def_h, l;
	int cols;
	FormInputType input_type=InputTypeText;
	char * value;

	if ( !pcc->in_form)		/* error */
		return;

	pcc->have_space_after = 0;
	pcc->is_bol = 0;
	pcc->pf_lf_state = 0;

	XTextExtents(pcc->cur_font, "m", strlen("m"),
		&dir, &ascent, &descent, &all);
/* ##### use template TextField to compute the size */
	m_w = all.width;
	def_w = all.width;
	def_h = pcc->cur_font->ascent+pcc->cur_font->descent;

	text = (*mptr)->start;
	tptr = ParseMarkTag(text, MT_INPUT, "TYPE");
	if (tptr == NULL){ 		/* assume type = TEXT */
		input_type = InputTypeText;
		cols = 40;
		sptr = ParseMarkTag(text, MT_INPUT, "SIZE");
		if ( sptr != NULL){
			cols = atoi(sptr);
			free(sptr);
		}
		def_w = m_w * cols;
	}else {
		if ( !strcasecmp(tptr, "text") ){
			input_type = InputTypeText;
			cols = 40;
			sptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if ( sptr != NULL){
				cols = atoi(sptr);
				free(sptr);
			}
			def_w = m_w * cols;
		} else if ( !strcasecmp(tptr, "password") ){
			input_type = InputTypePassword;
			cols = 40;
			sptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if ( sptr != NULL){
				cols = atoi(sptr);
				free(sptr);
			}
			def_w = m_w * cols;
		} else if ( !strcasecmp(tptr, "checkbox") ){
			input_type = InputTypeCheckbox;
			def_w = 2 * m_w ;
		} else if ( !strcasecmp(tptr, "radio") ){
			input_type = InputTypeRadio;
			def_w = 2 * m_w ;
		} else if ( !strcasecmp(tptr, "submit") ){
			input_type = InputTypeSubmit;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if ((value == NULL)||(*value == '\0')) {
				value = strdup("Submit");
			}
			l = strlen ( value);
			def_w = m_w * l;
			free(value);
		} else if ( !strcasecmp(tptr, "reset") ){
			input_type = InputTypeReset;
			value = ParseMarkTag(text, MT_INPUT, "VALUE");
			if ((value == NULL)||(*value == '\0')) {
				value = strdup("Reset");
			}
			l = strlen ( value);
			def_w = m_w * l;
			free(value);
		} else if ( !strcasecmp(tptr, "file") ){
			input_type = InputTypeFile;
			cols = 40;
			sptr = ParseMarkTag(text, MT_INPUT, "SIZE");
			if ( sptr != NULL){
				cols = atoi(sptr);
				free(sptr);
			}
			def_w = m_w * cols;
		} else if ( !strcasecmp(tptr, "hidden") ){
			input_type = InputTypeHidden;
			def_w = 0;
			def_h = 0;
		} else if ( !strcasecmp(tptr, "image") ){
			input_type = InputTypeImage;
			def_w = 20;	/* ######## pour l'instant !!! */
			def_h = 20;	/* a calculer.. ######## */
		} else {	/* error */
			free (tptr);
			return;
		}
	}

	if(pcc->computed_min_x < (def_w + pcc->eoffsetx+pcc->left_margin)){
		pcc->computed_min_x = def_w + pcc->eoffsetx + pcc->left_margin;
	}
	if(pcc->cw_only ) { 		/* compute size only */
		if (pcc->cur_line_height < def_h )
			pcc->cur_line_height = def_h;
		pcc->x = pcc->x + def_w;
		if (pcc->x > pcc->computed_max_x)  
			pcc->computed_max_x = pcc->x;
		if (tptr) free (tptr);
		return;
	}
	if ( !pcc->cur_form) {		/* error */
		if (tptr) free (tptr);
		return;
	}

	if (tptr && !strcasecmp(tptr, "image")) {
		free(tptr);
		tptr = (char *)malloc( strlen((*mptr)->start) +
				strlen(" ISMAP") + strlen(MT_IMAGE) -
				strlen(MT_INPUT) + 1);
		strcpy(tptr, MT_IMAGE);
		strcat(tptr, (char *) ((*mptr)->start + strlen(MT_INPUT)));
		strcat(tptr, " ISMAP");
		tptr2 = (*mptr)->start;
		(*mptr)->start = tptr;
		ImagePlace(hw, *mptr, pcc);
		(*mptr)->start = tptr2;
		free(tptr);
	} else if (tptr && !strcasecmp(tptr, "hidden")) {
/* hidden inputs have no element associated with them, just a widget record. */
		free(tptr);
		pcc->widget_id++;
		(void)MakeWidget(hw, (*mptr)->start, pcc, pcc->widget_id);
	} else {
		if (tptr) free(tptr);
		WidgetPlace(hw, *mptr, pcc);
	}
}

/* TEXTAREA is a replacement for INPUT type=text size=rows,cols
 * name REQUIRED
 * rows REQUIRED
 * cols REQUIRED
 */             
void FormTextAreaBegin(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	char *buf;
	int len;        
	struct mark_up * mark = *mptr;
	char *cptr, *rptr;
	int dir, ascent, descent;
	XCharStruct all;
	char * text;
	int m_w, def_w, def_h;
	int cols, rows;

	if ( !pcc->in_form)		/* error */
		return;

	pcc->have_space_after = 0;
	pcc->is_bol = 0;
	pcc->pf_lf_state = 0;

	XTextExtents(pcc->cur_font, "m", strlen("m"),
		&dir, &ascent, &descent, &all);
/* ##### use template TextArea to compute the size */
	m_w = all.width;
	def_w = all.width;
	def_h = pcc->cur_font->ascent+pcc->cur_font->descent;

	text = (*mptr)->start;
	cols = 40;
	cptr = ParseMarkTag(text, MT_INPUT, "COLS");
	if ( cptr != NULL){
		cols = atoi(cptr);
		free(cptr);
	}
	if (cols <=0)
		cols = 40;
	def_w = m_w * cols;

	rows = 4; 
	rptr = ParseMarkTag(text, MT_INPUT, "ROWS");
	if (rptr != NULL) {
		rows = atoi(rptr);
		free(rptr);
	}
	if ( rows <= 0)
		rows = 4; 
	def_h = (pcc->cur_font->ascent+pcc->cur_font->descent) * rows;

	if(pcc->cw_only ) { 		/* compute size only */
		if (pcc->cur_line_height < def_h )
			pcc->cur_line_height = def_h;
		pcc->x = pcc->x + def_w;
		return;
	}

	if (pcc->cur_form == NULL)	/* error */
		return;

	if (pcc->text_area_buf == NULL) {
/* Construct  the start of a fake INPUT tag. */
		len = strlen(MT_INPUT) + strlen( " type=textarea value=\"\"");
		buf = (char *)malloc(len + strlen(mark->start)+1);
		strcpy(buf, MT_INPUT);
		strcat(buf, (char *) (mark->start + strlen(MT_TEXTAREA)));
		strcat(buf, " type=textarea");
		strcat(buf, " value=\"");
		pcc->text_area_buf = buf;  
		pcc->ignore = 1;    
	}                      
}

void FormTextAreaEnd(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	struct mark_up * mark = *mptr;
	char *start;
	char *buf;

	if (!pcc->in_form)
		return;
        if(pcc->cw_only ) {             /* compute size only */
                return;
        } 
	if (pcc->cur_form == NULL)	/* error */
		return;

	if (pcc->text_area_buf == NULL)	/* error */
		return;

/* Finish a fake INPUT tag. */
	buf = (char *)malloc( strlen(pcc->text_area_buf) + 2); 
	strcpy(buf, pcc->text_area_buf);
	strcat(buf, "\"");
/* stick the fake in, saving the real one. */
	start = mark->start;
	mark->start = buf;
	mark->is_end = 0;
/*####attention a pcc->cw_only et cur_form ###*/
	WidgetPlace(hw, mark, pcc);
/* free the fake, put the original back */
	free(buf);
	free(pcc->text_area_buf);
	mark->start = start;
	mark->is_end = 1;
	pcc->text_area_buf = NULL;
	pcc->ignore = 0;
}


/* Can only be inside a SELECT tag. */
void FormSelectOptionField(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	struct mark_up * mark = *mptr;
	char *tptr;

	if ( !pcc->in_form)		/* error */
		return;
	if ( !pcc->in_select)		/* error */
		return;
	if(pcc->cw_only ) { 		/* compute size only */
#ifdef TO_DO
/* c'est a faire */
#endif
		return;
	}
	if ( !pcc->cur_form) {		/* error */
		return;
	}

	if (pcc->current_select != NULL) {
		if (pcc->current_select->option_buf != NULL)
			ProcessOption(pcc->current_select);
		pcc->current_select->option_buf = (char *)malloc(1);
		strcpy(pcc->current_select->option_buf, "");
/* Check if this option starts selected */
		tptr = ParseMarkTag(mark->start, MT_OPTION, "SELECTED");
		if (tptr != NULL) {
			pcc->current_select->is_value = 1;
			free(tptr);
		} else {
			pcc->current_select->is_value = 0;
		}       
/* Check if this option has an different return value field. */
		tptr = ParseMarkTag(mark->start, MT_OPTION, "VALUE");
		if (tptr != NULL) {
			if (*tptr != '\0') {
				pcc->current_select->retval_buf = tptr;
			} else {
				pcc->current_select->retval_buf = NULL;
				free(tptr); 
			}          
		} else {       
			pcc->current_select->retval_buf = NULL;
		}              
	}                      
}

/* Special INPUT tag.  Allows an option menu or a scrolled list.
 * Due to a restriction in SGML, this can't just be a subset of
 * the INPUT markup.  However, I can treat it that way to avoid duplicating
 * code. As a result I combine SELECT and OPTION into a faked up INPUT mark.
 */
void FormSelectBegin(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	if (!pcc->in_form)
		return;
        if(pcc->cw_only ) {             /* compute size only */
#ifdef TO_DO
/*###       faire qqes choses et detecter la fin de Select             */
#endif
                return;
        } 
	if (pcc->cur_form == NULL)	/* error */
		return;
	if (pcc->current_select == NULL) {
		pcc->current_select = (SelectInfo *)malloc( sizeof(SelectInfo));
		pcc->current_select->hw = (Widget)hw;
		pcc->current_select->mptr = *mptr;
		pcc->current_select->option_cnt = 0;
		pcc->current_select->returns = NULL;
		pcc->current_select->retval_buf = NULL;
		pcc->current_select->options = NULL;
		pcc->current_select->option_buf = NULL;
		pcc->current_select->value_cnt = 0;
		pcc->current_select->value = NULL;
		pcc->current_select->is_value = -1;
		pcc->ignore = 1;
	}                      
	pcc->in_select = True;
}

void FormSelectEnd(HTMLWidget hw, struct mark_up ** mptr,
        PhotoComposeContext * pcc)
{
	char *start;
	int len;  
	char *buf;  
	char *options, *returns, *value;

	if (!pcc->in_form)
		return;
        if(pcc->cw_only ) {             /* compute size only */
#ifdef TO_DO
/*###       faire qqes choses              */
#endif
                return;
        } 
	if (pcc->cur_form == NULL)	/* error */
		return;

	if (pcc->current_select == NULL)	/* error */
		return;

        if (pcc->current_select->option_buf != NULL)
		ProcessOption(pcc->current_select);
	options = ComposeCommaList( pcc->current_select->options,
			pcc->current_select->option_cnt);
	returns = ComposeCommaList( pcc->current_select->returns,
			pcc->current_select->option_cnt);
	value = ComposeCommaList( pcc->current_select->value,
			pcc->current_select->value_cnt);
	FreeCommaList( pcc->current_select->options,
			pcc->current_select->option_cnt);   
	FreeCommaList( pcc->current_select->returns,
			pcc->current_select->option_cnt);
	FreeCommaList( pcc->current_select->value,
			pcc->current_select->value_cnt);
/* Construct a fake INPUT tag. */
	len = strlen(MT_INPUT) + strlen(options) +
		strlen(returns) + strlen(value) + 
		strlen(" type=select options=\"\" returns=\"\" value=\"\"");
	buf = (char *)malloc(len + strlen(pcc->current_select->mptr->start) + 1);
	strcpy(buf, MT_INPUT);
	strcat(buf, " type=select");
	strcat(buf, " options=\"");
	strcat(buf, options);
	strcat(buf, "\" returns=\"");
	strcat(buf, returns);
	strcat(buf, "\" value=\"");
	strcat(buf, value);
	strcat(buf, "\"");  
	strcat(buf, (char *) (pcc->current_select->mptr->start + strlen(MT_SELECT)));
/* stick the fake in, saving the real one. */
	start = pcc->current_select->mptr->start;
	pcc->current_select->mptr->start = buf;
/*####attention a pcc->cw_only et cur_form ###*/
	WidgetPlace(hw, pcc->current_select->mptr, pcc);
/* free the fake, put the original back */
	free(buf);
	free(options);
	free(returns);
	free(value);
	pcc->current_select->mptr->start = start;
	free((char *)pcc->current_select);
	pcc->current_select = NULL;
	pcc->ignore = 0;
	pcc->in_select = False;
}

/* We've just terminated the current OPTION.
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
                sptr->value[cnt - 1] =(char *)malloc(strlen(sptr->option_buf) +1);                strcpy(sptr->value[cnt - 1], sptr->option_buf);
                sptr->value_cnt = cnt;
        }       
}        

