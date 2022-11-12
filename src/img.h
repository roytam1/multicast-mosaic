/* img.h
 * Author: Gilles Dauphin
 * Version 1.0 [Jul96]
 *
 * Copyright (C) - 1996 G.Dauphin, P.Dax (ENST)
 *
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION
 * 
 * Bug report : dauphin@tsi.enst.fr
 */

extern void MMPreParseInputTagImage(mo_window * win, ImageInfo *picd, struct mark_up *mptr);
extern void MMPreParseImageTag(mo_window * win, ImageInfo *picd, struct mark_up *mptr);
extern void MMPreParseImageBody(mo_window * win, ImageInfo *picd, struct mark_up *mptr);
extern void MMGetImageFromCache(Widget hw, ImageInfo * picd, char * url,
	char *base_url);
extern void MMPreloadImage(mo_window *win, struct mark_up *mptr, MimeHeaderStruct *mhs,
	char *fname);

