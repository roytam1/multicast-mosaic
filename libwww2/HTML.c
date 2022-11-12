/*		Structured stream to Rich hypertext converter
**		============================================
**
**	This generates a hypertext object.  It converts from the
**	structured stream interface from HTML events into the style-
**	oriented interface of the HText.h interface.  This module is
**	only used in clients and should not be linked into servers.
**
**	Override this module if you are making a new GUI browser.
**
*/
#include "HTML.h"

#include <ctype.h>
#include <stdio.h>

#include "HTAtom.h"
#include "HTChunk.h"
#include "HText.h"

#include "HTAlert.h"
#include "HTParse.h"

/*		HTML Object		 */

struct _HTStructured {
    WWW_CONST HTStructuredClass * 	isa;
    HTParentAnchor * 		node_anchor;
    HText * 			text;
    HTStream*			target;			/* Output stream */
    HTStreamClass		targetClass;		/* Output routines */
};

struct _HTStream {
    WWW_CONST HTStreamClass *	isa;
    /* .... */
};

/* 	Entity values -- for ISO Latin 1 local representation
**
**	This MUST match exactly the table referred to in the DTD!
*/
static char * ISO_Latin1[] = {
  	"\306",	/* capital AE diphthong (ligature) */ 
  	"\301",	/* capital A, acute accent */ 
  	"\302",	/* capital A, circumflex accent */ 
  	"\300",	/* capital A, grave accent */ 
  	"\305",	/* capital A, ring */ 
  	"\303",	/* capital A, tilde */ 
  	"\304",	/* capital A, dieresis or umlaut mark */ 
  	"\307",	/* capital C, cedilla */ 
  	"\320",	/* capital Eth, Icelandic */ 
  	"\311",	/* capital E, acute accent */ 
  	"\312",	/* capital E, circumflex accent */ 
  	"\310",	/* capital E, grave accent */ 
  	"\313",	/* capital E, dieresis or umlaut mark */ 
  	"\315",	/* capital I, acute accent */ 
  	"\316",	/* capital I, circumflex accent */ 
  	"\314",	/* capital I, grave accent */ 
  	"\317",	/* capital I, dieresis or umlaut mark */ 
  	"\321",	/* capital N, tilde */ 
  	"\323",	/* capital O, acute accent */ 
  	"\324",	/* capital O, circumflex accent */ 
  	"\322",	/* capital O, grave accent */ 
  	"\330",	/* capital O, slash */ 
  	"\325",	/* capital O, tilde */ 
  	"\326",	/* capital O, dieresis or umlaut mark */ 
  	"\336",	/* capital THORN, Icelandic */ 
  	"\332",	/* capital U, acute accent */ 
  	"\333",	/* capital U, circumflex accent */ 
  	"\331",	/* capital U, grave accent */ 
  	"\334",	/* capital U, dieresis or umlaut mark */ 
  	"\335",	/* capital Y, acute accent */ 
  	"\341",	/* small a, acute accent */ 
  	"\342",	/* small a, circumflex accent */ 
  	"\346",	/* small ae diphthong (ligature) */ 
  	"\340",	/* small a, grave accent */ 
  	"\046",	/* ampersand */ 
  	"\345",	/* small a, ring */ 
  	"\343",	/* small a, tilde */ 
  	"\344",	/* small a, dieresis or umlaut mark */ 
  	"\347",	/* small c, cedilla */ 
  	"\351",	/* small e, acute accent */ 
  	"\352",	/* small e, circumflex accent */ 
  	"\350",	/* small e, grave accent */ 
  	"\360",	/* small eth, Icelandic */ 
  	"\353",	/* small e, dieresis or umlaut mark */ 
  	"\076",	/* greater than */ 
  	"\355",	/* small i, acute accent */ 
  	"\356",	/* small i, circumflex accent */ 
  	"\354",	/* small i, grave accent */ 
  	"\357",	/* small i, dieresis or umlaut mark */ 
  	"\074",	/* less than */ 
  	"\361",	/* small n, tilde */ 
  	"\363",	/* small o, acute accent */ 
  	"\364",	/* small o, circumflex accent */ 
  	"\362",	/* small o, grave accent */ 
  	"\370",	/* small o, slash */ 
  	"\365",	/* small o, tilde */ 
  	"\366",	/* small o, dieresis or umlaut mark */ 
  	"\337",	/* small sharp s, German (sz ligature) */ 
  	"\376",	/* small thorn, Icelandic */ 
  	"\372",	/* small u, acute accent */ 
  	"\373",	/* small u, circumflex accent */ 
  	"\371",	/* small u, grave accent */ 
  	"\374",	/* small u, dieresis or umlaut mark */ 
  	"\375",	/* small y, acute accent */ 
  	"\377",	/* small y, dieresis or umlaut mark */ 
};

/*
**			A C T I O N 	R O U T I N E S
*/

/*	Character handling
**	------------------
*/
PRIVATE void HTML_put_character ARGS2(HTStructured *, me, char, c)
{
	if (!me->text) {
		me->text = HText_new();
		HText_beginAppend(me->text);
	}
	HText_appendCharacter(me->text, c);
}

/*	String handling
**	---------------
**
**	This is written separately from put_character becuase the loop can
**	in some cases be promoted to a higher function call level for speed.
*/
PRIVATE void HTML_put_string ARGS2(HTStructured *, me, WWW_CONST char*, s)
{
	if (!me->text) {
		me->text = HText_new();
		HText_beginAppend(me->text);
	}
	HText_appendText(me->text, s);
}

/*	Buffer write
**	------------
*/
PRIVATE void HTML_write ARGS3(HTStructured *, me, WWW_CONST char*, s, int, l)
{
	WWW_CONST char* p;
	WWW_CONST char* e = s+l;

	for (p=s; s<e; p++)
		HTML_put_character(me, *p);
}

/*		Expanding entities
**		------------------
**	(In fact, they all shrink!)
*/

PRIVATE void HTML_put_entity ARGS2(HTStructured *, me, int, entity_number)
{
    HTML_put_string(me, ISO_Latin1[entity_number]);/* @@ Other representations */
}

/*	Free an HTML object
**	-------------------
**
** If the document is empty, the text object will not yet exist.
   So we could in fact abandon creating the document and return
   an error code.  In fact an empty document is an important type
   of document, so we don't.
**
**	If non-interactive, everything is freed off.   No: crashes -listrefs
**	Otherwise, the interactive object is left.	
*/
PRIVATE void HTML_free ARGS1(HTStructured *, me)
{
	if (me->text)
		HText_endAppend(me->text);
  
	if (me->target) {
		(*me->targetClass.end_document)(me->target);
		(*me->targetClass.free)(me->target);
	}
	free(me);
}

PRIVATE void HTML_handle_interrupt ARGS1(HTStructured *, me)
{
	if (me->text)
		HText_doAbort (me->text);
  
	if (me->target) {
		(*me->targetClass.handle_interrupt)(me->target);
	}
/* Not necessarily safe... */
/* free(me); */
}

PRIVATE void HTML_end_document ARGS1(HTStructured *, me)
{			/* Obsolete */
}

/*	Structured Object Class
**	-----------------------
*/
PRIVATE WWW_CONST HTStructuredClass HTMLPresentation =/*As opposed to print etc */
{		
	"text/html",
	HTML_free,
	HTML_end_document, HTML_handle_interrupt,
	HTML_put_character, 	HTML_put_string,  HTML_write,
	HTML_put_entity
}; 


/*		New Structured Text object
**		--------------------------
**
**	The strutcured stream can generate either presentation,
**	or plain text, or HTML.
*/
PUBLIC HTStructured* HTML_new ARGS3(
	HTParentAnchor *, 	anchor,
	HTFormat,		format_out,
	HTStream*,		stream)
{
	HTStructured * me;

	me = (HTStructured*) malloc(sizeof(*me));
	if (me == NULL)
		outofmem(__FILE__, "HTML_new");
	me->isa = &HTMLPresentation;
	me->node_anchor =  anchor;
	me->text = 0;
	me->target = stream;
	if (stream) 
		me->targetClass = *stream->isa;	/* Copy pointers */
	return (HTStructured*) me;
}
