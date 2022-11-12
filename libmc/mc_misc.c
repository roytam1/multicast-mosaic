/*
 * sockio.c
 * Author: Gilles Dauphin
 * Version 2.7b4m1 [May96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 *
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report :
 * 
 * dauphin@sig.enst.fr
 * dax@inf.enst.fr
 */


#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Xm/XmAll.h>

#include "../libhtmlw/HTML.h"
#include "mc_rtp.h"
#include "mc_defs.h"
#include "mc_misc.h"
#include "mc_sockio.h"

/*
 * calcul du temps gmt
 */

static struct tm *p_gmt;
static struct tm gmt;
time_t McDate()
{
	time_t t;
	time_t rt;

	t = time(NULL);

	p_gmt = gmtime(&t);
	gmt = *p_gmt;

	rt = mktime(&gmt);

	if (rt == -1){
		fprintf(stderr,"Error in McDate()\n");
		return t;
	}
	return rt;
}

char * McReadEo(char * fnam, unsigned int * len_ret)
{
	struct stat st;
	int r;
	char *p;
	int fd;

	r = stat(fnam, &st);
	if ( r != 0){
		*len_ret = 0;
		return NULL;
	}

	*len_ret = st.st_size;

	p = (char*) malloc(*len_ret);
	fd = open(fnam,O_RDONLY);
	read(fd,p,*len_ret);
	close (fd);
	return p;
}
