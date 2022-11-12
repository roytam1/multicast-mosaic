/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/*		CCI redirect object	 */
#include <stdio.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"

#include "cciServer.h"
#include "../libhtmlw/list.h"

#include "HTPlain.h"
#include "HTUtils.h"
#include "HText.h"
#include "HTFile.h"
#include "HTCompressed.h"
#include "bla.h"

extern void MoCCISendOutputToClient();
extern void HTCompressedFileToFile (char *fnam, int compressed);

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

/*		HTML Object	 */

struct _HTStream {
	WWW_CONST HTStreamClass *	isa;
	HTAtom *dataType;
	char * fileName; /* name of temp file... kept for unlink()ing*/
	FILE *fp; 
        int compressed;
};

/*			A C T I O N 	R O U T I N E S		 */

/*	Character handling */
PRIVATE void CCI_put_character ARGS2(HTStream *, me, char, c)
{
	fputc(c,me->fp);
}

/*	String handling */
PRIVATE void CCI_put_string ARGS2(HTStream *, me, WWW_CONST char*, s)
{
	fwrite(s,1,strlen(s),me->fp);
}

PRIVATE void CCI_write ARGS3(HTStream *, me, WWW_CONST char*, s, int, l)
{
	fwrite(s,1,l,me->fp);
}

/*	Free an HTML object
**
** Note that the SGML parsing context is freed, but the created object is not,
** as it takes on an existence of its own unless explicitly freed.
*/
PRIVATE void CCI_free ARGS1(HTStream *, me)
{
#ifndef DISABLE_TRACE
	if (srcTrace) {
		fprintf(stderr,"CCI_free()\n");
	}
#endif
}

/*	End writing */

PRIVATE void CCI_end_document ARGS1(HTStream *, me)
{

	fclose(me->fp);
/* ship it */
	if ( me->compressed != COMPRESSED_NOT)
		HTCompressedFileToFile (me->fileName, me->compressed);	
	MoCCISendOutputToClient(HTAtom_name(me->dataType),me->fileName);
}

PRIVATE void CCI_handle_interrupt ARGS1(HTStream *, me)
{
	fclose(me->fp);
	unlink(me->fileName);
	free(me->fileName);
}

/*		Structured Object Class */
PUBLIC WWW_CONST HTStreamClass CCIout =
{		
	"CCIout",
	CCI_free,
	CCI_end_document,
	CCI_put_character, 	CCI_put_string, CCI_write,
        CCI_handle_interrupt
}; 


/*		New object */
PUBLIC HTStream* CCIPresent ARGS5(
	HTPresentation *,	pres,
	HTParentAnchor *,	anchor,	
	HTStream *,		sink,
        HTFormat,               format_in,
        int,                    compressed)
{
	HTStream* me = (HTStream*)malloc(sizeof(HTStream));

	me->isa = &CCIout;       

	me->fileName = mo_tmpnam(NULL);
	if (!(me->fp = fopen(me->fileName,"w"))) { /*error, can't open tmp file */
		return(sink);
	}
	me->dataType = pres->rep;
	me->compressed = compressed;

	return (HTStream*) me;
}
