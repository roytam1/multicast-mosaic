#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLparse.h"
#include "../src/mosaic.h"
#include "../src/gui-documents.h"
#include "../src/gui.h"

#include "mc_mosaic.h"
#include "mc_gui.h"
#include "../src/img.h"
#include "../src/navigate.h"
 
static Widget mc_member_list_top_win; /* toplevel widget */
static Widget mc_member_list_rc_w;
static GuiEntry * mc_gui_member_list;
static int mc_gui_member_count = 0;

void McPopdownMemberList(void)
{
        XtPopdown(mc_member_list_top_win);
}
void McPopupMemberList(void)
{       
        XtPopup(mc_member_list_top_win, XtGrabNone);
}
  
void McCreateMemberlist(void)
{
        Arg args[2];
        Widget sw;
 
        XtSetArg(args[0], XmNallowShellResize, True);
        XtSetArg(args[1], XmNtitle, "mMosaic - Members");
        mc_member_list_top_win = XtCreatePopupShell("Members",
                     topLevelShellWidgetClass, mMosaicToplevelWidget, args, 2);
        sw = XtVaCreateManagedWidget ("scrolled_w",
                xmScrolledWindowWidgetClass, mc_member_list_top_win,
                XmNwidth,           200,
                XmNheight,          200,
                XmNscrollingPolicy, XmAUTOMATIC,
                NULL);
 
/* RowColumn is the work window for the widget */
        mc_member_list_rc_w = XtVaCreateWidget ("mc_main_rc",
                        xmRowColumnWidgetClass, sw,
                        XmNorientation, XmVERTICAL,
                        XmNpacking, XmPACK_COLUMN,
                        NULL);

/* create at least one entry for me */
/* for my entry, create a form containing a toggle button and a label */
        mc_gui_member_list = (GuiEntry*) malloc(sizeof(GuiEntry));
	mc_gui_member_list[0].form =XtVaCreateWidget("mc_form",
		 xmFormWidgetClass, mc_member_list_rc_w,
                 XmNborderWidth,      1,
                 NULL);
        mc_gui_member_list[0].toggle = XtVaCreateManagedWidget("mc_alias_name",
                 xmToggleButtonWidgetClass, mc_gui_member_list[0].form,
                 XmNborderWidth,      1,
                 XmNalignment,        XmALIGNMENT_BEGINNING,
                 XmNtopAttachment,    XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_FORM,
                 XmNleftAttachment,   XmATTACH_FORM,
                 XmNrightAttachment,  XmATTACH_NONE,
                 XmNsensitive,        False,
                 NULL);
        mc_gui_member_list[0].label = XtVaCreateManagedWidget("mc_local_url",
                 xmLabelWidgetClass, mc_gui_member_list[0].form,
                 XmNborderWidth,      1,
                 XmNalignment,        XmALIGNMENT_BEGINNING,
                 XmNtopAttachment,    XmATTACH_FORM,
                 XmNbottomAttachment, XmATTACH_FORM,
                 XmNleftAttachment,   XmATTACH_WIDGET,
                 XmNleftWidget,       mc_gui_member_list[0].toggle,
                 XmNrightAttachment,  XmATTACH_NONE,
                 NULL);

	mc_gui_member_count = 1;
	XtManageChild(mc_gui_member_list[0].form);
	XtManageChild(mc_member_list_rc_w);
	XtManageChild(sw);
	XtManageChild(mc_member_list_top_win);
	XtPopdown(mc_member_list_top_win);
}

void PopUpOrDownMMosaicUser(Widget w, XtPointer clid, XtPointer calld)
{
	Source *s = (Source*) clid;
	mo_window * win;

	fprintf(stderr,"PopUpOrDownMMosaicUser\n");
	if (!s->mute) {
/*		sinon destruction de la fenetre pour cette source */
		s->mute = True;
		s->win->first_node = NULL; /* don't free mlist */
		mo_delete_window(s->win);
		s->win = NULL;
		return;
	}
	/* Creation d'une fenetre pour cette source */
	win = mo_make_window(NULL, MC_MO_TYPE_RCV_ALL);
	s->mute = False;
	s->win = win;
	win->source = s;
	/* affichage de se qu'on connait de cette source */
	if(s->last_valid_state_id != -1) {
		McDoWindowText(s, s->last_valid_state_id);
	}
	return;
}

/* add a member at end */
GuiEntry * CreateMemberGuiEntry(Source *s)
{
        mc_gui_member_list = (GuiEntry*) realloc(mc_gui_member_list,
		sizeof(GuiEntry) * (mc_gui_member_count + 1));
        mc_gui_member_list[mc_gui_member_count].form = XtVaCreateWidget(
		"mc_form", xmFormWidgetClass,
                mc_member_list_rc_w,
                XmNborderWidth,      1,
                NULL);
        mc_gui_member_list[mc_gui_member_count].toggle = XtVaCreateManagedWidget(s->s_srcid,
                xmToggleButtonWidgetClass, mc_gui_member_list[mc_gui_member_count].form,
                XmNborderWidth,      1,
                XmNalignment,        XmALIGNMENT_BEGINNING,
                XmNtopAttachment,    XmATTACH_FORM,
                XmNbottomAttachment, XmATTACH_FORM,
                XmNleftAttachment,   XmATTACH_FORM,
                XmNrightAttachment,  XmATTACH_NONE,
                NULL);
        mc_gui_member_list[mc_gui_member_count].label =
		XtVaCreateManagedWidget("s->url",
                xmLabelWidgetClass, mc_gui_member_list[mc_gui_member_count].form,
                        XmNborderWidth,      1,
                        XmNalignment,        XmALIGNMENT_BEGINNING,
                        XmNtopAttachment,    XmATTACH_FORM,
                        XmNbottomAttachment, XmATTACH_FORM,
                        XmNleftAttachment,   XmATTACH_WIDGET,
                        XmNleftWidget,       mc_gui_member_list[mc_gui_member_count].toggle,
                        XmNrightAttachment,  XmATTACH_NONE,
                        NULL);
        XtManageChild (mc_gui_member_list[mc_gui_member_count].form);
        XtManageChild (mc_member_list_rc_w);
	XtRealizeWidget(mc_gui_member_list[mc_gui_member_count].form);
        mc_gui_member_list[mc_gui_member_count].source = s;
	mc_gui_member_list[mc_gui_member_count].nu = mc_gui_member_count;
/*
        XmxAdjustLabelText(mc_wulst[nu].toggle,u->alias);
        XmxAdjustLabelText(mc_wulst[nu].label,u->url);
*/
        XtAddCallback(mc_gui_member_list[mc_gui_member_count].toggle,
		XmNvalueChangedCallback, PopUpOrDownMMosaicUser,s);
        mc_gui_member_count++;
	return &mc_gui_member_list[mc_gui_member_count-1];
}

typedef struct _McHtmlTopInfoWindow {
	struct mark_up *mlist;
	char * aurl;
	char * base_url;
	char * base_target;
	char * title;
	char * html_text;
	MimeHeaderStruct *mhs;
	int docid;
	mo_window * win;
} McHtmlTopInfoWindow;

static void McPreParseAndBindTopObject(Source *s, int start_moid,
	McHtmlTopInfoWindow *top_ret, mo_window *win)
{
	struct mark_up *mlist, *mptr;
	int docid = 0;		/* we are not in back or forward */
	char * aurl = NULL;
	char * base_url = NULL;
	char * base_target = NULL;
	char * title = NULL;
	McHtmlTopInfoWindow topinfo;
	MimeHeaderStruct *mhs;
	McObjectStruct object;
	char * text;
	int text_len;
	char * fname;
	int fdr;
	int num_of_eo = 0;
	HtmlTextInfo * htinfo;
	int moid;

	object = s->objects[start_moid];
	mhs = object.mhs;
	aurl = s->objects[start_moid].aurl;

	text_len = object.file_len;
	fname = object.fname;
/* read html text */
	text = (char*) malloc(text_len+1);
	fdr = open(fname,O_RDONLY);
	read(fdr, text, text_len);
	text[text_len] = '\0';
	close (fdr);

/* parse the text */
	htinfo = HTMLParseRepair(text);
	mlist = htinfo->mlist;

	if (!htinfo->base_url)
		base_url = strdup(aurl);
	else
		base_url = htinfo->base_url;
	base_target = htinfo->base_target;
	if (htinfo->title)
		title = htinfo->title;
	else
		title = strdup(aurl);

/* ### scan the mlist and bind href or frame to depend-moid... #### */
/* ### we can do that because parsing on both side is the same #### */
/* ### else if not we must bind URL to moid. Establish a reverse path ### */
/* ### like the sender part... ### */

/* for each object : load it */
	mptr = mlist;
	num_of_eo = 0;
	while (mptr != NULL){
		char * tptr;
		ImageInfo * picd;
		switch (mptr->type){
		case M_BODY:
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageBody(win, picd, mptr);
			mptr->s_picd = picd;    /* if image body is wrong,*/
						/* display nothing */
			if (!picd->src) {
				free(picd);
				mptr->s_picd = NULL;
				break;
			}

                	mptr->s_picd->src = picd->src;	/*object.aurl;*/
			moid = object.dot[num_of_eo];
/*fectch this object in cache. load it in mptr...*/
                	MMPreloadImage(win, mptr, mhs, s->objects[moid].fname);
                	if ( !mptr->s_picd->fetched ){
				abort();
                       		 free(mptr->s_picd); 
                        	mptr->s_picd = NULL;
                	} 
			num_of_eo++;
                	break;
		case M_INPUT:
			if (mptr->is_end)       /* no end mark on <input> */
				break;
/* get only if type == image */
			tptr = ParseMarkTag(mptr->start, MT_INPUT, "TYPE");
			if (tptr == NULL)       /* assume text */
				break;
			if ( strcasecmp(tptr, "image") ) { /* not an image */
				free(tptr); 
				break;
			}             
			free(tptr);   
/* continue with image processing */  
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseInputTagImage(win, picd, mptr);
			mptr->s_picd = picd; /* in all case display something*/
			if (picd->internal && !picd->fetched && !picd->delayed){
				abort();
				break; /* error in image tag */
				/* don't try to find it */
			}             
			mptr->s_picd = picd; /* in all case display something*/
			if ( picd->fetched) {
				abort();
				break; /* we have it */
			}
                	mptr->s_picd->src = picd->src;	/*object.aurl;*/
			moid = object.dot[num_of_eo];
/*fectch this object in cache. load it in mptr...*/
                	MMPreloadImage(win, mptr, mhs, s->objects[moid].fname);
			num_of_eo++;
			break;
		case M_IMAGE:         
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageTag(win, picd, mptr);
/* on return : two case on fetched:   
 * - fetched = True => this is an internal , width height and image are ok
 * - fetched = False => remaing field is not uptodate and need to be updated
 *       MMPreParseImageTag returns a delayimage
 */                                   
			mptr->s_picd = picd; /* in all case display something*/
			if (picd->internal && !picd->fetched && !picd->delayed){
				abort();
				break; /* error in image tag */
					/* don't try to find it */
			}             
/* now we have an image. It is :      
        - an internal-gopher    (internal=1,fetched=1, delayed =0)
        - a delayed image       (internal=1, fetched=0, delayed=1)
        - the requested image   (internal=0, fetched=1, delayed=0) never happen
 a no image (internal=1, fetched=0, delayed=0) will be made when a resquest
 to get the image is not a succes. or SRC tag is not present.
*/                                    
			if ( picd->fetched) { /* internal image found */
				abort();
				break; /* we have it */
			}             
                	mptr->s_picd->src = picd->src;	/*object.aurl;*/
			moid = object.dot[num_of_eo];
                	MMPreloadImage(win, mptr, mhs, s->objects[moid].fname);
			num_of_eo++;
                	break;
/* see it later ###########           
 *              case M_APPLET:        
 *                      parse for codebase= 
 *              case M_APROG:    must become OBJECT
 *                      parse for something=
 */                                   
/*		case M_FRAME: for frame we have a callback... */
		case M_FRAMESET:
			s->frameset_moid = start_moid;
			s->frameset_dot_count = 0;
			break;
		default:
			break;
		}                     
		mptr = mptr->next;    
	}

	topinfo.base_url 	= base_url;
	topinfo.base_target 	= base_target;
	topinfo.title 		= title;
	topinfo.mhs 		= mhs;
	topinfo.html_text	= text;
	topinfo.mlist 		= mlist;
	topinfo.win 		= win;;
	topinfo.aurl 		= object.aurl;
	topinfo.docid = 0;	/* ### just now FIXE ME */

	*top_ret = topinfo;

/*abort();/* where to free the text ???? */
/*abort(); /* where to free mlist ??? */
/*abort(); /* where to free htinfo and htinfo.mlist ??? */
}

XmxCallback (mc_frame_callback) 
{
	Source *s;
	int moid;
	McHtmlTopInfoWindow topinfo;
        mo_window *win = (mo_window*) client_data;
        mo_window *sub_win=NULL, *parent;
        Widget htmlw;
        char *url = NULL , *frame_name;
        XmHTMLFrameCallbackStruct *cbs = (XmHTMLFrameCallbackStruct *)call_data;
	int docid = 0;
        char * goto_anchor = NULL;
        char * aurl = NULL;
        char * base_url = NULL;
        char * base_target = NULL;
        char * title = NULL;
	char * html_text;
        struct mark_up *mlist;
	MimeHeaderStruct *mhs;
	int frame_count;
	int frameset_moid;

        if (!cbs) { 
                fprintf(stderr, "mc_frame_callback: NULL call_data\n");
                return;
        }
        switch (cbs->reason) {
        case XmCR_HTML_FRAMEDONE:
#ifdef DEBUG_FRAME                    
                fprintf(stderr, "mc_frame_callback: reason: XmCR_HTML_FRAMEDONE\n");                                     
#endif          
                break;                
        case XmCR_HTML_FRAMEDESTROY:  
#ifdef DEBUG_FRAME                    
                fprintf(stderr, "mc_frame_callback: reason: XmCR_HTML_FRAMEDESTROY\n");                                  
#endif          
                break;                
        case XmCR_HTML_FRAMECREATE:   
#ifdef DEBUG_FRAME                    
                fprintf(stderr, "mc_frame_callback: reason: XmCR_HTML_FRAMECREATE\n");                                   
#endif          
                break;                
        case XmCR_HTML_FRAMESETDESTROY:
#ifdef DEBUG_FRAME
                fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMESETDESTROY\n"); 
#endif
                parent = win;
                free(parent->frame_sons) ;
                win->frame_name = NULL; 
                win->frame_parent =NULL;
                win->frame_sons = NULL; 
                win->frame_sons_nbre =0;
                win->number_of_frame_loaded = 0;
                break;
        case XmCR_HTML_FRAMESET_INIT: 
#ifdef DEBUG_FRAME
                fprintf(stderr, "frame_callback: reason: XmCR_HTML_FRAMESET_INIT\n");
#endif 
                win->frame_name = NULL; 
                win->frame_parent =NULL; 
                win->frame_sons = NULL; 
/*                win->frame_sons_nbre =0;  */
/*??? = cbs.nframe */ 
                win->frame_sons_nbre =  cbs->nframe ;
                win->number_of_frame_loaded = 0;
                parent = win;
                parent->frame_sons = (mo_window **) realloc(parent->frame_sons,
                        (parent->frame_sons_nbre) * sizeof(mo_window *));
                break; 
        default:                      
		abort();
                fprintf(stderr, "mc_frame_callback: reason: Unknowed...\n");
                break;                
        }                             
        if (cbs->reason != XmCR_HTML_FRAMEDONE ){
                return;         /* just for test now */
        }                             
#ifdef DEBUG_FRAME                    
        fprintf(stderr,"cbs.event = %08x\n cbs.src = %s\n cbs.name = %s\n",
                cbs->event, cbs->src, cbs->name);
        fprintf(stderr,"bs.html = %08x\n cbs.doit = %d\n",
                cbs->html, cbs->doit);
#endif
/* reason = XmCR_HTML_FRAMEDONE */    
        parent = win;                 
        htmlw = cbs->html;            
        url = cbs->src;       
        frame_name = cbs->name;       
        sub_win = MMMakeSubWindow(parent, htmlw, url, frame_name);

	sub_win->frame_dot_index = cbs->index;
	sub_win->frame_sons_nbre = 0;
	parent->frame_sons[cbs->index] = sub_win;

/* get the source */
	s = parent->source;
	sub_win->source = s;

	frame_count = s->frameset_dot_count;
	frameset_moid = s->frameset_moid;	/* current moid frameset */
	moid = s->objects[frameset_moid].dot[frame_count]; /* object (html text) to load */
	McPreParseAndBindTopObject(s, moid, &topinfo,sub_win);
	frame_count++;
	s->frameset_dot_count = frame_count;

        aurl = topinfo.aurl;            /* in mc context : no anchor */
        mlist = topinfo.mlist;         
        title = topinfo.title;         
        html_text = topinfo.html_text; 
        mhs = topinfo.mhs;             
        base_url = topinfo.base_url;   
        base_target = topinfo.base_target;
/* on met a jour immediatement la partie navigation. Car on doit avoir un
/* current_node qui memorise tout la requete */
/* title is alway allocated. */        
        MMUpdNavigationOnNewURL(sub_win, aurl, aurl, goto_anchor, base_url,
                base_target, title, html_text, mhs, docid, mlist);
/* Remarque: la requete (partie HTML) est termine et on a change de current_node*//* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */
                                       
        HTMLSetHTMLmark (sub_win->scrolled_win, mlist, docid, goto_anchor, aurl);
        XFlush(XtDisplay(sub_win->scrolled_win));
}

/* we can do that only if depend object is in multicast cache */
/* we have check for this before */
/* Source have cached filename for html data  and mime part */
/* for all object and depend object */
/* data for state is here too . All we need now is to retrieve data */
/* from file and memory, Order them and display them. */
void McDoWindowText(Source *s, unsigned int state_id)
{
	McStateStruct state;
	int start_moid;
	struct mark_up *mlist;
	int docid = 0;		/* we are not in back or forward */
	char * goto_anchor = NULL;
	char * aurl = NULL;
	char * base_url = NULL;
	char * base_target = NULL;
	char * title = NULL;
	char * html_text;
	McHtmlTopInfoWindow topinfo;
	MimeHeaderStruct *mhs;

	state = s->states[state_id];
	start_moid = state.start_moid;

/* bind moid to its URL , find the cache file name , build html markup */
/* make that for the very top first object */
/* that is a recursive func. depending of FRAMESET */
	McPreParseAndBindTopObject(s, start_moid, &topinfo, s->win);

	aurl = topinfo.aurl;		/* in mc context : no anchor */
	mlist = topinfo.mlist;
	title = topinfo.title;
	html_text = topinfo.html_text;
	mhs = topinfo.mhs;
	base_url = topinfo.base_url;
	base_target = topinfo.base_target;

/* on met a jour immediatement la partie navigation. Car on doit avoir un
/* current_node qui memorise tout la requete */
/* title is alway allocated. */
	MMUpdNavigationOnNewURL(s->win, aurl, aurl, goto_anchor, base_url,
		base_target, title, html_text, mhs, docid, mlist);
/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */

	HTMLSetHTMLmark (s->win->scrolled_win, mlist, docid, goto_anchor, aurl);
        XFlush(XtDisplay(s->win->scrolled_win));
        mo_set_win_headers(s->win, aurl);
        
	s->last_valid_state_id = state_id;
	s->current_state_id_in_window = state_id;
/* MAJ de l'history etc... */
/*### faire un record history pour le multicast  MMUpdateGlobalHistory(aurl);*/
}
