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

/*			Parse WAIS Source file			HTWSRC.c
**
**	This module parses a stream with WAIS source file
**	format information on it and creates a structured stream.
**	That structured stream is then converted into whatever.
**
**	3 June 93	Bug fix: Won't crash if no description
*/

const char * hex = "0123456789ABCDEF";

/*	Decode one hex character */

char from_hex (char c)
{
    return 		  (c>='0')&&(c<='9') ? c-'0'
			: (c>='A')&&(c<='F') ? c-'A'+10
			: (c>='a')&&(c<='f') ? c-'a'+10
			:		       0;
}

static unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{    0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,   /* 2x   !"#$%&'()*+,-./  */
     1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,   /* 3x  0123456789:;<=>?  */
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   /* 4x  @ABCDEFGHIJKLMNO  */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,   /* 5x  PQRSTUVWXYZ[\]^_  */
     0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,   /* 6x  `abcdefghijklmno  */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 }; /* 7x  pqrstuvwxyz{\}~  DEL */

#define HT_HEX(i) (i < 10 ? '0'+i : 'A'+ i - 10)

/* The string returned from here, if any, can be free'd by caller. */
static char *HTEscape (char *part)
{
  char *q;
  char *p;              /* Pointers into keywords */
  char *escaped;
 
  if (!part)
    return NULL;
 
  escaped = (char *)malloc (strlen (part) * 3 + 1);
  
  for (q = escaped, p = part; *p != '\0'; p++) {
      int c = (int)((unsigned char)(*p));
      if (c >= 32 && c <= 127 && isAcceptable[c-32]) {
          *q++ = *p;                   
        } else {                       
          *q++ = '%';                  
          *q++ = HT_HEX(c / 16);       
          *q++ = HT_HEX(c % 16);       
        }                              
    }                                  
  *q=0;                                
  return escaped;                      
}                                      
