/* Copyright ENST 1999 */

#include <stdio.h>
#include <stdlib.h>
#include <malloc.h>
#include <ctype.h>
#include <math.h>

#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

#include "HTMLP.h"

/*
 * suivant la cmap determine le nbre de couleur disponibles
 *	XAllocColorCells puis XFreeColors
 * prendre la racine cubique, cela donne la taille mini d'un cote du cube
 * Augmente chaque cote du cube pour avoir un cube max:
 * Exemple max_col = 50 => cube de 3 3 4 (48 couleurs)
 */

/* Cette widget a ete cree avec sa colormap, La colormap a deja qqes choses
 * d'alloue, puisque utilisee par Motif, mis dans la TopLevelShell par XtSetValues,
 * hw est la widget HTML, On appelle
 * cette routine apres la creation de la premiere widget HTML.
 * Pour completer la table de couleur
 */

static XColor ***ColorCube;
static unsigned int Nr, Ng, Nb;	/* ColorCube size */
static Colormap Cmap;
static Display *Dpy;
static int V_class;	/* visual class DirectColor, PseudoColor, */
			/* GrayScale, StaticColor, StaticGray, TrueColor*/

#define REDUCE_RED 1
#define REDUCE_GREEN 2
#define REDUCE_BLUE 3

static void ComputeSide(unsigned *n, unsigned *nr, unsigned *ng, unsigned *nb,
	int r_what)
{
	unsigned int nc = *n;
	unsigned int r = *nr;
	unsigned int g = *ng;
	unsigned int b = *nb;
	unsigned int nccube;

	nccube = r * g * b;
	if (nccube <= 4) {
		*n = 4;
		*nr = 2;
		*ng = 2;
		*nb = 1;
		return;
	}
	if (nccube <= nc ) {
		*n = nccube;
		return;	/* value of nr ng nb is good */
	}
	switch (r_what) {
	case REDUCE_RED :
		*nr = *nr - 1;
		ComputeSide(n, nr, ng, nb, REDUCE_BLUE);
		break;
	case REDUCE_GREEN :
		*ng = *ng - 1;
		ComputeSide(n, nr, ng, nb, REDUCE_RED);
		break;
	case REDUCE_BLUE :
		*nb = *nb - 1;
		ComputeSide(n, nr, ng, nb, REDUCE_GREEN);
		break;
	}
}

static void GetColorCubeSize(unsigned *n, unsigned *nr, unsigned *ng, unsigned *nb)
{
	unsigned int nc = *n;
	unsigned int r = 16;
	unsigned int g = 16;
	unsigned int b = 16;

	if (nc > 4096) {
		nc = *n = 4096;
	}
	if (nc <= 4) {
		*n = 4;
		*nr = 2;
		*ng = 2;
		*nb = 1;
		return;
	}
	ComputeSide(&nc, &r, &g, &b, REDUCE_BLUE);
	*n = nc;
	*nr = r;
	*ng = g;
	*nb = b;
	return;
}

static XColor ***AllocColorCube(unsigned int nr, unsigned int ng, unsigned int nb)
{
	XColor ***cc;
	int i,j;

	cc = (XColor ***) malloc(nb * sizeof(XColor **));
	for(i = 0; i < nb; i++) {
		cc[i] = (XColor **) malloc(ng * sizeof(XColor *));
		for(j = 0; j < ng; j++) {
			cc[i][j] = (XColor *) malloc(nr * sizeof(XColor));
		}
	}
	return cc;
}

void HTMLInitColors(Widget w, unsigned int nc)
{
	Screen * screen;
	int screen_num;
	Arg args[10];
	int n,i, j, k, cnt, status;
	XVisualInfo vinfo, *vptr;
	unsigned int depth, use_depth, use_ncell;
	Pixel black,white, prev_pixel;
	Pixel *ct;
	static XColor ***cc;
	double step, cur_color, stepr, stepg, stepb, cur_r, cur_g, cur_b, dr,dg,db;
	XColor color;
	unsigned int alloc;

	n = 0;
	XtSetArg(args[n],XtNcolormap,&Cmap); n++;
	XtGetValues(w,args,n);

	Dpy = XtDisplay(w);
	screen = XtScreen(w);
	screen_num = DefaultScreen(Dpy);

	vinfo.visualid = XVisualIDFromVisual(DefaultVisual(Dpy, screen_num));
	vptr = XGetVisualInfo (Dpy, VisualIDMask, &vinfo, &cnt);

#if defined(__cplusplus) || defined(c_plusplus)
	V_class = vptr->c_class;          /* C++ */
#else
	V_class = vptr->class;
#endif
	depth = vptr->depth;

	black = BlackPixel(Dpy, screen_num);
	white = WhitePixel(Dpy, screen_num);
	if ( depth <= 2 ) {	/* like B&W */
		Nr = 2; Ng = Nb = 1;
		cc = AllocColorCube(Nr,Ng,Nb);
		/* gray is based on red */
		/* b  g  r */
		cc[0][0][0].pixel = black;
		cc[0][0][0].red = 0;
		cc[0][0][0].green = 0;
		cc[0][0][0].blue = 0;

		cc[0][0][1].pixel =  white;
		cc[0][0][1].red = 0xFFFF;
		cc[0][0][1].green = 0xFFFF;
		cc[0][0][1].blue = 0xFFFF;

		ColorCube = cc;
/* Localmap[c->pixel] */
		return;
	}

	use_depth = depth > 12 ? 12 : depth; /* max 4096 couleur 16**3  */
	use_ncell = 1 << depth;
	use_ncell = (use_ncell * 90) /100 ;
	if (nc == 0)
		nc = use_ncell;
	if ( nc < use_ncell)
		use_ncell = nc;
	if (use_ncell < 4)
		use_ncell = 4;

	switch (V_class) {
	case StaticGray: /* read only hardware */
	case GrayScale:	 /* read/write hardware */
		Nr = use_ncell; Ng = Nb = 1;
		cc = AllocColorCube(Nr,Ng,Nb);
		prev_pixel = black;
		step = 65536.0/ (double) use_ncell;
		cur_color = 0.0;
		for ( i = 0; i < Nr; i++) {
			color.red = color.green = color.blue = cur_color;
			color.flags = DoRed | DoGreen |DoBlue;
			if (!XAllocColor(Dpy, Cmap, &color)) {
				fprintf(stderr,"Missing Color %x ColorClass is %x\n",
					color.red, V_class);
				color.pixel = prev_pixel;
			}
			cc[0][0][i].pixel = color.pixel;
			cc[0][0][i].red = color.red ;
			cc[0][0][i].green = color.red ;
			cc[0][0][i].blue = color.red ;
			prev_pixel = color.pixel;
			cur_color += step;
		}
		cc[0][0][Nr-1].pixel = white;
		ColorCube = cc;
		break;

	case StaticColor: 	/* read only hardware one colormap*/
	case TrueColor: 	/* read only hardware with 3 colormap*/
	case DirectColor:	/* r/w hardware with 3 colormap*/
	case PseudoColor:	/* r/w hardware one colormap*/
		GetColorCubeSize(&use_ncell, &Nr, &Ng, &Nb);
		cc = AllocColorCube(Nr,Ng,Nb);
		ct = (Pixel*) malloc(use_ncell * sizeof(Pixel));
		status = 1;
		alloc = 0;
		dr = Nr > 1 ? (double) Nr - 1.0 : 1.0;
		dg = Nr > 1 ? (double) Ng - 1.0 : 1.0;
		db = Nr > 1 ? (double) Nb - 1.0 : 1.0;
		
		stepr = 65535.0 /  dr;
		stepg = 65535.0 /  dg;
		stepb = 65535.0 /  db ;
		cur_b = 0.0;
		for(i = 0; (i < Nb) && status ; i++) {
			cur_g = 0.0;
			for(j = 0; (j < Ng) && status ; j++) {
				cur_r = 0.0;
				for(k = 0; (k < Nr) && status ; k++) {
					color.blue = floor( cur_b);
					color.green = floor( cur_g);
					color.red = floor( cur_r);
					color.flags = DoRed | DoGreen |DoBlue;
					if ( !XAllocColor(Dpy, Cmap, &color)) {
						status = 0;
						break;
					}
					ct[alloc] = color.pixel;
					alloc++;
					cc[i][j][k] = color;
					cur_r += stepr;
				}
				cur_g += stepg;
			}
			cur_b += stepb;
		}
		ColorCube = cc;
		if (status) {
			return;
		}
/* recommence avec un nombre de couleur reduit */
		XFreeColors (Dpy, Cmap, ct, alloc, 0);
		free(ct);

		use_ncell = (alloc * 80) /100;
		if (use_ncell < 4)
			use_ncell = 4;
		GetColorCubeSize(&use_ncell, &Nr, &Ng, &Nb);
		dr = Nr > 1 ? (double) Nr - 1.0 : 1.0;
		dg = Nr > 1 ? (double) Ng - 1.0 : 1.0;
		db = Nr > 1 ? (double) Nb - 1.0 : 1.0;
		
		stepr = 65535.0 /  dr;
		stepg = 65535.0 /  dg;
		stepb = 65535.0 /  db ;
		cur_b = 0.0;
		for(i = 0; (i < Nb) && status ; i++) {
			cur_g = 0.0;
			for(j = 0; (j < Ng) && status ; j++) {
				cur_r = 0.0;
				for(k = 0; (k < Nr) && status ; k++) {
					color.blue = floor(cur_b);
					color.green = floor(cur_g);
					color.red = floor(cur_r);
					color.flags = DoRed | DoGreen |DoBlue;
					if ( !XAllocColor(Dpy, Cmap, &color)) {
/*						color.pixel = white;	*/
/*						color.red = 0xffff;	*/
/*						color.green = 0xffff;	*/
/*						color.blue = 0xffff;	*/
						fprintf(stderr,"mMosaic Warning: Cannot allocate colormap entry for \"#%4x%4x%4x\n", color.red, color.green, color.blue);
					}
					cc[i][j][k] = color;
					cur_r += stepr;
				}
				cur_g += stepg;
			}
			cur_b += stepb;
		}
		break;
	}
}


/* in
 *	xc : the XColor to get
 * return
 *	a pixel
 */
Pixel HTMLXColorToPixel(XColor * xc)
{
	unsigned short r,g,b;
	unsigned short min;
	int dist,i,ri,gi,bi;

	r = xc->red;
	g = xc->green;
	b = xc->blue;

	min = 0xFFFF;
	ri = 0;
	gi = 0;
	bi = 0;
	for ( i = 0; i < Nr ; i++ ) {
		dist = r - ColorCube[0][0][i].red;
		dist = dist > 0 ? dist : -dist;
		
		if ( min > dist ) {
			ri = i;
			min = dist;
		}
	}
	min = 0xFFFF;
	for ( i = 0; i < Ng ; i++) {
		dist = g - ColorCube[0][i][ri].green;
		dist = dist > 0 ? dist : -dist;
		if ( min > dist ) {
			gi = i;
			min = dist;
		}
	}
	min = 0xFFFF;
	for ( i = 0; i < Nb ; i++) {
		dist = b - ColorCube[i][gi][ri].blue;
		dist = dist > 0 ? dist : -dist;
		if ( min > dist ) {
			bi = i;
			min = dist;
		}
	}
	xc->pixel = ColorCube[bi][gi][ri].pixel;
	return (ColorCube[bi][gi][ri].pixel);
}


void HTMLPixelToXColor(XColor * c)
{
/*  DirectColor, PseudoColor, GrayScale, StaticColor, StaticGray, TrueColor*/
	switch (V_class) {
	case PseudoColor:
	case GrayScale:
	case StaticColor:
	case StaticGray:
/*		if (Localmap[c->pixel].allocated){
/*			*c = Localmap[c->pixel].xcolor;
/*			return;
/*		}
/*		XQueryColor(Dpy, Cmap, c);
/*		localmap[c->pixel].allocated = 1;
/*		localmap[c->pixel].xcolor = *c;
/*		return;
*/
	case TrueColor:
	case DirectColor:
		XQueryColor(Dpy, Cmap, c);
	}
}

void MMResetWidgetColorStack(HTMLWidget hw)
{
	ColorStack *ocstack;

	while ( hw->html.color_stack_bg->next != NULL) {
		ocstack = hw->html.color_stack_bg;
		hw->html.color_stack_bg = hw->html.color_stack_bg->next;
		free(ocstack);
	}
	while (hw->html.color_stack_fg->next != NULL) {
		ocstack = hw->html.color_stack_fg;
		hw->html.color_stack_fg = hw->html.color_stack_fg->next;
		free(ocstack);
	}
}

Pixel MMPopColorBg(HTMLWidget hw)
{
	ColorStack *ocstack;
        if (hw->html.color_stack_bg->next == NULL) {
#ifdef HTMLTRACE
                fprintf(stderr, "Warning, pop empty color stack!\n");
#endif
                return hw->html.color_stack_bg->pixel;
        }
        ocstack = hw->html.color_stack_bg;
        hw->html.color_stack_bg = hw->html.color_stack_bg->next;
        free(ocstack);
        return hw->html.color_stack_bg->pixel;

}
Pixel MMPopColorFg(HTMLWidget hw)
{
	ColorStack *ocstack;
        if (hw->html.color_stack_fg->next == NULL) {
#ifdef HTMLTRACE
                fprintf(stderr, "Warning, pop empty color stack!\n");
#endif
                return hw->html.color_stack_fg->pixel;
        }
        ocstack = hw->html.color_stack_fg;
        hw->html.color_stack_fg = hw->html.color_stack_fg->next;
        free(ocstack);
        return hw->html.color_stack_fg->pixel;
}

Pixel MMPushColorBg(HTMLWidget hw, Pixel new_pix)
{
	ColorStack *ocstack;

	ocstack = hw->html.color_stack_bg;

        hw->html.color_stack_bg = (ColorStack*) calloc(1,sizeof(ColorStack));
	hw->html.color_stack_bg->pixel = new_pix;
        hw->html.color_stack_bg->next = ocstack;
        return hw->html.color_stack_bg->pixel;
}
Pixel MMPushColorFg(HTMLWidget hw, Pixel new_pix)
{
	ColorStack *ocstack;

	ocstack = hw->html.color_stack_fg;

        hw->html.color_stack_fg = (ColorStack*) calloc(1,sizeof(ColorStack));
	hw->html.color_stack_fg->pixel = new_pix;
        hw->html.color_stack_fg->next = ocstack;
        return hw->html.color_stack_fg->pixel;
}

static void GetDefaultWidgetColorBg (HTMLWidget hw, Pixel * bg)
{
	*bg = hw->core.background_pixel;

}
static void GetDefaultWidgetColorFg (HTMLWidget hw, Pixel * fg)
{
	*fg = hw->manager.foreground;
}
void MMInitWidgetColorStack(HTMLWidget hw)
{
        static Pixel opixfg, opixbg;

        GetDefaultWidgetColorFg(hw, &opixfg);
        GetDefaultWidgetColorBg(hw, &opixbg);

        hw->html.color_stack_fg = (ColorStack*) calloc(1,sizeof(ColorStack));
        hw->html.color_stack_bg = (ColorStack*) calloc(1,sizeof(ColorStack));
	hw->html.color_stack_fg->pixel = opixfg;
	hw->html.color_stack_bg->pixel = opixbg;
        hw->html.color_stack_fg->next = NULL;
        hw->html.color_stack_bg->next = NULL;
}
