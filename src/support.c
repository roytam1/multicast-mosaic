/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <string.h>

/*for memcpy*/
#include <memory.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "accept.h"
#include "support.h"

/* this routine reads from the specified port, but also considers contents
 * read and in buffer from the GetLine() routine.
 * return the number of chars read */
int ReadBuffer( MCCIPort s, char *data, int numBytesToRead)
{
	int numRead = 0;

	if (numBytesToRead <= s->numInBuffer) {
		memcpy(data,s->buffer,numBytesToRead);
		s->numInBuffer -= numBytesToRead;
		return(numBytesToRead);
	}
	if (s->numInBuffer > 0) {
		memcpy(data,s->buffer,s->numInBuffer);
		data += s->numInBuffer;
		numBytesToRead -= s->numInBuffer;
		numRead = s->numInBuffer;
		s->numInBuffer = 0;
	}
	numRead += NetRead(s, data, numBytesToRead);
	return(numRead);
}

/* This routine returns a line read in from the socket file descriptor. 
 * The location of the string returned is good until the next call.
 * Limitation of this routine: A line read in must not be bigger than
 * the buffersize.
 * 0 returned on error
 */
char *GetLine(MCCIPort s) /****** this routine needs an overhaul.... */
{
	int numBytes;
	char buf2[PORTBUFFERSIZE +1];
	char *endptr;
	static char returnLine[PORTBUFFERSIZE * 2 +2];
	register char *rptr,*ptr;
	register int count;

	if (s->numInBuffer < 1) {
		/* no character in s->buffer, so fill it up */
/*
		if (!connectedToServer) {
			return(0);
		}
*/
		if (1 > (numBytes = NetRead(s, s->buffer, PORTBUFFERSIZE))) {
			/*  End Of Connection */
/*
			DisconnectFromServer(s);
*/

			if (mMosaicSrcTrace) {
				fprintf(stderr,"GetLine: End of Connection\n");
			}
			return(0);
		}
		s->numInBuffer = numBytes;
	}
	s->buffer[s->numInBuffer]='\0';
	if (!(endptr = strstr(s->buffer, "\r\n"))) {
		/* There is no <CRLF> in s->buffer */
/*
		if (!connectedToServer) {
			if (mMosaicSrcTrace) {
				fprintf(stderr,"GetLine: return 0 at point 3\n");
			}

			return(0);
		}
*/
/*
		if (! NetIsThereInput(s)) {
			if (mMosaicSrcTrace) {
				fprintf(stderr,"GetLine: return 0 at point 4\n");
			}
			return(0);
		}
*/
		/* read in <CRLF> */
		if (1 > (numBytes = NetRead(s, buf2, PORTBUFFERSIZE))) {
			/*  End Of Connection */
/*
			NNTPDisconnectFromServer(s);
*/
			if (mMosaicSrcTrace) {
				fprintf(stderr,"GetLine: return 0 at point 5\n");
			}
			return(0);
		}
		memcpy(&(s->buffer[s->numInBuffer]),buf2,numBytes);
		s->numInBuffer += numBytes;
		s->buffer[s->numInBuffer]='\0';
		if (!(endptr = strstr(s->buffer, "\r\n"))) {
			/* protocol error on server end 
			   Everything sent should be terminated with
			   a <CRLF>... just return for now */
			if (mMosaicSrcTrace) {
				fprintf(stderr,"GetLine: return NULL at point 6\n");
			}
			return(NULL);
		}
	}
	endptr++;endptr++; /* <CRLF> should be included in line*/

	/* copy the line to the returnLine s->buffer */
	count = 0;
	rptr = returnLine;
	ptr = s->buffer;
	while (ptr != endptr) {
		*rptr++ =  *ptr++;
		count++;
	}
	*rptr = '\0'; /* null terminate the return line */

/* shift the s->buffer contents to the front */
	s->numInBuffer -= count;
	memcpy(s->buffer,ptr,s->numInBuffer);
	return(returnLine);
} /* NNTPGetLine() */


/* return a word out of the text */
void GetWordFromString(
	char *text,	 /* text to get a word out of */
	char **retStart, /* RETURNED: start of word in text */
	char **retEnd)	 /* RETURNED: end of word in text */
{
	char *start;
	char *end;

	if (!text) {
		*retStart = *retEnd = text;
		return;
	}

	start = text;
	while ((*start) && isspace(*start)){ /*skip over leading space*/
		start++;
	}

	end = start;
	while((*end) && (!isspace(*end))){ /* find next space */
		end++;
	}

	*retStart = start;
	*retEnd = end;
	return;
}
