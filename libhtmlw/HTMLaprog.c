/* HTMLaprog.c
 * Version 3.0 [Sep96]
 *
 * Copyright (C) 1996 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 */

#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <Xm/XmAll.h>

void AprogPlace(HTMLWidget hw, struct mark_up ** mptr, PhotoComposeContext * pcc,
	Boolean save_obj)
{
	char * param_namePtr;
	char * param_valuePtr;
	struct mark_up * amptr = *mptr;
	struct mark_up * pmptr ;
	char * codetypePtr, *srcPtr, *wPtr, *hPtr, *bwPtr, *alignPtr;
	char * namePtr;
	CodeType codetype = CODE_TYPE_UNKNOW;
	int border_width;
	ValignType valignment;
	struct ele_rec * eptr;
	AprogInfo * aps=NULL;
	AprogInfo * saved_aps=amptr->saved_aps;
	int extra = 0;
	int argcnt ;
	Arg arg[10];
	int baseline=0;

	fprintf(stderr, "AprogPlace: *x=%d,*y=%d,Width=%d)\n",
			pcc->x,pcc->y,pcc->width_of_viewable_part);

	codetypePtr = ParseMarkTag(amptr->start, MT_APROG, "CODETYPE");
	if (caseless_equal(codetypePtr, "BINARY"))/*si c'est du binaire on doit*/
				/* l'avoir sur place : c'est un plugins */
		codetype = CODE_TYPE_BIN;
	if (caseless_equal(codetypePtr, "SOURCE")) /* si c'est du source faut */
				/* le rapatrie, */
				/* le compiler et le mettre en plugins */
		codetype = CODE_TYPE_SRC;
	if(codetype == CODE_TYPE_UNKNOW){
		fprintf(stderr,"Unknow code type in <APROG>\n");
		return;
	}
	srcPtr = ParseMarkTag(amptr->start, MT_APROG, "SRC"); /* src est l'url */
				/* du source ou le nom du plugging( du binaire) */
				/* REQUIRED */
	if (srcPtr == NULL && codetype == CODE_TYPE_SRC){
		fprintf(stderr,"SRC is required in <APROG>\n");
		return;
	}
	wPtr = ParseMarkTag(amptr->start, MT_APROG, "WIDTH"); /* REQUIRED */
	hPtr = ParseMarkTag(amptr->start, MT_APROG, "HEIGHT"); /* REQUIRED */
	if ((wPtr == NULL) || (hPtr == NULL)){
		fprintf(stderr,"WIDTH & HEIGHT required in <APROG>\n");
		return;
	}
	bwPtr = ParseMarkTag(amptr->start, MT_APROG, "BORDER"); /* IMPLIED */
	if (!bwPtr || !*bwPtr)
		border_width=IMAGE_DEFAULT_BORDER;
	else 
		if ((border_width=atoi(bwPtr))<0)
			border_width=0;
				/* in case we have no source or bin get name */
	namePtr = ParseMarkTag(amptr->start, MT_APROG, "NAME");
	if (namePtr == NULL && codetype == CODE_TYPE_BIN){
		fprintf(stderr,"NAME is required in <APROG>\n");
		return;
	}
	if (strchr(namePtr,'/') ){
		fprintf(stderr,"NAME not secure in <APROG>\n");
		return;
	}
				/* Check if this image will be top aligned */
	alignPtr = ParseMarkTag(amptr->start, MT_APROG, "ALIGN");
	if (caseless_equal(alignPtr, "TOP")) {
		valignment = ALIGN_TOP;
	} else if (caseless_equal(alignPtr, "MIDDLE")) {
		valignment = ALIGN_MIDDLE;
	} else {
		valignment = ALIGN_BOTTOM;
	}

	aps = (AprogInfo *) malloc(sizeof(AprogInfo));

/*#### on doit avance mptr pour trouver <PARAM name=nnn value="une valeur"> */
/* boucler tant qu'on a des <PARAM> puis boucler jusqu'a </APROG> */
	pmptr = amptr->next;
	aps->param_count = 0;
	aps->param_name_t = (char **) malloc( sizeof(char *)); /* alloc one */
	aps->param_value_t = (char**) malloc( sizeof(char *));
	aps->param_name_t[aps->param_count] = NULL;
	aps->param_value_t[aps->param_count] = NULL;

	aps->url_arg_count = 0;
	aps->url_arg = (char **) malloc( sizeof(char *)); /* alloc one */
	aps->url_arg[aps->url_arg_count] = NULL;

	aps->wtype = hw->html.mc_wtype;
	aps->internal_numeos = (int*)malloc( sizeof(int)); /*alloc one */
	aps->ret_filenames = (char **) malloc( sizeof(char *)); /* alloc one */
	aps->internal_numeos[aps->url_arg_count] = pcc->internal_mc_eo;
	aps->ret_filenames[aps->url_arg_count] = NULL;

	aps->cw_only = pcc->cw_only;

	while (pmptr && ((pmptr->type == M_PARAM) || (pmptr->type == M_NONE))){
		if (pmptr->type == M_NONE){ 	/* on saute le texte */
			pmptr = pmptr->next;
			continue;
		}
			/*# derouler & sauver les PARAM */
			/*####<PARAM NAME=param_name VALUE=param_value> */
		param_namePtr = ParseMarkTag(pmptr->start,MT_PARAM,"NAME");
		param_valuePtr = ParseMarkTag(pmptr->start,MT_PARAM,"VALUE");
		if ( !param_namePtr)
			continue;
		if( ! strcmp(param_namePtr,"_URL_TYPED_ARG") ){
			aps->url_arg[aps->url_arg_count] = param_valuePtr;
			aps->url_arg_count++;
			aps->url_arg = (char**)realloc(aps->url_arg,
				(aps->url_arg_count +1) * sizeof(char *));
			aps->internal_numeos = (int*)realloc(aps->internal_numeos,
				(aps->url_arg_count +1) * sizeof(int ));
			aps->internal_numeos[aps->url_arg_count] = aps->internal_numeos[aps->url_arg_count - 1];
			aps->ret_filenames = (char**)realloc(aps->ret_filenames,
				(aps->url_arg_count +1) * sizeof(char *));
			aps->ret_filenames[aps->url_arg_count] = NULL;
			aps->url_arg[aps->url_arg_count] = NULL;
			pmptr = pmptr->next;
			continue;
		}
		aps->param_name_t[aps->param_count] = param_namePtr;
		aps->param_value_t[aps->param_count] = param_valuePtr;
		aps->param_count++;
		aps->param_name_t = (char**)realloc(aps->param_name_t,
					(aps->param_count+1) * sizeof(char *));
		aps->param_value_t = (char**)realloc(aps->param_value_t,
					(aps->param_count+1) * sizeof(char *));
		aps->param_name_t[aps->param_count] = NULL;
		aps->param_value_t[aps->param_count] = NULL;

		pmptr = pmptr->next;
	}
/* pmptr pointe sur NULL ou le prochain element */
	while (pmptr && (pmptr->type != M_APROG) && (!pmptr->is_end)) {
		/* ####derouler jusqu'a </APROG>  */
		pmptr = pmptr->next;
	}
	if (!pmptr ){
		/*### la fin est obligatoire ### */
		fprintf(stderr,"[TriggerMarkChanges] Tag </APROG> not seen\n");
		*mptr = pmptr;
		return;
	}

/*### mettre a jour mptr. Le mettre sur le tag </APROG> */
	*mptr = pmptr;

	aps->height = (atoi(hPtr) * pcc->width_of_viewable_part) / 100;
	aps->width = (atoi(wPtr) * pcc->width_of_viewable_part) / 100;
	baseline = aps->height;

	if (!pcc->preformat) {	 /* if line too long , add LINEFEED  */
		if( (pcc->x + aps->width + extra) >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)){
			LinefeedPlace(hw, *mptr,pcc);
		}
	}
        if(pcc->computed_min_x < (aps->width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = aps->width+ pcc->eoffsetx+ pcc->left_margin;
        }
        if (pcc->x + aps->width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + aps->width;

	if (valignment == ALIGN_TOP) {
		baseline = 0;
	} else if (valignment == ALIGN_MIDDLE) {
		baseline = baseline / 2;
	}else {
		valignment = ALIGN_BOTTOM;
	}

/* mettre a jour l'element . 'aps' contient toutes les infos: parametres taille */
/* etc...  set some info in aps */
	aps->ctype = codetype;
	aps->src = srcPtr;
	aps->name = namePtr;
	aps->x = pcc->x;
	aps->y = pcc->y;
	aps->border_width = border_width;
	aps->valignment = valignment;
	aps->frame = NULL;
        if (!pcc->cw_only){            
		pcc->aprog_id++;
                eptr = CreateElement(hw, E_APROG, pcc->cur_font,
                                pcc->x, pcc->y,
				aps->width, aps->height, baseline, pcc);
                eptr->underline_number = 0; /* APROG can't be underlined! */
                eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
                /* check the max line height. */
                AdjustBaseLine(hw,eptr,pcc); 
		eptr->aprog_struct = aps;
                eptr->bwidth=border_width ;  
		eptr->valignment = valignment;
		eptr->aprog_id = pcc->aprog_id;
        } else {
                if (pcc->cur_line_height < aps->height)
                        pcc->cur_line_height = aps->height;
        }


/* update pcc */                       
/* calcul du placement */
        pcc->have_space_after = 0;     
        pcc->x = pcc->x + aps->width ;      
        pcc->is_bol = False; 

	if (pcc->cw_only) { /* just compute size */
		return;
	}

/*##################*/
/* active callback to get the source. Il n'y a pas de callback pour du binaire*/
/* le callback retourne le chemin du binaire compile */
/* Y a pu qu'a plugguer */
/* pour plugguer il faut creer une widget container avec le 'bon' placement */
/* et la 'bonne' taille qui doit rester */
/* UnMap-er jusqu'au 1er refresh */
/* il faut forker et activer le programme , en memorisant son 'pid' */
/* passer en parametre la XtWindow de la widget ainsi creee */

/* Creation d'une widget */
/*	argcnt = 0;
 *	XtSetArg(arg[argcnt], XmNx, aps->x); argcnt++;
 *	XtSetArg(arg[argcnt], XmNy, aps->y); argcnt++;
 *	XtSetArg(arg[argcnt], XmNshadowType, XmSHADOW_IN); argcnt++;
 *	aps->frame = XmCreateFrame(hw->html.view, "Frame",arg, argcnt);
 *	argcnt = 0;
 *	XtSetArg(arg[argcnt], XmNwidth, aps->width); argcnt++;
 *	XtSetArg(arg[argcnt], XmNheight, aps->height); argcnt++;
 *	aps->w = XmCreateDrawingArea(aps->frame, "Aprog", arg, argcnt);
 *	XtManageChild(aps->w);
 *	XtSetArg(arg[argcnt], XmNuserData, (XtPointer)aps->w); argcnt++;
 *	XtSetValues(aps->frame, arg, argcnt);
 *	XtSetMappedWhenManaged(aps->frame, False);
 *	XtManageChild(aps->frame);
 *	XFlush(XtDisplay(hw));
*/
	if (save_obj == False){		/* it's a creation */
		int i;

		if (aps->ctype == CODE_TYPE_BIN) {
			char cmdline[15000];
			char allcmdline[16000];
			int get_cnt = 0;

			strcpy(cmdline," ");
			for(i=0; aps->param_name_t[i] != NULL; i++){
				strcat(cmdline," ");
				strcat(cmdline, aps->param_name_t[i]);
				if (aps->param_value_t[i]){
					strcat(cmdline," ");
					strcat(cmdline,aps->param_value_t[i]);
				}
			}
			for(i=0; aps->url_arg[i] != NULL; i++){
				EODataStruct eo;

				eo.src = aps->url_arg[i];
				eo.ret_filename = NULL;
				eo.num_eo = pcc->internal_mc_eo;
				eo.wtype = hw->html.mc_wtype;
				eo.cw_only = pcc->cw_only;
				if (hw->html.get_url_data_cb){
					strcat(cmdline," ");
					XtCallCallbackList((Widget) hw,
						hw->html.get_url_data_cb,
						(XtPointer) &eo);
					pcc->internal_mc_eo++;
					if(eo.ret_filename!=NULL){
						strcat(cmdline, eo.ret_filename);
						get_cnt++;
					}
				}
			}
			if (get_cnt == aps->url_arg_count){
/* all data id here . Create */
				XWindowAttributes xwa;
				XSetWindowAttributes xswa;

				argcnt = 0;
				XtSetArg(arg[argcnt], XmNx, aps->x); argcnt++;
				XtSetArg(arg[argcnt], XmNy, aps->y); argcnt++;
				XtSetArg(arg[argcnt], XmNwidth, aps->width); argcnt++;
				XtSetArg(arg[argcnt], XmNheight, aps->height); argcnt++;
				aps->frame = XmCreateLabel(hw->html.view, 
					"If this text appear then APROG is not running", arg, argcnt);
				XtSetMappedWhenManaged(aps->frame, False);
				XtManageChild(aps->frame);
				XFlush(XtDisplay(hw));
				XGetWindowAttributes(XtDisplay(aps->frame),
					XtWindow(aps->frame),&xwa);
/*				printf("xwa.do_not_propagate_mask = %x\n",
					xwa.do_not_propagate_mask);
*/
				xswa.do_not_propagate_mask = xwa.do_not_propagate_mask & (~(PointerMotionMask));
				XChangeWindowAttributes(XtDisplay(aps->frame),
					XtWindow(aps->frame),CWDontPropagate,
					&xswa);
				XFlush(XtDisplay(hw));
/*
				XGetWindowAttributes(XtDisplay(aps->frame),
					XtWindow(aps->frame),&xwa);
				printf("xwa.do_not_propagate_mask = %x\n",
					xwa.do_not_propagate_mask);

				printf("Aprog create Window = %d\n",
						XtWindow(aps->frame));
*/
				sprintf(allcmdline,"/usr/local/mMosaic/bin/%s -windowId %d %s &",
					namePtr, XtWindow(aps->frame),cmdline );

/*
				printf("Executing : %s\n",allcmdline);
*/
				system(allcmdline);
				amptr->saved_aps = aps ;
			}
		}
	} else {		/* use the old window. It's a Resize */
		if (saved_aps == NULL){
			fprintf(stderr,"Abug in APROG when resizing\n");
		} else {
			aps->frame = saved_aps->frame;
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, aps->x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, aps->y); argcnt++;
			XtSetArg(arg[argcnt], XmNwidth, aps->width); argcnt++;
			XtSetArg(arg[argcnt], XmNheight, aps->height); argcnt++;
			XtSetValues(aps->frame, arg, argcnt);
			XtSetMappedWhenManaged(aps->frame, False);
			XtManageChild(aps->frame);
			XFlush(XtDisplay(hw));
		}
	}

	if (srcPtr)
		free(srcPtr);
	if (wPtr)
		free(wPtr);
	if (hPtr)
		free(hPtr);
	if (bwPtr)
		free(bwPtr);
}

void AprogRefresh(HTMLWidget hw, struct ele_rec *eptr)
{
	int x;
	int y;
	Position px;
	Position py;
	Arg args[3];

/* fprintf(stderr,"[PlaceLine] need E_APROG tool\n"); */
/*#### faire un unmap/map de la widget creer dans 'case M_APROG' ###*/
/* voir ImageRefresh comme exemple de traitement pour placer la Widget */
/* attention au placement x et y qui doivent etre des shorts */
/* x,y dans eptr sont des entiers et des short dans X-Window */
/* faire le calcul avec les scrool-bar pour que ca 'tombe' bien */
/*	x = x - hw->html.scroll_x; */
/*	y = y - hw->html.scroll_y; */

	if(eptr->aprog_struct->frame == NULL ) return;
	x = eptr->x;
	y = eptr->y;
	px= x = x - hw->html.scroll_x;
	py= y = y - hw->html.scroll_y;
	XtSetArg(args[0], XtNx, px);
	XtSetArg(args[1], XtNy, py);
	XtSetValues(eptr->aprog_struct->frame, args,2);
	XtSetMappedWhenManaged(eptr->aprog_struct->frame, True);
}
