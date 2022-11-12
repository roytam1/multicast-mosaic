/*		Case-independent string comparison
**
**	Original version came with listserv implementation.
**	Version TBL Oct 91 replaces one which modified the strings.
**	02-Dec-91 (JFG) Added stralloccopy and stralloccat
**	23 Jan 92 (TBL) Changed strallocc* to 8 char HTSAC* for VM and suchlike
*/
#include <stdio.h>
#include <ctype.h>
#include "HTUtils.h"
#include "tcp.h"

WWW_CONST char * HTLibraryVersion = "2.12m"; /* String for help screen etc */

/*	Strings of any length */
int strcasecomp(WWW_CONST char *a, WWW_CONST char *b)
{
	WWW_CONST char *p =a;
	WWW_CONST char *q =b;
	for(p=a, q=b; *p && *q; p++, q++) {
	    int diff = TOLOWER(*p) - TOLOWER(*q);
	    if (diff) return diff;
	}
	if (*p) return 1;	/* p was longer than q */
	if (*q) return -1;	/* p was shorter than q */
	return 0;		/* Exact match */
}

/*	With count limit */

int strncasecomp(WWW_CONST char *a, WWW_CONST char *b, int n)
{
	WWW_CONST char *p =a;
	WWW_CONST char *q =b;
	
	for(p=a, q=b;; p++, q++) {
	    int diff;
	    if (p == a+n) return 0;	/*   Match up to n characters */
	    if (!(*p && *q)) return *p - *q;
	    diff = TOLOWER(*p) - TOLOWER(*q);
	    if (diff) return diff;
	}
	/*NOTREACHED*/
}

/*	Allocate a new copy of a string, and returns it */

char * HTSACopy(char **dest, WWW_CONST char *src)
{
  if (!dest)
	return NULL;
  if (*dest) free(*dest);
  if (! src)
    *dest = NULL;
  else {
    *dest = (char *) malloc (strlen(src) + 1);
    if (*dest == NULL) outofmem(__FILE__, "HTSACopy");
    strcpy (*dest, src);
  }
  return *dest;
}

/*	String Allocate and Concatenate */

char * HTSACat (char **dest, WWW_CONST char *src)
{
  if (src && *src) {
    if (*dest) {
      int length = strlen (*dest);
      *dest = (char *) realloc (*dest, length + strlen(src) + 1);
      if (*dest == NULL) outofmem(__FILE__, "HTSACat");
      strcpy (*dest + length, src);
    } else {
      *dest = (char *) malloc (strlen(src) + 1);
      if (*dest == NULL) outofmem(__FILE__, "HTSACat");
      strcpy (*dest, src);
    }
  }
  return *dest;
}


/*	Find next Field
**
** On entry,
**	*pstr	points to a string containig white space separated
**		field, optionlly quoted.
**
** On exit,
**	*pstr	has been moved to the first delimiter past the
**		field
**		THE STRING HAS BEEN MUTILATED by a 0 terminator
**
**	returns	a pointer to the first field
*/
char * HTNextField (char **pstr)
{
    char * p = *pstr;
    char * start;			/* start of field */
    
    while(*p && WHITE(*p)) p++;		/* Strip white space */
    if (!*p) {
	*pstr = p;
        return NULL;		/* No first field */
    }
    if (*p == '"') {			/* quoted field */
        p++;
	start = p;
	for(;*p && *p!='"'; p++) {
	    if (*p == '\\' && p[1]) p++;	/* Skip escaped chars */
	}
    } else {
	start = p;
	while(*p && !WHITE(*p)) p++;	/* Skip first field */
    }
    if (*p) *p++ = 0;
    *pstr = p;
    return start;
}
