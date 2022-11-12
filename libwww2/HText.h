/* Rich Hypertext object for libWWW
   RICH HYPERTEXT OBJECT

   This is the C interface to the Objective-C (or whatever) HyperText class.
 */

#ifndef HTEXT_H
#define HTEXT_H

#include "HTAnchor.h"
#include "HTStream.h"
#include "../src/mo-www.h"

/*                      Creation and deletion
**
**      Create hypertext object                     HText_new
*/
extern HText * HText_new PARAMS(());

/*      Free hypertext object                       HText_free
*/
extern void     HText_free PARAMS((HText * me));


/*                      Object Building methods
**                      -----------------------
**
**      These are used by a parser to build the text in an object
**      HText_beginAppend must be called, then any combination of other
**      append calls, then HText_endAppend. This allows optimised
**      handling using buffers and caches which are flushed at the end.
*/
extern void HText_beginAppend PARAMS((HText * text));

extern void HText_endAppend PARAMS((HText * text));
extern void HText_doAbort PARAMS((HText * text));

/*      Add one character */
extern void HText_appendCharacter PARAMS((HText * text, char ch));

/*      Add a zero-terminated string */
extern void HText_appendText PARAMS((HText * text, WWW_CONST char * str));
/*      Add a block.  */
extern void HText_appendBlock PARAMS((HText * text, WWW_CONST char * str, int len));

/*      New Paragraph */
extern void HText_appendParagraph PARAMS((HText * text));

/*      Dump diagnostics to stderr */

extern char *HText_getText (HText *me);
extern int HText_getTextLength (HText *me);
extern char **HText_getPtrToText (HText *me);

/*              Browsing functions
**              ------------------
*/

/*      Bring to front and highlight it */

extern HT_BOOL HText_select PARAMS((HText * text));

#endif /* HTEXT_H */
