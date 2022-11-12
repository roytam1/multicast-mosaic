/* Some of code came from libwww2/HTFile.c library. Don't know which
 * Copyright apply... */
/* rewrote by G.Dauphin 11 Oct 1997 */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <dirent.h>

#if defined(SVR4) && !defined(SCO) && !defined(linux)
#include <sys/filio.h>
#include <unistd.h>
#endif

extern int errno;

#include "../libnut/system.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mime.h"
#include "paf.h"
#include "file-proto.h"
#include "URLParse.h"

#define CHECK_OUT_OF_MEM(x) { if ( x == NULL) {\
                                fprintf(stderr,"Out of memory\n");\
                                exit(1);\
                             }}

/* Convert filenames between local and WWW formats
** returns a malloc'ed string which must be freed by the caller.
*/
static char * HTLocalName(const char *name)
{
	char * path = URLParse(name, "", PARSE_PATH+PARSE_PUNCTUATION);
	return(path);
}

#define LBUF 1024
void read_file_local(PafDocDataStruct * pafd)
{
	char * local_name;
	struct stat s;
	struct stat statbuf;
	int count;
	char *filepath;
	char *ct;
	int cmpr;
	char buf[LBUF];
	MimeHeaderStruct mhs;
	int soc,i;

	local_name = HTLocalName(pafd->aurl);
	if (!local_name){
		free(pafd->www_con_type);
		pafd->www_con_type = NULL;
		(*pafd->call_me_on_error)(pafd,"Can't open local file");
		return ;
	}
/* stat the file */
	if (stat(local_name,&s) == -1) {
		free(local_name);
		free(pafd->www_con_type);
		pafd->www_con_type = NULL;
		(*pafd->call_me_on_error)(pafd,"Can't open local file");
		return ;
	}
/* check if localname is a directory */
	if ( (s.st_mode & S_IFMT) == S_IFDIR) { /* a dir */
		DIR *dfp;
		struct dirent * dp;
		FILE * fp;
		char * buffer;
		char * ptr;
               	char *dataptr;

		dfp = opendir(local_name);
		if (!dfp) {
			free(local_name);
			perror("read_file_local:");
			free(pafd->www_con_type);
			pafd->www_con_type = NULL;
			(*pafd->call_me_on_error)(pafd,"Can't open local file");
			return ;
		}
/* Suck the directory up into a list to be sorted */
		HTSortInit();          
		for(dp=readdir(dfp);dp != NULL;dp=readdir(dfp)) {
			ptr = strdup(dp->d_name);
			CHECK_OUT_OF_MEM(ptr);
			HTSortAdd(ptr);
		}
		closedir(dfp);
/* Sort the dir list */                
		HTSortSort();
/* Start a new HTML page */            
		fp = fopen(pafd->fname, "w");
		fprintf(fp, "<H1>Local Directory %s",local_name);
		fprintf(fp, "</H1>\n<DL>\n");
		for(count=0,dataptr=HTSortFetch(count); dataptr != NULL;
		   free(dataptr), count++, dataptr=HTSortFetch(count)) {
/* We dont want to see . */            
			if(strcmp(dataptr,".") == 0)
				continue;
/* If its .. *and* the current directory is / dont show anything, otherwise
 * print out a nice Parent Directory entry. */
		if(strcmp(dataptr,"..") == 0) {
				if(strcmp(local_name,"/") == 0)
					continue;
				buffer = strdup(local_name);
				ptr = strrchr(buffer, '/');
				if(ptr != NULL) *ptr='\0';
				if(buffer[0] == '\0')
					strcpy(buffer,"/");
				fprintf(fp,"<DD><A HREF=\"%s",buffer);
				fprintf(fp,"\"><IMG SRC=\"%s",
					HTgeticonname(NULL,"directory"));
				fprintf(fp,"\"> Parent Directory</a>");
				free(buffer);
			}
/* Get the filesize information from a stat, if we cant stat it, we probably */
/* cant read it either, so ignore it. */
			filepath = (char*)malloc(
				strlen(local_name)+strlen(dataptr)+10);
			sprintf(filepath,"%s/%s",local_name, dataptr);
			if(stat(filepath, &statbuf) == -1)
				continue;
			free(filepath);

			fprintf(fp,"<DD><A HREF=\"%s",local_name);
			if(local_name[strlen(local_name)-1] != '/')
				fprintf (fp, "/");
			fprintf (fp, "%s",dataptr);
			fprintf (fp, "\">");
/* If its a directory, dump out a dir icon, dont bother with anything else */
			if(statbuf.st_mode & S_IFDIR) {
				fprintf(fp, "<IMG SRC=\"");
				fprintf(fp, HTgeticonname(NULL,"directory"));
				fprintf (fp, "\"> %s</A>\n",dataptr);
				continue;
			}
/* if it is a file try and figure out what type of file it is, and grab    */
/* the appropriate icon.  If we cant figure it out, call it text.  If its  */
/* a compressed file, call it binary no matter what                        */
			ct = HTFileName2ct(dataptr, "text/html", &cmpr);
/* If its executable then call it application, else it might as well be text */
			if(cmpr == NO_ENCODING) {
				fprintf(fp, "<IMG SRC=\"");
				if((statbuf.st_mode & S_IXUSR) ||
				   (statbuf.st_mode & S_IXGRP) ||
				   (statbuf.st_mode & S_IXOTH)) {
					fprintf(fp,
					HTgeticonname(ct, "application"));
				} else {   
					fprintf(fp,HTgeticonname(ct,"text"));
				}          
				fprintf(fp, "\"> ");
			} else {       
				fprintf(fp, "<IMG SRC=\"");
				fprintf(fp, HTgeticonname(NULL, "application"));
				fprintf(fp, "\"> ");
			}              
			fprintf(fp,"%s (%d bytes)</A>\n",dataptr,statbuf.st_size);
		}
/* End of list, clean up and we are done */ 
		fprintf (fp, "</DL>\n");
		fclose(fp);
		free(pafd->www_con_type);
		pafd->www_con_type = NULL;
		free(local_name);
		mhs.content_encoding = NO_ENCODING;
		mhs.content_type = strdup("text/html");
		stat(pafd->fname, &statbuf);
		mhs.content_length = statbuf.st_size;
		*(pafd->mhs) = mhs;
		(*pafd->call_me_on_succes)(pafd);
		return;
	} /* end if localname is directory */
/* a file */
	soc = open(local_name, O_RDONLY);
	pafd->www_con_type->prim_fd = soc;
	if ( soc < 0 ) {
		free(local_name);
		perror("read_file_local:");
		free(pafd->www_con_type);
		pafd->www_con_type = NULL;
		(*pafd->call_me_on_error)(pafd,"Can't open local file");
		return ;
	}
/* read the file */
/* copy in target pafd->fd */
	while( (i = read(soc,buf,LBUF)) >0)
		write(pafd->fd,buf,i);
	close(soc);
	free(pafd->www_con_type);
	pafd->www_con_type = NULL;

/* set mhs from file type */
	ct = HTFileName2ct(local_name,"text/plain", &cmpr);
	mhs.content_encoding = cmpr;
/* get len of file */
	mhs.content_length = s.st_size;
	mhs.content_type = ct;
	free(local_name);
	*(pafd->mhs) = mhs;
	(*pafd->call_me_on_succes)(pafd);
	return;
}
