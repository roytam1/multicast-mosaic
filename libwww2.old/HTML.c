/*		Structured stream to Rich hypertext converter
**
**	This generates a hypertext object.  It converts from the
**	structured stream interface from HTML events into the style-
**	oriented interface of the HText.h interface.  This module is
**	only used in clients and should not be linked into servers.
**
**	Override this module if you are making a new GUI browser.
*/

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


/*		Expanding entities
**	(In fact, they all shrink!)
*/

static void HTML_put_entity (HTStructured *me, int entity_number, caddr_t appd)
{
    HTML_put_string(me, ISO_Latin1[entity_number],appd);/* @@ Other representations */
}
