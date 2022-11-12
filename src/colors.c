
/*    Color Stabilization for Motif Widgets (1/5/98)   */
/*    See end of file for public function              */

#include <stdlib.h>
#include <X11/StringDefs.h>
#include <X11/Intrinsic.h>

/*    Widgets with Special Color Resources    */

#include <Xm/ScrollBar.h>
#include <Xm/ToggleB.h>
#include <Xm/PushB.h>
#include "../libhtmlw/HTML.h"

#include "colors.h"


#define MX_C    6    /* at most 6 color resources per Motif widget */

typedef struct {
    XColor rgb[MX_C];
    Pixel ocolor[MX_C];
} ColorSaver;

#define INIT_COLOR     0
#define RESTORE_COLOR  1
#define APPROX_COLOR   2

/*    Maintain color stability for a Pixel resource of a widget    */
static void fix_resource(Widget w, int cause, ColorSaver * csp,
			 XtPointer cdata, char * resource, int index)
{
    switch(cause) {
    case INIT_COLOR:    /* First Time Colormap is attached to Window */
/*         XtVaGetValues(w, resource, &(csp->rgb[index].pixel), NULL); */
/*         csp->ocolor[index] = csp->rgb[index].pixel; */

        /*    Fill RGB values for pixel    */
/*         XQueryColor(XtDisplay(w), (Colormap)cdata, &(csp->rgb[index])); */
        return;

    case RESTORE_COLOR:    /* Own Colormap (re)installed */
/*         XtVaSetValues(w, resource, csp->ocolor[index], NULL); */
        return;

    case APPROX_COLOR: /* Other Colormap installed */
/*         if(match(&(csp->rgb[index]), (XColor *)cdata)) */
/*             XtVaSetValues(w, resource, csp->rgb[index].pixel, NULL); */
        return;
    }
}

/*    Maintain color stability for ALL Pixel resources of a widget    */
static void motif_colors(Widget w, int n, ColorSaver * csp, XtPointer cdata)
{
    if(XtIsSubclass(w, xmPrimitiveWidgetClass) == False
       && XtIsSubclass(w, xmManagerWidgetClass) == False)
	return;
    XtVaSetValues(w, XtNbackground, approx_grey68.pixel, NULL);
    XtVaSetValues(w, XtNforeground, approx_black.pixel, NULL);
    XtVaSetValues(w, XmNtopShadowColor, approx_white.pixel, NULL);
    XtVaSetValues(w, XmNbottomShadowColor, approx_black.pixel, NULL);
    XtVaSetValues(w, XmNhighlightColor, approx_purple4.pixel, NULL);

    if(XtClass(w) == xmScrollBarWidgetClass) {
	XtVaSetValues(w, XmNtroughColor, approx_grey68.pixel, NULL);
    }
    else if(XtClass(w) == xmToggleButtonWidgetClass) {
	XtVaSetValues(w, XmNselectColor, approx_grey68.pixel, NULL);
    }
    else if(XtClass(w) == xmPushButtonWidgetClass) {
	XtVaSetValues(w, XmNarmColor, approx_grey68.pixel, NULL);
    }
    else if(XtClass(w) == htmlWidgetClass) {
	XtVaSetValues(w, WbNanchorColor, approx_blue2.pixel, NULL);
	XtVaSetValues(w, WbNvisitedAnchorColor, approx_purple4.pixel, NULL);
	XtVaSetValues(w, WbNactiveAnchorFG, approx_red.pixel, NULL);
	XtVaSetValues(w, WbNactiveAnchorBG, approx_grey68.pixel, NULL);
    }
}

/*    Color approximation using Manhattan distance    */
/*    Changes only pixel value, leaves RGB intact    */

static int match(XColor * cellp, XColor * given)
{
/*     long i, d, d0, i0; */
/*     XColor *cp; */
     int status = 0;

/*     d0 = 65535*3; */
/*     for(i=0, cp=given; i < 256; i++, cp++) { */
/*         d = abs(cellp->red - cp->red) + */
/*             abs(cellp->green - cp->green) + */
/*             abs(cellp->blue - cp->blue); */
/*         if(d < d0) { d0 = d;  i0 = i; status = 1; } */
/*     } */
/*     if(status) cellp->pixel = i0; */
     return status;
}

Boolean near_color(Display *dpy, Colormap cmap, XColor *cp)
{
    long i, d, d0, i0;
    static XColor existing_cells[256];
    Boolean status = False;
    
    /*    Find out the existing colors in the given colormap */
    for (i = 0; i < 256; i++) existing_cells[i].pixel = i;
    XQueryColors(dpy, cmap, existing_cells, 256);
    
    d0 = 65535*3;    /* distance between pure white and pure black */
    for (i = 0; i < 256; i++) {
        d = abs(cp->red - existing_cells[i].red) +
            abs(cp->green - existing_cells[i].green) +
            abs(cp->blue - existing_cells[i].blue);
        if(d < d0) { d0 = d;  i0 = i; status = True; }
    }
    if(status) cp->pixel = i0;
    return status;
}

/*    Event Handler for Colormap Change    */
static void colormap_change(Widget w, XtPointer client_data, XEvent * ep, Boolean * disp)
{
    ColorSaver *csp = (ColorSaver *)client_data;

#if defined(__cplusplus) || defined(c_plusplus)
    if(ep->xcolormap.c_new) {
#else
    if(ep->xcolormap.new) {
#endif
        /*    Establish Colors    */
        motif_colors(w, INIT_COLOR,
                csp, (XtPointer)ep->xcolormap.colormap);
    }
    else {
        if(ep->xcolormap.state==ColormapInstalled) {
            /* Restore Colors */
            motif_colors(w, RESTORE_COLOR, csp, NULL);
        }
        else {    /* Own Colormap Uninstalled - Approximate Colors */
            int i;
            int nc;
            XColor existing_cells[256];
            Display *Dpy = XtDisplay(w);
            Colormap *i_cmap = XListInstalledColormaps(Dpy,
                        DefaultRootWindow(Dpy), &nc);
            for(i=0; i < 256; i++) existing_cells[i].pixel = i;
            XQueryColors(Dpy, *i_cmap, existing_cells, 256);
            motif_colors(w, APPROX_COLOR,
                    csp, (XtPointer)existing_cells);
        }
    }
}

/*    Color approximation using Manhattan distance    */
static Boolean approx_color(Display * dpy, Colormap cmap,
			    XColor * existing_cells, int cells_number,
		     XColor *cp, int r, int g, int b)
{
    long i, d, d0, i0;
    Boolean status = False;

    cp->red = r;
    cp->green = g;
    cp->blue = b;
    /* if color will allocate in colormap that's all */
    if (XAllocColor(dpy, cmap, cp)) return 0;

    d0 = 65535*3;    /* distance between pure white and pure black */
    for (i = 0; i < cells_number; i++) {
        d = abs(cp->red - existing_cells[i].red) +
            abs(cp->green - existing_cells[i].green) +
            abs(cp->blue - existing_cells[i].blue);
        if(d < d0) { d0 = d;  i0 = i; status = True; }
    }
    if(status) cp->pixel = i0;
    return status;
}

void approx_colors(Display * dpy, Colormap cmap)
{
    int i;
    static XColor existing_cells[256];

    /*    Find out the existing colors in the given colormap */
    for (i = 0; i < 256; i++) existing_cells[i].pixel = i;
    XQueryColors(dpy, cmap, existing_cells, 256);
    /* find out approximate pixels by given RGB values */
    approx_color(dpy, cmap, existing_cells, 256, &approx_black, 0, 0, 0);
    approx_color(dpy, cmap, existing_cells, 256, &approx_white, 65535, 65535, 65535);
    approx_color(dpy, cmap, existing_cells, 256, &approx_grey68, 44288, 44288, 44288);
    approx_color(dpy, cmap, existing_cells, 256, &approx_grey, 48640, 48640, 48640);
    approx_color(dpy, cmap, existing_cells, 256, &approx_red, 65535, 0, 0);
    approx_color(dpy, cmap, existing_cells, 256, &approx_green, 0, 65535, 0);
    approx_color(dpy, cmap, existing_cells, 256, &approx_blue, 0, 0, 65535);
    approx_color(dpy, cmap, existing_cells, 256, &approx_blue2, 0, 0, 61166);
    approx_color(dpy, cmap, existing_cells, 256, &approx_purple4, 21760, 6656, 35584);
    approx_color(dpy, cmap, existing_cells, 256, &approx_dark_slate_grey,
		 12079, 20303, 20303);
}

/*    PUBLIC FUNCTION                                           */
/*    Stabilize resource colors of widget tree                  */
/*    It should be called just before XtRealizeWidget()         */

/*    The function adds an event handler for colormap changes   */
/*    to each widget in the tree. Space is allocated for        */
/*    client data in the event handler that is used for         */
/*    passing information between invocations                   */

void FixMotifColors(Widget w)
{
    int i, nk;
    Widget *children;

    if(XtIsComposite(w)) {
        XtVaGetValues(w, XtNnumChildren, &nk, NULL);
        XtVaGetValues(w, XtNchildren, &children, NULL);
        for(i=0; i < nk; i++) FixMotifColors(children[i]);
    }
    motif_colors(w, APPROX_COLOR, NULL, NULL);
/*     XtAddEventHandler( w, ColormapChangeMask, False, colormap_change, */
/*             (XtPointer)malloc(sizeof(ColorSaver)) ); */
}
