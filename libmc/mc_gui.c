#include <stdio.h>
#include <time.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

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
 
GuiEntry * mc_gui_member_list;

static Widget mc_member_list_top_win; /* toplevel widget */
static Widget mc_member_list_rc_w;
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
	XmxAdjustLabelText(mc_gui_member_list[0].toggle, mMosaicAppData.author_full_name);
	XmxAdjustLabelText(mc_gui_member_list[0].label, "Xmit Off");
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

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"PopUpOrDownMMosaicUser\n");
#endif
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
/*	if(s->last_valid_state_id != -1) {
/*		McDoWindowText(s, s->last_valid_state_id);
/*	}
*/
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
        XtAddCallback(mc_gui_member_list[mc_gui_member_count].toggle,
		XmNvalueChangedCallback, PopUpOrDownMMosaicUser,s);
        mc_gui_member_count++;
	return &mc_gui_member_list[mc_gui_member_count-1];
}

void UpdGuiMemberName( Source * s)
{
	int nu = s->gui_ent->nu;
	Widget wname = mc_gui_member_list[nu].toggle;

	XmxAdjustLabelText(wname, s->sdes_name);
}

void UpdGuiMemberPage( Source * s)
{
	int nu = s->gui_ent->nu;
	Widget wname = mc_gui_member_list[nu].label;
	char buf[200];

	sprintf(buf,"Page %d", s->current_view_state);
	XmxAdjustLabelText(wname, buf);
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

	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAME_TYPE);
	memset(&topinfo,0,sizeof(McHtmlTopInfoWindow));
	object = s->objects[start_moid];

	assert(s->objects[start_moid].frame_dot == NULL);

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
	win->htinfo = htinfo;

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
/* ######################################################################## */
/*	free(htinfo); */

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
				assert(0);
                       		/* free(mptr->s_picd); mptr->s_picd = NULL; */
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
				assert(0);
				break; /* error in image tag */
				/* don't try to find it */
			}             
			mptr->s_picd = picd; /* in all case display something*/
			if ( picd->fetched) {
				assert(0);
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
				assert(0);
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
				assert(0);
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
/* ######################################################################## */
		case M_FRAME:  /* for frame we have a callback... */
			assert(0);
			break;
		case M_FRAMESET:
			assert(0);
			s->frameset_moid = start_moid;
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
}

void McSetScrollbars(mo_window *win, int vbar, int hbar)
{
	int val, size, inc, pageinc;

	XmScrollBarGetValues(win->mc_vbar, &val, &size, &inc, &pageinc); 
	XmScrollBarSetValues(win->mc_vbar, vbar, size, inc, pageinc, True);
	XmScrollBarGetValues(win->mc_hbar, &val, &size, &inc, &pageinc); 
	XmScrollBarSetValues(win->mc_hbar, hbar, size, inc, pageinc, True);
}

void McMoveVirtualScrollbar(Source *s, RtpPacket *rs)
{
	McSbValues *nsbvs;	/*netorder */
	McSbValues *sbvs;	/* computer order */
	int sb_id;
	int sid;
	int n;
	int *pi;
	mo_window * win;
	int i;

	win = s->win;
	pi = (int*) rs->d;

	sb_id = ntohl(*pi); pi++;
	sid = ntohl(*pi); pi++;
	n = ntohl(*pi); pi++;

	if ( sid != s->current_view_state) {
		return;
	}
	if (sb_id <= s->current_sb_id) {
		return;
	}
	s->current_sb_id = sb_id;
	sbvs = (McSbValues *) calloc(n,sizeof(McSbValues));
	nsbvs = (McSbValues *)pi;
	for (i=0; i< n; i++) {
		sbvs[i].oid = ntohl(nsbvs[i].oid);
		sbvs[i].vbar = ntohl(nsbvs[i].vbar);
		sbvs[i].hbar = ntohl(nsbvs[i].hbar);
	}

	assert(win->frame_type == NOTFRAME_TYPE || win->frame_type == FRAMESET_TYPE);
	if (win->frame_type == NOTFRAME_TYPE ){
		assert(sbvs[0].oid == win->cur_sb_moid);
		McSetScrollbars(win, sbvs[0].vbar, sbvs[0].hbar);
		free(sbvs);
		return;
	}
	for(i=0; i<win->frame_sons_nbre; i++) {
		mo_window* swin= win->frame_sons[i];

		assert(sbvs[i].oid == swin->cur_sb_moid);

		McSetScrollbars(swin, sbvs[i].vbar, sbvs[i].hbar);
	}

/* frameset */
	free(sbvs);
	return;
}

/* We can do that only if depend object is in multicast cache */
/* We have check for this before */
/* Source have cached filename for html data  and mime part for all object
/* and depend object */
/* Data for state is here too . All we need now is to retrieve data */
/* from file and memory, Order and display them. */
void McDisplayWindowText(Source *s, unsigned int state_id)
{
	McStateStruct state;
	McObjectStruct object;
	McHtmlTopInfoWindow topinfo;
	HtmlTextInfo * htinfo;
	MimeHeaderStruct *mhs;
	mo_window *win = s->win;

	struct mark_up *mlist=NULL;
	char * goto_anchor = NULL;
	char * aurl = NULL;
	char * aurl_wa = NULL;
	char * base_url = NULL;
	char * base_target = NULL;
	char * title = NULL;
	int    start_moid;
	int    docid = 0;		/* we are not in back or forward */

	int    text_len;
	char * text=NULL;
	int    fdr;

	assert(s->states[state_id].state_status == STATE_COMPLETED);

        if (s->mute)                   
                return;                

	if (s->current_view_state == state_id ) {
			/* l'etat recu est celui affiche... */
		return;        
	}                      

/* state is COMPLETE and all depend object of object are here, play with them*/
/* Display it now */                   
        s->current_view_state = state_id;

 	UpdGuiMemberPage(s);
 
        memset(&topinfo,0,sizeof(McHtmlTopInfoWindow)); /* sanity */

/* dewrap old frameset */
        if (win->frame_type == FRAMESET_TYPE) { /* dewrap old */
                int i;                 

                for(i = 0; i < win->frame_sons_nbre; i++) {
                        MMDestroySubWindow(win->frame_sons[i]);
                        win->frame_sons[i] = NULL; /* sanity */
                }                      
                free(win->frame_sons); 
                HTMLUnsetFrameSet (win->scrolled_win);
/* ###################################################################### */
/*              FreeHtmlTextInfo(win->htinfo); don't free here but in nav. */
/* ###################################################################### */
                win->frame_type = NOTFRAME_TYPE;
        } 

	state = s->states[state_id];
	start_moid = state.start_moid;

	object = s->objects[start_moid];
	mhs = object.mhs;
	aurl = s->objects[start_moid].aurl;
	aurl_wa = strdup(aurl);

/* read html text */    
	text_len = object.file_len;
	text = (char*) malloc(text_len+1);
	fdr = open(object.fname,O_RDONLY);
	read(fdr, text, text_len);
	text[text_len] = '\0'; 
	close (fdr);

	win->cur_sb_sid = state_id;
	win->cur_sb_moid = start_moid;

/* parse the text */                   
        htinfo = HTMLParseRepair(text);
	win->htinfo = htinfo;

	if (htinfo->nframes) { /* I am a frameset */
                mo_window *sub_win=NULL;
                Widget htmlw;
                char *url = NULL , *frame_name;
                Widget * htmlw_tabs; /*html widgets return by HTMLSetHTMLmark */
                int i;
 
/* Remarque: la requete (partie HTML) est termine. Tous les frames sont dispo*/
/* puisque les objets dependants sont la */
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */
 
                win->frame_type = FRAMESET_TYPE;
                docid =0;    /* ##### docid = HTMLGetDocId(win->scrolled_win);*/
/* win->scrolled_win est le framset contenant les frame*/
/* htmlw_tabs est un tableau de widget frame qui va contenir les frames*/
        
                HTMLSetFrameSet (win->scrolled_win, htinfo->mlist, aurl,
                        htinfo->nframes, htinfo->frameset_info, &htmlw_tabs);
 
/* on met a jour immediatement la partie navigation. Car on doit avoir un
 * current_node qui memorise tout la requete. Title is always allocated. */
        
                MMUpdNavigationOnNewURL(win, aurl_wa, aurl,
                        goto_anchor, text, mhs, docid, htinfo);
        
                mo_set_win_headers(win, aurl_wa, title);
/*                MMUpdateGlobalHistory(aurl); ######################### */
                XFlush(XtDisplay(win->scrolled_win));
/* init parent FRAMESET */
                win->frame_name = NULL;
                win->frame_parent =NULL;
                win->frame_sons = NULL;
                win->frame_sons_nbre =  htinfo->nframes;
                win->number_of_frame_loaded = 0;
                win->frame_sons = (mo_window **) calloc( win->frame_sons_nbre,
                         sizeof(mo_window *));
                                       
/* use htmlw_tabs. creer les sub_mo_window et demarer la requete pour */
/* htinfo->frameset_info->frames[i].frame_src; */
/* Rechercher les sources pour les frames. Ils sont la! */
                for(i = 0 ; i < htinfo->nframes; i++ )  {
			int frameset_moid;
			int moid;

                        htmlw = htmlw_tabs[i];
                        url = strdup(htinfo->frameset_info->frames[i].frame_src);
                        frame_name = htinfo->frameset_info->frames[i].frame_name;
                        sub_win = MMMakeSubWindow(win, htmlw, url, frame_name);
                        sub_win->frame_sons_nbre = 0;
                        sub_win->frame_dot_index = i;
                        win->frame_sons[i] = sub_win;
        		sub_win->source = s;           
			s->frameset_moid = start_moid;
        		frameset_moid = s->frameset_moid; /* current moid frameset */
			assert(s->objects[frameset_moid].dot == NULL);
        		moid = s->objects[frameset_moid].frame_dot[i]; /* object (html text) to load */
        		McPreParseAndBindTopObject(s, moid, &topinfo,sub_win);
			sub_win->cur_sb_sid = state_id;
			sub_win->cur_sb_moid = moid;
                                       
        		aurl = topinfo.aurl;            /* in mc context : no anchor */
        		mlist = topinfo.mlist;         
        		title = topinfo.title;         
        		text = topinfo.html_text; 
        		mhs = topinfo.mhs;             
        		base_url = topinfo.base_url;   
        		base_target = topinfo.base_target;  
/* on met a jour immediatement la partie navigation. Car on doit avoir un
/* current_node qui memorise tout la requete */
/* title is alway allocated. */        
                                       
        		MMUpdNavigationOnNewURL(sub_win, aurl, aurl,
				goto_anchor, text, mhs, docid, htinfo);

/* Remarque: la requete (partie HTML) est termine et on a change de current_node*//* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */
                                       
        		HTMLSetHTMLmark (sub_win->scrolled_win, mlist, docid, goto_anchor, aurl);
        		XFlush(XtDisplay(sub_win->scrolled_win));
		} /*  for(i = 0 ; i < htinfo->nframes; i++ )  */

/*en finir avec le frameset , quand tous les frames sont loader. */

                return;              
	} /* if iam a framset */

/* c'est le frameset qui s'occupe de l'affichage des frame */
	assert(win->frame_type == NOTFRAME_TYPE);

/* bind moid to its URL , find the cache file name , build html markup */

/*########### part for html only ############ */
	McPreParseAndBindTopObject(s, start_moid, &topinfo, s->win);

	aurl = topinfo.aurl;		/* in mc context : no anchor */
	mlist = topinfo.mlist;
	title = topinfo.title;
	text = topinfo.html_text;
	mhs = topinfo.mhs;
	base_url = topinfo.base_url;
	base_target = topinfo.base_target;

/* on met a jour immediatement la partie navigation. Car on doit avoir un
/* current_node qui memorise tout la requete */
/* title is alway allocated. */
	MMUpdNavigationOnNewURL(s->win, aurl, aurl, goto_anchor, text,
		mhs, docid, s->win->htinfo);
/* Remarque: la requete (partie HTML) est termine et on a change de current_node*/
/* mlist mhs html_text title aurl_wa aurl  must be free in navigation stuff */
/* afficher le texte. le mettre dans la Widget. id come from back and forward */

	HTMLSetHTMLmark (s->win->scrolled_win, mlist, docid, goto_anchor, aurl);
        XFlush(XtDisplay(s->win->scrolled_win));
        mo_set_win_headers(s->win, aurl, title);
        
/*	s->last_valid_state_id = state_id; */
/*	s->current_state_id_in_window = state_id; */
/* MAJ de l'history etc... */
/*### faire un record history pour le multicast  MMUpdateGlobalHistory(aurl);*/
}
