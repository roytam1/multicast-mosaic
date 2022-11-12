/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

/* this file contains stuff from the old "mosaic.h" file. I am breaking
   that file up because it was too big, and required a re-compile of all
   the source whenever something changed. */

#ifndef __XPMREAD_H__
#define __XPMREAD_H__

unsigned char *ReadXpm3Pixmap(FILE *fp, char *datafile, int *w, int *h,
                              XColor *colrs, int *bg);
unsigned char *ProcessXpm3Data(Widget wid, char **xpmdata, int *w,
                               int *h, XColor *colrs, int *bg);

#endif
