/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* this module intended to handle child process clean up through callbacks*/
/* written for version 2.5 */

#include <stdio.h>
#include <signal.h>
#include <sys/wait.h>
#include <sys/types.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "child.h"
#include "../libhtmlw/list.h"

#ifndef DISABLE_TRACE
extern int srcTrace;
#endif

static List childProcessList;

typedef struct {
	pid_t pid;
	void (*callback)(EditFile*, int);
	void *callBackData;
} ProcessHandle;

static ProcessHandle *SearchForChildRecordByPID(pid_t pid);

void InitChildProcessor(void)
{
	childProcessList = ListCreate();
}

/* Add a child process handler.  Callback is made when child dies */
/* callback is of the form callback(callBackData,pid); */
void AddChildProcessHandler(pid_t pid,
	void (*callback)(EditFile *e, int pid), void *callBackData)
{
	ProcessHandle *p;

	if (!(p = (ProcessHandle *) malloc(sizeof(ProcessHandle)))) {
		fprintf(stderr,"Out of Memory\n");
		return;
	}
	p->pid = pid;
	p->callback = callback;
	p->callBackData = callBackData;
	ListAddEntry(childProcessList, (char*)p);
}

static ProcessHandle *SearchForChildRecordByPID(pid_t pid)
{
	ProcessHandle *p;

	p = (ProcessHandle *) ListHead(childProcessList);
	while(p) {
		if (p->pid == pid)
			return(p);
		p = (ProcessHandle *) ListNext(childProcessList);
	}
	return(NULL);
}

/* terminate the children... 
   you may want to remove SIGCHLD signal handler before calling this routine
*/
void KillAllChildren()
{
	ProcessHandle *p;

	/* first, be nice and send SIGHUP */
	p = (ProcessHandle *) ListHead(childProcessList);
	while(p) {
		kill(p->pid,SIGHUP);
		p = (ProcessHandle *) ListNext(childProcessList);
	}

	/* hack and slash */
	p = (ProcessHandle *) ListHead(childProcessList);
	while(p) {
		kill(p->pid,SIGKILL);
		p = (ProcessHandle *) ListNext(childProcessList);
	}
}

/* callback routine for SIGCHLD signal handler */
void ChildTerminated(int sig)
{
	pid_t pid;
	ProcessHandle *p;
	int stat_loc;

#ifdef SVR4
	pid = waitpid((pid_t)(-1),NULL,WNOHANG);
	signal(SIGCHLD, (void (*)(int))ChildTerminated); /*Solaris resets the signal on a catch*/
#else
	pid = wait3(&stat_loc,WNOHANG,NULL);
#endif
	p = SearchForChildRecordByPID(pid);
	if (!p)  	/* un registered child process */
		return;

	(p->callback)((EditFile*)p->callBackData,p->pid);
	ListDeleteEntry(childProcessList, (char*)p);
	free(p);
	return;
}
