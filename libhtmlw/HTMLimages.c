#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"
#include "../libmc/mc_defs.h"
#include "HTMLP.h"
#include "HTMLPutil.h"

#include "images/NoImage.xbm"
#include "images/DelayedImage.xbm"

#include "images/gopher_image.xbm"
#include "images/gopher_movie.xbm"
#include "images/gopher_menu.xbm"
#include "images/gopher_text.xbm"
#include "images/gopher_sound.xbm"
#include "images/gopher_index.xbm"
#include "images/gopher_telnet.xbm"
#include "images/gopher_binary.xbm"
#include "images/gopher_unknown.xbm"

static ImageInfo *gopher_image = NULL;
static ImageInfo *gopher_movie = NULL;
static ImageInfo *gopher_menu = NULL;
static ImageInfo *gopher_text = NULL;
static ImageInfo *gopher_sound = NULL;
static ImageInfo *gopher_index = NULL;
static ImageInfo *gopher_telnet = NULL;
static ImageInfo *gopher_binary = NULL;
static ImageInfo *gopher_unknown = NULL;

static ImageInfo *no_image = NULL;
static ImageInfo *delayed_image = NULL;

static int allocation_index[256];

extern int installed_colormap;
extern Colormap installed_cmap;

#define IMGINFO_FROM_BITMAP(x) \
{ \
	if (!x) { \
		x = (ImageInfo *)malloc (sizeof (ImageInfo)); \
		x->ismap = 0; \
		x->usemap = NULL; \
		x->width = x##_width; \
		x->height = x##_height; \
		x->req_width = -1;\
		x->req_height = -1;\
		x->image_data = NULL; \
		x->internal = 1; \
		x->delayed = 0; \
		x->fetched = 1; \
		x->cached =1; \
		x->num_colors = 0; \
		x->transparent = 0; \
		x->internal_numeo = -1; \
		x->image = XCreatePixmapFromBitmapData(XtDisplay(hw), \
			XtWindow(hw), \
			(char*) x##_bits,  \
			x##_width, x##_height, \
			fg_pixel, bg_pixel, DefaultDepthOfScreen(XtScreen(hw)));\
	}\
	picd->ismap = 0; \
	picd->usemap = NULL; \
	picd->width = x##_width; \
	picd->height = x##_height; \
	picd->req_width = -1;\
	picd->req_height = -1;\
	picd->image_data = NULL; \
	picd->internal = 1; \
	picd->delayed = 0; \
	picd->fetched = 1; \
	picd->cached =1; \
	picd->num_colors = 0; \
	picd->transparent = 0; \
	picd->internal_numeo = -1; \
	picd->image = x->image; \
}

/* definition for MedianCut */

#define RED     0
#define GREEN   1
#define BLUE    2

#define FindHash(red, green, blue, h_ptr) \
	h_ptr = Hash[((((red * 306) + (green * 601) + (blue * 117)) >> 10) * NCells) >> 16]; \
	while(h_ptr != NULL) { \
		if ((h_ptr->pixel[RED] == red)&& \
		    (h_ptr->pixel[GREEN] == green)&& \
		    (h_ptr->pixel[BLUE] == blue)) { \
			break; \
		} \
		h_ptr = h_ptr->hash_next; \
	}

static struct color_rec {
	int pixel[3];
	int box_num;
	struct color_rec *hash_next;
	struct color_rec *next;
} *Hash[256];

static struct c_box_rec {
	int min_pix[3];
	int max_pix[3];
	int count;
	struct color_rec *c_data;
} C_boxes[256];

static int BoxCount;
static struct color_rec *hash_ptr;
static struct color_rec *free_hash = (struct color_rec *)NULL;
static struct color_rec *tptr;
static int Width, Height;
static int ColorCnt;
static int NCells;

static void InitMinMax( int boxnum)
{
	C_boxes[boxnum].min_pix[RED] = 65536;
	C_boxes[boxnum].max_pix[RED] = 0;
	C_boxes[boxnum].min_pix[GREEN] = 65536;
	C_boxes[boxnum].max_pix[GREEN] = 0;
	C_boxes[boxnum].min_pix[BLUE] = 65536;
	C_boxes[boxnum].max_pix[BLUE] = 0;
}

struct color_rec *AddHash( int red, int green, int blue)
{
	int lum;

	lum = (((red * 306 + green * 601 + blue * 117) >> 10) * NCells) >> 16;;
	if (free_hash != NULL) {
		hash_ptr = free_hash;
		free_hash = free_hash->hash_next;
	} else {
		hash_ptr = (struct color_rec *)XtMalloc(sizeof(struct color_rec));
	}
	CHECK_OUT_OF_MEM(hash_ptr);

	hash_ptr->pixel[RED] = red;
	hash_ptr->pixel[GREEN] = green;
	hash_ptr->pixel[BLUE] = blue;
	hash_ptr->box_num = 0;
	hash_ptr->next = NULL;
	hash_ptr->hash_next = Hash[lum];
	Hash[lum] = hash_ptr;
	return(hash_ptr);
}

void AddColor( struct color_rec *cptr, int boxnum)
{
	struct color_rec *ptr;

	while (cptr != NULL) {
		ptr = cptr;
		cptr = cptr->next;
		ptr->box_num = boxnum;
		ptr->next = C_boxes[boxnum].c_data;
		C_boxes[boxnum].c_data = ptr;
		if (ptr->pixel[RED] < C_boxes[boxnum].min_pix[RED])
			C_boxes[boxnum].min_pix[RED] = ptr->pixel[RED];
		if (ptr->pixel[RED] > C_boxes[boxnum].max_pix[RED])
			C_boxes[boxnum].max_pix[RED] = ptr->pixel[RED];
		if (ptr->pixel[GREEN] < C_boxes[boxnum].min_pix[GREEN])
			C_boxes[boxnum].min_pix[GREEN] = ptr->pixel[GREEN];
		if (ptr->pixel[GREEN] > C_boxes[boxnum].max_pix[GREEN])
			C_boxes[boxnum].max_pix[GREEN] = ptr->pixel[GREEN];
		if (ptr->pixel[BLUE] < C_boxes[boxnum].min_pix[BLUE])
			C_boxes[boxnum].min_pix[BLUE] = ptr->pixel[BLUE];
		if (ptr->pixel[BLUE] > C_boxes[boxnum].max_pix[BLUE])
			C_boxes[boxnum].max_pix[BLUE] = ptr->pixel[BLUE];
	}
}

void CountColors( unsigned char *data, XColor *colrs, int *color_used)
{
	unsigned char *dptr;
	register int i;
	int red, green, blue;
	register struct color_rec *tptr;

	InitMinMax(0);
	C_boxes[0].c_data = NULL;
	tptr = C_boxes[0].c_data;
	ColorCnt = 0;

	for (i=0; i<256; i++)
		color_used[i] = 0;

	dptr = data;
	for (i=(Width * Height); i>0; i--) {
		color_used[*dptr] = 1;
		dptr++;
	}

	for (i=0; i<256; i++) {
		if (!color_used[i])
			continue;
		red = colrs[i].red;
		green = colrs[i].green;
		blue = colrs[i].blue;
		FindHash(red, green, blue, tptr);
		if (tptr == NULL) {
			tptr = AddHash(red, green, blue);
			tptr->next = NULL;
			AddColor(tptr, 0);
			ColorCnt++;
		}
	}
}


int FindTarget( int *tptr)
{
	int range, i, indx;

	range = 0;
	for (i=0; i<BoxCount; i++) {
		int rr, gr, br;

		rr = C_boxes[i].max_pix[RED] - C_boxes[i].min_pix[RED];
		gr = C_boxes[i].max_pix[GREEN] - C_boxes[i].min_pix[GREEN];
		br = C_boxes[i].max_pix[BLUE] - C_boxes[i].min_pix[BLUE];
		if (rr > range) {
			range = rr;
			*tptr = i;
			indx = RED;
		}
		if (gr > range) {
			range = gr;
			*tptr = i;
			indx = GREEN;
		}
		if (br > range) {
			range = br;
			*tptr = i;
			indx = BLUE;
		}
	}
	return(indx);
}


void SplitBox( int boxnum, int color_indx)
{
	struct color_rec *low, *high;
	struct color_rec *data;
	int med_cnt, split_val;
	int low_cnt, high_cnt;
	int Low_cnt, High_cnt;
	int Greater, Lesser;

	Greater = BoxCount++;
	Lesser = boxnum;
	InitMinMax(Lesser);
	InitMinMax(Greater);
	data = C_boxes[boxnum].c_data;
	med_cnt = C_boxes[boxnum].count / 2;
	C_boxes[Lesser].c_data = NULL;
	C_boxes[Greater].c_data = NULL;
	Low_cnt = 0;
	High_cnt = 0;
	while(med_cnt > 0) {
		if (data->pixel[color_indx] < data->next->pixel[color_indx]) {
			low = data;
			high = data->next;
			data = high->next;
		} else {
			high = data;
			low = data->next;
			data = low->next;
		}
		low->next = NULL;
		high->next = NULL;
		low_cnt = 1;
		high_cnt = 1;
		split_val = low->pixel[color_indx];
		while(data != NULL) {
			tptr = data;
			data = data->next;
			if (tptr->pixel[color_indx] > split_val) {
				tptr->next = high;
				high = tptr;
				high_cnt++;
			} else {
				tptr->next = low;
				low = tptr;
				low_cnt++;
			}
		} /* end while data->next != NULL */
		if (low_cnt <= med_cnt) {
			AddColor(low, Lesser);
			Low_cnt += low_cnt;
			med_cnt -= low_cnt;
			if (med_cnt == 0) {
				AddColor(high, Greater);
				High_cnt += high_cnt;
			}
			data = high;
		} else {
			AddColor(high, Greater);
			High_cnt += high_cnt;
			data = low;
		}
	} /* end while med_cnt */
	C_boxes[Lesser].count = Low_cnt;
	C_boxes[Greater].count = High_cnt;
}

void SplitColors( int e_cnt)
{
	if (ColorCnt < e_cnt) {
		int i;

		tptr = C_boxes[0].c_data;
		for (i=0; i<ColorCnt; i++) {
			hash_ptr = tptr;
			tptr = tptr->next;
			C_boxes[i].c_data = hash_ptr;
			C_boxes[i].count = 1;
			hash_ptr->box_num = i;
			hash_ptr->next = NULL;
		}
		BoxCount = ColorCnt;
	} else {
		BoxCount = 1;
		while (BoxCount < e_cnt) {
			int target, color_indx;
	
			target = 0;
			color_indx = 0;
			color_indx = FindTarget(&target);
			SplitBox(target, color_indx);
		}
	}
}

void ConvertData( unsigned char *data, XColor *colrs, int *colors_used)
{
	unsigned char *dptr;
	register int i/*, j*/;
	int red, green, blue;
	register struct color_rec *hash_ptr;
	int pixel_map[256];

	/*
	 * Generate translation map.
	 */
	for (i=0; i<256; i++) {
		if (!colors_used[i]) {
			continue;
		}
		red = colrs[i].red;
		green = colrs[i].green;
		blue = colrs[i].blue;
		FindHash(red, green, blue, hash_ptr);
		if (hash_ptr == NULL) { /* Unknown color */
			hash_ptr = Hash[0];
		}
		pixel_map[i] = hash_ptr->box_num;
	}
	dptr = data;
	for (i=(Width*Height); i>0; i--) {
		*dptr = (unsigned char)pixel_map[(int)*dptr];
		dptr++;
	}
}

void PrintColormap( int e_cnt, XColor *colrs)
{
	int i;

	for(i=0; i<BoxCount; i++) {
		int Tred, Tgreen, Tblue;
		int c_cnt;

		c_cnt = 0;
		Tred = Tgreen = Tblue = 0;
		tptr = C_boxes[i].c_data;
		while (tptr != NULL) {
			Tred += tptr->pixel[RED];
			Tgreen += tptr->pixel[GREEN];
			Tblue += tptr->pixel[BLUE];
			c_cnt++;
			tptr = tptr->next;
		}
		colrs[i].red = Tred / c_cnt;
		colrs[i].green = Tgreen / c_cnt;
		colrs[i].blue = Tblue / c_cnt;
	}
	for(i=BoxCount; i<e_cnt; i++) {
		colrs[i].red = 0;
		colrs[i].green = 0;
		colrs[i].blue = 0;
	}
}

void MedianCut( unsigned char *data, int w, int h, XColor *colrs,
	int start_cnt, int end_cnt)
{
	int i;
	int colors_used[256];

	Width = w;
	Height = h;
	NCells = start_cnt;
	BoxCount = 0;
	ColorCnt = 0;
	for (i=0; i<256; i++) {
		Hash[i] = NULL;
		C_boxes[i].c_data = NULL;
		C_boxes[i].count = 0;
	}
	CountColors(data, colrs, colors_used);
	C_boxes[0].count = ColorCnt;
	SplitColors(end_cnt);
	ConvertData(data, colrs, colors_used);
	PrintColormap(end_cnt, colrs);
	for (i=0; i<256; i++) {
		hash_ptr = Hash[i];
		while (hash_ptr != NULL) {
			tptr = hash_ptr;
			hash_ptr = hash_ptr->hash_next;
			tptr->hash_next = free_hash;
			free_hash = tptr;
		}
	}
}

/*
 * Free all the colors in the default colormap that we have allocated so far.
 */
void FreeColors(Display *d, Colormap colormap)
{
	int i, j;
	unsigned long pix;

	for (i=0; i<256; i++) {
		if (allocation_index[i]) {
			pix = (unsigned long)i;
			/* Because X is stupid, we have to Free the color
			 * once for each time we've allocated it.
			 */
			for (j=0; j<allocation_index[i]; j++)
				XFreeColors(d, colormap, &pix, 1, 0L);
		}
		allocation_index[i] = 0;
	}
}

/* Find closest color by allocating it, or picking an already allocated color
 */
void FindColor(Display *d, Colormap colormap, XColor *colr)
{
	int i, match;
	int rd, gd, bd, dist, mindist;
	int cindx;
	XColor tempcolr;
	static XColor def_colrs[256];
	static int have_colors = 0;
	int NumCells;

        tempcolr.red=colr->red;      
        tempcolr.green=colr->green;  
        tempcolr.blue=colr->blue;    
        tempcolr.flags=colr->flags;  

	match = XAllocColor(d, colormap, colr);
	if (match == 0) {
                colr->red=tempcolr.red;
                colr->green=tempcolr.green;
                colr->blue=tempcolr.blue;  
                colr->flags=tempcolr.flags;

		NumCells = DisplayCells(d, DefaultScreen(d));
		if (!have_colors) {
			for (i=0; i<NumCells; i++)
				def_colrs[i].pixel = i;
			XQueryColors(d, colormap, def_colrs, NumCells);
			have_colors = 1;
		}
		mindist = 196608;		/* 256 * 256 * 3 */
		cindx = -1;
		for (i=0; i<NumCells; i++) {
			rd=(int)(def_colrs[i].red >> 8) - (int)(colr->red >> 8);
			gd=(int)(def_colrs[i].green>> 8)-(int)(colr->green >> 8);
			bd=(int)(def_colrs[i].blue >> 8)-(int)(colr->blue >> 8);
			dist = (rd * rd) + (gd * gd) + (bd * bd);
			if (dist < mindist) {
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0)
					break;
			}
		}
                if (cindx==(-1)) {   
                        colr->pixel=BlackPixel(d, DefaultScreen(d));
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

/* returns position of highest set bit in 'ul' as an integer (0-31),
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

static void RescalePic(HTMLWidget hw, ImageInfo * picd, int nw, int nh)
{ 
	int          cy,ex,ey,*cxarr, *cxarrp;
	unsigned char *clptr,*elptr,*epptr, *epic;
	int bperpix;                      

/* change image_data width height in picd*/

	clptr = NULL; cxarrp = NULL; cy = 0;  /* shut up compiler */
	bperpix =  1 ;			/* bperpix = (picType == PIC8) ? 1 : 3; */
                                      
/* create a new pic of the appropriate size */
	epic = (unsigned char *) malloc((size_t) (nw * nh * bperpix));
	cxarr = (int *) malloc(nw * sizeof(int));
	if (!epic || !cxarr) {
		fprintf(stderr,"memory overflow\n");
		exit(1);
	}
/* the scaling routine.  not really all that scary after all... */
/* OPTIMIZATON:  Malloc an nw array of ints which will hold the */
/* values of the equation px = (pWIDE * ex) / nw.  Faster than doing */
/* a mul and a div for every point in picture */
                                      
	for (ex=0; ex<nw; ex++)        
		cxarr[ex] = bperpix * ((picd->width * ex) / nw);
	elptr = epptr = epic;             
	for (ey=0;  ey<nh;  ey++, elptr+=(nw*bperpix)) {
		cy = (picd->height * ey) / nh;      
		epptr = elptr;
		clptr = picd->image_data + (cy * picd->width * bperpix);
/* if (bperpix == 1) {             */
		for (ex=0, cxarrp = cxarr;  ex<nw;  ex++, epptr++)
			*epptr = clptr[*cxarrp++];  
/* } else {                          */
/* 	int j;  unsigned char *cp;  */
/* 	for (ex=0, cxarrp = cxarr; ex<nw; ex++,cxarrp++) {*/
/* 		cp = clptr + *cxarrp;       */
/* 		for (j=0; j<bperpix; j++)   */
/* 			*epptr++ = *cp++;         */
/* 	}                             */
/* }                               */
	}                                 
	free(cxarr);

/* at this point, we have a raw epic.  Potentially dither it */
	free(picd->image_data);
	picd->image_data = epic;
	picd->width = nw;
	picd->height = nh;
}

/* Make am image of appropriate depth for display from image data.
 */
XImage * MakeImage( Display *d, unsigned char *data, int width, int height,
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
        switch(bpp=bits_per_pixel(d,depth)) {
#else                                
        switch(depth) {
#endif 
	case 6:
	case 8:
		bit_data = (unsigned char *)malloc(width * height);
		memcpy(bit_data, data, (width * height));
		bytesperline = width;
		if (clip)
			depth = 1;
		newimage =XCreateImage(d,DefaultVisual(d, DefaultScreen(d)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 8, bytesperline);
		break;
	case 1:
	case 2:
	case 4:
		if (BitmapBitOrder(d) == LSBFirst) {
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
		newimage =XCreateImage(d,DefaultVisual(d, DefaultScreen(d)),
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
		theVisual = DefaultVisual(d, DefaultScreen(d));
		rshift = 15 - highbit(theVisual->red_mask);
		gshift = 15 - highbit(theVisual->green_mask);
		bshift = 15 - highbit(theVisual->blue_mask);
		bmap_order = BitmapBitOrder(d);
     
		for (w = (width * height); w > 0; w--) {
			temp = (((img_info->colrs[(int)*datap].red >> rshift) & 
				 theVisual->red_mask) |
				((img_info->colrs[(int)*datap].green >> gshift) & 
				 theVisual->green_mask) |
				((img_info->colrs[(int)*datap].blue >> bshift) & 
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
		newimage =XCreateImage(d,DefaultVisual(d, DefaultScreen(d)),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 16, 0);
		break;
	case 24:
#ifdef NEW                           
            case 32:                 
#endif
		bit_data = (unsigned char *)malloc(width * height * 4);
		theVisual = DefaultVisual(d, DefaultScreen(d));
		rshift = highbit(theVisual->red_mask) - 7;
		gshift = highbit(theVisual->green_mask) - 7;
		bshift = highbit(theVisual->blue_mask) - 7;
		bmap_order = BitmapBitOrder(d);
		bitp = bit_data;
		datap = data;
		for (w = (width * height); w > 0; w--) {
		    c = (((img_info->colrs[(int)*datap].red >> 8) & 0xff) << rshift) |
		     (((img_info->colrs[(int)*datap].green >> 8) & 0xff) << gshift) |
		     (((img_info->colrs[(int)*datap].blue >> 8) & 0xff) << bshift);

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
		newimage =XCreateImage(d,DefaultVisual(d, DefaultScreen(d)),
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

ImageInfo * DelayedImageData( HTMLWidget hw)
{
        if (delayed_image == NULL) {
		delayed_image = (ImageInfo *)malloc (sizeof (ImageInfo));
		delayed_image->usemap = NULL;
		delayed_image->map = NULL;
		delayed_image->ismap = 0;
		delayed_image->fptr = NULL;

		delayed_image->internal = 1;
		delayed_image->delayed = 1;
		delayed_image->fetched = 0;
		delayed_image->cached = 0;
		delayed_image->width = DelayedImage_width;
		delayed_image->height = DelayedImage_height;
		delayed_image->num_colors = 0;
/*		delayed_image->colrs = NULL;	*/
		delayed_image->bg_index = 0;
		delayed_image->image_data = NULL;
		delayed_image->clip_data = NULL;
		delayed_image->transparent=0;
		delayed_image->image = None;
		delayed_image->clip = None;
		delayed_image->alt_text = NULL;
		delayed_image->src = NULL;
		delayed_image->internal_numeo = 0;
		delayed_image->cw_only = 0;
		delayed_image->transparent=0;
                delayed_image->image = XCreatePixmapFromBitmapData( XtDisplay(hw),
                        XtWindow(hw), (char*) DelayedImage_bits,
                        DelayedImage_width, DelayedImage_height,
                        hw->manager.foreground,
                        hw->core.background_pixel,
                        DefaultDepthOfScreen(XtScreen(hw)));
	}
	return(delayed_image);
}

ImageInfo * NoImageData( HTMLWidget hw)
{
	if (no_image == NULL) {
		no_image = (ImageInfo *)malloc (sizeof (ImageInfo));
		no_image->usemap = NULL;
		no_image->map = NULL;
		no_image->ismap = 0;
		no_image->fptr = NULL;

		no_image->internal = 1;
		no_image->delayed = 0;
		no_image->fetched = 0;
		no_image->cached = 1;
		no_image->width = NoImage_width;
		no_image->height = NoImage_height;
		no_image->num_colors = 0;
/*		no_image->colrs = NULL;		*/
		no_image->bg_index = 0;
		no_image->image_data = NULL;
		no_image->clip_data = NULL;
		no_image->transparent=0;
		no_image->image = None;
		no_image->clip = None;
		no_image->alt_text = NULL;
		no_image->src = NULL;
		no_image->internal_numeo = 0;
		no_image->cw_only = 0;
		no_image->image = XCreatePixmapFromBitmapData( XtDisplay(hw),
			XtWindow(hw), (char*) NoImage_bits,
			NoImage_width, NoImage_height,
                        hw->manager.foreground,
			hw->core.background_pixel,
			DefaultDepthOfScreen(XtScreen(hw)));
	}
	return(no_image);
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
			tmpcolr.red = img_info->colrs[i].red;
			tmpcolr.green = img_info->colrs[i].green;
			tmpcolr.blue = img_info->colrs[i].blue;
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
	tmpdata = (unsigned char *)malloc(size);
	CHECK_OUT_OF_MEM(tmpdata);
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
				if (not_right_col = (cx < (img_info->width-1))) {
					*(ptr2+1) += delta*7 >> 4;
				}
				if (not_last_row = (cy < (img_info->height-1))) {
					(*(ptr2+img_info->width)) += delta*5 >> 4;
				}
				if (not_right_col && not_last_row) {
					(*(ptr2+img_info->width+1)) += delta >> 4;
				}
				if (cx && not_last_row) {
					(*(ptr2+img_info->width-1)) += delta*3 >> 4;
				}
				ptr2++;
			}
		}
	} else { /* end if (need_to_dither==True) */
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
        free (tmpdata);
	Img = XCreatePixmap(XtDisplay(hw), XtWindow(hw->html.view),
			img_info->width, img_info->height, depth);

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
        free((char *)Mapping);
	return(Img);
}


static unsigned char nums[]={ 1, 2, 4, 8, 16, 32, 64, 128 };

void HtmlGetImage(HTMLWidget hw, ImageInfo * picd, PhotoComposeContext *pcc,
	int force_load)
{
	ImageInfo icbs;
	ImageInfo *dlim;
	ImageInfo *noim;
	int bg;
	unsigned short bg_red, bg_green, bg_blue;
	unsigned char * bg_map;
        int widthbyheight;
	int i, j, cnt, bcnt;
	int Used[256];
	unsigned char *bgptr;
	unsigned char *cptr;
	unsigned char *ptr;

	if (strncmp (picd->src, "internal-", 9) == 0) {    /* Internal images. */
		Pixel fg_pixel,bg_pixel;

		fg_pixel = hw->manager.foreground;
		bg_pixel = hw->core.background_pixel;

		if (strcmp (picd->src, "internal-gopher-image") == 0) {
			IMGINFO_FROM_BITMAP(gopher_image)
		} else if (strcmp (picd->src, "internal-gopher-movie") == 0) {
			IMGINFO_FROM_BITMAP(gopher_movie)
		} else if (strcmp (picd->src, "internal-gopher-menu") == 0) {
			IMGINFO_FROM_BITMAP(gopher_menu)
		} else if (strcmp (picd->src, "internal-gopher-text") == 0) {
			IMGINFO_FROM_BITMAP(gopher_text)
		} else if (strcmp (picd->src, "internal-gopher-sound") == 0) {
			IMGINFO_FROM_BITMAP(gopher_sound)
		} else if (strcmp (picd->src, "internal-gopher-index") == 0) {
			IMGINFO_FROM_BITMAP(gopher_index)
		} else if (strcmp (picd->src, "internal-gopher-telnet") == 0) {
			IMGINFO_FROM_BITMAP(gopher_telnet)
		} else if (strcmp (picd->src, "internal-gopher-binary") == 0) {
			IMGINFO_FROM_BITMAP(gopher_binary)
		} else if (strcmp (picd->src, "internal-gopher-unknown") == 0) {
			IMGINFO_FROM_BITMAP(gopher_unknown)
		} else {
			IMGINFO_FROM_BITMAP(gopher_unknown)
		}
/* we have internal image. Some pointer need to be freed */
		if (picd->alt_text){
			free(picd->alt_text);
			picd->alt_text = NULL;
		}
		if (picd->usemap){
			free(picd->usemap);
			picd->usemap = NULL;
		}
		return;
	}

	icbs = *picd ;
/* not internal, maybe delayed or in cache? */
	if(hw->html.delay_image_loads && !force_load) {
		icbs.look_only_cache = True;
		XtCallCallbackList((Widget)hw, hw->html.image_callback,
			(XtPointer) &icbs);
		if (!icbs.fetched) { /* the image is not cached. */
/* update picd from the delayed image */
			dlim = DelayedImageData(hw);
			picd->height = dlim->height;
			picd->width = dlim->width;
			picd->internal = 1;
			picd->delayed = 1;
			picd->fetched = 0;
			picd->image = dlim->image; 
			picd->internal_numeo = pcc->internal_mc_eo;
			pcc->internal_mc_eo++;
                	return;
		}
	} else {	/* load image from the net */
		icbs.look_only_cache = False;
		XtCallCallbackList ((Widget)hw, hw->html.image_callback,
                                (XtPointer) &icbs);
		if (!icbs.fetched) { /* Can't find image. Put NoImage in picd */
			noim = NoImageData(hw);
			picd->height = noim->height;
			picd->width = noim->width;
			picd->internal = 1;
			picd->delayed = 0;
			picd->fetched = 0;
			picd->image = noim->image; 
			picd->internal_numeo = pcc->internal_mc_eo;
			pcc->internal_mc_eo++;
                	return;
		}
	}
	if( (icbs.width * icbs.height == 0) ||
	    (icbs.image_data == NULL) ) {	/* size null ??? */
			noim = NoImageData(hw);
			picd->height = noim->height;
			picd->width = noim->width;
			picd->internal = 1;
			picd->delayed = 0;
			picd->fetched = 0;
			picd->image = noim->image; 
			picd->internal_numeo = pcc->internal_mc_eo;
			pcc->internal_mc_eo++;
                	return;
	}

/* here we have an image from the cache or the net */
/* data is in icbs */
	pcc->internal_mc_eo++;

/* field set to const. thoses are untouched  */
/*	src alt_text align req_height req_width border hspace vspace	*/
/*	usemap map ismap fptr						*/
/*	internal = 0							*/
/*	delayed = 0 							*/
/*	image = (Pixmap)NULL; 						*/
/*	clip = (Pixmap)NULL;						*/
/*	internal_numeo 							*/
/*	cw_only	 							*/

/* after the callback here is the modified field 			*/
/*	height = original height of image				*/
/*	width = original width of image					*/
/*	fetched = 1	if we are here					*/
/*	cached = 1	en principe					*/
/*	colrs (no more than 256 colors)					*/
/*	bg_index							*/
/*	image_data							*/
/* we need to compute 							*/
/*	num_colors							*/
/*	clip_data							*/
/*	transparent							*/

/* rescale image here */
/* Make stuff to create image with correct colors and size */
	if (icbs.req_width < 1)
		icbs.req_width = icbs.width;
	if (icbs.req_height < 1)
		icbs.req_height = icbs.height;

	if( (icbs.req_width != icbs.width ) ||
	    (icbs.req_height != icbs.height)) { /* rescale*/
		RescalePic(hw, &icbs, icbs.req_width, icbs.req_height);
	}

/* if we have a transparent background, prepare for it */
	bg = icbs.bg_index;
	bg_map = NULL;

	if (bg >= 0 ) {
		unsigned long bg_pixel;
		XColor tmpcolr;

		icbs.transparent=1;
		icbs.clip_data=(unsigned char *)malloc(icbs.width * icbs.height);
		memset(icbs.clip_data,0,(icbs.width * icbs.height));
/* This code copied from xpmread.c. I could almost delete the code from there,
 * but I suppose an XPM file could pathalogially have multiple transparent
* colour indicies. -- GWP
*/
		bg_pixel = hw->core.background_pixel;
		tmpcolr.pixel = bg_pixel;
		XQueryColor(XtDisplay(hw), hw->core.colormap, &tmpcolr);
		bg_red = icbs.colrs[bg].red = tmpcolr.red;
		bg_green = icbs.colrs[bg].green = tmpcolr.green;
		bg_blue = icbs.colrs[bg].blue = tmpcolr.blue;
		icbs.colrs[bg].flags = DoRed|DoGreen|DoBlue;
		bg_map = (unsigned char *)malloc(icbs.width * icbs.height);
	}

	widthbyheight = icbs.width * icbs.height;
	for (i=0; i < 256; i++)          /* Fill out used array. */
		Used[i] = 0;
	cnt = 1;
	bgptr = bg_map;
	cptr = icbs.clip_data;
	ptr = icbs.image_data;
 
/*This sets the bg map and also creates bitmap data for the
 * clip mask when there is a bg image */

	for (i = 0; i < icbs.height; i++) {
		for (j = 0, bcnt = 0; j < icbs.width; j++) {
			if ( Used[*ptr] == 0) {
				Used[*ptr] = cnt;
				cnt++;
			}
			if ( bg >= 0) {
				if (*ptr == bg) {
					*bgptr = 1;
				} else {
					*bgptr = 0;
					*cptr += nums[bcnt % 8];
				}
				if ( (bcnt % 8)==7 || j==(icbs.width-1) )
					cptr++;
				bgptr++;
				bcnt++;
			}
			ptr++;
		}
	}
	cnt--;

/* If the image has too many colors, apply a median cut algorithm to
 * reduce the color usage, and then reprocess it. */

	if( cnt > hw->html.max_colors_in_image ){
		MedianCut(icbs.image_data, icbs.width, icbs.height,
			icbs.colrs, 256, hw->html.max_colors_in_image);
		for (i=0; i < 256; i++)
			Used[i] = 0;
		cnt = 1;
		ptr = icbs.image_data;
		for (i=0; i < widthbyheight; i++) {
			if (Used[(int)*ptr] == 0) {
				Used[*ptr] = cnt;
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
	icbs.num_colors = cnt;

	*picd = icbs;
/* bg is not set in here if it gets munged by MedCut */
	for (i=0; i < 256; i++) {
		int indx;

		if (Used[i] != 0) {
			indx = Used[i] - 1;
			picd->colrs[indx].red = icbs.colrs[i].red;
			picd->colrs[indx].green = icbs.colrs[i].green;
			picd->colrs[indx].blue = icbs.colrs[i].blue;
/* squeegee in the background color */
			if ((bg >= 0)&&(i == bg)) {
				picd->colrs[indx].red = bg_red;
				picd->colrs[indx].green = bg_green;
				picd->colrs[indx].blue = bg_blue;
				picd->bg_index=indx;
			}
		}
	}

/* if MedianCut ate our background, add the new one now. */
	if (bg == 256) {
		picd->colrs[cnt - 1].red = bg_red;
		picd->colrs[cnt - 1].green = bg_green;
		picd->colrs[cnt - 1].blue = bg_blue;
		picd->bg_index=(cnt-1); 
	}                              
	bgptr = bg_map;
	ptr = icbs.image_data;
	for (i=0; i < widthbyheight; i++) {
		*ptr = (unsigned char)(Used[*ptr] - 1);
/* if MedianCut ate the background, enforce it here */
		if (bg == 256) {
			if (*bgptr)
				*ptr = (unsigned char) picd->bg_index;
			bgptr++;
		}
		ptr++;
	}
	if (bg_map )      /* free the background map if we have one */
		free (bg_map);
	return;
}

/* Place an image. Add an element record for it. */
void ImagePlace(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext *pcc)
{
	char *srcPtr, *altPtr, *alignPtr, *heightPtr, *widthPtr,
		*borderPtr, *hspacePtr, *vspacePtr, *usemapPtr, *ismapPtr;
	int border_width = IMAGE_DEFAULT_BORDER;
	struct ele_rec *eptr;
	int width=0;
	int height = 0;
	int baseline =0;
	ImageInfo * picd;
	AlignType valignment,halignment;
	int extra=0;
	ImageInfo lpicd;
	ImageInfo * saved_picd = mptr->s_picd;

	srcPtr = ParseMarkTag(mptr->start, MT_IMAGE, "SRC");	/* required */
	if ( ! srcPtr )
		return;				/* Bogus IMG */

	altPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ALT");
	alignPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ALIGN");
	heightPtr = ParseMarkTag(mptr->start, MT_IMAGE, "HEIGHT");
	widthPtr = ParseMarkTag(mptr->start, MT_IMAGE, "WIDTH");
	borderPtr = ParseMarkTag(mptr->start, MT_IMAGE, "BORDER");
	hspacePtr = ParseMarkTag(mptr->start, MT_IMAGE, "HSPACE");
	vspacePtr = ParseMarkTag(mptr->start, MT_IMAGE, "VSPACE");
	usemapPtr = ParseMarkTag(mptr->start, MT_IMAGE, "USEMAP");
	ismapPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ISMAP");

	lpicd.src = srcPtr;
	lpicd.alt_text = altPtr;
	lpicd.align = ALIGN_NONE;
	lpicd.height = 0;
	lpicd.req_height = -1;	/* no req_height */
	lpicd.width = 0;
	lpicd.req_width = -1;	/* no req_width */
	lpicd.border = IMAGE_DEFAULT_BORDER; 
	lpicd.hspace = DEF_IMAGE_HSPACE;
	lpicd.vspace = DEF_IMAGE_VSPACE;
	lpicd.usemap = usemapPtr;
	lpicd.map = NULL;
	lpicd.ismap = 0;
	lpicd.fptr = NULL; 
	lpicd.internal = 0;
	lpicd.delayed = hw->html.delay_image_loads;
	lpicd.fetched = 0;
	lpicd.cached = 0;
	lpicd.num_colors = 0;
/*	lpicd.colrs = NULL;	*/
	lpicd.bg_index = 0; 
	lpicd.image_data = NULL;
	lpicd.clip_data = NULL;
	lpicd.transparent=0; 
	lpicd.image = None;
	lpicd.clip = None;
	lpicd.internal_numeo = pcc->internal_mc_eo;
	lpicd.cw_only = pcc->cw_only;

        if (caseless_equal(alignPtr, "TOP")) {
                lpicd.align = VALIGN_TOP;
        } else if (caseless_equal(alignPtr, "MIDDLE")) {
                lpicd.align = VALIGN_MIDDLE;
        } else if (caseless_equal(alignPtr, "BOTTOM")){
                lpicd.align = VALIGN_BOTTOM;
        } else if (caseless_equal(alignPtr, "LEFT")){
		lpicd.align = HALIGN_LEFT;
	} else if (caseless_equal(alignPtr, "RIGHT")) {
		lpicd.align = HALIGN_RIGHT;
	}				/* don't know how to center img */
        free(alignPtr);

	if ( heightPtr ) {
		lpicd.req_height = atoi(heightPtr);
		free(heightPtr);
		if (lpicd.req_height < 1 ) 	/* too small ... */
			lpicd.req_height = -1;
	}
	if ( widthPtr ) {
		lpicd.req_width = atoi(widthPtr);
		free(widthPtr);
		if (lpicd.req_width < 1 ) 	/* too small ... */
			lpicd.req_width = -1;
	}
			/* border apply if in anchor (clickable)*/
	if ( borderPtr ){
		lpicd.border = atoi(borderPtr);
		free(borderPtr);
		if (lpicd.border < 1)
			lpicd.border = IMAGE_DEFAULT_BORDER;
	}
	if (pcc->anchor_tag_ptr->anc_href == NULL)
		lpicd.border = 0;

	if (hspacePtr){
		lpicd.hspace = atoi(hspacePtr);
		free(hspacePtr);
		if ( lpicd.hspace < 0 )
			lpicd.hspace = DEF_IMAGE_HSPACE;
	}
	if (vspacePtr) {
		lpicd.vspace = atoi(vspacePtr);
		free(vspacePtr);
		if (lpicd.vspace < 0 )
			lpicd.vspace = DEF_IMAGE_VSPACE; 
	}
	if (ismapPtr) {
		lpicd.ismap = 1;
		free(ismapPtr);
	}

	/* now initialise the image part */
	picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
	*picd = lpicd;		/* for work */

/* got an image in picd */
	HtmlGetImage(hw, picd, pcc, False); /* don't force load */

/* now we have an image. It is :
	- an internal-gopher	(internal=1,fetched=1, delayed =0)
	- a delayed image	(internal=1, fetched=0, delayed=1)
	- a no image		(internal=1, fetched=0, delayed=0)
	- the requested image	(internal=0, fetched=1, delayed=0)
  The data of image is here. The image and clip too.
  We need to work with pcc now.
*/

/* save the work */
	mptr->s_picd = picd;

	extra = 0;
	if (pcc->anchor_tag_ptr->anc_href != NULL) {
		extra = 2*picd->border;
	} else if (picd->ismap && (pcc->cur_form != NULL) && picd->fetched){
/* SUPER SPECIAL CASE!  (Thanks Marc)
 * If you have an ISMAP image inside a form, And that form doesn't already
 * have an HREF by being inside an anchor, (Being a DelayedHRef is considered
 * no href) clicking in that image will submit the form, adding
 * the x,y coordinates of the click as part of the list of name/value pairs.
 */
		picd->fptr = pcc->cur_form;
	}
	baseline = height = picd->height + extra;
	width = picd->width + extra;

	valignment = VALIGN_BOTTOM;
	halignment = ALIGN_NONE;
/* Check if this image will be top aligned */
	if (picd->align == VALIGN_TOP) {
		valignment = VALIGN_TOP;
		baseline =0;
	} else if ( picd->align == VALIGN_MIDDLE) {
		valignment = VALIGN_MIDDLE;
		baseline = baseline/2;
	} else if ( picd->align == VALIGN_BOTTOM){
		valignment = VALIGN_BOTTOM;
		/* baseline unchanged */
	} else if ( picd->align == HALIGN_LEFT) {
		halignment = HALIGN_LEFT;
		/* baseline unchanged ######??? /* */
	} else if ( picd->align == HALIGN_RIGHT) {
		halignment = HALIGN_RIGHT;
		/* baseline unchanged ######??? /* */
	}
		
/* Now look if the image is too wide, if so insert a linebreak. */
	if (!pcc->preformat) {
		if ( (pcc->x + width) >
		     (pcc->eoffsetx+pcc->left_margin+pcc->cur_line_width)) 
			LinefeedPlace(hw, mptr, pcc);
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
		eptr->bwidth=picd->border ;
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
	if ((eptr->anchor_tag_ptr->anc_href != NULL)&&
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
				eptr->pic_data->image = (DelayedImageData(hw))->image;
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

/*	if((eptr->pic_data.delayed)&&
/*	   (eptr->anchor_tag_ptr->anc_href != NULL)&&
/*	   (!IsDelayedHRef(hw, eptr->anchor_tag_ptr->anc_href))&&
/*	   (!IsIsMapForm(hw, eptr->anchor_tag_ptr->anc_href))) {
/*		XSetForeground(XtDisplay(hw), hw->html.drawGC, eptr->fg);
/*		XFillRectangle(XtDisplay(hw->html.view),
/*			       XtWindow(hw->html.view),
/*			       hw->html.drawGC,
/*			       x, (y + AnchoredImage_height + IMAGE_DEFAULT_BORDER),
/*			       (eptr->width + (2 * extra)), extra);
/*	}
*/
}
