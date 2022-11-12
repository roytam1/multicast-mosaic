/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <ctype.h>

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include "support.h"
#include "accept.h"
#include "cciServer.h"
#include "cciBindings2.h"
#include "mo-www.h"
#include "history.h"
#include "gui-documents.h"
#include "cache.h"
#include "globalhist.h"

/* for setting some selections buttons*/

int cci_get = 0;
int cci_event = 0;
int cci_docommand = 0;


/***************************************************************************/
/* This module contains bindings between the new cci code and the browser  */
/* If you are retrofitting the cci on to your browser strip out everything */
/* except the function headers and the return values and stick in the      */
/* appropriate calls for your browser.                                     */
/***************************************************************************/

void MCCIRequestDoCommand( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	char *command,
	char *parameter)
{
	mo_window *win;
	char *s, *end, *tmp_end;

/* default assume to work */
	*retCode = MCCIR_DOCOMMAND_OK;
	strcpy(retText, "Executed the command");
	if (!strcmp(command, MCCI_EXITPROGRAM)){
		mo_exit ();
		return;
	}
	if (!strcmp(command, MCCI_RELOADCONFIG)){
		mo_re_init_formats ();
		return;
	}
	if (!parameter){
		*retCode = MCCIR_DOCOMMAND_FAILED;
		strcpy(retText, "Parameters Required");
		return;
	}
	s = parameter;
/* got the window id here */
	if (!mo_main_next_window(NULL)) {
		*retCode = MCCIR_DOCOMMAND_FAILED;
		strcpy(retText, "Current Window Not Available");
		return;
	}
	win=mo_main_next_window(NULL);
	if (!strcmp(command, MCCI_BACK))
		mo_back_node(win);
	else if (!strcmp(command, MCCI_FORWARD))
		mo_forward_node(win);	
	else if (!strcmp(command, MCCI_HOME))
		mo_load_window_text (win, mMosaicAppData.home_document,NULL);
	else if (!strcmp(command, MCCI_RELOAD))
		mo_reload_window_text (win);
	else if (!strcmp(command, MCCI_CLONE))
		mo_duplicate_window (win);
	else if (!strcmp(command, MCCI_CLOSEWINDOW))
		mo_delete_window (win);
	else if (!strcmp(command, MCCI_RELOADIMAGES))
		mo_reload_window_text (win);
	else if (!strcmp(command, MCCI_REFRESHCURRENT))
		mo_refresh_window_text (win);
	else if (!strcmp(command, MCCI_VIEWSOURCE))
		mo_post_source_window (win);
	else if (!strcmp(command, MCCI_EDITSOURCE))
		mo_edit_source(win);
	else if (!strcmp(command, MCCI_NEWWINDOW))
		mo_open_another_window (win, mMosaicAppData.home_document, NULL, NULL);
	else if (!strcmp(command, MCCI_FLUSHCACHE))
		MMCacheClearCache ();
	else if (!strcmp(command, MCCI_CLEARGLOBALHISTORY)){
		mo_window *w = NULL;

		mo_wipe_global_history (win);
		while (w = mo_main_next_window (w))
			mo_redisplay_window (w);
	} else if (!strcmp(command, MCCI_SAVEAS)){/*need to get format filename */
		char *format, *filename;
		mo_status status;

					/* s is pointed pass the window id part */
		GetWordFromString(s,&format,&end); /* Get command */
		if ((!format) || (format == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a format");
			return;
		}
		s = end;
		format = strdup(format);
		tmp_end = strchr(format, ' ');
		if (tmp_end) *tmp_end = '\0';

		GetWordFromString(s,&filename,&end); /* Get command */
		if ((!filename) || (filename == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a filename");
			if (format) free(format);
			return;
		}
		filename = strdup(filename);
		sscanf(filename,"%s",filename); /* to get rid of /r/n */
		cci_docommand = 1;
		if (!strcmp(format, MCCI_PLAINTEXT))
			status = mo_save_window(win, filename, 0);
		else if (!strcmp(format, MCCI_FORMATTEDTEXT))
			status = mo_save_window(win, filename, 1);
		else if (!strcmp(format, MCCI_HTML)) 
			status = mo_save_window(win, filename, 2);
		else if (!strcmp(format, MCCI_POSTSCRIPT))
			status = mo_save_window(win, filename, 4);
		else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Invalid Format");
			if (filename) free(filename);
			if (format) free(format);
			cci_docommand = 0;
			return;
		}
		cci_docommand = 0;

		if (filename) free(filename);
		if (format) free(format);
		if (status == mo_fail){
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Unable to save file");
			return;
		}
	} else if (!strcmp(command, MCCI_FINDINCURRENT)){
/* need to get search_string and CASE|NOCASE */
		char *s_string, *c;   
		mo_status found;

/* s is pointed pass the window id part */
		GetWordFromString(s,&s_string,&end); /* Get command */
		if ((!s_string) || (s_string == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a search string");
			return;
		}
		s = end;
		s_string = strdup(s_string);
		tmp_end = strchr(s_string, ' ');
		if (tmp_end) *tmp_end = '\0';

		GetWordFromString(s,&c,&end); /* Get command */
		if ((!c) || (c == end) ){ 
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify CASE");
			if (s_string) free(s_string);
			return;
		}
		c = strdup(c);
		sscanf(c,"%s",c); /* to get rid of /r/n */

		cci_docommand = 1;
		if (!strcmp(c, MCCI_NOCASE))
			found = mo_search_window(win, s_string, 0, 1,0);
		else if (!strcmp(c, MCCI_CASE))
			found = mo_search_window(win, s_string, 0, 0,0);
  		else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Invalid CASE Specification");
			cci_docommand = 0;
			if(s_string) free(s_string);
			if (c) free(c);
			return;
		}
		cci_docommand = 0;

		/**** should I return a error when not found ****/
		if(s_string) free(s_string);
		if (c) free(c);

	} else if (!strcmp(command, MCCI_PRINT)){
/* need to get format and printCommand */
		char *format, *printCommand;
		mo_status status;

	/* s is pointed pass the window id part */
		GetWordFromString(s,&format,&end); /* Get command */
		if ((!format) || (format == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a print format");
			return;
		}
		s = end;
		format = strdup( format);
		tmp_end = strchr(format, ' ');
		if (tmp_end) *tmp_end = '\0';

		if (s == NULL){
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a print command");
			if (format) free(format);
			return;
		}

		printCommand = s;
		while(isalnum( (int) (*s)) || (*s == ' ') || (*s == '-'))
			s++;
		if (s) *s = '\0';
		printCommand = strdup(printCommand);

		cci_docommand = 1;
		if (!strcmp(format, MCCI_PLAINTEXT))
			status = mo_print_window(win, 0, printCommand);
		else if (!strcmp(format, MCCI_FORMATTEDTEXT))
			status = mo_print_window(win, 1, printCommand);
		else if (!strcmp(format, MCCI_HTML)) 
			status = mo_print_window(win, 2, printCommand);
		else if (!strcmp(format, MCCI_POSTSCRIPT))
			status = mo_print_window(win, 4, printCommand);
		else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Invalid Format");
	   		cci_docommand = 0;
			if (format) free(format);
			return;
		}
		cci_docommand = 0;
		if (format) free(format);
		if (status == mo_fail){
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Unable to print file");
			return;
		}
	} else if (!strcmp(command, MCCI_LOADTOLOCALDISK)){
/* need to get ON|OFF */
	char *on_off;

/* s is pointed pass the window id part */
		GetWordFromString(s,&on_off,&end); /* Get command */
		if ((!on_off) || (on_off == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify ON|OFF");
			return;
		}
		on_off = strdup(on_off);
		sscanf(on_off,"%s", on_off);

		if (!strcmp(on_off, MCCI_ON)){
			win->binary_transfer = 1;
			XmxRSetToggleState (win->menubar,
				(XtPointer) mo_binary_transfer,
				(win->binary_transfer ? XmxSet : XmxNotSet));
		} else if (!strcmp(on_off, MCCI_OFF)){
			win->binary_transfer = 0;
			XmxRSetToggleState (win->menubar,
				(XtPointer) mo_binary_transfer,
				(win->binary_transfer ? XmxSet : XmxNotSet));
		} else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify ON|OFF");
			if (on_off) free(on_off);
			return;
		}
		if (on_off) free(on_off);
	} else if (!strcmp(command, MCCI_DELAYIMAGELOAD)){
/* need to get ON|OFF */
	char *on_off;

		/* s is pointed pass the window id part */
		GetWordFromString(s,&on_off,&end); /* Get command */
		if ((!on_off) || (on_off == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify ON|OFF");
			return;
		}
		on_off = strdup(on_off);
		sscanf(on_off,"%s", on_off);

		if (!strcmp(on_off, MCCI_ON)){
			win->delay_image_loads = 1;
			XmxRSetToggleState(win->menubar,
				(XtPointer) mo_delay_image_loads,
				(win->delay_image_loads?XmxSet : XmxNotSet));
			XmxRSetSensitive (win->menubar, 
				(XtPointer)mo_expand_images_current, 
				win->delay_image_loads?XmxSensitive:XmxNotSensitive);
		} else if (!strcmp(on_off, MCCI_OFF)){
			win->delay_image_loads = 0;
			XmxRSetToggleState(win->menubar,
				(XtPointer) mo_delay_image_loads,
				(win->delay_image_loads?XmxSet : XmxNotSet));
			XmxRSetSensitive (win->menubar, 
				(XtPointer)mo_expand_images_current, 
				win->delay_image_loads?XmxSensitive:XmxNotSensitive);
		} else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify ON|OFF");
			if (on_off) free(on_off);
			return;
		}
		if (on_off) free(on_off);
	} else if (!strcmp(command, MCCI_WINDOWHISTORY)){
	} else if (!strcmp(command, MCCI_HOTLIST)){
	} else if (!strcmp(command, MCCI_FONT)){
/* has to get fontname */
		char *fontname;

		/* s is pointed pass the window id part */
		GetWordFromString(s,&fontname,&end); /* Get command */
		if ((!fontname) || (fontname == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify fontname");
			return;
		}
		fontname = strdup(fontname);
		sscanf(fontname,"%s", fontname);

		if (!strcmp(fontname, MCCI_TIMES_REGULAR))
			mo_set_fonts(win, mo_regular_fonts_tkn);
		else if(!strcmp(fontname, MCCI_TIMES_SMALL))
			mo_set_fonts(win, mo_small_fonts_tkn);
		else if(!strcmp(fontname, MCCI_TIMES_LARGE))
			mo_set_fonts(win, mo_large_fonts_tkn);
		else if(!strcmp(fontname, MCCI_HELVETICA_LARGE))
			mo_set_fonts(win, mo_large_helvetica_tkn);
		else if(!strcmp(fontname, MCCI_HELVETICA_SMALL))
			mo_set_fonts(win, mo_small_helvetica_tkn);
		else if(!strcmp(fontname, MCCI_HELVETICA_REGULAR))
			mo_set_fonts(win, mo_regular_helvetica_tkn);
		else if(!strcmp(fontname, MCCI_NEWCENTURY_LARGE))
			mo_set_fonts(win, mo_large_newcentury_tkn);
		else if(!strcmp(fontname, MCCI_NEWCENTURY_SMALL))
			mo_set_fonts(win, mo_small_newcentury_tkn);
		else if(!strcmp(fontname, MCCI_NEWCENTURY_REGULAR))
			mo_set_fonts(win, mo_regular_newcentury_tkn);
		else if(!strcmp(fontname, MCCI_LUCIDABRIGHT_LARGE))
			mo_set_fonts(win, mo_large_lucidabright_tkn);
		else if(!strcmp(fontname, MCCI_LUCIDABRIGHT_REGULAR))
			mo_set_fonts(win, mo_regular_lucidabright_tkn);
		else if(!strcmp(fontname, MCCI_LUCIDABRIGHT_SMALL))
			mo_set_fonts(win, mo_small_lucidabright_tkn);
		else{
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Invalid Fontname Specification");
			if (fontname) free(fontname);
			return;
		}
		if (fontname) free(fontname);
	} else if (!strcmp(command, MCCI_ANCHORUNDERLINE)){
/* need to get level */
		char *level;

		/* s is pointed pass the window id part */
		GetWordFromString(s,&level,&end); /* Get command */
		if ((!level) || (level == end)) {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "You Need to Specify a level");
			return;
		}
		level = strdup(level);
		sscanf(level,"%s", level);

		if (!strcmp(level, MCCI_UNDERLINE_DEFAULT))
			mo_set_underlines (win,mo_default_underlines_tkn);
		else if (!strcmp(level, MCCI_UNDERLINE_LIGHT))
		 	mo_set_underlines (win,mo_l1_underlines_tkn);
		else if (!strcmp(level, MCCI_UNDERLINE_MEDIUM))
		 	mo_set_underlines (win,mo_l2_underlines_tkn);
		else if (!strcmp(level, MCCI_UNDERLINE_HEAVY))
		 	mo_set_underlines (win,mo_l3_underlines_tkn);
		else if (!strcmp(level, MCCI_UNDERLINE_NONE))
		 	mo_set_underlines (win,mo_no_underlines_tkn);
		else {
			*retCode = MCCIR_DOCOMMAND_FAILED;
			strcpy(retText, "Invalid Level Specification");
			if (level) free(level);
			return;
		}
		if (level) free(level);
	} else {/* command not recongized */
  		*retCode = MCCIR_DOCOMMAND_FAILED;
		strcpy(retText, "Invalid Command");
	}
}

void MCCIRequestGetURL( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	char *url,
	int output,
	char *additionalHeader) 	/* currently additional header ignored */
{
	mo_status moStatus;

	if (mMosaicCCITrace) {
		if (additionalHeader)
			fprintf(stderr,"MCCIRequestGetURL(url=\"%s\",output=%d,header=\"%s\")\n",
				url,output,additionalHeader);
		else
			fprintf(stderr,"MCCIRequestGetURL(url=\"%s\",output=%d)\n",
				url,output);
	}

        if (! mMosaicAppData.load_local_file)
		if (!strncasecmp(url,"file:",5)) {
			*retCode = MCCIR_GET_FAILED;
			strcpy(retText,
				"Can't get local file(for CCI security reasons)");
			return;
		}

/*do it */
	if (!strchr (url, ':')) {
		url = mo_url_canonicalize_local (url);
	}

	switch (output) {
	case MCCI_OUTPUT_CURRENT:
		/* turn flag on so mosaic will know to do a get*/
		cci_get = 1; 	
		moStatus = mo_load_window_text (mo_main_next_window(NULL), url, NULL);
		if (moStatus == mo_succeed) {
			*retCode = MCCIR_GET_OK;
			strcpy(retText,"Got the URL");
		} else {
			*retCode = MCCIR_GET_FAILED;
			sprintf(retText,"Couldn't get URL %s",url);
		}
		cci_get = 0;  	/* done with get, turn flag off */
		break;
	case MCCI_OUTPUT_NEW:
		/* turn flag on so mosaic will know to do a get*/
		cci_get = 1; 	
		if (!mo_open_another_window(mo_main_next_window(NULL),url,NULL,NULL)) {
			*retCode = MCCIR_GET_FAILED;
			sprintf(retText,"Couldn't get URL %s",url);
		} else {
			*retCode = MCCIR_GET_OK;
			strcpy(retText,"Got the URL");
		}
		cci_get = 0;  	/* done with get, turn flag off */
		break;
	case MCCI_OUTPUT_NONE:
		*retCode = MCCIR_GET_FAILED;
		strcpy(retText,"Sorry, OUTPUT to no where not supported yet");
		break;
	default:
		*retCode = MCCIR_GET_FAILED;
		strcpy(retText,"Send output where???");
		break;
	}
}

void MCCIRequestForm( MCCIPort client, int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	char *actionID, int status)
{
	MoCCIForm(client, actionID, status, 0);
	*retCode = MCCIR_FORM_OK;
	strcpy(retText, "Form Submit Received");
	return;
}

void MCCIRequestSendAnchor( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	MCCIPort client,
	int status) /* 0, MCCI_SEND_BEFORE, or MCCI_SEND_AFTER */
            /* or MCCI_SEND_HANDLER  ADC ZZZ */
/* anchor replies may be sent back using MCCISendAnchorHistory(client,url)*/
{

	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestSendAnchor(%d)\n",status);
	}

	switch (status) {
	case MCCI_SEND_BEFORE:
		MoCCISendAnchor(client,1);
		*retCode = MCCIR_SEND_ANCH_BEF_OTHR_OK;
		strcpy(retText,"Send Anchor Before enabled");
		break;
	case MCCI_SEND_AFTER:
	        MoCCISendAnchor(client,2);
		*retCode = MCCIR_SEND_ANCH_AFT_OTHR_OK;
		strcpy(retText,"Send Anchor After enabled");
		break;
	case MCCI_SEND_HANDLER:           /* ADC ZZZ */
                MoCCISendAnchor(client,3);
                *retCode = MCCIR_SEND_ANCH_HAN_OTHR_OK;
                strcpy(retText,"Send Anchor Handler enabled");
                break;  
	case 0:
		MoCCISendAnchor(client,0);
		*retCode = MCCIR_SEND_A_STOP_OK;
		strcpy(retText,"Send Anchor disabled");
		break;
	}
}

void MCCIRequestSendOutput( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	MCCIPort client,
	int on,		/* boolean value....turn on - true, off - false */
	char *type)	/* if null, assume all types */
{

	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestSendOutput(%d,%s)\n",on,type);
	}

	if (on) {
		MoCCISendOutput(client,1,type);
		*retCode = MCCIR_SEND_OUTPUT_OK;
		strcpy(retText,"Send OUTPUT enabled");
	} else {
		MoCCISendOutput(client,0,type);
		*retCode = MCCIR_SEND_O_STOP_OK;
		strcpy(retText,"Send OUTPUT disabled");
	}
}


void MCCIRequestSendEvent( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	MCCIPort client,
	int on)		/* boolean value....turn on - true, off - false */
{

	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestEvent(%d)\n",on);
	}

	if (on) {
		cci_event = 1;
		MoCCISendEvent(client,1);
		*retCode = MCCIR_SEND_EVENT_OK;
		strcpy(retText,"Send EVENT enabled");
	} else {
		/* set cci_event to 0 only if there are no client request */
		MoCCISendEvent(client,0);
		*retCode = MCCIR_SEND_EVENT_STOP_OK;
		strcpy(retText,"Send EVENT disabled");
	}
}

void MCCIRequestSendBrowserView( int *retCode,
	char *retText, /* must be less MCCI_MAX_RETURN_TEXT*/
	MCCIPort client,
	int on)		/* boolean value....turn on - true, off - false */
{

	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestSendBrowserView(%d)\n",on);
	}

	if (on) {
		MoCCISendBrowserView(client,1);
		*retCode = MCCIR_BROWSERVIEW_OK;
		strcpy(retText,"Send BROWSERVIEW enabled");
	} else {
		MoCCISendBrowserView(client,0);
		*retCode = MCCIR_BROWSERVIEW_STOP_OK;
		strcpy(retText,"Send BROWSERVIEW disabled");
	}
}

void MCCIRequestPost( MCCIPort client, int *retCode, char *retText,
	char *url, char *contentType, char *postData, int dataLength,
	int output)
{
	char *response;
	char buff[256];
	int length;

	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestPost(): about to \n");
		fprintf(stderr,"mo_post_pull_er_over(url=\"%s\",type=\"%s\",postData=\"%s\")\n"
				,url,contentType,postData);
	}

	*retCode = MCCIR_POST_OK;
	strcpy(retText,"Post Request ok");

	switch(output) {
	case MCCI_OUTPUT_NONE:
		/* do not display output of post, but send the
		   output back through the cci to the client */
		response = mo_post_pull_er_over(url, 
				contentType, postData,mo_main_next_window(NULL));

		/* send response back through cci */
		if (response && (length = strlen(response))) {
			MCCISendResponseLine(client,MCCIR_POST_OUTPUT,
							"POST output");
			sprintf(buff,"Content-Length: %d\r\n",length);
			if (length!=NetServerWrite(client, buff,strlen(buff))){
					/* this is pointless... I know */
				strcpy(retText,"couldn't send output");
				*retCode = MCCI_FAIL;
			}
			if (length!=NetServerWrite(client, response,length)) {
					/* this is pointless... I know */
				strcpy(retText,"couldn't send output");
				*retCode = MCCI_FAIL;
				}
		}
		break;
	case MCCI_OUTPUT_NEW:
			/* open a new window and display posting... */
			/* ...not done yet...*/

	case MCCI_DEFAULT: /* default to output current */
	case MCCI_OUTPUT_CURRENT:
	default:
		/* display in current window */
		response = mo_post_pull_er_over(url, 
				contentType, postData,mo_main_next_window(NULL));
		/*mo_decode_internal_reference(url,response,url);*/
/*#######*/
		mo_do_window_text(mo_main_next_window(NULL),url, response,1,url,0,0);
		break;
	}

	if (mMosaicCCITrace) {
		fprintf(stderr,"result from mo_post_pull_er_over():\"%s\"\n",response);
		fprintf(stderr,"MCCIRequestPost(): returning now\n");
	}
}

void MCCIRequestDisplay( MCCIPort client, int *retCode, char *retText, char *url,
	char *contentType, char *displayData, int dataLength, int output)
{
	char *ref;
	char *new_url;

#ifndef DISABLE_TRACE
	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestDisplay(): about to \n");
		fprintf(stderr,"mo_post_pull_er_over(url=\"%s\",type=\"%s\",	\
						     displayData=\"%s\")\n" ,url,contentType,displayData);
	}
#endif

	*retCode = MCCIR_DISPLAY_OK;
	strcpy(retText,"Display Request ok");

	switch(output) {
	case MCCI_OUTPUT_NONE:
		strcpy(retText, "OUTPUT NONE not support yet\n");
		break;
	case MCCI_OUTPUT_NEW:
			/* open a new window and display... */
			/* ...not done yet...*/
		strcpy(retText, "OUTPUT NEW not support yet\n");

	case MCCI_DEFAULT: /* default to output current */
	case MCCI_OUTPUT_CURRENT:
	default:
		/* display in current window */
		ref = strdup(url);
		new_url = strdup(url);
		if (strcmp(contentType, "text/html") == 0)
			mo_do_window_text(mo_main_next_window(NULL), new_url, 
				displayData, 1, ref, 
				mo_main_next_window(NULL)->current_node->last_modified, 
				mo_main_next_window(NULL)->current_node->expires);
		else
			strcpy(retText, "Display text/html only");	
		break;
	}

#ifndef DISABLE_TRACE
	if (mMosaicCCITrace) {
		fprintf(stderr,"MCCIRequestDisplay(): returning now\n");
	}
#endif
}

void MCCIRequestQuit() /* time to die */
{
	mo_exit();
}

void MCCIRequestFileToURL( int *retCode, char *retText, char *fileName)
{
	char *url;

	url = MoReturnURLFromFileName(fileName);
	if (url) {
		strcpy(retText,url);
		*retCode = MCCIR_FILE_TO_URL;
	} else {
		strcpy(retText,"No URL for given file name");
		*retCode = MCCIR_NO_URL_FOR_FILE; 
	}
}
