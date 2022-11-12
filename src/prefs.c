/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Created: Wed Sep 20 11:05:19 CDT 1995
 * Modified: All the time.
 * Author: Dan Pape
 */

#include "../libhtmlw/HTML.h"
#include "mosaic.h"

#include <pwd.h>
#include <sys/utsname.h>

/* Static Global Variables */

static prefsStructP thePrefsStructP;

/*            Preference initialization and closing functions */

/****************************************************************************
   Function: preferences_genesis(void)
   Desc:     Initializes the preferences manager.
 ***************************************************************************/
Boolean preferences_genesis(void)
{
    Boolean successful = 1;

        /* initialize preferences structure */
    thePrefsStructP = (prefsStructP) malloc (sizeof(prefsStruct));
    if(thePrefsStructP == NULL) {
        fprintf(stderr, "Error: no memory for preferences structure\n");
        return 0;
    }

    thePrefsStructP->RdataP = (AppDataPtr) malloc (sizeof(AppData));
    if(thePrefsStructP->RdataP == NULL) {
        free(thePrefsStructP);
        fprintf(stderr, "Error: no memory for appdata structure\n");
        return 0;
    }
    return successful;
}

/****************************************************************************
   Function: preferences_armegeddon(void)
   Desc:     Kills the preferences manager.
 ***************************************************************************/
Boolean preferences_armegeddon(void)
{
    Boolean successful = 1;

    free(thePrefsStructP);	 /* free preferences structure */
    return(successful);
}

/****************************************************************************
 *                   Preference Structure access functions
 ***************************************************************************/


/****************************************************************************
   Function: get_ptr_to_preferences(void)
   Desc:     Returns a pointer to the main preferences structure
 ***************************************************************************/
prefsStructP get_ptr_to_preferences(void) 
{
    return thePrefsStructP;
}


/****************************************************************************
   Function: get_pref(long pref_id)
   Desc:     Returns a pointer to the single preference variable
                 denoted by pref_id
 ***************************************************************************/
void *get_pref(long pref_id) 
{
    switch(pref_id) {
        case  eTRACK_VISITED_ANCHORS:
            return (void *)&(thePrefsStructP->RdataP->track_visited_anchors);
        case  eDISPLAY_URLS_NOT_TITLES:
            return (void *)&(thePrefsStructP->RdataP->display_urls_not_titles);
        case  eTRACK_POINTER_MOTION:
            return (void *)&(thePrefsStructP->RdataP->track_pointer_motion);
        case  eTRACK_FULL_URL_NAMES:
            return (void *)&(thePrefsStructP->RdataP->track_full_url_names);
        case  eANNOTATIONS_ON_TOP:
            return (void *)&(thePrefsStructP->RdataP->annotations_on_top);
        case  eCONFIRM_DELETE_ANNOTATION:
            return (void *)&(thePrefsStructP->RdataP->confirm_delete_annotation);
        case  eANNOTATION_SERVER:
            return (void *)(thePrefsStructP->RdataP->annotation_server);
        case  eRECORD_COMMAND_LOCATION:
            return (void *)(thePrefsStructP->RdataP->record_command_location);
        case  eRECORD_COMMAND:
            return (void *)(thePrefsStructP->RdataP->record_command);
        case  eRELOAD_PRAGMA_NO_CACHE:
            return (void *)&(thePrefsStructP->RdataP->reload_pragma_no_cache);
        case  eSENDMAIL_COMMAND:
            return (void *)(thePrefsStructP->RdataP->sendmail_command);
        case  eEDIT_COMMAND:
            return (void *)(thePrefsStructP->RdataP->edit_command);
        case  eXTERM_COMMAND:
            return (void *)(thePrefsStructP->RdataP->xterm_command);
        case  eMAIL_FILTER_COMMAND:
            return (void *)(thePrefsStructP->RdataP->mail_filter_command);
        case  ePRIVATE_ANNOTATION_DIRECTORY:
            return (void *)(thePrefsStructP->RdataP->private_annotation_directory);
        case  eHOME_DOCUMENT:
            return (void *)(thePrefsStructP->RdataP->home_document);
        case  eTMP_DIRECTORY:
            return (void *)(thePrefsStructP->RdataP->tmp_directory);
        case  eDOCS_DIRECTORY:
            return (void *)(thePrefsStructP->RdataP->docs_directory);
        case  eDEFAULT_FONT_CHOICE:
            return (void *)(thePrefsStructP->RdataP->default_font_choice);
        case  eGLOBAL_HISTORY_FILE:
            return (void *)(thePrefsStructP->RdataP->global_history_file);
        case  eHISTORY_FILE:
            return (void *)(thePrefsStructP->RdataP->history_file);
        case  eUSE_GLOBAL_HISTORY:
            return (void *)&(thePrefsStructP->RdataP->use_global_history);
        case  eDEFAULT_HOTLIST_FILE:
            return (void *)(thePrefsStructP->RdataP->default_hotlist_file);
        case  eDEFAULT_HOT_FILE:
            return (void *)(thePrefsStructP->RdataP->default_hot_file);
        case  eADD_HOTLIST_ADDS_RBM:
            return (void *)&(thePrefsStructP->RdataP->addHotlistAddsRBM);
        case  eADD_RBM_ADDS_RBM:
            return (void *)&(thePrefsStructP->RdataP->addRBMAddsRBM);
        case  eCOLORS_PER_INLINED_IMAGE:
            return (void *)&(thePrefsStructP->RdataP->colors_per_inlined_image);
        case  eIMAGE_CACHE_SIZE:
            return (void *)&(thePrefsStructP->RdataP->image_cache_size);
        case  eRELOAD_RELOADS_IMAGES:
            return (void *)&(thePrefsStructP->RdataP->reload_reloads_images);
        case  eREVERSE_INLINED_BITMAP_COLORS:
            return (void *)&(thePrefsStructP->RdataP->reverse_inlined_bitmap_colors);
        case  eDELAY_IMAGE_LOADS:
            return (void *)&(thePrefsStructP->RdataP->delay_image_loads);
        case  eDEFAULT_AUTHOR_NAME:
            return (void *)(thePrefsStructP->RdataP->default_author_name);
        case  eDEFAULT_AUTHOR_EMAIL:
            return (void *)(thePrefsStructP->RdataP->default_author_email);
        case  eSIGNATURE:
            return (void *)(thePrefsStructP->RdataP->signature);
        case  eMAIL_MODE:
            return (void *)(thePrefsStructP->RdataP->mail_mode);
        case  ePRINT_COMMAND:
            return (void *)(thePrefsStructP->RdataP->print_command);
        case  eUNCOMPRESS_COMMAND:
            return (void *)(thePrefsStructP->RdataP->uncompress_command);
        case  eGUNZIP_COMMAND:
            return (void *)(thePrefsStructP->RdataP->gunzip_command);
        case  eUSE_DEFAULT_EXTENSION_MAP:
            return (void *)&(thePrefsStructP->RdataP->use_default_extension_map);
        case  eUSE_DEFAULT_TYPE_MAP:
            return (void *)&(thePrefsStructP->RdataP->use_default_type_map);
        case  eGLOBAL_EXTENSION_MAP:
            return (void *)(thePrefsStructP->RdataP->global_extension_map);
        case  ePERSONAL_EXTENSION_MAP:
            return (void *)(thePrefsStructP->RdataP->personal_extension_map);
        case  eGLOBAL_TYPE_MAP:
            return (void *)(thePrefsStructP->RdataP->global_type_map);
        case  ePERSONAL_TYPE_MAP:
            return (void *)(thePrefsStructP->RdataP->personal_type_map);
        case  eTWEAK_GOPHER_TYPES:
            return (void *)&(thePrefsStructP->RdataP->tweak_gopher_types);
        case  ePRINT_MODE:
            return (void *)(thePrefsStructP->RdataP->print_mode);
        case  ePRINT_BANNERS:
            return (void *)&(thePrefsStructP->RdataP->print_banners);
        case  ePRINT_FOOTNOTES:
            return (void *)&(thePrefsStructP->RdataP->print_footnotes);
        case  ePRINT_PAPER_SIZE_US:
            return (void *)&(thePrefsStructP->RdataP->print_us);
        case  ePROXY_SPECFILE:
            return (void *)(thePrefsStructP->RdataP->proxy_specfile);
        case  eNOPROXY_SPECFILE:
            return (void *)(thePrefsStructP->RdataP->noproxy_specfile);
        case  eCCIPORT:
            return (void *)&(thePrefsStructP->RdataP->cciPort);
        case  eMAX_NUM_OF_CCI_CONNECTIONS:
            return (void *)&(thePrefsStructP->RdataP->max_num_of_cci_connections);
        case  eMAX_WAIS_RESPONSES:
            return (void *)&(thePrefsStructP->RdataP->max_wais_responses);
        case  eKEEPALIVE:
            return (void *)&(thePrefsStructP->RdataP->keepAlive);
        case  eFTP_TIMEOUT_VAL:
            return (void *)&(thePrefsStructP->RdataP->ftp_timeout_val);
        case  eDEFAULT_WIDTH:
            return (void *)&(thePrefsStructP->RdataP->default_width);
        case  eDEFAULT_HEIGHT:
            return (void *)&(thePrefsStructP->RdataP->default_height);
        case  eAUTO_PLACE_WINDOWS:
            return (void *)&(thePrefsStructP->RdataP->auto_place_windows);
        case  eINITIAL_WINDOW_ICONIC:
            return (void *)&(thePrefsStructP->RdataP->initial_window_iconic);
        case  eTITLEISWINDOWTITLE:
            return (void *)&(thePrefsStructP->RdataP->titleIsWindowTitle);
        case  eUSEICONBAR:
            return (void *)&(thePrefsStructP->RdataP->useIconBar);
        case  eUSETEXTBUTTONBAR:
            return (void *)&(thePrefsStructP->RdataP->useTextButtonBar);
        case  eTWIRLING_TRANSFER_ICON:
            return (void *)&(thePrefsStructP->RdataP->twirling_transfer_icon);
        case  eSECURITYICON:
            return (void *)&(thePrefsStructP->RdataP->securityIcon);
        case  eTWIRL_INCREMENT:
            return (void *)&(thePrefsStructP->RdataP->twirl_increment);
        case  eSAVE_MODE:
            return (void *)(thePrefsStructP->RdataP->save_mode);
        case  eHDF_MAX_IMAGE_DIMENSION:
            return (void *)&(thePrefsStructP->RdataP->hdf_max_image_dimension);
        case  eHDF_MAX_DISPLAYED_DATASETS:
            return (void *)&(thePrefsStructP->RdataP->hdf_max_displayed_datasets);
        case  eHDF_MAX_DISPLAYED_ATTRIBUTES:
            return (void *)&(thePrefsStructP->RdataP->hdf_max_displayed_attributes);
        case  eHDF_POWER_USER:
            return (void *)&(thePrefsStructP->RdataP->hdf_power_user);
        case  eHDFLONGNAME:
            return (void *)&(thePrefsStructP->RdataP->hdflongname);
        case  eFULL_HOSTNAME:
            return (void *)(thePrefsStructP->RdataP->full_hostname);
        case  eLOAD_LOCAL_FILE:
            return (void *)&(thePrefsStructP->RdataP->load_local_file);
        case  eEDIT_COMMAND_USE_XTERM:
            return (void *)&(thePrefsStructP->RdataP->edit_command_use_xterm);
        case  eCONFIRM_EXIT:
            return (void *)&(thePrefsStructP->RdataP->confirm_exit);
        case  eDEFAULT_FANCY_SELECTIONS:
            return (void *)&(thePrefsStructP->RdataP->default_fancy_selections);
        case  eCATCH_PRIOR_AND_NEXT:
            return (void *)&(thePrefsStructP->RdataP->catch_prior_and_next);
        case  eSIMPLE_INTERFACE:
            return (void *)&(thePrefsStructP->RdataP->simple_interface);
        case  ePROTECT_ME_FROM_MYSELF:
            return (void *)&(thePrefsStructP->RdataP->protect_me_from_myself);
        case  eGETHOSTBYNAME_IS_EVIL:
            return (void *)&(thePrefsStructP->RdataP->gethostbyname_is_evil);
#ifdef __sgi
        case  eDEBUGGING_MALLOC:
            return (void *)&(thePrefsStructP->RdataP->debugging_malloc);
#endif
        case  eUSEAFSKLOG:
            return (void *)&(thePrefsStructP->RdataP->useAFSKlog);

/* New in 2.7 */

        case eSEND_REFERER:
            return (void *)&(thePrefsStructP->RdataP->sendReferer);
        case eSEND_AGENT:
            return (void *)&(thePrefsStructP->RdataP->sendAgent);
        case eEXPAND_URLS:
            return (void *)&(thePrefsStructP->RdataP->expandUrls);
        case eEXPAND_URLS_WITH_NAME:
            return (void *)&(thePrefsStructP->RdataP->expandUrlsWithName);
        case eDEFAULT_PROTOCOL:
            return (void *)(thePrefsStructP->RdataP->defaultProtocol);
        case eMETER_FOREGROUND:
            return (void *)(thePrefsStructP->RdataP->meterForeground);
        case eMETER_BACKGROUND:
            return (void *)(thePrefsStructP->RdataP->meterBackground);
        case eMETER_FONT_FOREGROUND:
            return (void *)(thePrefsStructP->RdataP->meterFontForeground);
        case eMETER_FONT_BACKGROUND:
            return (void *)(thePrefsStructP->RdataP->meterFontBackground);
        case eMETER:
            return (void *)&(thePrefsStructP->RdataP->use_meter);
        case eBACKUP_FILES:
            return (void *)&(thePrefsStructP->RdataP->backup_files);
        case ePIX_BASENAME:
            return (void *)(thePrefsStructP->RdataP->pix_basename);
        case ePIX_COUNT:
            return (void *)&(thePrefsStructP->RdataP->pix_count);
        case eACCEPT_LANGUAGE_STR:
            return (void *)(thePrefsStructP->RdataP->acceptlanguage_str);
        case eFTP_REDIAL:
            return (void *)&(thePrefsStructP->RdataP->ftpRedial);
        case eFTP_REDIAL_SLEEP:
            return (void *)&(thePrefsStructP->RdataP->ftpRedialSleep);
        case eFTP_FILENAME_LENGTH:
            return (void *)&(thePrefsStructP->RdataP->ftpFilenameLength);
        case eFTP_ELLIPSIS_LENGTH:
            return (void *)&(thePrefsStructP->RdataP->ftpEllipsisLength);
        case eFTP_ELLIPSIS_MODE:
            return (void *)&(thePrefsStructP->RdataP->ftpEllipsisMode);
        case eTITLE_ISWINDOW_TITLE:
            return (void *)&(thePrefsStructP->RdataP->titleIsWindowTitle);
        case eUSE_SCREEN_GAMMA:
            return (void *)&(thePrefsStructP->RdataP->useScreenGamma);
        case eSCREEN_GAMMA:
            return (void *)&(thePrefsStructP->RdataP->screen_gamma);
        case eHTTPTRACE:
            return (void *)&(thePrefsStructP->RdataP->httpTrace);
        case eWWW2TRACE:
            return (void *)&(thePrefsStructP->RdataP->www2Trace);
        case eHTMLWTRACE:
            return (void *)&(thePrefsStructP->RdataP->htmlwTrace);
        case eCCITRACE:
            return (void *)&(thePrefsStructP->RdataP->cciTrace);
        case eSRCTRACE:
            return (void *)&(thePrefsStructP->RdataP->srcTrace);
        case eCACHETRACE:
            return (void *)&(thePrefsStructP->RdataP->cacheTrace);
        case eNUTTRACE:
            return (void *)&(thePrefsStructP->RdataP->nutTrace);
        case eANIMATEBUSYICON:
            return (void *)&(thePrefsStructP->RdataP->animateBusyIcon);
        case eINSTALL_COLORMAP:
            return (void *)&(thePrefsStructP->RdataP->instamap);
        case eIMAGEVIEWINTERNAL:
            return (void *)&(thePrefsStructP->RdataP->imageViewInternal);
        case eURLEXPIRED:
            return (void *)&(thePrefsStructP->RdataP->urlExpired);
        case ePOPUPCASCADEMAPPINGDELAY:
            return (void *)&(thePrefsStructP->RdataP->popupCascadeMappingDelay);
        case eFRAME_HACK:
            return (void *)&(thePrefsStructP->RdataP->frame_hack);
        case eUSETHREADVIEW:
	    return (void *)&(thePrefsStructP->RdataP->newsConfigView);
        case eSHOWREADGROUPS:
	  return (void *)&(thePrefsStructP->RdataP->newsShowReadGroups);
        case eNOTHREADJUMPING:
	  return (void *)&(thePrefsStructP->RdataP->newsNoThreadJumping);
        case eSHOWALLGROUPS:
	  return (void *)&(thePrefsStructP->RdataP->newsShowAllGroups);
        case eSHOWALLARTICLES:
	  return (void *)&(thePrefsStructP->RdataP->newsShowAllArticles);
        case eUSEBACKGROUNDFLUSH:
	  return (void *)&(thePrefsStructP->RdataP->newsUseBackgroundFlush);
        case eBACKGROUNDFLUSHTIME:
	  return (void *)&(thePrefsStructP->RdataP->newsBackgroundFlushTime);
        case eCLIPPING:
          return (void *)&(thePrefsStructP->RdataP->clipping);
        case eMAX_CLIPPING_SIZE_X:
          return (void *)&(thePrefsStructP->RdataP->max_clip_x);
        case eMAX_CLIPPING_SIZE_Y:    
          return (void *)&(thePrefsStructP->RdataP->max_clip_y);
        case eUSE_LONG_TEXT_NAMES:
	  return (void *)&(thePrefsStructP->RdataP->long_text_names);
        case eTOOLBAR_LAYOUT:
            return (void *)(thePrefsStructP->RdataP->toolbar_layout);
        case eNEXTISUNREAD:
          return (void *)&(thePrefsStructP->RdataP->newsNextIsUnread);
        case ePREVISUNREAD:
          return (void *)&(thePrefsStructP->RdataP->newsPrevIsUnread);
        case eUSENEWSRC:
          return (void *)&(thePrefsStructP->RdataP->newsUseNewsrc);
        case eNEWSRCPREFIX:
          return (void *)(thePrefsStructP->RdataP->newsNewsrcPrefix);
        case eNEWSSUBJECTWIDTH:
          return (void *)&(thePrefsStructP->RdataP->newsSubjectWidth);
        case eNEWSAUTHORWIDTH:
          return (void *)&(thePrefsStructP->RdataP->newsAuthorWidth);
        case eFOCUS_FOLLOWS_MOUSE:
            return (void *)&(thePrefsStructP->RdataP->focusFollowsMouse);
        case eSESSION_HISTORY_ON_RBM:
            return (void *)&(thePrefsStructP->RdataP->sessionHistoryOnRBM);
        case eNUMBER_OF_ITEMS_IN_RBM_HISTORY:
            return (void *)&(thePrefsStructP->RdataP->numberOfItemsInRBMHistory);
        case eHOTLIST_ON_RBM:         
            return (void *)&(thePrefsStructP->RdataP->hotlistOnRBM);
        case eUSESHORTNEWSRC:         
          return (void *)&(thePrefsStructP->RdataP->newsUseShortNewsrc);
    }
}

/****************************************************************************
   Function: get_pref_string(long pref_id)
   Desc:     Returns a pointer to the single preference variable
                 denoted by pref_id
 ***************************************************************************/
char *get_pref_string(long pref_id)
{
    char *tmp_string = (char *)get_pref(pref_id);

    if(tmp_string == NULL)
        return (char *)NULL;
    else if(strcmp(tmp_string, "") == 0)
        return (char *)NULL;
    else
        return (char *)tmp_string;
}

/****************************************************************************
   Function: get_pref_int(long pref_id)
   Desc:     Returns a pointer to the single preference variable
                 denoted by pref_id
 ***************************************************************************/
int get_pref_int(long pref_id) 
{
    return *(int *)get_pref(pref_id);
}

/****************************************************************************
   Function: get_pref_boolean(long pref_id)
   Desc:     Returns a pointer to the single preference variable
                 denoted by pref_id
 ***************************************************************************/
Boolean get_pref_boolean(long pref_id) 
{
    return *(Boolean *)get_pref(pref_id);
}

/****************************************************************************
   Function: get_pref_float(long pref_id)
   Desc:     Returns a pointer to the single preference variable
                 denoted by pref_id
 ***************************************************************************/
float get_pref_float(long pref_id) 
{
    return *(float *)get_pref(pref_id);
}

/****************************************************************************
   Function: set_pref_boolean(long pref_id, int value)
   Desc:     Convenience for boolean setting.
 ***************************************************************************/
void set_pref_boolean(long pref_id, int value) 
{
	int val=value;

	set_pref(pref_id,&val);
}
            
/****************************************************************************
   Function: set_pref_int(long pref_id, int value)
   Desc:     Convenience for integer setting.
 ***************************************************************************/
void set_pref_int(long pref_id, int value) 
{
	int val=value;  
            
        set_pref(pref_id,&val); 
}     


/****************************************************************************
   Function: set_pref(long pref_id, void *incoming)
   Desc:     set the single preference variable denoted by pref_id, to 
                 whatever incoming points to.
 ***************************************************************************/
void set_pref(long pref_id, void *incoming) 
{
    switch(pref_id) {

        case  eTRACK_VISITED_ANCHORS:
            thePrefsStructP->RdataP->track_visited_anchors =
                *((Boolean *)incoming);
            break;
        case  eDISPLAY_URLS_NOT_TITLES:
            thePrefsStructP->RdataP->display_urls_not_titles =
                *((Boolean *)incoming);
            break;
        case  eTRACK_POINTER_MOTION:
            thePrefsStructP->RdataP->track_pointer_motion =
                *((Boolean *)incoming);
            break;
        case  eTRACK_FULL_URL_NAMES:
            thePrefsStructP->RdataP->track_full_url_names =
                *((Boolean *)incoming);
            break;
        case  eANNOTATIONS_ON_TOP:
            thePrefsStructP->RdataP->annotations_on_top = *((Boolean *)incoming);
            break;
        case  eCONFIRM_DELETE_ANNOTATION:
            thePrefsStructP->RdataP->confirm_delete_annotation =
                *((Boolean *)incoming);
            break;
        case  eANNOTATION_SERVER:
            thePrefsStructP->RdataP->annotation_server = (char *)incoming;
            break;
        case  eRECORD_COMMAND_LOCATION:
            thePrefsStructP->RdataP->record_command_location =
                (char *)incoming;
            break;
        case  eRECORD_COMMAND:
            thePrefsStructP->RdataP->record_command = (char *)incoming;
            break;
        case  eRELOAD_PRAGMA_NO_CACHE:
            thePrefsStructP->RdataP->reload_pragma_no_cache =
                *((Boolean *)incoming);
            break;
        case  eSENDMAIL_COMMAND:
            thePrefsStructP->RdataP->sendmail_command = (char *)incoming;
            break;
        case  eEDIT_COMMAND:
            thePrefsStructP->RdataP->edit_command = (char *)incoming;
            break;
        case  eXTERM_COMMAND:
            thePrefsStructP->RdataP->xterm_command = (char *)incoming;
            break;
        case  eMAIL_FILTER_COMMAND:
            thePrefsStructP->RdataP->mail_filter_command = (char *)incoming;
            break;
        case  ePRIVATE_ANNOTATION_DIRECTORY:
            thePrefsStructP->RdataP->private_annotation_directory =
                (char *)incoming;
            break;
        case  eHOME_DOCUMENT:
            thePrefsStructP->RdataP->home_document = (char *)incoming;
            break;
        case  eTMP_DIRECTORY:
            thePrefsStructP->RdataP->tmp_directory = (char *)incoming;
            break;
        case  eDOCS_DIRECTORY:
            thePrefsStructP->RdataP->docs_directory = (char *)incoming;
            break;
        case  eDEFAULT_FONT_CHOICE:
            thePrefsStructP->RdataP->default_font_choice = (char *)incoming;
            break;
        case  eGLOBAL_HISTORY_FILE:
            thePrefsStructP->RdataP->global_history_file = (char *)incoming;
            break;
        case  eHISTORY_FILE:
            thePrefsStructP->RdataP->history_file = (char *)incoming;
            break;
        case  eUSE_GLOBAL_HISTORY:
            thePrefsStructP->RdataP->use_global_history = *((Boolean *)incoming);
            break;
        case  eDEFAULT_HOTLIST_FILE:
            thePrefsStructP->RdataP->default_hotlist_file =
                (char *)incoming;
            break;
        case  eDEFAULT_HOT_FILE:
            thePrefsStructP->RdataP->default_hot_file =
                (char *)incoming;
            break;
        case  eADD_HOTLIST_ADDS_RBM:
            thePrefsStructP->RdataP->addHotlistAddsRBM =
                *((Boolean *)incoming);
            break;
        case  eADD_RBM_ADDS_RBM:
            thePrefsStructP->RdataP->addRBMAddsRBM =
                *((Boolean *)incoming);
            break;
        case  eCOLORS_PER_INLINED_IMAGE:
            thePrefsStructP->RdataP->colors_per_inlined_image =
                *((int *)incoming);
            break;
        case  eIMAGE_CACHE_SIZE:
            thePrefsStructP->RdataP->image_cache_size = *((int *)incoming);
            break;
        case  eRELOAD_RELOADS_IMAGES:
            thePrefsStructP->RdataP->reload_reloads_images =
                *((Boolean *)incoming);
            break;
        case  eREVERSE_INLINED_BITMAP_COLORS:
            thePrefsStructP->RdataP->reverse_inlined_bitmap_colors =
                *((Boolean *)incoming);
            break;
        case  eDELAY_IMAGE_LOADS:
            thePrefsStructP->RdataP->delay_image_loads = *((Boolean *)incoming);
            break;
        case  eDEFAULT_AUTHOR_NAME:
            thePrefsStructP->RdataP->default_author_name = (char *)incoming;
            break;
        case  eDEFAULT_AUTHOR_EMAIL:
            thePrefsStructP->RdataP->default_author_email = (char *)incoming;
            break;
        case  eSIGNATURE:
            thePrefsStructP->RdataP->signature = (char *)incoming;
            break;
        case  eMAIL_MODE:
            thePrefsStructP->RdataP->mail_mode = (char *)incoming;
            break;
        case  ePRINT_COMMAND:
            thePrefsStructP->RdataP->print_command = (char *)incoming;
            break;
        case  eUNCOMPRESS_COMMAND:
            thePrefsStructP->RdataP->uncompress_command = (char *)incoming;
            break;
        case  eGUNZIP_COMMAND:
            thePrefsStructP->RdataP->gunzip_command = (char *)incoming;
            break;
        case  eUSE_DEFAULT_EXTENSION_MAP:
            thePrefsStructP->RdataP->use_default_extension_map =
                *((Boolean *)incoming);
            break;
        case  eUSE_DEFAULT_TYPE_MAP:
            thePrefsStructP->RdataP->use_default_type_map =
                *((Boolean *)incoming);
            break;
        case  eGLOBAL_EXTENSION_MAP:
            thePrefsStructP->RdataP->global_extension_map =
                (char *)incoming;
            break;
        case  ePERSONAL_EXTENSION_MAP:
            thePrefsStructP->RdataP->personal_extension_map =
                (char *)incoming;
            break;
        case  eGLOBAL_TYPE_MAP:
            thePrefsStructP->RdataP->global_type_map = (char *)incoming;
            break;
        case  ePERSONAL_TYPE_MAP:
            thePrefsStructP->RdataP->personal_type_map = (char *)incoming;
            break;
        case  eTWEAK_GOPHER_TYPES:
            thePrefsStructP->RdataP->tweak_gopher_types =
                *((Boolean *)incoming);
            break;
        case  ePRINT_MODE:
            thePrefsStructP->RdataP->print_mode = (char *)incoming;
            break;
        case  ePRINT_BANNERS:
            thePrefsStructP->RdataP->print_banners = *((Boolean *)incoming);
            break;
        case  ePRINT_FOOTNOTES:
            thePrefsStructP->RdataP->print_footnotes = *((Boolean *)incoming);
            break;
        case  ePRINT_PAPER_SIZE_US:
            thePrefsStructP->RdataP->print_us = *((Boolean *)incoming);
            break;
        case  ePROXY_SPECFILE:
            thePrefsStructP->RdataP->proxy_specfile = (char *)incoming;
            break;
        case  eNOPROXY_SPECFILE:
            thePrefsStructP->RdataP->noproxy_specfile = (char *)incoming;
            break;
        case  eCCIPORT:
            thePrefsStructP->RdataP->cciPort = *((int *)incoming);
            break;
        case  eMAX_NUM_OF_CCI_CONNECTIONS:
            thePrefsStructP->RdataP->max_num_of_cci_connections =
                *((int *)incoming);
            break;
        case  eMAX_WAIS_RESPONSES:
            thePrefsStructP->RdataP->max_wais_responses = *((int *)incoming);
            break;
        case  eKEEPALIVE:
            thePrefsStructP->RdataP->keepAlive = *((Boolean *)incoming);
            break;
        case  eFTP_TIMEOUT_VAL:
            thePrefsStructP->RdataP->ftp_timeout_val = *((int *)incoming);
            break;
        case  eDEFAULT_WIDTH:
            thePrefsStructP->RdataP->default_width = *((int *)incoming);
            break;
        case  eDEFAULT_HEIGHT:
            thePrefsStructP->RdataP->default_height = *((int *)incoming);
            break;
        case  eAUTO_PLACE_WINDOWS:
            thePrefsStructP->RdataP->auto_place_windows = *((Boolean *)incoming);
            break;
        case  eINITIAL_WINDOW_ICONIC:
            thePrefsStructP->RdataP->initial_window_iconic =
                *((Boolean *)incoming);
            break;
        case  eTITLEISWINDOWTITLE:
            thePrefsStructP->RdataP->titleIsWindowTitle =
                *((Boolean *)incoming);
            break;
        case  eUSEICONBAR:
            thePrefsStructP->RdataP->useIconBar = *((Boolean *)incoming);
            break;
        case  eUSETEXTBUTTONBAR:
            thePrefsStructP->RdataP->useTextButtonBar = *((Boolean *)incoming);
            break;
        case  eTWIRLING_TRANSFER_ICON:
            thePrefsStructP->RdataP->twirling_transfer_icon =
                *((Boolean *)incoming);
            break;
        case  eSECURITYICON:
            thePrefsStructP->RdataP->securityIcon = *((Boolean *)incoming);
            break;
        case  eTWIRL_INCREMENT:
            thePrefsStructP->RdataP->twirl_increment = *((int *)incoming);
            break;
        case  eSAVE_MODE:
            thePrefsStructP->RdataP->save_mode = (char *)incoming;
            break;
        case  eHDF_MAX_IMAGE_DIMENSION:
            thePrefsStructP->RdataP->hdf_max_image_dimension =
                *((int *)incoming);
            break;
        case  eHDF_MAX_DISPLAYED_DATASETS:
            thePrefsStructP->RdataP->hdf_max_displayed_datasets =
                *((int *)incoming);
            break;
        case  eHDF_MAX_DISPLAYED_ATTRIBUTES:
            thePrefsStructP->RdataP->hdf_max_displayed_attributes =
                *((int *)incoming);
            break;
        case  eHDF_POWER_USER:
            thePrefsStructP->RdataP->hdf_power_user = *((Boolean *)incoming);
            break;
        case  eHDFLONGNAME:
            thePrefsStructP->RdataP->hdflongname = *((Boolean *)incoming);
            break;
        case  eFULL_HOSTNAME:
            thePrefsStructP->RdataP->full_hostname = (char *)incoming;
            break;
        case  eLOAD_LOCAL_FILE:
            thePrefsStructP->RdataP->load_local_file = *((int *)incoming);
            break;
        case  eEDIT_COMMAND_USE_XTERM:
            thePrefsStructP->RdataP->edit_command_use_xterm =
                *((Boolean *)incoming);
            break;
        case  eCONFIRM_EXIT:
            thePrefsStructP->RdataP->confirm_exit = *((Boolean *)incoming);
            break;
        case  eDEFAULT_FANCY_SELECTIONS:
            thePrefsStructP->RdataP->default_fancy_selections =
                *((Boolean *)incoming);
            break;
        case  eCATCH_PRIOR_AND_NEXT:
            thePrefsStructP->RdataP->catch_prior_and_next =
                *((Boolean *)incoming);
            break;
        case  eSIMPLE_INTERFACE:
            thePrefsStructP->RdataP->simple_interface = *((Boolean *)incoming);
            break;
        case  ePROTECT_ME_FROM_MYSELF:
            thePrefsStructP->RdataP->protect_me_from_myself =
                *((Boolean *)incoming);
            break;
        case  eGETHOSTBYNAME_IS_EVIL:
            thePrefsStructP->RdataP->gethostbyname_is_evil =
                *((Boolean *)incoming);
            break;
#ifdef __sgi
        case  eDEBUGGING_MALLOC:
            thePrefsStructP->RdataP->debugging_malloc = *((Boolean *)incoming);
            break;
#endif
        case  eUSEAFSKLOG:
            thePrefsStructP->RdataP->useAFSKlog = *((Boolean *)incoming);
            break;

/* New in 2.7 */

        case eSEND_REFERER:
            thePrefsStructP->RdataP->sendReferer = *((Boolean *)incoming);
            break;
        case eSEND_AGENT:
            thePrefsStructP->RdataP->sendAgent = *((Boolean *)incoming);
            break;
        case eEXPAND_URLS:
            thePrefsStructP->RdataP->expandUrls = *((Boolean *)incoming);
            break;
        case eEXPAND_URLS_WITH_NAME:
            thePrefsStructP->RdataP->expandUrlsWithName = *((Boolean *)incoming);
            break;
        case eDEFAULT_PROTOCOL:
            thePrefsStructP->RdataP->defaultProtocol = (char *)incoming;
            break;
        case eMETER_FOREGROUND:
            thePrefsStructP->RdataP->meterForeground = (char *)incoming;
            break;
        case eMETER_BACKGROUND:
            thePrefsStructP->RdataP->meterBackground = (char *)incoming;
            break;
        case eMETER:
            thePrefsStructP->RdataP->use_meter = *((Boolean *)incoming);
            break;
        case eBACKUP_FILES:
            thePrefsStructP->RdataP->backup_files = *((Boolean *)incoming);
            break;
        case ePIX_BASENAME:
            thePrefsStructP->RdataP->pix_basename = (char *)incoming;
            break;
        case ePIX_COUNT:
            thePrefsStructP->RdataP->pix_count = *((int *)incoming);
            break;
        case eACCEPT_LANGUAGE_STR:
            thePrefsStructP->RdataP->acceptlanguage_str = (char *)incoming;
            break;
        case eFTP_REDIAL:
            thePrefsStructP->RdataP->ftpRedial = *((int *)incoming);
            break;
        case eFTP_REDIAL_SLEEP:
            thePrefsStructP->RdataP->ftpRedialSleep = *((int *)incoming);
            break;
        case eFTP_FILENAME_LENGTH:
            thePrefsStructP->RdataP->ftpFilenameLength = *((int *)incoming);
            break;
        case eFTP_ELLIPSIS_LENGTH:
            thePrefsStructP->RdataP->ftpEllipsisLength = *((int *)incoming);
            break;
        case eFTP_ELLIPSIS_MODE:
            thePrefsStructP->RdataP->ftpEllipsisMode = *((int *)incoming);
            break;
        case eTITLE_ISWINDOW_TITLE:
            thePrefsStructP->RdataP->titleIsWindowTitle = *((Boolean *)incoming);
            break;
        case eUSE_SCREEN_GAMMA:
            thePrefsStructP->RdataP->useScreenGamma = *((Boolean *)incoming);
            break;
        case eSCREEN_GAMMA:
            thePrefsStructP->RdataP->screen_gamma = *((float *)incoming);
            break;
        case eHTTPTRACE:
            thePrefsStructP->RdataP->httpTrace = *((Boolean *)incoming);
            break;
        case eWWW2TRACE:
            thePrefsStructP->RdataP->www2Trace = *((Boolean *)incoming);
            break;
        case eHTMLWTRACE:
            thePrefsStructP->RdataP->htmlwTrace = *((Boolean *)incoming);
            break;
        case eCCITRACE:
            thePrefsStructP->RdataP->cciTrace = *((Boolean *)incoming);
            break;
        case eSRCTRACE:
            thePrefsStructP->RdataP->srcTrace = *((Boolean *)incoming);
            break;
        case eCACHETRACE:
            thePrefsStructP->RdataP->cacheTrace = *((Boolean *)incoming);
            break;
        case eNUTTRACE:
            thePrefsStructP->RdataP->nutTrace = *((Boolean *)incoming);
            break;
        case eANIMATEBUSYICON:
            thePrefsStructP->RdataP->animateBusyIcon = *((Boolean *)incoming);
            break;
        case eIMAGEVIEWINTERNAL:
            thePrefsStructP->RdataP->imageViewInternal = *((Boolean *)incoming);
            break;
        case eINSTALL_COLORMAP:
            thePrefsStructP->RdataP->instamap = *((Boolean *)incoming);
            break;
        case eURLEXPIRED:
            thePrefsStructP->RdataP->urlExpired = *((Boolean *)incoming);
            break;
        case ePOPUPCASCADEMAPPINGDELAY:
            thePrefsStructP->RdataP->popupCascadeMappingDelay =
                *((int *)incoming);
	    break;
        case eUSETHREADVIEW:
	    thePrefsStructP->RdataP->newsConfigView = *((int *)incoming);
	    break;
        case eSHOWREADGROUPS:
	  thePrefsStructP->RdataP->newsShowReadGroups = *((int *)incoming);
	  break;
        case eNOTHREADJUMPING:
	  thePrefsStructP->RdataP->newsNoThreadJumping = *((int *)incoming);
          break;
        case eSHOWALLGROUPS:
	  thePrefsStructP->RdataP->newsShowAllGroups = *((int *)incoming);
          break;
        case eSHOWALLARTICLES:
	  thePrefsStructP->RdataP->newsShowAllArticles = *((int *)incoming);
          break;
        case eUSEBACKGROUNDFLUSH:
	  thePrefsStructP->RdataP->newsUseBackgroundFlush = *((int *)incoming);
          break;
        case eBACKGROUNDFLUSHTIME:
	  thePrefsStructP->RdataP->newsBackgroundFlushTime = *((int *)incoming);
          break;
        case eCLIPPING:
            thePrefsStructP->RdataP->clipping =
                *((Boolean *)incoming);
          break;
        case eMAX_CLIPPING_SIZE_X:
          thePrefsStructP->RdataP->max_clip_x = *((int *)incoming);
          break;
        case eMAX_CLIPPING_SIZE_Y:
          thePrefsStructP->RdataP->max_clip_y = *((int *)incoming);
          break;
        case eUSE_LONG_TEXT_NAMES:
            thePrefsStructP->RdataP->long_text_names = *((Boolean *)incoming);
          break;
        case eTOOLBAR_LAYOUT:
            thePrefsStructP->RdataP->toolbar_layout = (char *)incoming;
            break;
        case eNEXTISUNREAD:
          thePrefsStructP->RdataP->newsNextIsUnread = *((int *)incoming);
          break;
        case ePREVISUNREAD:
          thePrefsStructP->RdataP->newsPrevIsUnread = *((int *)incoming);
          break;
        case eUSENEWSRC:
          thePrefsStructP->RdataP->newsUseNewsrc = *((int *)incoming);
          break;
        case eNEWSRCPREFIX:
          thePrefsStructP->RdataP->newsNewsrcPrefix = (char *)incoming;
          break;
        case eNEWSSUBJECTWIDTH:
          thePrefsStructP->RdataP->newsSubjectWidth = *((int *)incoming);
          break;
        case eNEWSAUTHORWIDTH:
          thePrefsStructP->RdataP->newsAuthorWidth = *((int *)incoming);
          break;
        case eFOCUS_FOLLOWS_MOUSE:
            thePrefsStructP->RdataP->focusFollowsMouse =
                *((Boolean *)incoming);
            break;
        case eSESSION_HISTORY_ON_RBM:
            thePrefsStructP->RdataP->sessionHistoryOnRBM =
                *((Boolean *)incoming);
            break;                    
        case eNUMBER_OF_ITEMS_IN_RBM_HISTORY:
            thePrefsStructP->RdataP->numberOfItemsInRBMHistory = *((int *)incoming);                                  
            break;
        case eHOTLIST_ON_RBM:         
            thePrefsStructP->RdataP->hotlistOnRBM =
                *((Boolean *)incoming);
            break;                    
        case eUSESHORTNEWSRC:         
          thePrefsStructP->RdataP->newsUseShortNewsrc = *((int *)incoming);
          break;  
    }
}
