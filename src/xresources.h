/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* This document should be included in whatever source document
   sets up the Intrinsics.  It is in a separate file so it doesn't
   clutter up that file.  So sue me. */

#ifndef __MOSAIC_XRESOURCES_H__

/* ----------------------------- X Resources ------------------------------ */

#define offset(x) XtOffset (AppDataPtr, x)

static Boolean faux = False;
static int Izero = 0;
static int ttl192 = 192;

static XtResource resources[] = {

/* default font choice from Options menu choices */
  { "defaultFontChoice", "DefaultFontChoice", XtRString, sizeof (char *),
    offset (default_font_choice), XtRString, "TimesRegular" },
/* Default width for a Document View window.  This will change as windows
 *   are cloned. */
  { "defaultWidth", "DefaultWidth", XtRInt, sizeof (int),
      offset (default_width), XtRString, "640" },
/* Default height for a Document View window. */
  { "defaultHeight", "DefaultHeight", XtRInt, sizeof (int),
      offset (default_height), XtRString, "700" },
/* Startup document. */
  { "homeDocument", "HomeDocument", XtRString, sizeof (char *),
      offset (home_document), XtRString, HOME_PAGE_DEFAULT },
  { "confirmExit", "ConfirmExit", XtRBoolean, sizeof (Boolean),
      offset (confirm_exit), XtRString, "True" },
/* THIS USED TO BE mailCommand BUT IS NOW sendmailCommand. */
#ifdef __bsdi__
  { "sendmailCommand", "SendmailCommand", XtRString, sizeof (char *),
      offset (sendmail_command), XtRString, "/usr/sbin/sendmail -t" },
#else
  { "sendmailCommand", "SendmailCommand", XtRString, sizeof (char *),
      offset (sendmail_command), XtRString, "/usr/lib/sendmail -t" },
#endif
/* Ignore this.  Stealth feature. */
  { "mailFilterCommand", "MailFilterCommand", XtRString, sizeof (char *),
      offset (mail_filter_command), XtRString, NULL },
  { "printCommand", "PrintCommand", XtRString, sizeof (char *),
      offset (print_command), XtRString, "lpr" },
  { "loadLocalFile","LoadLocalFile",XtRInt,sizeof(int),
      offset (load_local_file), XtRString, "1"},
  { "editCommand", "EditCommand", XtRString, sizeof (char *),
      offset (edit_command), XtRString, NULL },
  { "editCommandUseXterm", "EditCommandUseXterm", XtRBoolean, sizeof (Boolean),
      offset (edit_command_use_xterm), XtRString, "True" },
#ifdef _AIX
  { "xtermCommand", "XtermCommand", XtRString, sizeof (char *),
      offset (xterm_command), XtRString, 
      "aixterm -v",
    },
#else /* not _AIX */
  { "xtermCommand", "XtermCommand", XtRString, sizeof (char *),
      offset (xterm_command), XtRString, 
      "xterm",
    },
#endif /* not _AIX */
  { "defaultAuthorFullName", "DefaultAuthorFullName",XtRString, sizeof (char *),
      offset (author_full_name), XtRString, NULL },
  { "defaultAuthorName", "DefaultAuthorName", XtRString, sizeof (char *),
      offset (author_name), XtRString, NULL },
  { "defaultAuthorEmail", "DefaultAuthorEmail", XtRString, sizeof (char *),
      offset (author_email), XtRString, NULL },
  { "signature", "Signature", XtRString, sizeof (char *),
      offset (signature), XtRString, NULL },
  { "colorsPerInlinedImage", "ColorsPerInlinedImage", XtRInt, sizeof (int),
      offset (colors_per_inlined_image), XtRString, "50" },
  { "trackVisitedAnchors", "TrackVisitedAnchors", XtRBoolean, sizeof (Boolean),
      offset (track_visited_anchors), XtRString, "True" },

  { "gunzipCommand", "GunzipCommand", XtRString, 
      sizeof (char *), offset (gunzip_command), XtRString, "gunzip -f -n" },
  { "initialWindowIconic", "InitialWindowIconic", XtRBoolean, sizeof (Boolean),
      offset (initial_window_iconic), XtRString, "False" },

  { "catchPriorAndNext", "CatchPriorAndNext", XtRBoolean, sizeof (Boolean),
      offset (catch_prior_and_next), XtRString, "True" },

  { "fullHostname", "FullHostname", XtRString, sizeof (char *),
      offset (full_hostname), XtRString, NULL },
  { "tweakGopherTypes", "TweakGopherTypes", XtRBoolean, sizeof (Boolean),
      offset (tweak_gopher_types), XtRString, "True" },

  /* --- new in 2.0 --- */
  { "trackPointerMotion", "TrackPointerMotion", XtRBoolean, sizeof (Boolean),
      offset (track_pointer_motion), XtRString, "True" },
  { "trackFullURLs", "TrackFullURLs", XtRBoolean, sizeof (Boolean),
      offset (track_full_url_names), XtRString, "True" },

  { "docsDirectory", "DocsDirectory", XtRString, sizeof (char *),
      offset (docs_directory), XtRString, NULL },

  { "reloadPragmaNoCache", "ReloadPragmaNoCache", XtRBoolean, sizeof (Boolean),
      offset (reload_pragma_no_cache), XtRString, "False" },

  { "maxWaisResponses", "MaxWaisResponses", XtRInt, sizeof (int),
      offset (max_wais_responses), XtRString, "200" },
  { "delayObjectLoads", "DelayObjectLoads", XtRBoolean, sizeof (Boolean),
      offset (delay_object_loads), XtRString, "False" },

  { "securityIcon", "securityIcon", 
      XtRBoolean, sizeof (Boolean),
      offset (securityIcon), XtRString, "True" },

  { "protectMeFromMyself", "ProtectMeFromMyself", 
      XtRBoolean, sizeof (Boolean),
      offset (protect_me_from_myself), XtRString, "False" },

  { "printMode", "PrintMode", XtRString, sizeof (char *),
      offset (print_mode), XtRString, "plain" },

  { "mailMode", "MailMode", XtRString, sizeof (char *),
      offset (mail_mode), XtRString, "plain" },

  { "saveMode", "SaveMode", XtRString, sizeof (char *),
      offset (save_mode), XtRString, "plain" },

  { "printBanners", "PrintBanners",
      XtRBoolean, sizeof (Boolean),
      offset (print_banners), XtRString, "True" },

  { "printFootnotes", "PrintFootnotes",
      XtRBoolean, sizeof (Boolean),
      offset (print_footnotes), XtRString, "True" },

  { "printPaperSizeUS", "PrintPaperSizeUS",
      XtRBoolean, sizeof (Boolean),
      offset (print_us), XtRString, "True" },

          /* new in 2.7 */
  { "installColormap", "InstallColormap", XtRBoolean, sizeof (Boolean),
      offset (install_colormap), XtRString, "False" },

  { "urlExpired", "UrlExpired", XtRInt, sizeof (int),
      offset (urlExpired), XtRString, "30" },

  { "wwwTrace", "WwwTrace", XtRBoolean, sizeof (Boolean),
      offset (wwwTrace), XtRString, "False" },

  { "htmlwTrace", "HtmlwTrace", XtRBoolean, sizeof (Boolean),
      offset (htmlwTrace), XtRString, "False" },

  { "srcTrace", "SrcTrace", XtRBoolean, sizeof (Boolean),
      offset (srcTrace), XtRString, "False" },

  { "meterForeground", "MeterForeground", XtRString, sizeof (char *),
      offset (meterForeground), XtRString, "#FFFF00000000"},

  { "meterBackground", "MeterBackground", XtRString, sizeof (char *),
      offset (meterBackground), XtRString, "#2F2F4F4F4F4F"},

  { "meterFontForeground", "MeterFontForeground", XtRString, sizeof (char *),
      offset (meterFontForeground), XtRString, "#FFFFFFFFFFFF"},

  { "meterFontBackground", "MeterFontBackground", XtRString, sizeof (char *),
      offset (meterFontBackground), XtRString, "#000000000000"},

/* Accept-Language stuff - BJS */
  { "acceptLanguage", "AcceptLanguage", XtRString, sizeof (char *),
      offset (acceptlanguage_str), XtRString, NULL },

  { "ftpTimeoutVal", "FtpTimeoutVal", XtRInt, sizeof (int),
      offset (ftp_timeout_val), XtRString, "90" },

  { "ftpRedial", "FtpRedial", XtRInt, sizeof (int),
      offset (ftpRedial), XtRString, "10" },

  { "screenGamma", "ScreenGamma", XtRFloat, sizeof (float),
      offset (screen_gamma), XtRString, "2.2" },
  { "useScreenGamma", "UseScreenGamma", XtRBoolean, sizeof(Boolean),
    offset(use_screen_gamma), XtRString, "False"},

  { "popupCascadeMappingDelay", "PopupCascadeMappingDelay", XtRInt, 
    sizeof(int), offset(popupCascadeMappingDelay), XtRString, "180" },

/* New news stuff in B4 */

  { "newsBackgroundFlushTime", "NewsBackgroundFlushTime", XtRInt,
    sizeof(int), offset(newsBackgroundFlushTime), XtRString, "300" },
#ifdef MULTICAST
    {"debug", "Debug", XtRBoolean, sizeof(Boolean),
    offset(mc_debug), XtRBoolean, (caddr_t) & faux},
    {"sessionName", "sessionName", XtRString, sizeof(char *),
    offset(mc_sess_name), XtRString, (caddr_t) "Mosaic Multicast Session"},
    {"mediaName", "mediaName", XtRString, sizeof(char *),
    offset(mc_media_name), XtRString, (caddr_t) "mMosaic"},
    {"lifeTime", "lifeTime", XtRInt, sizeof(int),
    offset(mc_life_time), XtRInt, (caddr_t) & Izero},
    {"ttl", "ttl", XtRInt, sizeof(int),
    offset(mc_ttl), XtRInt, (caddr_t) & ttl192},
    {"aliasName", "aliasName", XtRString, sizeof(char *),
    offset(mc_alias_name), XtRString, (caddr_t) NULL},
    {"multiCastAddr", "multiCastAddr", XtRString, sizeof(char *),
    offset(mc_dest), XtRString, (caddr_t) NULL},
#endif
  { "numberOfItemsInRBMHistory", "NumberOfItemsInRBMHistory", XtRInt,
    sizeof(int), offset(numberOfItemsInRBMHistory), XtRString, "12" },
};

#undef offset

static XrmOptionDescRec options[] = {
  {"-fn",     "*fontList",            XrmoptionSepArg, NULL},
  {"-ft",     "*XmText*fontList",     XrmoptionSepArg, NULL},
  {"-fm",     "*menubar*fontList",    XrmoptionSepArg, NULL},
  {"-home",   "*homeDocument",        XrmoptionSepArg, NULL},
  {"-ngh",    "*useGlobalHistory",    XrmoptionNoArg,  "False"},
  {"-iconic", "*initialWindowIconic", XrmoptionNoArg,  "True"},
  {"-i",      "*initialWindowIconic", XrmoptionNoArg,  "True"},
  /* New in 1.1 */
  /* -nd isn't documented since defaults in the widget still take effect,
     so the benefits of using it are kinda iffy (as if they weren't 
     anyway)... */
  {"-nd",     "*nothingUseful",       XrmoptionNoArg,  "True"},
  {"-tmpdir", "*tmpDirectory",        XrmoptionSepArg, NULL},
  {"-dil",    "*delayImageLoads",     XrmoptionNoArg,  "True"},
  {"-ics",    "*imageCacheSize",      XrmoptionSepArg, NULL},
  {"-protect","*protectMeFromMyself", XrmoptionNoArg,  "True"},
  {"-kraut",  "*mailFilterCommand",   XrmoptionNoArg,  "kraut"},
#ifdef __sgi
  {"-dm",     "*debuggingMalloc",     XrmoptionNoArg,  "True"},
#endif
  {"-installColormap",  "*installColormap",     XrmoptionNoArg,  "True"},
#ifdef MULTICAST
    {"-v", ".debug", XrmoptionNoArg, "True"},
    {"-verbose", ".debug", XrmoptionNoArg, "True"},
    {"-debug", ".debug", XrmoptionNoArg, "True"},
    {"-S", ".sessionName", XrmoptionSepArg, 0},
    {"-sessionName", ".sessionName", XrmoptionSepArg, 0},
    {"-M", ".mediaName", XrmoptionSepArg, 0},
    {"-mediaName", ".mediaName", XrmoptionSepArg, 0},
    {"-l", ".lifetime", XrmoptionSepArg, 0},
    {"-lifeTime", ".lifetime", XrmoptionSepArg, 0},
    {"-t", ".ttl", XrmoptionSepArg, 0},
    {"-ttl", ".ttl", XrmoptionSepArg, 0},
    {"-relay", ".relay", XrmoptionNoArg, "False"},
    {"-a", ".aliasName", XrmoptionSepArg, 0},
    {"-aliasName", ".aliasName", XrmoptionSepArg, 0},
    {"-mcDest", ".multiCastAddr", XrmoptionSepArg, 0},
#endif
};

static String color_resources[] = {
  "*XmLabel*fontList:   		-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*XmLabelGadget*fontList:	-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*XmScale*fontList:   		-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*XmBulletinBoard*labelFontList:	-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*optionmenu.XmLabelGadget*fontList:	-*-helvetica-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*XmPushButton*fontList:		-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmPushButtonGadget*fontList:	-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmToggleButton*fontList:	-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmToggleButtonGadget*fontList:	-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*optionmenu*fontList:		-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmIconGadget*fontList:		-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmBulletinBoard*buttonFontList: -*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*menubar*fontList:   		-*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmPushButton*fontList:  -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmLabelGadget*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmPushButtonGadget*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmCascadeButton*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmCascadeButtonGadget*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmCascadeButton*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmCascadeButtonGadget*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmToggleButton*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmMenuShell*XmToggleButtonGadget*fontList: -*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*pulldownmenu*fontList:	-*-helvetica-bold-o-normal-*-14-*-iso8859-1",
  "*XmList*fontList:	-*-helvetica-medium-r-normal-*-14-*-iso8859-1",
  "*XmText.fontList:      -*-lucidatypewriter-medium-r-normal-*-14-*-iso8859-1",
  "*XmTextField.fontList: -*-lucidatypewriter-medium-r-normal-*-14-*-iso8859-1",
  
  "*optionmenu*marginHeight: 	0",
  "*optionmenu*marginTop: 		5",
  "*optionmenu*marginBottom: 	5",
  "*optionmenu*marginWidth: 	5",
  "*pulldownmenu*XmPushButton*marginHeight:	1",
  "*pulldownmenu*XmPushButton*marginWidth:	1",
  "*pulldownmenu*XmPushButton*marginLeft:	3",
  "*pulldownmenu*XmPushButton*marginRight:	3",
  "*XmList*listMarginWidth:        3",
  "*menubar*marginHeight: 		1",
  "*menubar.marginHeight: 		0",
  "*menubar*marginLeft:  		1",
  "*menubar.spacing:  		7",
  "*XmMenuShell*marginLeft:  	3",
  "*XmMenuShell*marginRight:  	4",
  "*XmMenuShell*XmToggleButtonGadget*spacing: 	 2",
  "*XmMenuShell*XmToggleButtonGadget*marginHeight:  0",
  "*XmMenuShell*XmToggleButtonGadget*indicatorSize: 12",
  "*XmMenuShell*XmLabelGadget*marginHeight: 4",
  "*XmToggleButtonGadget*spacing: 	4",
  "*XmToggleButton*spacing: 	4",
  "*XmScrolledWindow*spacing: 	0",
  "*XmScrollBar*width: 		        18",
  "*XmScrollBar*height: 		18",
  "*Hbar*height:                        22",
  "*Vbar*width:                         22",
  "*XmScale*scaleHeight: 		20",
  "*XmText*marginHeight:		4",
  "*fsb*XmText*width:                   420",
  "*fsb*XmTextField*width:                   420",
  "*fillOnSelect:			True",
  "*visibleWhenOff:		        True",
  "*XmText*highlightThickness:		0",
  "*XmTextField*highlightThickness:	0",
  "*XmPushButton*highlightThickness:	0",
  "*XmScrollBar*highlightThickness:     0",
  "*highlightThickness:	                0",
  /* "*geometry:                           +400+200", */
  
  "*TitleFont: -adobe-times-bold-r-normal-*-24-*-*-*-*-*-iso8859-1",
  "*Font: -adobe-times-medium-r-normal-*-17-*-*-*-*-*-iso8859-1",
  "*ItalicFont: -adobe-times-medium-i-normal-*-17-*-*-*-*-*-iso8859-1",
  "*BoldFont: -adobe-times-bold-r-normal-*-17-*-*-*-*-*-iso8859-1",
  "*FixedFont: -adobe-courier-medium-r-normal-*-17-*-*-*-*-*-iso8859-1",
  "*Header1Font: -adobe-times-bold-r-normal-*-24-*-*-*-*-*-iso8859-1",
  "*Header2Font: -adobe-times-bold-r-normal-*-18-*-*-*-*-*-iso8859-1",
  "*Header3Font: -adobe-times-bold-r-normal-*-17-*-*-*-*-*-iso8859-1",
  "*Header4Font: -adobe-times-bold-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*Header5Font: -adobe-times-bold-r-normal-*-12-*-*-*-*-*-iso8859-1",
  "*Header6Font: -adobe-times-bold-r-normal-*-10-*-*-*-*-*-iso8859-1",
  "*AddressFont: -adobe-times-medium-i-normal-*-17-*-*-*-*-*-iso8859-1",
  "*PlainFont: -adobe-courier-medium-r-normal-*-14-*-*-*-*-*-iso8859-1",
  "*ListingFont: -adobe-courier-medium-r-normal-*-12-*-*-*-*-*-iso8859-1",
  "*SupSubFont: -adobe-times-medium-r-normal-*-10-*-*-*-*-*-iso8859-1",
  "*MeterFont: -adobe-courier-bold-r-normal-*-14-*-*-*-*-*-*-*",
  "*ToolbarFont: -adobe-times-bold-r-normal-*-12-*-*-*-*-*-iso8859-1",
  "*AnchorUnderlines:                   1",
  "*VisitedAnchorUnderlines:            1",
  "*DashedVisitedAnchorUnderlines:      True",
  "*VerticalScrollOnRight:              True",

#ifdef __sgi
  "*Foreground:			 	#000000000000",
  "*XmScrollBar*Foreground:             #afafafafafaf",
  "*Background:                         #afafafafafaf",
  "*XmList*Background:     		#afafafafafaf",
  "*XmText*Background:	 	        #afafafafafaf",
  "*TroughColor:                        #545454545454",
  "*XmSelectionBox*Background:	 	#afafafafafaf",
  "*XmMessageBox*Background:	 	#afafafafafaf",
  "*XmLabel*Foreground:                 #1d1d15155b5b",
  "*XmToggleButton*Foreground:          #1d1d15155b5b",
  "*XmPushButton*Foreground:            #5b5b00000000",
  "*logo*Foreground:                    #1d1d15155b5b",
  "*XmTextField*Background: 		#8c8c8c8c8c8c",
  "*SelectColor:			#ffffffff0000",
  "*HighlightColor:		 	#afafafafafaf",

  "*TopShadowColor:                     #dfdfdfdfdfdf",
  "*XmList*TopShadowColor:              #dfdfdfdfdfdf",
  "*XmText*TopShadowColor:              #dfdfdfdfdfdf",
  "*XmSelectionBox*TopShadowColor:      #dfdfdfdfdfdf",
  "*XmMessageBox*TopShadowColor:        #dfdfdfdfdfdf",
  
  "*visitedAnchorColor:                 #272705055b5b",
  "*anchorColor:                        #00000000b0b0",
  "*activeAnchorFG:                     #ffff00000000",
  "*activeAnchorBG:                     #afafafafafaf",
#else /* not sgi */
  "*Foreground:			 	#000000000000",
  "*XmScrollBar*Foreground:             #bfbfbfbfbfbf",
  "*XmLabel*Foreground:                 #1d1d15155b5b",
  "*XmToggleButton*Foreground:          #1d1d15155b5b",
  "*XmPushButton*Foreground:            #5b5b00000000",
  "*logo*Foreground:                    #1d1d15155b5b",

  "*Background:                         #bfbfbfbfbfbf",

  "*XmList*Background:     		#bfbfbfbfbfbf",
  "*XmText*Background:	 	        #bfbfbfbfbfbf",
  "*XmSelectionBox*Background:	 	#bfbfbfbfbfbf",
  "*XmMessageBox*Background:	 	#bfbfbfbfbfbf",
  "*XmTextField*Background: 		#9c9c9c9c9c9c",

  "*TopShadowColor:                     #e7e7e7e7e7e7",
  "*XmList*TopShadowColor:              #e7e7e7e7e7e7",
  "*XmText*TopShadowColor:              #e7e7e7e7e7e7",
  "*XmSelectionBox*TopShadowColor:      #e7e7e7e7e7e7",
  "*XmMessageBox*TopShadowColor:        #e7e7e7e7e7e7",
  
  "*TroughColor:                        #646464646464",
  "*SelectColor:			#ffffffff0000",
  "*HighlightColor:		 	#bfbfbfbfbfbf",

  /* Remember to update this in the app-defaults file. */
  "*visitedAnchorColor:                 #3f3f0f0f7b7b",
  "*anchorColor:                        #00000000b0b0",
  "*activeAnchorFG:                     #ffff00000000",
  "*activeAnchorBG:                     #bfbfbfbfbfbf",
#endif
  /* Disable Motif Drag-N-Drop - BJS */
  "*dragInitiatorProtocolStyle: XmDRAG_NONE",
  "*dragReceiverProtocolStyle:  XmDRAG_NONE",
 
  NULL,
};

#define __MOSAIC_XRESOURCES_H__
#endif /* __MOSAIC_XRESOURCES_H__ */











