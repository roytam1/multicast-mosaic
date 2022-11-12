#include <stdio.h>
#include <sys/wait.h>
#include <sys/time.h>
#ifndef __QNX__
#include <sys/resource.h>
#endif
#include <unistd.h>

#include "HText.h"
#include "HTFormat.h"
#include "HTFile.h"
#include "HTUtils.h"
#include "tcp.h"
#include "HTML.h"
#include "HTAlert.h"
#include "HTList.h"
#include "HTInit.h"
#include "HTFWriter.h"
#include "HTPlain.h"
#include "SGML.h"
#include "HTParams.h"		/* params from X resources */

#include "../libhtmlw/HTML.h"
#include "../src/mosaic.h"

#include "../libnut/system.h"

struct _HTStream 
{
  WWW_CONST HTStreamClass*	isa;
  /* ... */
};

int is_uncompressed=0;

extern char *mo_tmpnam (char *);

/* Given a filename of a local compressed file, compress it in place.
   We assume that the file does not already have a .Z or .z extension
   at this point -- this is a little weird but it's convenient. */

void HTCompressedFileToFile (char *fnam, int compressed, caddr_t appd)
{
	char *znam;
	char *cmd;
	char retBuf[BUFSIZ];
	int status;
	int skip_output=0;
	char final[BUFSIZ];

	cmd=NULL;

	if (wWWParams.trace)
		fprintf 
		(stderr, "[HTCompressedFileToFile] Entered; fnam '%s', compressed %d\n",
		fnam, compressed);

/* Punt if we can't handle it. */
	if (compressed != COMPRESSED_BIGZ && compressed != COMPRESSED_GNUZIP)
		return;

	HTProgress ("Preparing to uncompress data.",appd);
	znam = (char *)malloc (sizeof (char) * (strlen (fnam) + 8));

/* Either compressed or gzipped. */
	if (compressed == COMPRESSED_BIGZ)
		sprintf (znam, "%s.Z", fnam);
	else
		sprintf (znam, "%s.gz", fnam);

/*SWP -- New "mv" fucntion to take care of these /bin/mv things*/

	if ((status=my_move(fnam,znam,retBuf,BUFSIZ,1))!=SYS_SUCCESS) {
		sprintf(retBuf,"Unable to uncompress compressed data;\nresults may be in error.\n%s",retBuf);
		application_user_info_wait(retBuf);
		free (znam);
		return;
	}
	if (wWWParams.trace)
		fprintf (stderr, "[HTCompressedFileToFile] Moved '%s' to '%s'\n",
			fnam, znam);
	if (compressed == COMPRESSED_BIGZ) {
		cmd = (char *)malloc(strlen(wWWParams.uncompress_program)+strlen(znam)+8);
		sprintf (cmd, "%s %s", wWWParams.uncompress_program, znam);
	} else {
		cmd =(char *)malloc (strlen (wWWParams.gunzip_program) + strlen (znam) + 8);
		sprintf (cmd, "%s %s", wWWParams.gunzip_program, znam);
	}
	HTProgress ("Uncompressing data.",appd);

	*retBuf='\0';
	*final='\0';

	if ((status=my_system(cmd,retBuf,BUFSIZ))!=SYS_SUCCESS) {
		switch(status) {
		case SYS_NO_COMMAND:
			sprintf(final,"%sThere was no command to execute.\n",final);
			break;
		case SYS_FORK_FAIL:
			sprintf(final,"%sThe fork call failed.\n",final);
			break;
		case SYS_PROGRAM_FAILED:
			sprintf(final,"%sThe program specified was not able to exec.\n",final);
			break;
		case SYS_NO_RETBUF:
			sprintf(final,"%sThere was no return buffer.\n",final);
			break;
		case SYS_FCNTL_FAILED:
			sprintf(final,"%sFcntl failed to set non-block on the pipe.\n",final);
			break;
		}
/*give them the output*/
		if (*retBuf) {
			sprintf(final,"%s%s",final,retBuf);
		}
	} else if (*retBuf) {
/*give them the output*/
		sprintf(final,"%s%s",final,retBuf);
	} else {
/*a-okay*/
		skip_output=1;
	}
	if (!skip_output) {
		application_user_info_wait(final);
		free (cmd);
		free (znam);
		HTProgress ("Uncompress failed.",appd);
		return;
	}
	HTProgress ("Data uncompressed.",appd);
	is_uncompressed=1;  
	if (wWWParams.trace)
		fprintf 
	(stderr, "[HTCompressedFileToFile] Uncompressed '%s' with command '%s'\n",
		znam, cmd);
	free (cmd);
	free (znam);
	return;
}

void HTCompressedHText (HText *text, int compressed, int plain, caddr_t appd)
{
	char *fnam;
	FILE *fp;
	int rv, size_of_data;
  
	if (wWWParams.trace)
		fprintf(stderr,"[HTCompressedHText] Enter; compressed %d\n",
			compressed);

/* Punt if we can't handle it. */
	if (compressed != COMPRESSED_BIGZ && compressed != COMPRESSED_GNUZIP)
		return;

/* Hmmmmmmmmm, I'm not sure why we subtract 1 here, but it is
 * indeed working... */
	size_of_data = HText_getTextLength (text) - 1;

	if (size_of_data == 0) {
		fprintf (stderr, "[HTCompressedHText] size_of_data 0; punting\n");
		return;
	}
	fnam = mo_tmpnam ((char *) 0);
	fp = fopen (fnam, "w");
	if (!fp) {
		if (wWWParams.trace)
		fprintf (stderr, "COULD NOT OPEN TMP FILE '%s'\n", fnam);
		HTapplication_user_feedback("Unable to uncompress compressed data;\nresults may be in error.",appd);
		free (fnam);
		return;
	}

	if (wWWParams.trace)
		fprintf (stderr, "[HTCmopressedHText] Going to write %d bytes.\n",
			size_of_data);
	rv = fwrite (HText_getText (text), sizeof (char), size_of_data, fp);
	if (rv != size_of_data) {
		if (wWWParams.trace)
			fprintf (stderr, "ONLY WROTE %d bytes\n", rv);
		HTapplication_user_feedback("Unable to write compressed data to local disk;\nresults may be in error.",appd);
	}
	fclose (fp);
	if (wWWParams.trace)
		fprintf (stderr, "HTCompressedHText: Calling CompressedFileToFile\n");
	HTCompressedFileToFile (fnam, compressed, appd);
	HText_doAbort (text);
	HText_beginAppend (text);
 
	if (plain) {
		if (wWWParams.trace)
			fprintf (stderr, "[HTCompressedHText] Throwing in PLAINTEXT token...\n");
		HText_appendText(text, "<PLAINTEXT>\n");
	}
	fp = fopen (fnam, "r");
	if (!fp) {
		if (wWWParams.trace)
			fprintf (stderr, "COULD NOT OPEN TMP FILE FOR READING '%s'\n", fnam);
/* We already get error dialog up above. */
		free (fnam);
		return;
	}
	HTFileCopyToText (fp, text);
	if (wWWParams.trace)
		fprintf (stderr, "[HTCompressedHText] I think we're done...\n");
	unlink(fnam);
	return;
}
