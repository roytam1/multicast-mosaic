/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Created: Wed Apr 10 17:41:00 CDT 1996
 * Author: Dan Pape
 */

#ifndef __GIFREAD_H__
#define __GIFREAD_H__

unsigned char *ReadGIF(FILE *fd, int *w, int *h, XColor *colrs, int *bg);

#endif
