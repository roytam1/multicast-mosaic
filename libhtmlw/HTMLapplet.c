/* HTMLapplet.c
 * Version 3.0.14 [Apr97]
 *
 * Copyright (C) 1997 - G.Dauphin
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

void _FreeAppletStruct(AppletInfo * ats)
{
	int i;

	if(ats->src)
		free(ats->src);
	for(i=0 ; i < ats->param_count; i++){
		free(ats->param_name_t[i]);
		free(ats->param_value_t[i]);
	} 
	free(ats->param_name_t);
	free(ats->param_value_t);
	for(i=0 ; i < ats->url_arg_count; i++){
		free(ats->url_arg[i]);
		if (ats->ret_filenames[i])
			free(ats->ret_filenames[i]);
	} 
	free(ats->url_arg);
	free(ats->ret_filenames);
	free(ats->internal_numeos);     
	if(ats->frame) {
		static Atom delete_atom = 0;
		static Atom proto_atom = 0;
		XClientMessageEvent ev;

		if (!delete_atom)
			delete_atom = XInternAtom(XtDisplay(ats->frame),
				"WM_DELETE_WINDOW", False);
		if(!proto_atom)
			proto_atom = XInternAtom(XtDisplay(ats->frame),
				"WM_PROTOCOLS", False);
		fprintf(stderr,"****mMosaic*** send WM_DELETE_WINDOW****\n");
		ev.type = ClientMessage;
		ev.window = XtWindow(ats->frame);
		ev.message_type = proto_atom;
		ev.format = 32;
		ev.data.l[0] = delete_atom;
		ev.data.l[1] = CurrentTime;
		XSendEvent (XtDisplay(ats->frame),
			XtWindow(ats->frame), True, 0x00ffffff, (XEvent *) &ev);
		XFlush(XtDisplay(ats->frame));
		XtSetMappedWhenManaged(ats->frame, False);
		XFlush(XtDisplay(ats->frame));
		XtDestroyWidget(ats->frame);
	}
	free(ats); 
}

void AppletPlace(HTMLWidget hw, struct mark_up ** mptr, PhotoComposeContext * pcc,
	Boolean save_obj)
{
	char * param_namePtr;
	char * param_valuePtr;
	struct mark_up * amptr = *mptr;
	struct mark_up * pmptr ;
	char  *srcPtr, *wPtr, *hPtr, *bwPtr, *alignPtr;
	CodeType codetype = CODE_TYPE_APPLET;
	int border_width;
	AlignType valignment;
	struct ele_rec * eptr;
	AppletInfo * ats=NULL;
	AppletInfo * saved_ats=amptr->s_ats;
	int extra = 0;
	int argcnt ;
	Arg arg[10];
	int baseline=0;

	fprintf(stderr, "AppletPlace: *x=%d,*y=%d,Width=%d)\n",
			pcc->x,pcc->y,pcc->width_of_viewable_part);

	/* code=applet.class est le nom du fichier qu'il faut rapatrier. */
	/* c'est du code java */
	/* URL_du_document + applet.class forme le nom absolu a prendre */
	srcPtr = ParseMarkTag(amptr->start, MT_APPLET, "code"); 
			/* code est l'url du fichier java */
			/* REQUIRED */
	if (srcPtr == NULL ){
		fprintf(stderr,"CODE is required in <APPLET>\n");
		return;
	}
	wPtr = ParseMarkTag(amptr->start, MT_APPLET, "WIDTH"); /* REQUIRED */
	hPtr = ParseMarkTag(amptr->start, MT_APPLET, "HEIGHT"); /* REQUIRED */
	if ((wPtr == NULL) || (hPtr == NULL)){
		fprintf(stderr,"WIDTH & HEIGHT required in <APPLET>\n");
		if (srcPtr) free(srcPtr);
		if (wPtr) free(wPtr);
		if (hPtr) free(hPtr);
		return;
	}
	bwPtr = ParseMarkTag(amptr->start, MT_APPLET, "BORDER"); /* IMPLIED */
	if (!bwPtr || !*bwPtr)
		border_width=IMAGE_DEFAULT_BORDER;
	else 
		if ((border_width=atoi(bwPtr))<0)
			border_width=0;
				/* Check if this image will be top aligned */
	if (bwPtr) free(bwPtr);
	alignPtr = ParseMarkTag(amptr->start, MT_APPLET, "ALIGN");
	if (caseless_equal(alignPtr, "TOP")) {
		valignment = VALIGN_TOP;
	} else if (caseless_equal(alignPtr, "MIDDLE")) {
		valignment = VALIGN_MIDDLE;
	} else {
		valignment = VALIGN_BOTTOM;
	}
	free(alignPtr);

	ats = (AppletInfo *) malloc(sizeof(AppletInfo));
	ats->height = atoi(hPtr) ;	/* in pixel */
	ats->width = atoi(wPtr) ;
	ats->frame = NULL;
	free(hPtr);
	free(wPtr);

/*#### on doit avance mptr pour trouver <PARAM name=nnn value="une valeur"> */
/* boucler tant qu'on a des <PARAM> puis boucler jusqu'a </APPLET> */
	pmptr = amptr->next;
	ats->param_count = 0;
	ats->param_name_t = (char **) malloc( sizeof(char *)); /* alloc one */
	ats->param_value_t = (char**) malloc( sizeof(char *));
	ats->param_name_t[ats->param_count] = NULL;
	ats->param_value_t[ats->param_count] = NULL;

	ats->url_arg_count = 0;
	ats->url_arg = (char **) malloc( sizeof(char *)); /* alloc one */
	ats->url_arg[ats->url_arg_count] = NULL;

#ifdef MULTICAST
	ats->wtype = hw->html.mc_wtype;
#endif
	ats->internal_numeos = (int*)malloc( sizeof(int)); /*alloc one */
	ats->ret_filenames = (char **) malloc( sizeof(char *)); /* alloc one */
	ats->internal_numeos[ats->url_arg_count] = pcc->internal_mc_eo;
	ats->ret_filenames[ats->url_arg_count] = NULL;


/* prepare le requete pour prendre le code java */
	ats->url_arg[ats->url_arg_count] = srcPtr;
	ats->url_arg_count++;
	ats->url_arg = (char**)realloc(ats->url_arg,
		(ats->url_arg_count +1) * sizeof(char *));
	ats->internal_numeos = (int*)realloc(ats->internal_numeos,
		(ats->url_arg_count +1) * sizeof(int ));
	ats->internal_numeos[ats->url_arg_count] = ats->internal_numeos[ats->url_arg_count - 1];
	ats->ret_filenames = (char**)realloc(ats->ret_filenames,
		(ats->url_arg_count +1) * sizeof(char *));
	ats->ret_filenames[ats->url_arg_count] = NULL;
	ats->url_arg[ats->url_arg_count] = NULL;

	ats->cw_only = pcc->cw_only;

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
		ats->param_name_t[ats->param_count] = param_namePtr;
		ats->param_value_t[ats->param_count] = param_valuePtr;
		ats->param_count++;
		ats->param_name_t = (char**)realloc(ats->param_name_t,
					(ats->param_count+1) * sizeof(char *));
		ats->param_value_t = (char**)realloc(ats->param_value_t,
					(ats->param_count+1) * sizeof(char *));
		ats->param_name_t[ats->param_count] = NULL;
		ats->param_value_t[ats->param_count] = NULL;

		pmptr = pmptr->next;
	}
/* pmptr pointe sur NULL ou le prochain element */
	while (pmptr && (pmptr->type != M_APPLET) && (!pmptr->is_end)) {
		/* ####derouler jusqu'a </APPLET>  */
		pmptr = pmptr->next;
	}
	if (!pmptr ){
		/*### la fin est obligatoire ### */
		fprintf(stderr,"[TriggerMarkChanges] Tag </APPLET> not seen\n");
		*mptr = pmptr;
		_FreeAppletStruct(ats);
		return;
	}

/*### mettre a jour mptr. Le mettre sur le tag </APPLET> */
	*mptr = pmptr;

	baseline = ats->height;

	if (!pcc->preformat) {	 /* if line too long , add LINEFEED  */
		if( (pcc->x + ats->width + extra) >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)){
			LinefeedPlace(hw, *mptr,pcc);
		}
	}
        if(pcc->computed_min_x < (ats->width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = ats->width+ pcc->eoffsetx+ pcc->left_margin;
        }
        if (pcc->x + ats->width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + ats->width;

	if (valignment == VALIGN_TOP) {
		baseline = 0;
	} else if (valignment == VALIGN_MIDDLE) {
		baseline = baseline / 2;
	}else {
		valignment = VALIGN_BOTTOM;
	}

/* mettre a jour l'element . 'ats' contient toutes les infos: parametres taille */
/* etc...  set some info in ats */
	ats->ctype = codetype;
	ats->src = srcPtr;
	ats->x = pcc->x;
	ats->y = pcc->y;
	ats->border_width = border_width;
	ats->valignment = valignment;
        if (!pcc->cw_only){            
		pcc->applet_id++;
                eptr = CreateElement(hw, E_APPLET, pcc->cur_font,
                                pcc->x, pcc->y,
				ats->width, ats->height, baseline, pcc);
                eptr->underline_number = 0; /* APPLET can't be underlined! */
                eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
                /* check the max line height. */
                AdjustBaseLine(hw,eptr,pcc); 
		eptr->ats = ats;
                eptr->bwidth=border_width ;  
		eptr->valignment = valignment;
		eptr->applet_id = pcc->applet_id;
        } else {
                if (pcc->cur_line_height < ats->height)
                        pcc->cur_line_height = ats->height;
        }


/* update pcc */                       
/* calcul du placement */
        pcc->have_space_after = 0;     
        pcc->x = pcc->x + ats->width ;      
        pcc->is_bol = False; 

	if (pcc->cw_only) { /* just compute size */
		_FreeAppletStruct(ats);
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
 *	XtSetArg(arg[argcnt], XmNx, ats->x); argcnt++;
 *	XtSetArg(arg[argcnt], XmNy, ats->y); argcnt++;
 *	XtSetArg(arg[argcnt], XmNshadowType, XmSHADOW_IN); argcnt++;
 *	ats->frame = XmCreateFrame(hw->html.view, "Frame",arg, argcnt);
 *	argcnt = 0;
 *	XtSetArg(arg[argcnt], XmNwidth, ats->width); argcnt++;
 *	XtSetArg(arg[argcnt], XmNheight, ats->height); argcnt++;
 *	ats->w = XmCreateDrawingArea(ats->frame, "Applet", arg, argcnt);
 *	XtManageChild(ats->w);
 *	XtSetArg(arg[argcnt], XmNuserData, (XtPointer)ats->w); argcnt++;
 *	XtSetValues(ats->frame, arg, argcnt);
 *	XtSetMappedWhenManaged(ats->frame, False);
 *	XtManageChild(ats->frame);
 *	XFlush(XtDisplay(hw));
*/
	if (save_obj == False){		/* it's a creation */
		int i;
		EODataStruct eo;
		char cmdline[15000];
		char allcmdline[16000];
		char zfile[1000];
		char * tmp;
		int get_cnt = 0;
		char * indot;

/* charger le code de l'applet */
		eo.src = ats->url_arg[0];
		tmp = eo.src;
		strcpy(zfile,eo.src);
		indot=strrchr(zfile,'.');
		if (indot && !strcmp(indot,".class") ){
			*indot = '.';
			indot[1] = 'z';
			indot[2] = 'i' ;
			indot[3] = 'p';
			indot[4] = '\0';
		}
		eo.src = zfile;
		eo.ret_filename = NULL;
		eo.num_eo = pcc->internal_mc_eo;
#ifdef MULTICAST
		eo.wtype = hw->html.mc_wtype;
#endif
		eo.cw_only = pcc->cw_only;
		strcpy(cmdline," ");
		if (hw->html.get_url_data_cb){
			strcat(cmdline," ");
			XtCallCallbackList((Widget) hw, hw->html.get_url_data_cb,
				(XtPointer) &eo);
			pcc->internal_mc_eo++;
			if(eo.ret_filename!=NULL){
				sprintf(allcmdline,"mv %s $HOME/.mMosaic/classes/%s",eo.ret_filename,
					eo.src);
				system(allcmdline);
				eo.src = tmp;
/*				strcat(cmdline, eo.ret_filename); */
				indot=strrchr(eo.src,'.');
				if (indot && !strcmp(indot,".class") ){
					*indot = '\0';
				}
				strcat(cmdline, eo.src);
/* l'applet est dans le fichier eo.ret_filename */
				get_cnt++;
			}
		}

/* les parametres */
		for(i=0; ats->param_name_t[i] != NULL; i++){
			strcat(cmdline," ");
			strcat(cmdline, ats->param_name_t[i]);
			if (ats->param_value_t[i]){
				strcat(cmdline," ");
				strcat(cmdline,ats->param_value_t[i]);
			}
		}

		if (get_cnt == ats->url_arg_count){
/* all data id here . Create */
			XWindowAttributes xwa;
			XSetWindowAttributes xswa;

			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, ats->x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, ats->y); argcnt++;
			XtSetArg(arg[argcnt], XmNwidth, ats->width); argcnt++;
			XtSetArg(arg[argcnt], XmNheight, ats->height); argcnt++;
			ats->frame = XmCreateLabel(hw->html.view, 
				"If this text appear then APPLET is not running", arg, argcnt);
			XtSetMappedWhenManaged(ats->frame, False);
			XtManageChild(ats->frame);
			XFlush(XtDisplay(hw));
			XGetWindowAttributes(XtDisplay(ats->frame),
				XtWindow(ats->frame),&xwa);
/*			printf("xwa.do_not_propagate_mask = %x\n",
				xwa.do_not_propagate_mask);
*/
			xswa.do_not_propagate_mask = xwa.do_not_propagate_mask & (~(PointerMotionMask));
			XChangeWindowAttributes(XtDisplay(ats->frame),
				XtWindow(ats->frame),CWDontPropagate, &xswa);
			XFlush(XtDisplay(hw));
/*
			XGetWindowAttributes(XtDisplay(ats->frame),
				XtWindow(ats->frame),&xwa);
			printf("xwa.do_not_propagate_mask = %x\n",
				xwa.do_not_propagate_mask);

			printf("Applet create Window = %d\n",
					XtWindow(ats->frame));
*/
			sprintf(allcmdline,"/usr/local/mMosaic/bin/mMosaicAppletViewer -windowId %d %s &",
				XtWindow(ats->frame),cmdline );

			printf("Executing : %s\n",allcmdline);
			system(allcmdline);
			amptr->s_ats = ats ;
		}
		eptr->ats = ats;
	} else {		/* use the old window. It's a Resize */
		if (saved_ats == NULL){
			fprintf(stderr,"Abug in APPLET when resizing\n");
		} else {
			eptr->ats = saved_ats;
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, ats->x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, ats->y); argcnt++;
			XtSetArg(arg[argcnt], XmNwidth, ats->width); argcnt++;
			XtSetArg(arg[argcnt], XmNheight, ats->height); argcnt++;
			XtSetValues(saved_ats->frame, arg, argcnt);
			XtSetMappedWhenManaged(saved_ats->frame, False);
			XtManageChild(saved_ats->frame);
			XFlush(XtDisplay(hw));
			_FreeAppletStruct(ats);	
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

void AppletRefresh(HTMLWidget hw, struct ele_rec *eptr)
{
	int x;
	int y;
	Position px;
	Position py;
	Arg args[3];

/* fprintf(stderr,"[PlaceLine] need E_APPLET tool\n"); */
/*#### faire un unmap/map de la widget creer dans 'case M_APPLET' ###*/
/* voir ImageRefresh comme exemple de traitement pour placer la Widget */
/* attention au placement x et y qui doivent etre des shorts */
/* x,y dans eptr sont des entiers et des short dans X-Window */
/* faire le calcul avec les scrool-bar pour que ca 'tombe' bien */
/*	x = x - hw->html.scroll_x; */
/*	y = y - hw->html.scroll_y; */

	if(eptr->ats == NULL ) return;
	if(eptr->ats->frame == NULL ) return;
	x = eptr->x;
	y = eptr->y;
	px = x - hw->html.scroll_x;
	py = y - hw->html.scroll_y;
	XtSetArg(args[0], XtNx, px);
	XtSetArg(args[1], XtNy, py);
	XtSetValues(eptr->ats->frame, args,2);
	XtSetMappedWhenManaged(eptr->ats->frame, True);
}
