/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"
#include "../Copyrights/copyright.bull"

#include <sys/time.h>
struct timeval Tv;
struct timezone Tz;

#include "../libhtmlw/HTML.h"
#include "mosaic.h"

#include <X11/Xos.h>
#include "xpm.h"
#include "xpmread.h"

/*for memset*/
#include <memory.h>

char *_MMxpmColorKeys[] = {
    "s",				/* key #1: symbol */
    "m",				/* key #2: mono visual */
    "g4",				/* key #3: 4 grays visual */
    "g",				/* key #4: gray visual */
    "c"					/* key #5: color visual */
};

xpmDataType _MMxpmDataTypes[] = {
    {"", "!", "\n", '\0', '\n', "", "", "", ""},	/* Natural type */
    {"C", "/*", "*/", '"', '"', ",\n", "static char *", "[] = {\n", "};\n"},
    {"Lisp", ";", "\n", '"', '"', "\n", "(setq ", " '(\n", "))\n"},
    {NULL, NULL, NULL, 0, 0, NULL, NULL, NULL, NULL}
};

/* Free the computed color table */
static void _MMxpmFreeColorTable_( char ***colorTable, int ncolors)
{
    int a, b;
    char ***ct, **cts;

    if (colorTable) {
	for (a = 0, ct = colorTable; a < ncolors; a++, ct++)
	    if (*ct) {
		for (b = 0, cts = *ct; b <= NKEYS; b++, cts++)
		    if (*cts)
			_MMXpmFree(*cts);
		_MMXpmFree(*ct);
	    }
	_MMXpmFree(colorTable);
    }
}


/*
 * Intialize the xpmInternAttrib pointers to Null to know
 * which ones must be freed later on.
 */
void _MMxpmInitInternAttrib( xpmInternAttrib *attrib)
{
    attrib->ncolors = 0;
    attrib->colorTable = NULL;
    attrib->pixelindex = NULL;
    attrib->xcolors = NULL;
    attrib->colorStrings = NULL;
    attrib->mask_pixel = UNDEF_PIXEL;
}


/*
 * Free the xpmInternAttrib pointers which have been allocated
 */
void _MMxpmFreeInternAttrib( xpmInternAttrib *attrib)
{
    unsigned int a, ncolors;
    char **sptr;

    if (attrib->colorTable)
	_MMxpmFreeColorTable_(attrib->colorTable, attrib->ncolors);
    if (attrib->pixelindex)
	_MMXpmFree(attrib->pixelindex);
    if (attrib->xcolors)
	_MMXpmFree(attrib->xcolors);
    if (attrib->colorStrings) {
	ncolors = attrib->ncolors;
	for (a = 0, sptr = attrib->colorStrings; a < ncolors; a++, sptr++)
	    if (*sptr)
		_MMXpmFree(*sptr);
	_MMXpmFree(attrib->colorStrings);
    }
}

/*
 * open the given file to be read as an xpmData which is returned.
 */
int _MMxpmReadFile( char *filename, xpmData *mdata)
{
#ifdef ZPIPE
    char *compressfile, buf[BUFSIZ];
    struct stat status;
#endif

    if (!filename) {
	mdata->stream.file = (stdin);
	mdata->type = XPMFILE;
    } else {
#ifdef ZPIPE
	if ((strlen(filename) > 2) &&
	    !strcmp(".Z", filename + (strlen(filename) - 2))) {
	    mdata->type = XPMPIPE;
	    sprintf(buf, "uncompress -c %s", filename);
	    if (!(mdata->stream.file = popen(buf, "r")))
		return (XpmOpenFailed);

	} else if ((strlen(filename) > 2) &&
		   !strcmp(".z", filename + (strlen(filename) - 2))) {
	    mdata->type = XPMPIPE;
	    sprintf(buf, "gunzip -c %s", filename);
	    if (!(mdata->stream.file = popen(buf, "r")))
		return (XpmOpenFailed);

	} else {
	    if (!(compressfile = (char *) XpmMalloc(strlen(filename) + 3)))
		return (XpmNoMemory);

	    strcpy(compressfile, filename);
	    strcat(compressfile, ".Z");
	    if (!stat(compressfile, &status)) {
		sprintf(buf, "uncompress -c %s", compressfile);
		if (!(mdata->stream.file = popen(buf, "r"))) {
		    _MMXpmFree(compressfile);
		    return (XpmOpenFailed);
		}
		mdata->type = XPMPIPE;
	    } else {
		strcpy(compressfile, filename);
		strcat(compressfile, ".z");
		if (!stat(compressfile, &status)) {
		    sprintf(buf, "gunzip -c %s", compressfile);
		    if (!(mdata->stream.file = popen(buf, "r"))) {
			_MMXpmFree(compressfile);
			return (XpmOpenFailed);
		    }
		    mdata->type = XPMPIPE;
		} else {
#endif
		    if (!(mdata->stream.file = fopen(filename, "r"))) {
#ifdef ZPIPE
			_MMXpmFree(compressfile);
#endif
			return (XpmOpenFailed);
		    }
		    mdata->type = XPMFILE;
#ifdef ZPIPE
		}
	    }
	    _MMXpmFree(compressfile);
	}
#endif
    }
    mdata->CommentLength = 0;
    return (XpmSuccess);
}

/* close the file related to the xpmData if any */
int _MMxpmDataClose( xpmData *mdata)
{
    switch (mdata->type) {
    case XPMARRAY:
    case XPMBUFFER:
	break;
    case XPMFILE:
	if (mdata->stream.file != (stdout) && mdata->stream.file != (stdin))
	    fclose(mdata->stream.file);
	break;
#ifdef ZPIPE
    case XPMPIPE:
	pclose(mdata->stream.file);
	break;
#endif
    }
    return 0;
}


static int ParseComment( xpmData *mdata)
{
    if (mdata->type == XPMBUFFER) {
	register char c;
	register unsigned int n = 0;
	unsigned int notend;
	char *s, *s2;

	s = mdata->Comment;
	*s = mdata->Bcmt[0];

	/* skip the string beginning comment */
	s2 = mdata->Bcmt;
	do {
	    c = *mdata->cptr++;
	    *++s = c;
	    n++;
	    s2++;
	} while (c == *s2 && *s2 != '\0' && c && c != mdata->Bos);

	if (*s2 != '\0') {
	    /* this wasn't the beginning of a comment */
	    mdata->cptr -= n;
	    return 0;
	}
	/* store comment */
	mdata->Comment[0] = *s;
	s = mdata->Comment;
	notend = 1;
	n = 0;
	while (notend) {
	    s2 = mdata->Ecmt;
	    while (*s != *s2 && c && c != mdata->Bos) {
		c = *mdata->cptr++;
		*++s = c;
		n++;
	    }
	    mdata->CommentLength = n;
	    do {
		c = *mdata->cptr++;
		n++;
		*++s = c;
		s2++;
	    } while (c == *s2 && *s2 != '\0' && c && c != mdata->Bos);
	    if (*s2 == '\0') {
		/* this is the end of the comment */
		notend = 0;
		mdata->cptr--;
	    }
	}
	return 0;
    } else {
	FILE *file = mdata->stream.file;
	register int c;
	register unsigned int n = 0, a;
	unsigned int notend;
	char *s, *s2;

	s = mdata->Comment;
	*s = mdata->Bcmt[0];

	/* skip the string beginning comment */
	s2 = mdata->Bcmt;
	do {
	    c = getc(file);
	    *++s = c;
	    n++;
	    s2++;
	} while (c == *s2 && *s2 != '\0'
		 && c != EOF && c != mdata->Bos);

	if (*s2 != '\0') {
	    /* this wasn't the beginning of a comment */
	    /* put characters back in the order that we got them */
	    for (a = n; a > 0; a--, s--)
		ungetc(*s, file);
	    return 0;
	}
	/* store comment */
	mdata->Comment[0] = *s;
	s = mdata->Comment;
	notend = 1;
	n = 0;
	while (notend) {
	    s2 = mdata->Ecmt;
	    while (*s != *s2 && c != EOF && c != mdata->Bos) {
		c = getc(file);
		*++s = c;
		n++;
	    }
	    mdata->CommentLength = n;
	    do {
		c = getc(file);
		n++;
		*++s = c;
		s2++;
	    } while (c == *s2 && *s2 != '\0'
		     && c != EOF && c != mdata->Bos);
	    if (*s2 == '\0') {
		/* this is the end of the comment */
		notend = 0;
		ungetc(*s, file);
	    }
	}
	return 0;
    }
}


/*
 * skip to the end of the current string and the beginning of the next one
 */
int _MMxpmNextString( xpmData *mdata)
{
    if (!mdata->type)
	mdata->cptr = (mdata->stream.data)[++mdata->line];
    else if (mdata->type == XPMBUFFER) {
	register char c;

	/* get to the end of the current string */
	if (mdata->Eos)
	    while ((c = *mdata->cptr++) && c != mdata->Eos);

	/*
	 * then get to the beginning of the next string looking for possible
	 * comment
	 */
	if (mdata->Bos) {
	    while ((c = *mdata->cptr++) && c != mdata->Bos)
		if (mdata->Bcmt && c == mdata->Bcmt[0])
		    ParseComment(mdata);
	} else if (mdata->Bcmt) {	/* XPM2 natural */
	    while ((c = *mdata->cptr++) == mdata->Bcmt[0])
		ParseComment(mdata);
	    mdata->cptr--;
	}
    } else {
	register int c;
	FILE *file = mdata->stream.file;

	/* get to the end of the current string */
	if (mdata->Eos)
	    while ((c = getc(file)) != mdata->Eos && c != EOF);

	/*
	 * then get to the beginning of the next string looking for possible
	 * comment
	 */
	if (mdata->Bos) {
	    while ((c = getc(file)) != mdata->Bos && c != EOF)
		if (mdata->Bcmt && c == mdata->Bcmt[0])
		    ParseComment(mdata);

	} else if (mdata->Bcmt) {	/* XPM2 natural */
	    while ((c = getc(file)) == mdata->Bcmt[0])
		ParseComment(mdata);
	    ungetc(c, file);
	}
    }
    return 0;
}

static unsigned int atoui( register char *p, unsigned int l, unsigned int *ui_return)
{
    register int n, i;

    n = 0;
    for (i = 0; i < l; i++)
	if (*p >= '0' && *p <= '9')
	    n = n * 10 + *p++ - '0';
	else
	    break;

    if (i != 0 && i == l) {
	*ui_return = n;
	return 1;
    } else
	return 0;
}

static int ParseValues( xpmData *data, unsigned int *width, unsigned int *height,
	unsigned int *ncolors, unsigned int *cpp, 
	unsigned int *x_hotspot, unsigned int *y_hotspot, unsigned int *hotspot,
    	unsigned int *extensions)
{
    unsigned int l;
    char buf[BUFSIZ];

    /*
     * read values: width, height, ncolors, chars_per_pixel
     */
    if (!(_MMxpmNextUI(data, width) && _MMxpmNextUI(data, height)
	  && _MMxpmNextUI(data, ncolors) && _MMxpmNextUI(data, cpp)))
	return (XpmFileInvalid);

    /*
     * read optional information (hotspot and/or XPMEXT) if any
     */
    l = _MMxpmNextWord(data, buf);
    if (l) {
	*extensions = l == 6 && !strncmp("XPMEXT", buf, 6);
	if (*extensions)
	    *hotspot = _MMxpmNextUI(data, x_hotspot)
		&& _MMxpmNextUI(data, y_hotspot);
	else {
	    *hotspot = atoui(buf, l, x_hotspot) && _MMxpmNextUI(data, y_hotspot);
	    l = _MMxpmNextWord(data, buf);
	    *extensions = l == 6 && !strncmp("XPMEXT", buf, 6);
	}
    }
    return (XpmSuccess);
}

static int ParseColors( xpmData *data, unsigned int ncolors,
    unsigned int cpp,
    char ****colorTablePtr,		/* Jee, that's something! */
    xpmHashTable *hashtable)
{
    unsigned int key, l, a, b;
    unsigned int curkey;		/* current color key */
    unsigned int lastwaskey;		/* key read */
    char buf[BUFSIZ];
    char curbuf[BUFSIZ];		/* current buffer */
    char ***ct, **cts, **sptr, *s;
    char ***colorTable;
    int ErrorStatus;

    colorTable = (char ***) XpmCalloc(ncolors, sizeof(char **));
    if (!colorTable)
	return (XpmNoMemory);

    for (a = 0, ct = colorTable; a < ncolors; a++, ct++) {
	_MMxpmNextString(data);		/* skip the line */
	cts = *ct = (char **) XpmCalloc((NKEYS + 1), sizeof(char *));
	if (!cts) {
	    _MMxpmFreeColorTable_(colorTable, ncolors);
	    return (XpmNoMemory);
	}

	/*
	 * read pixel value
	 */
	*cts = (char *) XpmMalloc(cpp + 1);	/* + 1 for null terminated */
	if (!*cts) {
	    _MMxpmFreeColorTable_(colorTable, ncolors);
	    return (XpmNoMemory);
	}
	for (b = 0, s = *cts; b < cpp; b++, s++)
	    *s = _MMxpmGetC(data);
	*s = '\0';

	/*
	 * store the string in the hashtable with its color index number
	 */
	if (USE_HASHTABLE) {
	    ErrorStatus = _MMxpmHashIntern(hashtable, *cts, HashAtomData(a));
	    if (ErrorStatus != XpmSuccess) {
		_MMxpmFreeColorTable_(colorTable, ncolors);
		return (ErrorStatus);
	    }
	}

	/*
	 * read color keys and values
	 */
	curkey = 0;
	lastwaskey = 0;
	while (l = _MMxpmNextWord(data, buf)) {
	    if (!lastwaskey) {
		for (key = 0, sptr = _MMxpmColorKeys; key < NKEYS; key++, sptr++)
		    if ((strlen(*sptr) == l) && (!strncmp(*sptr, buf, l)))
			break;
	    }
	    if (!lastwaskey && key < NKEYS) {	/* open new key */
		if (curkey) {		/* flush string */
		    s = cts[curkey] = (char *) XpmMalloc(strlen(curbuf) + 1);
		    if (!s) {
			_MMxpmFreeColorTable_(colorTable, ncolors);
			return (XpmNoMemory);
		    }
		    strcpy(s, curbuf);
		}
		curkey = key + 1;	/* set new key  */
		*curbuf = '\0';		/* reset curbuf */
		lastwaskey = 1;
	    } else {
		if (!curkey) {		/* key without value */
		    _MMxpmFreeColorTable_(colorTable, ncolors);
		    return (XpmFileInvalid);
		}
		if (!lastwaskey)
		    strcat(curbuf, " ");/* append space */
		buf[l] = '\0';
		strcat(curbuf, buf);	/* append buf */
		lastwaskey = 0;
	    }
	}
	if (!curkey) {			/* key without value */
	    _MMxpmFreeColorTable_(colorTable, ncolors);
	    return (XpmFileInvalid);
	}
	s = cts[curkey] = (char *) XpmMalloc(strlen(curbuf) + 1);
	if (!s) {
	    _MMxpmFreeColorTable_(colorTable, ncolors);
	    return (XpmNoMemory);
	}
	strcpy(s, curbuf);
    }
    *colorTablePtr = colorTable;
    return (XpmSuccess);
}

static int ParsePixels( xpmData *data, unsigned int width, unsigned int height,
    unsigned int ncolors, unsigned int cpp,
    char ***colorTable, xpmHashTable *hashtable,
    unsigned int **pixels)
{
    unsigned int *iptr, *iptr2;
    unsigned int a, x, y;

    iptr2 = (unsigned int *) XpmMalloc(sizeof(unsigned int) * width * height);
    if (!iptr2)
	return (XpmNoMemory);

    iptr = iptr2;
    switch (cpp) {

    case (1):			/* Optimize for single character colors */
	{
	    unsigned short colidx[256];

	    memset(colidx, 0, 256 * sizeof(short));
	    for (a = 0; a < ncolors; a++)
		colidx[colorTable[a][0][0]] = a + 1;

	    for (y = 0; y < height; y++) {
		_MMxpmNextString(data);
		for (x = 0; x < width; x++, iptr++) {
		    int idx = colidx[_MMxpmGetC(data)];

		    if (idx != 0)
			*iptr = idx - 1;
		    else {
			_MMXpmFree(iptr2);
			return (XpmFileInvalid);
		    }
		}
	    }
	}
	break;

    case (2):			/* Optimize for double character colors */
	{
	    unsigned short cidx[256][256];

	    memset(cidx, 0, 256 * 256 * sizeof(short));
	    for (a = 0; a < ncolors; a++)
		cidx[colorTable[a][0][0]][colorTable[a][0][1]] = a + 1;

	    for (y = 0; y < height; y++) {
		_MMxpmNextString(data);
		for (x = 0; x < width; x++, iptr++) {
		    int cc1 = _MMxpmGetC(data);
		    int idx = cidx[cc1][_MMxpmGetC(data)];

		    if (idx != 0)
			*iptr = idx - 1;
		    else {
			_MMXpmFree(iptr2);
			return (XpmFileInvalid);
		    }
		}
	    }
	}
	break;

    default:			/* Non-optimized case of long color names */
	{
	    char *s;
	    char buf[BUFSIZ];

	    buf[cpp] = '\0';
	    if (USE_HASHTABLE) {
		xpmHashAtom *slot;

		for (y = 0; y < height; y++) {
		    _MMxpmNextString(data);
		    for (x = 0; x < width; x++, iptr++) {
			for (a = 0, s = buf; a < cpp; a++, s++)
			    *s = _MMxpmGetC(data);
			slot = _MMxpmHashSlot(hashtable, buf);
			if (!*slot) {	/* no color matches */
			    _MMXpmFree(iptr2);
			    return (XpmFileInvalid);
			}
			*iptr = HashColorIndex(slot);
		    }
		}
	    } else {
		for (y = 0; y < height; y++) {
		    _MMxpmNextString(data);
		    for (x = 0; x < width; x++, iptr++) {
			for (a = 0, s = buf; a < cpp; a++, s++)
			    *s = _MMxpmGetC(data);
			for (a = 0; a < ncolors; a++)
			    if (!strcmp(colorTable[a][0], buf))
				break;
			if (a == ncolors) {	/* no color matches */
			    _MMXpmFree(iptr2);
			    return (XpmFileInvalid);
			}
			*iptr = a;
		    }
		}
	    }
	}
	break;
    }
    *pixels = iptr2;
    return (XpmSuccess);
}


/*
 * skip whitespace and return the following word
 */
unsigned int _MMxpmNextWord( xpmData *mdata, char *buf)
{
    register unsigned int n = 0;
    int c;

    if (!mdata)
      return 0;

    if (!mdata->type || mdata->type == XPMBUFFER) {
	while (isspace(c = *mdata->cptr) && c != mdata->Eos)
	    mdata->cptr++;
	do {
	    c = *mdata->cptr++;
	    *buf++ = c;
	    n++;
	} while (!isspace(c) && c != mdata->Eos && (n < BUFSIZ));
	n--;
	mdata->cptr--;
    } else {
	FILE *file = mdata->stream.file;

	while ((c = getc(file)) != EOF && isspace(c) && c != mdata->Eos);
	while (!isspace(c) && c != mdata->Eos && c != EOF && (n < BUFSIZ)) {
	    *buf++ = c;
	    n++;
	    c = getc(file);
	}
	ungetc(c, file);
    }
    return (n);
}


/*
 * skip whitespace and compute the following unsigned int,
 * returns 1 if one is found and 0 if not
 */
int _MMxpmNextUI( xpmData *mdata, unsigned int *ui_return)
{
    char buf[BUFSIZ];
    int l;

    l = _MMxpmNextWord(mdata, buf);
    return atoui(buf, l, ui_return);
}


/*
 * parse xpm header
 */
int _MMxpmParseHeader( xpmData *mdata)
{
    char buf[BUFSIZ];
    int l, n = 0;

    if (mdata->type) {
	mdata->Bos = '\0';
	mdata->Eos = '\n';
	mdata->Bcmt = mdata->Ecmt = NULL;
	_MMxpmNextWord(mdata, buf);	/* skip the first word */
	l = _MMxpmNextWord(mdata, buf);	/* then get the second word */
	if ((l == 3 && !strncmp("XPM", buf, 3)) ||
	    (l == 4 && !strncmp("XPM2", buf, 4))) {
	    if (l == 3)
		n = 1;			/* handle XPM as XPM2 C */
	    else {
		l = _MMxpmNextWord(mdata, buf);	/* get the type key word */

		/*
		 * get infos about this type
		 */
		while (_MMxpmDataTypes[n].type
		       && strncmp(_MMxpmDataTypes[n].type, buf, l))
		    n++;
	    }
	    if (_MMxpmDataTypes[n].type) {
		if (n == 0) {		/* natural type */
		    mdata->Bcmt = _MMxpmDataTypes[n].Bcmt;
		    mdata->Ecmt = _MMxpmDataTypes[n].Ecmt;
		    _MMxpmNextString(mdata);	/* skip the end of
						 * headerline */
		    mdata->Bos = _MMxpmDataTypes[n].Bos;
		} else {
		    _MMxpmNextString(mdata);	/* skip the end of
						 * headerline */
		    mdata->Bcmt = _MMxpmDataTypes[n].Bcmt;
		    mdata->Ecmt = _MMxpmDataTypes[n].Ecmt;
		    mdata->Bos = _MMxpmDataTypes[n].Bos;
		    mdata->Eos = '\0';
		    _MMxpmNextString(mdata);	/* skip the assignment line */
		}
		mdata->Eos = _MMxpmDataTypes[n].Eos;
	    } else
		return XpmFileInvalid;
	} else
	    return XpmFileInvalid;
    }
    return XpmSuccess;
}


/*
 * get the current comment line
 */
int _MMxpmGetCmt( xpmData *mdata, char **cmt)
{
    if (!mdata->type)
	*cmt = NULL;
    else if (mdata->CommentLength) {
	*cmt = (char *) XpmMalloc(mdata->CommentLength + 1);
	strncpy(*cmt, mdata->Comment, mdata->CommentLength);
	(*cmt)[mdata->CommentLength] = '\0';
	mdata->CommentLength = 0;
    } else
	*cmt = NULL;
    return 0;
}


#undef RETURN
#define RETURN(status) \
  { if (colorTable) _MMxpmFreeColorTable_(colorTable, ncolors); \
    if (pixelindex) _MMXpmFree(pixelindex); \
    if (hints_cmt)  _MMXpmFree(hints_cmt); \
    if (colors_cmt) _MMXpmFree(colors_cmt); \
    if (pixels_cmt) _MMXpmFree(pixels_cmt); \
    return(status); }

/*
 * This function parses an Xpm file or data and store the found informations
 * in an an xpmInternAttrib structure which is returned.
 */
int _MMxpmParseData( xpmData *data, xpmInternAttrib *attrib_return,
    XpmAttributes *attributes)
{
    /* variables to return */
    unsigned int width, height, ncolors, cpp;
    unsigned int x_hotspot, y_hotspot, hotspot = 0, extensions = 0;
    char ***colorTable = NULL;
    unsigned int *pixelindex = NULL;
    char *hints_cmt = NULL;
    char *colors_cmt = NULL;
    char *pixels_cmt = NULL;

    int ErrorStatus;
    xpmHashTable hashtable;

    /*
     * parse the header
     */
    ErrorStatus = _MMxpmParseHeader(data);
    if (ErrorStatus != XpmSuccess)
	return (ErrorStatus);

    /*
     * read values
     */
    ErrorStatus = ParseValues(data, &width, &height, &ncolors, &cpp,
			    &x_hotspot, &y_hotspot, &hotspot, &extensions);
    if (ErrorStatus != XpmSuccess)
	return (ErrorStatus);

    /*
     * store the hints comment line
     */
    if (attributes && (attributes->valuemask & XpmReturnInfos))
	_MMxpmGetCmt(data, &hints_cmt);

    /*
     * init the hastable
     */
    if (USE_HASHTABLE) {
	ErrorStatus = _MMxpmHashTableInit(&hashtable);
	if (ErrorStatus != XpmSuccess)
	    return (ErrorStatus);
    }

    /*
     * read colors
     */
    ErrorStatus = ParseColors(data, ncolors, cpp, &colorTable, &hashtable);
    if (ErrorStatus != XpmSuccess)
	RETURN(ErrorStatus);

    /*
     * store the colors comment line
     */
    if (attributes && (attributes->valuemask & XpmReturnInfos))
	_MMxpmGetCmt(data, &colors_cmt);

    /*
     * read pixels and index them on color number
     */
    ErrorStatus = ParsePixels(data, width, height, ncolors, cpp, colorTable,
			      &hashtable, &pixelindex);

    /*
     * free the hastable
     */
    if (USE_HASHTABLE)
	_MMxpmHashTableFree(&hashtable);

    if (ErrorStatus != XpmSuccess)
	RETURN(ErrorStatus);

    /*
     * store the pixels comment line
     */
    if (attributes && (attributes->valuemask & XpmReturnInfos))
	_MMxpmGetCmt(data, &pixels_cmt);

    /*
     * store found informations in the xpmInternAttrib structure
     */
    attrib_return->width = width;
    attrib_return->height = height;
    attrib_return->cpp = cpp;
    attrib_return->ncolors = ncolors;
    attrib_return->colorTable = colorTable;
    attrib_return->pixelindex = pixelindex;

    if (attributes) {
	if (attributes->valuemask & XpmReturnInfos) {
	    attributes->hints_cmt = hints_cmt;
	    attributes->colors_cmt = colors_cmt;
	    attributes->pixels_cmt = pixels_cmt;
	}
	if (hotspot) {
	    attributes->x_hotspot = x_hotspot;
	    attributes->y_hotspot = y_hotspot;
	    attributes->valuemask |= XpmHotspot;
	}
    }
    return (XpmSuccess);
}


/*
 * open the given array to be read or written as an xpmData which is returned
 */
void _MMxpmOpenArray( char **data, xpmData *mdata)
{
    mdata->type = XPMARRAY;
    mdata->stream.data = data;
    mdata->cptr = *data;
    mdata->line = 0;
    mdata->CommentLength = 0;
    mdata->Bcmt = mdata->Ecmt = NULL;
    mdata->Bos = mdata->Eos = '\0';
}

unsigned char *_MMReadXpm3Pixmap( Widget view, FILE *fp, char *datafile, int *w, int *h,
	XColor *colrs, int *bg)
{
	xpmData mdata;
	XpmAttributes attributes;
	xpmInternAttrib attrib;
	int ErrorStatus;
	int Colors;
	XColor tmpcolr;
	int i/*, j*/;
	char **colorStrings;
	char *colorName;
	unsigned char *pix_data;
	unsigned char *bptr;
	unsigned int *pixels;

	*w = 0;
	*h = 0;

	attributes.valuemask = XpmReturnPixels;

	if ((ErrorStatus = _MMxpmReadFile(datafile, &mdata)) != XpmSuccess) {
		return(NULL);
	}

	_MMxpmInitInternAttrib(&attrib);

	ErrorStatus = _MMxpmParseData(&mdata, &attrib, &attributes);
	if (ErrorStatus != XpmSuccess) {
		_MMxpmFreeInternAttrib(&attrib);
		_MMxpmDataClose(&mdata);
		return(NULL);
	}

	*w = (int)attrib.width;
	*h = (int)attrib.height;
	Colors = (int)attrib.ncolors;

	for (i=0; i<Colors; i++) {
		colorStrings = attrib.colorTable[i];
		colorName = colorStrings[NKEYS];
		if (strcmp(colorName, TRANSPARENT_COLOR) == 0) {
			unsigned long bg_pixel;

			/* First, go fetch the pixel. */
			XtVaGetValues (view, XtNbackground, &bg_pixel, NULL);

			/* Now, load up tmpcolr. */
			tmpcolr.pixel = bg_pixel;

			/* Now query for the full color info. */
			HTMLPixelToXColor(&tmpcolr);
			*bg = i;
		} else {
			XParseColor(mMosaicDisplay, mMosaicColormap, colorName, &tmpcolr);
		}
		colrs[i].red = tmpcolr.red;
		colrs[i].green = tmpcolr.green;
		colrs[i].blue = tmpcolr.blue;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}
	for (i=Colors; i<256; i++) {
		colrs[i].red = 0;
		colrs[i].green = 0;
		colrs[i].blue = 0;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}

	pixels = attrib.pixelindex;
	pix_data = (unsigned char *)malloc((*w) * (*h));
        if (pix_data == NULL) {
		fprintf(stderr, "Not enough memory for data.\n");

		_MMxpmFreeInternAttrib(&attrib);
		_MMxpmDataClose(&mdata);
                return((unsigned char *)NULL);
        }
	bptr = pix_data;
        for (i=0; i<((*w) * (*h)); i++) {
		int pix;

		pix = (int)*pixels;
                if (pix > (256 - 1))
                        pix = 0;
		*bptr++ = (unsigned char)pix;
                pixels++;
        }

	_MMxpmFreeInternAttrib(&attrib);
	_MMxpmDataClose(&mdata);

        return(pix_data);
}

unsigned char *_MMProcessXpm3Data( Widget wid, char **xpmdata,
	int *w, int *h, XColor *colrs, int *bg)
{
	xpmData mdata;
	XpmAttributes attributes;
	xpmInternAttrib attrib;
	int ErrorStatus;
	int Colors;
	XColor tmpcolr;
	int i/*, j*/;
	char **colorStrings;
	char *colorName;
	unsigned char *pix_data;
	unsigned char *bptr;
	unsigned int *pixels;

	*w = 0;
	*h = 0;
	attributes.valuemask = XpmReturnPixels;
	_MMxpmOpenArray(xpmdata, &mdata);
	_MMxpmInitInternAttrib(&attrib);
	ErrorStatus = _MMxpmParseData(&mdata, &attrib, &attributes);
	if (ErrorStatus != XpmSuccess) {
		_MMxpmFreeInternAttrib(&attrib);
		_MMxpmDataClose(&mdata);
		return(NULL);
	}

	*w = (int)attrib.width;
	*h = (int)attrib.height;
	Colors = (int)attrib.ncolors;

	for (i=0; i<Colors; i++) {
		colorStrings = attrib.colorTable[i];
		colorName = colorStrings[NKEYS];
		if (strcmp(colorName, TRANSPARENT_COLOR) == 0) {
			unsigned long bg_pixel;

			/* First, go fetch the pixel. */
			XtVaGetValues (wid, XtNbackground, &bg_pixel, NULL);

			/* Now, load up tmpcolr. */
			tmpcolr.pixel = bg_pixel;

			/* Now query for the full color info. */
			HTMLPixelToXColor(&tmpcolr);
			*bg = i;
		} else {
			XParseColor(XtDisplay(wid), mMosaicColormap,
				colorName, &tmpcolr);
		}
		colrs[i].red = tmpcolr.red;
		colrs[i].green = tmpcolr.green;
		colrs[i].blue = tmpcolr.blue;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}
	for (i=Colors; i<256; i++) {
		colrs[i].red = 0;
		colrs[i].green = 0;
		colrs[i].blue = 0;
		colrs[i].pixel = i;
		colrs[i].flags = DoRed|DoGreen|DoBlue;
	}

	pixels = attrib.pixelindex;
	pix_data = (unsigned char *)malloc((*w) * (*h));
        if (pix_data == NULL) {
		fprintf(stderr, "Not enough memory for data.\n");
		_MMxpmFreeInternAttrib(&attrib);
		_MMxpmDataClose(&mdata);
                return((unsigned char *)NULL);
        }
	bptr = pix_data;
        for (i=0; i<((*w) * (*h)); i++) {
		int pix;

		pix = (int)*pixels;
                if (pix > (256 - 1))
                        pix = 0;
		*bptr++ = (unsigned char)pix;
                pixels++;
        }

	_MMxpmFreeInternAttrib(&attrib);
	_MMxpmDataClose(&mdata);

        return(pix_data);
}

