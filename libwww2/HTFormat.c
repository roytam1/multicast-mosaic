/*		Manage different file formats */

/* Connection: Keep-Alive support -bjs */

#include <stdio.h>
#include <unistd.h>

#include "HText.h"
#include "HTMIME.h"

#include "HTFormat.h"

PUBLIC float HTMaxSecs = 1e10;		/* No effective limit */
PUBLIC float HTMaxLength = 1e10;	/* No effective limit */

#include "HTUtils.h"
#include "tcp.h"
#include "HTTCP.h"

#include "HTAlert.h"
#include "HTList.h"
#include "HTInit.h"
#include "HTFWriter.h"
#include "HTPlain.h"
#include "SGML.h"
#include "HTML.h"
#include "HTParams.h"		/* params from X resources */

struct _HTStream {
      WWW_CONST HTStreamClass*	isa;
      /* ... */
};

/* Whoooooooooooooa ugly!!! */
int loading_length = -1;
int noLength=1;

/*SWP -- Even Uglier*/
extern int ftpKludge;

/*	Presentation methods */

PUBLIC  HTList * HTPresentations = 0;
PUBLIC  HTPresentation* default_presentation = 0;


/*	Define a presentation system command for a content-type */

void HTSetPresentation( WWW_CONST char *representation,
	WWW_CONST char *command,
	float	quality,
	float	secs, 
	float	secs_per_byte)
{
    HTPresentation * pres = (HTPresentation *)malloc(sizeof(HTPresentation));
    
    pres->rep = HTAtom_for(representation);
    pres->rep_out = WWW_PRESENT;		/* Fixed for now ... :-) */
    pres->converter = HTSaveAndExecute;		/* Fixed for now ...     */
    pres->quality = quality;
    pres->secs = secs;
    pres->secs_per_byte = secs_per_byte;
    pres->command = 0;
    StrAllocCopy(pres->command, command);
    
    if (!HTPresentations) HTPresentations = HTList_new();
    
    if (strcmp(representation, "*")==0) {
        if (default_presentation) free(default_presentation);
	default_presentation = pres;
    } else {
        HTList_addObjectAtEnd(HTPresentations, pres);
    }
}

/*	Define a built-in function for a content-type
*/
void HTSetConversion(
	WWW_CONST char * representation_in,
	WWW_CONST char * representation_out,
	HTConverter*	converter,
	float	quality,
	float	secs, 
	float	secs_per_byte)
{
    HTPresentation * pres = (HTPresentation *)malloc(sizeof(HTPresentation));
    
    pres->rep = HTAtom_for(representation_in);
    pres->rep_out = HTAtom_for(representation_out);
    pres->converter = converter;
    pres->command = NULL;		/* Fixed */
    pres->quality = quality;
    pres->secs = secs;
    pres->secs_per_byte = secs_per_byte;
    pres->command = 0;
    
    if (!HTPresentations) HTPresentations = HTList_new();
    
    if (strcmp(representation_in, "*")==0) {
        if (default_presentation) free(default_presentation);
	default_presentation = pres;
    } else {
        HTList_addObject(HTPresentations, pres);
    }
}

/********************ddt*/
/*
** Remove a conversion routine from the presentation list.
** The conversion routine must match up with the given args.
*/
PUBLIC void HTRemoveConversion ARGS3(
	WWW_CONST char *, representation_in,
	WWW_CONST char *, representation_out,
	HTConverter*,	converter)
{
	int numberOfPresentations; 
	HTPresentation * pres;
	HTAtom *rep_in, *rep_out;
	int x;

	numberOfPresentations = HTList_count(HTPresentations);
	rep_in = HTAtom_for(representation_in);
	rep_out = HTAtom_for(representation_out);
	for (x = 0; x < numberOfPresentations; x++) {
		pres = (HTPresentation *)HTList_objectAt(HTPresentations, x);
		if (pres) {
			if ((!strcmp(pres->rep->name,rep_in->name)) &&
			    (!strcmp(pres->rep_out->name,rep_out->name)) &&
			    (pres->converter == converter)) {
				HTList_removeObject(HTPresentations,pres);
			}
		}
	}
}
/***************** end ddt*/

/*	File buffering
**
**	The input file is read using the macro which can read from
**	a socket or a file.
**	The input buffer size, if large will give greater efficiency and
**	release the server faster, and if small will save space on PCs etc.
*/
#define INPUT_BUFFER_SIZE 65536
PRIVATE char input_buffer[INPUT_BUFFER_SIZE];
PRIVATE char * input_pointer;
PRIVATE char * input_limit;
PRIVATE int input_file_number;


/*	Set up the buffering
**
**	These routines are public because they are in fact needed by
**	many parsers, and on PCs and Macs we should not duplicate
**	the static buffer area.
*/
PUBLIC void HTInitInput (int file_number)
{
    input_file_number = file_number;
    input_pointer = input_limit = input_buffer;
}

PUBLIC int interrupted_in_htgetcharacter = 0;
PUBLIC char HTGetCharacter (caddr_t appd)
{
  char ch;
  interrupted_in_htgetcharacter = 0;
  do {
      if (input_pointer >= input_limit) {
          int status = 
            HTDoRead(input_file_number, input_buffer, INPUT_BUFFER_SIZE,appd);
          if (status <= 0) {
              if (status == 0) 
                return (char)EOF;
              if (status == HT_INTERRUPTED) {
                  if (wWWParams.trace)
                    fprintf (stderr, "HTFormat: Interrupted in HTGetCharacter\n");
                  interrupted_in_htgetcharacter = 1;
                  return (char)EOF;
                }
              if (wWWParams.trace) 
                fprintf(stderr,
                        "HTFormat: File read error %d\n", status);
              return (char)EOF;
	    }
          input_pointer = input_buffer;
          input_limit = input_buffer + status;
	}
      ch = *input_pointer++;
    }
  while (ch == (char) 13); /* Ignore ASCII carriage return */
  
  return ch;
}

static int partial_wildcard_matches (HTFormat r1, HTFormat r2)
{
  /* r1 is the presentation format we're currently looking at out
     of the list we understand.  r2 is the one we need to get to. */
  char *s1, *s2, *subtype1 = NULL, *subtype2 = NULL;
  int i;

  s1 = HTAtom_name (r1);
  s2 = HTAtom_name (r2);
  if (!s1 || !s2)
    return 0;
  
  s1 = strdup (s1);
  s2 = strdup (s2);
  for (i = 0; i < strlen (s1); i++)
    if (s1[i] == '/') {
        s1[i] = '\0';
        subtype1 = &(s1[i+1]);
        /* Now s1 contains the main type and subtype1 contains the subtype. */
        goto done1;
    }

done1:
  if (!subtype1)
    goto nope;
  
  /* Bail if we don't have a wildcard possibility. */
  if (subtype1[0] != '*')
    goto nope;

  for (i = 0; i < strlen (s2); i++)
    if (s2[i] == '/') {
        s2[i] = '\0';
        subtype2 = &(s2[i+1]);
        /* Now s2 contains the main type and subtype2 contains
           the subtype. */
        goto done2;
    }

done2:
  if (!subtype2)
    goto nope;

  /* Bail if s1 and s2 aren't the same and s1[0] isn't '*'. */
  if (strcmp (s1, s2) && s1[0] != '*')
    goto nope;

  /* OK, so now either we have the same main types or we have a wildcard
     type for s1.  We also know that we have a wildcard possibility in
     s1.  Therefore, at this point, we have a match. */
  free (s1);
  free (s2);
  return 1;

nope:
  free (s1);
  free (s2);
  return 0;
}
  

/*		Create a filter stack
**
**	If a wildcard match is made, a temporary HTPresentation
**	structure is made to hold the destination format while the
**	new stack is generated. This is just to pass the out format to
**	MIME so far.  Storing the format of a stream in the stream might
**	be a lot neater.
*/
PUBLIC HTStream * HTStreamStack (
	HTFormat	format_in,
	HTFormat	rep_out,
        int             compressed,
	HTStream*	sink,
	HTParentAnchor*	anchor,
	caddr_t		appd)
{
	HTAtom * wildcard = HTAtom_for("*");
	HTPresentation temp;

  /* Inherit force_dump_filename from mo-www.c. */
  extern char* force_dump_filename;

  if (wWWParams.trace) {
    fprintf(stderr,
            "[HTStreamStack] Constructing stream stack for %s to %s ...\n",
            HTAtom_name(format_in),	
            HTAtom_name(rep_out));
    fprintf (stderr, "... Compressed is %d\n", compressed);
  }
    
  if (rep_out == WWW_SOURCE || rep_out == format_in) {
      if (wWWParams.trace)
        fprintf (stderr,
                 "[HTStreamStack] rep_out == WWW_SOURCE | rep_out == format_in; returning sink\n");
      return sink;
  }
  
  if (!HTPresentations) 
    HTFormatInit();	/* set up the list */
  
  if (force_dump_filename && format_in != WWW_MIME)
      return HTSaveAndExecute (NULL, anchor, sink, format_in, compressed,appd);
  
  {
    int n = HTList_count(HTPresentations);
    int i;
    HTPresentation * pres;
    for(i=0; i<n; i++) {
        pres = (HTPresentation*)HTList_objectAt(HTPresentations, i);
        if (wWWParams.trace) {
            fprintf (stderr, "HTFormat: looking at pres '%s'\n",
                     HTAtom_name (pres->rep));
            if (pres->command)
              fprintf (stderr, "HTFormat: pres->command is '%s'\n",
                       pres->command);
            else
              fprintf (stderr, "HTFormat: pres->command doesn't exist\n");
        }
        if (pres->rep == format_in ||
            partial_wildcard_matches (pres->rep, format_in)) {
            if (pres->command && strstr(pres->command, "mosaic-internal-present"))
              {
                if (wWWParams.trace)
                  fprintf (stderr, "[HTStreamStack] HEY HEY HEY caught internal-present\n");
                return HTPlainPresent(pres, anchor, sink, format_in, compressed,appd);
              }
            if (pres->rep_out == rep_out) {
                if (wWWParams.trace)
                  fprintf (stderr,
                           "[HTStreamStack] pres->rep_out == rep_out\n");
                return (*pres->converter)(pres, anchor, sink, format_in, compressed,appd);
              }
            if (pres->rep_out == wildcard) {
                if (wWWParams.trace)
                  fprintf (stderr,
                           "[HTStreamStack] pres->rep_out == wildcard\n");
                temp = *pres;/* make temp conversion to needed fmt */
                temp.rep_out = rep_out;		/* yuk */
                return (*pres->converter)(&temp, anchor, sink, format_in, compressed,appd);
              }
          }
    }
  }
  if (wWWParams.trace) {
      fprintf (stderr, "[HTStreamStack] Returning NULL at bottom.\n");
  }
  return NULL;
}


/*		Find the cost of a filter stack
**
**	Must return the cost of the same stack which StreamStack would set up.
**
** On entry,
**	length	The size of the data to be converted
*/
PUBLIC float HTStackValue ARGS4(
	HTFormat,		format_in,
	HTFormat,		rep_out,
	float,			initial_value,
	long int,		length)
{
	HTAtom * wildcard = HTAtom_for("*");
	HTPresentation * pres;
	int i,n;

	if (wWWParams.trace)
		fprintf(stderr,
			"HTFormat: Eval stream stack for %s worth %.3f to %s\n",
			HTAtom_name(format_in),	initial_value,
			HTAtom_name(rep_out));
	if (rep_out == WWW_SOURCE || rep_out == format_in)
		return 0.0;
	if (!HTPresentations)
		HTFormatInit();	/* set up the list */

	n = HTList_count(HTPresentations);
	for(i=0; i<n; i++) {
		pres = (HTPresentation*)HTList_objectAt(HTPresentations, i);
		if (pres->rep == format_in && 
		    ( pres->rep_out == rep_out || pres->rep_out == wildcard)) {
			float value = initial_value * pres->quality;
			if (HTMaxSecs != 0.0)
				value = value - 
					(length*pres->secs_per_byte + pres->secs)/
					HTMaxSecs;
			return value;
		}
	}
	return -1e30;		/* Really bad */
}
	
/*	Push data from a socket down a stream
**
**   This routine is responsible for creating and PRESENTING any
**   graphic (or other) objects described by the file.
**
**   The file number given is assumed to be a TELNET stream ie containing
**   CRLF at the end of lines which need to be stripped to LF for unix
**   when the format is textual.
*/

int HTCopy(int file_number, HTStream *sink, int bytes_already_read, caddr_t appd)
{
	HTStreamClass targetClass;    
	char line[256];
	char *msg;
	int bytes = bytes_already_read;
	int next_twirl = wWWParams.twirl_increment;
	int rv = 0;
	int left = -1, total_read = bytes_already_read, hdr_len = 0;

	HTClearActiveIcon(appd);

/* Push the data down the stream */
	targetClass = *(sink->isa);	/* Copy pointers to procedures */

	hdr_len = HTMIME_get_header_length(sink);

/* Push binary from socket down sink */
	for(;;) {
		int status, intr;

		if (bytes > next_twirl) {
			intr = HTCheckActiveIcon(1,appd);
			next_twirl += wWWParams.twirl_increment;
		} else {
			intr = HTCheckActiveIcon(0,appd);
		}
		if (intr) {
			loading_length=(-1);
			HTProgress ("Data transfer interrupted.",appd);
			noLength=0;
			HTMeter(100,NULL,appd);
			noLength=1;
			(*targetClass.handle_interrupt)(sink,appd);
			rv = -1;
			/* Reset ourselves so we don't get confused. */
			loading_length = -1;
			return rv;
		}

		if(loading_length == -1) {
			left = -1;
			status = HTDoRead(file_number, input_buffer, INPUT_BUFFER_SIZE,appd);
		} else {
			left = (loading_length+hdr_len)-total_read;
			if(left>0) 
				status = HTDoRead(file_number, input_buffer, 
					(left>INPUT_BUFFER_SIZE?
					INPUT_BUFFER_SIZE:left),appd);
			else 
				status=0;
		}
		if (status > 0)
			total_read += status;
		if (status <= 0) {
			if (status == 0) 
			break;
			if (status == HT_INTERRUPTED) {
				loading_length=(-1);
				HTProgress ("Data transfer interrupted.",appd);
				noLength=0;
				HTMeter(100,NULL,appd);
				noLength=1;
				(*targetClass.handle_interrupt)(sink,appd);
				rv = -1;
				/* Reset ourselves so we don't get confused. */
				loading_length = -1;
				return rv;
			}
			if (errno == ENOTCONN || errno == ECONNRESET || errno == EPIPE) {
/* Arrrrgh, HTTP 0/1 compability problem, maybe. */
				rv = -2;
				/* Reset ourselves so we don't get confused. */
				loading_length = -1;
				return rv;
			}
			break;
		}
/* write the block to target */
		(*targetClass.put_block)(sink, input_buffer, status,appd);
		if (ftpKludge) {
			hdr_len=0;
		} else {
			hdr_len = HTMIME_get_header_length(sink);
		}
		bytes += status;
/* moved msg stuff here as loading_length may change midstream -bjs*/
		if (loading_length == -1){
			msg = "Read %d bytes of data.";
			sprintf (line, msg, bytes);
		}else{
			msg = "Read %d of %d bytes of data.";
			sprintf (line, msg, bytes, loading_length+hdr_len);
			HTMeter((bytes*100)/(loading_length+hdr_len),NULL,appd);
		}
		HTProgress (line,appd);
		if((loading_length != -1) && (total_read>=(loading_length+hdr_len))) {
			break;
		}
	} 			/* next bufferload */
	HTProgress ("Data transfer complete.",appd);    
	noLength=0;
	HTMeter(100,NULL,appd);
	noLength=1;
	rv = 0;			 /* Success. */
	loading_length = -1;	 /* Reset ourselves so we don't get confused. */
	return rv;
}

/*	Push data from a file pointer down a stream
**	-------------------------------------
**
**   This routine is responsible for creating and PRESENTING any
**   graphic (or other) objects described by the file.
*/
PUBLIC void HTFileCopy(
	FILE *		fp,
	HTStream*	sink,
	caddr_t		appd)
{
    HTStreamClass targetClass;    
    
    targetClass = *(sink->isa);	/* Copy pointers to procedures */
    for(;;) {
	int status = fread(input_buffer, 1, INPUT_BUFFER_SIZE, fp);
	if (status == 0) { /* EOF or error */
	    if (ferror(fp) == 0) break;
	    if (wWWParams.trace) fprintf(stderr,
		"HTFormat: Read error, read returns %d\n", ferror(fp));
	    break;
	}
	(*targetClass.put_block)(sink, input_buffer, status,appd);
    } /* next bufferload */

    fclose (fp);
    return;
}

PUBLIC void HTFileCopyToText ARGS2(
	FILE *,			fp,
	HText *,		text)
{
  for(;;) {
      int status = fread(input_buffer, 1, INPUT_BUFFER_SIZE, fp);
      if (status == 0) { /* EOF or error */
          if (ferror(fp) == 0)
		 break;
          if (wWWParams.trace) 
		fprintf(stderr, "HTFormat: Read error, read returns %d\n", ferror(fp));
          break;
      }
      HText_appendBlock (text, input_buffer, status);
  } /* next bufferload */
  fclose (fp);
  return;
}


/*	Parse a socket given format and file number
**
**   This routine is responsible for creating and PRESENTING any
**   graphic (or other) objects described by the file.
**
**   The file number given is assumed to be a TELNET stream ie containing
**   CRLF at the end of lines which need to be stripped to LF for unix
**   when the format is textual.
*/
PUBLIC int HTParseSocket (
	HTFormat		format_in,
	HTFormat		format_out,
	HTParentAnchor *	anchor,
	int			file_number,
	HTStream*		sink,
        int                    compressed,
	caddr_t			appd)
{
  HTStream * stream;
  HTStreamClass targetClass;    
  int rv;
  
  stream = HTStreamStack(format_in, format_out,
                         compressed, sink, anchor,appd);
  if (!stream) {
      char buffer[1024];	/* @@@@@@@@ */
      sprintf(buffer, "Sorry, can't convert from %s to %s.",
              HTAtom_name(format_in), HTAtom_name(format_out));
      if (wWWParams.trace) fprintf(stderr, "HTFormat: %s\n", buffer);
      HTAlert( buffer,appd);
      return -1;
  }
  targetClass = *(stream->isa);	/* Copy pointers to procedures */
  rv = HTCopy(file_number, stream, 0,appd);
  if (rv == -1) {
      /* handle_interrupt should have been done in HTCopy */
      /* (*targetClass.handle_interrupt)(stream); */
      return HT_INTERRUPTED;
  }
  (*targetClass.end_document)(stream,appd);

  /* New thing: we force close the data socket here, so that if
     an external viewer gets forked off in the free method below,
     the connection doesn't remain upon until the child exits --
     which it does if we don't do this. */

  NETCLOSE (file_number);
  (*targetClass.free)(stream,appd);
  return HT_LOADED;
}

/*	Parse a file given format and file pointer
**
**   This routine is responsible for creating and PRESENTING any
**   graphic (or other) objects described by the file.
**
**   The file number given is assumed to be a TELNET stream ie containing
**   CRLF at the end of lines which need to be stripped to LF for unix
**   when the format is textual.
*/
PUBLIC int HTParseFile (
	HTFormat		format_in,
	HTFormat		format_out,
	HTParentAnchor *	anchor,
	FILE *			fp,
	HTStream*		sink,
        int                    compressed,
	caddr_t			appd)
{
    HTStream * stream;
    HTStreamClass targetClass;    
    
    stream = HTStreamStack(format_in, format_out,
                           compressed, sink , anchor,appd);
    if (!stream) {
        char buffer[1024];	/* @@@@@@@@ */
	sprintf(buffer, "Sorry, can't convert from %s to %s.",
		HTAtom_name(format_in), HTAtom_name(format_out));
	if (wWWParams.trace) fprintf(stderr,"HTFormat(in HTParseFile): %s\n", buffer);
        HTAlert(buffer,appd);
	return -1;
    }
    targetClass = *(stream->isa);	/* Copy pointers to procedures */
    HTFileCopy(fp, stream,appd);
    (*targetClass.end_document)(stream,appd);
    (*targetClass.free)(stream,appd);
    return HT_LOADED;
}
