/* paf.c Copyright (C) 1997 - G.Dauphin.
 * Version 3.2.4. [Oct 97]
 * See the file "license.mMosaic"
 */

/* paf : post and forget.
 * intended to 'multithread' a data's load as an autonom widget.
 * Unicast only.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "URLParse.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "telnet-proto.h"
#include "gui.h"
#include "gui-documents.h"
#include "mime.h"
#include "navigate.h"
#include "cache.h"
#include "img.h"
#include "paf.h"
#include "www-con.h"
#include "compress.h"
#include "globalhist.h"
#include "converter.h"

#ifdef MULTICAST
extern mo_window * mc_send_win;
#endif

void MMPafLoadEmbeddedObjInDoc(PafDocDataStruct * paf_child);

static XmxCallback (icon_pressed_cb)
{
	mo_window * win = (mo_window*) client_data;

	if (!win->pafd)         /* no transfert in progress */
		return;
	(*win->pafd->call_me_on_stop)(win->pafd); /* call up->down proc */
}

void MMFinishPafSaveData(PafDocDataStruct * pafd)
{
	mo_window * win;

	win = pafd->win;

/* free the things we have build in MMPafSaveData */
	close(pafd->fd);
	free(pafd->fname);
	free(pafd->aurl);

/* stop the twirl */                  
	XtRemoveTimeOut(pafd->twirl_struct->time_id);
	free(pafd->twirl_struct);
	free(pafd->sps.accept);
	free(pafd);
	win->pafd = NULL;
	XtPopdown(win->base);
	XtDestroyWidget(win->base);
	free(win);
}

void MMErrorPafSaveData(PafDocDataStruct * pafd, char * reason)
{
	mo_window * win;

	win = pafd->win;

/* free the things we have build in MMPafSaveData */
	close(pafd->fd);
	unlink(pafd->fname);
	free(pafd->fname);
	free(pafd->aurl);

	free(pafd);
	XtPopdown(win->base);
	XtDestroyWidget(win->base);
	free(win);
}

void MMStopPafSaveData(PafDocDataStruct * pafd)
{
	mo_window * win = pafd->win;

	XtRemoveTimeOut(pafd->twirl_struct->time_id);
	free(pafd->twirl_struct);

	if (pafd->www_con_type && pafd->www_con_type->call_me_on_stop_cb ) {
                (*pafd->www_con_type->call_me_on_stop_cb)(pafd);
        }
	
        pafd->www_con_type = NULL;

	free(pafd->sps.accept);

	free(pafd->aurl);
	free(pafd->aurl_wa);
	close(pafd->fd);
	unlink(pafd->fname);
	free(pafd->fname);
	free(pafd->mhs);
	free(pafd);
	XtPopdown(win->base);
	XtDestroyWidget(win->base);
	free(win);
}

/* Popup a 'load data' widget.
 * top :	The toplevel widget.
 * aurl:	the absolute URL. where to find the data.
 *		It is a 'clean' url with notag like '?'
 * fname:	the filename where to put the data.
 *
 * This is a autonomous run. make 'something' to have enought code
 * running by itself.
 */
void MMPafSaveData(Widget top, char * aurl, char * fname)
{
	int fd;		/* the filedes of fname */
	Widget form, url_label, dest_label,
		sep;
	mo_window * win;
	PafDocDataStruct * pafd = NULL;

	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if ( fd < 0) {
		char * info = (char*) malloc(strlen(fname) + 100);

		sprintf(info,"Can't open file :\n%s",fname);
		XmxMakeErrorDialog(top, info, "Error");
		free(info);
		return;
	}
/* alloc the data to be autonomous */
	win = (mo_window *) malloc(sizeof(mo_window));
	pafd = (PafDocDataStruct *) calloc(1, sizeof(PafDocDataStruct));
	pafd->mhs = (MimeHeaderStruct*) calloc(1, sizeof(MimeHeaderStruct));
	pafd->fname = strdup(fname);
	pafd->fd = fd;
	pafd->aurl = strdup(aurl);
	pafd->aurl_wa = strdup(aurl);
	pafd->win = win;
	win->pafd = pafd;

	Xmx_n = 0;
	XmxSetArg (XmNiconName, (long)"mMosaic Loading");
	XmxSetArg (XmNallowShellResize, False);
	win->base = XtCreatePopupShell("mMosaic: Load to disk",
		topLevelShellWidgetClass, top, Xmx_wargs, Xmx_n);
	Xmx_n = 0;

	form = XmxMakeForm(win->base);

/* create Source(the url), Destination (File), KByte loaded and Stop */
	url_label = XmxMakeLabel(form, "Source:");
	XmxSetArg (XmNeditable, False);
	win->url_widget = XmxMakeTextField(form);

	dest_label = XmxMakeLabel(form, "Destination:");
	XmxSetArg (XmNeditable, False);
	win->dest_widget = XmxMakeTextField(form);

	win->tracker_widget = XmxMakeLabel(form, "Requesting Connection...");
	sep = XmxMakeHorizontalSeparator(form);
	win->logo = XmxMakePushButton( form, "Stop", icon_pressed_cb, (XtPointer)win);
	XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);

/* update pafd */
	pafd->www_con_type = NULL;

	pafd->call_me_on_succes = MMFinishPafSaveData;
	pafd->call_me_on_error = MMErrorPafSaveData;
	pafd->call_me_on_stop = MMStopPafSaveData;
	pafd->sps.accept = strdup("*/*");

	pafd->pragma_no_cache = False;

	pafd->twirl_struct = (TwirlStruct*) malloc(sizeof(TwirlStruct));
	pafd->twirl_struct->logo_widget = win->logo;
	pafd->twirl_struct->logo_count = 0;
	pafd->twirl_struct->time_id = XtAppAddTimeOut(mMosaicAppContext,
		100L, twirl_icon_cb, pafd->twirl_struct);

/* Constraints */
	XmxSetOffsets(url_label,14,10,10,10);
	XmxSetConstraints(url_label, XmATTACH_FORM, XmATTACH_NONE,
					XmATTACH_FORM, XmATTACH_NONE,
					 NULL, NULL, NULL, NULL);
	XmxSetOffsets(win->url_widget, 10,10,10,10);
	XmxSetConstraints(win->url_widget, XmATTACH_FORM, XmATTACH_NONE,
					XmATTACH_WIDGET, XmATTACH_FORM,
					NULL, NULL, url_label, NULL);

	XmxSetOffsets(dest_label,14,10,10,10);
	XmxSetConstraints(dest_label,XmATTACH_WIDGET, XmATTACH_NONE,
					XmATTACH_FORM, XmATTACH_NONE,
					url_label, NULL, NULL, NULL);
	XmxSetOffsets(win->url_widget, 10,10,10,10);
	XmxSetConstraints(win->url_widget,XmATTACH_WIDGET, XmATTACH_NONE,
					XmATTACH_WIDGET, XmATTACH_FORM,
					url_label, NULL, dest_label, NULL);

	XmxSetOffsets(win->tracker_widget,14,10,10,10);
	XmxSetConstraints(win->tracker_widget, XmATTACH_WIDGET, XmATTACH_NONE,
					XmATTACH_FORM, XmATTACH_NONE,
					dest_label, NULL, NULL, NULL);
	XmxSetArg (XmNtopOffset, 10);
	XmxSetConstraints(sep, XmATTACH_WIDGET, XmATTACH_NONE,
				XmATTACH_FORM, XmATTACH_FORM,
				win->tracker_widget, NULL, NULL, NULL);

	XmxSetOffsets(win->logo,14,10,10,10);
	XmxSetConstraints(win->logo, XmATTACH_WIDGET, XmATTACH_FORM,
				XmATTACH_FORM, XmATTACH_FORM,
				sep, NULL, NULL, NULL);

/* Set text field */
	XmxTextSetString(win->url_widget, aurl);
	XmxTextSetString(win->dest_widget,fname);

	XtManageChild (form);
	XtPopup(win->base, XtGrabNone);

/* Next step is to connect. Read the data and put the data in file */
	PostRequestAndGetTypedData(aurl, fname, pafd);
}

/* -------------------- Loading HTML document in Window -----------------*/

/* down->up procedure */
void MMErrorPafDocData (PafDocDataStruct * pafd, char *reason)
{
	mo_window * win = pafd->win;

/* stop the twirl */
	XtRemoveTimeOut(pafd->twirl_struct->time_id);
	free(pafd->twirl_struct);

	if (pafd->post_ct && pafd->post_data){
		free(pafd->post_data);
		free(pafd->post_ct);
		pafd->post_ct = NULL;
		pafd->post_data = NULL;
	}
	free(pafd->sps.accept);
	free(pafd->aurl);
	free(pafd->aurl_wa);
	if (pafd->goto_anchor)
		free(pafd->goto_anchor);
	close(pafd->fd);
	unlink(pafd->fname);
	free(pafd->fname);
	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;
	XmxMakeErrorDialog(win->base, reason , "Net Error");

	free(pafd);
	win->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
}

/* top-> down procedure */
void MMStopPafDocData(PafDocDataStruct * pafd)
{
	mo_window * win = pafd->win;

/* stop the twirl */
	XtRemoveTimeOut(pafd->twirl_struct->time_id);
	free(pafd->twirl_struct);

	if (pafd->paf_child) { /* a embedded object in progress */
		(*pafd->paf_child->call_me_on_stop)(pafd->paf_child);
	}
	pafd->paf_child =NULL;

/* if a www connection is in progress, abort it */
	if (pafd->www_con_type && pafd->www_con_type->call_me_on_stop_cb ) {
		(*pafd->www_con_type->call_me_on_stop_cb)(pafd);
	}
	pafd->www_con_type = NULL;

	if (pafd->post_ct && pafd->post_data){
		free(pafd->post_data);
		free(pafd->post_ct);
		pafd->post_ct = NULL;
		pafd->post_data = NULL;
	}
	free(pafd->sps.accept);

	free(pafd->aurl_wa);
	free(pafd->aurl);
	if (pafd->goto_anchor)
		free(pafd->goto_anchor);

	close(pafd->fd);
	unlink(pafd->fname);
	free(pafd->fname);

	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;
	free(pafd);
	win->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
}

/* Not a telnet and not an override, but text present (the "usual" case): */

void MMFinishPafDocData(PafDocDataStruct * pafd)
{
	PafDocDataStruct * pafc;
	EmbeddedObjectEntry * eo_tab;
	mo_window * win;
	char * cached_fname;
	char * presentation;
	char * data;
	struct mark_up *mlist, *mptr;
	int fd;
	int lread;
	int docid;
	char * title;
	char * base_url;
	int in_doc_head = 0;
	int in_title = 0;
	char * title_text;

	win = pafd->win;

/* free the things we have build in MMPafDocData */
	close(pafd->fd);

/* test some HTTP return code */

	if (pafd->con_type == HTTP_CON_TYPE){
		switch (pafd->http_status){
		case 303:
		case 304:
		case 305: 
		case 307:
			(*pafd->call_me_on_error)(pafd,"can't process server respons code: 30[3457]");
		case 300:
		case 301:
		case 302:
			pafd->n_redirect++;
			if (pafd->n_redirect > 4){
				(*pafd->call_me_on_error)(pafd,"to many redirect");
				return;
			}
			unlink(pafd->fname);
			pafd->fd = open(pafd->fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			pafd->aurl_wa = strdup(pafd->mhs->location);
			pafd->aurl = strdup(pafd->mhs->location);
			FreeMimeStruct(pafd->mhs);

			pafd->pragma_no_cache = True;
/* And redo a reuqest for redirect */
			PostRequestAndGetTypedData(pafd->aurl,pafd->fname,pafd);
			return;
		} 
	}

/* fname is the file containing the HTML document */

	if (pafd->mhs->content_length == -1){
		/* determine la taille en fonction de la taille du fichier */
		/* si le serveur n'envoie pas de Content-length */
		pafd->mhs->content_length = pafd->total_read_data;
	}
/* mettre les donnes dans le cache si elles sont valides: date code etc... 
 * a faire dans le cache....last_modified expires
 * pas de cache pour le mode ftp 
 * donc on peut mettre ca dans le cache si c'est une 'bonne' url 
 */
/* dont't cache request with post data and CACHE_CONTROL_NO_STORE */
	if( !(( pafd->mhs->cache_control & CACHE_CONTROL_NO_STORE) ||
	      (pafd->post_ct && pafd->post_data)) ) {
		MMCachePutDataInCache(pafd->fname, pafd->aurl_wa, pafd->aurl,
			pafd->mhs);
	}

#ifdef MULTICAST
	if (mc_send_win == pafd->win) {/* A multicast send window */
/* prevenir qu'on a de nouvelles donnees */
		(*mc_send_win->mc_callme_on_new_doc)(pafd->fname,
			pafd->aurl_wa, pafd->mhs);
	}
#endif
/* faire qques conversions (voir MIME) sur les donnees et les mettre en memoire */
/* mettre dans le cache de premier niveau (en memoire) */

/* chercher comment doit etre presente le document  en fonction du type mime et de
 * la compression une fenetre mMosaic ne sait presenter que du MMOSAIC_PRESENT. 
 * content_encoding a ete mis a jour soit par :
 * - entete mime de HTTP
 * - type du fichier dans le cas ftp
 */
	if (pafd->mhs->content_encoding == COMPRESS_ENCODING ||
	    pafd->mhs->content_encoding == GZIP_ENCODING ){
			/* decompresser le fichier */
		XmxAdjustLabelText(pafd->win->tracker_widget,"Decompressing data.");
		XFlush(mMosaicDisplay);
		DeCompress(pafd->fname, pafd->mhs);
		pafd->mhs->content_encoding = NO_ENCODING;
	}
/* NO_ENCODING */
/* quelle presentation ? */
	presentation = MMct2Presentation(pafd->mhs->content_type);
	if (! presentation){
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMFinishPafDocData, presentation = NULL, content_type = '%s'\n", pafd->mhs->content_type);
		fprintf(stderr, "Aborting ...\n");
		abort();
		/* envoyer un message d'erreur */
	}
	if ( strcmp(presentation, MMOSAIC_PRESENT) ){
		int l = strlen(presentation) + strlen(pafd->fname)+10;
		char * command = (char*)malloc(6*l);
		char * cmd = (char*)malloc(10*l);

		if (strstr(presentation, "%s")){
			sprintf (command, presentation,
				pafd->fname, pafd->fname, pafd->fname,
				pafd->fname, pafd->fname);
			sprintf(cmd, "(%s ; /bin/rm -f %s) &",
				command, pafd->fname);
		} else { /* cat the file. */
			sprintf(cmd, "((cat %s | %s); /bin/rm -f %s) &",
				pafd->fname, presentation, pafd->fname);
		}
			
		
		system(cmd);
		free(command);
		free(cmd);
		win = pafd->win;
/* stop the twirl */                   
		XtRemoveTimeOut(pafd->twirl_struct->time_id);
		free(pafd->twirl_struct);
		free(pafd->sps.accept);
		close(pafd->fd);       
		free(pafd->fname);     
		free(pafd);            
		win->pafd = NULL;      
/* securityType=HTAA_UNKNOWN; */       
		return; /* it is finish!!! */
	} 

/* La presentation est pour mMosaic */
/* c"est qu'on a du html ou qqe chose que mMosaic sait convertir, comme par
 * exemple du text/plain, dans le corps du message HTTP */
	data = (char*) malloc(pafd->mhs->content_length +1);
	fd = open(pafd->fname,O_RDONLY);
	if ( fd <0){
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMFinishPafDocData, Out of fd\n");
		fprintf(stderr, "Aborting ...\n");
		abort();
	}
	lread = read(fd, data, pafd->mhs->content_length);
	if (lread != pafd->mhs->content_length) {
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMFinishPafDocData, Bug in reading data, lread = %d, content_length = %d\n", lread, pafd->mhs->content_length);
		fprintf(stderr, "Aborting ...\n");
		abort();
	}
	data[pafd->mhs->content_length] = '\0';
	close(fd);
	if( !strcmp(pafd->mhs->content_type,"text/plain") ) { /* convert plain to html*/
		int len_ret;

		MMa2html(&data, lread, &len_ret);
/* don't free old mhs->content_type, we need it */
		pafd->mhs->content_type = strdup("text/html");
		pafd->mhs->content_length = len_ret;
	}
	pafd->html_text = data;

/* dans le fichier decompresse (eventuellement) on a de l'HTML */
/* Faire un Parse pour le decomposer en objet */

	pafd->mlist = mlist = HTMLParse(data);
/* detecter les frames et dans ce cas ajouter un niveau d'indirection */

/* Faire une liste des embedded object */
/* initialiser et seulement initialiser ces objects */

	mptr = mlist;
	pafd->num_of_eo = 0;
	base_url = NULL;
	eo_tab = (EmbeddedObjectEntry *) malloc(sizeof(EmbeddedObjectEntry));
			/* alloc one */
/* recuperer le reste des embedded objects (si necessaire)*/
/* relancer la bete pour les embedded objets si il y en a */
	in_doc_head = 0;
	in_title =0;
	title_text = NULL;
	title = strdup(pafd->aurl_wa);
	while (mptr != NULL){
		char * tptr;
		char * url;
		ImageInfo * picd;
		int status;
		int force_load;

/* if in_title, grab the text until end marker */
		if (in_title && (mptr->type != M_TITLE) && (mptr->type !=M_NONE)){
			mptr = mptr->next;
			continue;
		}
		switch (mptr->type){
		case M_NONE:
			if (in_title) {
				if ( !title_text) {
					title_text = strdup(mptr->text);
					break;
				}
				title_text = (char*)realloc(title_text,
					strlen(title_text)+ strlen(mptr->text)+2);
				strcat(title_text,mptr->text);
				break;
			}
			break;
		case M_TITLE:
			if (mptr->is_end) {
				in_title = 0;
				if (title_text)
					title = title_text;
				break;
			}
			in_title = 1;
			title_text = NULL;
			break;
/* take care of tag BASE */
		case M_BASE:
			if (mptr->is_end)
				break;
			base_url = ParseMarkTag(mptr->start, MT_BASE, "HREF");
			break;
		case M_DOC_BODY:		/* BODY can have image... */
			if (mptr->is_end)
				break;
			in_doc_head = 0;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageBody(pafd->win, picd, mptr);
			mptr->s_picd = picd;	/* if image body is wrong,*/
						/* display nothing */
			if (!picd->src) {
				free(picd);
				mptr->s_picd = NULL;
				break;
			}
			eo_tab[pafd->num_of_eo].url = picd->src;
			eo_tab[pafd->num_of_eo].mark = mptr;
			eo_tab[pafd->num_of_eo].num = pafd->num_of_eo;
			pafd->num_of_eo++;
			eo_tab = (EmbeddedObjectEntry *) realloc(eo_tab,
				(pafd->num_of_eo + 1) * sizeof(EmbeddedObjectEntry)); 
			break;
		case M_INPUT:
			if (mptr->is_end)       /* no end mark on <input> */
				break;
/* get only if type == image */
			tptr = ParseMarkTag(mptr->start, MT_INPUT, "TYPE");
			if (tptr == NULL) 	/* assume text */
				break;
			if ( strcasecmp(tptr, "image") ) { /* not an image */
				free(tptr);
				break;
			}
			free(tptr);
/* continue with image processing */
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseInputTagImage(pafd->win, picd, mptr);
			mptr->s_picd = picd; /* in all case display something*/
			if (picd->internal && !picd->fetched && !picd->delayed){
				break; /* error in image tag */
					/* don't try to find it */
			}
			mptr->s_picd = picd; /* in all case display something*/

			if ( picd->fetched) {
				break; /* we have it */
			}
			eo_tab[pafd->num_of_eo].url = picd->src;
			eo_tab[pafd->num_of_eo].mark = mptr;
			eo_tab[pafd->num_of_eo].num = pafd->num_of_eo;
			pafd->num_of_eo++;
			eo_tab = (EmbeddedObjectEntry *) realloc(eo_tab,
				(pafd->num_of_eo + 1) * sizeof(EmbeddedObjectEntry)); 
			break;
		case M_IMAGE:
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageTag(pafd->win, picd, mptr);
/* on return : two case on fetched:               
 * - fetched = True => this is an internal , width height and image are ok 
 * - fetched = False => remaing field is not uptodate and need to be updated   
 * 	 MMPreParseImageTag returns a delayimage
 */
			mptr->s_picd = picd; /* in all case display something*/

			if (picd->internal && !picd->fetched && !picd->delayed){
				break; /* error in image tag */
					/* don't try to find it */
			}
/* now we have an image. It is :
        - an internal-gopher    (internal=1,fetched=1, delayed =0)
        - a delayed image       (internal=1, fetched=0, delayed=1)
        - the requested image   (internal=0, fetched=1, delayed=0) never happen
 a no image (internal=1, fetched=0, delayed=0) will be made when a resquest
 to get the image is not a succes. or SRC tag is not present.
*/ 
			if ( picd->fetched) { /* internal image found */
				break; /* we have it */
			}
			eo_tab[pafd->num_of_eo].url = picd->src;
			eo_tab[pafd->num_of_eo].mark = mptr;
			eo_tab[pafd->num_of_eo].num = pafd->num_of_eo;
			pafd->num_of_eo++;
			eo_tab = (EmbeddedObjectEntry *) realloc(eo_tab,
				(pafd->num_of_eo + 1) * sizeof(EmbeddedObjectEntry)); 
			break;
/* see it later ########### 
 *		case M_APPLET:
 *			parse for codebase=
 *		case M_APROG:	 must become OBJECT 
 *			parse for something=
 */
		}
		mptr = mptr->next;
	}

/* afficher le texte. le mettre dans la Widget. id come from back and forward */
	docid = 0;			/* we are not in back or forward */
	HTMLSetHTMLmark (pafd->win->scrolled_win, mlist, docid, pafd->goto_anchor,
		pafd->aurl);
	XFlush(XtDisplay(pafd->win->scrolled_win));
/* #####
	docid = HTMLGetDocId(pafd->win->scrolled_win);
*/


/* on met a jour immediatement la partie navigation. Car on doit avoir un
 * current_node qui memorise tout la requete */
/* title is alway allocated. */
	MMUpdNavigationOnNewURL(win, pafd->aurl_wa, pafd->aurl, pafd->goto_anchor, base_url, title,
		pafd->html_text, pafd->mhs, docid, mlist); 
/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */

	mo_set_win_headers(win, pafd->aurl_wa);

/* MAJ de l'history etc... */
	MMUpdateGlobalHistory(pafd->aurl);

	if (pafd->num_of_eo == 0 ){	/* no embedded object it is the end */
		free(eo_tab);
/* en finir avec la requete de pafd. */
        	win = pafd->win;
/* don't do this: registered in navigation */
/*		free(pafd->aurl); free(pafd->aurl_wa); free(pafd->mhs); */
 
/* stop the twirl */
		XtRemoveTimeOut(pafd->twirl_struct->time_id);
		free(pafd->twirl_struct);
		free(pafd->sps.accept);
		close(pafd->fd);
		unlink(pafd->fname); 
		free(pafd->fname);
		free(pafd);
		win->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
		return; /* it is finish!!! */
	}
/* sinon c'est pas fini... recuprer les embedded objects */
/* the next step is to process embedded object */
	pafd->embedded_object_tab = eo_tab; /* une selection de mlist*/
	pafd->cur_processing_eo = 0;		/* demande le premier */
/* a paf child inherit some data from its father */
	pafd->paf_child = (PafDocDataStruct *) malloc( sizeof(PafDocDataStruct));
	pafd->paf_child->parent_paf = pafd;
	pafc = pafd->paf_child;
	pafc->mhs = (MimeHeaderStruct*) malloc( sizeof(MimeHeaderStruct));
/* inherit from parent */
        pafc->twirl_struct = pafd->twirl_struct;
/* don't inherit from parent */
        pafc->proxent = NULL;;
        pafc->pragma_no_cache = False;  /* pafd->pragma_no_cache ; */  
	pafc->post_ct = NULL;
	pafc->post_data = NULL;

/* #### plus tard */                   
	MMPafLoadEmbeddedObjInDoc( pafd->paf_child);
/* la requete est complete pour la partie html du docu. */
/* On ne passe plus par ici */
}


/* ########### add the multicast sender code here ######## */
/*#ifdef MULTICAST
 *        if((win->mc_type == MC_MO_TYPE_MAIN) && mc_send_enable){
 *                if (mc_send_win !=NULL ){
 *                        McSetHtmlTexte(txt);
 *                }
 *        }
 *#endif

/* #########################################*/
/* mettre a jour le tracker_label */
/* XmString xmstr=XmStringCreateLtoR (" ", XmSTRING_DEFAULT_CHARSET);
 * XtVaSetValues (win->tracker_label, XmNlabelString, (XtArgVal)xmstr, NULL);
 * XmStringFree (xmstr);
/*if(win->links_win) /* vvv HREF ListBox Stuff -- BJS 10/2/95 */ 
/*mo_update_links_window(win);
/* Every time we view the document, we reset the search_start
 * struct so searches will start at the beginning of the document. */
/*((ElementRef *)win->search_start)->id = 0;
 *win->src_search_pos=0; 
/* some gui work */
/* Update source text if necessary. */      
/*if(win->source_text && XtIsManaged(win->source_text) && win->current_node) {
 *XmxTextSetString (win->source_text, win->current_node->text);
 *XmxTextSetString (win->source_url_text, win->current_node->url);
 *XmxTextSetString (win->source_date_text, (win->current_node->last_modified?win->current_node->last_modified:"Unknown")); 
 *}
 *if (win->current_node)              
 *mo_gui_check_security_icon_in_win(win->current_node->authType,win);
 *  mo_gui_done_with_icon (win);
/* } */

/* win : the mo_window to process. */
/* req_url : The document's url to load in win */
/*	 this request_url is maybe, relative... */
void MMPafLoadHTMLDocInWin( mo_window * win, RequestDataStruct * rds)
{
	PafDocDataStruct * pafd = NULL;
	PafDocDataStruct * opafd = NULL;
	char * req_url;
	char * cur_url;
	char * old_url;
	char * new_url_with_a;
	char * new_canon_url;
	char * fname;
	int fd;
	int reloading;

	opafd = win->pafd;	/* Is a previous load in progress */
	if (opafd) {		/* if YES stop the previous request */
		MMStopPafDocData(opafd);
	}
	reloading = rds->is_reloading;
	req_url = rds->req_url;

/* parse the new_url against the current_url in mo_window (if exist) */
	cur_url = "";
	if (win->current_node) { 	/* we used this win in the past */
		cur_url = win->current_node->base_url;
	}

/* We Turn a URL into its canonical form , based on the previous URL in this
 * context (if appropriate).
 *          INTERNAL ANCHORS ARE *NOT* STRIPPED OFF.
 * We KEEP anchor information already present in url, but NOT in old_url.*/

	old_url = URLParse (cur_url, "",
		PARSE_ACCESS | PARSE_HOST | PARSE_PATH | PARSE_PUNCTUATION);
	new_url_with_a = URLParse (req_url, old_url,
		PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
		PARSE_PUNCTUATION | PARSE_ANCHOR);
	free(old_url);

/* We LOSE anchor information. */
	new_canon_url =  URLParse (new_url_with_a, "",
                  PARSE_ACCESS | PARSE_HOST | PARSE_PATH | PARSE_PUNCTUATION);

/* test some easy things first: telnet rlogin */
/* because we don't really post it */
	if( !strncmp(new_url_with_a, "telnet:", 7) ||
	    !strncmp(new_url_with_a, "rlogin:", 7) ) {
/* we don't need to register it in history . Just fork a xterm with telnet */
/* free some allocated object */
		char * access;
		char * host;  

		access =  URLParse(new_canon_url, "telnet:", PARSE_ACCESS);
		host = URLParse(new_canon_url, "", PARSE_HOST);
		MMStartRemoteSession(access, host, win);
		free(host);
		free(access); 
		free(new_url_with_a);
		free(new_canon_url);
		return ;
	}

	fname = tempnam (mMosaicTmpDir,"mMo"); /* temp file for work */
	fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
	if ( fd < 0) {                
		XmxMakeErrorDialog(win->base, "Can't open temp file", "System Error");
		return;               
	}
/* fill the 'paf' struct for the request */
	pafd = (PafDocDataStruct *) malloc( sizeof(PafDocDataStruct));
	pafd->mhs = (MimeHeaderStruct*) malloc( sizeof(MimeHeaderStruct));
	pafd->mhs->cache_control = CACHE_CONTROL_NONE;
	pafd->www_con_type = NULL;
	pafd->fname = fname;
	pafd->fd = fd;
	pafd->aurl_wa = new_url_with_a;
	pafd->aurl = new_canon_url;
	pafd->goto_anchor = URLParse (pafd->aurl_wa,"", PARSE_ANCHOR);
	if ( !(*pafd->goto_anchor) ) {
		free(pafd->goto_anchor);
		pafd->goto_anchor = NULL;
	}
	pafd->win = win;
	pafd->n_redirect = 0;
	win->pafd = pafd;
	pafd->call_me_on_succes = MMFinishPafDocData;
 	pafd->call_me_on_error = MMErrorPafDocData;
	pafd->call_me_on_stop = MMStopPafDocData;
	pafd->sps.accept = strdup("*/*");
	pafd->pragma_no_cache = reloading;
	pafd->paf_child =NULL;
	pafd->post_ct = NULL;
	pafd->post_data = NULL;
	pafd->proxent = NULL;
	if (rds->ct && rds->post_data) {
		pafd->post_ct = strdup(rds->ct);
		pafd->post_data = strdup(rds->post_data);
	}

/* #### plus tard */
	pafd->twirl_struct = (TwirlStruct*) malloc(sizeof(TwirlStruct));
	pafd->twirl_struct->logo_widget = win->logo;
	pafd->twirl_struct->logo_count = 0;
	pafd->twirl_struct->time_id = XtAppAddTimeOut(mMosaicAppContext,
		100L, twirl_icon_cb, pafd->twirl_struct);

/* Next get the document as per protocol: http,file,ftp,news/nntp,gopher */
/* send a request via www-con */
/* activate the state machine to load the document */
/* Ne pas oublier qu'on peut etre interrompu pafd->call_me_on_interupt = ???; */
/* Dans MMFinishPafDocData on a le document HTML, il faut aller chercher
 * les embedded object connu de mMosaic : imag applet etc...  */

	PostRequestAndGetTypedData(new_canon_url,fname,pafd);
}

/* ################################ */
/* -------------------- Loading Embeded object in document -----------------*/
void MMStopPafEmbeddedObject(PafDocDataStruct * pafc)
{
	mo_window * win = pafc->win;
	PafDocDataStruct * ppaf;
	ppaf = pafc->parent_paf;

	if ( pafc->www_con_type)
		(*pafc->www_con_type->call_me_on_stop_cb)(pafc);

	close(pafc->fd);
	unlink(pafc->fname);
	free(pafc->sps.accept);
	free(pafc->aurl);
	free(pafc->aurl_wa);
	free(pafc->fname);
	free(pafc->mhs);
	free(pafc);
	ppaf->paf_child = NULL;
}

void MMErrorPafEmbeddedObject (PafDocDataStruct * pafc, char *reason)
{
	PafDocDataStruct * ppaf;
	struct mark_up * mptr;
	char * goto_anchor = NULL;
	mo_window * win;
	int id;
	int delayed;

	if ( !strcmp(reason, "DelayedRequest") ){
		delayed = True;
	} else {
		delayed = False;
	}
	close(pafc->fd);
	unlink(pafc->fname);
	ppaf = pafc->parent_paf;
	mptr = ppaf->embedded_object_tab[ppaf->cur_processing_eo].mark;
	switch(mptr->type){
	case M_DOC_BODY:
		free(mptr->s_picd);
		mptr->s_picd = NULL;
		break;
	case M_INPUT:
	case M_IMAGE:
		if( !delayed){	/* change to a break image */
			MMPreloadImage(pafc->win, mptr, pafc->mhs, NULL);
		}
		mptr->s_picd->src = pafc->aurl_wa;
		break;
	default:
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMErrorPafEmbeddedObject, Unknow EmbeddedObject type %d\n", mptr->type);
		fprintf(stderr, "Aborting ...\n");
		abort();
	}

	ppaf->cur_processing_eo++;
	if ( ppaf->cur_processing_eo >= ppaf->num_of_eo) {
/* we have all embedded and doc */ /* get the current id  #### */
		id = 0;
/* get the goto_anchor */
		goto_anchor = NULL;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, id, ppaf->goto_anchor, ppaf->aurl);
                free(ppaf->embedded_object_tab);
		free(ppaf->paf_child); /*### et le reste de paf_child*/
/* en finir avec la requete de pafd. */
                win = ppaf->win;
/* stop the twirl */
                XtRemoveTimeOut(ppaf->twirl_struct->time_id);
                free(ppaf->twirl_struct);
                free(ppaf->sps.accept);
                close(ppaf->fd);   
                unlink(ppaf->fname);
                free(ppaf->fname);
                free(ppaf);
                win->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
		return;	/* it is really the end !!! */
	}

/* else loop once more time */
	MMPafLoadEmbeddedObjInDoc( ppaf->paf_child);
}

/* Not a telnet and not an override, but text present (the "usual" case): */

void MMFinishPafEmbeddedObject(PafDocDataStruct * pafc)
{
	char * data;
	char * goto_anchor = NULL;
	PafDocDataStruct * ppaf;
	struct mark_up * mptr;
	mo_window * win;
	int id;
	int fd;
	int lread;

	close(pafc->fd);
	ppaf = pafc->parent_paf;
	mptr = ppaf->embedded_object_tab[ppaf->cur_processing_eo].mark;
/* pre-process the object */
	if (pafc->mhs->content_length == -1){
                /* determine la taille en fonction de la taille du fichier */
                /* si le serveur n'envoie pas de Content-length */
                pafc->mhs->content_length = pafc->total_read_data;
        } 
/* dont't cache request with post data and CACHE_CONTROL_NO_STORE */
	if( !(( pafc->mhs->cache_control & CACHE_CONTROL_NO_STORE) ||
	      (pafc->post_ct && pafc->post_data)) ) {
		MMCachePutDataInCache(pafc->fname, pafc->aurl_wa, pafc->aurl,
			pafc->mhs);
	}

	if (pafc->mhs->content_encoding == COMPRESS_ENCODING ||
	    pafc->mhs->content_encoding == GZIP_ENCODING ){
			/* decompresser le fichier */
		XmxAdjustLabelText(pafc->win->tracker_widget,"Decompressing data.");      
		XFlush(mMosaicDisplay);
		DeCompress(pafc->fname, pafc->mhs);
		pafc->mhs->content_encoding = NO_ENCODING;
	}

	switch(mptr->type){
	case M_DOC_BODY:
		mptr->s_picd->src = pafc->aurl_wa;
		MMPreloadImage(pafc->win, mptr, pafc->mhs, pafc->fname);
		if ( !mptr->s_picd->fetched ){
			free(mptr->s_picd);
			mptr->s_picd = NULL;
		}
		break;
	case M_INPUT:
	case M_IMAGE:
		mptr->s_picd->src = pafc->aurl_wa;
		MMPreloadImage(pafc->win, mptr, pafc->mhs, pafc->fname);
		break;
	default:
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMFinishPafEmbeddedObject, Unknow EmbeddedObject type %d\n", mptr->type);
		fprintf(stderr, "Aborting ...\n");
		abort();
	}

	unlink(pafc->fname);
	ppaf->cur_processing_eo++;
	if ( ppaf->cur_processing_eo >= ppaf->num_of_eo) {
/* we have all embedded and doc */
/* get the current id  #### */
		id = 0;
/* get the goto_anchor */
		goto_anchor = NULL;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, id, ppaf->goto_anchor, ppaf->aurl);
                free(ppaf->embedded_object_tab);
		free(ppaf->paf_child); /*### et le reste de paf_child*/
/* en finir avec la requete de pafd. */
                win = ppaf->win;
/* stop the twirl */
                XtRemoveTimeOut(ppaf->twirl_struct->time_id);
                free(ppaf->twirl_struct);
                free(ppaf->sps.accept);
                close(ppaf->fd);   
                unlink(ppaf->fname);
                free(ppaf->fname);
                free(ppaf);
                win->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
		return;	/* it is really the end !!! */
	}

/* else loop once more time */
	MMPafLoadEmbeddedObjInDoc(ppaf->paf_child);
}

/* we are call for all object */
void MMPafLoadEmbeddedObjInDoc(PafDocDataStruct * pafc)
{
	PafDocDataStruct * pafd;
	char * req_url, *cur_url, *old_url, *new_url_with_a, *new_canon_url;
	char *fname;
	int fd;

	pafd= pafc->parent_paf ;

	req_url = pafd->embedded_object_tab[pafd->cur_processing_eo].url;
	cur_url = pafd->win->current_node->base_url;
        old_url = URLParse (cur_url, "",
                PARSE_ACCESS | PARSE_HOST | PARSE_PATH | PARSE_PUNCTUATION);
        new_url_with_a = URLParse (req_url, old_url,
                PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                PARSE_PUNCTUATION | PARSE_ANCHOR);
        free(old_url);
        new_canon_url =  URLParse (new_url_with_a, "",
                  PARSE_ACCESS | PARSE_HOST | PARSE_PATH | PARSE_PUNCTUATION);
	
	fname = tempnam (mMosaicTmpDir,"mMo"); /* temp file for work */
        fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
        if ( fd < 0) {                 
                XmxMakeErrorDialog(pafc->win->base, "Can't open temp file", "System Error");                                   
                return;
        } 
        pafc->fname = fname;           
        pafc->fd = fd;                 
        pafc->win = pafd->win;               

        pafc->aurl_wa = new_url_with_a;
        pafc->aurl = new_canon_url;    
        pafc->call_me_on_succes = MMFinishPafEmbeddedObject;
        pafc->call_me_on_error = MMErrorPafEmbeddedObject;
	pafc->call_me_on_stop = MMStopPafEmbeddedObject; /* a voir ### */
        pafc->sps.accept = strdup("*/*");
/* completer paf_child 	... */
	/* process embedded object */
/* look in cache first */
	if(! (pafc->pragma_no_cache || (pafc->post_ct && pafc->post_data)) ) {
		int found;
/* aurl_wa : to determine if url is cachable */
/* aurl : the reference to find in cache */
/* fd : the file where to write the data */
/* mhs: to build a mime struct like (return) */
/* voir MMCachePutDataInCache pour mettre les donnees tel qu'on les a recu */
		found = MMCacheFindData( pafc->aurl_wa, pafc->aurl,
			pafc->fd, pafc->mhs);
		if (found) {    /* we have a hit */
			pafc->http_status = 200; /* OK */
/* don't recache a cached data */
			pafc->mhs->cache_control = CACHE_CONTROL_NO_STORE;
			(*pafc->call_me_on_succes)(pafc);
			return;
		}       
	}
	if (pafd->win->delay_object_loads){
/* call the error routine... but set a delay indication bit */ 
		(*pafc->call_me_on_error)(pafc, "DelayedRequest");
		return;

	}
/* process request until all object is loaded */
/* it is a loop */
	PostRequestAndGetTypedData(new_canon_url,fname,pafd->paf_child);
	return;
}


/* qques lignes de code pour les news ##### 
 * if(!strncmp(url,"news:",4)) {       
 * int p,n,pt,nt,f;            
 * news_status(url,&pt,&nt,&p,&n,&f);
 * mo_tool_state(&(win->tools[BTN_PTHR]), pt?XmxSensitive:XmxNotSensitive,BTN_PTHR);
 * XmxRSetSensitive (win->menubar, (XtPointer)mo_news_prevt, pt?XmxSensitive:XmxNotSensitive);
 * mo_tool_state(&(win->tools[BTN_NTHR]), nt?XmxSensitive:XmxNotSensitive,BTN_NTHR);
 * XmxRSetSensitive (win->menubar, (XtPointer)mo_news_nextt, nt?XmxSensitive:XmxNotSensitive);
 * mo_tool_state(&(win->tools[BTN_PART]), p?XmxSensitive:XmxNotSensitive,BTN_PART);
 * XmxRSetSensitive (win->menubar, (XtPointer)mo_news_prev, p?XmxSensitive:XmxNotSensitive);
 * mo_tool_state(&(win->tools[BTN_NART]), n?XmxSensitive:XmxNotSensitive,BTN_NART);
 * XmxRSetSensitive (win->menubar, (XtPointer)mo_news_next, n?XmxSensitive:XmxNotSensitive);
 * mo_tool_state(&(win->tools[BTN_POST]),XmxSensitive,BTN_POST);
 * mo_tool_state(&(win->tools[BTN_FOLLOW]), f?XmxSensitive:XmxNotSensitive,BTN_FOLLOW);
 * XmxRSetSensitive(win->menubar, (XtPointer)mo_news_follow, f?XmxSensitive:XmxNotSensitive);
/* set the popup too */                     
/*mo_popup_set_something("Previous Thread", pt?XmxSensitive:XmxNotSensitive, NULL);
 * mo_popup_set_something("Next Thread", nt?XmxSensitive:XmxNotSensitive, NULL);
 * mo_popup_set_something("Previous Article", p?XmxSensitive:XmxNotSensitive, NULL);
 * mo_popup_set_something("Next Article", n?XmxSensitive:XmxNotSensitive, NULL);
 * mo_popup_set_something("Followup", f?XmxSensitive:XmxNotSensitive, NULL);
 * newmode = moMODE_NEWS;      
 * }
 */
/* If news: URL, then we need to auto-scroll to the >>> marker if it
 * is here. We use a hacked version of the searching function here
 * which will need to be updated when we rewrite. --SWP */
/*if (win->current_node && win->current_node->url && !strncmp(win->current_node->url,"news:",5)) {
 *mo_search_window(win,">>>",0,1,1);
 *}                         
*/
