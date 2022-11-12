/* Rich Hypertext object for libWWW RICH HYPERTEXT OBJECT

   This is the C interface HyperText class.
*/

#ifndef HTEXT_H
#define HTEXT_H

/* Bare minimum. */
struct _HText {
	char *expandedAddress;
	char *simpleAddress;

/* This is what we should parse and display; it is *not* safe to free. */
	char *htmlSrc;
	int srcalloc;    /* amount of space allocated */
	int srclen;      /* amount of space used , and is the len of htmlSrc*/
};

typedef struct _HText HText;    /* Normal Library */

extern HText * HText_new ();	/* Create hypertext object */

extern void     HText_free (HText * me); /* Free hypertext object */


/*                      Object Building methods
**
**      These are used by a parser to build the text in an object
**      HText_beginAppend must be called, then any combination of other
**      append calls, then HText_endAppend. This allows optimised
**      handling using buffers and caches which are flushed at the end.
*/
extern void HText_beginAppend (HText * text);
extern void HText_endAppend (HText * text);
extern void HText_doAbort (HText * text);
extern void HText_appendCharacter(HText *text, char ch); /* Add one character */
extern void HText_appendText(HText *text, const char * str); /* Add a string */
extern void HText_appendBlock(HText *, const char *, int);/* Add block.*/

extern char *HText_getText (HText *me);
extern int HText_getTextLength (HText *me);

#endif /* HTEXT_H */
