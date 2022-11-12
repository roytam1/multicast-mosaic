#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>

#include "../libnut/list.h"

const char * MMOSAIC_PRESENT = "mmosaic_presentation" ;

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mime.h"
#include "util.h"

#define outofmem(file, func) \
 { fprintf(stderr, "%s %s: out of memory.\nProgram aborted.\n", file, func); \
  exit(1);}

/* Unknow by mMosaic when come from a http server */
/* 
 * Age
 * Allow
 * Connection : is for HTTP/1.1 . Those server assume persistant connection
 *	Connection: close
 *	for non-persistant connection
 * Content-Base
 * Content-Langage
 * Content-Location
 * Content-MD5
 * Content-Range
 * Date
 * ETag
 * Pragma
 * Proxy-Authenticate
 * Public
 * Retry-After
 * Server
 * Transfer-Encoding
 * Upgrade
 * Vary
 * Via
 * Warning
 * WWW-Authenticate
*/

/* Process by mMosaic
 * Cache-Control (partialy)
 * Content-Encoding
 *	can be x-gzip, gzip, x-compress, compress
 * Content-Length
 * Content-Type
 *	type/subtype ; paramater
 *	Content-Type: text/html; charset=ISO-8859-4
 * Expires : HTTP-date
 * Last-Modified : HTTP-date
 * Location : absoluteURI
 */

void FreeMimeStruct( MimeHeaderStruct *mhs)
{
/*	if( mhs->content_type ) free(mhs->content_type); */
/*	if( mhs->expires ) free(mhs->expires ); */
/*	if( mhs->last_modified ) free(mhs->last_modified); */
/*	if( mhs->location ) free(mhs->location); */
}

static void SetMimeToken(MimeHeaderStruct *mhs, char *token, char *value)
{
	char * tmp;

	switch (*token) {	/* for fast scan */
	case 'C':
		if ( !strncasecmp(token,"Content-Encoding",16)){
			mhs->content_encoding = NO_ENCODING;
			if ( !strncasecmp(value,"x-gzip",6) ||
			     !strncasecmp(value, "gzip",4) ){
				mhs->content_encoding = GZIP_ENCODING;
				return;
			} 
			if ( !strncasecmp(value,"compress",8) ||
			     !strncasecmp(value,"x-compress",10)) {
				mhs->content_encoding = COMPRESS_ENCODING;
				return;
			}
			return;
		}
		if ( !strncasecmp(token,"Content-Length",14)) {
			mhs->content_length = atoi(value);
			return;
		}
		if ( !strncasecmp(token,"Content-Type", 12)) {
			/* Lowercase it. And forget parameter */     
			for (tmp = value; *tmp; tmp++) {
				*tmp = tolower (*tmp);
				if (*tmp == ';') *tmp = '\0';
			}
			mhs->content_type = strdup(value);
			return;
		} 
		if ( !strncasecmp(token,"Content-Control", 15)){
			if ( !strncasecmp(value,"no-store",8)) {
				mhs->cache_control |= CACHE_CONTROL_NO_STORE;
				return;
			}
			if ( !strncasecmp(value,"no-cache",8)) {
				mhs->cache_control |= CACHE_CONTROL_NO_CACHE;
				return;
			}
		}
		return;
	case 'E':
		if ( !strncasecmp(token, "Expires",7)) {
			mhs->expires = strdup(value);
			return;
		}
		return;
	case 'L':
		if( !strncasecmp(token, "Last-Modified", 13)) {
			mhs->last_modified = strdup(value);
			return;
		}
		if( !strncasecmp(token, "Location", 8)) {
			mhs->location = strdup(value);
			return;
		}
		return;
	default:
		return;
	}
}

/* le block commence par un LF. Il se termine par "\015\000" ou "\012\000".
 * Le caractere de fin est toujours '\0'. */

#define EOL_FLF 1
#define EOL_LINE 2

#define CR '\015'
#define LF '\012'

void ParseMimeHeader(char * b, MimeHeaderStruct * mhs)
{
	int EOLstate;
	char * value = NULL;
	char * token = NULL;
	char *start_tok, *end_tok, *start_val, *end_val;

/* set some default */
	mhs->content_encoding = NO_ENCODING;
	mhs->content_length = -1;
	mhs->content_type = NULL;
	mhs->expires = NULL;
	mhs->last_modified = NULL;
	mhs->location = NULL;
	mhs->cache_control = CACHE_CONTROL_NONE;

/* parse the block and override default */

	EOLstate = EOL_FLF;

	while (*b) {	/* the buffer is ended by '\0' */
		if (EOLstate == EOL_FLF) {
			if (isspace(*b)) {	/* SPACE TAB CR LF VT FF */
				b++;
				continue;
			} else			/* New line */
				EOLstate = EOL_LINE;
		} else if (*b == CR || *b == LF ) {
			EOLstate = EOL_FLF;	/* Line found */
			b++;
			continue;
		} 
/* other char. */
		if (EOLstate != EOL_LINE) {
			b++;
			continue;
		}
/* Merde et merde : il y a des 'folded line' un truc qui commence par :
	token : value
	CRLF (SP|HT) suite de value...
*/

/* we are at Begin of line with a significant char */
		start_tok = b;
/* match  ':' */
		end_tok = strchr(b, ':');
		if (end_tok == NULL) { /* syntaxe error*/
			b = strpbrk(b,"\012\015");
			EOLstate = EOL_FLF;
			continue;
		}
		token = start_tok;
		*end_tok = '\0';

		start_val = end_tok + 1;
		while (*start_val == ' ' || *start_val == '\t')
			start_val++;
		end_val = strpbrk(start_val, "\012\015");
		*end_val = '\0';
		value = start_val;
		SetMimeToken(mhs, token, value);
		b = end_val+1;
		EOLstate = EOL_FLF;
		continue;
	}
	if (!mhs->content_type)
		mhs->content_type = strdup("text/html");
/* set a default last_modified according to the cache remanance */
	if (!mhs->last_modified) {
		time_t t;
		char *ts;

		t = time(NULL);
		t = t - 30 * 24 * 3600;	/* FIXME ### */
		ts = rfc822ctime(t);
		mhs->last_modified = strdup(ts);
	}
}

/* ############################################## */
/*      Suffix registration */

typedef struct _HTSuffix {
	char 	*suffix;
	char 	*content_type;
	float	quality;
} HTSuffix;

HTList * HTSuffixes = 0;
static HTSuffix no_suffix = { "*", NULL, 1.0 };
static HTSuffix unknown_suffix = { "*.*", NULL, 1.0};

/* Define the representation associated with a file suffix
**      Calling this with suffix set to "*" will set the default representation.
**      Calling this with suffix set to "*.*" will set the default
**      representation for unknown suffix files which contain a ".".
*/
static void HTSetSuffix ( const char *suffix, char *content_type, float value)
{
        HTSuffix * suff;

        if (strcmp(suffix, "*")==0)
                suff = &no_suffix;
        else if (strcmp(suffix, "*.*")==0)
                suff = &unknown_suffix;
        else {
                suff = (HTSuffix*) calloc(1, sizeof(HTSuffix));
                if (suff == NULL)
                        outofmem(__FILE__, "HTSetSuffix");
                if (!HTSuffixes)
                        HTSuffixes = HTList_new();
                HTList_addObject(HTSuffixes, suff);
                suff->suffix = strdup(suffix);
        }
        suff->content_type = strdup(content_type);
        suff->quality = value;
}

/* Extension config file reading */
/* The following is lifted from NCSA httpd 1.0a1, by Rob McCool;
   NCSA httpd is in the public domain, as is this code. */

#define MAX_STRING_LEN 256

static int getline(char *s, int n, FILE *f) 
{
	register int i=0;
  
	while(1) {
		s[i] = (char)fgetc(f);
		if(s[i] == CR)
			s[i] = fgetc(f);
		if((s[i] == EOF) || (s[i] == LF) || (i == (n-1))) {
			s[i] = '\0';
			return (feof(f) ? 1 : 0);
		}
		++i;
	}
	/* NOTREACHED */
}

static void getword(char *word, char *line, char stop, char stop2) 
{
	int x = 0, y;
  
	for (x = 0; line[x] && line[x] != stop && line[x] != stop2; x++) {
		word[x] = line[x];
	}
	word[x] = '\0';
	if (line[x]) 
		++x;
	y=0;
	while (line[y++] = line[x++])
		;
}

static void HTLoadExtensionsConfigFile (char *fn)
{
	char l[MAX_STRING_LEN],w[MAX_STRING_LEN],*ct,*ptr;
	FILE *f;
	int x, count = 0;

	if(!(f = fopen(fn,"r"))) {
		return ;
	}

	while(!(getline(l,MAX_STRING_LEN,f))) {
/* always get rid of leading white space for "line" -- SWP */
		for (ptr=l; *ptr && isspace(*ptr); ptr++)
			;
		getword(w,l,' ','\t');
		if(l[0] == '\0' || w[0] == '#')
			continue;
		ct = (char *)malloc(sizeof(char) * (strlen(w) + 1));
		strcpy(ct,w);
      
		while(ptr[0]) {
			getword(w,ptr,' ','\t');
			if(w[0] && (w[0] != ' ')) {
				char *ext = (char *)malloc(strlen(w)+1+1);

				for(x=0; w[x]; x++)
					ext[x+1] = tolower(w[x]);
				ext[0] = '.';
				ext[strlen(w)+1] = 0;
				HTSetSuffix (ext, ct, 1.0);
				count++;
				free (ext);
			}
		}
		free(ct);
	}
	fclose(f);
}

/* Define a basic set of suffixes
 * The LAST suffix for a type is that used for temporary files of that type.
 *	The quality is an apriori bias as to whether the file should be
 *	used.  Not that different suffixes can be used to represent files
 *	which are of the same format but are originals or regenerated,
 *	with different values.
*/

void HTExtensionMapInit (void)
{
	HTSetSuffix(".uu",	"application/octet-stream", 1.0);
	HTSetSuffix(".saveme",	"application/octet-stream", 1.0);
	HTSetSuffix(".dump",	"application/octet-stream", 1.0);

	HTSetSuffix(".hqx",	"application/octet-stream", 1.0);
	HTSetSuffix(".arc",	"application/octet-stream", 1.0);
	HTSetSuffix(".o",	"application/octet-stream", 1.0);
	HTSetSuffix(".a",	"application/octet-stream", 1.0);
	HTSetSuffix(".bin",	"application/octet-stream", 1.0);
	HTSetSuffix(".exe",	"application/octet-stream", 1.0);

	HTSetSuffix(".oda",	"application/oda", 1.0);

	HTSetSuffix(".pdf",	"application/pdf", 1.0);
	HTSetSuffix(".eps",	"application/postscript", 1.0);
	HTSetSuffix(".ai",	"application/postscript", 1.0);
	HTSetSuffix(".ps",	"application/postscript", 1.0);

	HTSetSuffix(".rtf",	"application/rtf", 1.0);
	HTSetSuffix(".dvi",	"application/x-dvi", 1.0);
	HTSetSuffix(".hdf",	"application/x-hdf", 1.0);
	HTSetSuffix(".latex", 	"application/x-latex", 1.0);
	HTSetSuffix(".cdf",	"application/x-netcdf", 1.0);
	HTSetSuffix(".nc",	"application/x-netcdf", 1.0);
	HTSetSuffix(".tex",	"application/x-tex", 1.0);
	HTSetSuffix(".texinfo",	"application/x-texinfo", 1.0);
	HTSetSuffix(".texi",	"application/x-texinfo", 1.0);
	HTSetSuffix(".t",	"application/x-troff", 1.0);
	HTSetSuffix(".tr",	"application/x-troff", 1.0);
	HTSetSuffix(".roff",	"application/x-troff", 1.0);
	HTSetSuffix(".man",	"application/x-troff-man", 1.0);
	HTSetSuffix(".me",	"application/x-troff-me", 1.0);
	HTSetSuffix(".ms",	"application/x-troff-ms", 1.0);
	HTSetSuffix(".src",	"application/x-wais-source", 1.0);
	HTSetSuffix(".wsrc",	"application/x-wais-source", 1.0);
	HTSetSuffix(".zip",	"application/zip", 1.0);
	HTSetSuffix(".bcpio",	"application/x-bcpio", 1.0);
	HTSetSuffix(".cpio",	"application/x-cpio", 1.0);
	HTSetSuffix(".gtar",	"application/x-gtar", 1.0);
	HTSetSuffix(".shar",	"application/x-shar", 1.0);
	HTSetSuffix(".sh",	"application/x-shar", 1.0); /* xtra */
	HTSetSuffix(".sv4cpio",	"application/x-sv4cpio", 1.0);
	HTSetSuffix(".sv4crc",	"application/x-sv4crc", 1.0);
	HTSetSuffix(".tar",	"application/x-tar", 1.0);
	HTSetSuffix(".ustar",	"application/x-ustar", 1.0);
	HTSetSuffix(".snd",	"audio/basic", 1.0);
	HTSetSuffix(".au",	"audio/basic", 1.0);
	HTSetSuffix(".aifc",	"audio/x-aiff", 1.0);
	HTSetSuffix(".aif",	"audio/x-aiff", 1.0);
	HTSetSuffix(".aiff",	"audio/x-aiff", 1.0);
	HTSetSuffix(".wav",	"audio/x-wav", 1.0);
	HTSetSuffix(".gif",	"image/gif", 1.0);
	HTSetSuffix(".png",	"image/png", 1.0);
	HTSetSuffix(".ief",	"image/ief", 1.0);
	HTSetSuffix(".jpe",	"image/jpeg", 1.0);
	HTSetSuffix(".jpg",	"image/jpeg", 1.0);
	HTSetSuffix(".jpeg",	"image/jpeg", 1.0);
	HTSetSuffix(".tif",	"image/tiff", 1.0);
	HTSetSuffix(".tiff",	"image/tiff", 1.0);
	HTSetSuffix(".ras",	"image/x-cmu-rast", 1.0);
	HTSetSuffix(".pnm",	"image/x-portable-anymap", 1.0);
	HTSetSuffix(".pbm",	"image/x-portable-bitmap", 1.0);
	HTSetSuffix(".pgm",	"image/x-portable-graymap", 1.0);
	HTSetSuffix(".ppm",	"image/x-portable-pixmap", 1.0);
	HTSetSuffix(".rgb",	"image/x-rgb", 1.0);
	HTSetSuffix(".xbm",	"image/x-xbitmap", 1.0);
	HTSetSuffix(".xpm",	"image/x-xpixmap", 1.0);
	HTSetSuffix(".xwd",	"image/x-xwindowdump", 1.0);
	HTSetSuffix(".htm",	"text/html", 1.0);
	HTSetSuffix(".html",	"text/html", 1.0);
	HTSetSuffix(".text",	"text/plain", 1.0);
	HTSetSuffix(".c",	"text/plain", 1.0);
	HTSetSuffix(".cc",	"text/plain", 1.0);
	HTSetSuffix(".c++",	"text/plain", 1.0);
	HTSetSuffix(".h",	"text/plain", 1.0);
	HTSetSuffix(".pl",	"text/plain", 1.0);
	HTSetSuffix(".txt",	"text/plain", 1.0);
	HTSetSuffix(".rtx",	"text/richtext", 1.0); /* MIME richtext */
	HTSetSuffix(".tsv",	"text/tab-separated-values", 1.0);
	HTSetSuffix(".etx",	"text/x-setext", 1.0);
	HTSetSuffix(".mpg",	"video/mpeg", 1.0);
	HTSetSuffix(".mpe",	"video/mpeg", 1.0);
	HTSetSuffix(".mpeg",	"video/mpeg", 1.0);
	HTSetSuffix(".mov",	"video/quicktime", 1.0);
	HTSetSuffix(".qt",	"video/quicktime", 1.0);
	HTSetSuffix(".avi",	"video/x-msvideo", 1.0);
	HTSetSuffix(".movie",	"video/x-sgi-movie", 1.0);
	HTSetSuffix(".mv",	"video/x-sgi-movie", 1.0);
	HTSetSuffix(".mime",	"message/rfc822", 1.0);

/* These should override everything else. */
	HTLoadExtensionsConfigFile (mMosaicPersonalExtensionMap);
}


/* Determine a suitable suffix, given the representation
 * On entry,                           
 *      rep     is the MIME style representation
 * On exit,                            
 *      returns a pointer to a suitable suffix string if one has been
 *              found, else "".        
 */                                     
/* content_type to file suffix */

const char * HTct2FileSuffix (char * ct)
{                                      
	HTSuffix * suff;                   
	int n;                             
	int i;                             

	n = HTList_count(HTSuffixes);      
	for(i=0; i<n; i++) {               
		suff = (HTSuffix *)HTList_objectAt(HTSuffixes, i);
		if ( !strcmp(suff->content_type , ct) ) {        
			return suff->suffix;
		}                              
	}                                  
	return "";          /* Dunno */    
}                                      

/* Determine file format from file name
 * This version will return the representation and also set a variable for
 * the encoding.   
 * It will handle for example  x.txt, x.txt.Z, x.Z
 */                                     
/* file name to content_type */

char * HTFileName2ct(char *filename, char *default_type, int *compressed)
{
        HTSuffix *suff;                
        int n, i, lf;                  
                                       
        *compressed = NO_ENCODING;               
        if (!filename)                 
                return NULL;           
                                       
/* Make a copy to hack and slash. */   
        filename = strdup (filename);  
        lf = strlen (filename);        
                                       
/* Step backward through filename, looking for '?'. */
        for (i = lf - 1; i >= 0; i--) {
                if (filename[i] == '?') {       /* Clip query. */
                        filename[i] = '\0'; 
/* Get new strlen, since we just changed it. */
                        lf = strlen (filename);
                        goto ok_ready; 
                }                      
        }                              

/* Check for .Z and .z and gz. */             
	if ( (lf > 2) && ! strcmp (&(filename[lf-2]), ".Z") ) {
		*compressed = COMPRESS_ENCODING;
		filename[lf-2] = '\0';
		lf = strlen (filename);
		goto ok_ready; 
	} else if ( (lf > 2) && !strcmp (&(filename[lf-2]), ".z") ) {
		*compressed = GZIP_ENCODING;
		filename[lf-2] = '\0';
		lf = strlen (filename);
		goto ok_ready; 
	} else if ( (lf > 3) && !strcmp (&(filename[lf-3]), ".gz") ) {
		*compressed = GZIP_ENCODING;
		filename[lf-3] = '\0';
		lf = strlen (filename);
		goto ok_ready;
	}              

ok_ready:
	n = HTList_count(HTSuffixes);  
        for(i=0; i<n; i++) {           
                int ls;                

                suff =(HTSuffix *) HTList_objectAt(HTSuffixes, i);
                ls = strlen(suff->suffix);  
                if((ls <= lf) && !strcasecmp(suff->suffix, filename + lf-ls)) {
                        if (suff->content_type) 
                                goto done;  
                }                      
        }                              
        suff = strchr(filename, '.') ?  /* Unknown suffix */
                ( unknown_suffix.content_type ? &unknown_suffix : &no_suffix)
                : &no_suffix;          
done:
/* Free our copy. */                   
        free (filename);               
        return suff->content_type ? suff->content_type : default_type;
}
 
/*      Determine file format from file name -- string version */
const char *HTFileMimeType( char *filename, char *default_type)
{
  char * content_type;
  int compressed;                      

  content_type = HTFileName2ct(filename, default_type, &compressed);
  if (content_type)            
    return content_type;       
  else                                 
    return default_type;               
}        

/* ################################################## */
/*      Presentation methods */
        
struct _HTPresentation {
        char*	rep;            /* representation name */
        char *  command;        /* command to execute for presenting something */
        float   quality;        /* Between 0 (bad) and 1 (good) */
        float   secs;
        float   secs_per_byte;
}; 

typedef struct _HTPresentation HTPresentation;
         
static HTList * HTPresentations = 0;
static HTPresentation* default_presentation = 0;
 
 
/*      Define a presentation system command for a content-type */
 
static void HTSetPresentation( char *content_type, const char *command,
	float quality, float secs, float secs_per_byte)
{
	HTPresentation * pres = (HTPresentation *)malloc(sizeof(HTPresentation));

	pres->rep = strdup(content_type);
	pres->command = strdup(command);
	pres->quality = quality;
	pres->secs = secs;
	pres->secs_per_byte = secs_per_byte;

	if (!HTPresentations)
		HTPresentations = HTList_new();

	if (strcmp(content_type, "*")==0) {
		if (default_presentation)
			free(default_presentation);
		default_presentation = pres;
	} else {
		HTList_addObjectAtEnd(HTPresentations, pres);
	}
}

/* Some of the following is taken from: */
/* Copyright (c) 1991 Bell Communications Research, Inc. (Bellcore)
 * Permission to use, copy, modify, and distribute this material for any purpose
 * and without fee is hereby granted, provided that the above copyright notice
 * and this permission notice appear in all copies, and that the name of
 * Bellcore not be used in advertising or publicity pertaining to this material
 * without the specific, prior written permission of an authorized representative
 * of Bellcore.  BELLCORE MAKES NO REPRESENTATIONS ABOUT THE ACCURACY OR
 * SUITABILITY OF THIS MATERIAL FOR ANY PURPOSE.  IT IS PROVIDED "AS IS", 
 * WITHOUT ANY EXPRESS OR IMPLIED WARRANTIES.
*/
/****************************************************** 
    Metamail -- A tool to help diverse mail readers 
                cope with diverse multimedia mail formats.
    Author:  Nathaniel S. Borenstein, Bellcore
 ******************************************************* */

static char *GetCommand(char *s, char **t)
{
	char *s2;
	int quoted = 0;

	s2 = (char*)malloc(strlen(s)*2 + 1); /* absolute max, if all % signs */
	*t = s2;
	while (s && *s) {
		if (quoted) {
			if (*s == '%')
				*s2++ = '%'; /* Quote through next level, ugh! */
			*s2++ = *s++;
			quoted = 0;
		} else {
			if (*s == ';') {
				*s2 = 0;
				return(++s);
			}
			if (*s == '\\') {
				quoted = 1;
				++s;
			} else {
				*s2++ = *s++;
			}
		}
	}
	*s2 = 0;
	return(NULL);
}

static char *Cleanse(char *s) /* no leading or trailing space, all lower case */
{
	char *tmp, *news;

/* strip leading white space */
	while (*s && isspace((unsigned char) *s))
		++s;
	news = s;

/* put in lower case */
	for (tmp=s; *tmp; ++tmp) {
		*tmp = tolower ((unsigned char)*tmp);
	}
/* strip trailing white space */
	while (*--tmp && isspace((unsigned char) *tmp))
		*tmp = 0;
	return(news);
}
#define LINE_BUF_SIZE	2000

static void ProcessMailcapEntry(FILE *fp)
{
	int rawentryalloc = LINE_BUF_SIZE, len;
	char *rawentry, *s, *t, *LineBuf;
	char * contenttype ;
	char * command;

	LineBuf = (char*)malloc(LINE_BUF_SIZE);
	rawentry = (char*)malloc(1 + rawentryalloc);
	*rawentry = 0;

	while (fgets(LineBuf, LINE_BUF_SIZE, fp)) {
		if (LineBuf[0] == '#')
			continue;
		len = strlen(LineBuf);
		if (LineBuf[len-1] == '\n')
			LineBuf[--len] = 0;
		if ((len + strlen(rawentry)) > rawentryalloc) {
			rawentryalloc += 2000;
			rawentry = (char*)realloc(rawentry, rawentryalloc+1);
		}
		if (len > 0 && LineBuf[len-1] == '\\') {
			LineBuf[len-1] = 0;
			strcat(rawentry, LineBuf);
		} else {
			strcat(rawentry, LineBuf);
			break;
		}
	}
	free(LineBuf);
	for (s=rawentry; *s && isspace((unsigned char) *s); ++s)
		;
	if (!*s) {
/* totally blank entry -- quietly ignore */
		free(rawentry);
		return;
	}
	s = strchr(rawentry, ';');
	if (!s) {
		fprintf(stderr, "Ignoring invalid mailcap entry: %s\n", rawentry);
		free(rawentry);
		return;
	}
	*s++ = 0;
	contenttype = (char*)malloc(1+strlen(rawentry));
	strcpy(contenttype, rawentry);
	t = GetCommand(s, &command);
	if (!t) {
		free(rawentry);
		goto do_presentation;
	}
	while (s && *s && isspace((unsigned char) *s)) ++s;
	s = t;
	while (s) {
		char *arg, *eq;

		t = GetCommand(s, &arg);
		eq = strchr(arg, '=');
		if (eq) *eq++ = 0;
/* Error check added by marca, oct 24 1993. */
		if (arg && *arg)
			arg = Cleanse(arg);
		s = t;
	}
	free(rawentry);

do_presentation:
	HTSetPresentation(contenttype, command, 1.0, 3.0, 0.0);
	free(contenttype);
	free(command);
	return;
}

static ProcessMailcapFile(char *file)
{
	FILE *fp;

	fp = fopen(file, "r");
	while (fp && !feof(fp)) {
		ProcessMailcapEntry(fp);
	}
	if (fp) fclose(fp);
	return(-1);
}

void HTPresentationInit (void)
{
/* Wonder what HTML will end up as? */
	HTSetPresentation("text/html", MMOSAIC_PRESENT, 1.0, 0.0, 0.0);
	HTSetPresentation("text/x-html", MMOSAIC_PRESENT, 1.0, 0.0, 0.0);
	HTSetPresentation("application/html",MMOSAIC_PRESENT, 1.0, 0.0, 0.0);
	HTSetPresentation("application/x-html",MMOSAIC_PRESENT, 1.0, 0.0, 0.0);
	HTSetPresentation("text/plain",MMOSAIC_PRESENT, 1.0, 0.0, 0.0);
 
/* These should override everything else. */
	ProcessMailcapFile(mMosaicPersonalTypeMap);

/* These should always be installed if we have internal support;
 * can be overridden by users. */

#if defined(__sgi)
	HTSetPresentation("audio/basic", "sfplay %s", 1.0, 3.0, 0.0);
	HTSetPresentation("audio/x-aiff", "sfplay %s", 1.0, 3.0, 0.0);
#else /* not __sgi */
#if defined(ultrix) || defined(__alpha)
	HTSetPresentation("audio/basic", "aplay %s", 1.0, 3.0, 0.0);
	HTSetPresentation("audio/x-aiff", "aplay %s", 1.0, 3.0, 0.0);
#else /* not ultrix or __alpha */
#if defined(SOLARIS)
	HTSetPresentation("audio/basic", "audiotool %s", 1.0, 3.0, 0.0);
	HTSetPresentation("audio/x-aiff", "audiotool %s", 1.0, 3.0, 0.0);
#else /* not SOLARIS */
	HTSetPresentation("audio/basic", "showaudio %s", 1.0, 3.0, 0.0);
	HTSetPresentation("audio/x-aiff", "showaudio %s", 1.0, 3.0, 0.0);
#endif /* not SOLARIS */
#endif /* not ultrix or __alpha */
#endif /* not __sgi */
	HTSetPresentation("image/gif", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/jpeg", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/png", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-png", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/tiff", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-portable-anymap", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-portable-bitmap", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-portable-graymap", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-portable-pixmap", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-rgb", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/rgb", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-xbitmap", "xv %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-xpixmap", "xv %s", 1.0, 3.0, 0.0); /* ?? */
	HTSetPresentation("image/xwd", "xwud -in %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-xwd", "xwud -in %s", 1.0, 3.0, 0.0);
	HTSetPresentation("image/x-xwindowdump", "xwud -in %s", 1.0, 3.0, 0.0);
	HTSetPresentation("video/mpeg", "mpeg_play %s", 1.0, 3.0, 0.0);
#ifdef __sgi
	HTSetPresentation("video/quicktime", "movieplayer -f %s", 1.0, 3.0, 0.0);
	HTSetPresentation("video/x-sgi-movie","movieplayer -f %s", 1.0, 3.0, 0.0);
#endif
	HTSetPresentation("application/postscript","ghostview %s", 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-dvi", "xdvi %s", 1.0, 3.0, 0.0);
	HTSetPresentation("message/rfc822","xterm -e metamail %s", 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-latex", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-tex", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-texinfo",MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-troff", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation("application/x-troff-man",MMOSAIC_PRESENT,1.0,3.0, 0.0);
	HTSetPresentation("application/x-troff-me",MMOSAIC_PRESENT,1.0, 3.0, 0.0);
	HTSetPresentation("application/x-troff-ms",MMOSAIC_PRESENT,1.0, 3.0, 0.0);
	HTSetPresentation("text/richtext", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation("text/tab-separated-values",MMOSAIC_PRESENT,1.0,3.0,0.0);
	HTSetPresentation("text/x-setext", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);

	HTSetPresentation ("*/*", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
	HTSetPresentation ("*", MMOSAIC_PRESENT, 1.0, 3.0, 0.0);
}

/* convert content_type 2 presentation */

char * MMct2Presentation( char * ct)
{
	HTPresentation * pres;
	int i;
	int n = HTList_count(HTPresentations);

	for(i = 0; i < n; i++) {
		pres = (HTPresentation*)HTList_objectAt(HTPresentations, i);
		if ( !strcmp(pres->rep, ct) ){
			return pres->command;
		}
	}
/* a ecricre eventuellement :
 * 	si NULL faire un essai sur le type de fichier
 */
	return MMOSAIC_PRESENT;
}

/* --------------- */
/* Author: Charles Henrich (henrich@crh.cl.msu.edu) October 2, 1993 */

struct typemap
{
        char *format;
        char *image;
};

struct typemap type_map[] = {
        {"image",       "internal-gopher-image"},
        {"text",        "internal-gopher-text"},
        {"audio",       "internal-gopher-sound"},
        {"application", "internal-gopher-binary"},
        {"message",     "internal-gopher-text"},
        {"video",       "internal-gopher-movie"},
        {"directory",   "internal-gopher-menu"},
        {"unknown",     "internal-gopher-unknown"},
        {"EOFEOF",      "EOFEOF"}
};

char *HTgeticonname(char *format, char *defaultformat)
{
        int count;
        char *ptr;
        char subtype[128];

        if(format != NULL) {
                strcpy(subtype, format);
                ptr=strchr(subtype,'/');
                if(ptr != NULL)
                        *ptr = '\0';
        } else {
                subtype[0] = '\0';
        }
        ptr = NULL;
        for(count=0 ;strcmp(type_map[count].image,"EOFEOF") !=0;count++) {
                if(strcmp(type_map[count].format, subtype) == 0)
                        return type_map[count].image;
                if(strcmp(type_map[count].format, defaultformat) == 0)
                        ptr = type_map[count].image;
        }
        if(ptr != NULL)
                return ptr;
        return "internal-gopher-unknown";
}

/*########### */
/* This doesn't do Gopher typing yet. */
/* This assumes we get a canonical URL and that URLParse works. */
/*char *HTDescribeURL (char *url)
{
  char line[512];
  const char *type;
  char *t, *st = NULL;
  char *host;
  char *access;
  int i;

  type = HTFileMimeType (url, "text/html");

  t = strdup (type);
  for (i = 0; i < strlen (t); i++) {
      if (t[i] == '/') {
          t[i] = '\0';
          if (t[i+1] != '\0' && t[i+1] != '*')
            st = &(t[i+1]);
          goto got_subtype;
        }
    }
got_subtype:
  
  access = URLParse (url, "", PARSE_ACCESS);
  host = URLParse (url, "", PARSE_HOST);

/*  if (st) {
      /* Uppercase type, to start sentence. */
/*      t[0] = toupper(t[0]);
      /* Crop x- from subtype. */
/*      if (st[0] == 'x' && st[1] == '-')
        st = &(st[2]);
      sprintf (line, "%s%s, type %s, on host %s, via %s.", t, 
               (strcmp (t, "Application") == 0 ? " data" : ""), st, host, access);
    } else {
      sprintf (line, "Type %s, on host %s, via %s.", type, host, access);
    }
  free (access);
  free (host);
  free (t);

  return strdup (line);
}
*/

/*
static int partial_wildcard_matches (char * r1, char * r2)
{
  /* r1 is the presentation format we're currently looking at out
     of the list we understand.  r2 is the one we need to get to. */
/*  char *s1, *s2, *subtype1 = NULL, *subtype2 = NULL;
  int i;

  s1 = strdup (r1);
  s2 = strdup (r2);
  for (i = 0; i < strlen (s1); i++)
    if (s1[i] == '/') {
        s1[i] = '\0';
        subtype1 = &(s1[i+1]);
        /* Now s1 contains the main type and subtype1 contains the subtype. */
/*        goto done1;
    }

done1:
  if (!subtype1)
    goto nope;
  
  /* Bail if we don't have a wildcard possibility. */
/*  if (subtype1[0] != '*')
    goto nope;

  for (i = 0; i < strlen (s2); i++)
    if (s2[i] == '/') {
        s2[i] = '\0';
        subtype2 = &(s2[i+1]);
        /* Now s2 contains the main type and subtype2 contains
           the subtype. */
/*        goto done2;
    }

done2:
  if (!subtype2)
    goto nope;

  /* Bail if s1 and s2 aren't the same and s1[0] isn't '*'. */
/*  if (strcmp (s1, s2) && s1[0] != '*')
    goto nope;

  /* OK, so now either we have the same main types or we have a wildcard
     type for s1.  We also know that we have a wildcard possibility in
     s1.  Therefore, at this point, we have a match. */
/*  free (s1);
  free (s2);
  return 1;

nope:
  free (s1);
  free (s2);
  return 0;
}
*/

/*
char *supportedTypes[]={
        "image/gif",
        "image/jpeg",
        "image/jpg",
        "image/png",
        "image/x-png",
        "image/x-pcd-jpeg",
        "image/x-pcd-jycc",
	"image/xpm",
	"image/xbm",
	"image/xpixmap",
	"image/xbitmap",
	"image/x-xpixmap",
	"image/x-xbitmap",
        "\n"
};

int supportedImageType(char *mt) 
{
	int i;

	if (!mt || !*mt)
		return(0);

	for (i=0; supportedTypes[i][0]!='\n'; i++) {
		if (!strcmp(supportedTypes[i],mt)) {
			return(1);
		}
	}
	return(0);
}
*/
