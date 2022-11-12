/*		Plain text object		HTWrite.c
**
**	This version of the stream object just writes to a socket.
**	The socket is assumed open and left open.
**
**	Bugs:
**		strings written must be less than buffer size.
*/
#include <stdio.h>
#include <sys/types.h>

#include "HText.h"
#include "HTPlain.h"
#include "HTUtils.h"
#include "HTFile.h"
#include "HTCompressed.h"
#include "HTParams.h"

/*		HTML Object */

struct _HTStream {
	WWW_CONST HTStreamClass *	isa;
	HText * 		text;
        int compressed;
};

/*			A C T I O N 	R O U T I N E S */

/*	Character handling */

PRIVATE void HTPlain_put_character (HTStream *me, char c, caddr_t appd)
{
	HText_appendCharacter(me->text, c);
}

/*	String handling */
PRIVATE void HTPlain_put_string (HTStream *me, WWW_CONST char* s, caddr_t appd)
{
	HText_appendText(me->text, s);
}

PRIVATE void HTPlain_write (HTStream * me, WWW_CONST char* s, int l, caddr_t appd)
{
	HText_appendBlock (me->text, s, l);
}

/*	Free an HTML object
**
** Note that the SGML parsing context is freed, but the created object is not,
** as it takes on an existence of its own unless explicitly freed.
*/
PRIVATE void HTPlain_free (HTStream * me, caddr_t appd)
{
	if (me->compressed != COMPRESSED_NOT) {
if (wWWParams.trace) fprintf (stderr, 
"[HTPlain_free] OK, we're going to decompress HText\n");
		HTCompressedHText (me->text, me->compressed, 1,appd);
	}
	free(me);
}

/*	End writing */

PRIVATE void HTPlain_end_document (HTStream * me, caddr_t appd)
{
	HText_endAppend(me->text);
}

PRIVATE void HTPlain_handle_interrupt (HTStream * me, caddr_t appd)
{
	HText_doAbort(me->text);
}

/*		Structured Object Class */

PUBLIC WWW_CONST HTStreamClass HTPlain =
{		
	"SocketWriter",
	HTPlain_free,
	HTPlain_end_document,
	HTPlain_put_character, 	HTPlain_put_string, HTPlain_write,
        HTPlain_handle_interrupt
}; 

/*		New object */

PUBLIC HTStream* HTPlainPresent(HTPresentation *pres, HTParentAnchor *anchor,
	HTStream *sink, HTFormat format_in, int compressed, caddr_t appd)
{
	HTStream* me = (HTStream*)malloc(sizeof(*me));

if (wWWParams.trace) fprintf (stderr, "[HTPlainPresent] here we are; format_in is '%s' and compressed is %d\n", HTAtom_name (format_in), compressed);

	me->isa = &HTPlain;       
	me->text = HText_new();
	me->compressed = compressed;
	HText_beginAppend(me->text);
	if (me->compressed == COMPRESSED_NOT)
	HText_appendText(me->text, "<PLAINTEXT>\n");
	return (HTStream*) me;
}
