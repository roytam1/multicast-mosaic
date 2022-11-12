/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/wait.h>
#include <sys/time.h>
#ifndef __QNX__
#include <sys/resource.h>
#endif
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/uio.h>
#include <pwd.h>

#ifdef RS6000
#include <sys/select.h>
#endif

#include "system.h"

extern int errno;

int sleep_interrupt=0;

int my_system(char *cmd, char *retBuf, int bufsize);
char **buildArgv(char *cmd, int *new_argc);
char *findProgram(char *pname,char *spath);

#undef STANDALONE /* */

#ifdef STANDALONE

char *userPath={"/bin:/usr/bin:/sbin:/usr/sbin"};

int main(int argc, char **argv) 
{

	int retValue;
	char *cmd, *fnam, *lpr;
	char buf[BUFSIZ];

	if (argc==3) {
		if (my_sleep(atoi(argv[1]),atoi(argv[2]))) {
			printf("Interrupted\n");
		} else {
			printf("Not Interrupted\n");
		}
		exit(0);
	}
	lpr = (char *)malloc (50);
	strcpy(lpr,"dsfs/usr/ucb/lpr");

	fnam = (char *)malloc (50);
	strcpy(fnam,"/sdlfjsdusr5/spowers/.cshrc");

	cmd = (char *)malloc ((strlen (lpr) + strlen (fnam) + 24));
	sprintf (cmd, "%s %s", lpr, fnam);

	printf("Executing '%s'.\n",cmd);

	if ((retValue=my_system(cmd,buf,BUFSIZ))!=SYS_SUCCESS) {
		printf("-----\nError Code [%d]\n-----\n",retValue);
	}

	if (buf && *buf) {
		printf("------\n%s-----\n",buf);
	} else {
		printf("------\nNo output.\n------\n");
	}
}
#else
extern char *userPath;
#endif

/*
 * Written by: Scott Powers and Brad Viviano
 *
 * Takes a string command, executes the command, returns the output of
 *   the command (if any) in retBuf (passed in and pre-allocated).
 *
 * Returns one of the following:
 *   SYS_SUCCESS - The command executed without incident. Note, this does not
 *     necessarily mean the command was successful...e.g. some systems have
 *     a shell script for "lpr". In this case, the shell that runs "lpr" will
 *     execute fine, but the command "lp" which "lpr" calls may still fail.
 *   SYS_NO_COMMAND - There  was no command provided.
 *   SYS_FORK_FAIL - The fork failed.
 *   SYS_PROGRAM_FAILED - The exec could not start the program.
 *   SYS_NO_RETBUF - There was no retBuf allocated.
 *   SYS_FCNTL_FAILED - The set of NON_BLOCK on the parent end of the pipe
 *     failed.
 */
int my_system(char *cmd, char *retBuf, int bufsize)
{
	char **sys_argv=NULL;
	int sys_argc;
	pid_t pid;
	int status,statusloc;
	int fds[2];
	char *path=NULL;

	if (!retBuf)
		return(SYS_NO_RETBUF);
	*retBuf='\0';
	if (!cmd || !*cmd)
		return(SYS_NO_COMMAND);
	pipe(fds);
	if (fcntl(fds[0],F_SETFL,O_NONBLOCK)==(-1)) {
		perror("fcntl-nonblock");
		return(SYS_FCNTL_FAILED);
	}
	if ((pid=fork())==(-1))
		return(SYS_FORK_FAIL);
	if (pid==0) {
		/*in child -- so don't worry about frees*/
		sys_argv=buildArgv(cmd, &sys_argc);
		dup2(fds[1],1);
		dup2(fds[1],2);
		if (sys_argv!=NULL) {
			if(sys_argv[0] && sys_argv[0][0] && sys_argv[0][0]=='/') {
				path=strdup(sys_argv[0]);
			} else {
				path=findProgram(sys_argv[0],userPath);
			}
			execv(path,sys_argv);
			fprintf(stderr,"Exec of %s failed!\n",cmd);
			perror("exec");
		} else {
			fprintf(stderr,"Could not build argv for [%s].\n",cmd);
		}
		exit(1); /*child*/
	} else {
		int n;

		/*in parent*/
		status=wait(&statusloc);
		n=read(fds[0],retBuf,bufsize-1);
		if (n>0) {
			retBuf[n]='\0';
		} else {
			*retBuf='\0';
		}
		close(fds[0]);
		close(fds[1]);

		if (*retBuf)
			return(SYS_PROGRAM_FAILED);
		return(SYS_SUCCESS);
	}
}


/*
 * Written by: Scott Powers
 *
 * findProgram takes a program name and a path and searches it until:
 *   a) The program name is found, at which time the full path is returned.
 *   b) The end of the search path comes about, at which time NULL is returned.
 *
 */
char *findProgram(char *pname,char *spath) 
{
	char *start=NULL,*ptr=NULL,*endptr=NULL;
	char tryit[BUFSIZ];
	struct stat buf;

	if (!spath || !*spath || !pname || !*pname)
		return(NULL);

	start=spath;
	while (start && *start) {
		ptr=start;
		endptr=strchr(start,':');
		if (endptr) {
			start=endptr+1;
			*endptr='\0';
		} else {
			start=NULL;
		}
		sprintf(tryit,"%s/%s",ptr,pname);
		if (!stat(tryit,&buf)) {
			return(strdup(tryit));
		}
	}
	return(NULL);
}

/*
 * Written by: Brad Viviano and Scott Powers
 *
 * Takes a 1d string and turns it into a 2d array of strings.
 *
 * Watch out for the frees! You must free(*argv) and then free(argv)! NOTHING
 *   ELSE!! Do _NOT_ free the individual args of argv.
 */
char **buildArgv(char *cmd, int *new_argc) 
{
	char **new_argv=NULL;
	char *buf=NULL,*tmp=NULL;
	int i=0;

	if (!cmd && !*cmd) {
		*new_argc=0;
		return(NULL);
	}

	for(tmp=cmd; isspace(*tmp); tmp++);
	buf=strdup(tmp);
	if (!buf) {
		*new_argc=0;
		return(NULL);
	}

	tmp=buf;

	new_argv=(char **)calloc(1,sizeof(char *));
	if (!new_argv) {
		free(buf);
		*new_argc=0;
		return(NULL);
	}

	new_argv[0]=NULL;

	while (*tmp) {
		if (!isspace(*tmp)) { /*found the begining of a word*/
			new_argv[i]=tmp;
			for (; *tmp && !isspace(*tmp); tmp++);
			if (*tmp) {
				*tmp='\0';
				tmp++;
			}
			i++;
			new_argv=(char **)realloc(new_argv,((i+1)*sizeof(char *)));
			new_argv[i]=NULL;
		} else {
			tmp++;
		}
	}
	*new_argc=i;
	return(new_argv);
}


/*
 * Written by: Scott Powers
 *
 * Takes an integer which is the number of seconds to sleep and an integer
 *   which is a boolean for whether to interrupt the sleep or not.
 *
 * This function sleeps for X seconds. It is interruptable.
 *
 * Returns a 1 of interrupted (and allowed to interrupt) or 0 when done
 *   sleeping.
 *
 * Note that this is not _really_ _truly_ _exact_ as it is does perform some
 *   condition checking inbetween each 100 milliseconds. But...it's pretty
 *   darn close.
 */
int my_sleep(int length, int intrupt) 
{
	struct timeval timeout;
	int count=0;

	sleep_interrupt=0;
	length*=1000000;
	while (count<length) {
		timeout.tv_sec=0;
		timeout.tv_usec=100000;
		select(0, NULL, NULL, NULL, &timeout);
		count+=100000;
		if (intrupt && sleep_interrupt) {
			return(1);
		}
	}
	return(0);
}

/* Written by: Tommy Reilly (with major code snarfing from Scott Powers)
 *
 * This is essentially Scott's my_move function re-written without rename
 * and without erasing the source file. 
 *
 * If "overwrite" is true, the destination file will automatically be
 * overwritten. If it is false and the file exists, my_move will return
 * SYS_FILE_EXISTS. It is up to the programmer to tell the user this.
 *
 * Return Values:
 *   SYS_NO_SRC_FILE -- There was no source filename specified.
 *   SYS_NO_DEST_FILE -- There was no destination filename specified.
 *   SYS_DEST_EXISTS -- Overwrite was off and the destination exists.
 *   SYS_NO_MEMORY -- No memory to allocate with.
 *   SYS_SRC_OPEN_FAIL -- Open failed on the source file.
 *   SYS_INTERNAL_FAIL -- Error occured that user doesn't want to know about.
 *   SYS_SUCCESS -- Success.
 */
int my_copy(char *src, char *dest, char *retBuf, int bufsize, int overwrite)
{
	int n_src=1, n_dest=1, fd_src, fd_dest;
	char *copy_error=NULL;
	char buf[BUFSIZ];
	struct stat dest_stat;
  
	if (!retBuf)
		return(SYS_NO_RETBUF);

	if (!src || !*src) {
		strcpy(retBuf,"There was no source file specified.\n");
		return(SYS_NO_SRC_FILE);
	}
	if (!dest || !*dest) {
		strcpy(retBuf,"There was no destination file specified.\n");
		return(SYS_NO_DEST_FILE);
	}
  
	*retBuf='\0';
	if (!overwrite) {
		if (stat(dest,&dest_stat)) {
			sprintf(retBuf,"Stat [%s] error:\n     File already exists.\n",dest);
			return(SYS_DEST_EXISTS);
		}
	}

	if ((fd_src=open(src,O_RDONLY))==(-1)) {
		copy_error=strdup(strerror(errno));
		if (!copy_error) {
			strcpy(retBuf,"There was not enough memory allocate.\n");
			return(SYS_NO_MEMORY);
		}
		if (strlen(copy_error)>(bufsize-strlen(retBuf))) {
			fprintf(stderr,"%s\n",copy_error);
		} else {
			sprintf(retBuf,"%sCopy([%s] to [%s]) error:\n     %s\n\n",retBuf,src,dest,copy_error);
		}
		free(copy_error);
		return(SYS_SRC_OPEN_FAIL);
	}
	if ((fd_dest=open(dest,O_WRONLY|O_CREAT|O_TRUNC,0644))==(-1)) {
		copy_error=strdup(strerror(errno));
		if (!copy_error) {
			strcpy(retBuf,"There was not enough memory allocate.\n");
			return(SYS_NO_MEMORY);
		}
		if (strlen(copy_error)>(bufsize-strlen(retBuf))) {
			fprintf(stderr,"%s\n",copy_error);
		} else {
			sprintf(retBuf,"%sCopy([%s] to [%s]) error:\n     %s\n\n",retBuf,src,dest,copy_error);
		}
		free(copy_error);
		close(fd_src);
		return(SYS_DEST_OPEN_FAIL);
	}
/*both files open and ready*/
	while (n_src>0) {
		n_src=read(fd_src,buf,BUFSIZ-1);
		if (n_src>0) {
			n_dest=write(fd_dest,buf,n_src);
			if (n_dest>0)
				continue;
			close(fd_src);
			close(fd_dest);
			sprintf(retBuf,"Write([%s]) error:\n     %s\n\n",dest,strerror(errno));
			return(SYS_WRITE_FAIL);
		}
		if (!n_src)
			continue;
		close(fd_src);
		close(fd_dest);
		sprintf(retBuf,"Read([%s]) error:\n     %s\n\n",src,strerror(errno));
		return(SYS_READ_FAIL);
	}
	close(fd_src);
	close(fd_dest);
	return(SYS_SUCCESS);
} 
