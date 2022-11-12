/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../libwww2/HTParams.h"

extern void HT_SetExtraHeaders(char **headers); /*####### bad */

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "newsrc.h"
#include "main.h"
#include "gui.h"
#include "child.h"
#include "hotlist.h"
#include "globalhist.h"
#include "cciBindings2.h"
#include "cache.h"
#include "proxy.h"
#include "xresources.h"

#ifdef MULTICAST                      
#include "../libmc/mc_rtp.h"          
#include "../libmc/mc_defs.h"         
#include "../libmc/mc_misc.h"         
#include "../libmc/mc_sockio.h"       
#include "../libmc/mc_dispatch.h"     
#include "../libmc/mc_action.h"       
#endif 

#ifdef SOLARIS
#ifdef  __cplusplus             
extern "C" {
#endif
extern int gethostname(char *name, int namelen); /* because bug in header*/
#ifdef  __cplusplus             
}
#endif
#endif

char 		*mMosaicRootDirName = NULL;
char		*mMosaicMachineWithDomain;
char		*mMosaicTmpDir = NULL;

int		mMosaicCCITrace;
int		mMosaicSrcTrace;

AppData 	mMosaicAppData;
Display 	*mMosaicDisplay;
XtAppContext	mMosaicAppContext; 
Widget		mMosaicToplevelWidget;
Colormap 	mMosaicColormap;
int		mMosaicVisualClass; /* visual class for 24bit support hack */

/* If mMosaicStartupDocument is set to anything but NULL, it will be the
   initial document viewed */ 

char *mMosaicStartupDocument = NULL;


#ifdef MULTICAST
int             mc_fdread;
int             mc_rtcp_fdread;
unsigned char   mc_len_alias = 0;
char *          mc_alias_name = "Unknow";
char *          mc_sess_name;
char *          mc_media_name;
#endif


/*######################*/
#ifdef MULTICAST
extern void 	McSendRtcpBye(void);
extern int	mc_multicast_enable;
#endif

char *userPath=NULL;
/*######################*/

void mo_exit (void)
{
#ifdef MULTICAST
	if(mc_multicast_enable)
		McSendRtcpBye();
#endif
	MMCacheWriteCache();
	mo_write_default_hotlist ();
	MMWriteHistory();
	newsrc_kill ();
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
		signal (SIGUSR1, (void(*)(int))ProcessExternalDirective);
		return;
	}
	status = fgets (line, MO_LINE_LENGTH, fp);
	if (!status || !(*line)) {
		fclose(fp);
		signal (SIGUSR1, (void(*)(int))ProcessExternalDirective);
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
	signal (SIGUSR1, (void(*)(int))ProcessExternalDirective);
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

/* name:    error_handler (PRIVATE)
 * purpose: Handle X errors.
 * inputs:
 *   - Display       *dsp: The X display.
 *   - XErrorEvent *event: The error event to handle.
 * returns: 
 *   0, if it doesn't force an exit.
 * remarks:
 *   The main reason for this handler is to keep the application
 *   from crashing on BadAccess errors during calls to XFreeColors().
 */
static int error_handler (Display *dsp, XErrorEvent *event)
{
	char buf[128];
	char ans[10];

	XUngrabPointer(dsp, CurrentTime);  /* in case error occurred in Grab */
/* BadAlloc errors (on a XCreatePixmap() call)
 * and BadAccess errors on XFreeColors are 'ignoreable' errors */
	if (event->error_code == BadAlloc ||
	    event->error_code == BadPixmap || /* HUM #### */
	    (event->error_code == BadAccess && event->request_code == 88)) {
		XGetErrorText (dsp, event->error_code, buf, 128);
		fprintf (stderr, "X Error: %s\n", buf);
		fprintf (stderr, "  Major Opcode:  %d\n", event->request_code);
		return 0;
	} else {
		printf("Press RETURN\007\n");
		gets(ans);
/* All other errors are 'fatal'. */ 
		XGetErrorText (dsp, event->error_code, buf, 128);
		fprintf (stderr, "X Error: %s\n", buf);
		fprintf (stderr, "  Major Opcode:  %d\n", event->request_code);
		mo_exit (); /* Try to close down gracefully. */
	}
	return 0; /* never makes it here.... */
}

extern int _Xdebug ;

void main (int argc, char **argv, char **envp)
{
	XVisualInfo vinfo, *vptr;
	int cnt;
	struct stat s;
	struct passwd *pw = getpwuid (getuid ());
	int status;
	char *home;
	char *fnam;
	FILE *fp;
	struct utsname mm_uname;
	char * machine;
	char *author_full_name;
	char *author_name;
	char *author_email;
	char * home_opt;

/*	_Xdebug =1;	*/
	signal (SIGBUS, FatalProblem);
	signal (SIGSEGV, FatalProblem);
	signal (SIGILL, FatalProblem);

/* Since we're doing lots of TCP, just ignore SIGPIPE altogether. */
	signal (SIGPIPE, SIG_IGN);

	userPath=getenv("PATH");	/* used in libnut/system.c */

/* Initialize stuff in ~/.mMosaic directory */

	home = pw->pw_dir;
	mMosaicRootDirName = (char *) malloc(strlen(home) + 12);
	sprintf(mMosaicRootDirName, "%s/.mMosaic",home);
	if ( stat(mMosaicRootDirName, &s) !=0 ) { /* create ~/.mMosaic */
		status = mkdir(mMosaicRootDirName,0755);
		if (status) {
			perror("Unable to create ~/.mMosaic directory");
			exit(1);
		}
	}

/* Write pid into "~/.mosaicpid". */
	fnam = (char *)malloc (strlen (mMosaicRootDirName) + 20);
	sprintf (fnam, "%s/.mosaicpid", mMosaicRootDirName);
	fp = fopen (fnam, "w");
	if (fp) {
		fprintf (fp, "%d\n", getpid());
		fclose (fp);
	}
	free (fnam); 

	InitChildProcessor();
	MoCCIPreInitialize();

#if defined(SVR4) || defined(__QNX__)
	signal(SIGCHLD, (void (*)(int))ChildTerminated);
#else
	signal(SIGCLD, (void (*)())ChildTerminated);
#endif


/* Initialize toplevel Widget, look argument and preference */

#ifdef MOTIF1_2
	XmRepTypeInstallTearOffModelConverter();
#endif  
	Xmx_n = 0;
	XmxSetArg (XmNwidth,1);
	XmxSetArg (XmNheight,1); 
	XmxSetArg (XmNmappedWhenManaged, False);
	mMosaicToplevelWidget = XtAppInitialize(&mMosaicAppContext, "mMosaic",
			options, XtNumber (options),
			&argc, argv, color_resources,
			Xmx_wargs, Xmx_n);
	Xmx_n=0;                      
	mMosaicDisplay = XtDisplay (mMosaicToplevelWidget);    

/*      XSynchronize(mMosaicDisplay,True);*/ 

	XtVaGetApplicationResources(mMosaicToplevelWidget,
		(XtPointer) &mMosaicAppData,
		resources, XtNumber (resources), NULL);

/* system option */

	mMosaicTmpDir = getenv ("TMPDIR");
	if (!mMosaicTmpDir)
		mMosaicTmpDir = "/tmp";

	/* If there's no docs directory assigned by the X resource,
	 *t hen look at MMOSAIC_DOCS_DIRECTORY environment variable
	 * and then at hardcoded default. */
	if (!mMosaicAppData.docs_directory) {      
		mMosaicAppData.docs_directory = getenv("MMOSAIC_DOCS_DIRECTORY");
		if (!mMosaicAppData.docs_directory)
			mMosaicAppData.docs_directory = DOCS_DIRECTORY_DEFAULT;
	}

/* initialize some libwww variable */

        uname(&mm_uname);
	HTAppVersion = (char *)malloc (sizeof(char) * (
			strlen(MO_VERSION_STRING) + strlen(mm_uname.sysname) +
			strlen(mm_uname.release) + strlen(mm_uname.machine) +20));
	sprintf(HTAppVersion, "%s (X11;%s %s %s)",
				MO_VERSION_STRING, mm_uname.sysname,
				mm_uname.release, mm_uname.machine);

        wWWParams.trace = mMosaicAppData.wwwTrace;
        wWWParams.global_xterm_str = mMosaicAppData.xterm_command;
        wWWParams.uncompress_program = mMosaicAppData.uncompress_command;
        wWWParams.gunzip_program = mMosaicAppData.gunzip_command;
        wWWParams.tweak_gopher_types = mMosaicAppData.tweak_gopher_types;
        wWWParams.max_wais_responses = mMosaicAppData.max_wais_responses;
        wWWParams.ftp_timeout_val = mMosaicAppData.ftp_timeout_val;
        wWWParams.ftpRedial=mMosaicAppData.ftpRedial;
        wWWParams.ftpFilenameLength=mMosaicAppData.ftpFilenameLength;
        wWWParams.ftpEllipsisLength=mMosaicAppData.ftpEllipsisLength;
        wWWParams.ftpEllipsisMode=mMosaicAppData.ftpEllipsisMode;
        wWWParams.use_default_extension_map =
			mMosaicAppData.use_default_extension_map;
	wWWParams.personal_extension_map = (char *)malloc(
		strlen(mMosaicRootDirName) + strlen("mime.types") + 8);
	sprintf(wWWParams.personal_extension_map, "%s/mime.types",
			mMosaicRootDirName);
        wWWParams.use_default_type_map = mMosaicAppData.use_default_type_map;
	wWWParams.personal_type_map = (char *)malloc(
		strlen(mMosaicRootDirName) + strlen("mailcap") + 8);   
	sprintf(wWWParams.personal_type_map, "%s/mailcap", mMosaicRootDirName);
	/* how many byte for activating icon */
        wWWParams.twirl_increment = mMosaicAppData.twirl_increment;

        if(mMosaicAppData.acceptlanguage_str) { 
                char **extras;
 
                extras =  (char**)malloc(sizeof(char *) * 2);
                extras[0]= (char*)malloc(
			strlen(mMosaicAppData.acceptlanguage_str) + 19);       
                sprintf(extras[0], "Accept-Language: %s", 
                        mMosaicAppData.acceptlanguage_str);
                extras[1] = NULL;
                HT_SetExtraHeaders(extras); 
        }

/* debug options */
        htmlwTrace = mMosaicAppData.htmlwTrace;
        mMosaicCCITrace = mMosaicAppData.cciTrace;
        mMosaicSrcTrace = mMosaicAppData.srcTrace;

/* First get the hostname. Then find out the full name, if possible. */
	if ( mMosaicAppData.full_hostname) {
		machine = mMosaicAppData.full_hostname;
	} else {
		machine = (char *)malloc (sizeof (char) * MAXHOSTNAMELEN);
		gethostname (machine, MAXHOSTNAMELEN);
	}

	mMosaicMachineWithDomain = machine ;
	if ( !strchr(machine,'.') ) { /* nodomain */
		mMosaicMachineWithDomain = (char*) malloc(strlen(machine) + 12);
		sprintf(mMosaicMachineWithDomain,"%s.nodomain",machine);
	}

	author_full_name=mMosaicAppData.author_full_name;
	author_name=mMosaicAppData.author_name;
	author_email=mMosaicAppData.author_email;

	if(!author_full_name) {
		mMosaicAppData.author_full_name = strdup(pw->pw_gecos);
	}
	if(!author_name) {
		mMosaicAppData.author_name = strdup(pw->pw_name);
	}
	if( !author_email ){
		mMosaicAppData.author_email = (char *) malloc(
			strlen(pw->pw_name)+strlen(mMosaicMachineWithDomain)+2);
		sprintf(mMosaicAppData.author_email,"%s@%s",
			pw->pw_name,mMosaicMachineWithDomain);
	}

/* Initialise the hotlist */
	MMHotlistInit(mMosaicRootDirName);

/* Initialize the history (visited anchor) before Cache */
	MMInitHistory(mMosaicRootDirName);

/* Initialise the cache data */
	MMCacheInit(mMosaicRootDirName);

/* initialize proxy lists */
        ReadProxies(mMosaicRootDirName);
        ReadNoProxies(mMosaicRootDirName);
 
#ifdef MULTICAST
	mc_alias_name = mMosaicAppData.author_email;
	mc_multicast_enable = 0;
	if (mMosaicAppData.mc_dest != NULL) {
		char * s;
 
		s = strchr(mMosaicAppData.mc_dest, '/');
		if (*s) {
			unsigned short port;
 
			*s = '\0';
			port = atoi(s+1);
			if (port%2!=0)
				port--;
			mc_port = htons(port);
			mc_rtcp_port = htons(port + 1);
		} else {
			fprintf(stderr,"invalid Multicast addr/port\n");
			exit(1);
		}
		if (mMosaicAppData.mc_debug)
			printf("dest/port: %s/%d, ttl=%d\n",
				mMosaicAppData.mc_dest, mc_port,
				mMosaicAppData.mc_ttl);
#ifdef IPV6
/* inet_pton () */
		if( ascii2addr(AF_INET6,mMosaicAppData.mc_dest,&mc_addr_ip_group) == -1){
			fprintf(stderr,"invalid IPV6Multicast addr\n");
			exit(1);       
		}                     
#else                                  
		mc_addr_ip_group = inet_addr(mMosaicAppData.mc_dest);
#endif
		mc_multicast_enable = 1;
/* initialise global variable for Multicast */
		mc_debug = mMosaicAppData.mc_debug;
		mc_sess_name = mMosaicAppData.mc_sess_name;
		mc_media_name = mMosaicAppData.mc_media_name;
		mc_ttl = mMosaicAppData.mc_ttl;
/*mc_dest become mc_addr_ip_group and mc_port */
		if( mMosaicAppData.mc_alias_name == NULL ){
			mc_alias_name = (char*) malloc(MC_MAX_ALIAS_SIZE + 1);
			strncpy(mc_alias_name,mMosaicAppData.author_email,
				MC_MAX_ALIAS_SIZE);
		} else {
			mc_alias_name = mMosaicAppData.mc_alias_name;
		}
		if (strlen(mc_alias_name) >= MC_MAX_ALIAS_SIZE )
			mc_alias_name[MC_MAX_ALIAS_SIZE] = '\0';
		mc_len_alias = strlen(mc_alias_name);
/* fd read of multicast socket */
		mc_fdread = McOpenRead(mc_addr_ip_group,mc_port,mc_ttl);
		mc_rtcp_fdread = McOpenRtcpRead(mc_addr_ip_group,mc_rtcp_port,mc_ttl);
		mc_init_gmt = McDate(); /* based on GM Time */
		mc_init_local_time = time(NULL); /* based on local time */
		mc_my_pid = getpid();
	}
#endif 

/* graphic and X variable . graphic options */

	mMosaicColormap =DefaultColormapOfScreen(XtScreen(mMosaicToplevelWidget));
	if (mMosaicAppData.install_colormap) {
		XColor bcolr;
		mMosaicColormap=XCreateColormap(mMosaicDisplay,
			RootWindow(mMosaicDisplay,DefaultScreen(mMosaicDisplay)),
			DefaultVisual(mMosaicDisplay,DefaultScreen(mMosaicDisplay)),
			AllocNone);
		XtVaGetValues(mMosaicToplevelWidget,
			XtNbackground, &(bcolr.pixel), NULL);
		XQueryColor(mMosaicDisplay,
			DefaultColormap(mMosaicDisplay, 
				DefaultScreen(mMosaicDisplay)), &bcolr);
		XtVaSetValues(mMosaicToplevelWidget,
			XmNcolormap, mMosaicColormap, NULL);
		XAllocColor(mMosaicDisplay, mMosaicColormap, &bcolr);
		XtVaSetValues(mMosaicToplevelWidget, 
			XmNbackground, bcolr.pixel, NULL);
	}

	vinfo.visualid = XVisualIDFromVisual(DefaultVisual(mMosaicDisplay,
		DefaultScreen (mMosaicDisplay)));
	vptr = XGetVisualInfo (mMosaicDisplay, VisualIDMask, &vinfo, &cnt);
#if defined(__cplusplus) || defined(c_plusplus)
	mMosaicVisualClass = vptr->c_class;          /* C++ */
#else     
	mMosaicVisualClass = vptr->class;
#endif
	XFree((char *)vptr);

	XSetErrorHandler(error_handler);

        if (mMosaicAppData.colors_per_inlined_image >256) {
                fprintf(stderr,"WARNING: Colors per inline image specification > 256.\n Auto-Setting to 256.\n");        
		mMosaicAppData.colors_per_inlined_image = 256;
        }

/* process remaining parameters */
	/* Pick up default or overridden value out of X resources. */
	/* Value of environment variable WWW_HOME overrides that. */
                                            
        if ((home_opt = getenv ("WWW_HOME")) != NULL)
                mMosaicAppData.home_document = home_opt;

	/* Value of argv[1], if it exists, sets startup_document. */
        if (argc > 1 && argv[1] && *argv[1])
                mMosaicStartupDocument = mo_url_prepend_protocol(argv[1]);
	/* Check for proper home document URL construction. */
        if (!strstr (mMosaicAppData.home_document, ":"))   
                mMosaicAppData.home_document = 
			mo_url_canonicalize_local(mMosaicAppData.home_document);
    
	mo_do_gui();

	if((mMosaicAppData.cciPort > 1023) && (mMosaicAppData.cciPort < 65536)) {
		MoCCIStartListening(mMosaicToplevelWidget,
			mMosaicAppData.cciPort);
	}

	XtAppMainLoop(mMosaicAppContext);
}
