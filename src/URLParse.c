/* Parse HyperText Document Address */
/* Source from libwww2 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "URLParse.h"

struct struct_parts {
	char * access;
	char * host;
	char * absolute;
	char * relative;
	char * anchor;
};

/*	Scan a filename for its consituents
** On entry,
**	name	points to a document name which may be incomplete.
** On exit,
**      absolute or relative may be nonzero (but not both).
**	host, anchor and access may be nonzero if they were specified.
**	Any which are nonzero point to zero terminated strings.
*/
static void scan(char * name, struct struct_parts *parts)
{
	char * after_access;
	char * p;
	int length;

	if (name && *name)
		length = strlen(name);
	else
		length = 0;
	parts->access = 0;
	parts->host = 0;
	parts->absolute = 0;
	parts->relative = 0;
	parts->anchor = 0;
/* Argh. */
	if (!length)
		return;
	after_access = name;
	for(p=name; *p; p++) {
		if (*p==':') {
			*p = 0;
			parts->access = name;	/* Access name has been specified */
			after_access = p+1;
		}
		if (*p=='/') break;
		if (*p=='#') break;
	}
	for(p=name+length-1; p>=name; p--) {
		if (*p =='#') {
			parts->anchor=p+1;
			*p=0;				/* terminate the rest */
		}
	}
	p = after_access;
	if (*p=='/'){
		if (p[1]=='/') {
			parts->host = p+2;		/* host has been specified 	*/
			*p=0;			/* Terminate access 		*/
			p=strchr(parts->host,'/');	/* look for end of host name if any */
			if(p) {
				*p=0;			/* Terminate host */
				parts->absolute = p+1;		/* Root has been found */
			}
		} else {
			parts->absolute = p+1;		/* Root found but no host */
		}	    
	} else {
		parts->relative = (*after_access) ? after_access : 0;	/* zero for "" */
	}
/* Access specified but no host: the anchor was not really one
e.g. news:j462#36487@foo.bar -- JFG 10/7/92, from bug report */
	if (parts->access && ! parts->host && parts->anchor) {
		*(parts->anchor - 1) = '#';  /* Restore the '#' in the address */
		parts->anchor = 0;
	}
} /*scan */    

/* Simplify a filename
 * A unix-style file is allowed to contain the seqeunce xxx/../ which may be
 * replaced by "" , and the seqeunce "/./" which may be replaced by "/".
 * Simplification helps us recognize duplicate filenames.
 *
 *	Thus, 	/etc/junk/../fred 	becomes	/etc/fred
 *		/etc/junk/./fred	becomes	/etc/junk/fred
 *
 *      but we should NOT change
 *		http://fred.xxx.edu/../..
 *
 *	or	../../albert.html
*/
static void HTSimplify(char * filename)
{
  char * p;
  char * q;
  if (filename[0] && filename[1]) {
      for(p=filename+2; *p; p++) {
          if (*p=='/') {
              if ((p[1]=='.') && (p[2]=='.') && (p[3]=='/' || !p[3] )) {
                  /* Changed clause below to (q>filename) due to attempted
                     read to q = filename-1 below. */
                  for (q = p-1; (q>filename) && (*q!='/'); q--)
                    ; /* prev slash */
                  if (q[0]=='/' && 0!=strncmp(q, "/../", 4)
                      && !(q-1>filename && q[-1]=='/')) {
                      strcpy(q, p+3);	/* Remove  /xxx/..	*/
                      if (!*filename) strcpy(filename, "/");
                      p = q-1;		/* Start again with prev slash 	*/
                    } 
                } else if ((p[1]=='.') && (p[2]=='/' || !p[2])) {
                  strcpy(p, p+2);			/* Remove a slash and a dot */
                }
            }
        }
    }
}

/*	Parse a Name relative to another name
**	This returns those parts of a name which are given (and requested)
**	substituting bits from the related name where necessary.
** On entry,
**	aName		A filename given
**      relatedName     A name relative to which aName is to be parsed
**      wanted          A mask for the bits which are wanted.
** On exit,
**	returns		A pointer to a malloc'd string which MUST BE FREED
*/
char * URLParse(const char * aName, const char * relatedName, int wanted)
{
	char * result = 0;
	char * return_value = 0;
	int len;
	char * name = 0;
	char * rel = 0;
	char * p;
	char *access;
	struct struct_parts given, related;

	if (!aName)
		aName = "\0";
	if (!relatedName)
		relatedName = "\0";

/* Make working copies of input strings to cut up: */
	len = strlen(aName) + strlen(relatedName) + 10;
	result = (char *) malloc(len); /* Lots of space: more than enough */
	name = strdup(aName);
	rel  = strdup(relatedName);

	scan(name, &given);
	scan(rel,  &related); 
	result[0]=0;		/* Clear string  */
	access = given.access ? given.access : related.access;
	if (wanted & PARSE_ACCESS) {
		if (access) {
			strcat(result, access);
			if(wanted & PARSE_PUNCTUATION)
				strcat(result, ":");
		}
	}
	if (given.access && related.access) /* If different, inherit nothing. */
		if (strcmp(given.access, related.access)!=0) {
			related.host=0;
			related.absolute=0;
			related.relative=0;
			related.anchor=0;
		}
	if (wanted & PARSE_HOST) {
		if(given.host || related.host) {
			char * tail = result + strlen(result);   

			if(wanted & PARSE_PUNCTUATION)
				strcat(result, "//");
			strcat(result, given.host ? given.host : related.host);
#define CLEAN_URLS
#ifdef CLEAN_URLS
/* Ignore default port numbers, and trailing dots on FQDNs
which will only cause identical adreesses to look different */
			p = strchr(tail, ':');
			if (p && access) {		/* Port specified */
				if ((strcmp(access, "http") == 0 && strcmp(p, ":80") == 0) ||
				(strcmp(access, "gopher") == 0 && 
				(strcmp(p, ":70") == 0 ||
				strcmp(p, ":70+") == 0)))
					*p = (char)0;	/* It is the default: ignore it */
				else if (p && *p && p[strlen(p)-1] == '+')
					p[strlen(p)-1] = 0;
			}
			if (!p) 
				p = tail + strlen(tail); /* After hostname */
			p--;				/* End of hostname */
			if (strlen (tail) > 3 && (*p == '.')) {
				*p = (char)0; /* chop final . */
/* OK, at this point we know that *(p+1) exists,
 * else we would not be here.
 * If it's 0, then we're done.
 * If it's not 0, then we move *(p+2) to *(p+1),
 * etc.
*/
				if (*(p+1) != '\0') {
					memcpy (p, p+1, strlen(p+1));
					*(p + strlen (p+1)) = '\0';
				}
			}
			{
				char *tmp;
				tmp = strchr (tail, '@');
				if (!tmp)
					tmp = tail;
				for (; *tmp; tmp++)
					*tmp = tolower (*tmp);
			}
#endif
		}
	}
	if (given.host && related.host)  /* If different hosts, inherit no path. */
		if (strcmp(given.host, related.host)!=0) {
			related.absolute=0;
			related.relative=0;
			related.anchor=0;
		}
	if (wanted & PARSE_PATH) {
		if(given.absolute) {		/* All is given */
			if(wanted & PARSE_PUNCTUATION)
				strcat(result, "/");
			strcat(result, given.absolute);
		} else if(related.absolute) {	/* Adopt path not name */
			strcat(result, "/");
			strcat(result, related.absolute);
			if (given.relative) {
				p = strchr(result, '?'); /* Search part? */
				if (!p)
				 	p=result+strlen(result)-1;
				for (; *p!='/'; p--);	/* last / */
				p[1]=0;		/* Remove filename */
				strcat(result, given.relative);	/* Add given one */
				HTSimplify (result);
			}
		} else if(given.relative) {
			strcat(result, given.relative);	/* what we've got */
		} else if(related.relative) {
			strcat(result, related.relative);
		} else {  /* No inheritance */
			strcat(result, "/");
		}
	}
	if (wanted & PARSE_ANCHOR)
		if(given.anchor || related.anchor) {
			if(wanted & PARSE_PUNCTUATION)
				strcat(result, "#");
			strcat(result, given.anchor ? given.anchor : related.anchor);
		}
	if (rel)
		free(rel);
	if (name)
		free(name);
	return_value = strdup(result);
	free(result);
	return return_value;	/* exactly the right length */
}


char *mo_url_canonicalize_local (char *url)
{
	char blah[129];
	char *cwd = getcwd (blah, 128);
	char *tmp;

	if (!url)
		return NULL;
	tmp = (char *)malloc ((strlen (url) + strlen (cwd) + 32));
	if (url[0] == '/')
		sprintf (tmp, "file://localhost%s", url);
	else
		sprintf (tmp, "file://localhost%s/%s", cwd, url);
	return tmp;
}

/* guess a complete url from incomplete
 * Contributed by martin@gizmo.lut.ac.uk, modified by spowers@ncsa.uiuc.edu
 */
char * UrlGuess(char *url)
{
	struct stat buf;
	char *xurl;

	if (strncasecmp(url,"mailto:",7) && strncasecmp(url,"news:",5) &&
	    !strstr(url,"://")) { /*no protocol specified, default*/
		if (!strncmp(url, "www.", 4)) {
			xurl = (char *)malloc(strlen(url) + (8 * sizeof(char)));
			sprintf(xurl, "http://%s", url);
		} else if (!strncmp(url, "gopher.", 7)) {
			xurl = (char *)malloc(strlen(url) + (10 * sizeof(char)));
			sprintf(xurl, "gopher://%s", url);
		} else if (!strncmp(url, "ftp.", 4)) {
			xurl = (char *)malloc(strlen(url) + (7 * sizeof(char)));
			sprintf(xurl, "ftp://%s", url);
		} else if (!strncmp(url, "wais.", 5)) {
			xurl = (char *)malloc(strlen(url) + (8 * sizeof(char)));
			sprintf(xurl, "wais://%s", url);
		} else if (!stat(url,&buf)) { /*its a file and we have access*/
			xurl=mo_url_canonicalize_local(url);
		} else { 
			xurl=(char *)calloc(strlen(url)+15,sizeof(char));
			sprintf(xurl,"http://%s",url);
		}        
	} else { /*protocol was specified*/
		xurl=strdup(url);
	}        
	return(xurl);
}

/* Turn a URL into its canonical form, based on the previous
 * URL in this context (if appropriate).
 * INTERNAL ANCHORS ARE STRIPPED OFF.
 * inputs:
 *   - char    *url: URL to canonicalize.
 *   - char *oldurl: The previous URL in this context.
 * returns:
 *   The canonical representation of the URL.
 */
char *mo_url_canonicalize (char *url, char *oldurl)
{
/* We LOSE anchor information. */
	return URLParse (url, oldurl, PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
		PARSE_PUNCTUATION);
}

/* Turn a URL into its canonical form, based on the previous
 * URL in this context (if appropriate).
 * INTERNAL ANCHORS ARE *NOT* STRIPPED OFF.
 * inputs:
 *   - char    *url: URL to canonicalize.
 *   - char *oldurl: The previous URL in this context.
 * returns:
 *   The canonical representation of the URL.
 */
char *mo_url_canonicalize_keep_anchor (char *url, char *oldurl)
{
	char *rv;
/* We KEEP anchor information already present in url, but NOT in oldurl. */
	oldurl = URLParse (oldurl, "", PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
		PARSE_PUNCTUATION);
	rv = URLParse (url, oldurl, PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
		PARSE_PUNCTUATION | PARSE_ANCHOR);
/* We made a new copy of oldurl, so free the new copy. */
	free (oldurl);
	return rv;
}

/* ---------------------------- escaping code ----------------------------- */

static unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{    0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,   /* 2x   !"#$%&'()*+,-./  */
     1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,   /* 3x  0123456789:;<=>?  */
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   /* 4x  @ABCDEFGHIJKLMNO  */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,   /* 5x  PQRSTUVWXYZ[\]^_  */
     0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   /* 6x  `abcdefghijklmno  */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }; /* 7x  pqrstuvwxyz{\}~  DEL */

#define MO_HEX(i) (i < 10 ? '0'+i : 'A'+ i - 10)

/* The string returned from here, if any, can be free'd by caller. */
char * EscapeUrl(char *part)
{
	char *q;
	char *p;              /* Pointers into keywords */
	char *escaped;

	if (!part)
		return NULL;
	escaped = (char *)malloc (strlen (part) * 3 + 1);
	for (q = escaped, p = part; *p != '\0'; p++) {
/* Makes sure that values 128 and over don't get converted to negative values. */
		int c = (int)((unsigned char)(*p));
		if (*p == ' ') {
			*q++ = '+';
		} else if (c >= 32 && c <= 127 && isAcceptable[c-32]) {
			*q++ = *p;
		} else {
			*q++ = '%';
			*q++ = MO_HEX(c / 16);
			*q++ = MO_HEX(c % 16);
		}
	}
	*q=0;
	return escaped;
}

static char x2c (char c)
{
	return ((c >= '0' && c <= '9') ? (c - '0') :
		((c >= 'A' && c <= 'F') ? (c - 'A' + 10) :
		(c - 'a' + 10)));
}

char *UnEscapeUrl (char *str)
{
	char *p = str, *q = str;

	while (*p) {     /* Plus's turn back into spaces. */
		if (*p == '+') {
			*q++ = ' ';
			p++;
		} else if (*p == '%') {
			p++;
			if (*p)
				*q = x2c(*p++) * 16;
			if (*p)
				*q += x2c(*p++);
			q++;
		} else {
			*q++ = *p++;
		}
	}
	*q++ = 0;
	return str;
}
