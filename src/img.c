/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <sys/stat.h>

#include "libhtmlw/HTMLmiscdefs.h"
#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "mosaic.h"
#include "picread.h"
#include "URLParse.h"
#include "mime.h"

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


/* definition for MedianCut */

#define RED     0 
#define GREEN   1
#define BLUE    2

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
static struct color_rec *glohash_ptr;
static struct color_rec *free_hash = (struct color_rec *)NULL;
static struct color_rec *glotptr;
static int Width, Height;
static int ColorCnt;
static int NCells;

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

#define IMGINFO_FROM_BITMAP(hw, x) \
{ \
        if (!x) { \
                x = (ImageInfo *)calloc (1,sizeof (ImageInfo)); \
                x->width = x##_width; \
                x->height = x##_height; \
                x->internal = 1; \
                x->delayed = 0; \
                x->fetched = 1; \
                x->cached =1; \
                x->image = XCreatePixmapFromBitmapData(XtDisplay(hw), \
                        XtWindow(hw), \
                        (char*) x##_bits,  \
                        x##_width, x##_height, \
                        hw->manager.foreground,\
                        hw->core.background_pixel,\
                        DefaultDepthOfScreen(XtScreen(hw)));\
        }\
        lpicd.width = x##_width; \
        lpicd.height = x##_height; \
        lpicd.internal = 1; \
        lpicd.delayed = 0; \
        lpicd.fetched = 1; \
        lpicd.cached =1; \
        lpicd.image = x->image; \
}

                        
ImageInfo * DelayedImageData( HTMLWidget hw)
{                       
        if (delayed_image == NULL) {
                delayed_image = (ImageInfo *)calloc (1,sizeof (ImageInfo));
                delayed_image->internal = 1;
                delayed_image->delayed = 1;
                delayed_image->fetched = 0;
                delayed_image->cached = 0;
                delayed_image->width = DelayedImage_width;
                delayed_image->height = DelayedImage_height;
                delayed_image->image = XCreatePixmapFromBitmapData( XtDisplay(hw),                        XtWindow(hw), (char*) DelayedImage_bits,
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
                no_image = (ImageInfo *)calloc (1,sizeof (ImageInfo));
                no_image->internal = 1;
                no_image->delayed = 0;
                no_image->fetched = 0;
                no_image->cached = 0;
                no_image->width = NoImage_width;
                no_image->height = NoImage_height;
                no_image->image = XCreatePixmapFromBitmapData( XtDisplay(hw),
                        XtWindow(hw), (char*) NoImage_bits,
                        NoImage_width, NoImage_height,
                        hw->manager.foreground,
                        hw->core.background_pixel,
                        DefaultDepthOfScreen(XtScreen(hw)));
        }
        return(no_image);              
}

static void InitMinMax( int boxnum)
{
	C_boxes[boxnum].min_pix[RED] = 65536;
	C_boxes[boxnum].max_pix[RED] = 0;
	C_boxes[boxnum].min_pix[GREEN] = 65536;
	C_boxes[boxnum].max_pix[GREEN] = 0;
	C_boxes[boxnum].min_pix[BLUE] = 65536;
	C_boxes[boxnum].max_pix[BLUE] = 0;
}

static struct color_rec *AddHash( int red, int green, int blue)
{
	int lum;

	lum = (((red * 306 + green * 601 + blue * 117) >> 10) * NCells) >> 16;;
	if (free_hash != NULL) {
		glohash_ptr = free_hash;
		free_hash = free_hash->hash_next;
	} else {
		glohash_ptr = (struct color_rec *)XtMalloc(sizeof(struct color_rec));
	}
	CHECK_OUT_OF_MEM(glohash_ptr);

	glohash_ptr->pixel[RED] = red;
	glohash_ptr->pixel[GREEN] = green;
	glohash_ptr->pixel[BLUE] = blue;
	glohash_ptr->box_num = 0;
	glohash_ptr->next = NULL;
	glohash_ptr->hash_next = Hash[lum];
	Hash[lum] = glohash_ptr;
	return(glohash_ptr);
}

static void AddColor( struct color_rec *cptr, int boxnum)
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

static void CountColors( unsigned char *data, XColor *colrs, int *color_used)
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

static int FindTarget( int *tptr)
{
	int range, i, indx=0;

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

static void SplitBox( int boxnum, int color_indx)
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
			glotptr = data;
			data = data->next;
			if (glotptr->pixel[color_indx] > split_val) {
				glotptr->next = high;
				high = glotptr;
				high_cnt++;
			} else {
				glotptr->next = low;
				low = glotptr;
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

static void SplitColors( int e_cnt)
{
	if (ColorCnt < e_cnt) {
		int i;

		glotptr = C_boxes[0].c_data;
		for (i=0; i<ColorCnt; i++) {
			glohash_ptr = glotptr;
			glotptr = glotptr->next;
			C_boxes[i].c_data = glohash_ptr;
			C_boxes[i].count = 1;
			glohash_ptr->box_num = i;
			glohash_ptr->next = NULL;
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

static void ConvertData( unsigned char *data, XColor *colrs, int *colors_used)
{
	unsigned char *dptr;
	register int i;
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

static void PrintColormap( int e_cnt, XColor *colrs)
{
	int i;

	for(i=0; i<BoxCount; i++) {
		int Tred, Tgreen, Tblue;
		int c_cnt;

		c_cnt = 0;
		Tred = Tgreen = Tblue = 0;
		glotptr = C_boxes[i].c_data;
		while (glotptr != NULL) {
			Tred += glotptr->pixel[RED];
			Tgreen += glotptr->pixel[GREEN];
			Tblue += glotptr->pixel[BLUE];
			c_cnt++;
			glotptr = glotptr->next;
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

static void MedianCut( unsigned char *data, int w, int h, XColor *colrs,
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
		glohash_ptr = Hash[i];
		while (glohash_ptr != NULL) {
			glotptr = glohash_ptr;
			glohash_ptr = glohash_ptr->hash_next;
			glotptr->hash_next = free_hash;
			free_hash = glotptr;
		}
	}
}

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
	cxarr = (int *) calloc(nw , sizeof(int));
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
	picd->len_image_data = nw * nh * bperpix;
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

/* Make an image of appropriate depth for display from image data.
 */
static XImage * MakeImage( Display *d, unsigned char *data, int width, int height,
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

static Pixmap InfoToImage( HTMLWidget hw, ImageInfo *img_info, int clip)
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
	Mapping = (int *)calloc(img_info->num_colors , sizeof(int));
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
				tmpcolr.pixel = HTMLXColorToPixel(&tmpcolr);
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
/*		img_info->width = NoImage_width;
		img_info->height = NoImage_height;
*/
		Img = (NoImageData(hw))->image;
	} else {
		XPutImage(XtDisplay(hw), Img, hw->html.drawGC, tmpimage, 0, 0,
			0, 0, img_info->width, img_info->height);
		XDestroyImage(tmpimage);
	}
        free((char *)Mapping);
	return(Img);
}

void MMPreParseImageBody(mo_window * win, ImageInfo *picd, struct mark_up *mptr)
{
	ImageInfo lpicd;

	lpicd.src = ParseMarkTag(mptr->start, MT_BODY,"background");

        lpicd.alt_text = NULL;       
        lpicd.align = HALIGN_NONE;      
        lpicd.height = 0;              
        lpicd.req_height = -1;  /* no req_height */
        lpicd.width = 0;               
        lpicd.req_width = -1;   /* no req_width */
        lpicd.border = -1; 
	lpicd.req_border = -1;
        lpicd.hspace = DEF_IMAGE_HSPACE;
        lpicd.vspace = DEF_IMAGE_VSPACE;
        lpicd.usemap = NULL;      
        lpicd.map = NULL;              
        lpicd.ismap = 0;               
        lpicd.fptr = NULL;             
        lpicd.internal = 0;            
        lpicd.delayed = win->delay_object_loads;
        lpicd.fetched = 0;             
        lpicd.cached = 0;              
        lpicd.num_colors = 0;          
/*      lpicd.colrs = NULL;     */     
        lpicd.bg_index = -1;           
        lpicd.image_data = NULL;       
        lpicd.len_image_data = 0;      
        lpicd.clip_data = NULL;        
        lpicd.transparent=0;           
        lpicd.image = None;            
        lpicd.clip = None;             
	lpicd.internal_numeo = -1;
        lpicd.cw_only = -1;

	*picd = lpicd;
}

void MMPreParseInputTagImage(mo_window * win, ImageInfo *picd, struct mark_up *mptr)
{
	ImageInfo *dl_img, *no_img;
	ImageInfo lpicd;
	char *altPtr, *alignPtr, *heightPtr, *widthPtr, *borderPtr, *hspacePtr, *vspacePtr, *usemapPtr, *ismapPtr;
	HTMLWidget hw = (HTMLWidget) win->scrolled_win;

	altPtr = ParseMarkTag(mptr->start, MT_INPUT, "ALT");
	alignPtr = ParseMarkTag(mptr->start, MT_INPUT, "ALIGN");
	heightPtr = ParseMarkTag(mptr->start, MT_INPUT, "HEIGHT");
	widthPtr = ParseMarkTag(mptr->start, MT_INPUT, "WIDTH");
	borderPtr = ParseMarkTag(mptr->start, MT_INPUT, "BORDER");
	hspacePtr = ParseMarkTag(mptr->start, MT_INPUT, "HSPACE");
	vspacePtr = ParseMarkTag(mptr->start, MT_INPUT, "VSPACE");
	usemapPtr = ParseMarkTag(mptr->start, MT_INPUT, "USEMAP");
	ismapPtr = ParseMarkTag(mptr->start, MT_INPUT, "ISMAP");

/* some of the following MUST not change during image processing */
/* src, alt_text, align, req_height, req_width, req_border, hspace, vspace,
 * usemap, ismap */

	lpicd.src = ParseMarkTag(mptr->start, MT_INPUT, "SRC");
        lpicd.alt_text = altPtr;       
        lpicd.height = 0;              
        lpicd.req_height = -1;  /* no req_height */
        lpicd.width = 0;               
        lpicd.req_width = -1;   /* no req_width */
        lpicd.border = -1; 
	lpicd.req_border = -1;
        lpicd.hspace = DEF_IMAGE_HSPACE;
        lpicd.vspace = DEF_IMAGE_VSPACE;
        lpicd.usemap = usemapPtr;      
        lpicd.map = NULL;              
        lpicd.ismap = 0;               
        lpicd.fptr = NULL;             
        lpicd.internal = 0;            
        lpicd.delayed = win->delay_object_loads;
        lpicd.fetched = 0;             
        lpicd.cached = 0;              
        lpicd.num_colors = 0;          
/*      lpicd.colrs = NULL;     */     
        lpicd.bg_index = -1;           
        lpicd.image_data = NULL;       
        lpicd.len_image_data = 0;      
        lpicd.clip_data = NULL;        
        lpicd.transparent=0;           
        lpicd.image = None;            
        lpicd.clip = None;             
	lpicd.internal_numeo = -1;
        lpicd.cw_only = -1;
/*##################################### */
/*--- HTML 4.01, 13.7.4 Alignment ---*/
	lpicd.align = VALIGN_BOTTOM;
/*        #########lpicd.align = ALIGN_NONE;      ### a virer */

	if (alignPtr) {
		if ( !strcasecmp(alignPtr, "TOP")) {
			lpicd.align = VALIGN_TOP;
		} else if ( !strcasecmp(alignPtr, "MIDDLE")) {
			lpicd.align = VALIGN_MIDDLE; 
		} else if (!strcasecmp(alignPtr, "BOTTOM")){
			lpicd.align = VALIGN_BOTTOM; 
		} else if (!strcasecmp(alignPtr, "LEFT")){
			lpicd.align = HALIGN_LEFT_FLOAT;
		} else if (!strcasecmp(alignPtr, "RIGHT")) {
			lpicd.align = HALIGN_RIGHT_FLOAT;
		}		/* don't know how to center img */
		free(alignPtr);
	}

	if ( heightPtr ) {
		lpicd.req_height = atoi(heightPtr);
		free(heightPtr);       
		if (lpicd.req_height < 1 )      /* too small ... */
			lpicd.req_height = -1;
	}                              
	if ( widthPtr ) {              
		lpicd.req_width = atoi(widthPtr);
		free(widthPtr);        
		if (lpicd.req_width < 1 )       /* too small ... */
			lpicd.req_width = -1;
	}                              

/* border apply if in anchor (clickable)*/
	if ( borderPtr ){              
		lpicd.req_border = atoi(borderPtr);
		free(borderPtr);       
		if (lpicd.req_border < 0)  
			lpicd.req_border = -1;
	}
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

/* remarque:
 * remaing field to update:
 * 	height = 0; width = 0; border = -1;
 *	map = NULL; fptr = NULL;
 *	internal = 0; delayed = 0; fetched = 0; cached = 0;
 *	num_colors = 0; bg_index = -1; colrs = NULL;
 *	image_data = NULL; len_image_data = 0;
 *	clip_data = NULL; transparent=0;
 *	image = None; clip = None;
 *	internal_numeo = -1; cw_only = -1;
 */

	if (!lpicd.src) { /* put a no_image */
		no_img = NoImageData(hw);
		lpicd.internal = 1;
		lpicd.width = no_img->width;
		lpicd.height = no_img->height;
		lpicd.delayed = 0;
		lpicd.image = no_img->image;
		*picd = lpicd;
		return;
	}

/* if this image is a 'well-know image' , load it now */

	if (strncmp (lpicd.src, "internal-gopher-", 16) == 0) {

/* Internal image. We update the following field :
 * width; height; internal = 1; delayed = 0;
 * fetched = 1; cached =1; image; all other remain to default value */ 

		if (strcmp (lpicd.src, "internal-gopher-image") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_image)
		} else if (strcmp (lpicd.src, "internal-gopher-movie") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_movie)
		} else if (strcmp (lpicd.src, "internal-gopher-menu") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_menu)
		} else if (strcmp (lpicd.src, "internal-gopher-text") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_text)
		} else if (strcmp (lpicd.src, "internal-gopher-sound") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_sound)
		} else if (strcmp (lpicd.src, "internal-gopher-index") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_index)
		} else if (strcmp (lpicd.src, "internal-gopher-telnet") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_telnet)
		} else if (strcmp (lpicd.src, "internal-gopher-binary") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_binary)
		} else if (strcmp (lpicd.src, "internal-gopher-unknown") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_unknown)
		} else {              
			IMGINFO_FROM_BITMAP(hw,gopher_unknown)
		}                     
		*picd = lpicd;
		return;
	} 

/* Not a 'well-know image' : make a default image  and let fetched = 0 */
/* so, in any case we have an image to display */
	dl_img = DelayedImageData(hw);
	lpicd.internal = 1;
	lpicd.width = dl_img->width;
	lpicd.height = dl_img->height;
	lpicd.delayed = 1;
	lpicd.image = dl_img->image;
	*picd = lpicd;
/* two case on fetched:
 * - fetched = True => this is an internal , width height and image are ok
 * - fetched = False => remaing field is not uptodate and need to be updated
 */
}

void MMPreParseImageTag(mo_window * win, ImageInfo *picd, struct mark_up *mptr)
{
	ImageInfo *dl_img, *no_img;
	ImageInfo lpicd;
	char *altPtr, *alignPtr, *heightPtr, *widthPtr, *borderPtr, *hspacePtr, *vspacePtr, *usemapPtr, *ismapPtr;
	HTMLWidget hw = (HTMLWidget) win->scrolled_win;

	altPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ALT");
	alignPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ALIGN");
	heightPtr = ParseMarkTag(mptr->start, MT_IMAGE, "HEIGHT");
	widthPtr = ParseMarkTag(mptr->start, MT_IMAGE, "WIDTH");
	borderPtr = ParseMarkTag(mptr->start, MT_IMAGE, "BORDER");
	hspacePtr = ParseMarkTag(mptr->start, MT_IMAGE, "HSPACE");
	vspacePtr = ParseMarkTag(mptr->start, MT_IMAGE, "VSPACE");
	usemapPtr = ParseMarkTag(mptr->start, MT_IMAGE, "USEMAP");
	ismapPtr = ParseMarkTag(mptr->start, MT_IMAGE, "ISMAP");

/* some of the following MUST not change during image processing */
/* src, alt_text, align, req_height, req_width, req_border, hspace, vspace,
 * usemap, ismap */

	lpicd.src = ParseMarkTag(mptr->start, MT_IMAGE, "SRC");
        lpicd.alt_text = altPtr;       
        lpicd.height = 0;              
        lpicd.req_height = -1;  /* no req_height */
        lpicd.width = 0;               
        lpicd.req_width = -1;   /* no req_width */
        lpicd.border = -1; 
	lpicd.req_border = -1;
        lpicd.hspace = DEF_IMAGE_HSPACE;
        lpicd.vspace = DEF_IMAGE_VSPACE;
        lpicd.usemap = NULL;      
        lpicd.map = NULL;              
        lpicd.ismap = 0;               
        lpicd.fptr = NULL;             
        lpicd.internal = 0;            
        lpicd.delayed = win->delay_object_loads;
        lpicd.fetched = 0;             
        lpicd.cached = 0;              
        lpicd.num_colors = 0;          
/*      lpicd.colrs = NULL;     */     
        lpicd.bg_index = -1;           
        lpicd.image_data = NULL;       
        lpicd.len_image_data = 0;      
        lpicd.clip_data = NULL;        
        lpicd.transparent=0;           
        lpicd.image = None;            
        lpicd.clip = None;             
	lpicd.internal_numeo = -1;
        lpicd.cw_only = -1;

/*################# */
/*--- HTML 4.01, 13.7.4 Alignment ---*/
	lpicd.align = VALIGN_BOTTOM;
/*        ###### lpicd.align = ALIGN_NONE;       */

	if (alignPtr) {
		if ( !strcasecmp(alignPtr, "TOP")) {
			lpicd.align = VALIGN_TOP;
		} else if ( !strcasecmp(alignPtr, "MIDDLE")) {
			lpicd.align = VALIGN_MIDDLE; 
		} else if (!strcasecmp(alignPtr, "BOTTOM")){
			lpicd.align = VALIGN_BOTTOM; 
		} else if (!strcasecmp(alignPtr, "LEFT")){
			lpicd.align = HALIGN_LEFT_FLOAT;
		} else if (!strcasecmp(alignPtr, "RIGHT")) {
			lpicd.align = HALIGN_RIGHT_FLOAT;
		}		/* don't know how to center img */
		free(alignPtr);
	}

	if ( heightPtr ) {             
		lpicd.req_height = atoi(heightPtr);
		free(heightPtr);       
		if (lpicd.req_height < 1 )      /* too small ... */
			lpicd.req_height = -1;
	}                              
	if ( widthPtr ) {              
		lpicd.req_width = atoi(widthPtr);
		free(widthPtr);        
		if (lpicd.req_width < 1 )       /* too small ... */
			lpicd.req_width = -1;
	}                              

/* border apply if in anchor (clickable)*/
	if ( borderPtr ){              
		lpicd.req_border = atoi(borderPtr);
		free(borderPtr);       
		if (lpicd.req_border < 0)  
			lpicd.req_border = -1;
	}
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

	if (usemapPtr && (*usemapPtr == '#')) {	/* find the map */
		int i;

		for (i = 0 ; i < win->htinfo->n_map; i++) {
			if (!strcmp(win->htinfo->maps[i]->name,usemapPtr+1) ){
				lpicd.map = win->htinfo->maps[i];
        			lpicd.usemap = usemapPtr;      
				break;
			}
		}
	}

/* remarque:
 * remaing field to update:
 * 	height = 0; width = 0; border = -1;
 *	map = NULL; fptr = NULL;
 *	internal = 0; delayed = 0; fetched = 0; cached = 0;
 *	num_colors = 0; bg_index = -1; colrs = NULL;
 *	image_data = NULL; len_image_data = 0;
 *	clip_data = NULL; transparent=0;
 *	image = None; clip = None;
 *	internal_numeo = -1; cw_only = -1;
 */

	if (!lpicd.src) { /* put a no_image */
		no_img = NoImageData(hw);
		lpicd.internal = 1;
		lpicd.width = no_img->width;
		lpicd.height = no_img->height;
		lpicd.delayed = 0;
		lpicd.image = no_img->image;
		*picd = lpicd;
		return;
	}

/* if this image is a 'well-know image' , load it now */

	if (strncmp (lpicd.src, "internal-gopher-", 16) == 0) {

/* Internal image. We update the following field :
 * width; height; internal = 1; delayed = 0;
 * fetched = 1; cached =1; image; all other remain to default value */ 

		if (strcmp (lpicd.src, "internal-gopher-image") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_image)
		} else if (strcmp (lpicd.src, "internal-gopher-movie") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_movie)
		} else if (strcmp (lpicd.src, "internal-gopher-menu") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_menu)
		} else if (strcmp (lpicd.src, "internal-gopher-text") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_text)
		} else if (strcmp (lpicd.src, "internal-gopher-sound") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_sound)
		} else if (strcmp (lpicd.src, "internal-gopher-index") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_index)
		} else if (strcmp (lpicd.src, "internal-gopher-telnet") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_telnet)
		} else if (strcmp (lpicd.src, "internal-gopher-binary") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_binary)
		} else if (strcmp (lpicd.src, "internal-gopher-unknown") == 0) {
			IMGINFO_FROM_BITMAP(hw,gopher_unknown)
		} else {              
			IMGINFO_FROM_BITMAP(hw,gopher_unknown)
		}                     
		*picd = lpicd;
		return;
	} 

/* Not a 'well-know image' : make a default image  and let fetched = 0 */
/* so, in any case we have an image to display */
	dl_img = DelayedImageData(hw);
	lpicd.internal = 1;
	lpicd.width = dl_img->width;
	lpicd.height = dl_img->height;
	lpicd.delayed = 1;
	lpicd.image = dl_img->image;
	*picd = lpicd;
/* two case on fetched:
 * - fetched = True => this is an internal , width height and image are ok
 * - fetched = False => remaing field is not uptodate and need to be updated
 */
}

static unsigned char nums[]={ 1, 2, 4, 8, 16, 32, 64, 128 };

/* remarque:
 * remaing field to update:
 * 	height = 0; width = 0; border = -1;
 *	map = NULL; fptr = NULL;
 *	internal = 0; delayed = 0; fetched = 0; cached = 0;
 *	num_colors = 0; bg_index = -1; colrs = NULL;
 *	image_data = NULL; len_image_data = 0;
 *	clip_data = NULL; transparent=0;
 *	image = None; clip = None;
 *	internal_numeo = -1; cw_only = -1;
 * Called because we have data from some www protocol.
 * we try to make an image with the data in fname. 
 */

void MMPreloadImage(mo_window *win, struct mark_up *mptr, MimeHeaderStruct *mhs,
	char *fname)
{
	ImageInfo *picd, lpicd;
	Pixel fg_pixel,bg_pixel;
	struct stat s;
	HTMLWidget hw = (HTMLWidget) win->scrolled_win;
	unsigned char * bit_data;
	int width,height;
	XColor colrs[256];
	unsigned short bg_red=0, bg_green=0, bg_blue=0;
	unsigned char * bg_map;
	int bg;
	int i,j, cnt,bcnt;
	int widthbyheight;
	int Used[256];
	unsigned char *ptr;
	unsigned char *bgptr;
	unsigned char *cptr;

	bg_pixel = hw->core.background_pixel;
	fg_pixel = hw->manager.foreground;

	picd = mptr->s_picd;
	lpicd = *picd;		 /* for work */

/*        if (pcc->anchor_tag_ptr->anc_href == NULL)
                lpicd.border = 0;
*/

	if (!fname) {
		ImageInfo *noim;

		noim = NoImageData(hw);
		picd->height = noim->height;
		picd->width = noim->width;
		picd->internal = 1;
		picd->delayed = 0;   
		picd->fetched = 0;
		picd->image = noim->image;
		return;
	}
/* now initialise the image part */

	width = height = 0;
	bit_data = ReadBitmap(win->scrolled_win, fname, &width, &height, colrs, &bg);
	
        if( (width * height == 0) || (bit_data == NULL) ) {  /* size null ??? */
			ImageInfo *noim;

                        noim = NoImageData(hw);
                        picd->height = noim->height;
                        picd->width = noim->width;
                        picd->internal = 1;
                        picd->delayed = 0;   
                        picd->fetched = 0;
                        picd->image = noim->image;
			if (bit_data)
				free(bit_data);
                        return;
        }                    

/* we have an image */
	lpicd.width = width;
	lpicd.height = height; 
	if ( lpicd.req_width == -1)
		lpicd.req_width = width;
	if ( lpicd.req_height == -1 )
		lpicd.req_height = height;
	
	lpicd.internal =  0;
	lpicd.delayed = 0;
	lpicd.fetched = 1;
	lpicd.cached = 1; 
	lpicd.image_data = bit_data;
	if (bg >= 0 )         
		lpicd.bg_index=bg;
	stat(fname, &s);
	lpicd.len_image_data = s.st_size;

	for ( i = 0; i < 256; i++) 
		lpicd.colrs[i] = colrs[i];

/**########################################## */

	/* here we have an image from the cache or the net */
/* data is in icbs */                  
/* Make stuff to create image with correct colors and size */

        if( (lpicd.req_width != lpicd.width ) ||
            (lpicd.req_height != lpicd.height)) { /* rescale*/
                RescalePic(hw, &lpicd, lpicd.req_width, lpicd.req_height);
        }
/* remarque : remaining field to update :
 *      border = -1; map = NULL; fptr = NULL;
 *      num_colors = 0; bg_index = -1; colrs = NULL;
 *      clip_data = NULL; transparent=0;
 *      image = None; clip = None;
 *      internal_numeo = -1; cw_only = -1;
 */
        
/* if we have a transparent background, prepare for it */
        bg = lpicd.bg_index;
        bg_map = NULL;                 
                                       
        if (bg >= 0 ) {
                unsigned long lbg_pixel;
                XColor tmpcolr;        

                lpicd.transparent=1;
                lpicd.clip_data=(unsigned char *)malloc(lpicd.width * lpicd.height);
                memset(lpicd.clip_data,0,(lpicd.width * lpicd.height));
/* This code copied from xpmread.c. I could almost delete the code from there,
 * but I suppose an XPM file could pathalogially have multiple transparent
* colour indicies. -- GWP
*/
                lbg_pixel = hw->core.background_pixel;
                tmpcolr.pixel = lbg_pixel;
                HTMLPixelToXColor(&tmpcolr);
                bg_red = lpicd.colrs[bg].red = tmpcolr.red;
                bg_green = lpicd.colrs[bg].green = tmpcolr.green;
                bg_blue = lpicd.colrs[bg].blue = tmpcolr.blue;
                lpicd.colrs[bg].flags = DoRed|DoGreen|DoBlue;
                bg_map = (unsigned char *)malloc(lpicd.width * lpicd.height);
        }
        widthbyheight = lpicd.width * lpicd.height;
        for (i=0; i < 256; i++)          /* Fill out used array. */
                Used[i] = 0;
        cnt = 1;
        bgptr = bg_map;
        cptr = lpicd.clip_data;
        ptr = lpicd.image_data; 
                                       
/*This sets the bg map and also creates bitmap data for the
 * clip mask when there is a bg image */
                                       
        for (i = 0; i < lpicd.height; i++) { 
                for (j = 0, bcnt = 0; j < lpicd.width; j++) {
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
                                if ( (bcnt % 8)==7 || j==(lpicd.width-1) )
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
                MedianCut(lpicd.image_data, lpicd.width, lpicd.height,
                        lpicd.colrs, 256, hw->html.max_colors_in_image);
                for (i=0; i < 256; i++)
                        Used[i] = 0;
                cnt = 1;
                ptr = lpicd.image_data;
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
        lpicd.num_colors = cnt;         

/* remarque : remaining field to update :
 *      border = -1; map = NULL; fptr = NULL;
 *      bg_index = -1; colrs = somethings;
 *      image = None; clip = None;
 *      internal_numeo = -1; cw_only = -1;
 */
                                       
        *picd = lpicd;
/* bg is not set in here if it gets munged by MedCut */
        for (i=0; i < 256; i++) {
                int indx;              
                                       
                if (Used[i] != 0) {    
                        indx = Used[i] - 1;  
                        picd->colrs[indx].red = lpicd.colrs[i].red;
                        picd->colrs[indx].green = lpicd.colrs[i].green;
                        picd->colrs[indx].blue = lpicd.colrs[i].blue;
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
        ptr = picd->image_data;
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

/* remarque : remaining field to update :
 *      border = -1; map = NULL; fptr = NULL;
 *      image = None; clip = None;
 *      internal_numeo = -1; cw_only = -1;
 */
/* we have data... make the image */

	picd->image = InfoToImage(hw,picd, 0);
	if(picd->image == None){
		ImageInfo *noim;

		noim = NoImageData(hw);
		picd->height = noim->height;
		picd->width = noim->width;
		picd->internal = 1;
		picd->delayed = 0;   
		picd->fetched = 0;
		picd->image = noim->image;
		if (picd->clip_data) {
			free(picd->clip_data);
			picd->clip_data = NULL;
		}
		picd->transparent = 0;
		free(picd->image_data);
		picd->image_data = NULL;
		picd->len_image_data = 0;
		return;
	} 
	if (picd->transparent) {
		picd->clip =XCreatePixmapFromBitmapData(XtDisplay(hw),
			XtWindow(hw->html.view), (char*) picd->clip_data,
			picd->width, picd->height, 1, 0, 1);
	}
	if (picd->clip_data) {
		free(picd->clip_data);
		picd->clip_data = NULL;
	}
	free(picd->image_data);
	picd->image_data = NULL;
	picd->len_image_data = 0;

	return;

/* remarque : remaining field to update :
 *      border = -1; map = NULL; fptr = NULL;
 *      internal_numeo = -1; cw_only = -1;
 */
}
