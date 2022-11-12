#include <stdio.h>
#ifndef __QNX__
#include <sys/resource.h>
#endif
#include <unistd.h>

#include "../libhtmlw/HTML.h"
#include "../src/mosaic.h"
#include "xresources.h"
#include "mime.h"
#include "compress.h"

#include "../libnut/system.h"

/* Given a filename of a local compressed file, compress it in place.
   We assume that the file does not already have a .Z or .z extension
   at this point -- this is a little weird but it's convenient. */

void DeCompress (char *fnam, MimeHeaderStruct * mhs)
{
	char buf[2048];
	char *znam;
	char *cmd;
	int i;
	FILE *fp, *pp;
	int len = 0;

/* Either compressed or gzipped. */
	znam = (char *)malloc (sizeof (char) * (strlen (fnam) + 8));
	sprintf (znam, "%s.gz", fnam);

	link(fnam, znam);
	unlink(fnam);
	fp = fopen(fnam,"w");
	cmd =(char *)malloc (strlen (mMosaicAppData.gunzip_command) + strlen (znam) + 8);
	sprintf(cmd, "%s -cdfq %s", mMosaicAppData.gunzip_command, znam);

	pp = popen( cmd, "r");
	while( (i = fread(buf, 1, sizeof(buf), pp)) != 0){
		fwrite(buf, 1, i, fp);
		len += i;
	}
	fclose(fp);
	fclose(pp);
	mhs->content_length = len;
	unlink(znam);
	free (cmd);
	free (znam);
}
