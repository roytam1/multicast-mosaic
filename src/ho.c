/* ho.c
 * Version 3.5.5 [Jun 2000]
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

#include <Xm/XmAll.h>

#include "../src/URLParse.h"
#include "../libhtmlw/HTMLparse.h"
#include "mosaic.h"

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
void MMPreParseObjectTag(mo_window * win, struct mark_up ** mptr)
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
	struct mark_up * pmptr;
	struct mark_up * omptr = *mptr;
	char * param_namePtr;
	char * param_valuePtr;
	char * param_valuetypePtr;
	AlignType valignment;
	HtmlObjectStruct * saved_obs=omptr->s_obs;
	HtmlObjectStruct *obs;

/* codetype: unused */
/* archive: unused */
/* declare: unused */
/* standby: unused */

/* comment arrive les donnees? 
 *   - en mode stream avec traitement a la volee.?
 *   - en mode fichier avec telechargement?
 * On choisit le mode stream pour plugger, mais on passe l'url a
 * l'application...!!! C'est a l'appli de se demerder. Comme d'hab.!
 */

	assert(omptr->s_obs == NULL);

	wPtr = ParseMarkTag(omptr->start, MT_OBJECT, "width");
	hPtr = ParseMarkTag(omptr->start, MT_OBJECT, "height");

/* if w or h is not here, that's a embed object with no window */
	if (!wPtr && hPtr)
		wPtr = strdup(hPtr);	/* set square */
	if (!hPtr && wPtr)
		hPtr = strdup(wPtr);	/* set square */

	if (!hPtr && !wPtr) {	/* object have no window */
		assert(0);
	}

/* classid="mtvp" */
	classidPtr = ParseMarkTag(omptr->start, MT_OBJECT, "classid");

	if (classidPtr && strchr(classidPtr,'/') ){
		fprintf(stderr,"Object not secure \n");
		/* virer l'objet */
		assert(0);
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
	if (!dataPtr) {
		assert(0);
	}
	dataPtr = URLParse(dataPtr, win->htinfo->base_url, 
		PARSE_ACCESS | PARSE_HOST | PARSE_PATH |PARSE_PUNCTUATION);

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
/* HALIGN_LEFT  HALIGN_RIGHT; */
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
		assert(0);
		return;
	}

	obs = (_HtmlObjectStruct *) calloc(1,sizeof(_HtmlObjectStruct));
	obs->bin_path = classidPtr;
	obs->height = atoi(hPtr);
	obs->width = atoi(wPtr);
	obs->content_type = content_typePtr;
	obs->codebase = codebasePtr;
	obs->data_url = dataPtr;
	obs->border_width = border_width;
	obs->valignment = valignment;
	obs->x =0;
	obs->y =0;		/* computed at render time */
	obs->frame = NULL;
	free(hPtr);
	free(wPtr);
	omptr->s_obs = obs;


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
		assert(0);
		*mptr = pmptr;
		_FreeObjectStruct(obs);
		return;
	}
/*#### obs->cw_only = pcc->cw_only; ############# */
/* mettre a jour mptr. Le mettre sur le tag </APROG> */
	*mptr = pmptr;
	omptr->s_obs = obs ;
/*	####_FreeObjectStruct(obs); */
}

static void RunP(mo_window *win, struct mark_up *mptr)
{
	char cmdline[15000];  
	char allcmdline[16000];
	int get_cnt = 0;      
	int i;
	HtmlObjectStruct *obs;
	Widget frame;

	obs = mptr->s_obs;
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
	strcat(cmdline, mptr->s_obs->data_url);

	frame = (Widget)mptr->s_obs->frame;

	sprintf(allcmdline,"exec %s -windowId %d %s ",
			mptr->s_obs->bin_path, XtWindow(frame),cmdline );
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
			mptr->s_obs->pid = pid;
/*			sleep(30); */
/*			kill(pid,9); */
		} else {	/* fils */
			execvp(argv[0], argv);
			assert(0);
		}
	}
}

void MMRunPlugins(mo_window *win, struct mark_up *mlist)
{
	struct mark_up *mptr = mlist;

	while (mptr) {
		if(mptr->type == M_OBJECT && !mptr->is_end) {
			RunP(win, mptr);
		}
		mptr = mptr->next;
	}
}

static void StopP(mo_window *win, struct mark_up *mptr)
{
	kill(mptr->s_obs->pid,9);
	XtDestroyWidget((Widget)mptr->s_obs->frame);
	mptr->s_obs->frame = NULL;
}

void MMStopPlugins(mo_window* win, mark_up* mlist)
{
	struct mark_up *mptr = mlist;

	while (mptr) {
		if(mptr->type == M_OBJECT && !mptr->is_end) {
			StopP(win, mptr);
		}
		mptr = mptr->next;
	}
}


#endif

/*	obs->internal_numeos = (int*)calloc(1, sizeof(int)); /*alloc one */
/*	obs->ret_filenames = (char **) malloc( sizeof(char *)); /* alloc one */
/*	obs->ret_filenames[obs->url_arg_count] = NULL; */
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
/*		} */
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
