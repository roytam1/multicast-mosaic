#ifndef NCSA_CCI_H
#define NCSA_CCI_H

#define MCCI_MAX_RETURN_TEXT	1024

#define MCCI_VERSION		"1.0"


/* return codes from cci api */
#define MCCI_OK			0
#define MCCI_FAIL		1
#define MCCI_OUTOFMEMORY	2
#define MCCI_REQUEST_FAIL	3
#define MCCI_NETWORK_ERROR	4

/* cci interface tokens used with api */
#define MCCI_DEFAULT		0
#define MCCI_OUTPUT_NONE	100
#define MCCI_OUTPUT_CURRENT	101
#define MCCI_OUTPUT_NEW		102
#define MCCI_ABSOLUTE		110
#define MCCI_RELATIVE		111

#define MCCI_SEND_BEFORE	210
#define MCCI_SEND_AFTER		220
#define MCCI_SEND_HANDLER       230     /* ADC ZZZ */


/* protocol token strings */
#define MCCI_S_GET		"GET"
#define MCCI_S_DISPLAY		"DISPLAY"
#define MCCI_S_DISCONNECT	"DISCONNECT"
#define MCCI_S_QUIT		"QUIT"
#define MCCI_S_SEND		"SEND"
#define MCCI_S_OUTPUT		"OUTPUT"
#define MCCI_S_ANCHOR		"ANCHOR" 
#define MCCI_S_HANDLER          "HANDLER"   /* ADC ZZZ */
#define MCCI_S_BEFORE		"BEFORE"
#define MCCI_S_AFTER		"AFTER"
#define MCCI_S_BROWSERVIEW	"BROWSERVIEW"
#define MCCI_S_FILE_TO_URL	"FILETOURL"
#define MCCI_S_FORM	 	"FORM"
#define MCCI_S_EVENT		"EVENT"
#define MCCI_S_DOCOMMAND	"DOCOMMAND"

#define MCCI_S_TO		"TO"
#define MCCI_S_STOP		"STOP"
#define MCCI_S_START		"START"
#define MCCI_S_CURRENT		"CURRENT"
#define MCCI_S_NEW		"NEW"
#define MCCI_S_NONE		"NONE"
#define MCCI_S_HEADER		"HEADER"
#define MCCI_S_POST		"POST"
#define MCCI_POST             	"cciPOST"

/* for DOCOMMAND options */
#define MCCI_ON			"ON"
#define MCCI_OFF		"OFF"
#define MCCI_UP			"UP"
#define MCCI_DOWN		"DOWN"
#define MCCI_CASE		"CASE"
#define MCCI_NOCASE		"NOCASE"

#define MCCI_UNDERLINE_DEFAULT	"UNDERLINE_DEFAULT"
#define MCCI_UNDERLINE_LIGHT	"UNDERLINE_LIGHT"
#define MCCI_UNDERLINE_MEDIUM	"UNDERLINE_MEDIUM"
#define MCCI_UNDERLINE_HEAVY	"UNDERLINE_HEAVY"
#define MCCI_UNDERLINE_NONE	"UNDERLINE_NONE"

#define MCCI_TIMES_REGULAR	"TIMES_REGULAR"
#define MCCI_TIMES_SMALL	"TIMES_SMALL"
#define MCCI_TIMES_LARGE	"TIMES_LARGE"
#define MCCI_HELVETICA_LARGE	"HELVETICA_LARGE"
#define MCCI_HELVETICA_SMALL	"HELVETICA_SMALL"
#define MCCI_HELVETICA_REGULAR	"HELVETICA_REGULAR"
#define MCCI_NEWCENTURY_LARGE	"NEWCENTURY_LARGE"
#define MCCI_NEWCENTURY_SMALL	"NEWCENTURY_SMALL"
#define MCCI_NEWCENTURY_REGULAR	"NEWCENTURY_REGULAR"
#define MCCI_LUCIDABRIGHT_LARGE	"LUCIDABRIGHT_LARGE"
#define MCCI_LUCIDABRIGHT_SMALL	"LUCIDABRIGHT_SMALL"
#define MCCI_LUCIDABRIGHT_REGULAR	"LUCIDABRIGHT_REGULAR"

#define MCCI_PLAINTEXT		"PLAINTEXT"
#define MCCI_FORMATTEDTEXT	"FORMATTEDTEXT"
#define MCCI_POSTSCRIPT		"POSTSCRIPT"
#define MCCI_HTML		"HTML" 

#define MCCI_BACK		"BACK"
#define MCCI_FORWARD		"FORWARD"
#define MCCI_HOME		"HOME"
#define MCCI_RELOAD		"RELOAD"
#define MCCI_SAVEAS		"SAVEAS"
#define MCCI_CLONE		"CLONE"
#define MCCI_NEWWINDOW		"NEWWINDOW"
#define MCCI_CLOSEWINDOW	"CLOSEWINDOW"
#define MCCI_OPENLOCAL		"OPENLOCAL"
#define MCCI_RELOADIMAGES	"RELOADIMAGES"
#define MCCI_REFRESHCURRENT	"REFRESHCURRENT"
#define MCCI_FINDINCURRENT	"FINDINCURRENT"
#define MCCI_VIEWSOURCE		"VIEWSOURCE"
#define MCCI_EDITSOURCE		"EDITSOURCE"
#define MCCI_PRINT		"PRINT"
#define MCCI_CLOSEWINDOW	"CLOSEWINDOW"
#define MCCI_EXITPROGRAM	"EXITPROGRAM"
#define MCCI_LOADTOLOCALDISK	"LOADTOLOCALDISK"
#define MCCI_DELAYIMAGELOAD	"DELAYIMAGELOAD"
#define MCCI_RELOADCONFIG	"RELOADCONFIG"
#define MCCI_FLUSHCACHE		"FLUSHCACHE"
#define MCCI_CLEARGLOBALHISTORY	"CLEARGLOBALHISTORY"
#define MCCI_WINDOWHISTORY	"WINDOWHISTORY"
#define MCCI_HOTLIST		"HOTLIST"
#define MCCI_FONT		"FONT"
#define MCCI_ANCHORUNDERLINE	"ANCHORUNDERLINE"


/******************* RETURN CODES ***********************/

/* successful return codes in protocol*/
#define MCCIR_OK			200
#define MCCIR_GET_OK			210
#define MCCIR_DISPLAY_OK		211
#define MCCIR_DISCONNECT_OK		212
#define MCCIR_QUIT_OK			213
#define MCCIR_SEND_OUTPUT_OK		214
#define MCCIR_SEND_O_STOP_OK		215
#define MCCIR_SEND_ANCHOR_OK		216
#define MCCIR_SEND_A_STOP_OK		217
#define MCCIR_POST_OK			218
#define MCCIR_BROWSERVIEW_OK		219
#define MCCIR_BROWSERVIEW_STOP_OK	220
#define MCCIR_FILE_TO_URL		223
#define MCCIR_FORM_OK			224
#define MCCIR_SEND_EVENT_OK		225
#define MCCIR_SEND_EVENT_STOP_OK	226
#define MCCIR_DOCOMMAND_OK 		227

/* Send Anchor Before return codes */
#define MCCIR_SEND_ANCH_BEF_LINK_OK	280  /* clicked link  */
#define MCCIR_SEND_ANCH_BEF_OPEN_OK	281  /* used open dialog */
#define MCCIR_SEND_ANCH_BEF_EDIT_OK	282  /* edited URL field  */
#define MCCIR_SEND_ANCH_BEF_FORW_OK	283  /* clicked forward */
#define MCCIR_SEND_ANCH_BEF_BACK_OK	284  /* clicked back */
#define MCCIR_SEND_ANCH_BEF_RELO_OK	285  /* clicked reload */
#define MCCIR_SEND_ANCH_BEF_HIST_OK	286  /* from history  */
#define MCCIR_SEND_ANCH_BEF_HOTL_OK	287  /* from hotlist */
#define MCCIR_SEND_ANCH_BEF_MCCI_OK	288  /* from CCI */
#define MCCIR_SEND_ANCH_BEF_OTHR_OK	289  /* from other source */      

/* sendAnchor After return codes */
#define MCCIR_SEND_ANCH_AFT_LINK_OK	290  /* clicked link  */          
#define MCCIR_SEND_ANCH_AFT_OPEN_OK	291  /* used open dialog */       
#define MCCIR_SEND_ANCH_AFT_EDIT_OK	292  /* edited URL field  */      
#define MCCIR_SEND_ANCH_AFT_FORW_OK	293  /* clicked forward */        
#define MCCIR_SEND_ANCH_AFT_BACK_OK	294  /* clicked back */           
#define MCCIR_SEND_ANCH_AFT_RELO_OK	295  /* clicked reload */         
#define MCCIR_SEND_ANCH_AFT_HIST_OK	296  /* from history  */          
#define MCCIR_SEND_ANCH_AFT_HOTL_OK	297  /* from hotlist */           
#define MCCIR_SEND_ANCH_AFT_MCCI_OK	298  /* from CCI */               
#define MCCIR_SEND_ANCH_AFT_OTHR_OK	299  /* from other source */      

/* ADC ZZZ */
#define MCCIR_SEND_ANCH_HAN_OTHR_OK     270  /*  ADC ZZZ */

/* additional data follows repsonse code */
#define MCCIR_ANCHOR_INFO	301 /* anchor visited information */
#define MCCIR_SEND_DATA_OUTPUT	302 /* output from Send Output protocol */
#define MCCIR_SEND_BROWSERVIEW	303 /* output from Send Browserview proto */
#define MCCIR_POST_OUTPUT	304 /* output from post */
#define MCCIR_FORM_RESPONSE	308 /* form submission reply */
#define MCCIR_SEND_EVENT	309 /* output form send event protocol */
#define MCCIR_SEND_MOUSE_ANCHOR 310 /* output from Send Mouse Anchor */

/* problem response codes... client problems*/
#define MCCIR_UNRECOGNIZED	401  /* what's this? */
#define MCCIR_ERROR		402  /* does not follow protocol */

/* problem response codes... broswer problems*/
#define MCCIR_REQ_FAILED	500  /* request failed */
#define MCCIR_GET_FAILED	501  /* request failed */
#define MCCIR_MAX_CONNECTIONS   502  /* Max number of connections exceeded */
#define MCCIR_NO_URL_FOR_FILE	503  /* couldn't translate filename to url */
#define MCCIR_DOCOMMAND_FAILED	504  /* command not implemented yet */

/* all possible events on the Web browser */
typedef enum{

/* selected from menu */
	MOSAIC_NEW, MOSAIC_CLONE, MOSAIC_OPEN_URL, FILE_OPEN_LOCAL, 
  	MOSAIC_RELOAD_CURRENT,
/*5*/	FILE_RELOAD_IMAGES, FILE_REFRESH_CURRENT, FILE_FIND_IN_CURRENT,
/*8*/	FILE_VIEW_SOURCE, FILE_EDIT_SOURCE, FILE_SOURCE_DATE, MOSAIC_SAVE_AS,
/*12*/	FILE_PRINT, FILE_MAIL_TO, FILE_CCI, MOSAIC_CLOSE, FILE_EXIT_PROGRAM,

/*17*/
	OPTIONS_LOAD_TO_LOCAL_DISK_ON, OPTIONS_LOAD_TO_LOCAL_DISK_OFF,
	OPTIONS_DELAY_IMAGE_LOADING_ON, OPTIONS_DELAY_IMAGE_LOADING_OFF,
	OPTIONS_LOAD_IMAGES_IN_CURRENT, OPTIONS_RELOAD_CONFIG_FILES, 
	OPTIONS_FLUSH_CACHE, OPTIONS_CLEAR_GLOBAL_HISTORY, 
/*25*/
	OPTIONS_FONTS_TR, OPTIONS_FONTS_TS, OPTIONS_FONTS_TL,
	OPTIONS_FONTS_HR, OPTIONS_FONTS_HS, OPTIONS_FONTS_HL, 
	OPTIONS_FONTS_NCR, OPTIONS_FONTS_NCS, OPTIONS_FONTS_NCL,
	OPTIONS_FONTS_LBR, OPTIONS_FONTS_LBS, OPTIONS_FONTS_LBL,
/*37*/
	OPTIONS_ANCHOR_UNDERLINES_DU, OPTIONS_ANCHOR_UNDERLINES_LU,
	OPTIONS_ANCHOR_UNDERLINES_MU, OPTIONS_ANCHOR_UNDERLINES_HU,
	OPTIONS_ANCHOR_UNDERLINES_NU,
/*42*/
  	MOSAIC_BACK, MOSAIC_FORWARD, MOSAIC_HOME_DOCUMENT, 
	NAVIGATE_WINDOW_HISTORY, NAVIGATE_HOTLIST, 
	NAVIGATE_ADD_CURRENT_TO_HOTLIST, NAVIGATE_INTERNET_STARTING_POINTS, 
	NAVIGATE_INTERNET_RESOURCES_META_INDEX,
/*50*/
  	NEWS_NEXT, NEWS_PREV, NEWS_NEXT_THREAD, NEWS_PREV_THREAD, NEWS_INDEX,
	NEWS_LIST_GROUPS, NEWS_POST, NEWS_FOLLOW_UP, NEWS_FORMAT_TV,
	NEWS_FORMAT_GV,
/*60*/
/* when user click on globe */
  	MOSAIC_GLOBE,

/* when user make selections from the bottom panel */
  	AUTHENTICATION_BUTTON,

/* when user submit a form, has to be valid */
  	FORM_SUBMIT,

/* when user edits the url text field in mosaic window and hit return */
  	MOSAIC_URL_TEXT_FIELD,

/*65*/
/* triggering anchor in HTML widget */
  	MOSAIC_URL_TRIGGER,

/* SWP -- 8/3/95 */
  	HELP_DEMO, HELP_MANUAL, HELP_ABOUT, HELP_ON_WINDOW, HELP_WHATS_NEW,
  	HELP_ON_VERSION, HELP_ON_FAQ, HELP_ON_HTML, HELP_ON_URLS,
  	HELP_MAIL_TECH_SUPPORT, FILE_OPEN_DTM_OUTPORT, FILE_BROADCAST_DOCUMENT,
  	FILE_KERBEROS_V4_LOGIN, FILE_KERBEROS_V5_LOGIN, HELP_COMMENT_CARD,
  	IMAGE_LOADED, LINK_LOADED, STARTUP,

/*83*/
/* Events for FTP transfer/commands. */
  	FTP_PUT, FTP_REMOVE, FTP_MKDIR ,

  	OPTIONS_FLUSH_PASSWD_CACHE
}CCI_events;


/*****************************************************************/

#include "port.h"
typedef PortDescriptor *MCCIPort;

void MoCCIStartListening( Widget w, int port);
void MoCCISendAnchorToCCI( char *url, int beforeAfter);
void MoCCISendBrowserViewOutput(char *url, char *contentType, char *data, int dataLength);
void MoCCISendBrowserView( MCCIPort client, int on);
void MoCCIAddAnchorToURL(char *url, char *urlAndAnchor);
int MoCCISendBrowserViewFile( char *url, char *contentType, char *filename);
void MoCCISendEvent( MCCIPort client, int on);

void MoCCITerminateAConnection(MCCIPort client);
int MoCCICurrentNumberOfConnections();

void MCCIRequestDoCommand( int *retCode, char *retText,
        char *command, char *parameter);
void MCCIRequestGetURL( int *retCode, char *retText, char *url,
        int output, char *additionalHeader);
void MCCIRequestForm( MCCIPort client, int *retCode,
        char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
        char *actionID, int status);
void MCCIRequestSendAnchor( int *retCode,
        char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
        MCCIPort client,        
        int status);
void MCCIRequestSendOutput( int *retCode,
        char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
        MCCIPort client,
        int on,         /* boolean value....turn on - true, off - false */
        char *type);
void MCCIRequestSendEvent( int *retCode,
        char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
        MCCIPort client,
        int on);
void MCCIRequestSendBrowserView( int *retCode,
        char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
        MCCIPort client,
        int on);
void MCCIRequestPost( MCCIPort client, int *retCode, char *retText,
        char *url, char *contentType, char *postData, int dataLength,
        int output);
void MCCIRequestDisplay(MCCIPort client, int *retCode, char *retText, char *url,
        char *contentType, char *displayData, int dataLength, int output);
void MCCIRequestQuit();
void MCCIRequestGetAnnotation( int *retCode, char *retText, char **retData,
        int *retDataLength, char *url, int type);
void MCCIRequestPutAnnotation( int *retCode, char *retText, int type,
        char *url, char *annotation, int annotationLength);
void MCCIRequestFileToURL( int *retCode, char *retText, char *fileName);
void cciStatPreventSendAnchor(MCCIPort client, char *url);

void MoCCISendAnchor( MCCIPort client, int sendIt);
void MoCCISendOutputToClient(char *contentType, char *fileName);
void MoCCISendOutput(MCCIPort client, Boolean sendIt, char *contentType);
void MoCCIForm(MCCIPort client, char *actionID, int stat, int close_connection);
#endif
