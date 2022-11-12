/* Please read copyright.nsca. Don't remove next line */
#include "copyright.ncsa"

/* Copyright (C) 1997 - G.Dauphin
 * Please read "license.mMosaic" too.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"

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
	{NULL, '\0'}
};

static MarkType ParseMarkType(char *str);


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
		int good_term;

/* Extract the escape sequence from start to ptr */
		start = ptr;
		good_term = 0;
		while (((*ptr != ';') && isalnum(*ptr) && (*ptr != '\0')) ||
		       (*ptr == '#') || (*ptr == '&' ) ) {
			ptr++;
		}
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
		if ( *ptr == ';')
			good_term = 1;
/* Replace escape sequence with appropriate character */
		tchar = *ptr;
		*ptr = '\0';
		val = ExpandEscapes(start);
		*ptr = tchar;
		if (val != '\0') {
			*ptr2 = val;
			if (!good_term)
				ptr--;
		} else { 	/* invalid escape sequence. skip it.  */
/*			fprintf(stderr, "Error bad & string\n"); */
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
/*LF preceed a END MARK is ignore */
	if ( (*ptr == '<') && (*(ptr-1) == '\n') && ( *(ptr+1) == '/') ) {
/*		*(ptr-1) = '\0' ; */
		len--;
		if (len == 0)
			return NULL;
	}
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
	char *text;
	struct mark_up *mark = NULL;
	struct mark_up *list = NULL;
	struct mark_up *current = NULL;
	int is_white = 0;		/* is a white text ? */

	if (str == NULL)
		return(NULL);
	start = str;
	end = str;
	while (*start != '\0') {

		text = get_text(start, &end, &is_white);
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
		if ( !mark->is_end) {
/* A linefeed immediately after <MARQUEUR> mark is to be ignored. */
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
	if (!strcasecmp(str, MT_ANCHOR)) {
		type = M_ANCHOR;
	} else if (!strcasecmp(str, MT_FRAME)) {
		type = M_FRAME;
	} else if (!strcasecmp(str, MT_TITLE)) {
		type = M_TITLE;
	} else if (!strcasecmp(str, MT_FIXED)) {
		type = M_FIXED;
	} else if (!strcasecmp(str, MT_BOLD)) {
		type = M_BOLD;
	} else if (!strcasecmp(str, MT_ITALIC)) {
		type = M_ITALIC;
	} else if (!strcasecmp(str, MT_EMPHASIZED)) {
		type = M_EMPHASIZED;
	} else if (!strcasecmp(str, MT_STRONG)) {
		type = M_STRONG;
	} else if (!strcasecmp(str, MT_CODE)) {
		type = M_CODE;
	} else if (!strcasecmp(str, MT_SAMPLE)) {
		type = M_SAMPLE;
	} else if (!strcasecmp(str, MT_KEYBOARD)) {
		type = M_KEYBOARD;
	} else if (!strcasecmp(str, MT_VARIABLE)) {
		type = M_VARIABLE;
	} else if (!strcasecmp(str, MT_CITATION)) {
		type = M_CITATION;
	} else if (!strcasecmp(str, MT_STRIKEOUT)) {
		type = M_STRIKEOUT;
	} else if (!strcasecmp(str, MT_HEADER_1)) {
		type = M_HEADER_1;
	} else if (!strcasecmp(str, MT_HEADER_2)) {
		type = M_HEADER_2;
	} else if (!strcasecmp(str, MT_HEADER_3)) {
		type = M_HEADER_3;
	} else if (!strcasecmp(str, MT_HEADER_4)) {
		type = M_HEADER_4;
	} else if (!strcasecmp(str, MT_HEADER_5)) {
		type = M_HEADER_5;
	} else if (!strcasecmp(str, MT_HEADER_6)) {
		type = M_HEADER_6;
	} else if (!strcasecmp(str, MT_ADDRESS)) {
		type = M_ADDRESS;
	} else if (!strcasecmp(str, MT_PARAGRAPH)) {
		type = M_PARAGRAPH;
	} else if (!strcasecmp(str, MT_UNUM_LIST)) {
		type = M_UNUM_LIST;
	} else if (!strcasecmp(str, MT_NUM_LIST)) {
		type = M_NUM_LIST;
	} else if (!strcasecmp(str, MT_MENU)) {
		type = M_MENU;
	} else if (!strcasecmp(str, MT_DIRECTORY)) {
		type = M_DIRECTORY;
	} else if (!strcasecmp(str, MT_LIST_ITEM)) {
		type = M_LIST_ITEM;
	} else if (!strcasecmp(str, MT_DESC_LIST)) {
		type = M_DESC_LIST;
	} else if (!strcasecmp(str, MT_DESC_TITLE)) {
		type = M_DESC_TITLE;
	} else if (!strcasecmp(str, MT_DESC_TEXT)) {
		type = M_DESC_TEXT;
	} else if (!strcasecmp(str, MT_PREFORMAT)) {
		type = M_PREFORMAT;
	} else if (!strcasecmp(str, MT_BLOCKQUOTE)) {
		type = M_BLOCKQUOTE;
	} else if (!strcasecmp(str, MT_INDEX)) {
		type = M_INDEX;
	} else if (!strcasecmp(str, MT_HRULE)) {
		type = M_HRULE;
	} else if (!strcasecmp(str, MT_BASE)) {
		type = M_BASE;
	} else if (!strcasecmp(str, MT_LINEBREAK)) {
		type = M_LINEBREAK;
	} else if (!strcasecmp(str, MT_IMAGE)) {
		type = M_IMAGE;
	} else if (!strcasecmp(str, MT_SELECT)) {
		type = M_SELECT;
	} else if (!strcasecmp(str, MT_OPTION)) {
		type = M_OPTION;
	} else if (!strcasecmp(str, MT_INPUT)) {
		type = M_INPUT;
	} else if (!strcasecmp(str, MT_TEXTAREA)) {
		type = M_TEXTAREA;
	} else if (!strcasecmp(str, MT_FORM)) {
		type = M_FORM;
	} else if (!strcasecmp(str, MT_SUP)) {
		type = M_SUP;
	} else if (!strcasecmp(str, MT_SUB)) {
		type = M_SUB;
	} else if (!strcasecmp(str, MT_DOC_HEAD)) {
	        type = M_DOC_HEAD;
	} else if (!strcasecmp(str, MT_UNDERLINED)) {
	        type = M_UNDERLINED;
	} else if (!strcasecmp(str, MT_DOC_BODY)) {
	        type = M_DOC_BODY;
	} else if (!strcasecmp(str, MT_TABLE)) {
		type = M_TABLE;
	} else if (!strcasecmp(str, MT_CAPTION)) {
		type = M_CAPTION;
	} else if (!strcasecmp(str, MT_TABLE_ROW)) {
		type = M_TABLE_ROW;
	} else if (!strcasecmp(str, MT_TABLE_HEADER)) {
		type = M_TABLE_HEADER;
	} else if (!strcasecmp(str, MT_TABLE_DATA)) {
		type = M_TABLE_DATA;
	} else if (!strcasecmp(str, MT_APROG)){
		type = M_APROG;
	} else if (!strcasecmp(str, MT_APPLET)){
		type = M_APPLET;
	} else if (!strcasecmp(str, MT_PARAM)){
		type = M_PARAM;
	} else if (!strcasecmp(str, MT_HTML)){
		type = M_HTML;
	} else if (!strcasecmp(str, MT_CENTER)){
		type = M_CENTER;
	} else if (!strcasecmp(str, MT_DOCTYPE)){
		type = M_DOCTYPE;
	} else if (!strcasecmp(str, MT_BIG)){
		type = M_BIG;
	} else if (!strcasecmp(str, MT_SMALL)){
		type = M_SMALL;
	} else if (!strcasecmp(str, MT_FONT)){
		type = M_FONT;
	} else if (!strcasecmp(str, MT_MAP)){
		type = M_MAP;
	} else if (!strcasecmp(str, MT_AREA)){
		type = M_AREA;
	} else if (!strcasecmp(str, MT_META)){
		type = M_META;
	} else {
		fprintf(stderr, "warning: unknown mark (%s)\n", str);
		type = M_UNKNOWN;
	}
	*tptr = tchar;
	return(type);
}

/* Parse a single anchor tag.  ptrp is a pointer to a pointer to the
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
		if (!strcasecmp(start, mtag)) {
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

/* #### remember */
 
/* HTMLescapeString ()
   Expects: str -- String to escape
            buf -- Buffer to store escaped string
   Returns: nothing

   Escapes all <'s and >'s and ...
*/
/*
void HTMLescapeString (char *str, char *buf)
{

  while (str && *str) {

    switch (*str) {

    case '<':
      *buf = '&'; buf++; *buf = 'l'; buf++;
      *buf = 't'; buf++; *buf = ';'; buf++;
      break;

    case '>':
      *buf = '&'; buf++; *buf = 'g'; buf++;
      *buf = 't'; buf++; *buf = ';'; buf++;
      break;

    case '&':
      *buf = '&'; buf++; *buf = 'a'; buf++;
      *buf = 'm'; buf++; *buf = 'p'; buf++;
      *buf = ';'; buf++;
      break;

    default:
      *buf = *str;
      buf++;
    }
    str++;
  }
  *buf = 0;
}
*/

