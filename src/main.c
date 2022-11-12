/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#include <signal.h>
#include <pwd.h>
#include <unistd.h>
#include <netdb.h>
#include <assert.h>
#include <sys/wait.h>
#include <sys/utsname.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "newsrc.h"
#include "gui.h"
#include "child.h"
#include "hotlist.h"
#include "globalhist.h"
#include "mime.h"
#include "cache.h"
#include "proxy.h"
#include "xresources.h"
#include "pixmaps.h"
#include "URLParse.h"
#include "paf.h"
#include "../libmc/mc_main.h"

#include "bitmaps/iconify.xbm"
#include "bitmaps/iconify_mask.xbm"

#ifdef JAVASCRIPT
/* include the JS engine API header */
#include "jsapi.h"

  /*set up global JS variables, including global and custom objects */

JSVersion js_version;
JSRuntime *js_rt;
JSContext *js_cx;
JSObject  *js_glob, *js_it;
JSBool js_result;

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

#ifndef MAXHOSTNAMELEN
#define MAXHOSTNAMELEN 128		/* Arbitrary limit */
#endif

char 		*mMosaicRootDirName = NULL;
char		*mMosaicMachineWithDomain;
char		*mMosaicTmpDir = NULL;

int		mMosaicSrcTrace;

AppData 	mMosaicAppData;
Display 	*mMosaicDisplay;
XtAppContext	mMosaicAppContext; 
Widget		mMosaicToplevelWidget;
Colormap 	mMosaicColormap;
int		mMosaicVisualClass; /* visual class for 24bit support hack */
Window		mMosaicRootWindow;

Pixmap		mMosaicWinIconPixmap;   
Pixmap		mMosaicWinIconMaskPixmap;
Cursor		mMosaicBusyCursor;

/* If mMosaicStartupDocument is set to anything but NULL, it will be the
   initial document viewed */ 

char *mMosaicStartupDocument = NULL;

char		*mMosaicPersonalTypeMap = NULL;
char		*mMosaicPersonalExtensionMap = NULL;
char		*mMosaicAppVersion;

#ifdef MULTICAST
extern mo_window * mc_send_win;
#endif

/* --------------BalloonHelpStuff---------------------------------------- */

static void BalloonHelpMe(Widget w, XEvent * event)
{
	BalloonInfoData *info;

	XtVaGetValues(w, XmNuserData, (XtPointer) &info, NULL);
	mo_gui_notify_progress(info->msg,info->win);
}

static void UnBalloonHelpMe(Widget w, XEvent * event)
{
	BalloonInfoData *info;

	XtVaGetValues(w, XmNuserData, (XtPointer) &info, NULL);
	mo_gui_notify_progress(" ",info->win);
}

char * mMosaicBalloonTranslationTable = "<Enter>: BalloonHelpMe()\n\
	<Leave>: UnBalloonHelpMe()";

static XtActionsRec balloon_action[] = {
	{"BalloonHelpMe", (XtActionProc)BalloonHelpMe},
	{"UnBalloonHelpMe", (XtActionProc)UnBalloonHelpMe}
};
/* to use balloon help, add these bits to your widget ...  BJS 2/7/96
 *    XmNtranslations, XtParseTranslationTable(xlattab),
 *    XmNuserData, (xtpointer) "Balloon Help String!",
 */
/* ------------------------------------------------------ */

/*######################*/
#ifdef MULTICAST
extern int	mc_multicast_enable;
#endif

char *userPath=NULL;
/*######################*/

void mo_exit (void)
{
#ifdef MULTICAST
/*
	if(mc_multicast_enable)
		McSendRtcpBye();
*/
#endif
	MMCacheWriteCache();
	mo_write_default_hotlist ();
	MMWriteHistory();
	newsrc_kill ();
#ifdef CHECK_MEMORY_LEAK
	MIME_Postlude();
#endif
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
	assert(0);
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
		fgets(ans, sizeof(ans), stdin);
/* All other errors are 'fatal'. */ 
		XGetErrorText (dsp, event->error_code, buf, 128);
		fprintf (stderr, "X Error: %s\n", buf);
		fprintf (stderr, "  Major Opcode:  %d\n", event->request_code);
		mo_exit (); /* Try to close down gracefully. */
	}
	return 0; /* never makes it here.... */
}

extern int _Xdebug ;

int main (int argc, char **argv, char **envp)
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
	mo_window *win;		/* The first mo_window */
	McMoWType mc_t; /*  A la creation : Soit type MAIN ou type UNICAST */
	RequestDataStruct rds;
	struct hostent * mmmmm;	/* mjr++ 12 Jan 1998 */

/*	_Xdebug =1; */
	signal (SIGBUS, FatalProblem);
	signal (SIGSEGV, FatalProblem);
	signal (SIGILL, FatalProblem);
	signal (SIGFPE, FatalProblem);

/* Since we're doing lots of TCP, just ignore SIGPIPE altogether. */
	signal (SIGPIPE, SIG_IGN);

	memset(&mMosaicAppData,0,sizeof(AppData));
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

/* From seanm@storm.ca  Fri Dec  8 04:44:43 2000 */
/* old : #if defined(SVR4) || defined(__QNX__) || defined(NETBSD) || defined(FreeBSD) */

/* new */
#if defined(SVR4) || defined(__QNX__) || defined(NETBSD) || defined(FreeBSD) || defined(linux)
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
	mMosaicRootWindow = DefaultRootWindow(mMosaicDisplay);

/*      	XSynchronize(mMosaicDisplay,True); */

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
	mMosaicAppVersion = (char *)malloc (sizeof(char) * (
			strlen(MO_VERSION_STRING) + strlen(mm_uname.sysname) +
			strlen(mm_uname.release) + strlen(mm_uname.machine) +20));
	sprintf(mMosaicAppVersion, "%s (X11;%s %s %s)",
				MO_VERSION_STRING, mm_uname.sysname,
				mm_uname.release, mm_uname.machine);

	mMosaicPersonalTypeMap = (char *)malloc(
		strlen(mMosaicRootDirName) + strlen("mailcap") + 8);   
	sprintf(mMosaicPersonalTypeMap, "%s/mailcap", mMosaicRootDirName);

	HTPresentationInit();

	mMosaicPersonalExtensionMap = (char *)malloc(
		strlen(mMosaicRootDirName) + strlen("mime.types") + 8);
	sprintf(mMosaicPersonalExtensionMap, "%s/mime.types",
			mMosaicRootDirName);

	HTExtensionMapInit();

/* debug options */
        mMosaicSrcTrace = mMosaicAppData.srcTrace;

/* First get the hostname. Then find out the full name, if possible. */
	if ( mMosaicAppData.full_hostname) {
		machine = mMosaicAppData.full_hostname;
	} else {
		machine = (char *)malloc (sizeof (char) * MAXHOSTNAMELEN);
		gethostname (machine, MAXHOSTNAMELEN);
	}

/* mjr 12 Jan 1998: really get fully qualified host name.
works on my linux box, hopefully on solaris, too */

	mmmmm = gethostbyname(machine);
	if( mmmmm != NULL ) {
		if ( !strchr(mmmmm->h_name,'.') ) { /* nodomain */
			fprintf(stderr, "domain information for %s not found\n", machine );
			mMosaicMachineWithDomain = (char*) malloc(strlen(machine) + 12);
			sprintf(mMosaicMachineWithDomain,"%s.nodomain",machine);
		} else {
			mMosaicMachineWithDomain = strdup(mmmmm->h_name);
		}
	} else {
		fprintf(stderr, "host information for %s not found\n", machine );
		mMosaicMachineWithDomain = machine ;
		if ( !strchr(machine,'.') ) { /* nodomain */
			mMosaicMachineWithDomain = (char*) malloc(strlen(machine) + 12);
			sprintf(mMosaicMachineWithDomain,"%s.nodomain",machine);
		}
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
	mMosaicAppData.print_us=0;

/* Initialise the hotlist */
	MMHotlistInit(mMosaicRootDirName);

/* Initialize the history (visited anchor) before Cache */
	MMInitHistory(mMosaicRootDirName);

/* Initialise the cache data */
	MMCacheInit(mMosaicRootDirName);

/* initialize proxy lists */
        ReadProxies(mMosaicRootDirName);
        ReadNoProxies(mMosaicRootDirName);
 
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
	HTMLInitColors(mMosaicToplevelWidget, 0);

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
                mMosaicStartupDocument = UrlGuess(argv[1]);
	/* Check for proper home document URL construction. */
        if (!strstr (mMosaicAppData.home_document, ":"))   
                mMosaicAppData.home_document = 
			mo_url_canonicalize_local(mMosaicAppData.home_document);
    

/* initialize some global graphic objects Pixmap, cursor, and canvas data*/

	mMosaicWinIconPixmap = XCreatePixmapFromBitmapData(mMosaicDisplay,
		mMosaicRootWindow,
		(char*)iconify_bits, iconify_width, iconify_height, 1, 0,
		1);
/*DefaultDepthOfScreen(DefaultScreenOfDisplay(mMosaicDisplay))*/

	mMosaicWinIconMaskPixmap = XCreatePixmapFromBitmapData(mMosaicDisplay,
		mMosaicRootWindow,
		(char*)iconify_mask_bits, iconify_mask_width, iconify_mask_height, 0, 1,
		1);

	mMosaicBusyCursor = XCreateFontCursor (mMosaicDisplay, XC_watch);

	loadAgents();      /* Agent Spoofing */

/* register balloon_action */
	XtAppAddActions(mMosaicAppContext, balloon_action, 2);

/* realize the toplevel because we need a window (X) to create other object */
/* you may notice: mMosaicToplevelWidget is never mapped !!! */

	XtRealizeWidget (mMosaicToplevelWidget);

	MakePixmaps(mMosaicToplevelWidget); /* global icons and pixmap */

/* Arm SIGUSR1 to process external directive */
	signal (SIGUSR1, (void(*)(int))ProcessExternalDirective);

/*##################################*/
/* ### Create the FIRST MAIN WINDOW */
/*##################################*/
	mc_t = MC_MO_TYPE_UNICAST;
	win = mo_make_window(NULL, mc_t);
#ifdef MULTICAST
	McInit(win);	/* Initialize the multicast database listen mode*/
#endif                               

#ifdef JAVASCRIPT
	version = JSVERSION_DEFAULT;
	rt = JS_NewRuntime(8L * 1024L * 1024L);
	if (!rt) {
		fprintf(stderr,"JS_NewRuntime: does not success\n");
		exit(1);
	}

	cx = JS_NewContext(rt, 8192);
	if (!cx) {
		fprintf(stderr,"JS_NewContext:does not success\n");
		exit(1);
	}
	JS_SetBranchCallback(cx, my_BranchCallback);
	JS_SetErrorReporter(cx, my_ErrorReporter);

glob = JS_NewObject(cx, &global_class, NULL, NULL);
    if (!glob)
        return 1;   
    if (!JS_InitStandardClasses(cx, glob))
        return 1;
    if (!JS_DefineFunctions(cx, glob, shell_functions))
        return 1;
    
    /* Set version only after there is a global object. */
    if (version != JSVERSION_DEFAULT)
        JS_SetVersion(cx, version);
    
    it = JS_DefineObject(cx, glob, "it", &its_class, NULL, 0);
    if (!it)
        return 1;
    if (!JS_DefineProperties(cx, it, its_props))
        return 1;

/*
	                                      
    JS_DestroyContext(cx);            
    JS_DestroyRuntime(rt);           
    JS_ShutDown();
*/
#endif

/* we have build an empty mo_window. Now we need to fill it with an HTML doc. */
/* PAF this request */

	rds.req_url = mMosaicStartupDocument ?
		mMosaicStartupDocument : mMosaicAppData.home_document;
	rds.ct = NULL;
	rds.is_reloading = False;
	rds.post_data = NULL;
	rds.gui_action = HTML_LOAD_CALLBACK;
	win->navigation_action = NAVIGATE_NEW;
	MMPafLoadHTMLDocInWin(win, &rds);

/* GO ! */

#ifndef DEBUG_EVENT
	XtAppMainLoop(mMosaicAppContext);
#else
	for (;;) {
		XEvent event;

		XtAppNextEvent(mMosaicAppContext, &event);
		XtDispatchEvent(&event);
	}
#endif
}
