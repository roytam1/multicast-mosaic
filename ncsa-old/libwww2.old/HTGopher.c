/*			GOPHER ACCESS
**
** History:
**	26 Sep 90	Adapted from other accesses (News, HTTP) TBL
**	29 Nov 91	Downgraded to C, for portable implementation.
*/

#define GOPHER_PORT 70		/* See protocol spec */
#define BIG 1024		/* Bug */
#define LINE_LENGTH 256		/* Bug */

/*	Gopher entity types: */
#define GOPHER_TEXT		'0'
#define GOPHER_MENU		'1'
#define GOPHER_CSO		'2'
#define GOPHER_ERROR		'3'
#define GOPHER_MACBINHEX	'4'
#define GOPHER_PCBINHEX		'5'
#define GOPHER_UUENCODED	'6'
#define GOPHER_INDEX		'7'
#define GOPHER_TELNET		'8'
#define GOPHER_BINARY           '9'
#define GOPHER_DUPLICATE	'+'

#define GOPHER_GIF              'g'
#define GOPHER_IMAGE            'I'
#define GOPHER_TN3270           'T'

#define GOPHER_HTML		'h'		/* HTML */
#define GOPHER_WWW		'w'		/* W3 address */
#define GOPHER_SOUND            's'

#define GOPHER_PLUS_IMAGE       ':'
#define GOPHER_PLUS_MOVIE       ';'
#define GOPHER_PLUS_SOUND       '<'


static int s;					/* Socket for GopherHost */

/*	Matrix of allowed characters in filenames */

static HT_BOOL acceptable[256];
static HT_BOOL acceptable_inited = NO;

static void init_acceptable (void)
{
    unsigned int i;
    char * good = 
      "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789./-_$";
    for(i=0; i<256; i++) acceptable[i] = NO;
    for(;*good; good++) acceptable[(unsigned int)*good] = YES;
    acceptable_inited = YES;
}

static const char hex[17] = "0123456789abcdef";

/*	Decode one hex character */

static char from_hex (char c)
{
    return 		  (c>='0')&&(c<='9') ? c-'0'
			: (c>='A')&&(c<='F') ? c-'A'+10
			: (c>='a')&&(c<='f') ? c-'a'+10
			:		       0;
}

/*	Paste in an Anchor
**
**	The title of the destination is set, as there is no way
**	of knowing what the title is when we arrive.
** On entry,
**	HT 	is in append mode.
**	text 	points to the text to be put into the file, 0 terminated.
**	addr	points to the hypertext refernce address 0 terminated.
*/
static void write_anchor(const char *text, const char *addr,
                                char *image_text, caddr_t appd)
{
    PUTS ("<A HREF=\"");
    PUTS (addr);
    PUTS ("\">");

    /* Throw in an inlined image, if one has been requested. */
    if (image_text) {
        PUTS ("<IMG SRC=\"");
        PUTS (image_text);
        PUTS ("\"> ");
    }
    PUTS(text);
    PUTS("</A>");
}

/*	Parse a Gopher Menu document */

static int parse_menu (
	const char *	arg,
	HTParentAnchor *	anAnchor,
	caddr_t			appd)
{
  char gtype;
  char ch;
  char line[BIG];
  char address[BIG];
  char *name, *selector;		/* Gopher menu fields */
  char *host;
  char *port;
  char *p = line;
  extern int interrupted_in_htgetcharacter;
  
#define TAB 		'\t'
#define HEX_ESCAPE 	'%'

  HTProgress ("Retrieving Gopher menu.",appd);

  PUTS("<H1>Gopher Menu</H1>\n");
  
  PUTS("\n<DL>\n");
  while ((ch=HTGetCharacter (appd)) != (char)EOF) {
      if (interrupted_in_htgetcharacter) {
          (*targetClass.handle_interrupt)(target, appd);
          return HT_INTERRUPTED;
        }
      if (ch != LF) {
          *p = ch;		/* Put character in line */
          if (p< &line[BIG-1]) p++;
        } else {
          *p++ = 0;		/* Terminate line */
          p = line;		/* Scan it to parse it */
          port = 0;		/* Flag "not parsed" */
          gtype = *p++;
          
          /* Break on line with a dot by itself */
          if ((gtype=='.') && ((*p=='\r') || (*p==0))) 
            break;
          
          if (gtype && *p) {
              name = p;
              selector = strchr(name, TAB);
              PUTS("\n<DD> ");
              if (selector) {
                  *selector++ = 0;	/* Terminate name */
                  host = strchr(selector, TAB);
                  if (host) {
                      *host++ = 0;	/* Terminate selector */
                      port = strchr(host, TAB);
                      if (port) {
                          char *junk;
                          port[0] = ':';	/* delimit host a la W3 */
                          junk = strchr(port, TAB);
                          if (junk) 
                            *junk++ = 0;	/* Chop port */
                          if ((port[1]=='0') && (!port[2]))
                            port[0] = 0;	/* 0 means none */
			} /* no port */
		    } /* host ok */
		} /* selector ok */
	    } /* gtype and name ok */
          
          if (gtype == GOPHER_WWW) {	/* Gopher pointer to W3 */
              write_anchor(name, selector, "internal-gopher-text",appd);
	    } else if (port) {		/* Other types need port */
              if (gtype == GOPHER_TELNET) {
                  if (*selector) 
                    sprintf(address, "telnet://%s@%s/",
                            selector, host);
                  else 
                    sprintf(address, "telnet://%s/", host);
                } else if (gtype == GOPHER_TN3270) {
                  if (*selector) 
                    sprintf(address, "tn3270://%s@%s/",
                            selector, host);
                  else 
                    sprintf(address, "tn3270://%s/", host);
                } else {			/* If parsed ok */
                  char *q;
                  unsigned char *p;
                  sprintf(address, "//%s/%c", host, gtype);
                  q = address+ strlen(address);
                  for(p=(unsigned char *)selector; *p; p++) {	/* Encode selector string */
                      if (acceptable[*p]) *q++ = *p;
                      else {
                          *q++ = HEX_ESCAPE;	/* Means hex coming */
                          *q++ = hex[(*p) >> 4];
                          *q++ = hex[(*p) & 15];
			}
		    }
                  *q++ = 0;			/* terminate address */
		}
              /* Error response from Gopher doesn't deserve to
                 be a hyperlink. */
              if (strcmp (address, "//error.host:1/0") != 0 &&
                  strcmp (address, "//error/0error") != 0 &&
                  strcmp (address, "//:/0") != 0 &&
                  gtype != GOPHER_ERROR) {
                  switch (gtype) {
                    case GOPHER_MENU:
                      write_anchor(name, address, "internal-gopher-menu",appd);
                      break;
                    case GOPHER_TEXT:
                      write_anchor(name, address, "internal-gopher-text",appd);
                      break;
                    case GOPHER_INDEX:
                    case GOPHER_CSO:
                      write_anchor(name, address, "internal-gopher-index",appd);
                      break;
                    case GOPHER_IMAGE:
                    case GOPHER_GIF:
                    case GOPHER_PLUS_IMAGE:
                      write_anchor(name, address, "internal-gopher-image",appd);
                      break;
                    case GOPHER_SOUND:
                    case GOPHER_PLUS_SOUND:
                      write_anchor(name, address, "internal-gopher-sound",appd);
                      break;
                    case GOPHER_PLUS_MOVIE:
                      write_anchor(name, address, "internal-gopher-movie",appd);
                      break;
                    case GOPHER_TELNET:
                    case GOPHER_TN3270:
                      write_anchor(name, address, "internal-gopher-telnet",appd);
                      break;
                    case GOPHER_BINARY:
                    case GOPHER_MACBINHEX:
                    case GOPHER_PCBINHEX:
                    case GOPHER_UUENCODED:
                      write_anchor(name, address, "internal-gopher-binary",appd);
                      break;
                    default:
                      write_anchor(name, address, "internal-gopher-unknown",appd);
                      break;
                  }
              } else {
                  /* Good error handling??? */
                  PUTS(line);
              }
	  } else { /* parse error */
              if (wWWParams.trace) fprintf(stderr,
                                 "HTGopher: Bad menu item.\n");
              PUTS(line);
	  } /* parse error */
          p = line;	/* Start again at beginning of line */
      } /* if end of line */
  } /* Loop over characters */
  PUTS("\n<DL>\n");
  END_TARGET;
  FREE_TARGET;
  HTProgress ("Retrieved Gopher menu.",appd);
  return 1;
}

/*	Display a Gopher Index document */

static void display_index(const char *arg, HTParentAnchor *anAnchor,
	caddr_t appd)
{
  PUTS("<H1>Searchable Gopher Index</H1> <ISINDEX>");

  END_TARGET;
  FREE_TARGET;
  return;
}


/*	Display a Gopher CSO document */

static void display_cso(const char *arg, HTParentAnchor *anAnchor,
	caddr_t appd)
{
  PUTS("<H1>Searchable CSO Phonebook</H1> <ISINDEX>");
  END_TARGET;
  FREE_TARGET;
  return;
}

/*	Parse a Gopher CSO document
 **
 **   Accepts an open socket to a CSO server waiting to send us
 **   data and puts it on the screen in a reasonable manner.
 **
 **   Perhaps this data can be automatically linked to some
 **   other source as well???
 **
 **   Hacked into place by Lou Montulli@ukanaix.cc.ukans.edu
 */
static int parse_cso (const char *	arg,
                       HTParentAnchor *anAnchor,
			caddr_t		appd)
{
  char ch;
  char line[BIG];
  char *p = line;
  char *second_colon, last_char='\0';
  extern int interrupted_in_htgetcharacter;

  HTProgress ("Retrieving CSO search results.",appd);

  PUTS("<H1>CSO Search Results</H1>\n<PRE>");

  /* start grabbing chars from the network */
  while ((ch=HTGetCharacter (appd)) != (char)EOF) {
      if (interrupted_in_htgetcharacter) {
          if (wWWParams.trace)
            fprintf (stderr, "parse_cso: picked up interrupt in htgc\n");
          (*targetClass.handle_interrupt)(target,appd);
          return HT_INTERRUPTED;
        }
      if (ch != '\n') {
          *p = ch;		/* Put character in line */
          if (p< &line[BIG-1]) p++;
        } else {
          *p++ = 0;		/* Terminate line */
          p = line;		/* Scan it to parse it */

	/* OK we now have a line in 'p' lets parse it and print it */
          
          /* Break on line that begins with a 2. It's the end of
           * data.
	   */
          if (*p == '2')
		break;

	  /*  lines beginning with 5 are errors, 
	   *  print them and quit
	   */
          if (*p == '5') {
            PUTS("<H2>");
            PUTS(p+4);
            PUTS("</H2>\n");
            break;
          }

	  if(*p == '-') {
	     /*  data lines look like  -200:#:
              *  where # is the search result number and can be multiple 
	      *  digits (infinate?)
              *  find the second colon and check the digit to the
              *  left of it to see if they are diferent
              *  if they are then a different person is starting. 
	      *  make this line an <h2>
              */

		/* find the second_colon */
             second_colon = strchr( strchr(p,':')+1, ':');

             if(second_colon != NULL) {  /* error check */

                 if (*(second_colon-1) != last_char)   /* print seperator */ {
                     PUTS("</PRE>\n");
                     PUTS("<H2>");
                   }

		 /* right now the record appears with the alias (first line)
		  * as the header and the rest as <pre> text
		  * It might look better with the name as the
		  * header and the rest as a <ul> with <li> tags
		  * I'm not sure whether the name field comes in any
		  * special order or if its even required in a record,
		  * so for now the first line is the header no matter
		  * what it is (it's almost always the alias)
		  * A <dl> with the first line as the <DT> and
		  * the rest as some form of <DD> might good also?
		  */

                 /* print data */
                 PUTS(second_colon+1);
                 PUTS("\n");

                 if (*(second_colon-1) != last_char)   /* end seperator */ {
                     PUTS("</H2>\n");
                     PUTS("<PRE>");
                   }

		  /* save the char before the second colon
		   * for comparison on the next pass
		   */
                 last_char =  *(second_colon-1) ;

	     } /* end if second_colon */
	  } /* end if *p == '-' */
        } /* if end of line */
      
    } /* Loop over characters */
  if (interrupted_in_htgetcharacter) {
      if (wWWParams.trace)
        fprintf (stderr, "parse_cso: picked up interrupt in htgc\n");
      (*targetClass.handle_interrupt)(target,appd);
      return HT_INTERRUPTED;
    }
  
  /* end the text block */
  PUTS("\n<PRE>");
  END_TARGET;
  FREE_TARGET;

  HTProgress ("Retrieved CSO search results.",appd);

  return 1;  /* all done */
} /* end of procedure */



/*		De-escape a selector into a command
**	The % hex escapes are converted. Otheriwse, the string is copied.
*/
static void de_escape (char *command, const char *selector)
{
  char *p;

  if (!selector)
    return;
  if (!command)
    return;
  p = strdup (selector);
  HTUnEscape (p);
  strcpy (command, p);
  free (p);
}

/*		Load by name
**	 Bug:	No decoding of strange data types as yet.
*/
int HTLoadGopher( char *arg, HTParentAnchor *anAnchor,
	HTFormat format_out, caddr_t appd)
{
  char *command;			/* The whole command */
  int status;				/* tcp return */
  char gtype;				/* Gopher Node type */
  char * selector;			/* Selector string */
  int rv = 0;
  
  if (!acceptable_inited) init_acceptable();
  
  if (!arg) 
    return -3;		/* Bad if no name sepcified	*/
  if (!*arg) 
    return -2;		/* Bad if name had zero length	*/
  
  /* Get entity type, and selector string.  */        
  {
    char * p1 = URLParse(arg, "", PARSE_PATH|PARSE_PUNCTUATION);
    gtype = '1';		/* Default = menu */
    selector = p1;
    if ((*selector++=='/') && (*selector)) {	/* Skip first slash */
        gtype = *selector++;			/* Pick up gtype */
      }
    if (gtype == GOPHER_INDEX) {
        char * query;
        query = strchr(selector, '?');	/* Look for search string */
        if (!query || !query[1]) {		/* No search required */
            target = HTML_new(anAnchor, format_out, NULL);
            targetClass = *target->isa;
            display_index(arg, anAnchor,appd);	/* Display "cover page" */
            return HT_LOADED;			/* Local function only */
          }
        *query++ = 0;			/* Skip '?' 	*/
        HTUnEscape (query);
        command = (char*)malloc(strlen(selector)+ 1 + strlen(query)+ 2 + 1);
        
        de_escape(command, selector);
        
        strcat(command, "\t");
        strcat(command, query);
      } else if (gtype == GOPHER_CSO) {
        char * query;
        query = strchr(selector, '?');      /* Look for search string */
        if (!query || !query[1]) {          /* No search required */
            target = HTML_new(anAnchor, format_out, NULL);
            targetClass = *target->isa;
            display_cso(arg, anAnchor,appd);     /* Display "cover page" */
            return HT_LOADED;                       /* Local function only */
          }
        *query++ = 0;                       /* Skip '?'     */
        HTUnEscape (query);
        command = (char*)malloc(strlen("query")+ 1 + strlen(query)+ 2 + 1);
        
        de_escape(command, selector);
        
        strcpy(command, "query ");
        strcat(command, query);
      } else {				/* Not index */
        command = (char*)malloc(strlen(selector)+2+1);
        de_escape(command, selector);
      }
    free(p1);
  }
  
  /* Patch security hole. */
  {
    char *tmp;
    for (tmp = command; *tmp; tmp++)
      if (*tmp == CR || *tmp == LF)
        *tmp = ' ';
      /*  "Fixed security hole: '%s'\n", command */
    *tmp++ = CR;
    *tmp++ = LF;
    *tmp++ = 0;
    if (wWWParams.trace)
      fprintf (stderr, "Prepared command: '%s'\n", command);
  }

  status = HTDoConnect (arg, 70, &s,appd);
  if (status<0) {
      return HT_NOT_LOADED;
    }
  
  HTInitInput(s);		/* Set up input buffering */
  
/* "HTGopher: Connected, writing command `%s' to socket %d\n", command, s); */
  
  status = write(s, command, (int)strlen(command));
  free(command);
  if (status<0) {
      close (s);
      return HT_NOT_LOADED;
    }
  
  /* Now read the data from the socket: */    
  switch (gtype) {
      int compressed;
      HTAtom *enc;
            
    case GOPHER_MENU:
    case GOPHER_INDEX:
      target = HTML_new(anAnchor, format_out, NULL);
      targetClass = *target->isa;
      rv = parse_menu(arg, anAnchor,appd);
      break;

    case GOPHER_CSO:
      target = HTML_new(anAnchor, format_out, NULL);
      targetClass = *target->isa;
      rv = parse_cso(arg, anAnchor,appd);
      break;
      
    case GOPHER_MACBINHEX:
    case GOPHER_PCBINHEX:
    case GOPHER_UUENCODED:
    case GOPHER_BINARY:
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(WWW_BINARY, format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket(HTFileName2ct(arg, WWW_BINARY, &compressed),
                           format_out, anAnchor, s, 0,appd);
      break;

    case GOPHER_GIF:
    case GOPHER_IMAGE:
    case GOPHER_PLUS_IMAGE:
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(HTAtom_for ("image/gif"), 
                           format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket(HTFileName2ct (arg, "image/gif", 
                                         &compressed),
                           format_out, anAnchor, s, 0,appd);
      break;

    case GOPHER_SOUND:
    case GOPHER_PLUS_SOUND:
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(HTAtom_for ("audio/basic"), 
                           format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket(HTFileName2ct (arg, "audio/basic", &compressed),
                           format_out, anAnchor, s, 0,appd);
      break;

    case GOPHER_PLUS_MOVIE:
      /* Sigh..... */
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(HTAtom_for ("video/mpeg"), 
                           format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket(HTFileName2ct (arg, "video/mpeg", &compressed),
                           format_out, anAnchor, s, 0,appd);
      break;

    case GOPHER_HTML:
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(WWW_HTML, format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket(HTFileName2ct (arg, WWW_HTML, &compressed),
                           format_out, anAnchor, s, 0,appd);
      break;
      
    case GOPHER_TEXT:
    default:			/* @@ parse as plain text */
      if (!wWWParams.tweak_gopher_types)
        rv = HTParseSocket(WWW_PLAINTEXT, format_out, anAnchor, s, 0,appd);
      else
        rv = HTParseSocket
          (HTFileName2ct (arg, WWW_PLAINTEXT, &compressed),
           format_out, anAnchor, s, 0,appd);
      break;
    } /* switch(gtype) */
  
  close(s);
      return HT_LOADED;
}