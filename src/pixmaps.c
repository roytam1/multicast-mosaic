/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "pixmaps.h"
#include "xpm.h"
#include "xpmread.h"

/*for memcpy*/
#include <memory.h>

Pixmap IconsBig[25], IconsSmall[25];

Pixmap *IconPix=NULL,*IconPixBig,*IconPixSmall;

Pixmap toolbarBack, toolbarForward, toolbarHome, toolbarReload,
    toolbarOpen, toolbarSave, toolbarClone, toolbarNew, toolbarClose,
    toolbarBackGRAY, toolbarForwardGRAY, toolbarAddHotlist,
    toolbarSearch, toolbarPrint, toolbarPost, toolbarFollow,
    tearv, tearh, toolbarPostGRAY, toolbarFollowGRAY,
    toolbarNewsFwd, toolbarNewsFFwd, toolbarNewsRev, toolbarNewsFRev,
    toolbarNewsIndex, toolbarNewsGroups,
    toolbarNewsFwdGRAY, toolbarNewsFFwdGRAY, toolbarNewsRevGRAY, toolbarNewsFRevGRAY,
    toolbarNewsIndexGRAY,
    toolbarFTPput, toolbarFTPmkdir;

Pixmap securityKerberos4, securityKerberos5, securityBasic, securityMd5,
	securityNone, securityUnknown, securityDomain, securityLogin, enc_not_secure;

struct pixload_info {
    char **raw;
    Pixmap *handle;
    int gray;
} pix_info[] = {
    {icon1,&IconsBig[0],0},
    {icon2,&IconsBig[1],0},
    {icon3,&IconsBig[2],0},
    {icon4,&IconsBig[3],0},
    {icon5,&IconsBig[4],0},
    {icon6,&IconsBig[5],0},
    {icon7,&IconsBig[6],0},
    {icon8,&IconsBig[7],0},
    {icon9,&IconsBig[8],0},
    {icon10,&IconsBig[9],0},
    {icon11,&IconsBig[10],0},
    {icon12,&IconsBig[11],0},
    {icon13,&IconsBig[12],0},
    {icon14,&IconsBig[13],0},
    {icon15,&IconsBig[14],0},
    {icon16,&IconsBig[15],0},
    {icon17,&IconsBig[16],0},
    {icon18,&IconsBig[17],0},
    {icon19,&IconsBig[18],0},
    {icon20,&IconsBig[19],0},
    {icon21,&IconsBig[20],0},
    {icon22,&IconsBig[21],0},
    {icon23,&IconsBig[22],0},
    {icon24,&IconsBig[23],0},
    {icon25,&IconsBig[24],0},
    {s_icon1,&IconsSmall[0],0},
    {s_icon2,&IconsSmall[1],0},
    {s_icon3,&IconsSmall[2],0},
    {s_icon4,&IconsSmall[3],0},
    {s_icon5,&IconsSmall[4],0},
    {s_icon6,&IconsSmall[5],0},
    {s_icon7,&IconsSmall[6],0},
    {s_icon8,&IconsSmall[7],0},
    {s_icon9,&IconsSmall[8],0},
    {s_icon10,&IconsSmall[9],0},
    {s_icon11,&IconsSmall[10],0},
    {s_icon12,&IconsSmall[11],0},
    {s_icon13,&IconsSmall[12],0},
    {s_icon14,&IconsSmall[13],0},
    {s_icon15,&IconsSmall[14],0},
    {s_icon16,&IconsSmall[15],0},
    {s_icon17,&IconsSmall[16],0},
    {s_icon18,&IconsSmall[17],0},
    {s_icon19,&IconsSmall[18],0},
    {s_icon20,&IconsSmall[19],0},
    {s_icon21,&IconsSmall[20],0},
    {s_icon22,&IconsSmall[21],0},
    {s_icon23,&IconsSmall[22],0},
    {s_icon24,&IconsSmall[23],0},
    {s_icon25,&IconsSmall[24],0},

    {unlock_none_xpm,&securityNone,0},
    {unlock_unknown_xpm,&securityUnknown,0},
    {lock_kerberos4_xpm,&securityKerberos4,0},
    {lock_kerberos5_xpm,&securityKerberos5,0},
    {lock_basic_xpm,&securityBasic,0},
    {lock_domain_xpm,&securityDomain,0},
    {lock_md5_xpm,&securityMd5,0},
    {lock_login_xpm,&securityLogin,0},
        
    {toolbar_back_1_xpm,&toolbarBack,0},        
    {toolbar_forw_1_xpm,&toolbarForward,0},        
    {toolbar_back_1_xpm,&toolbarBackGRAY,1},        
    {toolbar_forw_1_xpm,&toolbarForwardGRAY,1},        
    {toolbar_home_1_xpm,&toolbarHome,0},        
    {toolbar_reload_1_xpm,&toolbarReload,0},        
    {toolbar_open_1_xpm,&toolbarOpen,0},        
    {toolbar_save_1_xpm,&toolbarSave,0},        
    {toolbar_open_window_1_xpm,&toolbarNew,0},        
    {toolbar_clone_window_1_xpm,&toolbarClone,0},        
    {toolbar_close_window_1_xpm,&toolbarClose,0},
    {toolbar_hotlist_1_xpm,&toolbarAddHotlist,0},
    {toolbar_news_groups_1_xpm,&toolbarNewsGroups,0},
    {toolbar_news_list_1_xpm,&toolbarNewsIndex,0},
    {toolbar_next_art_1_xpm,&toolbarNewsFwd,0},
    {toolbar_next_thr_1_xpm,&toolbarNewsFFwd,0},
    {toolbar_prev_art_1_xpm,&toolbarNewsRev,0},
    {toolbar_prev_thr_1_xpm,&toolbarNewsFRev,0},
    {toolbar_post_1_xpm,&toolbarPost,0},
    {toolbar_followup_1_xpm,&toolbarFollow,0},
    {toolbar_next_art_1_xpm,&toolbarNewsFwdGRAY,1},
    {toolbar_next_thr_1_xpm,&toolbarNewsFFwdGRAY,1},
    {toolbar_prev_art_1_xpm,&toolbarNewsRevGRAY,1},
    {toolbar_prev_thr_1_xpm,&toolbarNewsFRevGRAY,1},
    {toolbar_post_1_xpm,&toolbarPostGRAY,1},
    {toolbar_followup_1_xpm,&toolbarFollowGRAY,1},
    {toolbar_search_1_xpm,&toolbarSearch,0},
    {toolbar_print_1_xpm,&toolbarPrint,0},
    {toolbar_ftp_put_1_xpm,&toolbarFTPput,0},
    {toolbar_ftp_mkdir_1_xpm,&toolbarFTPmkdir,0},
    
    {tearv_xpm,&tearv,0},        
    {tearh_xpm,&tearh,0},        

    {not_secure_xpm, &enc_not_secure, 0},
    {NULL, NULL,0}
};

    
static GC DrawGC = NULL;
int IconWidth = 0;
int IconHeight = 0;
int WindowWidth = 0;
int WindowHeight = 0;

static struct color_rec {
        int pixel[3];
        int pixelval;
        struct color_rec *hash_next;
} *Hash[256];

static void FindIconColor(Display *dsp, Colormap colormap, XColor *colr);
static void PixAddHash(int red, int green, int blue, int pixval);
static int highbit(unsigned long ul);
static Pixmap PixmapFromData(Widget wid, unsigned char *data, int width,
                             int height, XColor *colrs, int gray);

static XColor def_colrs[256];
static int init_colors = 1;

/* Find the closest color by allocating it, or picking an already
 * allocated color
 */
static void FindIconColor(Display *dsp, Colormap colormap, XColor *colr)
{
	int i, match;
	int rd, gd, bd, dist, mindist;
	int cindx;

	if (init_colors) {
		for (i=0; i<256; i++) {
			def_colrs[i].pixel = -1;
			def_colrs[i].red = 0;
			def_colrs[i].green = 0;
			def_colrs[i].blue = 0;
		}
		init_colors = 0;
	}

	match = XAllocColor(dsp, colormap, colr);
	if (match == 0) {
		mindist = 196608;		/* 256 * 256 * 3 */
/*
		cindx = colr->pixel;
*/
		cindx = (-1);
		for (i=0; i<256; i++) {
			if (def_colrs[i].pixel == -1) {
				continue;
			}
			rd = ((int)(def_colrs[i].red >> 8) -
				(int)(colr->red >> 8));
			gd = ((int)(def_colrs[i].green >> 8) -
				(int)(colr->green >> 8));
			bd = ((int)(def_colrs[i].blue >> 8) -
				(int)(colr->blue >> 8));
			dist = (rd * rd) + (gd * gd) + (bd * bd);
			if (dist < mindist) {
				mindist = dist;
				cindx = def_colrs[i].pixel;
				if (dist == 0) {
					break;
				}
			}
		}
                if (cindx<0) {
                        colr->pixel=BlackPixel(dsp, DefaultScreen(dsp));
                        colr->red = colr->green = colr->blue = 0;
		} else {
			colr->pixel = cindx;
			colr->red = def_colrs[cindx].red;
			colr->green = def_colrs[cindx].green;
			colr->blue = def_colrs[cindx].blue;
		}
	} else {
		def_colrs[colr->pixel].pixel = colr->pixel;
		def_colrs[colr->pixel].red = colr->red;
		def_colrs[colr->pixel].green = colr->green;
		def_colrs[colr->pixel].blue = colr->blue;
	}
}


#define PixFindHash(red, green, blue, h_ptr) \
	h_ptr = Hash[((((red * 306) + (green * 601) + (blue * 117)) >> 10) >> 8)]; \
	while(h_ptr != NULL) \
	{ \
		if ((h_ptr->pixel[0] == red)&& \
		    (h_ptr->pixel[1] == green)&& \
		    (h_ptr->pixel[2] == blue)) \
		{ \
			break; \
		} \
		h_ptr = h_ptr->hash_next; \
	}

static void PixAddHash(int red, int green, int blue, int pixval)
{
	int lum;
	struct color_rec *hash_ptr;

	lum = ((((red * 306) + (green * 601) + (blue * 117)) >> 10) >> 8);

	hash_ptr = (struct color_rec *)XtMalloc(sizeof(struct color_rec));
	if (hash_ptr == NULL)
		return;
	hash_ptr->pixel[0] = red;
	hash_ptr->pixel[1] = green;
	hash_ptr->pixel[2] = blue;
	hash_ptr->pixelval = pixval;
	hash_ptr->hash_next = Hash[lum];
	Hash[lum] = hash_ptr;
}

static int highbit(unsigned long ul)
{
	/*
	 * returns position of highest set bit in 'ul' as an integer (0-31),
	 * or -1 if none.
	 */
 
	int i;
	for (i=31; ((ul&0x80000000) == 0) && i>=0;  i--, ul<<=1);
	return i;
}

static Pixmap
PixmapFromData(Widget wid, unsigned char *data, int width, int height,
		XColor *colrs, int gray)
{
	int i,t;
	int linepad, shiftnum;
	int shiftstart, shiftstop, shiftinc;
	int bytesperline;
	int temp;
	int w, h;
	XImage *newimage;
	unsigned char *bit_data, *bitp, *datap;
	unsigned char *tmpdata;
	Pixmap pix;
	int Mapping[256];
	XColor tmpcolr;
	int size;
	int depth;
	Visual *theVisual;
	int bmap_order;
	unsigned long c;
	int rshift, gshift, bshift;

        
	if (data == NULL)
		return(0);

	depth = DefaultDepthOfScreen(XtScreen(wid));

        for (i=0; i < 256; i++) {
		struct color_rec *hash_ptr;

                tmpcolr.red = colrs[i].red;
                tmpcolr.green = colrs[i].green;
                tmpcolr.blue = colrs[i].blue;
                tmpcolr.flags = DoRed|DoGreen|DoBlue;
                if ((mMosaicVisualClass == TrueColor) || (mMosaicVisualClass == DirectColor)) {
                        Mapping[i] = i;
                } else {
			PixFindHash(tmpcolr.red, tmpcolr.green, tmpcolr.blue,
				hash_ptr);
			if (hash_ptr == NULL) {
				FindIconColor(XtDisplay(wid), mMosaicColormap ,
					&tmpcolr);
				PixAddHash(colrs[i].red, colrs[i].green,
					colrs[i].blue, tmpcolr.pixel);
				Mapping[i] = tmpcolr.pixel;
			} else {
				Mapping[i] = hash_ptr->pixelval;
			}
                }
        }

	size = width * height;
	tmpdata = (unsigned char *)malloc(size);
	datap = data;
	bitp = tmpdata;
        if(gray) {
            t = Mapping[(int)*datap];
            for (i=0; i < size; i++) {
                    *bitp++ = (i+((i/width)%2))%2?(unsigned char)Mapping[(int)*datap]:t;
                    datap++;
                }
        } else {
            for (i=0; i < size; i++) {
                    *bitp++ = (unsigned char)Mapping[(int)*datap];
                    datap++;
                }
        }
              
	free((char *)data);
	data = tmpdata;

	switch(depth) {
	    case 6:
	    case 8:
		bit_data = (unsigned char *)malloc(size);
		memcpy(bit_data, data, size);
		bytesperline = width;
		newimage = XCreateImage(XtDisplay(wid),
			DefaultVisual(XtDisplay(wid),
				DefaultScreen(XtDisplay(wid))),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 8, bytesperline);
		break;
	    case 1:
	    case 2:
	    case 4:
		if (BitmapBitOrder(XtDisplay(wid)) == LSBFirst) {
			shiftstart = 0;
			shiftstop = 8;
			shiftinc = depth;
		} else {
			shiftstart = 8 - depth;
			shiftstop = -depth;
			shiftinc = -depth;
		}
		linepad = 8 - (width % 8);
		bit_data = (unsigned char *)malloc(((width + linepad) * height)
				+ 1);
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
		bytesperline = (width + linepad) * depth / 8;
		newimage = XCreateImage(XtDisplay(wid),
			DefaultVisual(XtDisplay(wid),
				DefaultScreen(XtDisplay(wid))),
			depth, ZPixmap, 0, (char *)bit_data,
			(width + linepad), height, 8, bytesperline);
		break;
	    /*
	     * WARNING:  This depth-16 code is donated code for 16-bit
	     * TrueColor displays.  I have no access to such displays, so I
	     * can't really test it.
	     * Donated by:  andrew@icarus.demon.co.uk
	     * Fixed version donated by:  nosmo@ximage.com (Vince Kraemer)
	     * ...and patched by GRR
	     */
	    case 16:
		bit_data = (unsigned char *)malloc(size * 2);
		bitp = bit_data;
		datap = data;

		theVisual = DefaultVisual(XtDisplay(wid),
			DefaultScreen(XtDisplay(wid)));
		rshift = 15 - highbit(theVisual->red_mask);
		gshift = 15 - highbit(theVisual->green_mask);
		bshift = 15 - highbit(theVisual->blue_mask);
		bmap_order = BitmapBitOrder(XtDisplay(wid));

		for (w = size; w > 0; w--) {
			temp = (((colrs[(int)*datap].red >> rshift) &
					theVisual->red_mask) |
				((colrs[(int)*datap].green >> gshift) &
					theVisual->green_mask) |
				((colrs[(int)*datap].blue >> bshift) &
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

		newimage = XCreateImage(XtDisplay(wid),
			DefaultVisual(XtDisplay(wid),
				DefaultScreen(XtDisplay(wid))),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 16, 0);
		break;
	    case 24:
		bit_data = (unsigned char *)malloc(size * 4);

		theVisual = DefaultVisual(XtDisplay(wid),
			DefaultScreen(XtDisplay(wid)));
		rshift = highbit(theVisual->red_mask) - 7;
		gshift = highbit(theVisual->green_mask) - 7;
		bshift = highbit(theVisual->blue_mask) - 7;
		bmap_order = BitmapBitOrder(XtDisplay(wid));

		bitp = bit_data;
		datap = data;
		for (w = size; w > 0; w--) {
			c =
			  (((colrs[(int)*datap].red >> 8) & 0xff) << rshift) |
			  (((colrs[(int)*datap].green >> 8) & 0xff) << gshift) |
			  (((colrs[(int)*datap].blue >> 8) & 0xff) << bshift);

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

		newimage = XCreateImage(XtDisplay(wid),
			DefaultVisual(XtDisplay(wid),
				DefaultScreen(XtDisplay(wid))),
			depth, ZPixmap, 0, (char *)bit_data,
			width, height, 32, 0);
		break;
	    default:
		newimage = NULL;
	}
	free((char *)data);

	if (newimage != NULL) {
		GC drawGC;

		pix = XCreatePixmap(XtDisplay(wid), XtWindow(wid),
			width, height, depth);
		drawGC = XCreateGC(XtDisplay(wid), XtWindow(wid), 0, NULL);
		XSetFunction(XtDisplay(wid), drawGC, GXcopy);

		XPutImage(XtDisplay(wid), pix, drawGC, newimage, 0, 0,
			0, 0, width, height);
		XFreeGC(XtDisplay(wid), drawGC);
		XDestroyImage(newimage);
		return(pix);
	} else {
		return(0);
	}
}

void MakePixmaps(Widget wid)
{
	unsigned char *data;
	int i, w, h, bg;
	XColor colrs[256];

	for (i=0; i<256; i++)
		Hash[i] = NULL;

/* load pixmaps */
	for(i=0;pix_info[i].raw;i++) {
		data =_MMProcessXpm3Data(wid, pix_info[i].raw, &w, &h, colrs, &bg);
		*(pix_info[i].handle) = PixmapFromData(wid, data, w, h,
			colrs, pix_info[i].gray);
	}
	IconPixSmall = IconsSmall;
	IconPixBig = IconsBig;
	IconPix = IconPixBig;
}

void AnimatePixmapInWidget(Widget wid, Pixmap pix)
{
	Cardinal argcnt;
	Arg arg[5];
	int x, y;

	if ((WindowWidth == 0)||(WindowHeight == 0)) {
		Dimension w, h;

		argcnt = 0;
		XtSetArg(arg[argcnt], XtNwidth, &w); argcnt++;
		XtSetArg(arg[argcnt], XtNheight, &h); argcnt++;
		XtGetValues(wid, arg, argcnt);
		WindowWidth = w;
		WindowHeight = h;
	}

	if (DrawGC == NULL) {
		DrawGC = XCreateGC(XtDisplay(wid), XtWindow(wid), 0, NULL);
		XSetFunction(XtDisplay(wid), DrawGC, GXcopy);
	}
	x = (WindowWidth - IconWidth) / 2;
	if (x < 0)
		x = 0;
	y = (WindowHeight - IconHeight) / 2;
	if (y < 0)
		y = 0;
	XCopyArea(XtDisplay(wid), pix, XtWindow(wid), DrawGC,
		0, 0, IconWidth, IconHeight, x, y);
}
