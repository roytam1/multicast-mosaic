/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"
#include "images/NoImage.xbm"
#include "images/DelayedImage.xbm"
#include "images/AnchoredImage.xbm"
#include "HTMLimages.h"

ImageInfo no_image;
ImageInfo delayed_image;
ImageInfo anchored_image;

static int allocation_index[256];

extern int installed_colormap;
extern Colormap installed_cmap;

extern int htmlwTrace;

/*
 * Free all the colors in the default colormap that we have allocated so far.
 */
void FreeColors(Display *dsp, Colormap colormap)
{
	int i, j;
	unsigned long pix;

	for (i=0; i<256; i++) {
		if (allocation_index[i]) {
			pix = (unsigned long)i;
			/*
			 * Because X is stupid, we have to Free the color
			 * once for each time we've allocated it.
			 */
			for (j=0; j<allocation_index[i]; j++)
				XFreeColors(dsp, colormap, &pix, 1, 0L);
		}
		allocation_index[i] = 0;
	}
}

void FreeBodyImages(HTMLWidget hw)
{
	if (hw->html.bgmap_SAVE!=None) {
		XFreePixmap(XtDisplay(hw), hw->html.bgmap_SAVE);
		hw->html.bgmap_SAVE=None;
	}
	if (hw->html.bgclip_SAVE!=None) {
		XFreePixmap(XtDisplay(hw), hw->html.bgclip_SAVE);
		hw->html.bgclip_SAVE=None;
	}
}

/*
 * Find closest color by allocating it, or picking an already allocated color
 */
void FindColor(Display *dsp, Colormap colormap, XColor *colr)
{
	int i, match;
#ifdef MORE_ACCURATE
	double rd, gd, bd, dist, mindist;
#else
	int rd, gd, bd, dist, mindist;
#endif /* MORE_ACCURATE */
	int cindx;
	XColor tempcolr;
	static XColor def_colrs[256];
	static int have_colors = 0;
	int NumCells;

        tempcolr.pixel=colr->pixel;  
        tempcolr.red=colr->red;      
        tempcolr.green=colr->green;  
        tempcolr.blue=colr->blue;    
        tempcolr.flags=colr->flags;  
        tempcolr.pad=colr->pad;  

	match = XAllocColor(dsp, colormap, colr);
	if (match == 0) {
                colr->pixel=tempcolr.pixel;
                colr->red=tempcolr.red;
                colr->green=tempcolr.green;
                colr->blue=tempcolr.blue;  
                colr->flags=tempcolr.flags;
                colr->pad=tempcolr.pad;

		NumCells = DisplayCells(dsp, DefaultScreen(dsp));
		if (!have_colors) {
			for (i=0; i<NumCells; i++)
				def_colrs[i].pixel = i;
			XQueryColors(dsp, colormap, def_colrs, NumCells);
			have_colors = 1;
		}
#ifdef MORE_ACCURATE
		mindist = 196608.0;		/* 256.0 * 256.0 * 3.0 */
		cindx = -1;
		for (i=0; i<NumCells; i++) {
			rd = (def_colrs[i].red - colr->red) / 256.0;
			gd = (def_colrs[i].green - colr->green) / 256.0;
			bd = (def_colrs[i].blue - colr->blue) / 256.0;
			dist = (rd * rd) + (gd * gd) + (bd * bd);
			if (dist < mindist) {
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0.0)
					break;
			}
		}
#else
		mindist = 196608;		/* 256 * 256 * 3 */
		cindx = -1;
		for (i=0; i<NumCells; i++) {
			rd=((int)(def_colrs[i].red >> 8) - (int)(colr->red >> 8));
			gd=((int)(def_colrs[i].green>>8)-(int)(colr->green >> 8));
			bd=((int)(def_colrs[i].blue >> 8)-(int)(colr->blue >> 8));
			dist = (rd * rd) + (gd * gd) + (bd * bd);
			if (dist < mindist) {
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0)
					break;
			}
		}
#endif /* MORE_ACCURATE */
                if (cindx==(-1)) {   
                        colr->pixel=BlackPixel(dsp, DefaultScreen(dsp));
                        colr->red = colr->green = colr->blue = 0;
                } else {               
                        colr->pixel = cindx;
                        colr->red = def_colrs[cindx].red;
                        colr->green = def_colrs[cindx].green;
                        colr->blue = def_colrs[cindx].blue;
                } 
	} else {
			/* Keep a count of how many times we have allocated the
			 * same color, so we can properly free them later.
			 */
		allocation_index[colr->pixel]++;
			/* If this is a new color, we've actually changed default
			 * colormap, and may have to re-query it later.
			 */
		if (allocation_index[colr->pixel] == 1)
			have_colors = 0;
	}
}

/*
 * returns position of highest set bit in 'ul' as an integer (0-31),
 * or -1 if none.
 */
static int highbit( unsigned long ul)
{
	int i;

	for (i=31; ((ul&0x80000000) == 0) && i>=0;  i--, ul<<=1);
	return i;
}


/*
#ifndef NEW
#define NEW                          
#endif                               
*/                                   
                                     
#ifdef NEW                           
extern int bits_per_pixel(int dpy, int depth); /*this is in ../src/pixmaps.c*/
#endif   
/*
 * Rescale an image GD 24 Apr 97 #######
 * from the XV Software 3.10a . See the copyright notice of xv-3.10a 
 */

ImageInfo * RescalePic(HTMLWidget hw, ImageInfo * picd, int nw, int nh)
{ 
	int          cy,ex,ey,*cxarr, *cxarrp;
	unsigned char *clptr,*elptr,*epptr, *epic;
	int bperpix;                      


	if ( !picd->image_data)
		return picd;
/* change image_data width height in picd*/

	clptr = NULL;  cxarrp = NULL;  cy = 0;  /* shut up compiler */
/* fprintf(stderr,"RescalePic(%d,%d) picd.(w,h)=%d,%d\n",
			nw,nh,picd->width,picd->height);
*/
	bperpix =  1 ;	 /*    bperpix = (picType == PIC8) ? 1 : 3; */
                                      
/* create a new pic of the appropriate size */
	epic = (unsigned char *) malloc((size_t) (nw * nh * bperpix));
	cxarr = (int *) malloc(nw * sizeof(int));
	if (!epic || !cxarr) {
		fprintf(stderr,"memory overflow\n");
		exit(1);
	}
/* the scaling routine.  not really all that scary after all... */
/* OPTIMIZATON:  Malloc an nw array of ints which will hold the
/* values of the equation px = (pWIDE * ex) / nw.  Faster than doing
/* a mul and a div for every point in picture */
                                      
	for (ex=0; ex<nw; ex++)        
		cxarr[ex] = bperpix * ((picd->width * ex) / nw);
	elptr = epptr = epic;             
	for (ey=0;  ey<nh;  ey++, elptr+=(nw*bperpix)) {
		cy = (picd->height * ey) / nh;      
		epptr = elptr;
		clptr = picd->image_data + (cy * picd->width * bperpix);
/*		if (bperpix == 1) {             */
			for (ex=0, cxarrp = cxarr;  ex<nw;  ex++, epptr++)
				*epptr = clptr[*cxarrp++];  
/*		} else {                          */
/*			int j;  unsigned char *cp;  */
/*			for (ex=0, cxarrp = cxarr; ex<nw; ex++,cxarrp++) {*/
/*				cp = clptr + *cxarrp;       */
/*				for (j=0; j<bperpix; j++)   */
/*					*epptr++ = *cp++;         */
/*			}                             */
/*		}                               */
	}                                 
	free(cxarr);
/* at this point, we have a raw epic.  Potentially dither it */
/*      free(picd->image_data); ###### trouver un moyen pour faire free */
	picd->image_data = epic;
	picd->width = nw;
	picd->height = nh;
	return picd;
}
/*######################*/

/*
 * Make am image of appropriate depth for display from image data.
 */
XImage * MakeImage( Display *dsp, unsigned char *data,
	int width, int height,
	int depth, ImageInfo *img_info, int clip)
{
	int linepad, shiftnum;
	int shiftstart, shiftstop, shiftinc;
	int bytesperline,bpp;
	int temp;
	int w, h;
	XImage *newimage;
	unsigned char *bit_data, *bitp, *datap;
	Visual *theVisual;
	int bmap_order;
	unsigned long c;
	int rshift, gshift, bshift;

#ifdef NEW                           
        switch(bpp=bits_per_pixel(dsp,depth))
#else                                
        switch(depth)                
#endif 
	{
	case 6:
	case 8:
		bit_data = (unsigned char *)malloc(width * height);
		memcpy(bit_data, data, (width * height));
		bytesperline = width;
		if (clip)
			depth = 1;
		newimage =XCreateImage(dsp,DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 8, bytesperline);
		break;
	case 1:
	case 2:
	case 4:
		if (BitmapBitOrder(dsp) == LSBFirst) {
			shiftstart = 0;
			shiftstop = 8;
#ifndef NEW                          
                        shiftinc = depth; 
#else                                
                        shiftinc = bpp;
#endif
		} else {
#ifndef NEW                          
                        shiftstart = 8 - depth;
                        shiftstop = -depth;
                        shiftinc = -depth; 
#else                                
                        shiftstart = 8 - bpp;
                        shiftstop = -bpp; 
                        shiftinc = -bpp;  
#endif
		}
		linepad = 8 - (width % 8);
		bit_data =(unsigned char *)malloc(((width+linepad)*height) + 1);
		bitp = bit_data;
		datap = data;
		*bitp = 0;
		shiftnum = shiftstart;
		for (h=0; h<height; h++) {
			for (w=0; w<width; w++) {
				temp = *datap++ << shiftnum;
				*bitp = *bitp | temp;
				shiftnum = shiftnum + shiftinc;
				if (shiftnum == shiftstop) {
					shiftnum = shiftstart;
					bitp++;
					*bitp = 0;
				}
			}
			for (w=0; w<linepad; w++) {
				shiftnum = shiftnum + shiftinc;
				if (shiftnum == shiftstop) {
					shiftnum = shiftstart;
					bitp++;
					*bitp = 0;
				}
			}
		}
#ifndef NEW                          
                bytesperline = (width + linepad) * depth / 8;
#else                                
                bytesperline = (width + linepad) * bpp / 8;
#endif
		newimage =XCreateImage(dsp,DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			(width + linepad), height, 8, bytesperline);
		break;
/*
 * WARNING:  This depth 16 code is donated code for 16 but
 * TrueColor displays.  I have no access to such displays, so I
 * can't really test it.
 * Donated by - nosmo@ximage.com
 */
	case 16:
		bit_data = (unsigned char *)malloc(width * height * 2);
		bitp = bit_data;
		datap = data;
		theVisual = DefaultVisual(dsp, DefaultScreen(dsp));
		rshift = 15 - highbit(theVisual->red_mask);
		gshift = 15 - highbit(theVisual->green_mask);
		bshift = 15 - highbit(theVisual->blue_mask);
		bmap_order = BitmapBitOrder(dsp);
     
		for (w = (width * height); w > 0; w--) {
			temp = (((img_info->reds[(int)*datap] >> rshift) & 
				 theVisual->red_mask) |
				((img_info->greens[(int)*datap] >> gshift) & 
				 theVisual->green_mask) |
				((img_info->blues[(int)*datap] >> bshift) & 
				 theVisual->blue_mask));
			if (bmap_order == MSBFirst) {
				*bitp++ = (temp >> 8) & 0xff;
				*bitp++ = temp & 0xff;
			} else {
				*bitp++ = temp & 0xff;
				*bitp++ = (temp >> 8) & 0xff;
			}
			datap++;
		}
		newimage =XCreateImage(dsp,DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 16, 0);
		break;
	case 24:
#ifdef NEW                           
            case 32:                 
#endif
		bit_data = (unsigned char *)malloc(width * height * 4);
		theVisual = DefaultVisual(dsp, DefaultScreen(dsp));
		rshift = highbit(theVisual->red_mask) - 7;
		gshift = highbit(theVisual->green_mask) - 7;
		bshift = highbit(theVisual->blue_mask) - 7;
		bmap_order = BitmapBitOrder(dsp);
		bitp = bit_data;
		datap = data;
		for (w = (width * height); w > 0; w--) {
		    c = (((img_info->reds[(int)*datap] >> 8) & 0xff) << rshift) |
		     (((img_info->greens[(int)*datap] >> 8) & 0xff) << gshift) |
		     (((img_info->blues[(int)*datap] >> 8) & 0xff) << bshift);

			datap++;
			if (bmap_order == MSBFirst) {
				*bitp++ = (unsigned char)((c >> 24) & 0xff);
				*bitp++ = (unsigned char)((c >> 16) & 0xff);
				*bitp++ = (unsigned char)((c >> 8) & 0xff);
				*bitp++ = (unsigned char)(c & 0xff);
			} else {
				*bitp++ = (unsigned char)(c & 0xff);
				*bitp++ = (unsigned char)((c >> 8) & 0xff);
				*bitp++ = (unsigned char)((c >> 16) & 0xff);
				*bitp++ = (unsigned char)((c >> 24) & 0xff);
			}
		}
		newimage =XCreateImage(dsp,DefaultVisual(dsp, DefaultScreen(dsp)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 32, 0);
		break;
	default:
		fprintf(stderr, 
			"Don't know how to format image for display of depth %d\n", depth);
		return(NULL);
	}
	return(newimage);
}

int AnchoredHeight(HTMLWidget hw)
{
	return((int)(AnchoredImage_height + IMAGE_DEFAULT_BORDER));
}

char * IsMapForm(HTMLWidget hw)
{
	char *str;

	str = (char *)malloc(strlen("ISMAP Form") + 1);
	if (str != NULL)
		strcpy(str, "ISMAP Form");
	return(str);
}

int IsIsMapForm(HTMLWidget hw, char *href)
{
	if ((href != NULL)&&(strcmp(href, "ISMAP Form") == 0))
		return(1);
	return(0);
}

char * DelayedHRef( HTMLWidget hw)
{
	char *str;

	str = (char *)malloc(strlen("Delayed Image") + 1);
	if (str != NULL)
		strcpy(str, "Delayed Image");
	return(str);
}

int IsDelayedHRef(HTMLWidget hw, char *href)
{
	if ((href != NULL)&&(strcmp(href, "Delayed Image") == 0))
		return(1);
	return(0);
}

Pixmap DelayedImage( HTMLWidget hw, Boolean anchored)
{
        Pixmap pix;

        if (delayed_image.image == (Pixmap)NULL) {
		delayed_image.transparent=0;
                delayed_image.image = XCreatePixmapFromBitmapData(
                        XtDisplay(hw->html.view),
                        XtWindow(hw->html.view), (char*) DelayedImage_bits,
                        DelayedImage_width, DelayedImage_height,
                        hw->manager.foreground,
                        hw->core.background_pixel,
                        DefaultDepthOfScreen(XtScreen(hw)));
	}
        if ((anchored == True)&&(anchored_image.image == (Pixmap)NULL)) {
		anchored_image.transparent=0;
                anchored_image.image = XCreatePixmapFromBitmapData(
                        XtDisplay(hw->html.view),
                        XtWindow(hw->html.view), (char*) AnchoredImage_bits,
                        AnchoredImage_width, AnchoredImage_height,
                        hw->manager.foreground,
                        hw->core.background_pixel,
                        DefaultDepthOfScreen(XtScreen(hw)));
                pix = XCreatePixmap( XtDisplay(hw->html.view),
                        XtWindow(hw->html.view), DelayedImage_width,
                        (DelayedImage_height + AnchoredImage_height +
                                IMAGE_DEFAULT_BORDER),
                        DefaultDepthOfScreen(XtScreen(hw)));
                XSetForeground(XtDisplay(hw), hw->html.drawGC,
                        hw->core.background_pixel);
                XFillRectangle(XtDisplay(hw->html.view), pix,
                        hw->html.drawGC, 0, 0,
                        DelayedImage_width,
                        (DelayedImage_height + AnchoredImage_height +
                                IMAGE_DEFAULT_BORDER));
                XCopyArea(XtDisplay(hw->html.view),
                        anchored_image.image, pix, hw->html.drawGC,
                        0, 0, AnchoredImage_width, AnchoredImage_height,
                        0, 0);
                XCopyArea(XtDisplay(hw->html.view),
                        delayed_image.image, pix, hw->html.drawGC,
                        0, 0, DelayedImage_width, DelayedImage_height,
                        0, (AnchoredImage_height + IMAGE_DEFAULT_BORDER));
                XFreePixmap(XtDisplay(hw->html.view), anchored_image.image);
                anchored_image.image = pix;
                return(anchored_image.image);
	}
        return(delayed_image.image);
}

ImageInfo * DelayedImageData( Boolean anchored)
{
	delayed_image.delayed = 1;
	delayed_image.internal = 0;
	delayed_image.fetched = 0;
	delayed_image.width = DelayedImage_width;
	delayed_image.height = DelayedImage_height;
	delayed_image.num_colors = 0;
	delayed_image.reds = NULL;
	delayed_image.greens = NULL;
	delayed_image.blues = NULL;
	delayed_image.image_data = NULL;
	delayed_image.clip_data = NULL;
	delayed_image.image = None;
	delayed_image.clip = None;
	delayed_image.transparent = 0;

	if (anchored == True) {
		anchored_image.delayed = 0;
		anchored_image.internal = 0;
		anchored_image.fetched = 0;
		anchored_image.width = DelayedImage_width;
		anchored_image.height = DelayedImage_height +
			AnchoredImage_height + IMAGE_DEFAULT_BORDER;
		anchored_image.num_colors = 0;
		anchored_image.reds = NULL;
		anchored_image.greens = NULL;
		anchored_image.blues = NULL;
		anchored_image.image_data = NULL;
		anchored_image.image = None;
		anchored_image.clip_data = NULL;
		anchored_image.clip = None;
		anchored_image.transparent = 0;
		return(&anchored_image);
	}
	return(&delayed_image);
}

ImageInfo * NoImageData( HTMLWidget hw)
{
	no_image.delayed = 0;
	no_image.internal = 0;
	no_image.fetched = 0;
	no_image.width = NoImage_width;
	no_image.height = NoImage_height;
	no_image.num_colors = 0;
	no_image.reds = NULL;
	no_image.greens = NULL;
	no_image.blues = NULL;
	no_image.image_data = NULL;
	no_image.clip_data = NULL;
	no_image.image = None;
	no_image.clip = None;
	no_image.transparent=0;
	if (no_image.image == (Pixmap)NULL) {
		no_image.image = XCreatePixmapFromBitmapData( XtDisplay(hw),
			XtWindow(hw->html.view), (char*) NoImage_bits,
			NoImage_width, NoImage_height,
                        hw->manager.foreground,
			hw->core.background_pixel,
			DefaultDepthOfScreen(XtScreen(hw)));
	}
	return(&no_image);
}

Pixmap InfoToImage( HTMLWidget hw, ImageInfo *img_info, int clip)
{
	int i, size;
	int delta, not_right_col, not_last_row;
	Pixmap Img;
	XImage *tmpimage;
	XColor tmpcolr;
	int *Mapping;
	unsigned char *tmpdata;
	unsigned char *ptr;
	unsigned char *ptr2;
	int Vclass;
	XVisualInfo vinfo, *vptr;
	Boolean need_to_dither;
	unsigned long black_pixel;
	unsigned long white_pixel;
	int depth;

	/* find the visual class. */
	vinfo.visualid = XVisualIDFromVisual(DefaultVisual(XtDisplay(hw),
		DefaultScreen(XtDisplay(hw))));
	vptr = XGetVisualInfo(XtDisplay(hw), VisualIDMask, &vinfo, &i);

#if defined(__cplusplus) || defined(c_plusplus)
	Vclass = vptr->c_class;          /* C++ */
#else
  	Vclass = vptr->class;
#endif

	depth=vptr->depth;
	if (clip) {
		need_to_dither = False;
	} else 
		if (vptr->depth == 1) {
			need_to_dither = True;
			black_pixel = BlackPixel(XtDisplay(hw),
					DefaultScreen(XtDisplay(hw)));
			white_pixel = WhitePixel(XtDisplay(hw),
					DefaultScreen(XtDisplay(hw)));
		} else {
			need_to_dither = False;
		}
	XFree((char *)vptr);
	Mapping = (int *)malloc(img_info->num_colors * sizeof(int));
	if (!clip) {
		for (i=0; i < img_info->num_colors; i++) {
			tmpcolr.red = img_info->reds[i];
			tmpcolr.green = img_info->greens[i];
			tmpcolr.blue = img_info->blues[i];
			tmpcolr.flags = DoRed|DoGreen|DoBlue;
			if ((Vclass == TrueColor) || (Vclass == DirectColor)) {
				Mapping[i] = i;
			} else if (need_to_dither == True) {
				Mapping[i] = ((tmpcolr.red>>5)*11 +
					      (tmpcolr.green>>5)*16 +
					      (tmpcolr.blue>>5)*5) / (65504/64);
			} else {
				FindColor(XtDisplay(hw), (installed_colormap ?
					   installed_cmap :
					   DefaultColormapOfScreen(XtScreen(hw))),
					  &tmpcolr);
				Mapping[i] = tmpcolr.pixel;
			}
		}
	}
	/*
	 * Special case:  For 2 color non-black&white images, instead
	 * of 2 dither patterns, we will always drop them to be
	 * black on white.
	 */
	if ((need_to_dither == True)&&(img_info->num_colors == 2)) {
		if (Mapping[0] < Mapping[1]) {
			Mapping[0] = 0;
			Mapping[1] = 64;
		} else {
			Mapping[0] = 64;
			Mapping[1] = 0;
		}
	}
	size = img_info->width * img_info->height;
	if (size == 0) {
		tmpdata = NULL;
	} else {
		tmpdata = (unsigned char *)malloc(size);
	}
	if (tmpdata == NULL) {
		tmpimage = NULL;
		Img = (Pixmap)NULL;
	} else {
		if (clip) {
			ptr = img_info->clip_data;
		} else {
			ptr = img_info->image_data;
		}
		ptr2 = tmpdata;
		if (need_to_dither == True) {
			int cx, cy;

			if (clip) {
				for (ptr2 = tmpdata, ptr = img_info->clip_data;
				     ptr2 < tmpdata+(size-1); ptr2++, ptr++) {
					*ptr2 = Mapping[(int)*ptr];
				}
			} else {
				for (ptr2 = tmpdata, ptr = img_info->image_data;
				     ptr2 < tmpdata+(size-1); ptr2++, ptr++) {
					*ptr2 = Mapping[(int)*ptr];
				}
			}
			ptr2 = tmpdata;
			for (cy=0; cy < img_info->height; cy++) {
				for (cx=0; cx < img_info->width; cx++) {
					/*
					 * Assume high numbers are really negative
					 */
					if (*ptr2 > 128)
						*ptr2 = 0;
					if (*ptr2 > 64)
						*ptr2 = 64;
					/*
					 * Traditional Floyd-Steinberg
					 */
					if (*ptr2 < 32) {
						delta = *ptr2;
						*ptr2 = black_pixel;
					} else {
						delta = *ptr2 - 64;
						*ptr2 = white_pixel;
					}
					if (not_right_col =
					     (cx < (img_info->width-1))) {
						*(ptr2+1) += delta*7 >> 4;
					}
					if (not_last_row =
					     (cy < (img_info->height-1))) {
						(*(ptr2+img_info->width)) +=
							delta*5 >> 4;
					}
					if (not_right_col && not_last_row) {
						(*(ptr2+img_info->width+1)) +=
							delta >> 4;
					}
					if (cx && not_last_row) {
						(*(ptr2+img_info->width-1)) +=
							delta*3 >> 4;
					}
					ptr2++;
				}
			}
		} /* end if (need_to_dither==True) */
		else {
			for (i=0; i < size; i++) {
				if (clip) { 
					*ptr2++ = *ptr;
				} else {
				      *ptr2++ =(unsigned char)Mapping[(int)*ptr];
				}
				ptr++;
			}
		}
		depth=DefaultDepthOfScreen(XtScreen(hw));
		tmpimage = MakeImage(XtDisplay(hw), tmpdata,
			img_info->width, img_info->height,
			depth, img_info, clip);
                /* Caught by Purify; should be OK. */
                free (tmpdata);
		Img = XCreatePixmap(XtDisplay(hw), XtWindow(hw->html.view),
			img_info->width, img_info->height, depth);
	}
	if ((tmpimage == NULL)||(Img == (Pixmap)NULL)) {
		if (tmpimage != NULL)
			XDestroyImage(tmpimage);
		if (Img != (Pixmap)NULL)
			XFreePixmap(XtDisplay(hw), Img);
		img_info->width = NoImage_width;
		img_info->height = NoImage_height;
		Img = (NoImageData(hw))->image;
	} else {
		XPutImage(XtDisplay(hw), Img, hw->html.drawGC, tmpimage, 0, 0,
			0, 0, img_info->width, img_info->height);
		XDestroyImage(tmpimage);
	}
        /* Caught by Purify; should be OK. */
        free((char *)Mapping);
	return(Img);
}


/* Place an image. Add an element record for it. */
void ImagePlace(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext *pcc)
{
	char *tptr,*srcPtr;
	char *wTmp,*hTmp;
	int border_width = IMAGE_DEFAULT_BORDER;
	struct ele_rec *eptr;
	int width=0;
	int height = 0;
	int baseline =0;
	ImageInfo * picd;
	ValignType valignment;
	int extra=0;

	srcPtr = ParseMarkTag(mptr->start, MT_IMAGE, "SRC");

	/* now initialise the image part */
	picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
	picd->image_data = NULL;
        picd->ismap = 0;
        picd->fptr = NULL;
        picd->internal = 0;
        picd->delayed = 0;
        picd->fetched = 0;
        picd->cached = 0;
        picd->width = 0;
	picd->height = 0;
        picd->num_colors = 0;
        picd->reds = NULL;
        picd->greens = NULL;
        picd->blues = NULL;
        picd->clip_data = NULL;
        picd->transparent = 0;
        picd->image = (Pixmap)NULL;
        picd->clip = (Pixmap)NULL;
        picd->src = NULL;
        picd->wtype = MC_MO_TYPE_UNICAST;
        picd->internal_numeo = 0;
	picd->map = NULL;
	picd->usemap = NULL;

	tptr=ParseMarkTag(mptr->start, MT_IMAGE, "USEMAP");
	if (tptr!=NULL) {     
		if (*tptr) {  
			picd->usemap=tptr;
		} else {        
			free(tptr); 
		}             
	}                  
	if (srcPtr){
		picd->src=strdup(srcPtr);
		if (picd->src == NULL) {
			MEM_OVERFLOW;
		}
		free(srcPtr);
	}

	/* Picture stuff if we have an image resolver, use it. */
	picd->wtype = hw->html.mc_wtype;
	picd->internal_numeo = pcc->internal_mc_eo;
	picd->cw_only = pcc->cw_only;
	XtCallCallbackList ((Widget)hw, hw->html.image_callback,
                                (XtPointer)picd);
	if(!picd->internal)pcc->internal_mc_eo++;

	if (!picd->fetched ) {
		free(picd);
		picd = NoImageData(hw);
		picd->internal = 0;
	}

	wTmp = ParseMarkTag(mptr->start, MT_IMAGE, "WIDTH");
	if (wTmp && *wTmp){
		if ((width=atoi(wTmp))<0)
			width=0;
		free(wTmp);
	}
	if (width == 0)
		width = picd->width ;
	hTmp = ParseMarkTag(mptr->start, MT_IMAGE, "HEIGHT");
	if (hTmp && *hTmp){
		if ((height=atoi(hTmp))<=0)
			height=0;
		baseline=height;
		free(hTmp);
	}
	if(height == 0)
		height = picd->height;
/*######## rescale  image */
	if((width != picd->width ) || (height != picd->height)) { /* rescale*/
		ImageInfo * n_picd;

		n_picd = RescalePic(hw, picd, width, height);
/*#### liberer picd */
		picd = n_picd;
	}

/*#######################*/
	if(baseline == 0)
		baseline = height;
	tptr = ParseMarkTag(mptr->start, MT_IMAGE, "BORDER");
	if (tptr && *tptr){
		if ((border_width=atoi(tptr))<0)
			border_width=0;
		free(tptr);
	}
	if ((pcc->anchor_tag_ptr->anchor_href != NULL)&& (!picd->internal)) {
		extra = 2*border_width;
	} 
	width = width + extra;
	height = height + extra;
	baseline = baseline + extra;
	/* Check if this image will be top aligned */
	tptr = ParseMarkTag(mptr->start, MT_IMAGE, "ALIGN");
	if (caseless_equal(tptr, "TOP")) {
		valignment = ALIGN_TOP;
		baseline =0;
	} else if (caseless_equal(tptr, "MIDDLE")) {
		valignment = ALIGN_MIDDLE;
		baseline = baseline/2;
	} else {
		valignment = ALIGN_BOTTOM;
	}
	/* Clean up parsed ALIGN string */
	if (tptr != NULL)
		free(tptr);

	 /* Now look if the image is too wide, if so insert a linebreak. */

	if (!pcc->preformat) {
		if ( (pcc->x + width) >
		     (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)) 
			LinefeedPlace(hw, mptr, pcc);
	}
	if (picd->fetched ) {
		tptr = ParseMarkTag(mptr->start, MT_IMAGE, "ISMAP");
		if (tptr != NULL) {
			free(tptr);
			picd->ismap = 1;
		}
	}
        if(pcc->computed_min_x < (width+pcc->eoffsetx+pcc->left_margin)){
                pcc->computed_min_x = width + pcc->eoffsetx + pcc->left_margin;
        }               
        if (pcc->x + width > pcc->computed_max_x)
                pcc->computed_max_x = pcc->x + width;

	if (!pcc->cw_only){
		eptr = CreateElement(hw, E_IMAGE, pcc->cur_font,
				pcc->x, pcc->y, width, height, baseline, pcc);
		eptr->underline_number = 0; /* Images can't be underlined! */
		eptr->anchor_tag_ptr = pcc->anchor_tag_ptr;
		/* check the max line height. */
		AdjustBaseLine(hw,eptr,pcc);
		eptr->pic_data=picd;
		eptr->bwidth=border_width ;
	} else {
		if (pcc->cur_line_height < height)
			pcc->cur_line_height = height;
	}

/* update pcc */
	pcc->have_space_after = 0;
	pcc->x = pcc->x + width ;
	pcc->is_bol = False;
	if (!pcc->preformat) {
		if (pcc->x >
		    (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)) 
			LinefeedPlace(hw, mptr, pcc);
	}
	/* Check if this image has the ISMAP attribute, so we know the
	 * x,y coordinates of the image click are important.
	 * Due to a special case (see below), this code can acutally
	 * change the size, or anchor status of the image, thus we MUST
	 * doit before we muck with the Baseline and stuff.
	 */
#ifdef TO_DO
	if (picd->ismap) {
		/* SUPER SPECIAL CASE!  (Thanks Marc)
		 * If you have an ISMAP image inside a form, And that form
		 * doesn't already have an HREF by being inside an anchor,
		 * (Being a DelayedHRef is considered no href)
		 * clicking in that image will submit the form,
		 * adding the x,y coordinates of the click as part
		 * of the list of name/value pairs.
		 */
/*		if((pcc->cur_form != NULL)&&
/*		   ((eptr->anchor_tag_ptr->anchor_href == NULL)||
/*		   (IsDelayedHRef( hw, eptr->anchor_tag_ptr->anchor_href)))) {
/*			picd->fptr = pcc->cur_form;
/*			eptr->anchor_tag_ptr->anchor_href = IsMapForm(hw);
/*			eptr->fg = hw->html.anchor_fg;
/*		}
*/
	}
#endif
}

/*
 * Redraw a formatted image element.
 * The color of the image border reflects whether it is an active anchor or not.
 * Actual Pixmap creation was put off until now to make sure we
 * had a window.  If it hasn't been already created, make the Pixmap now.
 */
void ImageRefresh(HTMLWidget hw, struct ele_rec *eptr)
{
	unsigned long valuemask;
	XGCValues values;
	int x, y, extra,baseline;

	x = eptr->x;
	y = eptr->y;
	baseline = eptr->baseline;
	extra = 0;
	if ((eptr->anchor_tag_ptr->anchor_href != NULL)&&
	   (!eptr->pic_data->internal)) {
			extra = eptr->bwidth;
	}
	x = x - hw->html.scroll_x;
	y = y - hw->html.scroll_y;
	XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
	XSetBackground(XtDisplay(hw), hw->html.drawGC, eptr->bg);
	if (extra) {
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view), 
				hw->html.drawGC,
				x, y,
				eptr->width, extra);
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view), 
				hw->html.drawGC,
				x, (y + eptr->pic_data->height + extra),
				eptr->width, extra);
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
				hw->html.drawGC,
				x, y,
				extra, (eptr->pic_data->height + (2 * extra)));
		XFillRectangle(XtDisplay(hw), XtWindow(hw->html.view),
				hw->html.drawGC,
				(x + eptr->width - extra), y,
				extra, (eptr->pic_data->height + (2 * extra)));
	}
	if (eptr->pic_data->image == (Pixmap)NULL) {
/*#############*/
		eptr->pic_data->image = InfoToImage(hw,eptr->pic_data, 0);
		if(eptr->pic_data->image == (Pixmap)NULL){
			if (eptr->pic_data->delayed) {
				eptr->pic_data->image = DelayedImage(hw, True);
				eptr->pic_data->image = DelayedImage( hw, False);
			} else {
				eptr->pic_data->image = (NoImageData(hw))->image;
			}
		} else {
			if (eptr->pic_data->transparent && eptr->pic_data->clip==None) {
				eptr->pic_data->clip =XCreatePixmapFromBitmapData(
						XtDisplay(hw),
						XtWindow(hw->html.view),
						(char*) eptr->pic_data->clip_data,
						eptr->pic_data->width,
						eptr->pic_data->height,
						1, 0, 1);
			} else 
				if (!eptr->pic_data->transparent)
					eptr->pic_data->clip = None;
		}
/*#############*/
	}
	if (eptr->pic_data->image) {

		values.clip_mask=None;
		values.clip_x_origin=0;
		values.clip_y_origin=0;
		valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
		XChangeGC(XtDisplay(hw), hw->html.drawGC, valuemask, &values);

		if (eptr->pic_data->transparent) {
			values.clip_mask=eptr->pic_data->clip;
			values.clip_x_origin=x+extra;
			values.clip_y_origin=y+extra;
			valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
			XChangeGC(XtDisplay(hw), hw->html.drawGC,
				  valuemask, &values);
		} 
		XCopyArea(XtDisplay(hw), eptr->pic_data->image,
			  XtWindow(hw->html.view), hw->html.drawGC,
			  0, 0,
			  eptr->pic_data->width, eptr->pic_data->height,
			  (x + extra), (y + extra));
		values.clip_mask=None;
		values.clip_x_origin=0;
		values.clip_y_origin=0;
		valuemask=GCClipMask|GCClipXOrigin|GCClipYOrigin;
		XChangeGC(XtDisplay(hw), hw->html.drawGC, valuemask, &values);
	}
/*
/*	if((eptr->pic_data.delayed)&&
/*	   (eptr->anchor_tag_ptr->anchor_href != NULL)&&
/*	   (!IsDelayedHRef(hw, eptr->anchor_tag_ptr->anchor_href))&&
/*	   (!IsIsMapForm(hw, eptr->anchor_tag_ptr->anchor_href))) {
/*		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
/*		XFillRectangle(XtDisplay(hw->html.view),
/*			       XtWindow(hw->html.view),
/*			       hw->html.drawGC,
/*			       x, (y + AnchoredHeight(hw)),
/*			       (eptr->width + (2 * extra)), extra);
/*	}
*/
}
