/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"


#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "newsrc.h"

#include <signal.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <unistd.h>

#include "main.h"
#include "gui.h"
#include "pan.h"
#include "child.h"
#include "hotlist.h"
#include "globalhist.h"
#include "cciBindings2.h"

extern void McSendRtcpBye(void);
extern int	mc_multicast_enable;

/* swp */
#define _KCMS_H_
#include "kcms.h"

char *userPath=NULL;

void mo_exit (void)
{
	if(mc_multicast_enable)
		McSendRtcpBye();
	mo_write_default_hotlist ();
	newsrc_kill ();
	if (get_pref_boolean(eUSE_GLOBAL_HISTORY))
		mo_write_global_history ();
	mo_write_pan_list ();
	preferences_armegeddon();
	exit (0);
}

MO_SIGHANDLER_RETURNTYPE ProcessExternalDirective (MO_SIGHANDLER_ARGS)
{
	char filename[512];
	char line[MO_LINE_LENGTH], *status, *directive, *url;
	FILE *fp;
	extern void mo_process_external_directive (char *directive, char *url);

	signal (SIGUSR1, SIG_IGN);
	/* Construct filename from our pid. */
	sprintf (filename, "/tmp/Mosaic.%d", getpid ());
	fp = fopen (filename, "r");
	if (!fp){
		signal (SIGUSR1, ProcessExternalDirective);
		return;
	}
	status = fgets (line, MO_LINE_LENGTH, fp);
	if (!status || !(*line)) {
		fclose(fp);
		signal (SIGUSR1, ProcessExternalDirective);
		return;
	}
	directive = strdup (line);

	/* We now allow URL to not exist, since some directives don't need it. */
	status = fgets (line, MO_LINE_LENGTH, fp);
	if (!status || !(*line))
		url = strdup ("dummy");
	else
		url = strdup (line);
	mo_process_external_directive (directive, url);
	free (directive);
	free (url);
	fclose(fp);
	signal (SIGUSR1, ProcessExternalDirective);
}  

static void RealFatal (void)
{
	signal (SIGBUS, 0);
	signal (SIGSEGV, 0);
	signal (SIGILL, 0);
	abort ();
}

static void FatalProblem (int sig)
{
	fprintf (stderr, "\nCongratulations, you have found a bug in\n");
	fprintf (stderr, "mMosaic %s on %s.\n\n", MO_VERSION_STRING, 
						MO_MACHINE_TYPE);
	fprintf (stderr, "If a core file was generated in your directory,\n");
	fprintf (stderr, "please do one of the following:\n\n");
	fprintf (stderr, "  %% dbx /path/to/Mosaic /path/to/core\n");
	fprintf (stderr, "  dbx> where\n\n");
	fprintf (stderr, "OR\n\n");
	fprintf (stderr, "  %% gdb /path/to/Mosaic /path/to/core\n");
	fprintf (stderr, "  gdb> where\n\n");
	fprintf (stderr, "Mail the results, and a description of what you were doing at the time,\n");
	fprintf (stderr, "(include any URLs involved!) to %s.\n\nWe thank you for your support.\n\n", 
			MO_DEVELOPER_ADDRESS);
	fprintf (stderr, "...exiting mMosaic now.\n\n");

	RealFatal ();
}

extern int _Xdebug ;
main (int argc, char **argv, char **envp)
{
	FILE *fp;

	_Xdebug =1;
	userPath=getenv("PATH");
/*      
        if (getenv("XKEYSYMDB")==NULL) {
                fprintf(stderr,"If you have key binding problems, set the environment variable XKEYSYMDB\nto the location of the correct XKeysymDB file on your
system.\n");
        }
*/

/*
	if (getenv("XKEYSYMDB")==NULL) {
		if (!(fp=fopen("/usr/openwin/lib/X11/XKeysymDB","r"))) {
			if (fp=fopen("/usr/openwin/lib/XKeysymDB","r")) {
				fclose(fp);
				putenv("XKEYSYMDB=/usr/openwin/lib/XKeysymDB");
			}
		} else {
			fclose(fp);
			putenv("XKEYSYMDB=/usr/openwin/lib/X11/XKeysymDB");
		}
	}
*/
	signal (SIGBUS, FatalProblem);
	signal (SIGSEGV, FatalProblem);
	signal (SIGILL, FatalProblem);

	/* Since we're doing lots of TCP, just ignore SIGPIPE altogether. */
	signal (SIGPIPE, SIG_IGN);

	InitChildProcessor();
	MoCCIPreInitialize();
#ifdef SVR4
	signal(SIGCHLD, (void (*)(int))ChildTerminated);
#else
	signal(SIGCLD, (void (*)())ChildTerminated);
#endif
	CheckKCMS();
	mo_do_gui (argc, argv);
}
