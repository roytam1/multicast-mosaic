/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "../libmc/mc_dispatch.h"
#include "../libmc/mc_misc.h"
#include "picread.h"
#include "cache.h"

extern int cci_event;

/* Defined in gui.c */
extern char *cached_url;

/* Defined in gui-documents.c */
extern int interrupted;

/* Image resolution function. */

void ImageResolve(Widget w, XtPointer clid, XtPointer calld)
{
	ImageInfo *img_info = (ImageInfo *) calld;
	mo_window * win = (mo_window *) clid;
	unsigned char *data;
	char * src = NULL;
	int width, height;
	XColor colrs[256];
	char *filename;
	char * url;
	int rc;
	int bg ;
	McMoWType wtype;
	unsigned char * bit_data;
	int len_data, rlen;
	int i;

        img_info->fetched = 0;
        img_info->cached = 0;
        img_info->width = 0;
        img_info->height = 0;
        img_info->num_colors = 0;
        img_info->image_data = NULL;
	img_info->len_image_data = 0;
        img_info->clip_data = NULL;
	img_info->transparent = 0;
	img_info->bg_index = -1 ;

/* if w is NULL we're stuffing the cache with our own info... BJS */
	if (w == NULL){
		printf("[ImageResolve] NoWidget !!!!\n");
		return;
	}

	if(img_info->src)
		src = strdup(img_info->src);
	if (!src)
		return ;
#ifdef MULTICAST
	wtype = win->mc_type;
#endif

/* First look in the cache */

/*#########################*/
	cached_url = win->cached_url ? win->cached_url : "lose";
	win->cached_url = cached_url;
	url = mo_url_canonicalize (src, cached_url);
	len_data = MMCacheFetchCachedData (url,&filename);
	bit_data=NULL;
	if (len_data){
        	bit_data = ReadBitmap(w, filename, &width, &height, colrs, &bg);
		free(filename);
	}
	if(bit_data){
        	img_info->internal =  0;
        	img_info->delayed = 0;
        	img_info->fetched = 1;
		img_info->cached = 1;
        	img_info->image_data = bit_data;
		if (bg >= 0 )
       			img_info->bg_index=bg;        
		img_info->width = width;
		img_info->height = height;
		img_info->len_image_data = len_data;
		for ( i = 0; i < 256; i++)
			img_info->colrs[i] = colrs[i];
		if (cci_event) 
			MoCCISendEventOutput(IMAGE_LOADED);
		free(src);
		free(url);
		return ;
	}

#ifdef MULTICAST
	/* if it is a receiver do something . Reassemble image if possible*/
	/* the image is update in McActionAllData when it's ready to do it */
	if (wtype == MC_MO_TYPE_RCV_ALL){
		McUser * d = win->mc_user;	/*#####*/
		int num_eo = img_info->internal_numeo + 2;

		if (d->neo < (num_eo+1))	/* GASP###### */
			return ;
		if (d->eos[num_eo] == NULL)
			return;
		len_data = d->seo[num_eo];
		data = (unsigned char*)d->eos[num_eo];
		rlen = MMCachePutDataInCache((char*)data,len_data, url, &filename);
		if ( rlen ){
        		bit_data = ReadBitmap(w, filename, &width, &height, colrs, &bg);
			free(filename);
		}
		if(bit_data){
        		img_info->internal =  0;
        		img_info->delayed = 0;
        		img_info->fetched = 1;
			img_info->cached = 1;
        		img_info->image_data = bit_data;
			if (bg >= 0 )
       				img_info->bg_index=bg;        
			img_info->width = width;
			img_info->height = height;
			img_info->len_image_data = len_data;
			for ( i = 0; i < 256; i++)
				img_info->colrs[i] = colrs[i];
			if (cci_event) 
				MoCCISendEventOutput(IMAGE_LOADED);
		}
		free(src);
		free(url);
		return;
	}
#endif

	/* If we don't have the image cached return if interrupted is high. */
	if ( interrupted) { /*RETURNING Null interrupted */
		free (src);
		free (url);
		return ;
	}

/* ####### tester si on charge les images */
	if ( win->delay_image_loads)
		return;

	rc = MMCacheGetDataAndCache(url, &filename, &len_data,win);
	if (!rc) { 		/* mo_pull_er_over_virgin returned bonging */
		free (filename);
		free(src);
		free(url);
		return;
	}
/* Send it through CCI if need be            */
	MoCCISendBrowserViewFile(src, "unknown", filename);
/*################################ */
#ifdef MULTICAST
	{
	McGlobalEo * geo;
/* if this is a sender widget, save the original data for futur send*/
	if ((wtype == MC_MO_TYPE_MAIN) && mc_send_enable){
		if (!img_info->cw_only){
			geo = McGetEmptyGlobalObject();	/* a structure to fill */
			geo->orig_data = McReadEo(filename,&(geo->len_eo));
			/* and that's all. Work is done in McFillData */
		}
	}
	}
	/* continue if wtype == MC_TYPE_NONE */
#endif	
	bit_data = ReadBitmap(w, filename, &width, &height, colrs, &bg);

/* Now delete the file. */
	free (filename);

/*################################ */
	if (bit_data == NULL) {
		free(src);
		free(url);
		return ;
	}

	if (bg >= 0 )
        	img_info->bg_index=bg;        
       	img_info->internal =  0;
       	img_info->delayed = 0;
	img_info->width = width;
	img_info->height = height;
	img_info->image_data = bit_data;
	img_info->len_image_data = len_data;
	img_info->fetched = 1;
	img_info->cached = 1;
	for ( i = 0; i < 256; i++)
		img_info->colrs[i] = colrs[i];

	if (cci_event) MoCCISendEventOutput(IMAGE_LOADED);
	return ;
}
