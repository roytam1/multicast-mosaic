/* paf : post and forget.
 * intended to 'multithread' a data's load as an autonom widget.
 */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <assert.h>

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

/* #define DEBUG_STOP 1 */

static void MMPafLoadEmbeddedObjInDoc(PafDocDataStruct * paf_child);
static void LateEndPafDoc(PafDocDataStruct * pafd);

static void MMStopTwirl(PafDocDataStruct * pafd)
{
	mo_window * win = pafd->win;

	if (win->frame_type == FRAME_TYPE) 
		return;

	assert(win->frame_type == FRAMESET_TYPE || win->frame_type == NOTFRAME_TYPE);
	XtRemoveTimeOut(pafd->twirl_struct->time_id);
	free(pafd->twirl_struct);
	pafd->twirl_struct = NULL;
}

/* Winfried */
static void FreePafDocDataStruct(PafDocDataStruct * pafd)
{
	assert(pafd->twirl_struct == NULL);
	assert(pafd->www_con_type == NULL);

/* Must be freed in navigation */
	assert(pafd->mhs == NULL);
	assert(pafd->aurl == NULL);
	assert(pafd->aurl_wa == NULL);

	if(pafd->goto_anchor) 	free(pafd->goto_anchor);
	if(pafd->post_ct) 	free(pafd->post_ct);
	if(pafd->post_data) 	free(pafd->post_data);
	if(pafd->html_text) 	free(pafd->html_text);
	if(pafd->embedded_object_tab) free(pafd->embedded_object_tab);
	if(pafd->format_in) 	free(pafd->format_in);
	if(pafd->sps.accept) 	free(pafd->sps.accept);

	pafd->proxent = NULL;

	if(pafd->iobs.iobuf) 	free(pafd->iobs.iobuf);
/* un frameset peut ou pas avoir de fname suivant comment on charge les frames */
	if(pafd->fname) {
		unlink(pafd->fname);
		free(pafd->fname);
	}
	memset(pafd,0,sizeof(PafDocDataStruct));     /* sanity */
	free(pafd);
}

#ifdef MULTICAST
static void McAddDepend( mo_window * win, int moid)
{
	if (win->n_do == 0){
		win->n_do++;
		win->dot = (int*) calloc(1,sizeof(int));
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
		assert(0);	/* Let me know */

	parent->dot[index] = moid;
}
static void McAddDependFrame(mo_window * topw, int moid, int index)
{
	int ndot = topw->frame_sons_nbre;

	if (topw->n_do == 0){
		topw->n_do = ndot;
		topw->dot = (int*) calloc(ndot, sizeof(int) );
	}
	topw->dot[index] = moid;
}

#endif

static XmxCallback (icon_pressed_save_cb)
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
	assert(pafd->fd >=0 ); 		/* let me know */
	close(pafd->fd);
	pafd->fd = -1;

/* stop the twirl */                  
	MMStopTwirl(pafd);

	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;	/* sanity */

	free(pafd->aurl);
	pafd->aurl = NULL;

	free(pafd->aurl_wa);
	pafd->aurl_wa = NULL;

	free(pafd->fname);
	pafd->fname = NULL;

	FreePafDocDataStruct(pafd);
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
	assert(pafd->fd >=0 ); 		/* let me know */
	close(pafd->fd);
	pafd->fd = -1;

/* stop the twirl */
        MMStopTwirl(pafd);

	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;	/* sanity */

	free(pafd->aurl);
	pafd->aurl = NULL;

	free(pafd->aurl_wa);
	pafd->aurl_wa = NULL;

	free(pafd->fname);
	pafd->fname = NULL;

	FreePafDocDataStruct(pafd);
	win->pafd = NULL;
	XtPopdown(win->base);
	XtDestroyWidget(win->base);
	free(win);
}

void MMStopPafSaveData(PafDocDataStruct * pafd)
{
	mo_window * win = pafd->win;

	MMStopTwirl(pafd);
	if (pafd->www_con_type && pafd->www_con_type->call_me_on_stop_cb ) {
                (*pafd->www_con_type->call_me_on_stop_cb)(pafd);
        }
	
	assert(pafd->fd >= 0); 	/* let me know */

	close(pafd->fd);
	pafd->fd = -1;
	unlink(pafd->fname);

	FreeMimeStruct(pafd->mhs);
	pafd->mhs = NULL;	/* sanity */

	free(pafd->aurl);
	pafd->aurl = NULL;

	free(pafd->aurl_wa);
	pafd->aurl_wa = NULL;

	free(pafd->fname);
	pafd->fname = NULL;

	FreePafDocDataStruct(pafd);
	win->pafd = NULL;

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
	win = (mo_window *) calloc(1,sizeof(mo_window));
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

/*	XmxSetArg (XmNheight,207); */
/*	XmxSetArg (XmNwidth,400); */

	win->base = XtCreatePopupShell("mMosaic: Load to disk",
		topLevelShellWidgetClass, top, Xmx_wargs, Xmx_n);
	win->frame_type = NOTFRAME_TYPE;
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
	win->logo = XmxMakePushButton( form, "Stop", icon_pressed_save_cb, (XtPointer)win);
	XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);

/* update pafd */
	pafd->www_con_type = NULL;

	pafd->call_me_on_succes = MMFinishPafSaveData;
	pafd->call_me_on_error = MMErrorPafSaveData;
	pafd->call_me_on_stop = MMStopPafSaveData;
	pafd->sps.accept = strdup("*/*");

	pafd->pragma_no_cache = False;

	pafd->twirl_struct = (TwirlStruct*) calloc(1,sizeof(TwirlStruct));
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
					XmATTACH_FORM, XmATTACH_FORM,
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
	mo_window * winset;
	int moid_ret;

	assert(pafd->fd >= 0);
	close(pafd->fd);
	pafd->fd = -1;
	unlink(pafd->fname);

	XmxMakeErrorDialog(win->base, reason , "Net Error");

/* en cas d'erreur sur une page on n'envoie rien en multicast */
	if (win->frame_type == FRAMESET_TYPE ) {
		MMStopTwirl(pafd);
		pafd->aurl =NULL; pafd->aurl_wa = NULL; pafd->mhs = NULL; /* sanity */
		FreePafDocDataStruct(pafd);
		win->pafd = NULL;
		return;
	}
	assert(win->frame_type != FRAMESET_TYPE);
	if (win->frame_type == NOTFRAME_TYPE ) {
		MMStopTwirl(pafd);
		pafd->aurl =NULL; pafd->aurl_wa = NULL; pafd->mhs = NULL; /* sanity */
		FreePafDocDataStruct(pafd);
		win->pafd = NULL;
		return;
	}
	assert(win->frame_type != NOTFRAME_TYPE);

/* Traitement du type FRAME_TYPE */
/* en cas d'erreur sur un frame, il faut envoyer un substitue a ce Frame */

#ifdef MULTICAST
	if (mc_send_win && (mc_send_win == win || mc_send_win == win->frame_parent)) {/* A multicast send window */
		(*mc_send_win->mc_callme_on_error_object)(pafd->aurl,
				pafd->http_status, &moid_ret ); /* set to Not Found */
                win->moid_ref = moid_ret; /* info for state */
		free(pafd->mc_fname_save);
		pafd->mc_fname_save = NULL;     /* sanity */

		if (win->frame_type == NOTFRAME_TYPE) { /* top html */
			(*mc_send_win->mc_callme_on_new_state)( win,
				win->moid_ref, NULL, 0);
/*win->dot, win->n_do); ### pas de dependance d'etat pour HTML ou FRAME */
		}
	}
#endif


	winset = win->frame_parent; /* the frameset */
	winset->number_of_frame_loaded++; /* one more is loaded */
	assert(winset->number_of_frame_loaded <=winset->frame_sons_nbre);

#ifdef MULTICAST
	if(mc_send_win && (mc_send_win==win || mc_send_win==win->frame_parent)){
		switch(pafd->gui_action) {
		case HTML_LOAD_CALLBACK:
			McChangeDepend(winset, win, win->frame_dot_index, moid_ret);
			break;
		case FRAME_LOADED_FROM_FRAMESET:
			McAddDependFrame(winset,moid_ret, win->frame_dot_index);
			break;
		}
	}
#endif

	if(winset->number_of_frame_loaded == winset->frame_sons_nbre ) {
					/* Is it the last ? */
		PafDocDataStruct * fs_pafd = winset->pafd;
		assert(fs_pafd->fd == -1 );
		MMStopTwirl(fs_pafd); /* en finir avec le frameset */
#ifdef MULTICAST
		if (mc_send_win && (mc_send_win==win || mc_send_win==win->frame_parent)){
			int fs_moid_ret;

			if (pafd->gui_action == FRAME_LOADED_FROM_FRAMESET) {
                                (*mc_send_win->mc_callme_on_new_object)(
                                        fs_pafd->mc_fname_save,
                                        fs_pafd->aurl, fs_pafd->mhs,
/* pas de dependance pour les framesets */
/* winset->dot, winset->n_do, */	NULL, 0, &fs_moid_ret);
                                winset->moid_ref=fs_moid_ret; /* info for state
*/
                                free(fs_pafd->mc_fname_save);
                                fs_pafd->mc_fname_save = NULL; /* sanity */
                        }
			(*mc_send_win->mc_callme_on_new_state)(
				winset, winset->moid_ref,
				winset->dot, winset->n_do);
		}
#endif
/* free parent frameset */            
/* Must be freed in navigation */     
                fs_pafd->aurl = fs_pafd->aurl_wa = NULL;
                fs_pafd->mhs = NULL;                    /*sanity*/
                FreePafDocDataStruct(fs_pafd);
                winset->pafd = NULL;  
        }
	pafd->aurl =NULL; pafd->aurl_wa = NULL; pafd->mhs = NULL; /* sanity */
	FreePafDocDataStruct(pafd);
	win->pafd = NULL;
}

/* top-> down procedure */
void MMStopPafDocData(PafDocDataStruct * pafd)
{
	mo_window * win = pafd->win;

/* stop the twirl */

        if (win->frame_type != FRAME_TYPE){	/* frameset or html */
                MMStopTwirl(win->pafd);     
        }

	if (win->frame_type == FRAMESET_TYPE) { /* stop frame sons */
		int i;

		for(i = 0; i< win->frame_sons_nbre; i++) {
			mo_window *winf = win->frame_sons[i];

			if(winf->pafd) {
				(*winf->pafd->call_me_on_stop)(winf->pafd);
				winf->pafd = NULL;
			}
		}
	}
/* if win have a pafd and is stopped, considere it as loaded */
	if (win->frame_type == FRAME_TYPE) {
		win->frame_parent->number_of_frame_loaded++;
	}
	if (pafd->paf_child) { /* a embedded object in progress */
		(*pafd->paf_child->call_me_on_stop)(pafd->paf_child);
	}
	pafd->paf_child =NULL;

/* if a http connection is in progress for html page, abort it */
	if (pafd->www_con_type /*&& pafd->www_con_type->call_me_on_stop_cb*/ ) {
		(*pafd->www_con_type->call_me_on_stop_cb)(pafd);
		assert( pafd->fd > 0);	/* finnish doc : fd is open*/
					/* else let me know */
		close(pafd->fd);
		pafd->fd = -1;
	}

	assert( pafd->fd == -1 ) ;	/* finnish doc : fd is closed*/


#ifdef DEBUG_STOP
	if (pafd->aurl_wa)
		printf("MMStopPafDocData: %s must be stooped\n", pafd->aurl_wa);
#endif
	pafd->aurl =NULL; pafd->aurl_wa = NULL; pafd->mhs = NULL; /* sanity */
	FreePafDocDataStruct(pafd);
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
	struct mark_up *old_mlist;
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
	assert(pafd->fd > 0);

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
				(*pafd->call_me_on_error)(pafd,"to many redirects");
				return;
			}
			close(pafd->fd);
			pafd->fd = -1;

			unlink(pafd->fname);
			pafd->fd = open(pafd->fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
			{
				char * new_url;
				new_url = mo_url_canonicalize_keep_anchor(pafd->mhs->location,pafd->aurl_wa);

				if(pafd->aurl_wa) free(pafd->aurl_wa);
				pafd->aurl_wa = new_url;
				new_url = mo_url_canonicalize(pafd->mhs->location, pafd->aurl);
				if(pafd->aurl) free(pafd->aurl);
				pafd->aurl = new_url;
			}
			FreeMimeStruct(pafd->mhs);
			pafd->mhs=(MimeHeaderStruct*) calloc(1,sizeof(MimeHeaderStruct));

			pafd->pragma_no_cache = True;
/* And redo a request for redirect */
			PostRequestAndGetTypedData(pafd->aurl,pafd);
			return;
		default:
			pafd->mhs->status_code = pafd->http_status;
			break;
		} 
	}
	close(pafd->fd);
	pafd->fd = -1;


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
		assert(0);
		/* envoyer un message d'erreur */
	}

	if ( strcmp(presentation, MMOSAIC_PRESENT) ){
/* la presentation n'est pas pour mMosaic : envoie la commande... */
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
/* on est la parceque :
 *	- soit on a clicque sur une url dans une page html
 *	- soit un src d'un frameset y fait reference
 *	- soit on a clique dans un frame sur une url
 */
/* target is maybe FRAMESET_TYPE or NOTFRAME_TYPE */
		if (win->frame_type == NOTFRAME_TYPE || win->frame_type== FRAMESET_TYPE)
			MMStopTwirl(pafd);
		assert(pafd->fd == -1);
		pafd->aurl = NULL; pafd->aurl_wa = NULL; pafd->mhs = NULL;
		FreePafDocDataStruct(pafd);
		win->pafd = NULL;      

/* traite le cas d'un frame */
		if (win->frame_type == FRAME_TYPE){ /* guess end of frameset? */
						/* if yes, stop frameset... */
			mo_window * winset = win->frame_parent;

			winset->number_of_frame_loaded++; /* one more is (external) loaded */
			assert(winset->number_of_frame_loaded <=winset->frame_sons_nbre);
			if(winset->number_of_frame_loaded == winset->frame_sons_nbre ) {
					/* Is it the last ? */
				MMStopTwirl(winset->pafd); /* finir avec le frameset */

				assert( winset->pafd->fd == -1 );/* finnish doc: fd is closed*/
				winset->pafd->aurl = NULL; winset->pafd->aurl_wa = NULL; winset->pafd->mhs = NULL;
				FreePafDocDataStruct(winset->pafd);
				winset->pafd = NULL;
/* securityType=HTAA_UNKNOWN; */
				return; /* it is finished!!! */
			}
		}
/* securityType=HTAA_UNKNOWN; */       
		return; /* it is finished!!! */
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
		assert(0);
	}
	lread = read(fd, data, pafd->mhs->content_length);
	if (lread != pafd->mhs->content_length) {
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMFinishPafDocData, Bug in reading data, lread = %d, content_length = %d\n", lread, pafd->mhs->content_length);
		fprintf(stderr, "Aborting ...\n");
		assert(0);
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

	if (win->frame_type ==  FRAMESET_TYPE) { /* dewrap old */
		int i;

#ifdef MULTICAST
        	win->mc_sbh_value = 0; 
        	win->mc_sbv_value = 0;
		win->mc_send_scrollbar_flag = False;
#endif
		for(i = 0; i < win->frame_sons_nbre; i++) {
/* stop old plugins if exist */
#ifdef OBJECT
			if (win->frame_sons[i]->htinfo) {
				MMStopPlugins(win->frame_sons[i],
					win->frame_sons[i]->htinfo->mlist);
			}
#endif
			MMDestroySubWindow(win->frame_sons[i]);
			win->frame_sons[i] = NULL; /* sanity */
		}
		free(win->frame_sons);
		win->frame_sons = NULL;
		HTMLUnsetFrameSet (win->scrolled_win);
/* ###################################################################### */
/*		FreeHtmlTextInfo(win->htinfo); don't free here but in nav. */
/* ###################################################################### */

		win->frame_type = NOTFRAME_TYPE;
	}

/* stop old plugins if exist */
#ifdef OBJECT
	if (win->htinfo) {
		old_mlist = win->htinfo->mlist;
		MMStopPlugins(win, old_mlist);
	}
#endif
#ifdef MULTICAST
        win->mc_sbh_value = 0; 
        win->mc_sbv_value = 0;
	win->mc_send_scrollbar_flag = False;
#endif
/* dans le fichier decompresse (eventuellement) on a de l'HTML */
/* Faire un Parse pour le decomposer en objet */
	pafd->html_text = data;
	htinfo = HTMLParseRepair(data);

	pafd->mlist = mlist = htinfo->mlist;
	mptr = mlist;
	pafd->num_of_eo = 0;
	if(!htinfo->base_url) {	/* if no base_url, get default */
		htinfo->base_url = strdup(pafd->aurl);
	}
	base_url = htinfo->base_url;
	if (!htinfo->title)
		title = strdup(pafd->aurl_wa);
	else
		title = htinfo->title;
	win->htinfo = htinfo;

/* detecter les frames et dans ce cas ajouter un niveau d'indirection */

/* ici, on ne peut avoir que FRAME_TYPE ou NOTFRAME_TYPE */
	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAME_TYPE);

	if ( (pafd->win->frame_type == FRAME_TYPE) && htinfo->nframes) {
			/* Un framset est charge dans un frame. */
			/* mMosaic n'accepte qu'un seul niveau de topframeset */
		char buf[8000];
		char * s;

		FreeHtmlTextInfo(htinfo); /* free here because it's an error */

		s="<HTML><BODY><P>A FRAMESET in a FRAME is not acceptable, click <a href=%s > here </a> on button 2 to see page</BODY></HTML>";
		sprintf(buf, s, pafd->aurl_wa);
		pafd->html_text = strdup(buf);
		htinfo = HTMLParseRepair(pafd->html_text);
		pafd->mlist = mlist = htinfo->mlist;
		mptr = mlist;
		pafd->num_of_eo = 0;
		base_url = htinfo->base_url;
		if (!htinfo->title)
			title = strdup(pafd->aurl_wa);
		else
			title = htinfo->title;
		win->htinfo = htinfo;

			/* ou peut etre : finir la demande et abort la requete */
			/* ne pas charger l'html dans le frame */
			/*call error.... peut etre */
			/*return; */

        } else if (htinfo->nframes) { /* this is a frameset */
		mo_window *sub_win=NULL;
		Widget htmlw;
		char *url = NULL , *frame_name;
		RequestDataStruct rds;
		Widget * htmlw_tabs;  /* html widgets return by hw */
		int i;

/* cas ou on charge un frameset */
		pafd->win->frame_type = FRAMESET_TYPE;
		docid =0;    /* ##### docid = HTMLGetDocId(win->scrolled_win); */

/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */

		docid = 0;		/* we are not in back or forward */
		spafd = *pafd;		/* why??? ############### */

/* win->scrolled_win est le framset contenant les frame*/
/* htmlw_tabs est un tableau de widget frame qui va contenir les frames*/

#ifdef MULTICAST
/* reset old depend object */
        	if (win->dot){
                	/*free(win->dot);*/ /* don't free old. Used in multicast cache */
                	win->dot =NULL;
                	win->n_do =0;
        	}               
#endif
		HTMLSetFrameSet (win->scrolled_win, htinfo->mlist, pafd->aurl,
			htinfo->nframes, htinfo->frameset_info, &htmlw_tabs);

/* on met a jour immediatement la partie navigation. Car on doit avoir un
 * current_node qui memorise tout la requete. Title is always allocated. */
		MMUpdNavigationOnNewURL(win, pafd->aurl_wa, pafd->aurl,
			pafd->goto_anchor, pafd->html_text, pafd->mhs, docid,
			htinfo);

		mo_set_win_headers(win, spafd.aurl_wa, title);
		MMUpdateGlobalHistory(spafd.aurl_wa);
		XFlush(XtDisplay(win->scrolled_win));

/* init parent FRAMESET */
		win->frame_name = NULL;
		win->frame_parent =NULL;
		win->frame_sons = NULL;
		win->frame_sons_nbre =  htinfo->nframes;
		win->number_of_frame_loaded = 0;
		win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre , sizeof(mo_window *));
/* use htmlw_tabs */
/* creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
		for(i = 0 ; i < htinfo->nframes; i++ )	{
			htmlw = htmlw_tabs[i];
			url = strdup(htinfo->frameset_info->frames[i].frame_src);
			frame_name = htinfo->frameset_info->frames[i].frame_name;
			sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
			sub_win->frame_sons_nbre = 0;
			sub_win->frame_dot_index = i;
			win->frame_sons[i] = sub_win;
			rds.ct = rds.post_data = NULL;
			rds.is_reloading = False;
			rds.req_url = url;
			rds.gui_action = FRAME_LOADED_FROM_FRAMESET;
			sub_win->navigation_action = NAVIGATE_NEW;
			MMPafLoadHTMLDocInWin (sub_win, &rds); 
			free(url);
		}

/*################################ */
/*en finir avec pafd du frameset , quand tous les frames sont loader. */
/* determine combien de frameset a attendre....;  */
/* ne detruire le pafd du frameset qu'a la fin du chargement des frames */
/* ##### where to destroy FRAMSET?? and sub_win??? ##### */
/*################################ */

		return;
	}

/* ici, on ne peut avoir que FRAME_TYPE ou NOTFRAME_TYPE */
	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAME_TYPE);

/* le html est une page 'normal' , ne contient pas de framset */
/* Faire une liste des embedded object */
/* initialiser et seulement initialiser ces objects */

	eo_tab = (EmbeddedObjectEntry *) calloc(1,sizeof(EmbeddedObjectEntry));
			/* alloc one */
/* recuperer le reste des embedded objects (si necessaire)*/
/* relancer la bete pour les embedded objets si il y en a */
	while (mptr != NULL){
		struct mark_up * omptr;
		char * tptr;
		ImageInfo * picd;

/* if in_title, grab the text until end marker */
		switch (mptr->type){
		case M_BODY:		/* BODY can have image... */
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) calloc(1, sizeof(ImageInfo ));
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
			picd = (ImageInfo *) calloc(1, sizeof(ImageInfo ));
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
			picd = (ImageInfo *) calloc(1,sizeof(ImageInfo ));
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
#ifdef OBJECT
 		case M_OBJECT:
			if(mptr->is_end)
				break;
			omptr = mptr;
			MMPreParseObjectTag(win, &mptr);
			if (! omptr->s_obs) {
				/* virer l'objet */
				assert(0);
				break;
			}
			omptr->s_obs->frame= NULL;
			break;
#endif
		default:
			break;
		}
		mptr = mptr->next;
	}
	pafd->embedded_object_tab = eo_tab; /* une selection de mlist*/

/* ##### docid = HTMLGetDocId(win->scrolled_win); */
	docid =0;
/* on met a jour immediatement la partie navigation. Car on doit avoir un 
 * current_node qui memorise tout la requete. title is always allocated.
 */

/* ici, on ne peut avoir que FRAME_TYPE ou NOTFRAME_TYPE */
	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAME_TYPE);

/* si c'est un Frame faire une navigation 'speciale' */
	MMUpdNavigationOnNewURL(win, pafd->aurl_wa, pafd->aurl,
		pafd->goto_anchor, pafd->html_text, pafd->mhs, docid,
		win->htinfo); 	/* ce n'est pas un FRAMESET */

/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */

/* afficher le texte. le mettre dans la Widget. id come from back and forward */
	docid = 0;			/* we are not in back or forward */
	spafd = *pafd;

#ifdef MULTICAST
/* reset old depend object */
	if (win->dot){
		/*free(win->dot);*/ /* don't free old. Used in multicast cache*/
		win->dot =NULL;
		win->n_do =0;
	}
#endif
	HTMLSetHTMLmark (win->scrolled_win, mlist, docid,
		pafd->goto_anchor, pafd->aurl);
	XFlush(XtDisplay(win->scrolled_win));

	if ( win->frame_type == NOTFRAME_TYPE){
		mo_set_win_headers(win, spafd.aurl_wa, title);
	}

/* MAJ de l'history etc... */
	MMUpdateGlobalHistory(spafd.aurl_wa);

/* HTMLWidget create a window for htmlObject, now run the plugin */
#ifdef OBJECT
	MMRunPlugins(win, mlist);
#endif


	if (spafd.num_of_eo == 0 ){	/* no embedded object it is the end */
		LateEndPafDoc(pafd);	/* en finir avec la requete de pafd. */
		return; /* it is finished!!! */
	}

/* sinon c'est pas fini... recuprer les embedded objects */
/* the next step is to process embedded object */
	pafd->cur_processing_eo = 0;		/* demande le premier */
/* a paf child inherit some data from its father */
	pafd->paf_child = (PafDocDataStruct *) calloc(1, sizeof(PafDocDataStruct));
	pafd->paf_child->parent_paf = pafd;
	pafc = pafd->paf_child;
	pafc->mhs = (MimeHeaderStruct*) calloc(1, sizeof(MimeHeaderStruct));
/* inherit from parent */
	pafc->twirl_struct = NULL;	/* only top twirl */
/* don't inherit from parent */
        pafc->pragma_no_cache = False;  /* pafd->pragma_no_cache ; */  
	pafc->post_ct = NULL;
	pafc->post_data = NULL;
	pafc->proxent = NULL;

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


/* ##########################################*/
/* user clic on an anchor in frame. On doit reconstituer le pafd du
 * du FRAMESET pour faire tourner le twirl, et pouvoir stopper un/les
 * fils du framset quand c'est necessaire
 */
/* ##########################################*/
void MMPafLoadHTMLDocInFrame(mo_window * fwin, RequestDataStruct * rds)
{
	PafDocDataStruct * fs_pafd = NULL;
	PafDocDataStruct * ofs_pafd = NULL;
	PafDocDataStruct * opafd = NULL;
	mo_window * fs_win = NULL;

	assert(fwin->frame_type == FRAME_TYPE);

	opafd = fwin->pafd;	/* Is a previous load in progress */
	if (opafd) {		/* if YES stop the previous request */
		MMStopPafDocData(opafd);
	}

	fs_win = fwin->frame_parent;

	assert(fs_win->frame_type == FRAMESET_TYPE);

	ofs_pafd = fs_win->pafd;
	if (ofs_pafd) {	/* frame in frameset is loading */
		fs_win->number_of_frame_loaded--;

		assert(fs_win->number_of_frame_loaded >= 0);

		MMPafLoadHTMLDocInWin(fwin, rds);	/* load frame */
		return;
	}

/* frameset is completly loaded, recreate a pafd for it */

	fs_pafd = (PafDocDataStruct *) calloc(1, sizeof(PafDocDataStruct));
	fs_pafd->gui_action = HTML_LOAD_CALLBACK;
	fs_pafd->mhs = NULL;
	fs_pafd->www_con_type = NULL;
	fs_pafd->fname = NULL;
	fs_pafd->fd = -1;
	fs_pafd->aurl_wa = NULL;
	fs_pafd->aurl = NULL;
	fs_pafd->goto_anchor = NULL;
	fs_pafd->http_status = 0;
	fs_pafd->win = fs_win;
	fs_pafd->n_redirect = 0;
	fs_win->pafd = fs_pafd;
	fs_pafd->call_me_on_succes = MMFinishPafDocData;
 	fs_pafd->call_me_on_error = MMErrorPafDocData;
	fs_pafd->call_me_on_stop = MMStopPafDocData;
	fs_pafd->sps.accept = NULL;
	fs_pafd->pragma_no_cache = False;
	fs_pafd->paf_child =NULL;
	fs_pafd->post_ct = NULL;
	fs_pafd->post_data = NULL;
	fs_pafd->proxent = NULL;

	fs_pafd->twirl_struct = (TwirlStruct*) calloc(1,sizeof(TwirlStruct));
	fs_pafd->twirl_struct->logo_widget = fs_win->logo;
	fs_pafd->twirl_struct->logo_count = 0;
	fs_pafd->twirl_struct->time_id = XtAppAddTimeOut(mMosaicAppContext,
		100L, twirl_icon_cb, fs_pafd->twirl_struct);

	fs_win->number_of_frame_loaded--;

	assert(fs_win->number_of_frame_loaded >= 0);

	MMPafLoadHTMLDocInWin(fwin, rds);	/* load frame */
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
		remove(fname);
		return;               
	}
/* fill the 'paf' struct for the request */
	pafd = (PafDocDataStruct *) calloc(1, sizeof(PafDocDataStruct));
	pafd->gui_action = gui_action;
	pafd->mhs = (MimeHeaderStruct*) calloc(1, sizeof(MimeHeaderStruct));
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
	if (win->frame_type != FRAME_TYPE) { /* a frame have no logo */
		pafd->twirl_struct = (TwirlStruct*) calloc(1,sizeof(TwirlStruct));
		pafd->twirl_struct->logo_widget = win->logo;
		pafd->twirl_struct->logo_count = 0;
		pafd->twirl_struct->time_id = XtAppAddTimeOut(mMosaicAppContext,
			100L, twirl_icon_cb, pafd->twirl_struct);
	}
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

	if ( pafc->www_con_type /*&& pafc->www_con_type->call_me_on_stop_cb*/) {
		(*pafc->www_con_type->call_me_on_stop_cb)(pafc);
	}

	assert(pafc->fd >0 );		/* let me know */

	close(pafc->fd);
	pafc->fd = -1;
#ifdef DEBUG_STOP
	printf("MMStopPafEmbeddedObject: %s must be stooped\n", pafc->aurl_wa);
#endif
	FreeMimeStruct(pafc->mhs);
	pafc->aurl = NULL; pafc->aurl_wa = NULL; pafc->mhs = NULL;
	FreePafDocDataStruct(pafc);
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

	assert(pafc->fd >=0);

	close(pafc->fd);
	pafc->fd = -1;
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
#ifdef OBJECT
	case M_OBJECT:
		assert(0);
		break;
#endif
	default:
		fprintf(stderr, "This a Bug. Please report\n");
		fprintf(stderr, "MMErrorPafEmbeddedObject, Unknow EmbeddedObject type %d\n", mptr->type);
		fprintf(stderr, "Aborting ...\n");
		assert(0);
	}

	ppaf->cur_processing_eo++;
	if ( ppaf->cur_processing_eo >= ppaf->num_of_eo) {
/* we have all embedded and doc */
/* get the current id  #### */
		elem_id = 0;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, elem_id, ppaf->goto_anchor, ppaf->aurl);
		free(ppaf->paf_child); /*### et le reste de paf_child*/
        	win = ppaf->win;
		LateEndPafDoc(ppaf); /* en finir avec la requete de pafd. */
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

	assert( pafc->fd >=0 );

	close(pafc->fd);
	pafc->fd = -1;
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
		assert(0);
	}

	unlink(pafc->fname);
	ppaf->cur_processing_eo++;

/*------------ ALL EMBEDDED OBJECTS DONE ? ------------*/
	if ( ppaf->cur_processing_eo >= ppaf->num_of_eo) {
/* we have all embedded and doc */
/* get the current id  #### */
		elem_id = 0;
		HTMLSetHTMLmark (ppaf->win->scrolled_win, ppaf->mlist, elem_id, ppaf->goto_anchor, ppaf->aurl);
		free(ppaf->paf_child); /*### et le reste de paf_child*/
        	win = ppaf->win;
		LateEndPafDoc(ppaf); /* en finir avec la requete de pafd. */
		return;	/* it is really the end !!! */
	}

/* else loop once more time */
	MMPafLoadEmbeddedObjInDoc(ppaf->paf_child);
}

/* we are called for all object */
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
		remove(fname);
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

/* utility by Winfried */
#ifdef DEBUG_PAFD
void PrintPafStruct(char *file,int line,PafDocDataStruct *pafd)
{
	char Buf[64];                    

	fprintf(stderr,"%s:%d:----- PafDocDataStruct -----\n",file,line);
	fprintf(stderr,"pafd->fname               '%s'\n",pafd->fname);
	fprintf(stderr,"pafd->fd                  '%d'\n",pafd->fd);
	fprintf(stderr,"pafd->aurl                '%s'\n",pafd->aurl);
	fprintf(stderr,"pafd->aurl_wa             '%s'\n",pafd->aurl_wa);
	fprintf(stderr,"pafd->goto_anchor         '%s'\n",pafd->goto_anchor);
	Buf[0] = 0;
	if(pafd->post_ct)strncpy(Buf,pafd->post_ct,20);
	fprintf(stderr,"pafd->post_ct             '%s'\n",Buf);
	Buf[0] = 0;
	if(pafd->post_data)strncpy(Buf,pafd->post_data,20);
	fprintf(stderr,"pafd->post_data           '%s'\n",Buf);
	fprintf(stderr,"pafd->mhs                 '%p'\n",pafd->mhs);
	Buf[0] = 0;
	if(pafd->html_text)strncpy(Buf,pafd->html_text,20);
	fprintf(stderr,"pafd->html_text           '%s'\n",Buf);
	fprintf(stderr,"pafd->mlist               '%p'\n",pafd->mlist);
	fprintf(stderr,"pafd->embedded_object_tab '%p'\n",pafd->embedded_object_tab);
	fprintf(stderr,"pafd->parent_paf          '%p'\n",pafd->parent_paf);
	fprintf(stderr,"pafd->paf_child           '%p'\n",pafd->paf_child);
	fprintf(stderr,"pafd->twirl_struct        '%p'\n",pafd->twirl_struct);
	fprintf(stderr,"pafd->lfcrlf_type         '%s'\n",pafd->lfcrlf_type);
	fprintf(stderr,"pafd->format_in           '%s'\n",pafd->format_in);
	fprintf(stderr,"pafd->www_con_type        '%p'\n",pafd->www_con_type);
	fprintf(stderr,"pafd->mc_fname_save       '%s'\n",pafd->mc_fname_save);
	fprintf(stderr,"pafd->sps.accept          '%s'\n",pafd->sps.accept);
	fprintf(stderr,"pafd->iobs.iobuf          '%p'\n",pafd->iobs.iobuf);
	fprintf(stderr,"pafd->proxent             '%p'\n",pafd->proxent);
	fputc('\n',stderr);              
} 
#endif

/* Il faut defaire et nettoyer le pafd quand tous les embedded sont charges */
/* Un pafd existe tant que des objects 'fils' sont en train d'etre charges */
/* Procedure commune pour les FRAME et HTML */
/* Le pafd d'un frameset est libere quanf tous les frames sont charge */
/* en finir avec la requete de pafd. */

static void LateEndPafDoc(PafDocDataStruct * pafd)
{
	mo_window * win;
	mo_window * winset=NULL;
#ifdef MULTICAST
	int moid_ret;
#endif

	win = pafd->win;

	assert(win->frame_type != FRAMESET_TYPE); /*cannot be Frameset */
        assert( pafd->fd == -1 ) ;      /* finnish doc : fd is closed*/

/* stop the twirl if only HTML*/                   
	if (win->frame_type == NOTFRAME_TYPE)
		MMStopTwirl(pafd);

#ifdef MULTICAST
        if (mc_send_win && (mc_send_win==win || mc_send_win==win->frame_parent)){
				/* The top window is a multicast sender */
                (*mc_send_win->mc_callme_on_new_object)(
                        pafd->mc_fname_save, pafd->aurl,
                        pafd->mhs,  win->dot , win->n_do, &moid_ret);
                win->moid_ref = moid_ret;      /* info for state */
                free(pafd->mc_fname_save);
		pafd->mc_fname_save = NULL;	/* sanity */

                if (win->frame_type == NOTFRAME_TYPE) { /* top html */
			(*mc_send_win->mc_callme_on_new_state)( win,
				win->moid_ref, NULL, 0);
/*win->dot, win->n_do); ### pas de dependance d'etat pour HTML ou FRAME */
		}
	}
#endif
        if (win->frame_type == NOTFRAME_TYPE) { /* top html */
/* Must be freed in navigation */
		pafd->aurl = pafd->aurl_wa = NULL; pafd->mhs = NULL;  /* sanity */
        	FreePafDocDataStruct(pafd);  
        	win->pafd = NULL;      
		return;		/* END of (only) html page */
	}

/* Traitement du type FRAME_TYPE */
/* dans le cas d'un frame , mettre a jour le framset */

	winset = win->frame_parent; /* the frameset */
	winset->number_of_frame_loaded++; /* one more is loaded */

	assert(winset->number_of_frame_loaded <=winset->frame_sons_nbre);

#ifdef MULTICAST
        if (mc_send_win && (mc_send_win==win || mc_send_win==win->frame_parent)){

/* gui_action: indique pour un frame si on clique dedans ou si on charge un
 * frameset...  */      
		switch(pafd->gui_action) {

/* loaded frame in frameset because a frameset is loaded */
/* the announce of top frameset obj is done at later time. When the last frame */
/* child will be loaded and all child is loaded.Save some data to be send later */

		case FRAME_LOADED_FROM_FRAMESET:
			McAddDependFrame(winset, moid_ret, win->frame_dot_index);
			break;

/* load a html in a frame because of clic in a anchor */ 
/* a html text change in a frame: do a new state */
		case HTML_LOAD_CALLBACK:
			McChangeDepend(winset,win,win->frame_dot_index,moid_ret);
			break;
		}
	}
#endif

	if(winset->number_of_frame_loaded == winset->frame_sons_nbre ) {
				/* Last Frame Loaded */
		PafDocDataStruct * fs_pafd = winset->pafd;

		assert(fs_pafd->fd == -1 );

		MMStopTwirl(fs_pafd); /* en finir avec le frameset */

#ifdef MULTICAST
        	if (mc_send_win && (mc_send_win==win || mc_send_win==win->frame_parent)){
			int fs_moid_ret;
/* late send state of frameset */
			if (pafd->gui_action == FRAME_LOADED_FROM_FRAMESET) {
				(*mc_send_win->mc_callme_on_new_object)(
					fs_pafd->mc_fname_save,
					fs_pafd->aurl, fs_pafd->mhs,
/* pas de dependance pour les framesets */
/* winset->dot, winset->n_do, */	NULL, 0, &fs_moid_ret);
				winset->moid_ref=fs_moid_ret; /* info for state */
				free(fs_pafd->mc_fname_save);
				fs_pafd->mc_fname_save = NULL; /* sanity */
			}
			(*mc_send_win->mc_callme_on_new_state)(
				winset, winset->moid_ref,
				winset->dot, winset->n_do);
		}
#endif

/* free parent frameset */
/* Must be freed in navigation */
		fs_pafd->aurl = fs_pafd->aurl_wa = NULL;
		fs_pafd->mhs = NULL; 			/*sanity*/
		FreePafDocDataStruct(fs_pafd);
		winset->pafd = NULL;
	}
/*free frame */
/* Must be freed in navigation */
	pafd->aurl = pafd->aurl_wa = NULL; pafd->mhs = NULL;  /* sanity */
       	FreePafDocDataStruct(pafd);  
       	win->pafd = NULL;      
	return;		/* END of frame page */
}
