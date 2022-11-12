/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Author: DXP */

#ifdef  __cplusplus
extern "C" {
#endif

#define PNG_INTERNAL 1

#include "png.h"

#ifdef  __cplusplus
}
#endif

unsigned char * ReadPNG(FILE *infile,int *width, int *height, XColor *colrs);

