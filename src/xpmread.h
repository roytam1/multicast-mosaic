/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __XPMREAD_H__
#define __XPMREAD_H__

unsigned char *_MMReadXpm3Pixmap(Widget view, FILE *fp,char *datafile, int *w, int *h,
                              XColor *colrs, int *bg);
unsigned char *_MMProcessXpm3Data(Widget wid, char **xpmdata, int *w,
                               int *h, XColor *colrs, int *bg);

#endif
