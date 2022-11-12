/* some code part is from XmHTML, so please read the Copyright. */
#include "../Copyrights/Copyright.XmHTML"

/***** font loading & caching routines.  *****/ 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>		/* isspace */
#include <assert.h>

#include "HTMLmiscdefs.h"
#include "HTMLP.h"

/* A single font cache entry.
* The font cache is a binary tree, sorted according to the name of a font.
*/
typedef struct _fontCacheEntry{
	FontRec *font;			/* font data */
	String name;			/* font name */
	struct _fontCacheEntry *left;
	struct _fontCacheEntry *right;
}FontCacheEntry;

/* Definition of a display-bound font cache.
* This structure contains the ID of the display this cache is used for,
* the actual font cache, the default font for this display
*****/
typedef struct _fontCache{
	Display *dpy;			/* display were fonts come from */
	int res_x;			/* horizontal screen resolution */
	int res_y;			/* vertical screen resolution */
	FontCacheEntry *top_ent;	/* cached fonts for this display */
	FontCacheEntry *cur_ent;	/* current */
	FontRec *default_font;		/* default font */
}FontCache;

static FontCache font_cache = {		/* master font cache */
	NULL, 0, 0, NULL, NULL, NULL};

static int PseudoXFontSize[] = {
	0, 8,
	9, 10, 11, 12, 14, 16, 18,
	20, 24, 30, 34, 40, 50, 60, 72
};

static int num_PseudoXFontSize = sizeof(PseudoXFontSize)/sizeof(int);
static int f_off3 = 3;	/* index offset to size 14 pica */

/* looks for a font in the fontcache;
*	entry:	current font cache;
*	name:	name of font to locat;
* Returns:
*	a valid font if name is found, NULL if not found.
*/
static FontRec* getCacheFont(FontCacheEntry *top, char *fontname)
{
	if(top != NULL) {
		int ret_val = strcmp(fontname, top->name);

		if(ret_val == 0 )
			return(top->font);
		if(ret_val < 0)
			return(getCacheFont(top->left, fontname));
		else
			return(getCacheFont(top->right, fontname));
	}
	return(NULL);
}

/* inserts the given font in the given font cache.
*	entry:		current font cache;
* Returns:
*	updated cache entry;
*/
static FontCacheEntry* CacheFontStore(FontCacheEntry *top, FontCacheEntry *ent)
{
	int ret_val;

	if (!top) 
		return ent;

	ret_val = strcmp(ent->name, top->name);
	if(ret_val == 0 ) { /* probleme because we test the cache before */
/* evguenii@ess.co.at */
		/* due to an algoritm of font choosing it's normal
		   that program can come in this point */
/*		assert(0);   */
/*		return(top); */
	}
	if(ret_val < 0)
		top->left = CacheFontStore(top->left, ent);
	else
		top->right = CacheFontStore(top->right, ent);
	return(top);
}

/*****
* Description: 	retrieves a font from the cache or loads one when it isn't
*		already available.
* in:
*	hw		: widget
*	xlfd_name_pixel	: name with pixel size
*	xlfd_name_point	: name with point size
*	cache_fontname	: the name we MUST cache
*	frbuf		: a buffer we return as a value
* Returns:
*	a valid font if the font was loaded successfully, NULL if not.
*****/
static FontRec* loadAndCacheFont(HTMLWidget hw,
	char *xlfd_name_pixel, char *xlfd_name_point,
	char *cache_fontname, FontRec* frbuf)
{
	XFontStruct *xfont;
	FontCacheEntry *cache_entry;

/* A new font, try 1st a pixel size */
	xfont = XLoadQueryFont(XtDisplay(hw), xlfd_name_pixel);
#ifdef DEBUG_FONT
	fprintf(stderr, "Request: %s ",xlfd_name_pixel);
#endif

/* A new font, try 2sd a point size */
	if ( xfont == NULL) {
#ifdef DEBUG_FONT
		fprintf(stderr," Ans: NO\n");
#endif
		xfont = XLoadQueryFont(XtDisplay(hw), xlfd_name_point);
#ifdef DEBUG_FONT
		fprintf(stderr, "Request: %s ",xlfd_name_point);
#endif
	}

	if ( xfont == NULL) {
#ifdef DEBUG_FONT
		fprintf(stderr," Ans: NO\n");
#endif
		return NULL;
	}
#ifdef DEBUG_FONT
	fprintf(stderr," Ans: YES\n");
#endif

/* store it if successfull */
	frbuf->xfont = xfont;	/* update Xfont */
	/* get a new fontentry */
	cache_entry = (FontCacheEntry*) malloc(sizeof(FontCacheEntry));
	cache_entry->name = strdup(cache_fontname);
	cache_entry->font = frbuf;
	cache_entry->left = NULL;
	cache_entry->right = NULL;
		
	/* store in the cache */
	if (!font_cache.top_ent) {
		font_cache.top_ent = cache_entry;
		font_cache.default_font = frbuf;
		font_cache.cur_ent = cache_entry;
	} else {
		font_cache.cur_ent = CacheFontStore(font_cache.top_ent, cache_entry);
		font_cache.cur_ent = cache_entry;
	}
	/* return the new font */
	return(frbuf);
}

/* Description:  creates a full 14 field font name from the given args.
* In: 
*       name:           font base name, required
*       foundry:        font foundry, optional
*       family:         font family, optional
*       weight:         font weight, required
*       slant:          font slant, required
*       points:         font pointsize (tenths of a point), required
*       charset:        current ISO character set encoding
* Returns:
*       the composed font name
*       -foundry-family-weight-slant-width--*-points-res_x-res_y-spacing-*-charset*****/
static char* makeXLFDNamePointSize(HTMLWidget hw, char *fndry, char *family,
	char *weight, char *slant, int ptsize, char *charset)
{
        static char new_name[1024];

	new_name[0] = '\0';

        /* screen resolutions are stored in the display-bound font cache */
        sprintf(new_name, "-%s-%s-%s-%s-normal-*-*-%i-%i-%i-*-*-%s",
                fndry, family, weight, slant, ptsize,
		font_cache.res_x, font_cache.res_y, charset);          
                                       
        return(new_name);              
}                                      


/* Description: 	creates a full 14 field font name from the given args.
* In: 
*	foundry:	font foundry
*	family:		font family
*	weight:		font weight
*	slant:		font slant
*	pxsize:		font pixel size 
*	charset:	current ISO character set encoding
* Returns:
*	the composed font name
*	-foundry-family-weight-slant-width--pix-*-*-*-*-*-charset
*****/
static char * makeXLFDNamePixelSize(HTMLWidget hw, char *fndry, char *family,
	char *weight, char *slant, int pxsize, char *charset)
{
	static char ret_xlfdnam[1024];

	ret_xlfdnam[0] = '\0';
	sprintf(ret_xlfdnam, "-%s-%s-%s-%s-normal-*-%i-*-*-*-*-*-%s",
		fndry, family, weight, slant, pxsize, charset);

	return(ret_xlfdnam);
}

/* Name:			loadQueryFont
* Return Type:	XFontStruct*
* Description:	loads a font from the given family in given size, weight and 
*		slant. Loaded fonts are cached to minimize the overhead spent 
*		in XLoadQueryFont().
* In:
*	w:	Widget for which this font is to be loaded.
*	name:	XmHTML fontFamily spec. Only used when family is NULL.
*	family:	font family name of the font to load. When non-null this
*		contains the typeface of the font, e.i.: helvetica, symbol,
*				etc...
*	ptsz:	size of font to load, in tenths of a point
*	style:	style of this font.
*	loaded:	indicates whether the requested font was loaded or the current 
*		font was returned. When loaded is initially True, a warning
*		message is displayed if the font can't be loaded.
* Returns:
*	A XFontStruct* for the font in the requested family/size and loaded
*	set to True or the current font and loaded set to False.
* Note: 
*	This routine was based on the LoadQueryScalableFont() routine found in
*	O'Reilly's Xlib Programming Manual by Adrian Nye, but that's no longer
*	recognizable...
*
*	This routine goes through *GREAT* lengths to find the requested font, and
*	it will almost never fail (unless the XmNfontFamily/XmNcharset resources
*	form an invalid pair, but then XmHTML will give up on startup immediatly).
*****/

static FontRec* _LoadFont(HTMLWidget hw, char *fndry, char *family,
	char *weight, char *slant, char *size, char *charset,
	FontRec* ofptr)
{
	FontRec *font = NULL;
	FontRec *frbuf = NULL;
	char buf[20];
	char buf10[20];
	int rsize, asize,isize;
	char *xlfd_name_pixel, *xlfd_name_point, *cache_fontname;

/* process size */
	if ( size != NULL) { 	/* size = * abs_num(1 7) or rel_num (+2 -1) */
				/* size is a html size */
		switch (size[0]) {
		case '+' :
		case '-' :
			rsize = atoi(size);
			asize = atoi(ofptr->pixelsize) + rsize;
			break;
		default:
			asize = atoi(size) + f_off3;
		}
		if ( asize < 1 )
			asize = 1;
		if (asize >= num_PseudoXFontSize)
			asize = num_PseudoXFontSize - 1;
		size = buf;
		sprintf(size,"%i", asize);
	} else {
		size = ofptr->pixelsize;	/* absolute in ofptr->size */
	}
	isize = atoi(size);

/* Okay, now we are going to try and load a font. 
 * check the font with all info. Enlarge the search with wildcar.
 * wildcard:
 *	fndry family charset weight slant in this order.
 *	if no font is found , get default with scalable size.
 */
	if (fndry == NULL)	/* fndry = * adobe itc b&h ... */
		fndry = ofptr->fndry;
	if (family == NULL)	/* family = * courier times helvetica ... */
		family = ofptr->family;
	if (weight == NULL)	/* weight = * bold medium */
		weight = ofptr->weight;
	if (slant == NULL)     /* slant = * i r */
                slant = ofptr->slant;
	if (charset == NULL)	/* charset = *-* iso8859-1 */
		charset = ofptr->charset;

	xlfd_name_pixel = makeXLFDNamePixelSize(hw,fndry, family,
		weight, slant, PseudoXFontSize[isize] , charset);
	xlfd_name_point = makeXLFDNamePointSize(hw,fndry, family,
		weight, slant, PseudoXFontSize[isize]*10, charset);
	cache_fontname = xlfd_name_pixel;

	font = getCacheFont(font_cache.top_ent, cache_fontname);
	if (font)
		return font;		/* good */

/* at this point a font goes in a cache: alloc an entry and fill it */
	frbuf = (FontRec *) malloc(sizeof(FontRec));
	frbuf->xfont = NULL;
	frbuf->fndry = strdup(fndry);
	frbuf->family = strdup(family);
	frbuf->weight = strdup(weight);
	frbuf->slant = strdup(slant);
	frbuf->pixelsize = strdup(size);
	sprintf(buf10,"%i", isize*10);
	frbuf->pointsize = strdup(buf10);
	frbuf->xres = strdup("*");
	frbuf->yres = strdup("*");
	frbuf->charset = strdup(charset);
	frbuf->cache_name = strdup(xlfd_name_pixel);

	font = loadAndCacheFont(hw, xlfd_name_pixel, xlfd_name_point,
			cache_fontname, frbuf);

	if (!font) {	/* reduce criteria  (fndry) */
		xlfd_name_pixel = makeXLFDNamePixelSize(hw,"*", family,
			weight, slant, PseudoXFontSize[isize] , charset);
		xlfd_name_point = makeXLFDNamePointSize(hw,"*", family,
			weight, slant, PseudoXFontSize[isize]*10, charset);
		font = loadAndCacheFont(hw, xlfd_name_pixel, xlfd_name_point,
			cache_fontname, frbuf);
	}

	if (!font) {	/* reduce criteria  (weight & slant) */
		xlfd_name_pixel = makeXLFDNamePixelSize(hw,"*", family,
			"*", "*", PseudoXFontSize[isize] , charset);
		xlfd_name_point = makeXLFDNamePointSize(hw,"*", family,
			"*", "*", PseudoXFontSize[isize]*10, charset);
		font = loadAndCacheFont(hw, xlfd_name_pixel, xlfd_name_point,
			cache_fontname, frbuf);
	}

	if (!font) {	/* reduce criteria  (family but weight & slant) */
		xlfd_name_pixel = makeXLFDNamePixelSize(hw,"*", "*",
			weight, slant, PseudoXFontSize[isize] , charset);
		xlfd_name_point = makeXLFDNamePointSize(hw,"*", "*",
			weight, slant, PseudoXFontSize[isize]*10, charset);
		font = loadAndCacheFont(hw, xlfd_name_pixel, xlfd_name_point,
			cache_fontname, frbuf);
	}

	if (!font) {	/* reduce criteria  (family weight & slant) */
		xlfd_name_pixel = makeXLFDNamePixelSize(hw,"*", "*",
			"*", "*", PseudoXFontSize[isize] , charset);
		xlfd_name_point = makeXLFDNamePointSize(hw,"*", "*",
			"*", "*", PseudoXFontSize[isize]*10, charset);
		font = loadAndCacheFont(hw, xlfd_name_pixel, xlfd_name_point,
			cache_fontname, frbuf);
	}
		
	if( !font) {	/* WE MUST HAVE A FONT get something */
		font = loadAndCacheFont(hw, ofptr->cache_name,
			ofptr->cache_name, cache_fontname, frbuf);
			/*font = ofptr->font; */
		assert(font);  /* somethings goes wrongs... */
	}

	return(font);
}

/* possible markup is:
	<FONT> <TT> <I> <B> <BIG> <SMALL>
 translate in html font name:
	fndry-family-weight-slant-size-charset
 in X-Window font:
	-fndry-fmly-wght-slant-stylewdth-adstyl-pxlsz-ptSz-resx-resy-spc-avgWdth-rgstry-encoding
	fndry : adobe, b&h, butstream, ...
	fmly : times, helvetica, ...
	wght : bold medium normal roman
	slant: i r o
	stylewdth: narrow normal
	adstyl: sans , sherif
	pxlsz: *
	ptSz: 20 ... 100 ... 400
	resx: 72 75 100
	resy: 72 75 100
	spc:*
	avgWdth: *
	rgstry-encoding : iso8859-1 <=> charset (have 2 values)
exemple:
	-*-helvetica-medium-r-normal-*-12-*-75-75-p-*-iso8859-1
ou
	-*-helvetica-medium-r-normal-*-12-*-75-75-p-iso8859-1	(nonSun)
XmHTML fait comme ca :
	-foundry-family-weight-slant-width--*-points-res_x-res_y-spacing-*-charset
*/

/*newfont = _LoadFont(hw, fndry, family, weight, slant, size, charset, ofptr); */
void MMPushFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc)
{
        FontRec *newfont;
	FontStack *ofstack;
	FontRec *ofptr=NULL;
	char * fndry, *family, *size;
	char * font_color = NULL;
	Pixel fg;

	ofptr = hw->html.font_stack->font;

	switch (mptr->type) {
        case M_CODE:
        case M_SAMPLE:
        case M_KEYBOARD:
        case M_FIXED:
 	case M_PREFORMAT:
		newfont = _LoadFont(hw, NULL, "courier", NULL, NULL, NULL, NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
        case M_EMPHASIZED:
        case M_VARIABLE:
        case M_CITATION:
        case M_ITALIC:
	case M_DFN:
	case M_ADDRESS:
		newfont = _LoadFont(hw, NULL, NULL, NULL, "i", NULL, NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
        case M_STRONG:          
        case M_BOLD:
	case M_CAPTION:
		newfont = _LoadFont(hw, NULL, NULL, "bold", NULL, NULL, NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_BIG:
		newfont = _LoadFont(hw, NULL, NULL, NULL, NULL, "8", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_SMALL:
		newfont = _LoadFont(hw, NULL, NULL, NULL, NULL, "1", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_FONT:
		fndry = NULL;
		family = NULL;
		/* #### missing processing of FACE attribute #### */
		size = ParseMarkTag(mptr->start, MT_FONT, "SIZE");
		newfont = _LoadFont(hw, fndry, family, NULL, NULL, size, NULL, ofptr);
		if (size)
			free(size);
/* font have color */
		font_color = ParseMarkTag(mptr->start, MT_FONT, "color");
		fg = pcc->fg_text;
		if (font_color) {
			XColor c;
			int status;

			status = XParseColor(XtDisplay(hw),hw->core.colormap,
				font_color, &c);
			if (status) {
				fg = HTMLXColorToPixel(&c);
			}
			free(font_color);
		}
/* always push color */
		pcc->fg_text = MMPushColorFg(hw, fg);
		break;

/* Since HTML Headings may not occur inside a <font></font> declaration,
* they *must* use the specified document font, and not derive their
* true font from the current font.
*/
	case M_HEADER_1:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"7", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_HEADER_2:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"6", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_HEADER_3:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"5", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_HEADER_4:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"4", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_HEADER_5:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"3", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	case M_HEADER_6:
		newfont = _LoadFont(hw, "adobe", "times", "bold", "r",
			"2", NULL, ofptr);
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;

	/* should never be reached */
	default:
		/* this will always succeed */
		newfont = _LoadFont(hw, "*", "times", "*", "*", "3", NULL, ofptr); 
		pcc->fg_text = MMPushColorFg(hw, pcc->fg_text);
		break;
	}
	ofstack = hw->html.font_stack;
        hw->html.font_stack = (FontStack*) malloc(sizeof(FontStack));
	hw->html.font_stack->font = newfont;
	hw->html.font_stack->next = ofstack;

	hw->html.cur_font = pcc->cur_font = newfont->xfont;
}

void MMPopFont(HTMLWidget hw, struct mark_up *mptr, PhotoComposeContext * pcc)
{
	FontStack *ofstack;

	if (hw->html.font_stack->next == NULL) {
#ifdef HTMLTRACE
		fprintf(stderr, "Warning, pop empty font stack!\n");
#endif
		return;
	}
	ofstack = hw->html.font_stack;
	hw->html.font_stack = hw->html.font_stack->next;
	free(ofstack);
	hw->html.cur_font = hw->html.font_stack->font->xfont;
	pcc->cur_font = hw->html.cur_font;
	pcc->fg_text = MMPopColorFg(hw);
	return;
}

void GetDefaultWidgetFont( HTMLWidget hw, FontRec * ofr)
{
/* FIXE ME  MUST MATCH WbNdefaultFont ############ */
	ofr->xfont = hw->html.default_font;
	ofr->fndry = "adobe" ;
	ofr->family = "times";
	ofr->weight = "medium";
	ofr->slant = "r";
	ofr->pixelsize = "14";
	ofr->pointsize = "140";
	ofr->xres = "*";
	ofr->yres = "*";
	ofr->charset = "iso8859-1";
	ofr->cache_name = "-adobe-times-medium-r-normal-*-14-*-*-*-*-*-*-*";
}

/* Initialize the bottom stck for the widget , update the cache for ALL
 * widget. We need to push at least One default font at bottom
 */

void MMInitWidgetFont( HTMLWidget hw)
{
	FontRec *fr;
	static FontRec ofptr;

	GetDefaultWidgetFont(hw, &ofptr);

	hw->html.font_stack = (FontStack*) calloc(1,sizeof(FontStack));
	hw->html.font_stack->next = NULL;
	fr = _LoadFont(hw, NULL, NULL, NULL, NULL, "3", NULL, &ofptr);
	hw->html.font_stack->font = fr ;
}
/*###################################################################
/* Font face changes only allowed when not in preformatted text.*/
/*if(!in_pre)
/*chPtr = temp->attributes ? _XmHTMLTagGetValue(temp->attributes, "face") : NULL;
/*if(chPtr != NULL) {
/*if(in_pre) { 
/*Ignore face but must allow for size change. (Font stack will get unbalanced otherwise!)*/
/*font = _XmHTMLLoadFont(html, HT_FONT, size, font);
/*} else
/*font = _XmHTMLLoadFontWithFace(html, size, chPtr, font);
/*free(chPtr);
/*} else 
/*font = _XmHTMLLoadFont(html, HT_FONT, size, font);
/*ignore = True; /* only need font data */
/*break;
/* from XmHTML */                     
/*##################################
/*case HT_BASEFONT:            
/*basefont = temp->attributes ? _XmHTMLTagGetNumber(temp->attributes, "size", 0) : 0;                  
/* take absolute value */      
/*basefont = Abs(basefont);
/*if(basefont < 1 || basefont > 7) {
/*if(HTML_ATTR(bad_html_warnings))
/*basefont = 4;
/*}                    
/*ignore = True;  /* only need font data */
/*break;               
/*#################################################################


/*****
* Name: 		_XmHTMLLoadFontWithFace
* Return Type: 	XmHTMLfont*
* Description: 	load a new font with given pixelsize and face. 
*				Style is determined by the current font: if current font
*				is bold, and new is italic then a bold-italic font will be 
*				returned.
* In: 
*	w:			Widget for which to load a font
*	size:		size of font to load. Only used for HT_FONT.
*	face:		a comma separated list of font faces to use, contents are 
*				destroyed when this function returns.
*	curr_font:	current font, required for propagating font style info.
* Returns:
*	A new font with a face found in the list of faces given upon success
*	or the default font on failure.
*****/
/*XmHTMLfont* _XmHTMLLoadFontWithFace(XmHTMLWidget html, int size, String face,
/*	XmHTMLfont *curr_font)
/*{
/*	XmHTMLfont *new_font = NULL;
/*	String chPtr, family, all_faces, first_face = NULL;
/*	Byte new_style = (Byte)0, font_style;
/*	int try;
/*
/*	/* pick up style of the current font */
/*	font_style = curr_font->style;
/*
/*	/* See if we need to proceed with bold font */
/*	if(font_style & FONT_BOLD)
/*		new_style = FONT_BOLD;
/*	else
/*		new_style &= ~FONT_BOLD;
/*
/*	/* See if we need to proceed with italic font */
/*	if(font_style & FONT_ITALIC)
/*		new_style |= FONT_ITALIC;
/*	else
/*		new_style &= ~FONT_ITALIC;
/*
/*	/***** 
/*	* See if we need to proceed with a fixed font, only used to determine
/*	* initial font family.
/*	*****/
/*	if(font_style & FONT_FIXED) {
/*		new_style |= FONT_FIXED;
/*		family = html->html.font_family_fixed;
/*	} else {
/*		new_style &= ~FONT_FIXED;
/*		family = html->html.font_family;
/*	}
/*
/*	/* we must have a ``,'' or strtok will fail */
/*	if((strstr(face, ",")) == NULL) {
/*		all_faces = (String)malloc(strlen(face) + 2);
/*		strcpy(all_faces, face);
/*		strcat(all_faces, ",\0");
/*	} else
/*		all_faces = strdup(face);
/*
/*	/* walk all possible spaces */
/*	try = 0;
/*	for(chPtr = strtok(all_faces, ","); chPtr != NULL;
/*		chPtr = strtok(NULL, ",")) {
/*		Boolean ok = False;
/*
/*		try++;
/*
/*		/* skip any leading spaces */
/*		while(isspace(*chPtr))
/*			chPtr++;
/*
/*		/***** 
/*		* Disable font not found warning message, we are trying to find
/*		* a font of which we don't know if it exists.
/*		*****/
/*		ok = False;
/*		new_font = loadQueryFont(html, family, chPtr, size,
/*			new_style, &ok);
/*		if(new_font && ok) {
/*######## add all font (with face) to the cache, if we found a match in the list##
/*### add only the first list until match ###
/*			break;
/*		}
/*		if(try == 1)
/*			first_face = strdup(chPtr);
/*	}
/*	free(all_faces);
/*	/*****
/*	* hmm, the first font in this face specification didn't yield a valid
/*	* font. To speed up things considerably, we add a font mapping for the
/*	* first face in the list of given spaces. There's no sense in doing this
/*	* when there is only one face specified as this will always get us the
/*	* default font. We only add a mapping if the name of the returned font
/*	* contains at least one of the allowed faces. Not doing this check would
/*	* ignore face specs which do have a face we know. We also want the font
/*	* styles to match as well.
/*	* BTW: this is a tremendous speedup!!!
/*	*****/
/*	if(first_face) {
/*****
* Only add a mapping if the returned name contains one of the allowed
* faces. No need to check for the presence of a comma: we only take
* lists that have multiple face specifications.
*****/
/*		if(try > 1) {
/*****
/** Walk all possible faces. Nukes the face array but that's not
/** bad as we are the only ones using it.
/******/
/*			for(chPtr = strtok(face, ","); chPtr != NULL;
/*				chPtr = strtok(NULL, ",")) {
/*				/* skip any leading spaces */
/*				while(isspace(*chPtr))
/*					chPtr++;
/*				/* caseless 'cause fontnames ignore case */
/*				if(my_strcasestr(new_font->font_name, chPtr) &&
/*					new_font->style == new_style) {
/*					_XmHTMLaddFontMapping(html, family, first_face, size,
/*						new_style, new_font);
/*					break;
/*				}
/*			}
/*		}
/*		free(first_face);
/*	}
/*	return(new_font);
/*}

/*
* Description: 	allocates a new font entry and retrieves all required
*			font properties;
* In: 
*	xfont:		ptr to an X font;
*	name:		name of this font;
*	family:		family to which this font belongs;
* Returns:
*	a new font entry;
*****/
/*static FontRec* allocFont(XFontStruct *xfont, String name)
/*{
/*	static FontRec *font;
/*	unsigned long value = 0;
/*
/*	font = (FontRec*)malloc(sizeof(FontRec));
/*
/*	/* default items */
/*	font->xfont = xfont;
/*	font->font_name = strdup(name);
/*	font->style = style;
/*
/*	/* normal interword spacing */
/*	if((XGetFontProperty(xfont, XA_NORM_SPACE, &value)) == True)
/*		font->isp = (Cardinal)value;
/*	else {
/*		/* use width of a single space */
/*		int dir, ascent, descent;
/*		XCharStruct sp;
/*		XTextExtents(font->xfont, " ", 1, &dir, &ascent, &descent, &sp);
/*		font->isp = sp.width;
/*	}
/*
/*	/* additional end-of-line spacing */
/*	if((XGetFontProperty(xfont, XA_END_SPACE, &value)) == True)
/*		font->eol_sp = (Cardinal)value;
/*	else
/*		font->eol_sp = 0;
/*
/*	/* superscript x-offset */
/*	if((XGetFontProperty(xfont, XA_SUPERSCRIPT_X, &value)) == True)
/*		font->sup_xoffset = (int)value;
/*	else
/*		font->sup_xoffset = 0;
/*
/*	/* superscript y-offset */
/*	if((XGetFontProperty(xfont, XA_SUPERSCRIPT_Y, &value)) == True)
/*		font->sup_yoffset = (int)value;
/*	else
/*		font->sup_yoffset = (int)(font->m_ascent  * -.4);
/*
/*	/* subscript x-offset */
/*	if((XGetFontProperty(xfont, XA_SUBSCRIPT_X, &value)) == True)
/*		font->sub_xoffset = (int)value;
/*	else
/*		font->sub_xoffset = 0;
/*
/*	/* subscript y-offset */
/*	if((XGetFontProperty(xfont, XA_SUBSCRIPT_Y, &value)) == True)
/*		font->sub_yoffset = (int)value;
/*	else
/*		font->sub_yoffset = (int)(font->m_descent * .8);
/*
/*	/* underline offset */
/*	if((XGetFontProperty(xfont, XA_UNDERLINE_POSITION, &value)) == True)
/*		font->ul_offset = (int)value;
/*	else
/*		font->ul_offset = (int)(font->m_descent-2);
/*
/*	/* underline thickness */
/*	if((XGetFontProperty(xfont, XA_UNDERLINE_THICKNESS, &value)) == True)
/*		font->ul_thickness = (Cardinal)value;
/*	else
/*		font->ul_thickness = (Cardinal)1;
/*
/*	/* strikeout offset */
/*	if((XGetFontProperty(xfont, XA_STRIKEOUT_ASCENT, &value)) == True) {
/*		/*****
/*		* strikeout_ascent gives the upper limit for a *bounding box*,
/*		* while strikeout_descent gives the lower limit. We simply
/*		* add them up and then divide by 2 to get the baseline offset
/*		* for strikeouts.
/*		*****/
/*		font->st_offset = (int)value;
/*		if((XGetFontProperty(xfont, XA_STRIKEOUT_DESCENT, &value)) == True) {
/*			font->st_offset += (int)value;
/*			font->st_offset *= 0.5;
/*		} else
/*			font->st_offset = (int)(0.5*(font->height));
/*	} else
/*		font->st_offset = (int)(0.5*(font->height));
/*
/*	/* strikeout thickness. No font property for this one */
/*	font->st_thickness = font->ul_thickness;
/*
/*	return(font);
/*} 
/*
/** Description: 	fills all arrays of font sizes.
/** In: 
/**	w:			widget containing font size specs.
/** Returns:
/**	nothing, but the font lists are updated to reflect the new sizes.
/**
/** The static size lists can cause unexpected results when multiple instances
/** of the Widget with different sizelists are being used.
/******/
/*static void initializeFontSizeLists(XmHTMLWidget html)
/*{
/*	char *chPtr;
/*	char size_list[64];
/*	int i;
/*	Boolean ok;
/*
/*	/*** Scalable font size list ***/
/*
/*	/* copy name, it gets destroyed */
/*	(void)memset(&size_list, 0, 64);
/*	strncpy(size_list, html->html.font_sizes, 63);	
/*	/* This list has 8 elements */
/*	for(chPtr = strtok(size_list, ","), i = 0; i < 8 && chPtr != NULL; 
/*		chPtr = strtok(NULL, ","), i++) {
/*		if((xmhtml_fn_sizes[i] = 10*atoi(chPtr)) == 0)
/*			xmhtml_fn_sizes[i] = def_fn_sizes[i];
/*	}
/*	/* fill up list if it is not complete */
/*	if(i != 8) {
/*		for(; i < 8; i++)
/*			xmhtml_fn_sizes[i] = def_fn_sizes[i];
/*	}
/*
/*	/*** Fixed font size list ***/
/*	/* copy name, it gets destroyed */
/*	(void)memset(&size_list, 0, 64);
/*	strncpy(size_list, html->html.font_sizes_fixed, 63);
/*
/*	/* list of possible font de/increments using the <FONT SIZE=""> element */
/*	xmhtml_basefont_sizes[0] = xmhtml_fn_sizes[1];	/* sub/superscript size */
/*	xmhtml_basefont_sizes[1] = xmhtml_fn_sizes[7];	/* H6 size */
/*	xmhtml_basefont_sizes[2] = xmhtml_fn_sizes[6];	/* H5 size */
/*	xmhtml_basefont_sizes[3] = xmhtml_fn_sizes[5];	/* H4 size */
/*	xmhtml_basefont_sizes[4] = xmhtml_fn_sizes[4];	/* H3 size (def font size)*/
/*	xmhtml_basefont_sizes[5] = xmhtml_fn_sizes[3];	/* H2 size */
/*	xmhtml_basefont_sizes[6] = xmhtml_fn_sizes[2];	/* H1 size */
/*
/*	/* First try to load the default font as specified by the resources */
/*	ok = False;
/*	html->html.default_font = loadQueryFont(html,
/*		html->html.font_family, NULL, xmhtml_fn_sizes[0],
/*		FONT_SCALABLE|FONT_REGULAR|FONT_MEDIUM, &ok);
/*
/*	/***** 
/*	* We can't load the default font, try again with a wildcarded family.
/*	* This time die if it fails
/*	*****/
/*	if(html->html.default_font == NULL) {
/*		ok = True;
/*		html->html.default_font = loadQueryFont(html,
/*			html->html.font_family, "*", xmhtml_fn_sizes[0],
/*			FONT_SCALABLE|FONT_REGULAR|FONT_MEDIUM, &ok);
/*
/*		/* too bad, we absolutely need it */
/*		if(ok == False) {
/*			/* die */
/*			_XmHTMLError(__WFUNC__(html,"initializeFontSizeLists"),
/*				"Failed to find a default font for %s\n    Check previous "
/*				"messages and adjust default font", html->html.font_family);
/*		}
/*	}
/*}
/*
/*
/** Description: 	selects a cache according to the display a widget is
/**		being displayed on (or creates one if it isn't present yet)
/** In: 
/**	html:		XmHTMLWidget id;
/**	reset:		hard reset flag (used by SetValues when any of the font
/**				resources changes);
/** Returns:
/**	the id of the default font for this cache;
/******/
/*XmHTMLfont* _XmHTMLSelectFontCache(XmHTMLWidget html, Boolean reset)
/*{
/*	fontCache *cache;
/*
/*	if(cache == NULL) {
/*		int screen = DefaultScreen(tka->dpy);
/*		cache = (fontCache*)malloc(sizeof(fontCache));
/*		cache->dpy = tka->dpy;
/*		cache->cache = (fontCacheEntry*)NULL;
/*		cache->default_font = (XmHTMLfont*)NULL;
/*		cache->next = (fontCache*)NULL;
/*		/* obtain screen resolution in dpi */
/*		cache->res_x =
/*			DisplayWidth(tka->dpy, screen)/(DisplayWidthMM(tka->dpy,screen)/25.4);
/*		cache->res_y =
/*			DisplayHeight(tka->dpy, screen)/(DisplayHeightMM(tka->dpy,screen)/25.4);
/*		/* adjust resolutions */
/*		cache->res_x = (cache->res_x < 87 ? 75 : 100);
/*		cache->res_y = (cache->res_y < 87 ? 75 : 100);
/*		/* make sure we have the same resolution in both directions */
/*		if(cache->res_x != cache->res_y) {
/*			if(cache->res_x > cache->res_y)
/*				cache->res_y = cache->res_x;
/*			else
/*				cache->res_x = cache->res_y;
/*		}
/*	} else {
/*		/* see if we have got a reference for this widget */
/*		for(i = 0; i < cache->nwidgets && cache->widgets[i] != (Widget)html;
/*			i++);
/*		if(i == cache->nwidgets) { 
/*			cache->widgets = (WidgetList)realloc(cache->widgets,
/*				(cache->nwidgets+1)*sizeof(Widget));
/*			cache->widgets[cache->nwidgets++] = (Widget)html;
/*		}
/*	}
/*	/*****
/*	* Only initialize font lists if the cache has changed, when we
/*	* are forced to do a reset or we haven't got a default font.
/*	*****/
/*	if(curr_cache != cache || reset || html->html.default_font == NULL) {
/*		curr_cache = cache;
/*		initializeFontSizeLists(html);
/*	}
/*	curr_cache->default_font = html->html.default_font;
/*	return(curr_cache->default_font);
/*}
*/

