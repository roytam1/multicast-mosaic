/* paf : post and forget.
 * intended to 'multithread' a data's load as an autonom widget.
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

static void MMPafLoadEmbeddedObjInDoc(PafDocDataStruct * paf_child);

#ifdef MULTICAST
static void McAddDepend( mo_window * win, int moid)
{
	if (win->n_do == 0){
		win->n_do++;
		win->dot = (int*) malloc(sizeof(int));
		win->dot[0] = moid;
		return;
	}
	win->n_do++;
	win->dot = (int*) realloc(win->dot, sizeof(int) * win->n_do);
	win->dot[win->n_do - 1] = moid;
}

static void McChangeDepend(mo_window *parent, mo_window *win, int index, int moid)
{
	if ( index < 0 || index >= parent->n_do)
		abort();	/* Let me know */

	parent->dot[index] = moid;
}
static void McAddDependFrame(mo_window * topw, int moid, int index)
{
	int ndot = topw->frame_sons_nbre;

	if (topw->n_do == 0){
		topw->n_do = ndot;
		topw->dot = (int*) malloc(sizeof(int) * ndot);
	}
	topw->dot[index] = moid;
}

#endif

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
	pafd->fd = -1;
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
	pafd->fd = -1;
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
	pafd->fd = -1;
	unlink(pafd->fname);
	free(pafd->fname);
	/* free(pafd->mhs);	### FIXME */
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
	XmxSetOffsets(win->dest_widget, 10,10,10,10);
	XmxSetConstraints(win->dest_widget,XmATTACH_WIDGET, XmATTACH_NONE,
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
	PostRequestAndGetTypedData(aurl, pafd);
}

/* -------------------- Loading HTML document in Window -----------------*/

/* down->up procedure */
void MMErrorPafDocData (PafDocDataStruct * pafd, char *reason)
{
	mo_window * win = pafd->win;

#ifdef MULTICAST
	if (mc_send_win && (mc_send_win == win || mc_send_win == win->frame_parent)) {/* A multicast send window */
		int moid_ret;

                if (win->frame_sons_nbre == 0) { /* frame or top html */
			(*mc_send_win->mc_callme_on_error_object)(pafd->aurl,
				pafd->http_status, &moid_ret ); /* set to Not Found */
                       win->moid_ref = moid_ret; /* info for state */
                } else {        /* a top frameset can't be in error */
                       abort(); /* impossible */
                }
		switch(pafd->gui_action) {
		case HTML_LOAD_CALLBACK:
			if (win->frame_parent != NULL) { /* in frameset*/
/* a html text change in a frame: do a new state */
				McChangeDepend(win->frame_parent, win,
					win->frame_dot_index, moid_ret);
				(*mc_send_win->mc_callme_on_new_state)(
					win->frame_parent,
					win->frame_parent->moid_ref,
					win->frame_parent->dot,
					win->frame_parent->n_do);
			} else if (win->frame_sons_nbre == 0) {
/* a html text by itself at top level , send error... */
					(*mc_send_win->mc_callme_on_new_state)(
						win, win->moid_ref, NULL, 0);
			} else {	/* impossible */
				abort();
                        } 
			break;
		case FRAME_CALLBACK:
			McAddDependFrame(win->frame_parent, moid_ret, win->frame_dot_index);
			win->frame_parent->number_of_frame_loaded++;
			if (win->frame_parent->frame_sons_nbre == win->frame_parent->number_of_frame_loaded ) {
/* late send state of frameset */
				PafDocDataStruct * pafd = win->frame_parent->pafd;
				mo_window *win = pafd->win;
				int moid_ret;

				(*mc_send_win->mc_callme_on_new_object)(
					pafd->mc_fname_save, pafd->aurl,
					pafd->mhs,  win->dot ,
					win->n_do, &moid_ret);
				win->moid_ref = moid_ret; /* info for state */
				free(pafd->mc_fname_save);
				(*mc_send_win->mc_callme_on_new_state)(
					win, win->moid_ref, win->dot, win->n_do);
					/* stop the twirl */
				XtRemoveTimeOut(pafd->twirl_struct->time_id);                                     
				free(pafd->twirl_struct);
				free(pafd->sps.accept);
				close(pafd->fd);
				pafd->fd = -1;
				unlink(pafd->fname);
				free(pafd->fname);
				free(pafd);
				win->pafd = NULL;
					/* securityType=HTAA_UNKNOWN; */
			}
			break;
		default:      
			abort();        /* let me know */
		}
	}
#endif
	XmxMakeErrorDialog(win->base, reason , "Net Error");
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
	pafd->fd = -1;
	unlink(pafd->fname);
	free(pafd->fname);
	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;

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
	pafd->fd = -1;
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
	char * presentation;
	char * data;
	struct mark_up *mlist, *mptr;
	int fd;
	int lread;
	int docid;
	char * title;
	char * base_url;
	char * base_target = NULL;
	int moid_ret = -1;
	HtmlTextInfo *htinfo;
	PafDocDataStruct spafd;

	win = pafd->win;

/* free the things we have build in MMPafDocData */
	close(pafd->fd);
	pafd->fd = -1;

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
			PostRequestAndGetTypedData(pafd->aurl,pafd);
			return;
		default:
			pafd->mhs->status_code = pafd->http_status;
			break;
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
/* don't cache a request we still found in cache */
	if( !(( pafd->mhs->cache_control & CACHE_CONTROL_NO_STORE) ||
	      (pafd->post_ct && pafd->post_data) || 
	      (pafd->http_status == HTTP_STATUS_INTERNAL_CACHE_HIT)) ) {
		MMCachePutDataInCache(pafd->fname, pafd->aurl_wa, pafd->aurl,
			pafd->mhs);
	}

#ifdef MULTICAST
	if (mc_send_win && ( mc_send_win == win || mc_send_win == win->frame_parent)) {/* A multicast send window */
#define LBUFFER (8092)
		int i;
		char buffer[LBUFFER];
		int mcfd, fdsrc;
/* prevenir qu'on a de nouvelles donnees */
/* sauver le fichier original qq part et son MIME pour le multicaster +tard */
/* mhs a besoin d'un complement. Attribue un MOID +tard */
		pafd->mc_fname_save = tempnam (mMosaicTmpDir,"mMo"); /* temp file for work */
		mcfd = open(pafd->mc_fname_save, O_WRONLY | O_CREAT | O_TRUNC, 0644);
		fdsrc = open(pafd->fname, O_RDONLY);
		while( (i = read(fdsrc,buffer,LBUFFER)) >0)
			write(mcfd,buffer,i);
		close(fdsrc);
		close(mcfd);
#undef LBUFFER
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
		XmxAdjustLabelText(win->tracker_widget,"Decompressing data.");
		XFlush(mMosaicDisplay);
/* take care: content of file change - decompress- */
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

/* winfried propal is: if (strstr(presentation, "%%s")){ */
/* but dont' understand why ##### 28/02/2000 */
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
		pafd->fd = -1;
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
	htinfo = HTMLParseRepair(data);
	pafd->mlist = mlist = htinfo->mlist;
/* detecter les frames et dans ce cas ajouter un niveau d'indirection */

/* Faire une liste des embedded object */
/* initialiser et seulement initialiser ces objects */

	mptr = mlist;
	pafd->num_of_eo = 0;
	base_url = htinfo->base_url;
	eo_tab = (EmbeddedObjectEntry *) malloc(sizeof(EmbeddedObjectEntry));
			/* alloc one */
/* recuperer le reste des embedded objects (si necessaire)*/
/* relancer la bete pour les embedded objets si il y en a */
	if (!htinfo->title)
		title = strdup(pafd->aurl_wa);
	else
		title = htinfo->title;
	while (mptr != NULL){
		char * tptr;
		ImageInfo * picd;

/* if in_title, grab the text until end marker */
		switch (mptr->type){
		case M_BODY:		/* BODY can have image... */
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageBody(win, picd, mptr);
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
			MMPreParseInputTagImage(win, picd, mptr);
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
			MMPreParseImageTag(win, picd, mptr);
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
		default:
			break;
		}
		mptr = mptr->next;
	}

/* ##### docid = HTMLGetDocId(win->scrolled_win); */
	docid =0;
/* on met a jour immediatement la partie navigation. Car on doit avoir un 
 * current_node qui memorise tout la requete 
 * title is alway allocated.
 */
	MMUpdNavigationOnNewURL(win, pafd->aurl_wa, pafd->aurl, pafd->goto_anchor, base_url,
		base_target, title,
		pafd->html_text, pafd->mhs, docid, mlist); 
/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */

/* afficher le texte. le mettre dans la Widget. id come from back and forward */
	docid = 0;			/* we are not in back or forward */
	spafd = *pafd;
	HTMLSetHTMLmark (win->scrolled_win, mlist, docid, pafd->goto_anchor,
		pafd->aurl);
	XFlush(XtDisplay(win->scrolled_win));

	mo_set_win_headers(win, spafd.aurl_wa);

/* MAJ de l'history etc... */
	MMUpdateGlobalHistory(spafd.aurl);

	if (spafd.num_of_eo == 0 ){	/* no embedded object it is the end */
#ifdef MULTICAST
		if (mc_send_win && ( mc_send_win == win || mc_send_win == win->frame_parent)) {/* A multicast send window */
			if (win->frame_sons_nbre == 0) { /* frame or top html */
				(*mc_send_win->mc_callme_on_new_object)(
					pafd->mc_fname_save, pafd->aurl,
			 		pafd->mhs,  win->dot ,
					win->n_do, &moid_ret);
				win->moid_ref = moid_ret; /* info for state */
				free(pafd->mc_fname_save);
			} else {	/* a top frameset */
				return; /* let pafd alive */
			  /* the announce of top frameset object is */
			  /* done at later time. When a frame */
			  /* child will be loaded and */
			  /* all child is loaded */
			  /* save some data to be send later */
			}
/* this is done in mo_window */
/*### gui_action: indique pour un frame si on clique dedans ou si
 *### on charge un frameset...
 *### je dois dectecter si ca vient de l'init d'un frameset
 *### ou si ca vient de ce que je clique dans une href d'un frame
 *### soit de framecallback ou de anchor callback
 *### dans le cas href d'un frame, utiliser frame_dot_index...
*/
			switch(pafd->gui_action) {
			case HTML_LOAD_CALLBACK:
				if (win->frame_parent != NULL) { /* click in frame*/
/* a html text change in a frame: do a new state */
					McChangeDepend(win->frame_parent, win,
						win->frame_dot_index, moid_ret);
					(*mc_send_win->mc_callme_on_new_state)(
						win->frame_parent,
						win->frame_parent->moid_ref,
						win->frame_parent->dot,
						win->frame_parent->n_do);
				} else if (win->frame_sons_nbre == 0) {
/* a html text by itself at top level */
						(*mc_send_win->mc_callme_on_new_state)(
							win, win->moid_ref, NULL, 0);
				}
				break;
			case FRAME_CALLBACK:
				McAddDependFrame(win->frame_parent, moid_ret, win->frame_dot_index);
				win->frame_parent->number_of_frame_loaded++;
				if (win->frame_parent->frame_sons_nbre == win->frame_parent->number_of_frame_loaded ) {
/* late send state of frameset */
					PafDocDataStruct * pafd = win->frame_parent->pafd;
					mo_window *win = pafd->win;
					int moid_ret;

					(*mc_send_win->mc_callme_on_new_object)(
						pafd->mc_fname_save, pafd->aurl,
						pafd->mhs,  win->dot ,
						win->n_do, &moid_ret);
                               		win->moid_ref = moid_ret; /* info for state */
                               		free(pafd->mc_fname_save);
					(*mc_send_win->mc_callme_on_new_state)(
						win, win->moid_ref, win->dot, win->n_do);
						/* stop the twirl */
					XtRemoveTimeOut(pafd->twirl_struct->time_id);
					free(pafd->twirl_struct);
					free(pafd->sps.accept);
					close(pafd->fd);
					pafd->fd = -1;
					unlink(pafd->fname); 
					free(pafd->fname);
					free(pafd);
					win->pafd = NULL;
						/* securityType=HTAA_UNKNOWN; */
				}
				break;
			default:
				abort();	/* let me know */
			}
		}
#endif
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
		pafd->fd = -1;
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
/* Every time we view the document, we reset the search_start */
/* struct so searches will start at the beginning of the document. */
        ((ElementRef *)win->search_start)->id = 0;
        win->src_search_pos = 0;
	return;
/* on doit avoir un return juste apres MMPafLoadEmbeddedObjInDoc */
/* la requete est complete pour la partie html du docu. */
/* On ne passe plus par ici . C'est FINI pour l'unicast et le multicast*/
/* le prochain appelle est, si tout est correct, MMFinishEmbedded... */
}


/* #########################################*/
/*if(win->links_win) /* vvv HREF ListBox Stuff -- BJS 10/2/95 */ 
/*mo_update_links_window(win);
/* some gui work */
/*if (win->current_node)              
/*mo_gui_check_security_icon_in_win(win->current_node->authType,win);
/*  mo_gui_done_with_icon (win);
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
	GuiActionType gui_action;

	opafd = win->pafd;	/* Is a previous load in progress */
	if (opafd) {		/* if YES stop the previous request */
		MMStopPafDocData(opafd);
	}
#ifdef MULTICAST
/* reset old depend object */
	if (win->dot){
		/*free(win->dot);*/ /* don't free old. Used in multicast cache */
		win->dot =NULL;
		win->n_do =0;
	}
#endif
	reloading = rds->is_reloading;
	req_url = rds->req_url;
	gui_action = rds->gui_action;

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
	pafd->gui_action = gui_action;
	pafd->mhs = (MimeHeaderStruct*) malloc( sizeof(MimeHeaderStruct));
	pafd->mhs->cache_control = CACHE_CONTROL_NONE;
	pafd->www_con_type = NULL;
	pafd->fname = fname;
	pafd->fd = fd;
	pafd->aurl_wa = new_url_with_a;
	pafd->aurl = new_canon_url;
	pafd->goto_anchor = URLParse (pafd->aurl_wa,"", PARSE_ANCHOR);
	pafd->http_status = 0;
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

	PostRequestAndGetTypedData(new_canon_url,pafd);
}

/* ################################ */
/* -------------------- Loading Embeded object in document -----------------*/
void MMStopPafEmbeddedObject(PafDocDataStruct * pafc)
{
	PafDocDataStruct * ppaf;
	ppaf = pafc->parent_paf;

	if ( pafc->www_con_type && pafc->www_con_type->call_me_on_stop_cb) {
		(*pafc->www_con_type->call_me_on_stop_cb)(pafc);
	}
	pafc->www_con_type = NULL;

	close(pafc->fd);
	unlink(pafc->fname);
	free(pafc->sps.accept);
	free(pafc->aurl);
	free(pafc->aurl_wa);
	free(pafc->fname);
	/*free(pafc->mhs);	### FIXME */
	free(pafc);
	ppaf->paf_child = NULL;
}

void MMErrorPafEmbeddedObject (PafDocDataStruct * pafc, char *reason)
{
	PafDocDataStruct * ppaf;
	struct mark_up * mptr;
	mo_window * win;
	int elem_id;
	int delayed;
	int moid_ret= -1;

	if ( !strcmp(reason, "DelayedRequest") ){
		delayed = True;
	} else {
		delayed = False;
	}
/* XmxMakeErrorDialog(pafc->win->base, reason, "Net Error"); */
	close(pafc->fd);
	unlink(pafc->fname);
	ppaf = pafc->parent_paf;
	mptr = ppaf->embedded_object_tab[ppaf->cur_processing_eo].mark;
#ifdef MULTICAST
	if (mc_send_win && (mc_send_win == ppaf->win || mc_send_win == ppaf->win->frame_parent)) {/* A multicast send window */
/* prevenir qu'on a une erreur */
		(*mc_send_win->mc_callme_on_error_object)(pafc->aurl,
			pafc->http_status, &moid_ret ); /* set to Not Found */
/* we add a depend object to father */
		McAddDepend(ppaf->win, moid_ret);
	}
#endif
	switch(mptr->type){
	case M_BODY:
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
#ifdef MULTICAST
/* reprendre le fichier sauver plus haut.Faire les dependances des objets */
/* pour cet objet chaque objet a un MOID.Voir si cet objet decrit un */
/* parfaitement un etat : win->frame_parent == NULL ??? */
		if (mc_send_win && (mc_send_win == ppaf->win || mc_send_win == ppaf->win->frame_parent)) {/* A multicast send window */

                        if (ppaf->win->frame_sons_nbre == 0) { /* frame or top html */
                                (*mc_send_win->mc_callme_on_new_object)(
                                        ppaf->mc_fname_save, ppaf->aurl,
                                        ppaf->mhs,  ppaf->win->dot ,
                                        ppaf->win->n_do, &moid_ret);
                                ppaf->win->moid_ref = moid_ret; /* info for state */
                                free(ppaf->mc_fname_save);
                        } else {        /* a top frameset */
                                if (ppaf->win->number_of_frame_loaded == ppaf->win->frame_sons_nbre) {
                                        (*mc_send_win->mc_callme_on_new_object)(
                                                ppaf->mc_fname_save, ppaf->aurl,
                                                ppaf->mhs,  ppaf->win->dot ,
                                        
                                                ppaf->win->n_do, &moid_ret);
                                        ppaf->win->moid_ref = moid_ret; /* info
for state */
                                        free(ppaf->mc_fname_save);
                                } else {
                                        abort(); /* nothing . make it later */
                                }
                        }
			switch(ppaf->gui_action) {
			case HTML_LOAD_CALLBACK:
				if (ppaf->win->frame_parent != NULL) { /* in frameset*/                              
/* a html text change in a frame: do a new state */
					McChangeDepend(ppaf->win->frame_parent, ppaf->win,
						ppaf->win->frame_dot_index, moid_ret);
					(*mc_send_win->mc_callme_on_new_state)(
						ppaf->win->frame_parent,
						ppaf->win->frame_parent->moid_ref,
						ppaf->win->frame_parent->dot,
						ppaf->win->frame_parent->n_do);
				} else {
					if (ppaf->win->frame_sons_nbre == 0)
/* a html text by itself at top level */
						(*mc_send_win->mc_callme_on_new_state)(
							ppaf->win, ppaf->win->moid_ref, NULL, 0);
					else if (ppaf->win->frame_sons_nbre == ppaf->win->number_of_frame_loaded) /* The top frameset */
						(*mc_send_win->mc_callme_on_new_state)(
							ppaf->win, ppaf->win->moid_ref, ppaf->win->dot, ppaf->win->n_do);
					else
						abort(); /* break; */
				}    
				break;
			case FRAME_CALLBACK:
				McAddDepend(ppaf->win->frame_parent, moid_ret);
                                ppaf->win->frame_parent->number_of_frame_loaded++;
				if (ppaf->win->frame_parent->frame_sons_nbre == ppaf->win->frame_parent->number_of_frame_loaded) {
					int lfs_moid_ret;
					abort();
/*	(*mc_send_win->mc_callme_on_new_object)(
/*		ppaf->win->frame_parent->lfs_mc_fname_save,
/*		ppaf->win->frame_parent->lfs_aurl,
/*		ppaf->win->frame_parent->lfs_mhs,
/*		ppaf->win->frame_parent->dot ,
/*		ppaf->win->frame_parent->n_do,
/*		&lfs_moid_ret);
/*     		ppaf->win->frame_parent->moid_ref = lfs_moid_ret; /* info for state */
/*	unlink(ppaf->win->frame_parent->lfs_mc_fname_save);
/*     		free(ppaf->win->frame_parent->lfs_mc_fname_save);
/*	free(ppaf->win->frame_parent->lfs_aurl);
/*	free(ppaf->win->frame_parent->lfs_mhs);
/*	(*mc_send_win->mc_callme_on_new_state)(
/*		ppaf->win->frame_parent,
/*		ppaf->win->frame_parent->moid_ref,
/*		ppaf->win->frame_parent->dot,
/*		ppaf->win->frame_parent->n_do); */
				}
				break;
			default:     
				abort();        /* let me know */
			}
		}
#endif
/* we have all embedded and doc */
/* get the current id  #### */
		elem_id = 0;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, elem_id, ppaf->goto_anchor, ppaf->aurl);
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
	PafDocDataStruct * ppaf;
	struct mark_up * mptr;
	mo_window * win;
	int elem_id;
	int moid_ret = -1;

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
/* don't cache a request we still found in cache */
	if( !(( pafc->mhs->cache_control & CACHE_CONTROL_NO_STORE) ||
	      (pafc->post_ct && pafc->post_data) ||
	      (pafc->http_status == HTTP_STATUS_INTERNAL_CACHE_HIT)) ) {
		MMCachePutDataInCache(pafc->fname, pafc->aurl_wa, pafc->aurl,
			pafc->mhs);
	}

#ifdef MULTICAST
	if (mc_send_win && (mc_send_win == ppaf->win || mc_send_win == ppaf->win->frame_parent)) {/* A multicast send window */
/* prevenir qu'on a de nouvelles donnees */
/* cet objet n'a pas de dependance (objet atomique)  */
/* il ne decrit pas un etat */
/* lui attribue un MOID  si il n'en a pas deja un dans le cache multicast */
/* sinon le mettre dans le cache multicast avec son MOID */
/* et son entete mhs(multicast) dans un fichier separe. */
/* il faut faire ceci avant la decompression ou transformation du fichier
/* pafc->fname */
		(*mc_send_win->mc_callme_on_new_object)(pafc->fname,
			pafc->aurl, pafc->mhs,
			NULL, 0, &moid_ret);
/* this is done in mo_window */
		McAddDepend(ppaf->win, moid_ret);
	}
#endif

	if (pafc->mhs->content_encoding == COMPRESS_ENCODING ||
	    pafc->mhs->content_encoding == GZIP_ENCODING ){
			/* decompresser le fichier */
		XmxAdjustLabelText(pafc->win->tracker_widget,"Decompressing data.");      
		XFlush(mMosaicDisplay);
		DeCompress(pafc->fname, pafc->mhs);
		pafc->mhs->content_encoding = NO_ENCODING;
	}

	switch(mptr->type){
	case M_BODY:
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

#ifdef MULTICAST
/* reprendre le fichier sauver plus haut.Faire les dependances des objets */
/* pour cet objet chaque objet a un MOID.Voir si cet objet decrit un */
/* parfaitement un etat : win->frame_parent == NULL ??? */
		if (mc_send_win && (mc_send_win == ppaf->win || mc_send_win == ppaf->win->frame_parent)) {/* A multicast send window */

                        if (ppaf->win->frame_sons_nbre == 0) { /* frame or top html */
                                (*mc_send_win->mc_callme_on_new_object)(
                                        ppaf->mc_fname_save, ppaf->aurl,
                                        ppaf->mhs,  ppaf->win->dot ,
                                        ppaf->win->n_do, &moid_ret);
                                ppaf->win->moid_ref = moid_ret; /* info for state
*/
                                free(ppaf->mc_fname_save);
                        } else {        /* a top frameset */
				/* a top frameset never have embedded obj*/
                              abort();
                        }
			switch(ppaf->gui_action) {
                        case HTML_LOAD_CALLBACK:
                                if (ppaf->win->frame_parent != NULL) { 
/* a html text change in a frame: do a new state */
                                        McChangeDepend(ppaf->win->frame_parent, ppaf->win,
                                                ppaf->win->frame_dot_index, moid_ret);
                                        (*mc_send_win->mc_callme_on_new_state)(
                                                ppaf->win->frame_parent,
                                                ppaf->win->frame_parent->moid_ref,
                                                ppaf->win->frame_parent->dot,
                                                ppaf->win->frame_parent->n_do);
                                } else if (ppaf->win->frame_sons_nbre == 0) {
/* a html text by itself at top level */
					(*mc_send_win->mc_callme_on_new_state)(
						ppaf->win, ppaf->win->moid_ref, NULL, 0);
                                }     
                                break;
                        case FRAME_CALLBACK:
                                McAddDependFrame(ppaf->win->frame_parent, moid_ret, ppaf->win->frame_dot_index);
				ppaf->win->frame_parent->number_of_frame_loaded++;
				if (ppaf->win->frame_parent->frame_sons_nbre == ppaf->win->frame_parent->number_of_frame_loaded) {
					PafDocDataStruct * pafd = ppaf->win->frame_parent->pafd;                               
					mo_window *win = pafd->win;
					int moid_ret;

					(*mc_send_win->mc_callme_on_new_object)(
						pafd->mc_fname_save, pafd->aurl,
						pafd->mhs,  win->dot ,
						win->n_do, &moid_ret);
					win->moid_ref = moid_ret; /* info for state */                                   
					free(pafd->mc_fname_save);
					(*mc_send_win->mc_callme_on_new_state)(
						win, win->moid_ref, win->dot, win->n_do);
						/* stop the twirl */
					XtRemoveTimeOut(pafd->twirl_struct->time_id);
					free(pafd->twirl_struct);
					free(pafd->sps.accept);
					close(pafd->fd);
					pafd->fd = -1;
					unlink(pafd->fname); 
					free(pafd->fname);
					free(pafd);
					win->pafd = NULL;
						/* securityType=HTAA_UNKNOWN; */
				}
                                break;
                        default:      
                                abort();        /* let me know */
                        }
		}
#endif
/* we have all embedded and doc */
/* get the current id  #### */
		elem_id = 0;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, elem_id, ppaf->goto_anchor, ppaf->aurl);
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
static void MMPafLoadEmbeddedObjInDoc(PafDocDataStruct * pafc)
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

	if (pafd->win->delay_object_loads){
/* call the error routine... but set a delay indication bit */ 
		(*pafc->call_me_on_error)(pafc, "DelayedRequest");
		return;

	}
/* process embedded object */
/* process request until all object is loaded */
/* it is a loop */
	PostRequestAndGetTypedData(new_canon_url,pafd->paf_child);
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
