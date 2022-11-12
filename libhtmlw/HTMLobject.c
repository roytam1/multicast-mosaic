/* HTMLobject.c
 * Version 3.6.9 [Mai 2000]
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

#ifdef linux
#include <X11/Xlib.h>
#endif

#include "HTMLparse.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include <Xm/XmAll.h>

#include "../src/URLParse.h"

void ObjectPlace(HTMLWidget hw, struct mark_up **mptr, PhotoComposeContext *pcc)
{
	int border_width;
	struct mark_up * omptr = *mptr;
	AlignType valignment;
	int height;
	int width;

	struct mark_up * pmptr  = NULL;
	struct ele_rec * eptr = NULL;
	HtmlObjectStruct * obs=NULL;
	int extra = 0;
	int argcnt ;
	Arg arg[20];
	int baseline=0;

	XWindowAttributes xwa;
	XSetWindowAttributes xswa;
	Widget frame;

#ifdef TODO
	assert(omptr->s_obs);
#else
	if (!omptr->s_obs)
		return;
#endif

	obs=omptr->s_obs;

#ifdef DEBUG_OBJECT
	fprintf(stderr, "ObjectPlace: *x=%d,*y=%d,Width=%d)\n",
			pcc->x,pcc->y,pcc->width_of_viewable_part);
#endif
/* codetype: unused */
/* archive: unused */
/* declare: unused */
/* standby: unused */

/* classid="mtvp" */ /* elsewhere bin_path*/
/* type="video/mpeg" */  /* content_type */
/* codebase="~dauphin/pluggin/mtvp/how-to-plug-mtvp */
/* data="fichier_video.mpg" */ /* data_url */

	height = obs->height;
	width  = obs->width;

	if (!height && !width) {	/* nowindow for plugin */
		return;
		assert(0);
	}
	
	border_width=obs->border_width;

	valignment = obs->valignment;
	baseline = obs->height;

/* on doit avance mptr pour trouver <PARAM name=nnn value="une valeur"> */
/* boucler tant qu'on a des <PARAM> puis boucler jusqu'a </APROG> */
	pmptr = omptr->next;
	obs->cw_only = pcc->cw_only;

/* pmptr pointe sur NULL ou le prochain element */
	while (pmptr && (pmptr->type != M_OBJECT) && (!pmptr->is_end)) {
		/* derouler jusqu'a </OBJECT>  */
		pmptr = pmptr->next;
	}
	if (!pmptr ){		/* la fin est obligatoire */
		assert(0);
		fprintf(stderr,"[ObjectPlace] Mark </OBJECT> not seen\n");
		*mptr = pmptr;
		/*_FreeObjectStruct(obs); */
		return;
	}

/* mettre a jour mptr. Le mettre sur le tag </APROG> */
	*mptr = pmptr;


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
		assert(0);
		/*_FreeObjectStruct(obs); */
		return;
	}

/*################## verifier si tout est bien liberer###### */
/* pour plugguer il faut creer une widget container avec le 'bon' placement */
/* et la 'bonne' taille qui doit rester */
/* UnMap-er jusqu'au 1er refresh */
/* il faut forker et activer le programme , en memorisant son 'pid' */
/* passer en parametre la XtWindow de la widget ainsi creee */

/* Creation d'une widget */

/* all data id here . Create */

	if (!omptr->s_obs->frame){
		argcnt = 0;
		XtSetArg(arg[argcnt], XmNx, obs->x); argcnt++;
		XtSetArg(arg[argcnt], XmNy, obs->y); argcnt++;
		XtSetArg(arg[argcnt], XmNwidth, obs->width); argcnt++;
		XtSetArg(arg[argcnt], XmNheight, obs->height); argcnt++;
		XtSetArg(arg[argcnt], XmNborderWidth, 0); argcnt++;
/*		XtSetArg(arg[argcnt], XmNnoResize, True);argcnt++; */
		XtSetArg(arg[argcnt], XmNresizeHeight, False); argcnt++;
		XtSetArg(arg[argcnt], XmNresizeWidth, False); argcnt++;
		XtSetArg(arg[argcnt], XmNmarginWidth, 0); argcnt++;
		XtSetArg(arg[argcnt], XmNmarginHeight, 0); argcnt++;
		XtSetArg(arg[argcnt], XmNadjustLast, False); argcnt++;
		XtSetArg(arg[argcnt], XmNadjustMargin, False); argcnt++;

/*		XtSetArg(arg[argcnt], XmNcolormap, hw->core.colormap); argcnt++;*/
		frame = XmCreateRowColumn(hw->html.view, "PlugWindow", arg, argcnt);

/* Set window for plugin */
		assert(!omptr->s_obs->frame); 	/* it must be ONE creation */
		omptr->s_obs->frame = frame;

		XtSetMappedWhenManaged(frame, False);
		XtManageChild(frame);
		XtRealizeWidget(frame);
		XFlush(XtDisplay(hw));
		XGetWindowAttributes(XtDisplay(frame), XtWindow(frame),&xwa);

/* printf("colormap = %d\n", xwa.colormap); */
/*printf("xwa.do_not_propagate_mask = %x\n", xwa.do_not_propagate_mask); */

		xswa.do_not_propagate_mask = xwa.do_not_propagate_mask & (~(PointerMotionMask));
		XChangeWindowAttributes(XtDisplay(frame),
				XtWindow(frame),CWDontPropagate, &xswa);
		XFlush(XtDisplay(hw));
	} else {		/* use the old window. It's a Resize */
		Widget frame;

		frame = (Widget)omptr->s_obs->frame;
		argcnt = 0;
		XtSetArg(arg[argcnt], XmNx, obs->x); argcnt++;
		XtSetArg(arg[argcnt], XmNy, obs->y); argcnt++;
		XtSetArg(arg[argcnt], XmNwidth, obs->width); argcnt++;
		XtSetArg(arg[argcnt], XmNheight, obs->height); argcnt++;
		XtSetValues(frame, arg, argcnt);
		XtSetMappedWhenManaged(frame, True);
		XtManageChild(frame);
		XFlush(XtDisplay(hw));
	}
	eptr->obs = omptr->s_obs;
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
