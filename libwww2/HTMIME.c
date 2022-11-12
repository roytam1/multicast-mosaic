/*			MIME Message Parse			HTMIME.c
**			==================
**
**	This is RFC 1341-specific code.
**	The input stream pushed into this parser is assumed to be
**	stripped on CRs, ie lines end with LF, not CR LF.
**	(It is easy to change this except for the body part where
**	conversion can be slow.)
**
** History:
**	   Feb 92	Written Tim Berners-Lee, CERN
*/

#include <stdio.h>
#include <sys/types.h>

#include "HText.h"
#include "HTMIME.h"		/* Implemented here */
#include "HTAlert.h"
#include "HTFile.h"
#include "HTAAUtil.h"
#include "tcp.h"
#include "HTParams.h"		/* params from X resources */

#if defined(KRB4) || defined(KRB5)              /* ADC, 6/28/95 */
#define HAVE_KERBEROS
#endif

extern int securityType;

/* This is UGLY. */
char *redirecting_url = NULL;
extern int loading_length;
extern int noLength;
char  *HTTP_last_modified;
char  *HTTP_expires;

/*		MIME Object */
typedef enum _MIME_state {
  BEGINNING_OF_LINE,
  CONTENT_,
  CONTENT_T,
  CONTENT_TRANSFER_ENCODING,
  CONTENT_TYPE,
  CONTENT_ENCODING,
  CONTENT_LENGTH,
  EXPIRES,
  E,
  EX,
  L,
  LOCATION,
  LAST_MODIFIED,
  EXTENSION,
  SKIP_GET_VALUE,		/* Skip space then get value */
  GET_VALUE,		        /* Get value till white space */
  JUNK_LINE,		        /* Ignore the rest of this folded line */
  NEWLINE,		        /* Just found a LF .. maybe continuation */
  CHECK,			/* check against check_pointer */
  MIME_TRANSPARENT,	        /* put straight through to target ASAP! */
  MIME_IGNORE		        /* ignore entire file */
  /* TRANSPARENT and IGNORE are defined as stg else in _WINDOWS */
#ifdef HAVE_KERBEROS
  ,WWW_AUTHENTICATE             /* for kerberos mutual authentication */
#endif
} MIME_state;

#define VALUE_SIZE 8192		/* @@@@@@@ Arbitrary? */
struct _HTStream 
{
  WWW_CONST HTStreamClass *	isa;
  
  MIME_state		state;		/* current state */
  MIME_state		if_ok;		/* got this state if match */
  MIME_state		field;		/* remember which field */
  MIME_state		fold_state;	/* state on a fold */
  WWW_CONST char *	check_pointer;	/* checking input */
  
  char *		value_pointer;	/* storing values */
  char 			value[VALUE_SIZE];
  
  HTParentAnchor *	anchor;		/* Given on creation */
  HTStream *		sink;		/* Given on creation */
  
  char *	        boundary;	/* For multipart */
  
  HTFormat		encoding;	/* Content-Transfer-Encoding */
  char *                compression_encoding;
  int                   content_length;
  int                   header_length;  /* for io accounting -bjs */
  HTFormat		format;		/* Content-Type */
  HTStream *		target;		/* While writing out */
  HTStreamClass		targetClass;
  
  HTAtom *		targetRep;	/* Converting into? */
  
  char *                location;
  char *		expires;
  char *		last_modified;

  int interrupted;
};


/*_________________________________________________________________________
**
**			A C T I O N 	R O U T I N E S
*/

/*	Character handling
**
**	This is a FSM parser which is tolerant as it can be of all
**	syntax errors.  It ignores field names it does not understand,
**	and resynchronises on line beginnings.
*/

PRIVATE void HTMIME_put_character(HTStream *me, char c, caddr_t appd)
{
#ifdef HAVE_KERBEROS
    static int got_kerb = 0;
    static HTAAScheme kscheme;
    extern int validate_kerberos_server_auth();
#endif


    if(me->state==MIME_TRANSPARENT){
	(*me->targetClass.put_character)(me->target, c,appd);    /* MUST BE FAST */
	return;
    } else {
	me->header_length ++; /* bjs - update this first */
    }

  switch(me->state) {
    case MIME_IGNORE:
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_put_character] Got MIME_IGNORE; returning...\n");
      return;
      
/*    case MIME_TRANSPARENT:*/
      
    case NEWLINE:
      if (c != '\n' && WHITE(c)) {
          /* Folded line */
          me->state = me->fold_state;	/* pop state before newline */
          break;
        }
      /* else Falls through */
      
    case BEGINNING_OF_LINE:
      switch(c) {
        case 'c':
        case 'C':
          me->check_pointer = "ontent-";
          me->if_ok = CONTENT_;
          me->state = CHECK;
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Got C at beginning of line; checking for 'ontent-'\n");
          break;

	/* SWP -- 7/10/95 */
	case 'E':	/* Extension or Expires */
	case 'e':
          {
		me->state = E;
		if (wWWParams.trace)
		fprintf (stderr, 
			 "[MIME] Got E at beginning of line; checking for 'X'\n");
	  }
	  break;
        case 'l':
        case 'L':
          me->state = L;
          if (wWWParams.trace)
            fprintf (stderr,
                     "[MIME] Got L at beginning of line\n");
          break;

#ifdef HAVE_KERBEROS
          /*  for kerberos mutual authentication  */
        case 'w':
        case 'W':
          me->check_pointer = "ww-authenticate:";
          me->if_ok = WWW_AUTHENTICATE;
          me->state = CHECK;
          if (wWWParams.trace)
              fprintf(stderr,
                  "[MIME] Got W at beginning of line; checking for 'ww-authenticate'\n");
          break;
#endif

        case '\n':			/* Blank line: End of Header! */
          {
            int compressed = COMPRESSED_NOT;
            if (wWWParams.trace) 
              fprintf (stderr,
                       "HTMIME: DOING STREAMSTACK: MIME content type is %s, converting to %s\n",
                       HTAtom_name(me->format), HTAtom_name(me->targetRep));
            if (wWWParams.trace)
              fprintf (stderr,
                       "                           Compression encoding '%s'\n",
                       (!me->compression_encoding || !*me->compression_encoding?"Undefined":me->compression_encoding));
            if (me->compression_encoding)
              {
                if (strcmp (me->compression_encoding, "x-compress") == 0)
                  {
                    compressed = COMPRESSED_BIGZ;
                  }
                else if (strcmp (me->compression_encoding, "x-gzip") == 0)
                  {
                    compressed = COMPRESSED_GNUZIP;
                  }
                else
                  {
                    if (wWWParams.trace)
                      fprintf (stderr, "HTMIME: Unknown compression_encoding '%s'\n",
                               me->compression_encoding);
                  }
              }

            if (wWWParams.trace)
              fprintf (stderr, "HTMIME: compressed == %d\n", compressed);
            me->target = HTStreamStack(me->format, me->targetRep, compressed,
                                       me->sink, me->anchor,appd);
            if (!me->target) 
              {
                if (wWWParams.trace) 
                  {
                    fprintf(stderr, "MIME: Can't translate! ** \n");
                    fprintf(stderr, "HTMIME: Defaulting to HTML.\n");
                  }
                /* Default to HTML. */
                me->target = HTStreamStack(HTAtom_for("text/html"),
                                           me->targetRep,
                                           compressed,
                                           me->sink,
                                           me->anchor,appd);
              }
            if (me->target) 
              {
                me->targetClass = *me->target->isa;
		/* Check for encoding and select state from there @@ */
                /* From now push straigh through */
                if (wWWParams.trace)
                  fprintf (stderr, "[MIME] Entering MIME_TRANSPARENT\n");
                me->state = MIME_TRANSPARENT; 
		/* bjs note: header is now completely read */
		
              } else {
                /* This is HIGHLY EVIL -- the browser WILL BREAK
                   if it ever reaches here.  Thus the default to
                   HTML above, which should always happen... */
                if (wWWParams.trace) 
                  fprintf (stderr, "MIME: HIT HIGHLY EVIL!!! ***\n");
                me->state = MIME_IGNORE;		/* What else to do? */
              }
          }
          break;

	default:
          if (wWWParams.trace)
            fprintf (stderr, "[MIME] Got nothing at beginning of line; bleah.\n");
          goto bad_field_name;
	} /* switch on character */
      break;
      
    case CHECK:				/* Check against string */
      if (TOLOWER(c) == *(me->check_pointer)++) {
          if (!*me->check_pointer) 
            me->state = me->if_ok;
        } else {		/* Error */
          if (wWWParams.trace) 
            fprintf(stderr,
                    "HTMIME: Bad character `%c' found where `%s' expected\n",
                    c, me->check_pointer - 1);
          goto bad_field_name;
        }
      break;

    case CONTENT_:
      if (wWWParams.trace)
        fprintf (stderr, 
                 "[MIME] in case CONTENT_\n");
      switch(c) {
	case 't':
	case 'T':
          me->state = CONTENT_T;
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Was CONTENT_, found T, state now CONTENT_T\n");
          break;
	case 'e':
	case 'E':
          me->check_pointer = "ncoding:";
          me->if_ok = CONTENT_ENCODING;
          me->state = CHECK;
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Was CONTENT_, found E, checking for 'ncoding:'\n");
          break;
          
	case 'l':
	case 'L':
          me->check_pointer = "ength:";
          me->if_ok = CONTENT_LENGTH;
          me->state = CHECK;
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Was CONTENT_, found L, checking for 'ength:'\n");
          break;
          
	default:
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Was CONTENT_, found nothing; bleah\n");
          goto bad_field_name;
          
	} /* switch on character */
      break;
      
    case CONTENT_T:
      if (wWWParams.trace)
        fprintf (stderr, 
                 "[MIME] in case CONTENT_T\n");
      switch(c) {
	case 'r':
	case 'R':
          me->check_pointer = "ansfer-encoding:";
          me->if_ok = CONTENT_TRANSFER_ENCODING;
          me->state = CHECK;
          if (wWWParams.trace)
            fprintf (stderr, 
                     "[MIME] Was CONTENT_T; going to check for ansfer-encoding:\n");
          break;
          
	case 'y':
	case 'Y':
          me->check_pointer = "pe:";
          me->if_ok = CONTENT_TYPE;
          me->state = CHECK;
          if (wWWParams.trace)
            fprintf (stderr, "[MIME] Was CONTENT_T; going to check for pe:\n");
          break;
          
	default:
          if (wWWParams.trace)
            fprintf (stderr,
                     "[MIME] Was CONTENT_T; found nothing; bleah\n");
          goto bad_field_name;
        } /* switch on character */
      break;

    case L:
      if (wWWParams.trace)
        fprintf (stderr,
                 "[MIME] in case L\n");
      switch(c)
        {
		case 'a':
		case 'A':
			me->check_pointer = "st-modified:";
			me->if_ok = LAST_MODIFIED;
			me->state = CHECK;
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was L; going to check for st-modified:\n");
			break;

		case 'o':
		case 'O':
			me->check_pointer = "cation:";
			me->if_ok = LOCATION;
			me->state = CHECK;
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was L; going to check for ocation:\n");
			break;

		default:
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was L; found nothing; bleah\n");
			goto bad_field_name;
	} /* switch on character */
      break;

    case E:
      if (wWWParams.trace)
        fprintf (stderr,
                 "[MIME] in case E\n");
      switch(c)
        {
		case 'x':
		case 'X':
			me->state = EX;
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was EX; going to check for EXP or EXT:\n");
			break;

		default:
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was E; found nothing; bleah\n");
			goto bad_field_name;
	} /* switch on character */
      break;

    case EX:
      if (wWWParams.trace)
        fprintf (stderr,
                 "[MIME] in case EX\n");
      switch(c)
        {
		case 'p':
		case 'P':
			me->check_pointer = "ires";
			me->if_ok = EXPIRES;
			me->state = CHECK;
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was EXP; going to check for 'ires'\n");
			break;

		case 't':
		case 'T':
			me->check_pointer = "ension:";
			me->if_ok = EXTENSION;
			me->state = CHECK;
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was EXT; going to check for 'ension:'\n");
			break;

		default:
			if (wWWParams.trace)
				fprintf (stderr,
					 "[MIME] Was EX; found nothing; bleah\n");
			goto bad_field_name;
	} /* switch on character */
      break;

#ifdef HAVE_KERBEROS
    case WWW_AUTHENTICATE:
#endif
    case EXTENSION:
    case CONTENT_TYPE:
    case CONTENT_TRANSFER_ENCODING:
    case CONTENT_ENCODING:
    case CONTENT_LENGTH:
    case LOCATION:
    case EXPIRES:
    case LAST_MODIFIED:
      me->field = me->state;		/* remember it */
      me->state = SKIP_GET_VALUE;
      /* Fall through! (no break!) */
    case SKIP_GET_VALUE:
      if (c == '\n') 
        {
          me->fold_state = me->state;
          me->state = NEWLINE;
          break;
        }
      if (WHITE(c)) 
        break;	/* Skip white space */
      
      me->value_pointer = me->value;
      me->state = GET_VALUE;   
      /* Fall through to store first character */
      
    case GET_VALUE:
      if (WHITE(c)) 
        {
          /* End of field */
          *me->value_pointer = 0;
          switch (me->field) 
            {
            case CONTENT_TYPE:
              if (wWWParams.trace)
                fprintf (stderr, "[MIME_put_char] Got content-type value '%s'\n", me->value);
              /* Lowercase it. */
              {
                char *tmp;
                for (tmp = me->value; *tmp; tmp++)
                  *tmp = TOLOWER (*tmp);
              }
              if (wWWParams.trace)
                fprintf (stderr, "[MIME_put_char] Lowercased to '%s'\n", me->value);
              me->format = HTAtom_for(me->value);
              if (wWWParams.trace)
                fprintf (stderr, "[MIME_put_char] Got content-type value atom 0x%08x\n",
                         me->format);
              break;
	    case CONTENT_TRANSFER_ENCODING:
              me->encoding = HTAtom_for(me->value);
              if (wWWParams.trace)
                fprintf (stderr, 
                         "[MIME_put_char] Picked up transfer_encoding '%s'\n",
                         me->encoding);
              break;
            case CONTENT_ENCODING:
              me->compression_encoding = strdup (me->value);
              if (wWWParams.trace)
                fprintf (stderr, 
                         "[MIME_put_char] Picked up compression encoding '%s'\n", 
                         me->compression_encoding);
              break;
            case CONTENT_LENGTH:
              me->content_length = atoi (me->value);
              /* This is TEMPORARY. */
              loading_length = me->content_length;
	      noLength=0;
              if (wWWParams.trace)
                fprintf (stderr, 
                         "[MIME_put_char] Picked up content length '%d'\n", 
                         me->content_length);
              break;
            case EXPIRES:
		if (me->value_pointer < me->value + VALUE_SIZE - 1) {
			*me->value_pointer++ = c;
			*me->value_pointer = 0;
		} else {
			goto value_too_long;
		}
		if (me->expires)
			free(me->expires);
		me->expires = strdup(me->value);
		if (wWWParams.trace)
			fprintf(stderr,
				"[MIME_put_char] Picked up expires '%s'\n", me->value);
		break;
            case LAST_MODIFIED:
		if (me->value_pointer < me->value + VALUE_SIZE - 1) {
			*me->value_pointer++ = c;
			*me->value_pointer = 0;
		} else {
			goto value_too_long;
		}
		if (me->last_modified)
			free(me->last_modified);
		me->last_modified = strdup(me->value);
		if (wWWParams.trace)
			fprintf(stderr,
				"[MIME_put_char] Picked up last modified '%s'\n", me->value);
		break;
            case LOCATION:
		me->location = me->value;
		redirecting_url = strdup (me->location);
		if (wWWParams.trace)
			fprintf(stderr,
				"[MIME_put_char] Picked up location '%s'\n", me->location);
		break;

#ifdef HAVE_KERBEROS
            case WWW_AUTHENTICATE:
                /*
                 * msg from server looks like:
                 * WWW-Authenticate: KerberosV4 [strified ktext]
                 * also allowed: KerberosV5, KerbV4-Encrypted, KerbV5-Encrypted
                 *
                 * This code is ugly: we have to keep this got_kerb static around because
                 * the FSM isn't really designed to have fields with values that
                 * include whitespace.  got_kerb tells us that we've been in this code
                 * before, and that we saw the word "kerberos"
                 */
                if (wWWParams.trace) fprintf(stderr, "[MIME put char] picked up Auth. arg '%s'\n",
                                   me->value);
                if (got_kerb) {
                    validate_kerberos_server_auth(kscheme, me->value);
                    got_kerb = 0;       /* reset kerb state */
                    me->state = me->field;
		} else if (!strncasecmp(me->value, "kerb", 4)) {
                    if (0) {    /* just to get things started */
		    }
#ifdef KRB4
                    else if (!strncasecmp(me->value, "KerberosV4", 10)) {
                        kscheme = HTAA_KERBEROS_V4;
                        got_kerb = 1;
                        me->state = SKIP_GET_VALUE;
		    }
#endif
#ifdef KRB5
                    else if (!strncasecmp(me->value, "KerberosV5", 10)) {
                        kscheme = HTAA_KERBEROS_V5;
                        got_kerb = 1;
                        me->state = SKIP_GET_VALUE;
		    }
#endif
                    else {
                        fprintf(stderr, "Unrecognized field in WWW-Authenticate header\n");
                        me->state = me->field;
		    }

		}
                break;
#endif

            case EXTENSION:
		if (wWWParams.trace)
			fprintf (stderr, "[MIME_put_char] Got Extension value '%s'\n", me->value);
		/* Lowercase it. */
		{
                char *tmp;
			for (tmp = me->value; *tmp; tmp++)
				*tmp = TOLOWER (*tmp);
		}
		if (wWWParams.trace)
			fprintf (stderr, "[MIME_put_char] Lowercased to '%s'\n", me->value);
		switch(*(me->value)) {
			case 'd': /*Domain*/
				if (!strcmp(me->value,"domain-restricted")) {
					securityType=HTAA_DOMAIN;
					if (wWWParams.trace)
						fprintf (stderr, "[MIME_put_char] Domain restricted extension header found.\n");
					break;
				}
				/*fall through*/
			default: /*Unknown*/
				if (wWWParams.trace)
					fprintf (stderr, "[MIME_put_char] Unknown extension header: '%s'\n", me->value);
				me->state=me->field;
				break;
		}
		break;

	    default:		/* Should never get here */
              break;
	    }
	} else {
          if (me->value_pointer < me->value + VALUE_SIZE - 1) {
              *me->value_pointer++ = c;
              break;
            } else {
              goto value_too_long;
	    }
	}
      /* Fall through */
      
    case JUNK_LINE:
      if (c == '\n') {
          me->state = NEWLINE;
          me->fold_state = me->state;
	}
      break;
      
    } /* switch on state*/
  
  return;
  
 value_too_long:
  if (wWWParams.trace) fprintf(stderr,
                     "HTMIME: *** Syntax error. (string too long)\n");
  
 bad_field_name:				/* Ignore it */
  me->state = JUNK_LINE;
  return;
}

/*	String handling
**
**	Strings must be smaller than this buffer size.
*/
PRIVATE void HTMIME_put_string (HTStream *me, WWW_CONST char*s, caddr_t appd)
{
  WWW_CONST char * p;
  if (wWWParams.trace)
    fprintf (stderr, "[HTMIME_put_string] Putting '%s'\n", s);
  if (me->state == MIME_TRANSPARENT)		/* Optimisation */
    {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_put_string] Doing transparent put_string\n");
      (*me->targetClass.put_string)(me->target,s,appd);
    }
  else if (me->state != MIME_IGNORE)
    {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_put_string] Doing char-by-char put_character\n");
      for (p=s; *p; p++) 
        HTMIME_put_character(me, *p,appd);
    } else {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_put_string] DOING NOTHING!\n");
    }
  return;
}


/*	Buffer write.  Buffers can (and should!) be big.  */
PRIVATE void HTMIME_write(HTStream *me, WWW_CONST char*s, int l, caddr_t appd)
{
  WWW_CONST char * p;
  if (wWWParams.trace)
    fprintf (stderr, "[HTMIME_write] Putting %d bytes\n", l);
  if (me->state == MIME_TRANSPARENT)		/* Optimisation */
    {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_write] Doing transparent put_block\n");
      (*me->targetClass.put_block)(me->target, s, l,appd);
    } else if (me->state != MIME_IGNORE) {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_write] Doing char-by-char put_character\n");

      for (p=s; p < s+l; p++) 
        HTMIME_put_character(me, *p,appd);
    } else {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_write] DOING NOTHING!\n");
    }
  return;
}

/*	Free an HTML object */
PRIVATE void HTMIME_free (HTStream *me, caddr_t appd)
{
  if (!me->target) {
      if (wWWParams.trace)
        fprintf (stderr, "[HTMIME_free] Caught case where we didn't get a target.\n");
      if (wWWParams.trace)
        fprintf (stderr, "  me 0x%08x, me->target 0x%08x\n", me, me->target);
      me->format = HTAtom_for ("text/html");
      me->target = HTStreamStack(me->format, me->targetRep, 0,
                                 me->sink, me->anchor,appd);
      if (wWWParams.trace)
        fprintf (stderr, "  me->target->isa 0x%08x\n", me->target->isa);
      if (wWWParams.trace)
        fprintf (stderr, "  *me->target->isa 0x%08x\n", *me->target->isa);
      me->targetClass = *me->target->isa;
      (*me->targetClass.put_string) (me->target, "<H1>ERROR IN HTTP/1.0 RESPONSE</H1> The remote server returned a HTTP/1.0 response that Mosaic's MIME parser could not understand.  Please contact the server maintainer.<P> Sorry for the inconvenience,<P> <ADDRESS>The Management</ADDRESS>",appd);
      securityType=HTAA_UNKNOWN;
    } 
  if (me->target) 
    (*me->targetClass.free)(me->target,appd);
      
  if (me->expires) {
       char *p;

       if (HTTP_expires)
       free(HTTP_expires);
       HTTP_expires = me->expires;
       for (p = HTTP_expires + strlen(HTTP_expires) - 1;
          p > HTTP_expires && isspace(*p); p--)
       {
         *p = '\0';
       }

     }

   if (me->last_modified) {
       char *p;

       if (HTTP_last_modified)
       free(HTTP_last_modified);
       HTTP_last_modified = me->last_modified;
       for (p = HTTP_last_modified + strlen(HTTP_last_modified) - 1;
          p > HTTP_last_modified && isspace(*p); p--)
       {
         *p = '\0';
       }
     }
  free(me);
  return;
}

/*	End writing */

PRIVATE void HTMIME_end_document (HTStream *me, caddr_t appd)
{
  if (me->target) 
    (*me->targetClass.end_document)(me->target,appd);
}

PRIVATE void HTMIME_handle_interrupt (HTStream *me, caddr_t appd)
{
  me->interrupted = 1;

  /* Propagate interrupt message down. */
  if (me->target)
    (*me->targetClass.handle_interrupt)(me->target,appd);

  return;
}

/*	Structured Object Class */
PUBLIC WWW_CONST HTStreamClass HTMIME = {		
  "MIMEParser",
  HTMIME_free,
  HTMIME_end_document,
  HTMIME_put_character, 	HTMIME_put_string,
  HTMIME_write,
  HTMIME_handle_interrupt
}; 


/*	Subclass-specific Methods */

PUBLIC HTStream* HTMIMEConvert ( HTPresentation *pres, HTParentAnchor *anchor,
	HTStream *sink, HTFormat format_in, int compressed, caddr_t appd)
{
    HTStream* me;
    
    me = (HTStream*)malloc(sizeof(*me));
    me->isa = &HTMIME;       

    if (wWWParams.trace)
      fprintf (stderr, "[HTMIMEConvert] HELLO!\n");

    me->sink = sink;
    me->anchor = anchor;
    me->target = NULL;
    me->state = BEGINNING_OF_LINE;
    me->format = WWW_PLAINTEXT;
    me->targetRep = pres->rep_out;
    me->boundary = 0;		/* Not set yet */
    me->location = 0;
    me->interrupted = 0;
    me->encoding = 0;
    me->compression_encoding = 0;
    me->content_length = -1;
    me->header_length = 0; /* bjs - to allow differentiation between
			      content and header for read length */
    me->expires = 0;
    me->last_modified = 0;
    return me;
}

/* bjs - a kludge for HTFormat.c */
int HTMIME_get_header_length(HTStream *me)
{
    if(me->isa != &HTMIME) return 0; /* in case we screw up */ 
    return me->header_length;
}
