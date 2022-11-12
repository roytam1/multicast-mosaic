#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/gui-documents.h"
#include "../src/gui.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLparse.h"


#include "mc_mosaic.h"
#include "../src/navigate.h"
#include "mc_gui.h"
 
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
	mc_gui_member_list[0].form =XtVaCreateWidget("mc_form", xmFormWidgetClass,
                                mc_member_list_rc_w,
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
	/* affichage de se qu'on connait de cette source */
	if(s->last_valid_url_id != -1)
		McDoWindowText(s, s->last_valid_url_id);
	return;
}

/* add a member at end */
GuiEntry * CreateMemberGuiEntry(Source *s)
{
        mc_gui_member_list = (GuiEntry*) realloc(mc_gui_member_list,
		sizeof(GuiEntry) * (mc_gui_member_count + 1));
        mc_gui_member_list[mc_gui_member_count].form =
		XtVaCreateWidget("mc_form", xmFormWidgetClass,
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

void McDoWindowText(Source *s, unsigned int url_id)
{
	DocEntry * doce;
	struct mark_up *mlist;
	int docid = 0;
	char * goto_anchor = NULL;
	char * aurl_wa = NULL;
	char * aurl = NULL;
	char * base_url = NULL;
	char * base_target = NULL;
	char * title = NULL;

	doce = s->doc[url_id];
	aurl_wa = doce->o_tab[0]->aurl_wa;
	aurl = aurl_wa;
	title = strdup(aurl_wa);
	
	HTMLSetHTMLmark (s->win->scrolled_win, doce->mlist, docid=0, 
		/*pafd->goto_anchor*/ NULL, aurl);
	XFlush(XtDisplay(s->win->scrolled_win));
	MMUpdNavigationOnNewURL(s->win, aurl_wa, aurl, goto_anchor, base_url,
		base_target, title, doce->o_tab[0]->d_part,
		doce->o_tab[0]->mhs, docid,
		doce->mlist);
	mo_set_win_headers(s->win, aurl_wa);
	s->last_valid_url_id = url_id;
}
