/* Please read copyright.nsca. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

/* Copyright (C) 1997 - G.Dauphin
 * Please read "license.mMosaic" too.
 */

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <assert.h>

#include "HTMLmiscdefs.h"
#include "HTMLparse.h"

#define DEFAULT_FRAME_MARGIN_HEIGHT   5
#define DEFAULT_FRAME_MARGIN_WIDTH    5

typedef struct amp_esc_rec {
	const char *tag;
	char value;
} AmpEsc;

/* Parser state stack object */  
typedef struct _ParserStack{
        MarkType mtype;			/* current state id */
        struct _ParserStack *next;	/* ptr to next record */
}ParserStack;

typedef struct _ParserContext {
	ParserStack *base_stk;			/* stack base point */
	ParserStack *top_stk; 			/* actual stack */
	int has_head;				/* head seen */
	int has_title;				/* title seen */
	char * title_text;			/* TITLE text */
	struct mark_up * title_mark;		/* TITLE text */
	int has_body;				/* body seen */
	int has_base;				/* base seen */
	int has_frameset;			/* frameset seen */
	int frameset_depth;			/* nested frameset */
	int nframes;				/* number of frame (at top level)*/
	struct mark_up * frameset_mark;		/* FIRST <FRAMESET> */
	struct mark_up * object_start;		/* object begin */

	struct mark_up *map_start;
	int n_area;

	MapRec ** map_table;		/* table of map */
	int map_count;			/* number of map */
} ParserContext;

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
static void AddMap(ParserContext *pc, struct mark_up *map_start, int area_count);

static void FreeMarkup( struct mark_up *mptr)
{
	if (mptr->text)
		free(mptr->text);
	if (mptr->start)
		free(mptr->start);
	if (mptr->pcdata)
		free(mptr->pcdata);
        if (mptr->end != NULL)
		free((char *)mptr->end);
	if ( (!mptr->is_end) && (mptr->type == M_ANCHOR) ) {
		if (mptr->anc_name)
			free(mptr->anc_name);
		if (mptr->anc_href)
			free(mptr->anc_href);
		if (mptr->anc_title)
			free(mptr->anc_title);
	}
/*	if (mptr->t_p1){       */ /* table */
/*		_FreeTableStruct(mptr->t_p1); */
/*	} */
/* #######
/*       if(mptr->s_picd) /* mptr->type == M_IMAGE */
/*    {HTMLWidget wid = (HTMLWidget) (win?win->scrolled_win:NULL);
/*         FreeImageInfo(wid, mptr->s_picd);
/*    }
/*####### */
	free(mptr);
}

/* Free up the passed linked list of parsed elements, freeing
 * all memory associates with each element.
 */
static void FreeMarkUpList(struct mark_up *List)
{
        struct mark_up *current;
        struct mark_up *mptr;
 
        current = List;
        while (current != NULL) {
                mptr = current;
                current = current->next;
                mptr->next = NULL;
		FreeMarkup(mptr);
        }
}


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
	char val=0;
	char *endc;
	int escLen, ampLen;
	int base = 10;

	esc++;
	if (*esc == '#') {	/* HTML4.0 page 41 */
		esc++;
		if( *esc == 'x' || *esc == 'X') {
			base = 16;
			esc++;
		}
		val = (char)strtol(esc ,&endc,base);
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
#ifdef HTMLTRACE
			fprintf(stderr,"warning: unterminated & (%s)\n", start);
#endif
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
	mark = (struct mark_up *)calloc(1,sizeof(struct mark_up));
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
	*ptr = '\000';
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
		mark->end = text;
	} else {
		mark->is_end = 0;
		mark->type = ParseMarkType(text);
		mark->start = text;
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
		current = mark;
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
struct mark_up * HTMLLexem( const char *str_in)
{
	char *start, *end;
	char *text, *str;
	struct mark_up *mark = NULL;
	struct mark_up *list = NULL;
	struct mark_up *current = NULL;
	int is_white = 0;		/* is a white text ? */

	if (str_in == NULL)
		return(NULL);
/* Patch From: szukw000@mail.Uni-Mainz.de 29 feb 2000 */
/* copy str_in , work space */
	str = strdup(str_in);
	start = str;
	end = str;
	while (*start != '\0') {

		text = get_text(start, &end, &is_white);
/* If text is OK, put it into a mark structure, and add it to the linked list. */
		if (text ) {
			mark = (struct mark_up *)calloc(1,sizeof(struct mark_up));
			CHECK_OUT_OF_MEM(mark);
			mark->type = M_NONE;	/* it's a text */
			mark->is_end = 0;
			mark->start = NULL;
			mark->text = text;
			mark->is_white_text = is_white;
			mark->end = NULL;
			mark->next = NULL;
			mark->s_picd = NULL;
			mark->t_p1 = NULL;
			mark->anc_name = NULL;
			mark->anc_href = NULL;
			mark->anc_title = NULL;
			mark->pcdata = NULL;
			mark->start_obj = NULL;
			mark->end_obj = NULL;
			mark->try_next_obj = NULL;
			current = AddObj(&list, current, mark);
		}
/* end is on '<' or '\0' */
		start = end;
		if (*start == '\0')
			break;		/* end html string, parse is done */

/* Get the next mark if any, and if it is valid, add it to the linked list. */
/* start is on '<' */
		mark = get_mark(start, &end);
		if (mark == NULL) {
#ifdef HTMLTRACE
			fprintf(stderr, "error parsing mark, missing '>'\n");
#endif
			mark = (struct mark_up *)calloc(1,sizeof(struct mark_up));
			CHECK_OUT_OF_MEM(mark);
			mark->type = M_END_STATE;	/* it's finish */
			mark->is_end = 0;
			mark->start = NULL;
			mark->text = NULL;
			mark->is_white_text = 0;
			mark->end = NULL;
			mark->next = NULL;
			mark->s_picd = NULL;
			mark->t_p1 = NULL;
			mark->anc_name = NULL;
			mark->anc_href = NULL;
			mark->anc_title = NULL;
			mark->pcdata = NULL;
			mark->start_obj = NULL;
			mark->end_obj = NULL;
			mark->try_next_obj = NULL;
			current = AddObj(&list, current, mark);
			free(str);
			return(list);
		}
/* end is on '>' */

		mark->is_white_text = is_white = 0;
		mark->next = NULL;
		mark->s_picd = NULL;
		mark->t_p1 = NULL;
		mark->anc_name = NULL;
		mark->anc_href = NULL;
		mark->anc_title = NULL;
		mark->pcdata = NULL;
		mark->start_obj = NULL;
		mark->end_obj = NULL;
		mark->try_next_obj = NULL;
		current = AddObj(&list, current, mark);
		start = (char *)(end + 1);
/* start is a pointer after the '>' character */
		if ( !mark->is_end) {
/* A linefeed immediately after <MARQUEUR> mark is to be ignored. */
			if (*start == '\n')
				start++;
		}
	}
	free(str);
	return(list);
}

ParserStack BaseStck =  { M_INIT_STATE, NULL }; /* never write here */

static  ParserContext Pcxt = {
	&BaseStck,	/* base_stk */
	NULL,		/* top_stk */
	0,		/* has_head */
	0,		/* has_body */
	NULL,		/* title_text */
	NULL,		/* title_mark */
	0,		/* has_body */
	0,		/* has_base */
	0,		/* has_frameset */
	0,		/* frameset_depth */
	0,		/* nframe */
	NULL,		/* frameset_mark */
	NULL,		/* object_start */
	NULL,		/* map_start */
	0		/* n_area */
};

static struct mark_up begin_mark;

#define END_STATE_TAG (-2)
#define REMOVE_TAG (-1)
#define INSERT_TAG (1)
#define GOOD_TAG (0)

#define SOP_POP_END (-2)
#define SOP_POP	(-1)
#define SOP_PUSH (1)
#define SOP_NONE (0)

/* sop: stack operation */
static int ParseElement(ParserContext *pc, struct mark_up *mptr,
        MarkType state, struct mark_up **im_ret,
	int *sop)
{
	struct mark_up *tmptr = NULL;

	*sop = SOP_NONE;

	switch (mptr->type) {		/* those tags are alway remove */
	case M_COMMENT:
	case M_DOCTYPE:
	case M_NOFRAMES:		/* we are configured with FRAME */
	case M_META:			/* we are not able to do with */
		return REMOVE_TAG;
	default:
		break;
	}

/* autorized state stack:
 * M_END_STATE M_INIT_STATE M_HTML M_HEAD M_STYLE M_TITLE M_SCRIPT
 * FRAMESET BODY */

	switch (state) {
	case M_END_STATE:
		return (REMOVE_TAG) ;
	case M_INIT_STATE:	/* ceci ne doit arrive qu'une fois a partir
				 * de <HTML> ou <FRAMESET> ou <BODY> */
		switch (mptr->type) {
		case M_NONE:		/* test a white string */
			if (mptr->is_white_text)
				return REMOVE_TAG;
			/* text is not empty in INIT_STATE: start HTML */
			tmptr = HTMLLexem("<HTML><HEAD><TITLE>Untitled</TITLE></HEAD><BODY>");
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_HTML:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_HEAD:
			if (mptr->is_end)
				return REMOVE_TAG;
			tmptr = HTMLLexem("<HTML>");
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_TITLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			tmptr = HTMLLexem("<HTML><HEAD>");
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_FRAMESET:
			if (mptr->is_end)
				return REMOVE_TAG;
			tmptr = HTMLLexem("<HTML><HEAD><TITLE>Untitled</TITLE></HEAD>");
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_BODY:
			if (mptr->is_end)
				return REMOVE_TAG;
                        tmptr = HTMLLexem("<HTML><HEAD><TITLE>Untitled</TITLE></HEAD>");
			*im_ret = tmptr;
			return INSERT_TAG;
		default:
			tmptr = HTMLLexem("<HTML><HEAD><TITLE>Untitled</TITLE></HEAD><BODY>");
			*im_ret = tmptr;
			return INSERT_TAG;
		}
		break;
/* we are in this state because of:
 * <HTML>mptr
 * <HTML><HEAD>...</HEAD>mptr
 * <HTML>...<BODY>...</BODY> mptr
 * <HTML>...<FRAMESET>...</FRAMESET> mptr
 */
	case M_HTML:			/* switch (state) */
		switch (mptr->type) {
		case M_NONE:            /* test a white string */
                        if (mptr->is_white_text)
                                return REMOVE_TAG;
					/* a non white text */
			if (pc->has_head ) {
				if ( !pc->has_body && !pc->has_frameset ) {
					tmptr = HTMLLexem("<BODY>");
				} else { /* remove all after /body or last /frameset */
					return REMOVE_TAG;
				}
			} else {	/* add head and body */
				tmptr = HTMLLexem("<HEAD><TITLE>Untitled</TITLE></HEAD><BODY>");
			}
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_HTML:
			if (!mptr->is_end)
				return REMOVE_TAG;	/* deja dans du html */
/* found </HTML> in M_HTML context stack: GOOD */
			*sop = SOP_POP_END;
			return END_STATE_TAG; /* et c'est la fin mettre sur le stack M_END_STATE */
		case M_END_STATE:
			*sop = SOP_POP_END;
			return END_STATE_TAG;	/* fin prematuree */
		case M_HEAD:
			if (mptr->is_end)
				return REMOVE_TAG;
			if (pc->has_head || pc->has_frameset || pc->has_body)
				return REMOVE_TAG;
			pc->has_head = 1;
			/* le tag suivant est en principe BODY ou FRAMESET */
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_BODY:
			if (mptr->is_end)
				return REMOVE_TAG;
			if (pc->has_frameset)
				return REMOVE_TAG;
			if (pc->has_body)
				return REMOVE_TAG;
			pc->has_body = 1;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_FRAMESET:
			if (mptr->is_end)
				return REMOVE_TAG;
			if (pc->has_body)
				return REMOVE_TAG;
			if (pc->has_frameset)
				return REMOVE_TAG;
			pc->has_frameset = 1;
			pc->frameset_mark = mptr;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		default:
			if (pc->has_head ) {
				if ( !pc->has_body && !pc->has_frameset ) {
					tmptr = HTMLLexem("<BODY>");
				} else { /* remove all after /body or last /frameset */
					return REMOVE_TAG;
				}
			} else {	/* add head and body */
				tmptr = HTMLLexem("<HEAD><TITLE>Untitled</TITLE></HEAD><BODY>");
			}
			*im_ret = tmptr;
			return INSERT_TAG;
		}
		break;
		/* end state == M_HTML */
	case M_HEAD:
		switch (mptr->type){
		case M_NONE:            /* test a white string */
                        if (mptr->is_white_text)
                                return REMOVE_TAG;
			if(pc->has_title){
				tmptr = HTMLLexem("</HEAD><BODY>");
			} else {
				tmptr = HTMLLexem("<TITLE>Untitled</TITLE></HEAD><BODY>");
			}
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		case M_TITLE:
			if(pc->has_title)
				return REMOVE_TAG;
			if (mptr->is_end)
				return REMOVE_TAG;
			pc->title_mark = mptr;
			pc->title_text = NULL;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_META:
		case M_LINK:
		case M_ISINDEX:
		case M_BASE:
			if (pc->has_base == 0) {
				pc->has_base = 1;
				return GOOD_TAG;
			}
			return REMOVE_TAG;
		case M_HEAD:
			if (!mptr->is_end)
				return REMOVE_TAG;      /* deja dans M_HEAD */
			pc->has_head = 2;
			/* le tag suivant est en principe BODY ou FRAMESET */
			*sop = SOP_POP;
			return GOOD_TAG; /* reste sur statck M_HTML */
		case M_BODY:
		case M_FRAMESET:
		case M_TABLE:
		case M_HEADER_1:
		case M_HEADER_2:
		case M_HEADER_3:
		case M_HEADER_4:
		case M_HEADER_5:
		case M_HEADER_6:
		case M_PARAGRAPH:
		case M_ANCHOR:
			tmptr = HTMLLexem("</HEAD>");
			*im_ret = tmptr; /* close the head */
			return INSERT_TAG;
		default:
			return REMOVE_TAG;
		}
		break;
	case M_TITLE:
		switch (mptr->type){
		case M_NONE:            /* test a white string */
                        if (mptr->is_white_text)
                                return REMOVE_TAG;
			if (!pc->title_text) {
				pc->title_text = strdup(mptr->text);
				return REMOVE_TAG;
			}
			pc->title_text = (char*)realloc(pc->title_text,
				strlen(pc->title_text)+ strlen(mptr->text)+2);
			strcat(pc->title_text,mptr->text);
			return REMOVE_TAG;
		case M_TITLE:
			if (!mptr->is_end)
				return REMOVE_TAG;      /* deja dans TITLE */
			pc->has_title = 1;
			if (!pc->title_text)
				pc->title_text = strdup("Untitled");
			pc->title_mark->pcdata = strdup(pc->title_text);
			*sop = SOP_POP;
			return REMOVE_TAG; /* next state will be HEAD */
		default:
/* because winfried feedback 21 Mar 2000 : title close on /title */
/*			tmptr = HTMLLexem("</TITLE>");	*/
/*			*im_ret = tmptr; *//* close the title */
/*			return INSERT_TAG; */

			return REMOVE_TAG;
		}
		break;
	case M_SCRIPT:
	case M_STYLE:
		if (mptr->type == state) {
			if (mptr->is_end) {
				*sop = SOP_POP; /* next state will be HEAD */
						/* or BODY */
				return REMOVE_TAG;
			}
		}
		return REMOVE_TAG;
		break;

	case M_FRAMESET:
		switch (mptr->type) {	/* only FRAME or FRAMESET is alowed */
		case M_NONE:
			if (mptr->is_white_text)
				return REMOVE_TAG;
			return REMOVE_TAG;	/* maybe something to repair...*/
		case M_FRAME:
			if (mptr->is_end) {
				return REMOVE_TAG;
			}
			pc->nframes++;
			return GOOD_TAG;
		case M_FRAMESET:
			if (mptr->is_end) { /* match the open FRAMESET */
				pc->frameset_depth--;
				if (pc->frameset_depth < 0) {
					*sop = SOP_POP_END;
					return END_STATE_TAG;
				}
				*sop = SOP_POP;
				return GOOD_TAG;
			}
			pc->frameset_depth++;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_END_STATE:		/* check depth */
						/* close FRAMESET cleanly */
			if (pc->frameset_depth >= 0 ) {
				tmptr = HTMLLexem("</FRAMESET>");
				*im_ret = tmptr; /* close the FRAMESET */
				return INSERT_TAG;
			}
			*sop = SOP_POP_END;
			return END_STATE_TAG;
		default:
			return REMOVE_TAG;	/* all other is an error */
		}
		break;
	case M_BODY:
		switch (mptr->type) {
		case M_BASE:			/* conscession */
			if (pc->has_base == 0) {
				pc->has_base = 1;
				return GOOD_TAG;
			}
			return REMOVE_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		case M_HTML:		/* some TAG is not in this block */
		case M_HEAD:
		case M_FRAME:
		case M_FRAMESET:
		case M_COMMENT:
		case M_LINK:
		case M_META:
		case M_DOCTYPE:
		case M_NOFRAMES:
		case M_TITLE:
		case M_ISINDEX:
		case M_UNKNOWN:
		case M_TD:
		case M_TD_CELL_PAD:
		case M_TD_CELL_FREE:
		case M_TD_CELL_PROPAGATE:
		case M_TH:
		case M_TR:
			return REMOVE_TAG;
		case M_BODY:
			if (mptr->is_end) { /* match the open BODY */
				/* ##### close all block ####### */
				*sop = SOP_POP;
				return GOOD_TAG;
			}
			return REMOVE_TAG;
		case M_END_STATE:
		case M_INIT_STATE:
			*sop = SOP_POP_END;
			return END_STATE_TAG;
		case M_OBJECT:		/* scan OBJECT */
			if (mptr->is_end)
				return REMOVE_TAG;
			pc->object_start = mptr;
			mptr->start_obj = NULL;	/*pc->object_start;*/
			mptr->end_obj = NULL;
                        mptr->try_next_obj = NULL; 
			*sop = SOP_PUSH;
			return GOOD_TAG;
		case M_MAP:		/* state is M_BODY, mptr->type is MAP */
			if(mptr->is_end)  /* see </MAP> whitout <MAP> */
				return REMOVE_TAG;
			pc->map_start = mptr;
			pc->n_area = 0;
			*sop = SOP_PUSH;	/* look for AREA */
			return GOOD_TAG;
		case M_AREA:	/* <area> outside <map> */
			return REMOVE_TAG;

		case M_FONT:
		case M_BIG:
		case M_NONE:
		case M_BOLD:
		case M_ANCHOR:
		case M_HRULE:
		case M_IMAGE:
		case M_BR:
		case M_HEADER_1:/*not really struct block only play with font*/
		case M_HEADER_2:
		case M_HEADER_3:
		case M_HEADER_4:
		case M_HEADER_5:
		case M_HEADER_6:
		case M_ADDRESS:
		case M_CITATION:
		case M_FORM:
		case M_ITALIC:
		case M_FIXED:
		case M_EMPHASIZED:
		case M_STRONG:
		case M_SUB:
		case M_SUP:
		case M_SMALL:
		case M_STRIKEOUT:
		case M_UNDERLINED:
		case M_SAMPLE:
		case M_KEYBOARD:
		case M_VARIABLE:
		case M_PARAGRAPH:
		case M_ABBR:
		case M_ACRONYM:
		case M_TBODY:
		case M_TFOOT:
		case M_THEAD:
			return GOOD_TAG;
					/* BLOCK */
		case M_TABLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return GOOD_TAG;	/* next state is TABLE */
		case M_CODE:
		case M_DIV:
		case M_PREFORMAT:
		case M_BUGGY_TABLE:
		case M_APPLET:
		case M_BLOCKQUOTE:
		case M_CAPTION:
		case M_CENTER:
		case M_DESC_LIST:
		case M_DESC_TITLE:
		case M_DESC_TEXT:
		case M_DFN:
		case M_DIRECTORY:
		case M_INPUT:
		case M_LIST_ITEM:
		case M_MENU:
		case M_NUM_LIST:
		case M_OPTION:
		case M_PARAM:
		case M_TEXTAREA:
		case M_SELECT:
		case M_UNUM_LIST:
			return GOOD_TAG;
		}
		return REMOVE_TAG;	/* Unknow tag for BODY state */
		break;
		/* end state == M_BODY */
	case M_MAP:			/* state is MAP */
		switch(mptr->type) {
		case M_AREA:
			if(mptr->is_end)
				return REMOVE_TAG;
			pc->n_area ++;
			return GOOD_TAG;
		case M_MAP:		/* pop stack */
			if (!mptr->is_end) { /* forbidden MAP in MAP */
				return REMOVE_TAG;
			}
			/* mptr is on </MAP> */
			AddMap(pc, pc->map_start, pc->n_area);
			*sop = SOP_POP;
			return GOOD_TAG; /* next state will be upper block*/
		default:		 /* only <area> is authorized */
			return REMOVE_TAG;
		}
	case M_OBJECT:			/* state is OBJECT */
		switch (mptr->type) {
		case M_PARAM:
			if (mptr->is_end) 
				return REMOVE_TAG;
			mptr->start_obj = pc->object_start;
			return GOOD_TAG;
		case M_OBJECT:
			if (mptr->is_end) { /* match the open OBJECT */
				mptr->start_obj = pc->object_start;
				pc->object_start->end_obj = mptr;
				pc->object_start = pc->object_start->start_obj;
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is upper block*/
			}
					/* object in object */
			mptr->start_obj = pc->object_start;
			mptr->try_next_obj = NULL;
			pc->object_start->try_next_obj = mptr;
			pc->object_start = mptr;
			*sop = SOP_PUSH;
			return GOOD_TAG;
		default:
			return REMOVE_TAG;
		}
		return REMOVE_TAG;
	case M_TABLE:		/* try to repair <TABLE> */
		switch (mptr->type) {
		case M_NONE:            /* test a white string */
			if (mptr->is_white_text)
				return REMOVE_TAG;
			/* add TR TD b4 a text */
			tmptr = HTMLLexem("<TR><TD>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TABLE */
						/* next Token is TR */
		case M_TABLE:
			if (mptr->is_end) { /* match the open TABLE */
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is upper block*/
			}
			/* after a TABLE we have an other TABLE*/
			/* insert a TR TD to correctly open nested table */
			tmptr = HTMLLexem("<TR><TD>");
			*sop = SOP_NONE;	/* TABLE maybe nested */
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TABLE */
						/* next Token is TR */
		case M_TR:
			if (mptr->is_end) {
				return REMOVE_TAG;
			}
			*sop = SOP_PUSH;
			return GOOD_TAG;	/* next state is M_TR */
		case M_TH:
		case M_TD:
			if (mptr->is_end) {
				return REMOVE_TAG;
			}
			/* after a TABLE we have TD or TH. miss of TR */
			/* add it */
			tmptr = HTMLLexem("<TR>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_CAPTION:
			if (mptr->is_end) {
				return REMOVE_TAG;
			}
			*sop = SOP_PUSH;
			return GOOD_TAG;        /* next state is M_CAPTION */
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
                case M_BASE:           /* some TAG is not in this block */
                case M_HEAD:          
                case M_FRAME:         
                case M_FRAMESET:      
                case M_COMMENT:       
                case M_LINK:          
                case M_META:          
                case M_DOCTYPE:       
                case M_NOFRAMES:      
                case M_TITLE:         
                case M_ISINDEX:       
                case M_UNKNOWN:       
                        return REMOVE_TAG;
		case M_HTML:
		case M_BODY:		/* close the TABLE. insert</TABLE> */
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TABLE>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			return REMOVE_TAG;
		default:
			break;
		}
		return GOOD_TAG;
	case M_TR:			/* table row state */
		switch (mptr->type) {
		case M_NONE:
			if (mptr->is_white_text)
				return REMOVE_TAG;
			/* add TD b4 a text */
			tmptr = HTMLLexem("<TD>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TR */
						/* next Token is TD */
		case M_TR:
			if (mptr->is_end) { /* match the open TR */
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is TABLE*/
			}
			/* must end the previous TR. Add </TR> */
			tmptr = HTMLLexem("</TR>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;	/* next state is TR */
						/* next Token is /TR */
		case M_TH:
		case M_TD:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return GOOD_TAG;        /* next state is M_TH or M_TD*/
		case M_TABLE:
			/* must end the previous TR. Add </TR> */
			tmptr = HTMLLexem("</TR>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_HTML:
		case M_BODY:
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TR>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			return REMOVE_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		default:
			break;
		}
		return REMOVE_TAG;
	case M_TH:			/* table header like TD */
		switch (mptr->type) {
		case M_NONE:
			return GOOD_TAG;
		case M_TH:
			if (mptr->is_end) { /* match the open TH */
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is TR */
			}
			/* must end the previous TH */
			tmptr = HTMLLexem("</TH>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TH */
						/* next Token is /TH */
		case M_TD:
			if (mptr->is_end)
				return REMOVE_TAG;
			/* must end the previous TH */
			tmptr = HTMLLexem("</TH>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TH */
						/* next Token is /TH */
		case M_TR:
			tmptr = HTMLLexem("</TH>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_TABLE:
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TH></TR>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			*sop = SOP_PUSH;
			return GOOD_TAG;        /* next state is TABLE */
		case M_HTML:
		case M_BODY:		/* close the TH. insert</TH> */
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TH>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			return REMOVE_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		default:
			break;
		}
		return GOOD_TAG;
	case M_TD:			/* table data */
		switch (mptr->type) {
		case M_NONE:
			return GOOD_TAG;
		case M_TD:
			if (mptr->is_end) { /* match the open TD */
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is TR */
			}
			/* must end the previous TD */
			tmptr = HTMLLexem("</TD>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TD */
						/* next Token is /TD */
		case M_TH:
			if (mptr->is_end)
				return REMOVE_TAG;
			/* must end the previous TD */
			tmptr = HTMLLexem("</TD>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;      /* next state is TD */
						/* next Token is /TD */
		case M_TR:
			tmptr = HTMLLexem("</TD>");
			*sop = SOP_NONE;
			*im_ret = tmptr;
			return INSERT_TAG;
		case M_TABLE:
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TD></TR>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			*sop = SOP_PUSH;
			return GOOD_TAG;        /* next state is TABLE */
		case M_HTML:
		case M_BODY:		/* close the TD. insert</TD> */
			if (mptr->is_end) {
				tmptr = HTMLLexem("</TD>");
				*sop = SOP_NONE;
				*im_ret = tmptr;
				return INSERT_TAG;
			}
			return REMOVE_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		default:
			break;
		}
		return GOOD_TAG;
	case M_CAPTION:			/* CAPTION of table */
		switch (mptr->type) {
		case M_NONE:
			return GOOD_TAG;
		case M_CAPTION:
			if (mptr->is_end) { /* match the open CAPTION */
				*sop = SOP_POP;
				return GOOD_TAG; /* next state is TABLE */
			}
			return REMOVE_TAG;
		case M_NOSCRIPT:		/* this is stupid */
			return REMOVE_TAG;
		case M_SCRIPT:
		case M_STYLE:
			if (mptr->is_end)
				return REMOVE_TAG;
			*sop = SOP_PUSH;
			return REMOVE_TAG;
		default:
			return REMOVE_TAG;
		}
	default:
		break;
	}
	
	assert(0); /* that is a unknow state!!!! */
	return REMOVE_TAG ;
}

static void PushParserStack(ParserContext *pc, MarkType id) 
{ 
        ParserStack *stmp;
 
        stmp = (ParserStack*)calloc(1,sizeof(ParserStack));
        stmp->mtype = id;
        stmp->next = pc->top_stk;
        pc->top_stk = stmp;    
}      

static MarkType PopParserStack(ParserContext *pc)
{
	MarkType id;
	ParserStack *stmp;

	if(pc->top_stk->next != NULL) {
		stmp = pc->top_stk;
		pc->top_stk = pc->top_stk->next;
		id = pc->top_stk->mtype;
		free((char*)stmp);
	} else
		id = pc->top_stk->mtype;
	return(id);
}

/* clears and resets the state stack of a parser to initiale stack */
static void ParserEndStack(ParserContext *pc)
{
	while(pc->top_stk->next != NULL)
		PopParserStack(pc);

	/* check the stateStack */
	assert(pc->top_stk->mtype == M_INIT_STATE); /* something goes wrong */
}

/* pc : Parser context.
 * pmptr: Previous marker
 * mptr:  current marker to analyze
 * lvm:   last valid marker
 * sbm:   Start 'block' marker ( TABLE FRAMSET ...) (maybe nested)
 * ebm:	  End 'block' marker (maybe null if not found or in current block)
 * state: top stack state of parser context b4 analazing mptr
 * pc_ret: Place return value here
 */
static void RecurHTMLParseRepair(ParserContext *pc,
	struct mark_up *pmptr, struct mark_up *mptr,
	MarkType state)
{
	struct mark_up *im;		/* insert this markup in list */
					/* for repair wish */
	int retval;
	int sop;			/* stack operation */
	struct mark_up *save_mptr;

	while (mptr) { 		/* check , analyze and repair */
		retval = ParseElement(pc, mptr, state, &im, &sop);
		switch (sop) {		/* must be first test because a SOP_PUSH
					 * and REMOVE_TAG at same time */
		case SOP_POP_END:
			ParserEndStack(pc);
			state = M_END_STATE;
			break;
		case SOP_POP:
			state = PopParserStack(pc);
			break;
		case SOP_PUSH:
			PushParserStack(pc, mptr->type);
			state = mptr->type;
			break;
		case SOP_NONE:
			break;
		}
		switch (retval) {
		case END_STATE_TAG:		/* finish */
			ParserEndStack(pc);
			state = M_END_STATE;
						/* remove */
			pmptr->next = mptr->next;
			FreeMarkup(mptr);
			mptr = pmptr->next;
			break;
		case REMOVE_TAG:		/* remove */
			pmptr->next = mptr->next;
			FreeMarkup(mptr);
			mptr = pmptr->next;
			break;
		case  GOOD_TAG:		/* good */
			pmptr = mptr;
			mptr = mptr->next;
			break;
		case  INSERT_TAG:		/* insert the im mark b4 mptr */
			pmptr->next = im;
			save_mptr = mptr;
			mptr = im;
			while(im->next) {
				im = im->next;
			}
			im->next = save_mptr;
			mptr = pmptr->next; /* we re-parse the new added tag*/
			break;
		}
	}
}

/* ########################################### */
/* ########################################### */

/* stack of framesets */
        
typedef struct _FrameStack{
        FrameSetInfo *frame_set;
        struct _FrameStack *next;
}FrameStack; 
        
static FrameSetInfo *frame_sets = NULL; /* list of all framesets processed */
static FrameStack *frame_stack = NULL;

static void recursiveDestroyFrameset(FrameSetInfo *fset)
{                                      
	if ( fset == NULL)
		return;

	assert(fset->type == FRAMESET_TYPE);

	recursiveDestroyFrameset(fset->fs_next_frameset);

	free(fset->fs_child_sizes);
	free(fset->fs_child_size_types);
	free(fset->fs_childs);

	free(fset);           
}    

static void FreeTopFrameSetInfo(HtmlTextInfo * hti)
{
	TopFrameSetInfo * top;
	FrameSetInfo *ffsi;

	top = hti->frameset_info;
	ffsi = top->frameset_info;

	recursiveDestroyFrameset(ffsi);
	free(top->frames);
	free(top);
}

static void FreeArea(AreaRec * area)
{
	free(area->coords);
	free(area->href);
	free(area->alt);
	free(area);
}
static void FreeMap(MapRec * map)
{
	int n_area = map->n_area;
	int i;

	for(i=0; i<n_area; i++) {
		FreeArea(map->areas[i]);
	}
	free(map->areas);
	free(map);
}

void FreeHtmlTextInfo(HtmlTextInfo * htinfo)
{
	if (htinfo->nframes)
		FreeTopFrameSetInfo(htinfo);

	if (htinfo->title)	free(htinfo->title);
	if (htinfo->base_url)	free(htinfo->base_url);
	if (htinfo->base_target)free(htinfo->base_target);
	if (htinfo->mlist)
		FreeMarkUpList(htinfo->mlist);
	if (htinfo->n_map){
		int i;
		MapRec * map;

		for(i=0; i<htinfo->n_map; i++) {
			map = htinfo->maps[i];
			FreeMap(map);
		}
		free(htinfo->maps);
	}
	free(htinfo);
}

static void pushFrameSet(FrameSetInfo *frame_set)
{
        FrameStack *tmp;              
        
        tmp = (FrameStack*)calloc(1, sizeof(FrameStack));
        tmp->frame_set = frame_set;
        tmp->next = frame_stack;
        frame_stack = tmp;
}

/* Name:                 popFrameSet
* Return Type:  frameSet*
* Description:  pops a frameset of the stack
* Returns:
*       the next frameset on the stack, or NULL when stack is empty
*/                                
static FrameSetInfo* popFrameSet(void)    
{                                     
        FrameStack *tmp;              
        FrameSetInfo *frame_set;          
                                      
        if(frame_stack->next) {       
                tmp = frame_stack;    
                frame_stack = frame_stack->next;
                frame_set = tmp->frame_set;
                free(tmp);            
                return(frame_set);    
        }                             
        return(NULL);                 
}


/* Description:
*	 creates and fills a frameSet structure with the info in it's
*        attributes
* In:     
*       attributes:     attributes for this frameset
* Returns:
*       a newly created frameset.
* Note:              
*       this routine inserts each frameset it creates in a linked list which
*       is used for stack purposes.
*/  
static FrameSetInfo * doFrameSet(char * attributes)
{
        FrameSetInfo *list, *tmp;
        char *chPtr, *tmpPtr, *ptr;
        int i;          

        /* create new entry */
        list = (FrameSetInfo*)calloc(1,sizeof(FrameSetInfo));

        list->fs_layout_type = FRAMESET_LAYOUT_ROWS;

        if((chPtr = ParseMarkTag(attributes, MT_FRAMESET,  "rows")) == NULL) {
                if((chPtr = ParseMarkTag(attributes, MT_FRAMESET, "cols")) == NULL) {     
                        list->fs_layout_type = FRAMESET_LAYOUT_ROWS;
                } else
                        list->fs_layout_type = FRAMESET_LAYOUT_COLS;
        }
 
        /*
        * count how many childs this frameset has: the no of childs is given by
        * the no of entries within the COLS or ROWS tag
        * Note that childs can be frames and/or framesets as well.
        */
        for(tmpPtr = chPtr; tmpPtr && *tmpPtr != '\0'; tmpPtr++)
                if(*tmpPtr == ',')
                        list->fs_nchilds++;
        list->fs_nchilds++;

        list->fs_child_sizes = (int*)calloc(list->fs_nchilds, sizeof(int));
        list->fs_child_size_types = (FrameSize*)calloc(list->fs_nchilds, sizeof(FrameSize));
/* a Child is either a Frame or a Frameset */
        list->fs_childs = (FrameChildInfo*)calloc(list->fs_nchilds, sizeof(FrameChildInfo));
         /*      
        * get dimensions: when we encounter a ``*'' in a size definition it
        * means we are free to choose any size we want. When its a number
        * followed by a ``%'' we must choose the size relative against the total
        * width of the render area. When it's a number not followed by anything
        * we have an absolute size.    
        */                             
        tmpPtr = ptr = chPtr;          
        i = 0;                         
        list->fs_child_size_types[i] = FRAME_SIZE_OPTIONAL;
        while(tmpPtr){                 
                if(*tmpPtr == ',' || *tmpPtr == '\0') {
                        if(*(tmpPtr-1) == '*')
                                list->fs_child_size_types[i] = FRAME_SIZE_OPTIONAL;
                        else if(*(tmpPtr-1) == '%')
                                list->fs_child_size_types[i] = FRAME_SIZE_RELATIVE;
                        else           
                                list->fs_child_size_types[i] = FRAME_SIZE_FIXED;
                                       
                        list->fs_child_sizes[i++] = atoi(ptr);
                                       
                        if(*tmpPtr == '\0') 
                                break; 
                        ptr = tmpPtr+1;
                }                      
                tmpPtr++;              
                /* sanity */           
                if(i == list->fs_nchilds) 
                        break;         
        }                              
        if (chPtr) free(chPtr);        
                                       
/* insert this new frame in the overal frameset list. */
        if(frame_sets == NULL) {
                frame_sets = list;     
        } else {                         
                for(tmp = frame_sets; tmp != NULL && tmp->fs_next_frameset != NULL;
                        tmp = tmp->fs_next_frameset);
                tmp->fs_next_frameset = list;      
        }                              
                                       
/* create actual representation of frameset */
	list->type = FRAMESET_TYPE ;
        return(list);                  
}

/*
* Description:  inserts a child frameset in it's parent list
* In:
*       parent:         parent of this frameset
*       child:          obvious
*/
static void insertFrameSetChild(FrameSetInfo *parent, FrameSetInfo *child)
{
        int idx = parent->fs_childs_done;
        FrameSetInfo *dad, *son;
	FrameChildInfo *c;
 
        assert(parent);
 
        child->fs_parent_fs = parent;
        child->fs_insert_pos = idx;
 
        dad = parent;
        son = child;
 
        son->fs_size_s = parent->fs_child_sizes[child->fs_insert_pos];
        son->fs_size_type = parent->fs_child_size_types[child->fs_insert_pos];
 
        if(son->fs_size_s == 0)
                son->fs_size_type = FRAME_SIZE_OPTIONAL;
 
        for(c = dad->fs_fc_children ; c != NULL ; c = c->fai.any_fc_next_child)
                if(!c->fai.any_fc_next_child)
                        break;
        if(c)
                c->fai.any_fc_next_child = (FrameChildInfo *)son;
        else
                dad->fs_fc_children = (FrameChildInfo *)son ;

        son->fs_fc_prev_child = c ;
        son->fs_parent_fs = dad ;

        parent->fs_childs[parent->fs_childs_done] = *((FrameChildInfo*)child);
        parent->fs_childs_done++;
}

     
/* Description:  fills a HTML frame structure with data from it's attributes
* In:
*       topfs:          The Top level Frameset
*       attributes:     frame attributes
*	idx:		numero du frame % au top level
* Returns:
*       updated frame
* Note:
*       this routine takes the frame to update from an already allocated list
*       of frames .
*****/
 
static FrameInfo * doFrame(TopFrameSetInfo *topfs, char *attributes, int idx)
{
        FrameInfo *frame;
        char *chPtr;
 
        frame = &topfs->frames[idx];
 
/* default frame sizing & scrolling */
        frame->frame_size_type = FRAME_SIZE_FIXED;
        frame->frame_scroll_type = FRAME_SCROLL_AUTO;
 
/* get frame name, default to _frame if not present */
        if(!attributes || (frame->frame_name = ParseMarkTag(attributes, MT_FRAME, "name")) == NULL) {
                char buf[24];

                sprintf(buf, "_frame%i", idx);
                frame->frame_name = strdup(buf);
        }
 
        /* pick up all remaining frame attributes */
        if(attributes) {
        	frame->frame_border = 2;
        	if((chPtr = ParseMarkTag(attributes, MT_FRAME, "frameborder")) != NULL){            
         
/* frameset definition allows a tag to have a textvalue or a number. */
                	if(!(strcasecmp(chPtr, "no")) || *chPtr == '0')
                        	frame->frame_border = 0;
                	else
                        	frame->frame_border = atoi(chPtr);
                	free(chPtr);         
        	} 
/* disable resizing if we don't have a border */
        	if(!frame->frame_border)
                	frame->frame_resize = 0;

                frame->frame_src = ParseMarkTag(attributes,MT_FRAME,"src");
                frame->frame_margin_width = DEFAULT_FRAME_MARGIN_WIDTH;
                if (chPtr = ParseMarkTag(attributes, MT_FRAME, "marginwidth")){
                        frame->frame_margin_width = atoi(chPtr);
                        free(chPtr);
                }
                frame->frame_margin_height = DEFAULT_FRAME_MARGIN_HEIGHT;
                if (chPtr = ParseMarkTag(attributes, MT_FRAME, "marginheight")){
                        frame->frame_margin_height = atoi(chPtr);
                        free(chPtr);
                }

 
/* inherit margins from parent if we'd gotten an invalid spec */
                if(!frame->frame_margin_width)
                        frame->frame_margin_width = topfs->def_margin_width;
                if(!frame->frame_margin_height)
                        frame->frame_margin_height =topfs->def_margin_height;                                       
/*                                     
 * This is useless as we don't support frame resizing. I think this is
 * a thing the caller must be able to do. A possible way could be to
 * overlay the render area with a PanedWidget and store these HTML
 * widgets as childs of this paned widget.  
 */                                    
                if (chPtr = ParseMarkTag(attributes, MT_FRAME, "noresize")){
                        frame->frame_resize = 0;
                        free(chPtr);   
                }                      
/* what about scrolling? */            
                if(chPtr = ParseMarkTag(attributes, MT_FRAME,"scrolling")){
                        if(!(strcasecmp(chPtr, "yes")))
                                frame->frame_scroll_type = FRAME_SCROLL_YES;
                        else if(!(strcasecmp(chPtr, "no")))
                                frame->frame_scroll_type = FRAME_SCROLL_NONE;                        free(chPtr);   
                }                      
        } else {                       
                frame->frame_src           = NULL;
                frame->frame_margin_width  = DEFAULT_FRAME_MARGIN_WIDTH;
                frame->frame_margin_height = DEFAULT_FRAME_MARGIN_HEIGHT;
                frame->frame_resize        = 1;
        }                              
                                       
#ifdef DEBUG_FRAME
fprintf(stderr,"doFrame, frame %i created\n\tname: %s\n\tsrc : %s\n"         
                "\tmargin width : %i\n\tmargin height: %i\n\tresize: %s\n"
                "\tscrolling    : %s\n",
                idx, frame->frame_name,
                frame->frame_src ? frame->frame_src : "<none>",
                frame->frame_margin_width, frame->frame_margin_height,
                frame->frame_resize ? "yes" : "no",
                frame->frame_scroll_type == FRAME_SCROLL_AUTO ? "auto" :
                (frame->frame_scroll_type == FRAME_SCROLL_YES ? "always" : "none")));                               
#endif   
/* Actual widget creation is postponed until the very last moment
 * of _XmHTMLCreateFrames       
 */                             
        return(frame);                 
}


/* Description:  sets the geometry constraints on a HTML frame
* In:
*       frame_set:      frameset parent of this frame;
*       frame:          frame for which to set the constraints
* Returns:
*       nothing, but frame is updated.
*/
static void insertFrameChild(FrameSetInfo *frame_set, FrameInfo *frame)
{
        FrameChildInfo *c;
	FrameSetInfo *dad;
        int insert_pos = frame_set->fs_childs_done;
 
        frame->frame_size_s = frame_set->fs_child_sizes[insert_pos];
        frame->frame_size_type = frame_set->fs_child_size_types[insert_pos];
 
        if(frame->frame_size_s == 0)
                frame->frame_size_type = FRAME_SIZE_OPTIONAL;
 
        dad = frame_set;
        for(c = dad->fs_fc_children ; c != NULL ; c = c->fai.any_fc_next_child)
                if(!c->fai.any_fc_next_child)
                        break;
        if(c)
                c->fai.any_fc_next_child = (FrameChildInfo *) frame;
        else                          
                dad->fs_fc_children = (FrameChildInfo *) frame;
        frame->frame_fc_prev_child = c;
        frame->frame_parent_fs = dad;

        frame_set->fs_childs_done++;     
}                                     
     
/* mptr is a pointer to the first frameset mark*/
static TopFrameSetInfo * makeFramesetInfo(struct mark_up * mptr, int o_nframe)
{
	struct mark_up *tmp;
	FrameSetInfo *current_set = NULL;
	FrameSetInfo *parent_set = NULL;
	TopFrameSetInfo *top_set = NULL;
	FrameInfo * frame_info = NULL;
	int idx = 0;
 
/* mptr is a pointer to the first frameset mark*/
	assert(mptr->type == M_FRAMESET);

	frame_stack = NULL;
	frame_sets = NULL;

	top_set = (TopFrameSetInfo*) calloc(1, sizeof(TopFrameSetInfo));
	top_set->n_allo_frames = o_nframe;
	top_set->frames = (FrameInfo*) calloc(o_nframe, sizeof(FrameInfo));

	pushFrameSet(current_set); 
	parent_set = frame_stack->frame_set;
	top_set->frameset_info = current_set = doFrameSet(mptr->start);
	mptr = mptr->next;
	
	for(tmp = mptr; tmp != NULL; tmp = tmp->next) {
		switch(tmp->type) {
		case M_FRAMESET:
			if(tmp->is_end) { 	/* end frameset  pop stack*/
				current_set = popFrameSet();
/* no more sets on the stack : we've reached end of outermost frameset tag */
				if(current_set == NULL) {
					top_set->nframes = idx;
					return top_set;
				}
			} else { /* A new frameset, push current frameset */
				pushFrameSet(current_set); 
				parent_set = frame_stack->frame_set;
/* Check if we still have room for this thing.*/
				if( parent_set->fs_childs_done < parent_set->fs_nchilds) {
						/* create a new frameset */
					current_set = doFrameSet(tmp->start);
					insertFrameSetChild(parent_set, current_set);
				} else {
/* No more room available, this is an unspecified
* frameset, kill it and all childs it might have. 
*/
					int depth = 1;
					for(tmp = tmp->next; tmp != NULL; tmp = tmp->next) {
						if(tmp->type == M_FRAMESET) {
							if(tmp->is_end) {
								if(--depth == 0)
									break;
							} else /*child frameset*/
								depth++;
						}
					}
#ifdef DEBUG_FRAME
	fprintf(sdterr,"makeFrameset: Bad <FRAMESET>: missing COLS/ROWS attribute on parent set\n    Skipped all frame declarations\n");
#endif 
				}
			}
			break;

		case M_FRAME:
					/* check if we have room left */
			if(current_set->fs_childs_done < current_set->fs_nchilds) {
					/* insert child in current frameset */
				frame_info = doFrame(top_set, tmp->start, idx);
				insertFrameChild(current_set, frame_info);
				idx++;
			} else {
#ifdef DEBUG_FRAME
	fprintf(sdterr,"makeFrameset: Bad <FRAME> tag: missing COLS or ROWS attribute\n    on parent set, skipped.");
#endif
				; /* do nothing : skip FRAME */
			}
			break;

/* </FRAME> doesn't exist. The parser is smart enough to kick these out.
 */
		default:
			break;		/* skip unknow tag */
		}	/* switch */

		if(idx == top_set->n_allo_frames){
			while (current_set)
				current_set = popFrameSet();
			top_set->nframes = idx;
			return top_set;		/* reach the number, so return */
		}
	}

/* there is too much frame than declared in frameset */
	if(idx < top_set->n_allo_frames){
		while (current_set)
			current_set = popFrameSet();
		top_set->nframes = idx;
		return top_set;		/* reach the number, so return */
	}

	assert(0);		/* never goes here, else parser error... */
}

int * CoordVals(char *coords, int n)
{
	char * beg_int;
	int * coord_tab;
	int i;

	coord_tab = (int *)calloc(n,sizeof(int));

	beg_int=coords;
	for(i=0; i<n; i++){
		coord_tab[i] = atoi(beg_int);
		beg_int = strchr(beg_int,',');
		beg_int++;
	}
	return coord_tab;
}

/*--- HTML 4.01, 13.6.1 Client-side image maps: the MAP and AREA elements  ---*/                                
/* Add a <MAP> to contexte */
static MapRec * CreateMap(struct mark_up * map_start, int area_count)
{
	char *map_name = 0, *coords;
	MapRec *cur_map = NULL;  
	AreaRec *cur_area = NULL;
	int i_area;
	struct mark_up *mptr;
 
	if( area_count <=0)
		return NULL;

	mptr = map_start;
	assert(mptr->type == M_MAP && !mptr->is_end);

	map_name=ParseMarkTag(mptr->start,MT_MAP,"name");
	if (!map_name )
		return NULL;

	cur_map=(MapRec *)calloc(1,sizeof(MapRec));
	cur_map->name = map_name;
	cur_map->n_area = area_count;
	cur_map->areas = (AreaRec **)calloc(area_count, sizeof(AreaRec *));
	i_area = 0;
	mptr = mptr->next;
	while(mptr) {
		char * href;
		char * shape_s, *tmpPtr;
		int shape;
		int comma_count,n_coord;
		int * area_coords=NULL;

		if( mptr->type == M_MAP && mptr->is_end)
			break;		/* end process */
		assert(mptr->type == M_AREA);
        
		href = ParseMarkTag(mptr->start,MT_AREA,"href");
		if (! href )
			continue;
		shape_s = ParseMarkTag(mptr->start,MT_AREA,"shape");
		shape = AREA_RECT;	/* default */
		if(shape_s) {
			if(strcasecmp(shape_s,"default") == 0)
				shape = AREA_DEFAULT;
			else if(strcasecmp(shape_s,"circle") == 0)
				shape = AREA_CIRCLE;
			else if((strcasecmp(shape_s,"poly") == 0) ||(strcasecmp(shape_s,"polygon") == 0))
				shape = AREA_POLYGON;
			free(shape_s);
		}
		
		coords = ParseMarkTag(mptr->start,MT_AREA,"coords");
		if(!coords)
			shape = AREA_DEFAULT;

		comma_count = 0;
		for(tmpPtr = coords; tmpPtr && *tmpPtr != '\0'; tmpPtr++)
			if(*tmpPtr == ',')
				comma_count++;

		n_coord = 0;
		if( shape == AREA_RECT ) {
			if (comma_count != 3) {
				mptr = mptr->next;
				continue;
			}
			area_coords = CoordVals(coords, 4);
			n_coord = 4;
		} else if( shape == AREA_CIRCLE) {
			if (comma_count != 2) {
				mptr = mptr->next;
				continue;
			}
			area_coords = CoordVals(coords, 3);
			n_coord=3;
		} else if( shape == AREA_POLYGON) {
			if ((comma_count < 5) && ((comma_count % 2) != 1)) {
				mptr = mptr->next;
				continue;
			}
			area_coords = CoordVals(coords, comma_count+1);
			n_coord = comma_count+1;
		}

		if(coords)	free(coords);       

		cur_area = (AreaRec *)calloc(1,sizeof(AreaRec));
		cur_area->href = href;
		cur_area->shape = shape;
		cur_area->alt = ParseMarkTag(mptr->start,MT_AREA,"alt");
		cur_area->coords = area_coords;
		cur_area->n_coords = n_coord;

		cur_map->areas[i_area] = cur_area;
		i_area++;
		mptr = mptr->next;
	} /* while(mptr) */      
	cur_map->n_area = i_area;
	return cur_map;
}

static void AddMap(ParserContext *pc, struct mark_up * map_start, int area_count)
{
	MapRec * new_map;

	new_map = CreateMap(map_start, area_count);
	if (! new_map)
		return;
	if (!pc->map_count) {
		pc->map_count = 1;
		pc->map_table = (MapRec **) calloc(1, sizeof(MapRec *));
	} else {
		pc->map_count++;
		pc->map_table = (MapRec **) realloc(pc->map_table,
					pc->map_count * sizeof(MapRec *));
	}
	pc->map_table[pc->map_count -1 ] = new_map;
}

/* ########################################### */
/* ########################################### */
/* Parser of HTML text.  Takes raw text, and produces a linked
 * list of mark objects SUITABLE FOR GRAPHIC PROCESSING.
 * It call HTMLLexem and analyze the stream of markup return by HTMLLexem.
 * We check for correct HTML syntax and possibly repair error.
 * We return a GOOD HTML markup stream.
 * the stack is used for checking 'block' element such as FRAMESET TABLE
 * H1 H2 BODY HTML... Thoses elements is designing the structure of a page.
 * We don't check for <FONT> because is designing the apparance of page.
 */
HtmlTextInfo * HTMLParseRepair( char *str)
{
	struct mark_up * mptr;		/* current to analyze */
	struct mark_up *pmptr;		/* previous analyze */
	struct mark_up begm;		/* begin of list */
	char * base_url;
	char * base_target =NULL;
	HtmlTextInfo *htinfo;
	TopFrameSetInfo *fs_info = NULL;

	if (str == NULL)
		return(NULL);

	mptr = HTMLLexem(str);
/* create a pseudo marker for initialisation */
	begm = begin_mark;
	begm.type = M_INIT_STATE;
	begm.next = mptr;
	pmptr = &begm;
/*initialize the parser context */
	Pcxt.base_stk = &BaseStck;
	Pcxt.top_stk = Pcxt.base_stk; /*mtype=M_INIT_STATE next = NULL */
	Pcxt.has_head =0;
	Pcxt.has_title =0;
	Pcxt.title_text =NULL;
	Pcxt.title_mark =NULL;
	Pcxt.has_body =0;
	Pcxt.has_base = 0;
	Pcxt.has_frameset = 0;
	Pcxt.frameset_depth = 0;
	Pcxt.nframes = 0;
	Pcxt.frameset_mark = NULL;
	Pcxt.object_start = NULL;

	Pcxt.map_start = NULL;
	Pcxt.n_area = 0;
	Pcxt.map_table = NULL;
	Pcxt.map_count = 0;
	RecurHTMLParseRepair(&Pcxt, pmptr, mptr, M_INIT_STATE);

        mptr = begm.next;
        base_url = NULL;
        while (mptr != NULL){ 
                switch (mptr->type){
/* take care of tag BASE */            
                case M_BASE:           
                        if (mptr->is_end)
                                break; 
                        base_url = ParseMarkTag(mptr->start, MT_BASE, "HREF");
                        base_target = ParseMarkTag(mptr->start, MT_BASE,"target");
                        break;
		default:
			break;
                }                      
                mptr = mptr->next;     
        }
	if (Pcxt.nframes) {	/* have frameset check the valid set */
				/* flatten frame */
		fs_info = makeFramesetInfo(Pcxt.frameset_mark, Pcxt.nframes);
		if (! fs_info) {	/* error */
			Pcxt.nframes = 0;
		} else {
			Pcxt.nframes = fs_info->nframes;
		}
	}
	htinfo = (HtmlTextInfo *) calloc(1,sizeof(HtmlTextInfo));
	if (!Pcxt.title_text)
		Pcxt.title_text = strdup("Untitled");
	htinfo->title = Pcxt.title_text;
	htinfo->base_url = base_url;
	htinfo->base_target = base_target;
	htinfo->mlist = begm.next;
	htinfo->nframes = Pcxt.nframes;	/* if not null, it's a frameset */
	htinfo->frameset_info = fs_info;
	htinfo->n_map = Pcxt.map_count;
	htinfo->maps = Pcxt.map_table;
	return htinfo ;
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
	} else if (!strcasecmp(str, MT_FRAMESET)) {
		type = M_FRAMESET;
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
	} else if (!strcasecmp(str, MT_ACRONYM)) {
		type = M_ACRONYM;
	} else if (!strcasecmp(str, MT_ABBR)) {
		type = M_ABBR;
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
	} else if (!strcasecmp(str, MT_ISINDEX)) {
		type = M_ISINDEX;
	} else if (!strcasecmp(str, MT_HRULE)) {
		type = M_HRULE;
	} else if (!strcasecmp(str, MT_BASE)) {
		type = M_BASE;
	} else if (!strcasecmp(str, MT_BR)) {
		type = M_BR;
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
	} else if (!strcasecmp(str, MT_HEAD)) {
	        type = M_HEAD;
	} else if (!strcasecmp(str, MT_UNDERLINED)) {
	        type = M_UNDERLINED;
	} else if (!strcasecmp(str, MT_BODY)) {
	        type = M_BODY;
	} else if (!strcasecmp(str, MT_TABLE)) {
		type = M_TABLE;
	} else if (!strcasecmp(str, MT_CAPTION)) {
		type = M_CAPTION;
	} else if (!strcasecmp(str, MT_TR)) {
		type = M_TR;
	} else if (!strcasecmp(str, MT_TH)) {
		type = M_TH;
	} else if (!strcasecmp(str, MT_TD)) {
		type = M_TD;
	} else if (!strcasecmp(str, MT_TBODY)) {
		type = M_TBODY;
	} else if (!strcasecmp(str, MT_THEAD)) {
		type = M_THEAD;
	} else if (!strcasecmp(str, MT_TFOOT)) {
		type = M_TFOOT;
	} else if (!strcasecmp(str, MT_OBJECT)){
		type = M_OBJECT;
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
	} else if (!strcasecmp(str, MT_LINK)){
		type = M_LINK;
	} else if (!strcasecmp(str, MT_SCRIPT)){
		type = M_SCRIPT;
	} else if (!strcasecmp(str, MT_NOSCRIPT)){
		type = M_NOSCRIPT;
	} else if (!strcasecmp(str, MT_STYLE)){
		type = M_STYLE;
	} else if (!strcasecmp(str, MT_NOFRAMES)){
		type = M_NOFRAMES;
	} else {
#ifdef HTMLTRACE
		fprintf(stderr, "warning: unknown mark (%s)\n", str);
#endif
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
	int quoted_single;
	int has_value;

	quoted = 0;
	quoted_single = 0;
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
	switch (*ptr) {
	case '\"' :
		quoted = 1;
		ptr++;
		break;
	case '\'' :
		quoted_single = 1;
		ptr++;
		break;
	}
	start = ptr;
	if(quoted) { /* Get tag value.  Either a quoted string or a single word */
		while ((*ptr != '\"')&&(*ptr != '\0'))
			ptr++;
	} else if (quoted_single) {
		while ((*ptr != '\'')&&(*ptr != '\0'))
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
	if ( (quoted_single || quoted) && *ptr!='\0')
		ptr++;
	*ptrp = ptr;
	return(tag_val);
}

/* Parse mark text for the value associated with the passed mark tag.
 * If the passed tag is not found, return NULL.
 * If the passed tag is found but has no value, return "".
 */
char* ParseMarkTag(char *text, const char *mtext, const char *mtag)
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

