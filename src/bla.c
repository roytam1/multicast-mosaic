/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/*		CCI redirect object	 */

#include <stdio.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "cciServer.h"
#include "../libhtmlw/list.h"
#include "HText.h"
#include "HTPlain.h"
#include "HTUtils.h"
#include "HText.h"
#include "HTFile.h"
#include "HTCompressed.h"
#include "bla.h"

extern void MoCCISendOutputToClient();

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
PRIVATE void CCI_put_character (HTStream * me, char c, caddr_t appd)
{
	fputc(c,me->fp);
}

/*	String handling */
PRIVATE void CCI_put_string (HTStream * me, WWW_CONST char* s, caddr_t appd)
{
	fwrite(s,1,strlen(s),me->fp);
}

PRIVATE void CCI_write (HTStream * me, WWW_CONST char* s, int l, caddr_t appd)
{
	fwrite(s,1,l,me->fp);
}

/*	Free an HTML object
**
** Note that the SGML parsing context is freed, but the created object is not,
** as it takes on an existence of its own unless explicitly freed.
*/
PRIVATE void CCI_free (HTStream * me, caddr_t appd)
{
}

/*	End writing */

PRIVATE void CCI_end_document (HTStream * me, caddr_t appd)
{

	fclose(me->fp);
/* ship it */
	if ( me->compressed != COMPRESSED_NOT)
		HTCompressedFileToFile (me->fileName, me->compressed,(caddr_t)mo_main_next_window(NULL));	
	MoCCISendOutputToClient(HTAtom_name(me->dataType),me->fileName);
}

PRIVATE void CCI_handle_interrupt (HTStream * me, caddr_t appd)
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
PUBLIC HTStream* CCIPresent (
	HTPresentation *	pres,
	HTParentAnchor *	anchor,	
	HTStream *		sink,
        HTFormat               format_in,
        int                    compressed,
	caddr_t			appd)
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
