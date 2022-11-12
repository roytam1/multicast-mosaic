#ifndef __APPROX_H__
#define __APPROX_H__

/* these colors filled in by function approx_colors() */
XColor approx_black;
XColor approx_white;
XColor approx_red;
XColor approx_green;
XColor approx_blue;
XColor approx_blue2;
XColor approx_grey;
XColor approx_grey68;
XColor approx_purple4;
XColor approx_dark_slate_grey;


void FixMotifColors(Widget w);
void approx_colors(Display * dpy, Colormap cmap);
Boolean near_color(Display *dpy, Colormap cmap, XColor *cp);

#endif /* not __APPROX_H__ */
