/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "mosaic.h"
#include "../libmc/mc_dispatch.h"
#include "../libmc/mc_misc.h"
#include "globalhist.h"

extern int cci_event;

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

#include "bitmaps/gopher_image.xbm"
#include "bitmaps/gopher_movie.xbm"
#include "bitmaps/gopher_menu.xbm"
#include "bitmaps/gopher_text.xbm"
#include "bitmaps/gopher_sound.xbm"
#include "bitmaps/gopher_index.xbm"
#include "bitmaps/gopher_telnet.xbm"
#include "bitmaps/gopher_binary.xbm"
#include "bitmaps/gopher_unknown.xbm"


static ImageInfo *gopher_image = NULL;
static ImageInfo *gopher_movie = NULL;
static ImageInfo *gopher_menu = NULL;
static ImageInfo *gopher_text = NULL;
static ImageInfo *gopher_sound = NULL;
static ImageInfo *gopher_index = NULL;
static ImageInfo *gopher_telnet = NULL;
static ImageInfo *gopher_binary = NULL;
static ImageInfo *gopher_unknown = NULL;

extern ImageInfo * DelayedImageData( Boolean anchored);

/* Defined in gui.c */
extern char *cached_url;
extern mo_window *current_win;

/* Defined in gui-documents.c */
extern int interrupted;
extern int loading_inlined_images;
extern int installed_colormap;
extern Colormap installed_cmap;

#define RETURN_IMGINFO_FROM_BITMAP(x) \
{ \
  if (!x) { \
	x = (ImageInfo *)malloc (sizeof (ImageInfo)); \
	x->ismap = 0; \
	x->width = x##_width; \
	x->height = x##_height; \
	x->image_data = NULL; \
	x->internal = 1; \
	x->delayed = 0; \
	x->fetched = 1; \
	x->num_colors = 0; \
	x->reds = NULL; \
	x->greens = NULL; \
	x->blues = NULL; \
	x->transparent = 0; \
	x->image = XCreatePixmapFromBitmapData \
          (XtDisplay(swin), XtWindow(view), \
          (char*) x##_bits,  \
           x##_width, x##_height, \
           fg_pixel, bg_pixel, DefaultDepth(dsp, DefaultScreen(dsp))); \
  }\
  if (cci_event) MoCCISendEventOutput(IMAGE_LOADED); \
	img_data->ismap = 0; \
	img_data->width = x##_width; \
	img_data->height = x##_height; \
	img_data->image_data = NULL; \
	img_data->internal = 1; \
	img_data->delayed = 0; \
	img_data->fetched = 1; \
	img_data->num_colors = 0; \
	img_data->reds = NULL; \
	img_data->greens = NULL; \
	img_data->blues = NULL; \
	img_data->transparent = 0; \
	img_data->image = x->image;\
  return; \
}

/* ------------------------------------------------------------------------ */

unsigned char nums[]={
	1, 2, 4, 8, 16, 32, 64, 128
  };

/* Width and Height Hack: (SWP)
	Added "width" and "height" to the parameter list. These are used to
	resize the image when we are done decoding. This'll be fun when we
	start processing on a line-by-line basis.
	If NULLs are passed, the width and height are not used.
	*/

/* Image resolution function. */
/* convertir ImageResolve en un Callback #######################*/
/*ImageInfo *ImageResolve (Widget w, char *src, int noload, 
		char *wid, char *hei,McMoWType wtype, int internal_numeo)
*/
void ImageResolve(Widget w, XtPointer clid, XtPointer calld)
{
	ImageInfo *img_data = (ImageInfo *) calld;
	mo_window * win = (mo_window *) clid;
	ImageInfo *imi = NULL;

	extern int Vclass;
	int i, cnt;
	unsigned char *data;
	unsigned char *bg_map;
	unsigned char *bgptr;
	unsigned char *cptr;
	unsigned char *ptr;
	char * src = NULL;
	int width, height;
	int Used[256];
	XColor colrs[256];
	int widthbyheight = 0;
	char *fnam;
	int rc;
	int bg, bg_red, bg_green, bg_blue;
	static Pixel fg_pixel, bg_pixel;
	static int done_fetch_colors = 0;
	int j,bcnt;
	McMoWType wtype;
	int internal_numeo;
	int noload;

					/*typedef struct image_rec { 	*/
				        /* int ismap;			*/
        				/* FormInfo *fptr;		*/
        img_data->internal = 0;
        img_data->delayed = 0;
        img_data->fetched = 0;
        img_data->cached = 0;
        img_data->width = 0;
        img_data->height = 0;
        img_data->num_colors = 0;
        img_data->reds = NULL;
        img_data->greens = NULL;
        img_data->blues = NULL;
        img_data->image_data = NULL;
        img_data->clip_data = NULL;
	img_data->transparent = 0;
        img_data->image = (Pixmap)NULL;
        img_data->clip = (Pixmap)NULL;
        				/* char *text;			*/
        				/* char *src;			*/
        				/* McMoWType wtype;		*/
        				/* int internal_numeo;		*/
					/* } ImageInfo; 		*/

	if (w == NULL){
		printf("[ImageResolve] NoWidget !!!!\n");
		return;
	}
	if(img_data->src)
		src = strdup(img_data->src);
	noload = win->delay_image_loads;
#ifndef DISABLE_TRACE
	if (srcTrace)
		fprintf (stderr, 
			"[ImageResolve] I'm entering, src '%s', noload %d!\n", 
			src, noload);
#endif
	if(noload){
		/*####################*/
		*img_data = *(DelayedImageData(True));
		return;
	}
/*	wtype = img_data->wtype; #############*/
	wtype = win->mc_type;
	internal_numeo = img_data->internal_numeo;

	if (!src)
		return ;
/*#########################*/
	mo_set_current_cached_win (win);
	cached_url = win->cached_url ? win->cached_url : "lose";
	win->cached_url = cached_url;

#ifndef DISABLE_TRACE
	if (srcTrace)
		fprintf (stderr, "[ImageResolve] hello! win 0x%08x\n", win);
#endif
 
	if (strncmp (src, "internal-", 9) == 0) {    /* Internal images. */
		extern Widget view; /*hw->html.view*/
		Widget swin = current_win->scrolled_win;

		if (!done_fetch_colors) {
			if (!view)
				return ;
			/* First, go fetch the pixels. */
			XtVaGetValues(view,
				XtNforeground, &fg_pixel,
				XtNbackground, &bg_pixel,
				NULL);
			done_fetch_colors = 1;
		}

		if (strcmp (src, "internal-gopher-image") == 0)
			{ RETURN_IMGINFO_FROM_BITMAP(gopher_image) }
		if (strcmp (src, "internal-gopher-movie") == 0)
			{ RETURN_IMGINFO_FROM_BITMAP(gopher_movie) }
		if (strcmp (src, "internal-gopher-menu") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_menu)}
		if (strcmp (src, "internal-gopher-text") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_text)}
		if (strcmp (src, "internal-gopher-sound") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_sound)}
		if (strcmp (src, "internal-gopher-index") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_index)}
		if (strcmp (src, "internal-gopher-telnet") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_telnet)}
		if (strcmp (src, "internal-gopher-binary") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_binary)}
		if (strcmp (src, "internal-gopher-unknown") == 0)
			{RETURN_IMGINFO_FROM_BITMAP(gopher_unknown)}
	}
#ifdef MULTICAST
	/* if it is a receiver do something . Reassemble image if possible*/
	/* the image is update in McActionAllData when it's ready to do it */
	if (wtype == MC_MO_TYPE_RCV_ALL){
		McUser * d = win->mc_user;	/*#####*/
		int num_eo = internal_numeo + 2;

		if (d->neo < (num_eo+1))	/* GASP###### */
			return ;
		if (d->eos[num_eo] == NULL)
			return;

		/*####################*/
		imi = McGetPicData(w, d->eos[num_eo],d->seo[num_eo]);
		
					/* ismap is managed by HTMLWidget */
        				/* fptr is managed by HTMLWidget */
        		img_data->internal =  0;
        		img_data->delayed = 0;
        		img_data->fetched = 1;
        		img_data->width = imi->width;
        		img_data->height = imi->height;
        		img_data->num_colors = imi->num_colors;
        		img_data->reds = imi->reds;
        		img_data->greens = imi->greens;
        		img_data->blues = imi->blues;
        		img_data->image_data = imi->image_data;
        		img_data->clip_data = imi->clip_data;
			img_data->transparent = imi->transparent;
        		img_data->image = (Pixmap)NULL;
        		img_data->clip = (Pixmap)NULL;
        				/* text is managed by HTMLWidget */
			img_data->src = imi->src;
        				/* wtype;		*/
        				/* internal_numeo;	*/
	


		return ;
	}
#endif
	src = mo_url_canonicalize (src, cached_url);

	/* Go see if we already have the image info hanging around. */
	if ((wtype != MC_MO_TYPE_MAIN) || (mc_send_enable == False)){
					/* no cache when sending */
		imi = mo_fetch_cached_image_data (src);
		if (imi){
					/* ismap is managed by HTMLWidget */
        				/* fptr is managed by HTMLWidget */
        		img_data->internal =  0;
        		img_data->delayed = 0;
        		img_data->fetched = 1;
        		img_data->width = imi->width;
        		img_data->height = imi->height;
        		img_data->num_colors = imi->num_colors;
        		img_data->reds = imi->reds;
        		img_data->greens = imi->greens;
        		img_data->blues = imi->blues;
        		img_data->image_data = imi->image_data;
        		img_data->clip_data = imi->clip_data;
			img_data->transparent = imi->transparent;
        		img_data->image = (Pixmap)NULL;
        		img_data->clip = (Pixmap)NULL;
        				/* text is managed by HTMLWidget */
			img_data->src = imi->src;
        				/* wtype;		*/
        				/* internal_numeo;	*/
/* ################## 
/*        		if (img_data->bg_index>=0) {  
/*        			unsigned long bg_pixel;
/*        			XColor tmpcolr;     
/*                		/* This code copied from xpmread.c.  I could almost
/*                 		* delete the code from there, but I suppose an XPM
/*                 		* file could pathalogially have multiple transparent
/*                 		* colour indicies. -- GWP  
/*                 		*/                   
/*                		XtVaGetValues(view, XtNbackground, &bg_pixel, NULL);
/*                		tmpcolr.pixel = bg_pixel;
/*                		XQueryColor(XtDisplay(view),
/*                            		(installed_colormap ?
/*                             		installed_cmap :
/*                             		DefaultColormap(XtDisplay(view), DefaultScreen(XtDisplay(view)))),                       
/*                            		&tmpcolr);
/*                		img_data->reds[img_data->bg_index]=tmpcolr.red;
/*                		img_data->greens[img_data->bg_index]=tmpcolr.green;
/*                		img_data->blues[img_data->bg_index]=tmpcolr.blue;
/*        		}        
*/
	
			if (cci_event) 
				MoCCISendEventOutput(IMAGE_LOADED);
			return ;
		}
	}

	/* If we don't have the image cached and noload is high,
	 * then just return NULL to avoid doing a network load. 
	 * Also return if interrupted is high. */
	if ( interrupted) {
#ifndef DISABLE_TRACE
	if (srcTrace)
		fprintf (stderr, "RETURNING Null interrupted %d\n",
			interrupted);
#endif
		free (src);
		return ;
	}
#ifndef DISABLE_TRACE
	if (srcTrace)
		fprintf (stderr, "GOING ON THROUGH...\n");
#endif
	/*
	 * No transparent background by default
	 */
	bg = -1;
	bg_map = NULL;
	/* if w is NULL we're stuffing the cache with our own info... BJS */
	if(w) { 		/* We have to load the image. */
		fnam = mo_tmpnam(src);
		interrupted = 0;
		rc = mo_pull_er_over_virgin (src, fnam);
		if (!rc) {
#ifndef DISABLE_TRACE
			if (srcTrace)
				fprintf (stderr, 
				"mo_pull_er_over_virgin returned %d; bonging\n",
				rc);
#endif
			free (fnam);
			return;
		}
#ifndef DISABLE_TRACE
		if (srcTrace)
			fprintf (stderr, 
			 "[ImageResolve] Got through mo_pull_er_over_virgin, rc %d\n", rc);
#endif
		/* Send it through CCI if need be            */
		MoCCISendBrowserViewFile(src, "unknown", fnam);
	}
/*################################ */
#ifdef MULTICAST
	{
	McGlobalEo * geo;
	/* if this is a sender widget, save the original data for futur send*/
	if ((wtype == MC_MO_TYPE_MAIN) && mc_send_enable){
		if (!img_data->cw_only){
			geo = McGetEmptyGlobalObject();	/* a structure to fill */
			geo->orig_data = McReadEo(fnam,&(geo->len_eo));
			/* and that's all. Work is done in McFillData */
		}
	}
	}
	/* continue if wtype == MC_TYPE_NONE */
#endif	
	data = ReadBitmap(fnam, &width, &height, colrs, &bg);
/*################################ */
#ifndef DISABLE_TRACE
	if (srcTrace)
		fprintf (stderr,
			 "[ImageResolve] Did ReadBitmap, got 0x%08x\n", data);
#endif
	/* if we have a transparent background, prepare for it */
	if ((bg >= 0)&&(data != NULL)) {
		unsigned long bg_pixel;
		XColor tmpcolr;
		extern Widget view;

		/* This code copied from xpmread.c.  I could almost
		 * delete the code from there, but I suppose an XPM
		 * file could pathalogially have multiple transparent
		 * colour indicies. -- GWP
		 */
		XtVaGetValues(view, XtNbackground, &bg_pixel, NULL);
		tmpcolr.pixel = bg_pixel;
		XQueryColor(XtDisplay(view),
			    (installed_colormap ? installed_cmap :
			     DefaultColormap(XtDisplay(view), 
			    DefaultScreen(XtDisplay(view)))),
			    &tmpcolr);
		bg_red = colrs[bg].red = tmpcolr.red;
		bg_green = colrs[bg].green = tmpcolr.green;
		bg_blue = colrs[bg].blue = tmpcolr.blue;
		colrs[bg].flags = DoRed|DoGreen|DoBlue;

		bg_map = (unsigned char *)malloc(width * height);
	}
      
	/* Now delete the file. */
	unlink(fnam); 
	{
		char *hfnam = (char *)malloc ((strlen (fnam) + strlen(".html") + 5) * sizeof (char));
		sprintf (hfnam, "%s.html", fnam);
		unlink(hfnam); 
		free(hfnam);
	}
	if (w)
		free (fnam);
	if (data == NULL) {
#ifndef DISABLE_TRACE
		if (srcTrace)
			fprintf (stderr, 
			     "[ImageResolve] data == NULL; punting...\n");
#endif
		return ;
	}
                                      
{                                     
int found_bg=0;                       
                                      
        if (data!=NULL) {             
                for (i=0; i<width*height; i++) {
                        if ((int)(data[i])==bg) {
                                found_bg=1; 
                                break;
                        }             
                }                     
                if (!found_bg) {      
                        bg=(-1);      
                }                     
        }                             
}                                     
    

	if ((bg >= 0)&&(data != NULL) &&
	    get_pref_boolean(eCLIPPING) &&
          (get_pref_int(eMAX_CLIPPING_SIZE_X)==(-1) ||
           get_pref_int(eMAX_CLIPPING_SIZE_X)>=width) &&
          (get_pref_int(eMAX_CLIPPING_SIZE_Y)==(-1) ||
           get_pref_int(eMAX_CLIPPING_SIZE_Y)>=height)) {
		img_data->transparent=1;
		img_data->clip_data=(unsigned char *)malloc(width * height);
		memset(img_data->clip_data,0,(width*height));
        	img_data->bg_index=bg;        
	} else {
		img_data->transparent=0;
		img_data->clip_data=NULL;
        	img_data->bg_index=(-1);
	}
	img_data->width = width;
	img_data->height = height;
	img_data->image_data = data;
	img_data->image = 0;
	img_data->clip = 0;
	img_data->src = strdup(src);
	/* Bandaid for bug afflicting Eric's code, apparently. */
	img_data->internal = 0;
	img_data->fetched = 1;

	widthbyheight = img_data->width * img_data->height;
	for (i=0; i < 256; i++)		 /* Fill out used array. */
		Used[i] = 0;
	cnt = 1;
	bgptr = bg_map;
	cptr = img_data->clip_data;
	ptr = img_data->image_data;

	/*This sets the bg map and also creates bitmap data for the
	 * clip mask when there is a bg image */
	for (i=0; i<img_data->height; i++) {
		for (j=0,bcnt=0; j<img_data->width; j++) {
			if (Used[(int)*ptr] == 0) {
				Used[(int)*ptr] = cnt;
				cnt++;
			}
			if (bg>=0) {
				if (*ptr == bg) {
					*bgptr = 1;
				} else {
					*bgptr = 0;
                                       if (img_data->transparent) {
                                               *cptr += nums[(bcnt % 8)];
                                       }
				}
                                if (img_data->transparent &&
                                    ((bcnt % 8)==7 ||
                                     j==(img_data->width-1)))
					cptr++;
				bgptr++;
				bcnt++;
			}
			ptr++;
		}
	}
	cnt--;

	/* If the image has too many colors, apply a median cut algorithm to
	* reduce the color usage, and then reprocess it.
	* Don't cut colors for direct mapped visuals like TrueColor.
	*/
	if((cnt>get_pref_int(eCOLORS_PER_INLINED_IMAGE))&&(Vclass != TrueColor)){
		MedianCut(img_data->image_data, &img_data->width, 
			&img_data->height, colrs, 256, 
			get_pref_int(eCOLORS_PER_INLINED_IMAGE));
      
		for (i=0; i < 256; i++)
			Used[i] = 0;
		cnt = 1;
		ptr = img_data->image_data;
		for (i=0; i < widthbyheight; i++) {
			if (Used[(int)*ptr] == 0) {
				Used[(int)*ptr] = cnt;
				cnt++;
			}
			ptr++;
		}
		cnt--;

		/*if we had a transparent bg, MedianCut used it. Get a new one */
		if (bg >= 0) {
			cnt++;
			bg = 256;
		}
	}
	img_data->num_colors = cnt;
	img_data->reds = (int *)malloc(sizeof(int) * cnt);
	img_data->greens = (int *)malloc(sizeof(int) * cnt);
	img_data->blues = (int *)malloc(sizeof(int) * cnt);

  /* bg is not set in here if it gets munged by MedCut */
	for (i=0; i < 256; i++) {
		int indx;
      
		if (Used[i] != 0) {
			indx = Used[i] - 1;
			img_data->reds[indx] = colrs[i].red;
			img_data->greens[indx] = colrs[i].green;
			img_data->blues[indx] = colrs[i].blue;
			/* squeegee in the background color */
			if ((bg >= 0)&&(i == bg)) {
				img_data->reds[indx] = bg_red;
				img_data->greens[indx] = bg_green;
				img_data->blues[indx] = bg_blue;
              			img_data->bg_index=indx;
			}
		}
	}
	/* if MedianCut ate our background, add the new one now. */
	if (bg == 256) {
		img_data->reds[cnt - 1] = bg_red;
		img_data->greens[cnt - 1] = bg_green;
		img_data->blues[cnt - 1] = bg_blue;
        	img_data->bg_index=(cnt-1);   
	}
	bgptr = bg_map;
	cptr = img_data->clip_data;
	ptr = img_data->image_data;
	for (i=0; i < widthbyheight; i++) {
		*ptr = (unsigned char)(Used[(int)*ptr] - 1);
		/* if MedianCut ate the background, enforce it here */
		if (bg == 256) {
			if (*bgptr)
				*ptr = (unsigned char)(cnt - 1);
			bgptr++;
		}
		ptr++;
	}
	if (bg_map != NULL)	 /* free the background map if we have one */
		free (bg_map);
	if (srcTrace)
		fprintf (stderr, 
			"[ImageResolve] Doing mo_cache_data on '%s', 0x%08x\n",
			src, img_data);

	imi = (ImageInfo *) malloc(sizeof(ImageInfo));
	*imi = *img_data;
	
	imi->ismap = 0;		/* ismap is managed by HTMLWidget */
        imi->fptr = NULL;	/* fptr is managed by HTMLWidget */
        imi->internal = 0;
        imi->delayed = 0;
        imi->fetched = 1;
        			/* img_data->width = 0; */
        			/* img_data->height = 0; */
        			/* img_data->num_colors = 0; */
        			/* img_data->reds = NULL; */
        			/* img_data->greens = NULL; */
        			/* img_data->blues = NULL; */
        			/* img_data->image_data = NULL; */
        			/* img_data->clip_data = NULL; */
				/* img_data->transparent = 0; */
        imi->image = (Pixmap)NULL;
        imi->clip = (Pixmap)NULL;
				/* img_data->src is still strduped */
        				/* McMoWType wtype;		*/
        				/* int internal_numeo;		*/
	
	mo_cache_data (src, (void *)imi, 0);
	if (srcTrace)
		fprintf (stderr, "[ImageResolve] Leaving...\n");
	if (cci_event) MoCCISendEventOutput(IMAGE_LOADED);
	return ;
}

mo_status mo_free_image_data (void *ptr)
{
	ImageInfo *img = (ImageInfo *)ptr;
  Widget swin=current_win->scrolled_win;

	if (srcTrace)
		fprintf (stderr, "[mo_free_image_info] Freeing 0x%08x\n", img);
	if (!img)
		return mo_fail;

	if (img->reds ) {
		free((char*)img->reds);
		img->reds = NULL;
	}
	if (img->greens) {
		free((char*)img->greens);
		img->greens = NULL;
	}
	if (img->blues ) {
		free((char*)img->blues);
		img->blues = NULL;
	}
	if (img->image_data) {
		free (img->image_data);
		img->image_data = NULL;
	}
	if (img->clip_data) {
		free (img->clip_data);
		img->clip_data = NULL;
	}
	if (img->src) {
		free (img->src);
		img->src = NULL;
	}
/*##### */
  if (img->image!=None) {             
        XFreePixmap(XtDisplay(swin),img->image);
        img->image=None;              
  }                                   
  if (img->clip!=None) {              
        XFreePixmap(XtDisplay(swin),img->clip);
        img->clip=None;               
  }      
	return mo_succeed;
}

