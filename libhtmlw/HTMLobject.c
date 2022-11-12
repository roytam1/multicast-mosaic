/* HTMLobject.c
 * Version 3.5.4 [Mai 2000]
 *
 * Copyright (C) 1996-2000 - G.Dauphin
 * See the file "GPL" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 */

#ifdef OBJECT
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <assert.h>
#include <signal.h>

#include "HTMLP.h"
#include "HTMLPutil.h"
#include <Xm/XmAll.h>

#include "../src/URLParse.h"

void _FreeObjectStruct(HtmlObjectStruct * obs)
{
	int i;

	assert(0);
/*	if(aps->src)
/*		free(aps->src);
/*	if(aps->name)
/*		free(aps->name);
/*	for(i=0 ; i < aps->param_count; i++){
/*		free(aps->param_name_t[i]);
/*		free(aps->param_value_t[i]);
/*	}
/*	free(aps->param_name_t);
/*	free(aps->param_value_t);
/*	for(i=0 ; i < aps->url_arg_count; i++){
/*		free(aps->url_arg[i]);
/*		if (aps->ret_filenames[i])
/*			free(aps->ret_filenames[i]);
/*	}
/*	free(aps->url_arg);
/*	free(aps->ret_filenames);
/*	free(aps->internal_numeos);
/*
/*	if(aps->frame){
/*		static Atom delete_atom = 0;
/*		static Atom proto_atom = 0;
/*		XClientMessageEvent ev;
/*
/*		if (!delete_atom)
/*			delete_atom = XInternAtom(XtDisplay(aps->frame),
/*				"WM_DELETE_WINDOW", False);
/*		if(!proto_atom)
/*			proto_atom = XInternAtom(XtDisplay(aps->frame),
/*				"WM_PROTOCOLS", False);
/*		ev.type = ClientMessage;
/*		ev.window = XtWindow(aps->frame); 
/*		ev.message_type = proto_atom;
/*		ev.format = 32;
/*		ev.data.l[0] = delete_atom;   
/*		ev.data.l[1] = CurrentTime;
/*		XSendEvent (XtDisplay(aps->frame), XtWindow(aps->frame),
/*				True, 0x00ffffff, (XEvent *) &ev);
/*
/*		XFlush(XtDisplay(aps->frame));
/*		XtSetMappedWhenManaged(aps->frame, False);
/*		XFlush(XtDisplay(aps->frame));
/*		XtDestroyWidget(aps->frame);
/*	}
/*	free(aps);
*/
}

#define MMOSAIC_PLUGIN_DIR "/usr/local/mMosaic/plugins/"

void ObjectPlace(HTMLWidget hw, struct mark_up ** mptr, PhotoComposeContext * pcc,
	Boolean save_obj)
{
	char * classidPtr;
	char * content_typePtr;
	char * codebasePtr;
	char * dataPtr;
	char * hPtr;
	char * wPtr;
	char * bwPtr;
	char * alignPtr;
	int border_width;
	struct mark_up * omptr = *mptr;
	char * param_namePtr;
	char * param_valuePtr;
	char * param_valuetypePtr;
	AlignType valignment;
	HtmlObjectStruct * saved_obs=omptr->s_obs;

	struct mark_up * pmptr ;
	struct ele_rec * eptr;
	HtmlObjectStruct * obs=NULL;
	int extra = 0;
	int argcnt ;
	Arg arg[10];
	int baseline=0;

#ifdef DEBUG_OBJECT
	fprintf(stderr, "ObjectPlace: *x=%d,*y=%d,Width=%d)\n",
			pcc->x,pcc->y,pcc->width_of_viewable_part);
#endif
/* codetype: unused */
/* archive: unused */
/* declare: unused */
/* standby: unused */

/* classid="mtvp" */
	classidPtr = ParseMarkTag(omptr->start, MT_OBJECT, "classid");

	if (classidPtr && strchr(classidPtr,'/') ){
		fprintf(stderr,"Object not secure \n");
		return;
	}
	if (classidPtr) {	/* get absolute path name */
		char * buf = (char*)malloc(strlen(MMOSAIC_PLUGIN_DIR) +
					   strlen(classidPtr) +20);

		strcpy(buf, MMOSAIC_PLUGIN_DIR);
		strcat(buf,classidPtr);
		free(classidPtr);
		classidPtr = buf;
	}
/* type="video/mpeg" */
	content_typePtr = ParseMarkTag(omptr->start, MT_OBJECT, "type");

/* codebase="~dauphin/pluggin/mtvp/how-to-plug-mtvp */
	codebasePtr = ParseMarkTag(omptr->start, MT_OBJECT, "codebase");

/* data="fichier_video.mpg" */
/* l'url est relative % a la page HTML (entorse a la regle codebase) */
	dataPtr = ParseMarkTag(omptr->start, MT_OBJECT, "data");
	dataPtr = URLParse(dataPtr, hw->html.base_url, 
		PARSE_ACCESS | PARSE_HOST | PARSE_PATH |PARSE_PUNCTUATION);
/* comment arrive les donnees? 
 *   - en mode stream avec traitement a la volee.?
 *   - en mode fichier avec telechargement?
 * On choisit le mode stream pour plugger, mais on passe l'url a
 * l'application...!!! C'est a l'appli de se demerder. Comme d'hab.!
 */
	wPtr = ParseMarkTag(omptr->start, MT_OBJECT, "width");
	hPtr = ParseMarkTag(omptr->start, MT_OBJECT, "height");

/* if w or h is not here, that's a embed object with no window */
	if (!wPtr && hPtr)
		wPtr = strdup(hPtr);	/* set square */
	if (!hPtr && wPtr)
		hPtr = strdup(wPtr);	/* set square */
	
	bwPtr = ParseMarkTag(omptr->start, MT_OBJECT, "border");
	if (!bwPtr || !*bwPtr)
		border_width=0;
	else 
		if ((border_width=atoi(bwPtr))<0)
			border_width=0;

	valignment = VALIGN_BOTTOM;
	alignPtr = ParseMarkTag(omptr->start, MT_OBJECT, "ALIGN");
	if (alignPtr) {
		if (!strcasecmp(alignPtr, "TOP")) {
			valignment = VALIGN_TOP;
		} else if (!strcasecmp(alignPtr, "MIDDLE")) {
			valignment = VALIGN_MIDDLE;
		} else {
			valignment = VALIGN_BOTTOM;
		}
		free(alignPtr);
	}

	if(!classidPtr){
		/* find a pluggin with content_type */
		/* if not found */
			/* find a pluggin with type of data */
			/* if not found */
				/* ask for loading the pluggin */
				/*fprintf(stderr,"SRC is required in <APROG>\n");*/
		assert(0);
		return;		/* what to call ? */
	}

	assert(classidPtr);
	if (access(classidPtr, X_OK) != 0){	 /* test exec */
		fprintf(stderr,"No permission to exec %s\n", classidPtr);
		return;
	}

	obs = (_HtmlObjectStruct *) calloc(1,sizeof(_HtmlObjectStruct));
	obs->bin_path = classidPtr;
	obs->height = atoi(hPtr);
	obs->width = atoi(wPtr);
	obs->frame = NULL;
	free(hPtr);
	free(wPtr);

/* on doit avance mptr pour trouver <PARAM name=nnn value="une valeur"> */
/* boucler tant qu'on a des <PARAM> puis boucler jusqu'a </APROG> */
	pmptr = omptr->next;
	obs->param_count = 0;
	obs->param_name_t = (char **) malloc( sizeof(char *)); /* alloc one */
	obs->param_value_t = (char**) malloc( sizeof(char *));
	obs->param_valuetype_t = (char**) malloc( sizeof(char *));
	obs->param_name_t[obs->param_count] = NULL;
	obs->param_value_t[obs->param_count] = NULL;
	obs->param_valuetype_t[obs->param_count] = NULL;

	obs->url_arg_count = 0;
	obs->url_arg = (char **) malloc( sizeof(char *)); /* alloc one */
	obs->url_arg[obs->url_arg_count] = NULL;

/*	obs->internal_numeos = (int*)calloc(1, sizeof(int)); /*alloc one */
/*	obs->ret_filenames = (char **) malloc( sizeof(char *)); /* alloc one */
/*	obs->ret_filenames[obs->url_arg_count] = NULL;
*/

	obs->cw_only = pcc->cw_only;

	while (pmptr && ((pmptr->type == M_PARAM) || (pmptr->type == M_NONE))){
		if (pmptr->type == M_NONE){ 	/* on saute le texte */
			pmptr = pmptr->next;
			continue;
		}
			/* derouler & sauver les PARAM */
			/*<PARAM NAME=param_name VALUE=param_value> */
		param_namePtr = ParseMarkTag(pmptr->start,MT_PARAM,"NAME");
		param_valuePtr = ParseMarkTag(pmptr->start,MT_PARAM,"VALUE");
		param_valuetypePtr = ParseMarkTag(pmptr->start,MT_PARAM,"valuetype");
		if ( !param_namePtr)
			continue;
/*		if( ! strcmp(param_namePtr,"_URL_TYPED_ARG") ){ */
/*		if( ! strcmp(param_valuetypePtr,"ref")) ){ */
/*			/* find how to load it stream or file ... */
/*			obs->url_arg[obs->url_arg_count] = param_valuePtr;
/*			obs->url_arg_count++;
/*			obs->url_arg = (char**)realloc(obs->url_arg,
/*				(obs->url_arg_count +1) * sizeof(char *));
/*			obs->internal_numeos = (int*)realloc(obs->internal_numeos,
/*				(obs->url_arg_count +1) * sizeof(int ));
/*			obs->internal_numeos[obs->url_arg_count] = obs->internal_numeos[obs->url_arg_count - 1];
/*			obs->ret_filenames = (char**)realloc(obs->ret_filenames,
/*				(obs->url_arg_count +1) * sizeof(char *));
/*			obs->ret_filenames[obs->url_arg_count] = NULL;
/*			obs->url_arg[obs->url_arg_count] = NULL;
/*			pmptr = pmptr->next;
/*			continue;
/*		}
*/
		obs->param_name_t[obs->param_count] = param_namePtr;
		obs->param_value_t[obs->param_count] = param_valuePtr;
		obs->param_count++;
		obs->param_name_t = (char**)realloc(obs->param_name_t,
					(obs->param_count+1) * sizeof(char *));
		obs->param_value_t = (char**)realloc(obs->param_value_t,
					(obs->param_count+1) * sizeof(char *));
		obs->param_name_t[obs->param_count] = NULL;
		obs->param_value_t[obs->param_count] = NULL;

		pmptr = pmptr->next;
	}
/* pmptr pointe sur NULL ou le prochain element */
	while (pmptr && (pmptr->type != M_OBJECT) && (!pmptr->is_end)) {
		/* derouler jusqu'a </OBJECT>  */
		pmptr = pmptr->next;
	}
	if (!pmptr ){		/* la fin est obligatoire */
		fprintf(stderr,"[ObjectPlace] Mark </OBJECT> not seen\n");
		*mptr = pmptr;
		_FreeObjectStruct(obs);
		return;
	}

/* mettre a jour mptr. Le mettre sur le tag </APROG> */
	*mptr = pmptr;

	baseline = obs->height;

	if (!pcc->preformat) {	 /* if line too long , add LINEFEED  */
		if( (pcc->x + obs->width + extra) >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)){
			LinefeedPlace(hw, *mptr,pcc);
		}
	}
        if(pcc->computed_min_x < (obs->width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = obs->width+ pcc->eoffsetx+ pcc->left_margin;
        }
        if (pcc->x + obs->width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + obs->width;

	if (valignment == VALIGN_TOP) {
		baseline = 0;
	} else if (valignment == VALIGN_MIDDLE) {
		baseline = baseline / 2;
	}else {
		valignment = VALIGN_BOTTOM;
	}

/* mettre a jour l'element . 'obs' contient toutes les infos: parametres taille */
/* etc...  set some info in obs */
	obs->x = pcc->x;
	obs->y = pcc->y;
	obs->border_width = border_width;
	obs->valignment = valignment;
        if (!pcc->cw_only){            
		pcc->aprog_id++;
                eptr = CreateElement(hw, E_OBJECT, pcc->cur_font,
                                pcc->x, pcc->y,
				obs->width, obs->height, baseline, pcc);
                eptr->underline_number = 0; /* APROG can't be underlined! */
                eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
                /* check the max line height. */
                AdjustBaseLine(hw,eptr,pcc); 
                eptr->bwidth=border_width ;  
		eptr->valignment = valignment;
		eptr->aprog_id = pcc->aprog_id;
        } else {
                if (pcc->cur_line_height < obs->height)
                        pcc->cur_line_height = obs->height;
        }


/* update pcc */                       
/* calcul du placement */
        pcc->have_space_after = 0;     
        pcc->x = pcc->x + obs->width ;      
        pcc->is_bol = False; 

	if (pcc->cw_only) { /* just compute size */
		_FreeObjectStruct(obs);
		return;
	}

/*################## verifier si tout est bien liberer###### */
/* active callback to get the source. Il n'y a pas de callback pour du binaire*/
/* le callback retourne le chemin du binaire compile */
/* Y a pu qu'a plugguer */
/* pour plugguer il faut creer une widget container avec le 'bon' placement */
/* et la 'bonne' taille qui doit rester */
/* UnMap-er jusqu'au 1er refresh */
/* il faut forker et activer le programme , en memorisant son 'pid' */
/* passer en parametre la XtWindow de la widget ainsi creee */

/* Creation d'une widget */
	if (save_obj == False){		/* it's a creation */
		XWindowAttributes xwa;
		XSetWindowAttributes xswa;
		int i;
		Widget frame;

		char cmdline[15000];
		char allcmdline[16000];
		int get_cnt = 0;

		strcpy(cmdline," ");
		for(i=0; obs->param_name_t[i] != NULL; i++){
			strcat(cmdline," ");
			strcat(cmdline, obs->param_name_t[i]);
			if (obs->param_value_t[i]){
				strcat(cmdline," ");
				strcat(cmdline,obs->param_value_t[i]);
			}
		}
/* at last cat the url */
		strcat(cmdline, " ");
		strcat(cmdline, dataPtr);

/*		for(i=0; obs->url_arg[i] != NULL; i++){
/*			EODataStruct eo;
/*
/*			eo.src = obs->url_arg[i];
/*			eo.ret_filename = NULL;
/*			eo.cw_only = pcc->cw_only;
/*			strcat(cmdline," ");
/* bug ###### */
/*			GetUrlData((Widget)hw, NULL, (XtPointer)&eo);
/*			if(eo.ret_filename!=NULL){
/*				strcat(cmdline, eo.ret_filename);
/*				get_cnt++;
/*				obs->ret_filenames[i] = 
/*					eo.ret_filename;
/*			}
/*		}
*/
/* if (get_cnt == obs->url_arg_count){ */
/* all data id here . Create */

		argcnt = 0;
		XtSetArg(arg[argcnt], XmNx, obs->x); argcnt++;
		XtSetArg(arg[argcnt], XmNy, obs->y); argcnt++;
		XtSetArg(arg[argcnt], XmNwidth, obs->width); argcnt++;
		XtSetArg(arg[argcnt], XmNheight, obs->height); argcnt++;
		frame = XmCreateLabel(hw->html.view, 
			"If this text appear then OBJECT is not running", arg, argcnt);
		obs->frame = frame;
		XtSetMappedWhenManaged(frame, False);
		XtManageChild(frame);
		XFlush(XtDisplay(hw));
		XGetWindowAttributes(XtDisplay(frame),
			XtWindow(frame),&xwa);
/*		printf("xwa.do_not_propagate_mask = %x\n", xwa.do_not_propagate_mask);
*/
		xswa.do_not_propagate_mask = xwa.do_not_propagate_mask & (~(PointerMotionMask));
		XChangeWindowAttributes(XtDisplay(frame),
			XtWindow(frame),CWDontPropagate, &xswa);
		XFlush(XtDisplay(hw));
/*
/*		XGetWindowAttributes(XtDisplay(frame), XtWindow(frame),&xwa);
/*		printf("xwa.do_not_propagate_mask = %x\n", xwa.do_not_propagate_mask);
/*		printf("Object create Window = %d\n", XtWindow(frame));
*/
/*		sprintf(allcmdline,"%s -windowId %d %s &",
				classidPtr, XtWindow(frame),cmdline );
*/
		sprintf(allcmdline,"exec %s -windowId %d %s ",
				classidPtr, XtWindow(frame),cmdline );

/*		 printf("Executing : %s\n",allcmdline); */
/*		system(allcmdline); */
		{
			int pid;
			char *argv[10];
			argv[0]="/bin/sh";
			argv[1]="-c";
			argv[2]=allcmdline;
			argv[3]=0;
			pid = fork();
			if (pid == -1)
				assert(0);
			if (pid) { /* pere */
				fprintf(stderr,"pid du fils = %d\n",pid);
/*				sleep(30); */
/*				kill(pid,9); */
			} else {	/* fils */
				execvp(argv[0], argv);
				assert(0);
			}
		}
		if (saved_obs)
			fprintf(stderr,"OBJECT bug when Create\n");
		omptr->s_obs = obs ;
/* } */
		eptr->obs = obs;
	} else {		/* use the old window. It's a Resize */
		if (saved_obs == NULL){
			fprintf(stderr,"Abug in OBJECT when resizing\n");
			assert(0);
		} else {
			Widget frame;

			eptr->obs = saved_obs;
			frame = (Widget)saved_obs->frame;
			argcnt = 0;
			XtSetArg(arg[argcnt], XmNx, obs->x); argcnt++;
			XtSetArg(arg[argcnt], XmNy, obs->y); argcnt++;
			XtSetArg(arg[argcnt], XmNwidth, obs->width); argcnt++;
			XtSetArg(arg[argcnt], XmNheight, obs->height); argcnt++;
			XtSetValues(frame, arg, argcnt);
			XtSetMappedWhenManaged(frame, False);
			XtManageChild(frame);
			XFlush(XtDisplay(hw));
			_FreeObjectStruct(obs);
		}
	}
}

void ObjectRefresh(HTMLWidget hw, struct ele_rec *eptr)
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

	if(eptr->obs == NULL ) return;
	if(eptr->obs->frame == NULL ) return;
	x = eptr->x;
	y = eptr->y;
	px = x - hw->html.scroll_x;
	py = y - hw->html.scroll_y;
	XtSetArg(args[0], XtNx, px);
	XtSetArg(args[1], XtNy, py);
	XtSetValues((Widget)eptr->obs->frame, args,2);
	XtSetMappedWhenManaged((Widget)eptr->obs->frame, True);
}
#endif
