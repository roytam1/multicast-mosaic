/* Please read copyright.nsca. Don't remove next line */
#include "copyright.ncsa"

/* Copyright (C) 1997 - G.Dauphin
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"

/* A hack to speed up caseless_equal.  Thanks to Quincey Koziol for
 * developing it for me
 */
static unsigned char map_table[256]={
    0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,
    24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,
    45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,97,98,
    99,100,101,102,103,104,105,106,107,108,109,110,111,112,113,114,115,
    116,117,118,119,120,121,122,91,92,93,94,95,96,97,98,99,100,101,102,
    103,104,105,106,107,108,109,110,111,112,113,114,115,116,117,118,119,
    120,121,122,123,124,125,126,127,128,129,130,131,132,133,134,135,136,
    137,138,139,140,141,142,143,144,145,146,147,148,149,150,151,152,153,
    154,155,156,157,158,159,160,161,162,163,164,165,166,167,168,169,170,
    171,172,173,174,175,176,177,178,179,180,181,182,183,184,185,186,187,
    188,189,190,191,192,193,194,195,196,197,198,199,200,201,202,203,204,
    205,206,207,208,209,210,211,212,213,214,215,216,217,218,219,220,221,
    222,223,224,225,226,227,228,229,230,231,232,233,234,235,236,237,238,
    239,240,241,242,243,244,245,246,247,248,249,250,251,252,253,254,255};

#define TOLOWER(x)	(map_table[x])

typedef struct amp_esc_rec {
	char *tag;
	char value;
} AmpEsc;

static AmpEsc AmpEscapes[] = {
	{"lt", '<'},
	{"LT", '<'},
	{"gt", '>'},
	{"GT", '>'},
	{"amp", '&'},
	{"AMP", '&'},
	{"quot", '\"'},
	{"QUOT", '\"'},
	{"nbsp", NBSP_CONST},
	{"iexcl", '\241'},
	{"cent", '\242'},
	{"pound", '\243'},
	{"curren", '\244'},
	{"yen", '\245'},
	{"brvbar", '\246'},
	{"sect", '\247'},
	{"uml", '\250'},
	{"copy", '\251'},
	{"ordf", '\252'},
	{"laquo", '\253'},
	{"not", '\254'},
	{"shy", '\255'},
	{"reg", '\256'},
	{"macr", '\257'},
	{"hibar", '\257'},
	{"deg", '\260'},
	{"plusmn", '\261'},
	{"sup2", '\262'},
	{"sup3", '\263'},
	{"acute", '\264'},
	{"micro", '\265'},
	{"para", '\266'},
	{"middot", '\267'},
	{"cedil", '\270'},
	{"sup1", '\271'},
	{"ordm", '\272'},
	{"raquo", '\273'},
	{"frac14", '\274'},
	{"frac12", '\275'},
	{"frac34", '\276'},
	{"iquest", '\277'},
	{"Agrave", '\300'},
	{"Aacute", '\301'},
	{"Acirc", '\302'},
	{"Atilde", '\303'},
	{"Auml", '\304'},
	{"Aring", '\305'},
	{"AElig", '\306'},
	{"Ccedil", '\307'},
	{"Egrave", '\310'},
	{"Eacute", '\311'},
	{"Ecirc", '\312'},
	{"Euml", '\313'},
	{"Igrave", '\314'},
	{"Iacute", '\315'},
	{"Icirc", '\316'},
	{"Iuml", '\317'},
	{"ETH", '\320'},
	{"Ntilde", '\321'},
	{"Ograve", '\322'},
	{"Oacute", '\323'},
	{"Ocirc", '\324'},
	{"Otilde", '\325'},
	{"Ouml", '\326'},
	{"times", '\327'}, /* ? */
	{"Oslash", '\330'},
	{"Ugrave", '\331'},
	{"Uacute", '\332'},
	{"Ucirc", '\333'},
	{"Uuml", '\334'},
	{"Yacute", '\335'},
	{"THORN", '\336'},
	{"szlig", '\337'},
	{"agrave", '\340'},
	{"aacute", '\341'},
	{"acirc", '\342'},
	{"atilde", '\343'},
	{"auml", '\344'},
	{"aring", '\345'},
	{"aelig", '\346'},
	{"ccedil", '\347'},
	{"egrave", '\350'},
	{"eacute", '\351'},
	{"ecirc", '\352'},
	{"euml", '\353'},
	{"igrave", '\354'},
	{"iacute", '\355'},
	{"icirc", '\356'},
	{"iuml", '\357'},
	{"eth", '\360'},
	{"ntilde", '\361'},
	{"ograve", '\362'},
	{"oacute", '\363'},
	{"ocirc", '\364'},
	{"otilde", '\365'},
	{"ouml", '\366'},
	{"divide", '\367'}, /* ? */
	{"oslash", '\370'},
	{"ugrave", '\371'},
	{"uacute", '\372'},
	{"ucirc", '\373'},
	{"uuml", '\374'},
	{"yacute", '\375'},
	{"thorn", '\376'},
	{"yuml", '\377'},
	{NULL, '\0'},
};

static MarkType ParseMarkType(char *str);

/* Check if two strings are equal, ignoring case. The strings must be of the
 * same length to be equal. return 1 if equal, 0 otherwise.
 */
int caseless_equal(char *str1, char *str2)
{
	if ((str1 == NULL)||(str2 == NULL))
		return(0);
	while ( *str1 && *str2 ) {
		if (TOLOWER(*str1) != TOLOWER(*str2))
			return(0);
		str1++;
		str2++;
	}
	if ((*str1 == '\0') && (*str2 == '\0'))
		return(1);
	return(0);
}

#if 0
/* Check if two strings are equal in the first count characters, ignoring case.
 * The strings must both be at least of length count to be equal.
 * return 1 if equal, 0 otherwise.
 */
static int caseless_equal_prefix(char *str1, char *str2, int cnt)
{
	int i;

	if ((str1 == NULL)||(str2 == NULL))
		return(0);
	if (cnt < 1)
		return(1);
	for (i=0; i < cnt; i++) {
		if (TOLOWER(*str1) != TOLOWER(*str2))
			return(0);
		str1++;
		str2++;
	}
	return(1);
}
#endif

/* Clean up the white space in a string. Remove all leading and trailing
 * whitespace, and turn all internal whitespace into single spaces separating
 * words. The cleaning is done by rearranging the chars in the passed txt buffer.
 * The resultant string will probably be shorter, it can never get longer.
 */
void clean_white_space(char *txt)
{
	char *ptr;
	char *start;

	start = txt;
	ptr = txt;
	while (isspace((int)*ptr))  /* Remove leading white space */
		ptr++;
	if(start == ptr){/*find a word, copying if we removed some space already*/
		while ((!isspace((int)*ptr))&&(*ptr != '\0'))
			ptr++;
		start = ptr;
	} else {
		while ((!isspace((int)*ptr))&&(*ptr != '\0'))
			*start++ = *ptr++;
	}
	while (*ptr != '\0') {
		while (isspace((int)*ptr))  /* Remove trailing whitespace.  */
			ptr++;
		if (*ptr == '\0')
			break;
/* If there are more words, insert a space and if space was 
 * removed move up remaining text. */
		*start++ = ' ';
		if (start == ptr) {
			while ((!isspace((int)*ptr))&&(*ptr != '\0'))
				ptr++;
			start = ptr;
		} else {
			while ((!isspace((int)*ptr))&&(*ptr != '\0'))
				*start++ = *ptr++;
		}
	}
	*start = '\0';
}

/* parse an amperstand escape, and return the appropriate character, or '\0' on
 * error. Turns out the escapes are case sensitive, use strncmp.
 */
static char ExpandEscapes(char *esc)
{               
        int cnt;        
        char val;               
        char *endc;             
        int escLen, ampLen;     
                                
        esc++;          
        if (*esc == '#') {
                val = (char)strtol((esc + 1),&endc,10);
                if ( *endc != '\0') 
                        return 0;
                return val; 
        }               
/* other case */ 
        cnt = 0;
        escLen = strlen(esc);
        while (AmpEscapes[cnt].tag != NULL) {
                ampLen = strlen(AmpEscapes[cnt].tag);
                if ((escLen == ampLen) &&
                   (strncmp(esc, AmpEscapes[cnt].tag, ampLen) == 0)) {
                        val = AmpEscapes[cnt].value;
                        break;  
                }       
                cnt++;
        }
        if (AmpEscapes[cnt].tag == NULL)
                return 0;
        return(val);
}

/* Clean the special HTML character escapes out of the text and replace
 * them with the appropriate characters "&lt;" = "<", "&gt;" = ">",
 * "&amp;" = "&"
 * Ok, better, they have to be terminated with ';'.
 * the '&' character must be immediately followed by a letter to be
 * a valid escape sequence.  Other &'s are left alone.
 * The cleaning is done by rearranging chars in the passed txt buffer.
 * if any escapes are replaced, the string becomes shorter.
 * We stop the nightmare: strict conformance to HTML3.2. I understand
 * '<' MUST be ';' terminated.
 */
static void clean_text(char *txt)
{
	char *ptr;
	char *ptr2;
	char *start;
	char tchar;
	char val;

	if (txt == NULL)
		return;
/* Quick scan to find escape sequences. Escape is '&' followed by a letter
 * (or a hash mark). return if there are none.
 */
	ptr = txt;
	while (*ptr != '\0') {
		if ((*ptr == '&')&&
			((isalpha((int)*(ptr + 1)))||(*(ptr + 1) == '#')))
				break;
		ptr++;
	}
	if (*ptr == '\0')
		return;
/* Loop, replaceing escape sequences, and moving up remaining text. */
	ptr2 = ptr;
	while (*ptr != '\0') {
/* Extract the escape sequence from start to ptr */
		start = ptr;
		while ((*ptr != ';') && (*ptr != '\0'))
			ptr++;
		if (*ptr == '\0') {
			fprintf(stderr,"warning: unterminated & (%s)\n", start);
			ptr = start;
			*ptr2++ = *ptr++;
/* Copy forward remaining text until next escape sequence */
			while (*ptr != '\0') {
				if ((*ptr == '&')&&
				    ((isalpha((int)*(ptr + 1)))||(*(ptr + 1) == '#')))
					break;
				*ptr2++ = *ptr++;
			}
			continue;
		} 
/* ptr is on ';' */
/* Replace escape sequence with appropriate character */
		tchar = *ptr;
		*ptr = '\0';
		val = ExpandEscapes(start);
		*ptr = tchar;
		if (val != '\0') {
			*ptr2 = val;
		} else { 	/* invalid escape sequence. skip it.  */
			fprintf(stderr, "Error bad & string\n");
			ptr = start;
			*ptr2 = *ptr;
		}
/* Copy forward remaining text until  next escape sequence */
		ptr2++;
		ptr++;
		while (*ptr != '\0') {
			if ((*ptr == '&')&&
			    ((isalpha((int)*(ptr + 1)))||(*(ptr + 1) == '#')))
				break;
			*ptr2++ = *ptr++;
		}
	}
	*ptr2 = '\0';
}

/* Get a block of text from a HTML document. All text from start to the end,
 * or the first mark (a mark is '<' followed by any char)
 * is returned in a malloced buffer.  Also, endp returns a pointer to the
 * next '<' or '\0'. The returned text has already expanded '&' escapes.
 */
static char * get_text(char *start, char **endp, int * is_white)
{
	char *ptr;
	char *text;
	char tchar;
	int len;

	*is_white = 1;
	len = 0;
/* Copy text up to beginning of a mark, or the end */
	ptr = start;
	while (*ptr != '\0') {
		if (*ptr == '<')
			break;
		if (! isspace(*ptr))
			*is_white = 0;
		ptr++;
		len++;
	}
	*endp = ptr;
	if (ptr == start)
		return(NULL);
/* Copy the text into its own buffer, and clean it of escape sequences. */
	text = (char *)malloc(len + 1);
	CHECK_OUT_OF_MEM(text);
	strncpy(text, start, len);
	text[len] = '\0';
	clean_text(text);
	return(text);
}

/*
 * Get the mark text between '<' and '>'.  From the text, determine
 * its type, and fill in a mark_up structure to return.  Also returns
 * endp pointing to the ttrailing '>' in the original string.
 */
static struct mark_up * get_mark(char *start, char **endp)
{
	char *ptr;
	char *text;
	char tchar;
	struct mark_up *mark;
	int  comment=0;       /* amb - comment==1 if we are in a comment */
	char *first_gt=NULL;  /* keep track of ">" for old broken comments */

	if (start == NULL)
		return(NULL);
	if (*start != '<')
		return(NULL);
/* amb - check if we are in a comment, start tag is <!-- */
	if (strncmp (start, "<!--", 4)==0)
		comment=1;
	start++;
	mark = (struct mark_up *)malloc(sizeof(struct mark_up));
	CHECK_OUT_OF_MEM(mark);
	ptr = start; 	/* Grab the mark text */

	/* amb - skip over the comment text */
	/* end tag is --*>, where * is zero or more spaces (ugh) */
	if (comment) {
		while (*ptr != '\0') {
			if ( (*ptr == '>') && (!first_gt) )
				first_gt = ptr;
			if(strncmp(ptr,"--",2)==0){ /*found double dash(--)*/
				ptr += 2;
				while((*ptr != '\0') && ((*ptr == ' ') || 
				     (*ptr == '\n') || (*ptr == '-') ))
					ptr++;    /* skip spaces and newlines */ 
				if (*ptr == '>'){  /* completed end comment */
					*endp = ptr;
					mark->is_end = 1;
					mark->type = M_COMMENT;
					mark->start = NULL;
					mark->text = NULL;
					mark->end = NULL;
					mark->next = NULL;
					return(mark);
				}
			} else   /* if no double dash (--) found */
				ptr++;
		} /* if we get here, this document must use the old broken
		   *	comment style */
		if(first_gt){
			ptr = first_gt;
		}
	} /* end of: if (comment) */

	while (*ptr && (*ptr != '>') )
		ptr++;
	if (*ptr)		/* is on '>' */
		*endp=ptr;
	else {
		free(mark);
		return(NULL); /*only if EOF and no close -- SWP*/
	}

/* Copy the mark text to its own buffer, and
 * clean it of escapes, and odd white space.
 */
	tchar = *ptr;
	*ptr = '\0';
	text = (char *)malloc(strlen(start) + 1);
	CHECK_OUT_OF_MEM(text);
	strcpy(text, start);
	*ptr = tchar;
	clean_text(text);

/* Set whether this is the start or end of a mark
 * block, as well as determining its type.
 */
	if (*text == '/') {
		mark->is_end = 1;
		mark->type = ParseMarkType((char *)(text + 1));
		mark->start = NULL;
		mark->text = NULL;
		mark->end = text;
	} else {
		mark->is_end = 0;
		mark->type = ParseMarkType(text);
		mark->start = text;
		mark->text = NULL;
		mark->end = NULL;
	}
	mark->text = NULL;
	mark->next = NULL;
	return(mark);
}

/* Special version of get_text.  It reads all text up to the
 * end of the plain text mark, or the end of the file.
 */
static char * get_plain_text(char *start, char **endp)
{
	char *ptr;
	char *text;
	char tchar;

	if (start == NULL)
		return(NULL);
/** Read until stopped by end plain text mark. */
	ptr = start;
	while (*ptr != '\0') {
/* Beginning of a mark is '<' followed by any letter, or followed by '!' for
 * a comment, or '</' followed by any letter.
 */
		if ((*ptr == '<')&&
		    (isalpha((int)(*(ptr + 1))) ||
		     (*(ptr + 1) == '!') ||
		     ((*(ptr + 1) == '/')&&(isalpha((int)(*(ptr + 2))))))) {
			struct mark_up *mp;
			char *ep;

/* We think we found a mark.  If it is the end of plain text, break out */
			mp = get_mark(ptr, &ep);
			if (mp != NULL) {
				if ((mp->type == M_PLAIN_TEXT) && (mp->is_end)) {
					if (mp->end != NULL)
						free((char *)mp->end);
					free((char *)mp);
					break;
				}
				if (mp->start != NULL)
					free((char *)mp->start);
				if (mp->end != NULL)
					free((char *)mp->end);
				free((char *)mp);
			}
		}
		ptr++;
	}
	*endp = ptr;
	if (ptr == start)
		return(NULL);
/* Copy text to its own malloced buffer, and clean it of HTML escapes. */
	tchar = *ptr;
	*ptr = '\0';
	text = (char *)malloc(strlen(start) + 1);
	CHECK_OUT_OF_MEM(text);
	strcpy(text, start);
	*ptr = tchar;
	clean_text(text);
	return(text);
}

/* Add an object to the parsed object list. Return a pointer to the
 * current (end) position in the list. If the object is a normal text object
 * containing nothing but white space, throw it out, unless we have been
 * told to keep white space.
 */
static struct mark_up * AddObj( struct mark_up **listp, struct mark_up *current,
	struct mark_up *mark)
{
	if (mark == NULL)
		return(current);

/* Add object to either the head of the list for a new list,
 * or at the end after the current pointer.
 */
	if (*listp == NULL) {
		*listp = mark;
		current = *listp;
	} else {
		current->next = mark;
		current = current->next;
	}
	current->next = NULL;
	return(current);
}

/* Main parser of HTML text.  Takes raw text, and produces a linked
 * list of mark objects.  Mark objects are either text strings, or
 * starting and ending mark delimiters.
 * The old list is passed in so it can be freed, and in the future we
 * may want to add code to append to the old list.
 */
struct mark_up * HTMLParse( char *str)
{
	char *start, *end;
	char *text, *tptr;
	struct mark_up *mark = NULL;
	struct mark_up *list = NULL;
	struct mark_up *current = NULL;
	int is_white = 0;		/* is a white text ? */

	if (str == NULL)
		return(NULL);
	start = str;
	end = str;
	while (*start != '\0') {

/* Get some text (if any).  If our last mark was a begin plain text we call
 * different function. If last mark was <PLAINTEXT> we lump all the rest of
 * the text in. */

		if((mark!=NULL) && (mark->type==M_PLAIN_FILE)&& (!mark->is_end)) {
			text = start;
			end = text;
			while (*end != '\0')
				end++;
/* Copy text to its own malloced buffer, and clean it of HTML escapes. */
			tptr = (char *)malloc(strlen(text) + 1);
			CHECK_OUT_OF_MEM(tptr);
			strcpy(tptr, text);
			text = tptr;
		} else {
			if ((mark != NULL)&& (mark->type == M_PLAIN_TEXT)&& 
			    (!mark->is_end)) {
				is_white = 0;
				text = get_plain_text(start, &end);
			} else {
				text = get_text(start, &end, &is_white);
			}
		}
/* If text is OK, put it into a mark structure, and add it to the linked list. */
		if (text ) {
			mark = (struct mark_up *)malloc(sizeof(struct mark_up));
			CHECK_OUT_OF_MEM(mark);
			mark->type = M_NONE;	/* it's a text */
			mark->is_end = 0;
			mark->start = NULL;
			mark->text = text;
			mark->is_white_text = is_white;
			mark->end = NULL;
			mark->next = NULL;
			mark->s_aps = NULL;
			mark->s_ats = NULL;
			mark->s_picd = NULL;
			mark->t_p1 = NULL;
			mark->anc_name = NULL;
			mark->anc_href = NULL;
			mark->anc_title = NULL;
			current = AddObj(&list, current, mark);
		}
/* end is on '<' or '\0' */
		start = end;
		if (*start == '\0')
			break;		/* end html string, parse is done */

/* Get the next mark if any, and if it is valid, add it to the linked list. */
/* star is on '<' */
		mark = get_mark(start, &end);
		if (mark == NULL) {
			fprintf(stderr, "error parsing mark, missing '>'\n");
			return(list);
		}
/* end is on '>' */

		mark->is_white_text = is_white = 0;
		mark->next = NULL;
		mark->s_aps = NULL;
		mark->s_ats = NULL;
		mark->s_picd = NULL;
		mark->t_p1 = NULL;
		mark->anc_name = NULL;
		mark->anc_href = NULL;
		mark->anc_title = NULL;
		current = AddObj(&list, current, mark);
		start = (char *)(end + 1);
/* start is a pointer after the '>' character */
		if ((mark != NULL)&&
		    (mark->type == M_PLAIN_FILE || 
		     mark->type == M_PLAIN_TEXT || mark->type == M_PREFORMAT) &&
		    (!mark->is_end)) {
/* A linefeed immediately after the <PLAINTEXT> mark is to be ignored. */
/* A linefeed immediately after the <XMP> mark is to be ignored. */
/* A linefeed immediately after <PRE> mark is to be ignored. */
			if (*start == '\n')
				start++;
		} 
	}
	return(list);
}

/* Determine mark type from the identifying string passed */

static MarkType ParseMarkType(char *str)
{
	MarkType type;
	char *tptr;
	char tchar;

	if (str == NULL)
		return(M_NONE);
	type = M_UNKNOWN;
	tptr = str;
	while (*tptr != '\0') {
		if (isspace((int)*tptr))
			break;
		tptr++;
	}
	tchar = *tptr;
	*tptr = '\0';
	if (caseless_equal(str, MT_ANCHOR)) {
		type = M_ANCHOR;
	} else if (caseless_equal(str, MT_FRAME)) {
		type = M_FRAME;
	} else if (caseless_equal(str, MT_TITLE)) {
		type = M_TITLE;
	} else if (caseless_equal(str, MT_FIXED)) {
		type = M_FIXED;
	} else if (caseless_equal(str, MT_BOLD)) {
		type = M_BOLD;
	} else if (caseless_equal(str, MT_ITALIC)) {
		type = M_ITALIC;
	} else if (caseless_equal(str, MT_EMPHASIZED)) {
		type = M_EMPHASIZED;
	} else if (caseless_equal(str, MT_STRONG)) {
		type = M_STRONG;
	} else if (caseless_equal(str, MT_CODE)) {
		type = M_CODE;
	} else if (caseless_equal(str, MT_SAMPLE)) {
		type = M_SAMPLE;
	} else if (caseless_equal(str, MT_KEYBOARD)) {
		type = M_KEYBOARD;
	} else if (caseless_equal(str, MT_VARIABLE)) {
		type = M_VARIABLE;
	} else if (caseless_equal(str, MT_CITATION)) {
		type = M_CITATION;
	} else if (caseless_equal(str, MT_STRIKEOUT)) {
		type = M_STRIKEOUT;
	} else if (caseless_equal(str, MT_HEADER_1)) {
		type = M_HEADER_1;
	} else if (caseless_equal(str, MT_HEADER_2)) {
		type = M_HEADER_2;
	} else if (caseless_equal(str, MT_HEADER_3)) {
		type = M_HEADER_3;
	} else if (caseless_equal(str, MT_HEADER_4)) {
		type = M_HEADER_4;
	} else if (caseless_equal(str, MT_HEADER_5)) {
		type = M_HEADER_5;
	} else if (caseless_equal(str, MT_HEADER_6)) {
		type = M_HEADER_6;
	} else if (caseless_equal(str, MT_ADDRESS)) {
		type = M_ADDRESS;
	} else if (caseless_equal(str, MT_PLAIN_TEXT)) {
		type = M_PLAIN_TEXT;
	} else if (caseless_equal(str, MT_PLAIN_FILE)) {
		type = M_PLAIN_FILE;
	} else if (caseless_equal(str, MT_PARAGRAPH)) {
		type = M_PARAGRAPH;
	} else if (caseless_equal(str, MT_UNUM_LIST)) {
		type = M_UNUM_LIST;
	} else if (caseless_equal(str, MT_NUM_LIST)) {
		type = M_NUM_LIST;
	} else if (caseless_equal(str, MT_MENU)) {
		type = M_MENU;
	} else if (caseless_equal(str, MT_DIRECTORY)) {
		type = M_DIRECTORY;
	} else if (caseless_equal(str, MT_LIST_ITEM)) {
		type = M_LIST_ITEM;
	} else if (caseless_equal(str, MT_DESC_LIST)) {
		type = M_DESC_LIST;
	} else if (caseless_equal(str, MT_DESC_TITLE)) {
		type = M_DESC_TITLE;
	} else if (caseless_equal(str, MT_DESC_TEXT)) {
		type = M_DESC_TEXT;
	} else if (caseless_equal(str, MT_PREFORMAT)) {
		type = M_PREFORMAT;
	} else if (caseless_equal(str, MT_BLOCKQUOTE)) {
		type = M_BLOCKQUOTE;
	} else if (caseless_equal(str, MT_INDEX)) {
		type = M_INDEX;
	} else if (caseless_equal(str, MT_HRULE)) {
		type = M_HRULE;
	} else if (caseless_equal(str, MT_BASE)) {
		type = M_BASE;
	} else if (caseless_equal(str, MT_LINEBREAK)) {
		type = M_LINEBREAK;
	} else if (caseless_equal(str, MT_IMAGE)) {
		type = M_IMAGE;
	} else if (caseless_equal(str, MT_FIGURE)) {
		type = M_FIGURE;
	} else if (caseless_equal(str, MT_SELECT)) {
		type = M_SELECT;
	} else if (caseless_equal(str, MT_OPTION)) {
		type = M_OPTION;
	} else if (caseless_equal(str, MT_INPUT)) {
		type = M_INPUT;
	} else if (caseless_equal(str, MT_TEXTAREA)) {
		type = M_TEXTAREA;
	} else if (caseless_equal(str, MT_FORM)) {
		type = M_FORM;
	} else if (caseless_equal(str, MT_SUP)) {
                type = M_SUP;
        } else if (caseless_equal(str, MT_SUB)) {
                type = M_SUB;
        } else if (caseless_equal(str, MT_DOC_HEAD)) {
	        type = M_DOC_HEAD;
        } else if (caseless_equal(str, MT_UNDERLINED)) {
	        type = M_UNDERLINED;
        } else if (caseless_equal(str, MT_DOC_BODY)) {
	        type = M_DOC_BODY;
        } else if (caseless_equal(str, MT_TABLE)) {
		type = M_TABLE;
	} else if (caseless_equal(str, MT_CAPTION)) {
		type = M_CAPTION;
	} else if (caseless_equal(str, MT_TABLE_ROW)) {
		type = M_TABLE_ROW;
	} else if (caseless_equal(str, MT_TABLE_HEADER)) {
		type = M_TABLE_HEADER;
	} else if (caseless_equal(str, MT_TABLE_DATA)) {
		type = M_TABLE_DATA;
	} else if (caseless_equal(str, MT_APROG)){
		type = M_APROG;
	} else if (caseless_equal(str, MT_APPLET)){
		type = M_APPLET;
	} else if (caseless_equal(str, MT_PARAM)){
		type = M_PARAM;
	} else if (caseless_equal(str, MT_HTML)){
		type = M_HTML;
	} else if (caseless_equal(str, MT_CENTER)){
		type = M_CENTER;
	} else if (caseless_equal(str, MT_DOCTYPE)){
		type = M_DOCTYPE;
	} else if (caseless_equal(str, MT_BIG)){
		type = M_BIG;
	} else if (caseless_equal(str, MT_SMALL)){
		type = M_SMALL;
	} else if (caseless_equal(str, MT_FONT)){
		type = M_FONT;
	} else if (caseless_equal(str, MT_MAP)){
		type = M_MAP;
	} else if (caseless_equal(str, MT_AREA)){
		type = M_AREA;
	} else if (caseless_equal(str, MT_META)){
		type = M_META;
	} else {
		fprintf(stderr, "warning: unknown mark (%s)\n", str);
		type = M_UNKNOWN;
	}
	*tptr = tchar;
	return(type);
}

/*
 * Parse a single anchor tag.  ptrp is a pointer to a pointer to the
 * string to be parsed.  On return, the ptr should be changed to
 * point to after the text we have parsed.
 * On return start and end should point to the beginning, and just
 * after the end of the tag's name in the original anchor string.
 * Finally the function returns the tag value in a malloced buffer.
 */
static char * AnchorTag( char **ptrp, char **startp, char **endp)
{
	char *tag_val;
	char *ptr;
	char *start;
	char tchar;
	int quoted;
	int has_value;

	quoted = 0;
	ptr = *ptrp; 		/* remove leading spaces, and set start */
	while (isspace((int)*ptr))
		ptr++;
	*startp = ptr;
				/* Find and set the end of the tag */
	while ((!isspace((int)*ptr))&&(*ptr != '=')&&(*ptr != '\0'))
		ptr++;
	*endp = ptr;
        has_value=0;
	if (*ptr == '\0') {
		*ptrp = ptr;
	} else {    	/* Move to the start of tag value, if there is one. */
            while ((isspace((int)*ptr))||(*ptr == '=')) {
		if (*ptr == '=')
                    has_value = 1;
		ptr++;
            }
        }
	/* For a tag with no value, this is a boolean flag.
	 * Return the string "1" so we know the tag is there.
	 */
	if (!has_value) {	/* set a tag value of 1. */
		*ptrp = *endp;
		tag_val = (char *)malloc(strlen("1") + 1);
		CHECK_OUT_OF_MEM(tag_val);
		strcpy(tag_val, "1");
		return(tag_val);
	}
	if (*ptr == '\"') {
		quoted = 1;
		ptr++;
	}
	start = ptr;
	if(quoted) { /* Get tag value.  Either a quoted string or a single word */
		while ((*ptr != '\"')&&(*ptr != '\0'))
			ptr++;
	} else {
		while ((!isspace((int)*ptr))&&(*ptr != '\0'))
			ptr++;
	}

	/* Copy the tag value out into a malloced string */
	tchar = *ptr;
	*ptr = '\0';
	tag_val = (char *)malloc(strlen(start) + 1);
	CHECK_OUT_OF_MEM(tag_val);
	strcpy(tag_val, start);
	*ptr = tchar;

	/* If you forgot the end quote, you need to make sure you aren't
		indexing ptr past the end of its own array -- SWP */
	if (quoted && *ptr!='\0')
		ptr++;
	*ptrp = ptr;
	return(tag_val);
}

/* Parse mark text for the value associated with the passed mark tag.
 * If the passed tag is not found, return NULL.
 * If the passed tag is found but has no value, return "".
 */
char* ParseMarkTag(char *text, char *mtext, char *mtag)
{
	char *ptr;
	char *start;
	char *end;
	char *tag_val;
	char tchar;

	if ((text == NULL)||(mtext == NULL)||(mtag == NULL))
		return(NULL);
	ptr = (char *)(text + strlen(mtext));

	while (*ptr != '\0') {
		tag_val = AnchorTag(&ptr, &start, &end);
		tchar = *end;
		*end = '\0';
		if (caseless_equal(start, mtag)) {
			*end = tchar;
			if (tag_val == NULL) {
				tag_val = (char *)malloc(1);
				*tag_val = '\0';
				return(tag_val);
			}
			return(tag_val);
		}
		*end = tchar;
		if (tag_val != NULL)
			free(tag_val);
	}
	return(NULL);
}
