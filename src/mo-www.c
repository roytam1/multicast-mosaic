/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <ctype.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#include "../libmc/mc_defs.h"
#include "libhtmlw/HTML.h"
#include "libhtmlw/HTMLP.h"
#include "libhtmlw/HTMLPutil.h"
#include "../libwww2/HText.h"
#include "libwww2/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "gui-dialogs.h"
#include "gui-documents.h"
#include "globalhist.h"
#include "libnut/system.h"
#define __SRC__
#include "../libwww2/HTAAUtil.h"
#include "HTUtils.h"
#include "HTString.h"
#include "tcp.h"
#include "HTTCP.h"
#include "HTParse.h"
#include "HTAccess.h"
#include "HText.h"
#include "HTList.h"
#include "HTInit.h"
#include "mo-www.h"
#include "cache.h"

#define __MAX_HOME_LEN__	256
#define MO_BUFFER_SIZE		8192
#define MAX_AGENTS		51

extern int 	securityType;
extern Pixmap 	*IconPixSmall, *IconPixBig,*IconPix;
extern int 	interrupted;			/* From gui-documents.c */
char 		*saveFileName=NULL;	/*SWP 9.21.95 - Binary save filename*/
extern int 	cci_get; /* From cciBindings.c */
extern int 	cci_docommand;
int 		numAgents;	 /* SWP -- Agent Spoofing */
char 		**agent;
int 		selectedAgent=0;

					/* From HTTP.c */
char 		*HTAppVersion;
extern char 	*HTLibraryVersion;
extern int 	do_post;
extern char 	*post_content_type;
extern char 	*post_data;
extern int 	do_put;  
extern int 	put_file_size;
extern FILE 	*put_fp;
extern char 	*HTTP_expires;
extern int 	is_uncompressed;
extern int 	binary_transfer;
char 		*force_dump_filename = NULL; /* hook to force dumping binary data
                                           straight to file named by... */
extern char 	*HTTP_last_modified; /* From HTMIME.c - AF */
extern char 	*use_this_url_instead; /* Check from HTAccess.c. */

/* Mosaic does NOT use either the anchor system or the style sheet
   system of libwww. */

static HText* 	HTMainText = 0;      /* Equivalent of main window */

/* purpose: Do really nasty things to a stream of HTML that just got
 *          pulled over from a server.
 *          Given a URL, go fetch information.
 * inputs:  
 *   - char       *url: The URL to fetch.
 *   - 		 (global HTMainText is assumed to contain current
 *               HText object)
 * returns: 
 *   The text intended for display (char *).
 */
static char *doit (char *url, mo_window *win)
{
	char *txt;
	int rv;
	int len;
	char * cached_fname;
	int fd;

	txt = NULL;
	if (HTMainText) {
		if ( HTMainText->htmlSrc)
			free(HTMainText->htmlSrc);
		free (HTMainText);
		HTMainText = NULL;
	}
	XmxApplyPixmapToLabelWidget(win->logo, IconPix[0]);
	is_uncompressed=0;   

	len = MMCacheFetchCachedData(url, &cached_fname);
	if ( len ) {	/* something in cache */
                HTMainText = (HText *)malloc (sizeof (HText));
		HTMainText->expandedAddress = NULL;
		HTMainText->simpleAddress = NULL;
		HTMainText->htmlSrc = (char*)malloc(len +1);
		HTMainText->srcalloc = len+1;
		HTMainText->srclen = len;
		fd = open(cached_fname, O_RDONLY);
		read(fd,HTMainText->htmlSrc,len);
		close(fd);
		HTMainText->htmlSrc[len] = '\0';
		return HTMainText->htmlSrc;
	}
/* nothing in cache */
	rv = HTLoadAbsolute(url, (caddr_t) win);	/* interface to libWWW*/
	if (rv == 1) {
		if (HTMainText && HTMainText->htmlSrc){
			MMCachePutDataInCache(HTMainText->htmlSrc,
				HTMainText->srclen, url, &cached_fname);
			return HTMainText->htmlSrc;
		}
	} else {
		if (rv == -1) {
			interrupted = 1;
			return NULL;
		}
	}

/*  Just because we errored out, doesn't mean there isn't markup to 
 ** look at.  For example, an FTP site that doesn't let a user in because
 ** the maximum number of users has been reached often has a message
 ** telling about other mirror sites.  The failed FTP connection returns
 ** a message that is taken care of below.  
 */
	if (HTMainText) {
		txt = HTMainText->htmlSrc;

		if (cci_get){
			if (txt)
				return txt;
			else {/* take care of failed local access */
				HTMainText->htmlSrc = txt = strdup("<H1>ERROR</H1>"); 
				HTMainText->srclen = strlen(txt);
				HTMainText->srcalloc = HTMainText->srclen+1;
			}
		}
		return txt;
	} else {
		HTMainText = (HText *)malloc (sizeof (HText));
		HTMainText->expandedAddress = NULL;
		HTMainText->simpleAddress = NULL;
		HTMainText->htmlSrc = NULL;
		HTMainText->srcalloc = 0;
		HTMainText->srclen = 0;
	}

	/* Return proper error message if we experienced redirection. */
	if (use_this_url_instead)
		url = use_this_url_instead;
	HTMainText->htmlSrc = txt = (char *)malloc ((strlen (url) + 200) * sizeof (char));
	sprintf (txt, "<H1>ERROR</H1> Requested document (URL %s) could not be accessed.<p>The information server either is not accessible or is refusing to serve the document to you.<p>", url);
	HTMainText->srclen = strlen(txt);
	HTMainText->srcalloc = HTMainText->srclen+1;
	securityType=HTAA_UNKNOWN;
	return txt;
}

/* purpose: Given a URL, pull 'er over.
 * inputs:  
 *   - char       *url: The URL to pull over.
 * returns: 
 *   Text to display (char *).
 */
char *mo_pull_er_over (char *url,mo_window * win)
{
	char *rv;

	if (binary_transfer) {
		force_dump_filename = mo_tmpnam(url);
	}
	if (saveFileName!=NULL)
		free(saveFileName);
	saveFileName=strdup(url);

	if (HTTP_last_modified) {
		free(HTTP_last_modified);
		HTTP_last_modified = 0;
	}
	rv = doit (url, win);
	if (binary_transfer) {
		force_dump_filename = NULL;
	}
	return rv;
}

/* purpose: Given a URL, pull 'er over in such a way that no format
 *          handling takes place and the data gets dumped in the filename
 *          of the calling routine's choice.
 * inputs:  
 *   - char  *url: The URL to pull over.
 *   - char *fnam: Filename in which to dump the received data.
 * returns: 
 *   mo_succeed on success; mo_fail otherwise.
 * remarks: 
 *   This routine is called when we know there's data out there we
 *   want to get and we know we just want it dumped in a file, no
 *   questions asked, by the WWW library.  Appropriate global flags
 *   are set to make this happen. This must be made cleaner.
 */
mo_status mo_pull_er_over_virgin (char *url, char *fnam, mo_window *win)
{
	int rv;

	force_dump_filename = fnam;
	if (saveFileName!=NULL)
		free(saveFileName);
	saveFileName=strdup(url);
	rv = HTLoadAbsolute (url, (caddr_t)win);
	force_dump_filename = NULL;
	if (rv == 1) {
		return mo_succeed;
	}
	if (rv == -1) {
		interrupted = 1;
		return mo_fail;
	}
	return mo_fail;
}


char *mo_post_pull_er_over (char *url, char *content_type, char *data,
	mo_window * win)
{
	char *rv;

	do_post = 1;
	post_content_type = content_type;
	post_data = data;
	if (binary_transfer) {
		force_dump_filename = mo_tmpnam(url);
	}
	if (HTTP_last_modified) {
		free(HTTP_last_modified);
		HTTP_last_modified = 0;
	}
	rv = doit (url, win);
	if (binary_transfer) {
		force_dump_filename = NULL;
	}
	do_post = 0;
	return rv;
}

mo_status mo_re_init_formats (void)
{
	HTReInit ();
	return mo_succeed;
}

/* ------------------------------------------------------------------------ */

HText *HText_new (void)
{
	HText *htObj = (HText *)malloc (sizeof (HText));

	htObj->expandedAddress = NULL;
	htObj->simpleAddress = NULL;
	htObj->htmlSrc = NULL;
	htObj->srcalloc = 0;
	htObj->srclen = 0;

	/* Free the struct and  the text. */
	if (HTMainText){
		if (HTMainText->htmlSrc)
			free(HTMainText->htmlSrc);
		free (HTMainText);
	}
	HTMainText = htObj;
	return htObj;
}

void HText_free (HText *self)
{
	if (self) {
		if (self->htmlSrc)
			free (self->htmlSrc);
		free (self);
	}
}

void HText_beginAppend (HText *text)
{
	HTMainText = text;
}

void HText_endAppend (HText *text)
{
	if (text)
		HText_appendCharacter (text, '\0');
	HTMainText = text;
}

void HText_doAbort (HText *self)
{
  /* Clean up -- we want to free htmlSrc here. */
	if (self) {
		if (self->htmlSrc)
			free (self->htmlSrc);
		self->htmlSrc = NULL;
		self->srcalloc = 0;
		self->srclen = 0;
	}
}

static void new_chunk (HText *text)
{
	if (text->srcalloc == 0) {
		text->htmlSrc = (char *)malloc (MO_BUFFER_SIZE);
		text->htmlSrc[0] = '\0';
	} else {
		text->htmlSrc = (char *)realloc
			(text->htmlSrc, text->srcalloc + MO_BUFFER_SIZE);
	}
	text->srcalloc += MO_BUFFER_SIZE;
}

void HText_appendCharacter (HText *text, char ch)
{
	if (text->srcalloc < text->srclen + 1)
		new_chunk (text);
	text->htmlSrc[text->srclen++] = ch;
}

void HText_appendText (HText *text, const char *str)
{
	int len;

	if (!str)
		return;
	len = strlen (str);
	while (text->srcalloc < text->srclen + len + 1)
		new_chunk (text);
	memcpy((text->htmlSrc + text->srclen), str, len);
	text->srclen += len;
	text->htmlSrc[text->srclen] = '\0';
}

void HText_appendBlock (HText *text, const char *data, int len)
{
	if (!data)
		return;

	while (text->srcalloc < text->srclen + len + 1)
		new_chunk (text);

	memcpy((text->htmlSrc + text->srclen), data, len);
	text->srclen += len;
	text->htmlSrc[text->srclen] = '\0';
}

char *HText_getText (HText *me)
{
	if (me)
		return me->htmlSrc;
	else
		return NULL;
}

int HText_getTextLength (HText *me)
{
	if (me)
		return me->srclen;
	else
		return 0;
}

/* purpose: Given a string, checks to see if it can stat it. If so, it is
 *   assumed the user expects to open the file, not a web site. If not, we
 *   assume it is supposed to be a server and prepend the default protocol.
 * inputs:  
 *   - char    *url: URL to canonicalize.
 * returns: 
 *   The canonical representation of the URL.
 * remarks: 
 *   Written by spowers@ncsa.uiuc.edu
 */
char *fileOrServer(char *url) 
{
	struct stat buf;
	char *xurl;

        /*   At this point we know the URL specified is of the form:
         *   shire.ncsa.uiuc.edu[:PORT]/path/to/something
         */     
                
        if (!stat(url,&buf)) { /*its a file and we have access*/
                xurl=mo_url_canonicalize_local(url);
        } else {
                xurl=(char *)calloc(strlen(url)+15,sizeof(char));
                sprintf(xurl,"http://%s",url);
        } 
        return(xurl);  
}

/* purpose: To prepend the proper protocol to the url if it is not present.
 * inputs:  
 *   - char    *url: URL to canonicalize.
 * returns: 
 *   The canonical representation of the URL.
 * remarks: 
 *   Contributed by martin@gizmo.lut.ac.uk, modified by spowers@ncsa.uiuc.edu
 */
char *mo_url_prepend_protocol(char *url)
{
	char *xurl;

	if (strncasecmp(url,"mailto:",7) && strncasecmp(url,"news:",5) &&
	    !strstr(url,"://")) { /*no protocol specified, default*/
		if (!strncmp(url, "www.", 4)) {
			xurl = (char *)malloc(strlen(url) + (8 * sizeof(char)));
			sprintf(xurl, "http://%s", url);
		} else if (!strncmp(url, "gopher.", 7)) {
			xurl = (char *)malloc(strlen(url) + (10 * sizeof(char)));
			sprintf(xurl, "gopher://%s", url);
		} else if (!strncmp(url, "ftp.", 4)) {
			xurl = (char *)malloc(strlen(url) + (7 * sizeof(char)));
			sprintf(xurl, "ftp://%s", url);
		} else if (!strncmp(url, "wais.", 5)) {
			xurl = (char *)malloc(strlen(url) + (8 * sizeof(char)));
			sprintf(xurl, "wais://%s", url);
		} else {
			xurl=fileOrServer(url);
		}
	} else { /*protocol was specified*/
		xurl=strdup(url);
	}
	return(xurl);
}

/* purpose: Turn a URL into its canonical form, based on the previous
 *          URL in this context (if appropriate).  
 *          INTERNAL ANCHORS ARE STRIPPED OFF.
 * inputs:  
 *   - char    *url: URL to canonicalize.
 *   - char *oldurl: The previous URL in this context.
 * returns: 
 *   The canonical representation of the URL.
 * remarks: 
 *   All we do is call HTParse.
 */
char *mo_url_canonicalize (char *url, char *oldurl)
{
  /* We LOSE anchor information. */
  return HTParse (url, oldurl,
                  PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                  PARSE_PUNCTUATION);
}

/* purpose: Turn a URL into its canonical form, based on the previous
 *          URL in this context (if appropriate).  
 *          INTERNAL ANCHORS ARE *NOT* STRIPPED OFF.
 * inputs:  
 *   - char    *url: URL to canonicalize.
 *   - char *oldurl: The previous URL in this context.
 * returns: 
 *   The canonical representation of the URL.
 * remarks: 
 *   All we do is call HTParse.
 */
char *mo_url_canonicalize_keep_anchor (char *url, char *oldurl)
{
  char *rv;
  /* We KEEP anchor information already present in url,
     but NOT in oldurl. */
  oldurl = HTParse (oldurl, "", PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                    PARSE_PUNCTUATION);
  rv = HTParse (url, oldurl,
                PARSE_ACCESS | PARSE_HOST | PARSE_PATH |
                PARSE_PUNCTUATION | PARSE_ANCHOR);
  /* We made a new copy of oldurl, so free the new copy. */
  free (oldurl);
  return rv;
}

/* purpose: Given a URL that may or may not contain an internal anchor,
 *          return a form that corresponds to a unique document -- i.e.,
 *          a URL that has annotations different than all other
 *          URL's, etc.  Generally this will be the URL without the
 *          target anchor, except for automatically generated representations
 *          of internal parts of HDF files.
 * inputs:  
 *   - char *url: The URL.
 * returns: 
 *   URL corresponding to a unique document.
 */
char *mo_url_to_unique_document (char *url)
{
	char *target = mo_url_extract_anchor (url), *rv;

	rv = mo_url_canonicalize (url, "");
	if (target)
		free (target);
	return rv;
}

/* purpose: Given a URL (presumably in canonical form), extract
 *          the internal anchor, if any.
 * inputs:  
 *   - char *url: URL to use.
 * returns: 
 *   Internal anchor, if one exists in the URL; else NULL.
 */
char *mo_url_extract_anchor (char *url)
{
	return HTParse (url, "", PARSE_ANCHOR);
}

/* purpose: Given a URL (presumably in canonical form), extract
 *          the access method, if any.
 * inputs:  
 *   - char *url: URL to use.
 * returns: 
 *   Access method, if one exists in the URL; else NULL.
 */
static char *mo_url_extract_access (char *url, char *oldurl)
{
	return HTParse (url, oldurl, PARSE_ACCESS);
}

char *mo_url_canonicalize_local (char *url)
{
  /* Convex OS apparently sucks. */
#ifdef CONVEX
  char blah[129];
  char *cwd = getcwd (blah, 128);
#else
  char *cwd = getcwd (NULL, 128);
#endif
  char *tmp;

  if (!url)
    return NULL;

  tmp = (char *)malloc ((strlen (url) +
                         strlen (cwd) + 32));
  if (url[0] == '/')
    sprintf (tmp, "file://localhost%s", url);
  else
    sprintf (tmp, "file://localhost%s/%s", cwd, url);

  /* Sigh... */
#ifndef CONVEX
  free (cwd);
#endif
  return tmp;
}

/* purpose: Make a temporary, unique filename.
 * inputs:  
 *   none
 * returns: 
 *   The new temporary filename.
 * remarks: 
 *   We call tmpnam() to get the actual filename, and use the value
 *   of mMosaicTmpDir, if any, for the directory.
 * added code for url=NULL, bjs, 2/7/96
 */
char *mo_tmpnam (char *url)
{
	extern void MoCCIAddFileURLToList(char *, char *);
	char *tmp ;

	tmp = tempnam (mMosaicTmpDir,"mMo");
	if(url) MoCCIAddFileURLToList(tmp,url);
	return tmp;
}

/* ------------------------------ dumb stuff ------------------------------ */

/* Grumble grumble... */
#if defined(ultrix) || defined(VMS) || defined(NeXT) || defined(M4310) || defined(vax)
char *strdup (char *str)
{
  char *dup;

  dup = (char *)malloc (strlen (str) + 1);
  dup = strcpy (dup, str);

  return dup;
}
#endif

/* Error from the library */
void application_error(char *str, char *title) 
{
	XmxMakeErrorDialogWait(mMosaicToplevelWidget, mMosaicAppContext, str, title, "OK");
}

/* Feedback from the library. */
void application_user_feedback (char *str, mo_window * win)
{
  XmxMakeInfoDialog (win->base, str, "mMosaic: Application Feedback");
  XmxManageRemanage (Xmx_w);
}

void application_user_info_wait (char *str)
{
XmxMakeInfoDialogWait(mMosaicToplevelWidget, mMosaicAppContext, str, "mMosaic: Application Feedback", "OK");
}

char *prompt_for_string (char *questionstr,mo_window * win)
{
  return XmxModalPromptForString (win->base, mMosaicAppContext,
                                  questionstr, "OK", "Cancel");
}

char *prompt_for_password (char *questionstr,mo_window * win)
{
  return XmxModalPromptForPassword (win->base, mMosaicAppContext,
                                    questionstr, "OK", "Cancel");
}

int prompt_for_yes_or_no (char *questionstr, mo_window * win)
{
  return XmxModalYesOrNo (win->base, mMosaicAppContext,
                          questionstr, "Yes", "No");
}

/* Simply loop through a string and convert all newlines to spaces. */
/* We now remove leading whitespace as well */
/* and trailing whitesapce as well */
char *mo_convert_newlines_to_spaces (char *str)
{
  int i;
  char *tptr;
  char * ptr;

  if (!str)
    return NULL;

  for (i = 0; i < strlen (str); i++)
    if (str[i] == '\n')
      str[i] = ' ';

  tptr = str;
  while ((*tptr != '\0')&&(isspace((int)(*tptr))))
	tptr++;

  if (tptr != str)
	memcpy(str, tptr, (strlen(tptr) + 1));

  for(ptr=(str + strlen(str) -1); ptr && *ptr == ' '; ptr--)
	*ptr = '\0';

  return str;
}

/* ---------------------------- escaping code ----------------------------- */

static unsigned char isAcceptable[96] =
/*   0 1 2 3 4 5 6 7 8 9 A B C D E F */
{    0,0,0,0,0,0,0,0,0,0,1,0,0,1,1,0,	/* 2x   !"#$%&'()*+,-./	 */
     1,1,1,1,1,1,1,1,1,1,0,0,0,0,0,0,	/* 3x  0123456789:;<=>?	 */
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 4x  @ABCDEFGHIJKLMNO  */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,1,	/* 5x  PQRSTUVWXYZ[\]^_	 */
     0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,	/* 6x  `abcdefghijklmno	 */
     1,1,1,1,1,1,1,1,1,1,1,0,0,0,0,0 };	/* 7x  pqrstuvwxyz{\}~	DEL */

#define MO_HEX(i) (i < 10 ? '0'+i : 'A'+ i - 10)

/* The string returned from here, if any, can be free'd by caller. */
char *mo_escape_part (char *part)
{
  char *q;
  char *p;		/* Pointers into keywords */
  char *escaped;

  if (!part)
    return NULL;

  escaped = (char *)malloc (strlen (part) * 3 + 1);
  
  for (q = escaped, p = part; *p != '\0'; p++) {
      /*
       * This makes sure that values 128 and over don't get
       * converted to negative values.
       */
      int c = (int)((unsigned char)(*p));
      if (*p == ' ') {
          *q++ = '+';
        } else if (c >= 32 && c <= 127 && isAcceptable[c-32]) {
          *q++ = *p;
        } else {
          *q++ = '%';
          *q++ = MO_HEX(c / 16);
          *q++ = MO_HEX(c % 16);
        }
    }
  *q=0;
  return escaped;
}

static char mo_from_hex (char c)
{
  return ((c >= '0' && c <= '9') ? (c - '0') : 
          ((c >= 'A' && c <= 'F') ? (c - 'A' + 10) : 
           (c - 'a' + 10)));
}

char *mo_unescape_part (char *str)
{
	char *p = str, *q = str;

	while (*p) {	 /* Plus's turn back into spaces. */
		if (*p == '+') {
			*q++ = ' ';
			p++;
		} else if (*p == '%') {
			p++;
			if (*p) 
				*q = mo_from_hex(*p++) * 16;
			if (*p) 
				*q += mo_from_hex(*p++);
			q++;
		} else {
			*q++ = *p++; 
		}
	}
	*q++ = 0;
	return str;
}

/* ---------------------------- Agent Spoofing ---------------------------- */
/*
 * Agent Spoofing is simple. NCSA's real agent is always a member of the
 *   menu. Any more than that, you can add to the file in your home directory
 *   called ".mosaic-spoof-agents".
 */

void loadAgents(void) 
{
	FILE *fp;
	char fname[BUFSIZ],buf[512];
	char *ptr;
	char buf1[512];

	agent=(char **)calloc(MAX_AGENTS+1,sizeof(char *));
	sprintf(buf1,"mMosaic/%s  libwww/%s",
		HTAppVersion ? HTAppVersion : "0.0",
		HTLibraryVersion);
	agent[0]=strdup(buf1);
	numAgents=1;

	sprintf(fname,"%s/agents",mMosaicRootDirName);

	if (!(fp=fopen(fname,"r")))
		return;

	while (!feof(fp)) {
		fgets(buf,511,fp);
		if (feof(fp))
			break;
		if (*buf && *buf!='#') {
			buf[strlen(buf)-1]='\0';
			for (ptr=buf; *ptr && isspace(*ptr); ptr++);
			if (*ptr=='+') { /* This is to be the default*/
				if (*(ptr+1)) {
					agent[numAgents]=strdup(ptr+1);
					selectedAgent=numAgents;
				} else
					continue;
			} else 	
				if (*ptr) {
					agent[numAgents]=strdup(ptr);
				} else
					continue;
			numAgents++;
		}
	}
	fclose(fp);
}

/* Originally in whine.c and then in techsupport.c...now it's here. - SWP */ 
/* ------------------------------------------------------------------------ */
 
static FILE *_fp = NULL;
 
FILE *mo_start_sending_mail_message (char *to, char *subj,
                                     char *content_type, char *url)         
{
	char cmd[2048];
	char *tmp;

	if (!to)
		return NULL;

	if(!strcmp(content_type,"url_only")){
		content_type = "text/plain"; 
	}
/* Try listing address on command line. */
	for (tmp = to; *tmp; tmp++)
		if (*tmp == ',')
			*tmp = ' '; 

	if (mMosaicAppData.mail_filter_command && content_type &&
	    strcmp (content_type, "application/postscript")) {
		sprintf (cmd, "%s | %s %s", mMosaicAppData.mail_filter_command,
			mMosaicAppData.sendmail_command, to);
	} else {
		sprintf(cmd, "%s %s", mMosaicAppData.sendmail_command, to);
	}
	if ((_fp = popen (cmd, "w")) == NULL)
		return NULL;

	fprintf (_fp, "To: %s\n", to);
	fprintf (_fp, "Subject: %s\n", subj);
	fprintf (_fp, "Reply-To: %s <%s>\n",mMosaicAppData.author_full_name,mMosaicAppData.author_email); 
	fprintf (_fp, "Content-Type: %s\n", content_type);
	fprintf (_fp, "Mime-Version: 1.0\n");
	fprintf (_fp, "X-Mailer: mMosaic %s on %s\n", MO_VERSION_STRING, MO_MACHINE_TYPE);
	if (url)
		fprintf (_fp, "X-URL: %s\n", url);
	fprintf (_fp, "\n"); 
/* Stick in BASE tag as appropriate. */
	if (url && content_type && strcmp (content_type, "text/x-html") == 0)
		fprintf (_fp, "<base href=\"%s\">\n", url);
	return _fp;
}

mo_status mo_finish_sending_mail_message (void)
{
	if (_fp)
		pclose (_fp);
	_fp = NULL;
	return mo_succeed;                  
}

/* ------------------------------------------------------------------------ */

mo_status mo_send_mail_message (char *text, char *to, char *subj,
                                char *content_type, char *url)
{
	FILE *fp;

	fp = mo_start_sending_mail_message (to, subj, content_type, url);
	if (!fp)
		return mo_fail;

	if(!strcmp(content_type,"url_only")){
		fputs(url,fp);
		fputs("\n\n",fp);
	} else {
		fputs (text, fp);
	}
	mo_finish_sending_mail_message ();
	return mo_succeed;
}
 
/* ------------------------- upload stuff -------------------------- */
static char *mo_put_er_over(char *url, mo_window * win) 
{
	char *rv;                             
                                      
        do_put=do_post=1;             
        if (saveFileName!=NULL)     
                free(saveFileName);  
        saveFileName=strdup(url);     
        if (HTTP_last_modified) {    
                free(HTTP_last_modified);
                HTTP_last_modified=0;
        }                            
        rv=doit(url,win);        
        do_put=do_post=0;             
        return(rv);                   
}     

int upload(mo_window *win,FILE *fp, char *fname) 
{
	char *put_url,*xurl;                  
	int res=mo_fail;                     
	char *newtext=NULL;
	char *last_modified=NULL,*expires=NULL;
	char *ref;                           
                                     
        if (!win)                   
                return(0);           
        put_url=prompt_for_string("Enter the URL you wish to upload the file as:",win);                                  
        if (!put_url)
                return(0);           
/*                                    
        if (win->current_node &&      
            win->current_node->url &&
            *(win->current_node->url)) {
                ref=strdup(win->current_node->url);
        } else {                        
*/                                   
                ref=NULL;             
/*                                   
        }                             
*/                                    
        xurl=mo_url_prepend_protocol(put_url);
        free(put_url);               
        put_url=xurl;                
        fseek(fp,0,SEEK_END);         
        put_file_size=ftell(fp);     
        rewind(fp);                  
        put_fp=fp;                   
        if (win->target_anchor)     
                free(win->target_anchor);
/*                                    
        win->target_anchor=mo_url_extract_anchor(put_url);
*/                                   
        win->target_anchor=NULL;      
        newtext=mo_put_er_over(put_url,win);
                                     
        if (newtext) {                
                if ((!strncmp(newtext,"<H1>ERROR<H1>",10)) ||
                    (!strncmp(newtext,"<HEAD><TITLE>404 Not Found</TITLE></HEAD>",28))) {                            
                        res=mo_fail;
                }                    
        }                            
        if (HTTP_last_modified)
                last_modified=strdup(HTTP_last_modified);
        if (HTTP_expires)
                expires=strdup(HTTP_expires);
        if (use_this_url_instead) { 
                mo_here_we_are_son(put_url);
                free(put_url);
                put_url=use_this_url_instead;

                        /* Go get another target_anchor. */
                if (win->target_anchor)
                        free(win->target_anchor);
/* 
                win->target_anchor=mo_url_extract_anchor(put_url);
*/
                win->target_anchor=NULL;
        }                    
        if (newtext) {                
                res=mo_do_window_text(win,  put_url, newtext,
                                      1, ref,  last_modified, expires);
        }                            
        if (win->current_node)      
                mo_gui_check_security_icon_in_win(win->current_node->authType,win);
        if (last_modified) {          
                free(last_modified); 
                last_modified=NULL;  
        }                            
        if (expires) {                
                free(expires);       
                expires=NULL;        
        }                            
/*                                    
        if (xurl==put_url) {          
                if (put_url) {       
                        free(put_url);
                        put_url=NULL;
                }                    
        } else {                        
                if (xurl) {          
                        free(xurl);  
                        xurl=NULL;   
                }                    
                if (put_url) {       
                        free(put_url);
                        put_url=NULL;
                }           
        }                            
*/                                    
        if (ref) {                    
                free(ref);           
                ref=NULL;            
        }                            
        mo_gui_done_with_icon (win);     
        return(res);                  
}

mo_status mo_upload_window(mo_window *win, char *fname) 
{
	char *efname = (char *)malloc (sizeof (char) * (__MAX_HOME_LEN__ * 2));
	FILE *fp;                            
	int res;                              

        if (pathEval(efname, fname)<0) {
		fprintf(stderr,"Error in evaluating the path. (mo-www.c)\n");
        }                             
        if (!(fp=fopen(efname,"r"))) {
                char *buf, *final, tmpbuf[80];
                int final_len;       
                                     
                /* don't display dialog if command issued by cci application */
                if (cci_docommand) 
                        return mo_fail;
                buf=my_strerror(errno);
                if (!buf || !*buf || !strcmp(buf,"Error 0")) {
                        sprintf(tmpbuf,"Uknown Error" );
                        buf=tmpbuf;  
                }                    
                final_len=30+((!efname || !*efname?3:strlen(efname))+13)+
                        15+(strlen(buf)+3); 
                final=(char *)calloc(final_len,sizeof(char));
                sprintf(final,"\nUnable to upload document:\n   %s\n\nUpload Error:\n   %s\n" ,(!efname || !*efname?" ":efname),buf);
                application_error(final,"Upload Error");
                if (final) {          
                        free(final); 
                        final=NULL;  
                }                    
                free(efname);         
                return(mo_fail);      
        }                            
        res=upload(win,fp,efname);    
        fclose (fp);              
        free(efname);                
        return(mo_succeed);           
}                                    

static XmxCallback (upload_win_cb) 
{  
	char *fname = (char *)malloc (sizeof (char) * 128);
	mo_window *win = (mo_window*)client_data;

        XtUnmanageChild(win->upload_win);
        XmStringGetLtoR(((XmFileSelectionBoxCallbackStruct *)call_data)->value,
                        XmSTRING_DEFAULT_CHARSET,
                        &fname);     
        mo_upload_window(win,fname);  
        free(fname);                  
        return;                       
}                                    

mo_status mo_post_upload_window (mo_window *win) 
{
        if (!win->upload_win) {
                Widget frame, workarea;
 
                win->upload_win= XmxMakeFileSBDialog(win->base,
                                            "NCSA Mosaic: Upload Document",
                                            "Name of document to upload:",
                                            upload_win_cb, (XtPointer)win);
 
                /* This makes a frame as a work area for the dialog box. */
                XmxSetArg(XmNmarginWidth,5);
                XmxSetArg(XmNmarginHeight,5);
                frame=XmxMakeFrame(win->upload_win, XmxShadowEtchedIn);
                workarea=XmxMakeForm(frame);
        } else {
                XmFileSelectionDoSearch(win->upload_win, NULL);
        }
        XmxManageRemanage (win->upload_win);
        return mo_succeed;
}
