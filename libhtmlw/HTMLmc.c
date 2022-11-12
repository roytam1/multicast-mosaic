/* HTMLmc.c
 * Author: Gilles Dauphin
 * Version 2.7b4m3 [May96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :  dauphin@sig.enst.fr dax@inf.enst.fr
 */

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../libmc/mc_defs.h"
#include "HTML.h"
#include "../src/mosaic.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

/* Defined in gui.c */
extern char *cached_url;
extern mo_window *current_win;

extern int installed_colormap;
extern Colormap installed_cmap;
extern unsigned char nums[];

/*
 * Convenience function to return the ele_rec of all images in the
 * document.
 * Function returns an array of ele_rec and fills num_ele_rec passed.
 * If there are no ele_rec NULL returned.
 */
struct ele_rec ** HTMLGetEoEleRec(Widget w, int *num_ele_rec)
{
        HTMLWidget hw = (HTMLWidget)w;
        struct ele_rec *eptr;
        int cnt;
        struct ele_rec **earray;

        cnt = 0;
        eptr = hw->html.formatted_elements;
        while (eptr != NULL) {
                if (eptr->type == E_IMAGE)
                        cnt++;
                eptr = eptr->next;
        }
        if (cnt == 0) {
                *num_ele_rec = 0;
                return(NULL);
        }
        *num_ele_rec = cnt;
        earray = (struct ele_rec  **)malloc(sizeof(struct ele_rec * ) * cnt);
        eptr = hw->html.formatted_elements;
        cnt = 0;
        while (eptr != NULL) {
                if (eptr->type == E_IMAGE) {
                        earray[cnt] = eptr;
                        cnt++;
                }
                eptr = eptr->next;
        }
        return(earray);
}

/* image converter fonction (from ImageResolve in img.c) */
ImageInfo * McGetPicData(Widget w, char * buf, int len_buf)
{
	extern int Vclass;
	int i, cnt;
	unsigned char *data;
	unsigned char *bg_map;
	unsigned char *bgptr;
	unsigned char *cptr;
	unsigned char *ptr;
	int width, height;
	int Used[256];
	XColor colrs[256];
	ImageInfo *img_data;
	int widthbyheight = 0;
	char *fnam;
	int bg, bg_red, bg_green, bg_blue;
	mo_window *win = NULL;
	static Pixel bg_pixel;
	static int done_fetch_colors = 0;
	int j,bcnt;
	int tmpfd;
	/*
	 * No transparent background by default
	 */
	bg = -1;
	bg_map = NULL;
	fnam = mo_tmpnam(NULL);
	tmpfd = open(fnam,O_WRONLY|O_CREAT,0644);
	write(tmpfd,buf,len_buf);
	close(tmpfd);
	data = ReadBitmap(fnam, &width, &height, colrs, &bg);
	unlink(fnam);
	/* if we have a transparent background, prepare for it */
	if ((bg >= 0)&&(data != NULL)) {
		XColor tmpcolr;

		/* This code copied from xpmread.c.  I could almost
		 * delete the code from there, but I suppose an XPM
		 * file could pathalogially have multiple transparent
		 * colour indicies. -- GWP
		 */
		XtVaGetValues(w, XtNbackground, &bg_pixel, NULL);
		tmpcolr.pixel = bg_pixel;
		XQueryColor(XtDisplay(w),
			    (installed_colormap ? installed_cmap :
			     DefaultColormap(XtDisplay(w), 
			    DefaultScreen(XtDisplay(w)))),
			    &tmpcolr);
		bg_red = colrs[bg].red = tmpcolr.red;
		bg_green = colrs[bg].green = tmpcolr.green;
		bg_blue = colrs[bg].blue = tmpcolr.blue;
		colrs[bg].flags = DoRed|DoGreen|DoBlue;

		bg_map = (unsigned char *)malloc(width * height);
	}
      
	img_data = (ImageInfo *)malloc(sizeof(ImageInfo));
	if ((bg >= 0)&&(data != NULL)) {
		img_data->transparent=1;
		img_data->clip_data=(unsigned char *)malloc(width * height);
		memset(img_data->clip_data,0,(width*height));
	} else {
		img_data->transparent=0;
		img_data->clip_data=NULL;
	}
	img_data->width = width;
	img_data->height = height;
	img_data->image_data = data;
	img_data->image = 0;
	img_data->clip = 0;
	/*img_data->src = strdup(src);*/
	img_data->src = strdup("Multicast");
	/* Bandaid for bug afflicting Eric's code, apparently. */
	img_data->internal = 0;

	widthbyheight = img_data->width * img_data->height;
	for (i=0; i < 256; i++)		 /* Fill out used array. */
		Used[i] = 0;
	cnt = 1;
	bgptr = bg_map;
	cptr = img_data->clip_data;
	ptr = img_data->image_data;

	/*This is sets the bg map and also creates bitmap data for the
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
					*cptr += nums[(bcnt % 8)];
				}
				if ((bcnt % 8)==7 || j==(img_data->width-1))
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
			}
		}
	}
	/* if MedianCut ate our background, add the new one now. */
	if (bg == 256) {
		img_data->reds[cnt - 1] = bg_red;
		img_data->greens[cnt - 1] = bg_green;
		img_data->blues[cnt - 1] = bg_blue;
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
	/*####### mo_cache_data (src, (void *)img_data, 0); #####*/
	/*free (src);*/
	return img_data;
}

void McUpdateWidgetObject(Widget w,int num_eo,char * data,int len_data)
{
	HTMLWidget hw = (HTMLWidget)w;
	struct ele_rec **earray;
	int neo_source;	/* the number of eo in html text source code */
	struct ele_rec *eptr;

	earray = HTMLGetEoEleRec(w, &neo_source);
	num_eo = num_eo - 3;	/* 0 1 2 is busy */

	if (neo_source == 0)
		return;
	if (neo_source  < (num_eo +1) ){
		printf("probleme getting embedded object \n");
		printf(" neo_source/num_eo = %d/%d\n",neo_source,num_eo);
		return;
	}
	
	eptr = earray[num_eo];
	eptr->pic_data = McGetPicData(w, data,len_data);

	if (eptr->pic_data->image_data != NULL) {
        	eptr->pic_data->delayed = 0;
        	eptr->pic_data->fetched = 1;
        }
         /*
         * we have to do a full reformat.
         */
#ifdef TO_DO
	/*####### C'est a optimiser ########## */
#endif
	ReformatWindow(hw,True);
	ScrollWidgets(hw);
	ViewClearAndRefresh(hw);
}

/* return the file name or NULL */

char * McGetEOFileData(Widget w, char * buf, int len_buf, McUser *u,int num_eo)
{
	char * fnam = NULL;
	int tmpfd;

	if (u->filename[num_eo])
		return u->filename[num_eo];

	fnam = mo_tmpnam(NULL);
	tmpfd = open(fnam,O_WRONLY|O_CREAT,0644);
	write(tmpfd,buf,len_buf);
	close(tmpfd);
	u->filename[num_eo] = fnam;

	return fnam;
}
