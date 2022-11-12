/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <Xm/PanedW.h>
#include <Xm/PushB.h>
#include <Xm/ToggleB.h>
#include <Xm/Form.h>
#include <Xm/DialogS.h>
#include <Xm/List.h>
#include <Xm/MenuShell.h>
#include <Xm/Separator.h>
#include <Xm/RowColumn.h>
#include <Xm/Text.h>
#include <Xm/TextF.h>
#include <Xm/Frame.h>
#include <Xm/Label.h>
#include <Xm/MessageB.h>

#ifdef __bsdi__
#include <sys/malloc.h>
#else
#include <malloc.h>
#endif
#include <string.h>
#include "proxy.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"

#define EDIT_ERROR "No entry in the proxy list is currently selected.\n\nTo edit an entry in the proxy list, select\nit with a double mouse click."

#define SAVE_ERROR "You do not have permission to write the\nfile %s to disk."

#define SAVED_AOK "%s has been saved."

#define REMOVE_ERROR "No entry in the list is currently selected.\n\nTo remove an entry in the list, select\n it and then click on the remove widget."

#define COMMIT_PROXY_EMPTY_ERROR "A scheme must be specified for this proxy entry."
#define COMMIT_ADDR_EMPTY_ERROR "An address must be specified for this proxy entry."
#define COMMIT_PORT_EMPTY_ERROR "A port number must be specified for this proxy entry."

#define COMMIT_DOMAIN_EMPTY_ERROR "A domain must be specified for this entry."

#define EDIT 0
#define ADD  1

struct EditInfo {
	int fProxy; /* Is this a Proxy List Dialog? */
	int type;  /* Is this an Edit or an Add ? */
	int domaintype;  /* Is this an Edit or an Add ? */
	char *help_file;
	Widget scrolled; 
	struct Proxy *editing;
	struct InfoFields *IF;
	struct Proxy *proxy_list;
};


#define BUFLEN 256
#define BLANKS " \t\n"

char * mMosaicProxyFileName = NULL;
char * mMosaicNoProxyFileName = NULL;

static struct Proxy * noproxy_list =NULL;
static struct Proxy * proxy_list =NULL;

Widget ProxyDialog, EditProxyDialog, EditNoProxyDialog;

void AppendProxy(struct EditInfo *pEditInfo, struct Proxy *p);

struct InfoFields {
	Widget proxy_text;
	Widget addr_text;
	Widget port_text;
	Widget alive;
};

static void AddProxyToList(struct EditInfo *pEditInfo, struct Proxy *proxy);
static void ShowProxyList(struct EditInfo *pEditInfo);
static void EditProxyInfo( Widget w, XtPointer client, XtPointer call, int type);
static void CommitProxyInfo( Widget w, XtPointer client, XtPointer call);
static void DismissProxy( Widget w, XtPointer client, XtPointer call);
static void ClearProxyText(struct EditInfo *p);
static void FillProxyText(struct EditInfo *p);
static void WriteProxies(Widget w, XtPointer client, XtPointer call);
static void RemoveProxyInfo(Widget w, XtPointer client, XtPointer call);
static void CallEdit(Widget w, XtPointer client, XtPointer call);
static void CallAdd(Widget w, XtPointer client, XtPointer call) ;
static void DestroyDialog(Widget w, XtPointer client, XtPointer call);
static void PopProxyDialog(mo_window *win, struct Proxy *list, int fProxy);
static void DeleteProxy(struct EditInfo *pEditInfo, struct Proxy *p);
static void EditNoProxyInfo( Widget w, XtPointer client, XtPointer call, int type);
static void CenterDialog(Widget dialog, XtPointer client, XtPointer call);
static void ProxyHelpWindow(Widget w, XtPointer client, XtPointer call);
static void HelpWindow(Widget w, XtPointer client, XtPointer call);
static struct Proxy * FindProxyEntry(struct EditInfo *pEditInfo, char *txt);

static mo_window *mo_main_window;

void ReadProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;

	mMosaicProxyFileName = (char*) malloc(strlen (mmosaic_root_dir) +
                                strlen ("proxy") + 8);
	sprintf (mMosaicProxyFileName, "%s/proxy", mmosaic_root_dir);
	if ((fp = fopen(mMosaicProxyFileName,"r")) == NULL)
		return; 
	head = NULL;
	cur = NULL;

/* read entries from the proxy list
 * These malloc()s should be checked for returning NULL
 */
	while (fgets(buf, BUFLEN, fp) != 0) {
		p = (struct Proxy *)calloc(1,sizeof(struct Proxy));
		p->next = NULL;
		p->prev = NULL;

/* Read the proxy scheme */
		if ((psb = strtok(buf, BLANKS)) == NULL){
			proxy_list = head;
			return ;
		}
		p->scheme = (char *)malloc(strlen(psb)+1);
		strcpy(p->scheme, psb);

/* Read the proxy address */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			proxy_list = head;
			return ;
		}
		p->address = (char *)malloc(strlen(psb)+1);
		strcpy(p->address, psb);

/* Read the proxy port */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			proxy_list = head;
			return ;
		}
		p->port = (char *)malloc(strlen(psb)+1);
		strcpy(p->port, psb);

/* Read the transport mechanism */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			proxy_list = head;
			return ;
		}
		p->transport = (char *)malloc(strlen(psb)+1);
		strcpy(p->transport, psb);

		p->alive = 0;
		if (cur == NULL) {
			head = p;
			cur = p;
		} else {
			p->prev = cur;
			cur->next = p;
			cur = p;
		}
		if (feof(fp) != 0)
			break;
	}
	proxy_list = head;
}

void ReadNoProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;

	mMosaicNoProxyFileName = (char*) malloc(strlen (mmosaic_root_dir) +
                                strlen ("noproxy") + 8);
	sprintf (mMosaicNoProxyFileName, "%s/noproxy", mmosaic_root_dir);
	if ((fp = fopen(mMosaicNoProxyFileName,"r")) == NULL)
		return ;
	head = NULL;
	cur = NULL;

/* read entries from the proxy list
 * These malloc()s should be checked for returning NULL
*/
	while (fgets(buf, BUFLEN, fp) != 0) {
		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		p->next = NULL;
		p->prev = NULL;

/* The proxy protocol, transport, and list are all null for no proxy. */
		p->scheme = NULL;
		p->transport = NULL;

/* Read the proxy address */
		if ((psb = strtok(buf, BLANKS)) == NULL) {
			noproxy_list = head;
			return ;
		}
		p->address = (char *)malloc(strlen(psb)+1);
		strcpy(p->address, psb);

/* Read the proxy port */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			p->port = NULL;
		} else {
			p->port = (char *)malloc(strlen(psb)+1);
			strcpy(p->port, psb);
		}
		if (cur == NULL) {
			head = p;
			cur = p;
		} else {
			p->prev = cur;
			cur->next = p;
			cur = p;
		}
		if (feof(fp) != 0)
			break;
	}
	noproxy_list = head;
}

/* Returns true if there is at least one fallback proxy for the specified
 * protocol (means more than one proxy server specified).
 * --SWP
 */
int has_fallbacks(char *protocol)
{
	int protocol_len;
	struct Proxy *ptr;

      if (!protocol || !*protocol || !proxy_list)
              return(0);

      protocol_len=strlen(protocol);
      ptr=proxy_list;

      while (ptr) {
              if (ptr->scheme && !strncmp(ptr->scheme,protocol,protocol_len))
                      return(1);
              ptr=ptr->next;
      }
      return(0);
}


void ShowProxyDialog(mo_window *win)
{
	PopProxyDialog(win, proxy_list, TRUE);
}

void ShowNoProxyDialog(mo_window *win)
{
	PopProxyDialog(win, noproxy_list, FALSE);
}

void PopProxyDialog(mo_window *win, struct Proxy *list, int fProxy)
{
	Widget main_form, action_area;
	Widget add, edit, remove, dismiss, help;
	Widget save;
	Widget scrolled;
	int  n;
	Arg args[20];
	static struct EditInfo *pEditInfo;

	XFontStruct *font;
	XmFontList *fontlist;

	static int fProxyDialog = 0;

	mo_main_window = win;

	if (fProxyDialog) {
		pEditInfo->proxy_list = list;
		pEditInfo->fProxy = fProxy;

		if (fProxy)
			pEditInfo->help_file = "help-proxylist.html";
		else
			pEditInfo->help_file = "help-noproxylist.html";
		ShowProxyList(pEditInfo);
		XtPopup(ProxyDialog, XtGrabNone);
		return;
	}

/* Try and get a nice non-proportional font.  If we can't get 
 * it, then the heck with it, just use the default.
*/
	font = XLoadQueryFont(XtDisplay(win->base), FONTNAME);
	if (font == NULL) {
		font = XLoadQueryFont(XtDisplay(win->base), "fixed");
	}
	if (font != NULL) {
		fontlist = (XmFontList *)XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
	}

	ProxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, XtParent(win->base),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"Proxies",
		NULL);

	fProxyDialog = 1;

	main_form = XtVaCreateWidget("proxy_form",
		xmFormWidgetClass, ProxyDialog,
		NULL);

/* Create action area widgets */
	action_area = XtVaCreateWidget("proxy_action",
		xmFormWidgetClass,	main_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,    XmATTACH_FORM,
		XmNfractionBase, 6,
		NULL);

	dismiss = XtVaCreateManagedWidget("Dismiss", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	0,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	1,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	save = XtVaCreateManagedWidget("Save", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	1,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	2,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	edit = XtVaCreateManagedWidget("Edit", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	2,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	3,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);

	add = XtVaCreateManagedWidget("Add", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	3,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	4,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	remove = XtVaCreateManagedWidget("Remove", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	4,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	5,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);

	help = XtVaCreateManagedWidget("Help...", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	5,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	6,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);

	XtManageChild(action_area);

	/*
	** Create Scrolled List
	*/

	n = 0;
	XtSetArg(args[n], XmNwidth,	150); n++;
	XtSetArg(args[n], XmNvisibleItemCount,	10); n++;
	XtSetArg(args[n], XmNmargin,		1); n++;
	XtSetArg(args[n], XmNtopAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNleftAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNrightAttachment, XmATTACH_FORM); n++;
	XtSetArg(args[n], XmNbottomAttachment, XmATTACH_WIDGET); n++;
	XtSetArg(args[n], XmNbottomWidget, action_area); n++;
	if (font)
		XtSetArg(args[n], XmNfontList, fontlist); n++;
	
	scrolled = XmCreateScrolledList(main_form, "proxy_info", args, n);
	XtManageChild(scrolled);

	pEditInfo = (struct EditInfo *)calloc(1,sizeof(struct EditInfo));
	pEditInfo->scrolled = scrolled;
	pEditInfo->proxy_list = list;
	pEditInfo->fProxy = fProxy;
	if (fProxy)
		pEditInfo->help_file = "help-proxylist.html";
	else
		pEditInfo->help_file = "help-noproxylist.html";


        XtAddCallback(edit, XmNactivateCallback, CallEdit, pEditInfo);
	XtAddCallback(scrolled, XmNdefaultActionCallback, CallEdit, pEditInfo);
	XtAddCallback(add, XmNactivateCallback, CallAdd, pEditInfo);
        XtAddCallback(remove, XmNactivateCallback, RemoveProxyInfo, pEditInfo);
        XtAddCallback(dismiss, XmNactivateCallback, DismissProxy, ProxyDialog);
	XtAddCallback(save, XmNactivateCallback, WriteProxies, pEditInfo);
	XtAddCallback(ProxyDialog, XmNdestroyCallback, DestroyDialog, &fProxyDialog);
	XtAddCallback(ProxyDialog, XmNpopupCallback, CenterDialog, NULL);
	XtAddCallback(help, XmNactivateCallback, ProxyHelpWindow, pEditInfo);
	XtManageChild(main_form);
	ShowProxyList(pEditInfo);
	XtPopup(ProxyDialog, XtGrabNone);
}

void ProxyHelpWindow(Widget w, XtPointer client, XtPointer call)
{
	char *html_file = ((struct EditInfo *)client)->help_file;
	HelpWindow(w, html_file, call);
}

void HelpWindow(Widget w, XtPointer client, XtPointer call)
{
	char *html_file = (char *)client;

	mo_open_another_window(mo_main_window, mo_assemble_help_url(html_file));
}

void CenterDialog(Widget dialog, XtPointer client, XtPointer call)
{
	Position x, y;
	Dimension width, height;
	Dimension dia_width, dia_height;
	Position center_x, center_y, dia_center_x, dia_center_y;

	XtVaGetValues(mo_main_window->base,
		XmNx,	&x,
		XmNy,	&y,
		XmNwidth, &width,
		XmNheight, &height,
		NULL);

	XtVaGetValues(dialog,
		XmNwidth, &dia_width,
		XmNheight, &dia_height,
		NULL);

	center_x = width/2;
	center_y = height/2;

	dia_center_x = center_x - (dia_width/2);
	dia_center_y = center_y - (dia_height/2);

	dia_center_x += x;
	dia_center_y += y;

	XtVaSetValues(dialog,
		XmNx, dia_center_x,
		XmNy, dia_center_y,
		NULL);
}

void CallEdit(Widget w, XtPointer client, XtPointer call)
{
	struct EditInfo *pEditInfo;

	pEditInfo = (struct EditInfo *)client;

	if (pEditInfo->fProxy)
		EditProxyInfo(w, client, call, EDIT);
	else
		EditNoProxyInfo(w, client, call, EDIT);
}

void CallAdd(Widget w, XtPointer client, XtPointer call)
{
	struct EditInfo *pEditInfo;
	struct ProxyDomain *p, *next;

	pEditInfo = (struct EditInfo *)client;

	if (pEditInfo->fProxy)
		EditProxyInfo(w, client, call, ADD);
	else
		EditNoProxyInfo(w, client, call, ADD);
}

XmString GetStringFromScrolled(Widget w)
{
	int selected_count;
	XmStringTable selected_table;

	XtVaGetValues(w,
		XmNselectedItemCount, &selected_count,
		XmNselectedItems, &selected_table,
		NULL);

	if (selected_count == 0)
		return NULL;

	return selected_table[0];
}

void EditNoProxyInfo( Widget w, XtPointer client, XtPointer call, int type)
{
	Widget text_form, form, address, port;
	Widget main_form, rc, rc2;
	Widget action_area, sep, dismiss, help;
	Widget commit;
	XmString selected_string;
	char *selected_text;
	struct EditInfo *pEditInfo;
	static int fEditProxyDialog = 0;

	/*
	** We obtain information from the client pointer, rather than getting
	** it from call->item because this routine can be called from
	** a pushbutton as well as from double clicking the list.
	*/

	pEditInfo = (struct EditInfo *)client;

	pEditInfo->type = type;
	if (type == EDIT) {
		selected_string = GetStringFromScrolled((Widget)pEditInfo->scrolled);

		if (selected_string == NULL) {
			XmxMakeErrorDialog(mo_main_window->base, EDIT_ERROR, "No Entry Selected");
			XtManageChild (Xmx_w);
			return;
		}

		XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, 
					&selected_text);

		pEditInfo->editing = FindProxyEntry(pEditInfo, selected_text);

		XtFree(selected_text);

		if (pEditInfo->editing == NULL)
			return; /* how did *that* happen? */
	} else {
		pEditInfo->editing = NULL;
	}

	if (fEditProxyDialog) {

		if (type == EDIT)
			FillProxyText(pEditInfo);
		else
			ClearProxyText(pEditInfo);

		XtPopup(EditNoProxyDialog, XtGrabNone);
		return;
	}

	EditNoProxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, XtParent(w),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"No_Proxy Information",
		NULL);

	fEditProxyDialog = 1;
	main_form = XtVaCreateWidget("edit_form",
		xmFormWidgetClass, EditNoProxyDialog,
		NULL);
	/*
	** Create action area widgets
	*/
	action_area = XtVaCreateWidget("edit_action",
		xmFormWidgetClass,	main_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,    XmATTACH_FORM,
		XmNfractionBase, 3,
		NULL);

	sep = XtVaCreateManagedWidget("separator",
		xmSeparatorWidgetClass, action_area,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	commit = XtVaCreateManagedWidget("Commit", 
		xmPushButtonWidgetClass, action_area,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	1,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	2,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	dismiss = XtVaCreateManagedWidget("Abort", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		sep,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	0,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	1,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	help = XtVaCreateManagedWidget("Help...", 
		xmPushButtonWidgetClass, action_area,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	2,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	3,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);

	XtManageChild(action_area);

	rc = XtVaCreateWidget("rowcolumn",
		xmRowColumnWidgetClass, main_form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNorientation, XmHORIZONTAL,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, action_area,
		NULL);

	text_form = XtVaCreateWidget("text_form",
		xmFormWidgetClass, rc,
		NULL);

	rc2 = XtVaCreateWidget("rowcolumn2",
		xmRowColumnWidgetClass, text_form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	pEditInfo->IF = (struct InfoFields *)calloc(1,sizeof(struct InfoFields));

	pEditInfo->IF->proxy_text = NULL;
	pEditInfo->IF->alive = NULL;
		
	form = XtVaCreateWidget("form1",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	address = XtVaCreateManagedWidget("Address",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_END,
		NULL);

	pEditInfo->IF->addr_text = XtVaCreateManagedWidget("addr_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(form);
		
	form = XtVaCreateWidget("form2",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	port = XtVaCreateManagedWidget("Port",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_END,
		NULL);

	pEditInfo->IF->port_text = XtVaCreateManagedWidget("port_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNmaxLength, 5,
		XmNcolumns, 6,
		NULL);
	XtManageChild(form);
	XtManageChild(rc2);
	XtManageChild(text_form);
	XtManageChild(rc);
	XtManageChild(main_form);

        XtAddCallback(commit, XmNactivateCallback, CommitProxyInfo, pEditInfo);
        XtAddCallback(dismiss, XmNactivateCallback, DismissProxy, EditNoProxyDialog);
	XtAddCallback(EditNoProxyDialog, XmNdestroyCallback, DestroyDialog, &fEditProxyDialog);
	XtAddCallback(EditNoProxyDialog, XmNpopupCallback, CenterDialog, NULL);
	XtAddCallback(help, XmNactivateCallback, HelpWindow, "help-noproxy-edit.html");

	if (type == EDIT)
		FillProxyText(pEditInfo);
	else
		ClearProxyText(pEditInfo);

	XtPopup(EditNoProxyDialog, XtGrabNone);
}

void EditProxyInfo( Widget w, XtPointer client, XtPointer call, int type)
{
	Widget text_form, form, protocol, address, port;
	Widget trans_rc, label;
	Widget main_form, rc, rc2, rc3;
	Widget action_area, sep, dismiss, help;
	Widget commit;
	static Widget  add, remove, edit;
	XmString selected_string;
	char *selected_text;
	struct EditInfo *pEditInfo ;
	static int fEditProxyDialog = 0;

	/*
	** We obtain information from the client pointer, rather than getting
	** it from call->item because this routine can be called from
	** a pushbutton as well as from double clicking the list.
	*/

	pEditInfo = (struct EditInfo *)client;

	pEditInfo->type = type;
	if (type == EDIT) {
		selected_string = GetStringFromScrolled((Widget)pEditInfo->scrolled);

		if (selected_string == NULL) {
			XmxMakeErrorDialog(mo_main_window->base, EDIT_ERROR, "No Entry Selected");
			XtManageChild (Xmx_w);
			return;
		}

		XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, 
					&selected_text);

		pEditInfo->editing = FindProxyEntry(pEditInfo, selected_text);

		XtFree(selected_text);

		if (pEditInfo->editing == NULL)
			return; /* how did *that* happen? */
	} else {
		pEditInfo->editing = NULL;
	}

	if (fEditProxyDialog) {

		if (type == EDIT) {
			FillProxyText(pEditInfo);
		} else {
			ClearProxyText(pEditInfo);

		}

		XtPopup(EditProxyDialog, XtGrabNone);
		return;
	}
	EditProxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, XtParent(w),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"Proxy Information",
		NULL);

	fEditProxyDialog = 1;
	main_form = XtVaCreateWidget("edit_form",
		xmFormWidgetClass, EditProxyDialog,
		NULL);
	/*
	** Create action area widgets
	*/
	action_area = XtVaCreateWidget("edit_action",
		xmFormWidgetClass,	main_form,
		XmNleftAttachment,	XmATTACH_FORM,
		XmNrightAttachment,	XmATTACH_FORM,
		XmNbottomAttachment,    XmATTACH_FORM,
		XmNfractionBase, 3,
		NULL);

	sep = XtVaCreateManagedWidget("separator",
		xmSeparatorWidgetClass, action_area,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	commit = XtVaCreateManagedWidget("Commit", 
		xmPushButtonWidgetClass, action_area,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	1,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	2,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	dismiss = XtVaCreateManagedWidget("Abort", 
		xmPushButtonWidgetClass, action_area,
		XmNtopAttachment,	XmATTACH_WIDGET,
		XmNtopWidget,		sep,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	0,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	1,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);
	
	help = XtVaCreateManagedWidget("Help...", 
		xmPushButtonWidgetClass, action_area,
		XmNbottomAttachment,	XmATTACH_FORM,
		XmNleftAttachment,	XmATTACH_POSITION,
		XmNleftPosition,	2,
		XmNrightAttachment,	XmATTACH_POSITION,
		XmNrightPosition,	3,
		XmNshowAsDefault,	True,
		XmNdefaultButtonShadowThickness,	1,
		NULL);

	XtManageChild(action_area);

	rc = XtVaCreateWidget("rowcolumn",
		xmRowColumnWidgetClass, main_form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNorientation, XmHORIZONTAL,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, action_area,
		NULL);

	text_form = XtVaCreateWidget("text_form",
		xmFormWidgetClass, rc,
		NULL);

	rc2 = XtVaCreateWidget("rowcolumn2",
		xmRowColumnWidgetClass, text_form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		NULL);

	form = XtVaCreateWidget("form1",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	protocol = XtVaCreateManagedWidget("Scheme",
		xmLabelWidgetClass, form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_END,
		NULL);

	pEditInfo->IF = (struct InfoFields *)calloc(1,sizeof(struct InfoFields));

	pEditInfo->IF->proxy_text= XtVaCreateManagedWidget("proxy_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(form);
		
	form = XtVaCreateWidget("form2",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	label = XtVaCreateManagedWidget("Proxy",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

	address = XtVaCreateManagedWidget("Address",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

	pEditInfo->IF->addr_text = XtVaCreateManagedWidget("addr_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(form);
		
	form = XtVaCreateWidget("form3",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	label = XtVaCreateManagedWidget("Proxy",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

	port = XtVaCreateManagedWidget("Port",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget, label,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

	pEditInfo->IF->port_text = XtVaCreateManagedWidget("port_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNtopAttachment, XmATTACH_FORM,
		XmNmaxLength, 5,
		XmNcolumns, 6,
		NULL);

	XtManageChild(form);

	form = XtVaCreateWidget("form4",
		xmFormWidgetClass, rc2,
		XmNfractionBase, 10,
		NULL);

	label = XtVaCreateManagedWidget("Alive",
		xmLabelWidgetClass,	form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_BEGINNING,
		NULL);

	pEditInfo->IF->alive = XtVaCreateManagedWidget("alive",
		xmToggleButtonWidgetClass, form,
		XmNlabelString, XmStringCreateSimple(""),
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 3,
		XmNrightAttachment, XmATTACH_FORM,
		XmNtopAttachment, XmATTACH_FORM,
		NULL);

	XtManageChild(form);

	XtManageChild(rc2);
	XtManageChild(text_form);
	XtManageChild(rc);
	XtManageChild(main_form);

        XtAddCallback(commit, XmNactivateCallback, CommitProxyInfo, pEditInfo);
        XtAddCallback(dismiss, XmNactivateCallback, DismissProxy, EditProxyDialog);
	XtAddCallback(EditProxyDialog, XmNdestroyCallback, DestroyDialog, &fEditProxyDialog);
	XtAddCallback(EditProxyDialog, XmNpopupCallback, CenterDialog, NULL);

	XtAddCallback(help, XmNactivateCallback, HelpWindow, "help-proxy-edit.html");

	if (type == EDIT)
		FillProxyText(pEditInfo);
	else
		ClearProxyText(pEditInfo);

	XtPopup(EditProxyDialog, XtGrabNone);
}

void ShowProxyList(struct EditInfo *pEditInfo)
{
	Widget scrolled;
	struct Proxy *proxy;

	scrolled = pEditInfo->scrolled;

	proxy = pEditInfo->proxy_list; 

	XmListDeleteAllItems(pEditInfo->scrolled);

	while (proxy != NULL) {
		AddProxyToList(pEditInfo, proxy);
		proxy = proxy->next;
	}
}

void AddProxyToList(struct EditInfo *pEditInfo, struct Proxy *proxy)
{
	char *p;
	XmString string;
	Widget scrolled = pEditInfo->scrolled;

	if ((p = (char *)malloc(256*sizeof(char))) == NULL)
		return;

	if (pEditInfo->fProxy)
		sprintf(p,"%-12.12s %s:%s",proxy->scheme, proxy->address, proxy->port);
	else {
		if (proxy->port)
			sprintf(p,"%s:%s",proxy->address,proxy->port);
		else
			sprintf(p,"%s", proxy->address);
	}

	string = XmStringCreateSimple(p);

	XmListAddItem(scrolled, string, 0);
	free(p);
	XmStringFree(string);

}

struct Proxy * GetNoProxy(char *access, char *site)
{
	struct Proxy *p = noproxy_list;
	char *port = NULL;
	int portnum = -1;

	if ((access == NULL) || (site == NULL))
		return NULL;

	if ((port = strchr(site,':')) != NULL) {
		*port++ = 0;
		portnum = atoi(port);
	} else {
		if      (!strcmp(access,"http"))    portnum = 80;
		else if (!strcmp(access,"gopher"))  portnum = 70;
		else if (!strcmp(access,"ftp"))     portnum = 21;
		else if (!strcmp(access,"wais"))    portnum = 210;
	}
	while (p != NULL) {
		if (strstr(site,p->address)) {
			if (p->port == NULL) {
				break;
			} else { 
				int match_port = atoi(p->port);
				if (match_port == portnum)
					break;
			}
		}
		p = p->next;
	}
	return p;
}

void ClearTempBongedProxies()
{
	struct Proxy *p = proxy_list;

	while (p!=NULL) {
		if (p->alive==2) {
			p->alive=0;
		}
		p=p->next;
	}
}

static char senv[100];
static struct Proxy proxy_env;

struct Proxy * GetProxy(char *acc)
{
	struct Proxy *p = proxy_list;
	char * port;
	char * renv;

	if (acc == NULL)
		return NULL;
	while (p != NULL) {
		if (strcmp(p->scheme, acc) != 0 || p->alive) {
			p = p->next;
			continue;
		}
		return p; /* found a proxy */
	}
	sprintf(senv,"%s_proxy", acc);
	renv = getenv(senv);
	if (renv == NULL)
		return NULL;
	if ((port = strchr(renv,':')) != NULL) {
		*port++ = 0;
		proxy_env.port = port;
	} else {
		if      (!strcmp(acc,"http"))    proxy_env.port = "80";
		else if (!strcmp(acc,"gopher"))  proxy_env.port = "70";
		else if (!strcmp(acc,"ftp"))     proxy_env.port = "21";
		else if (!strcmp(acc,"wais"))    proxy_env.port = "210";
	}
	proxy_env.address = renv;
	return &proxy_env;
}

struct Proxy * FindProxyEntry(struct EditInfo *pEditInfo, char *txt)
{
	struct Proxy *p;
	int fProxy;
	char proxy[30], address[50], port[8], *ptr;

	fProxy = pEditInfo->fProxy;

	if (fProxy) {
		sscanf(txt,"%s %s",proxy,address); 
		ptr = strchr(address,':');
		if (ptr) {  /* which should always be true.... */
			*ptr++ = '\0';
			strcpy(port,ptr);
		}
	} else {
		if ((ptr = strchr(txt,':')) != NULL) {
			*ptr = ' '; 
			sscanf(txt,"%s %s", address, port);
		} else {
			sscanf(txt,"%s",address);
			port[0] = '\0';
		}
	}

	p = pEditInfo->proxy_list;

	while (p != NULL) {
		if (strcmp(p->address, address) == 0) {
			if (fProxy == FALSE) {
				if ((port[0] == '\0') && (p->port == NULL))
					break;
				if (strcmp(p->port, port) == 0)
					break;
			} else if (strcmp(p->scheme, proxy) == 0) {
				   if (strcmp(p->port, port) == 0)
					break;
			}
		}
		p = p->next;
	}
					
	if (p == NULL)
		return NULL; /* whoops */
	return p;
}

void FillProxyText(struct EditInfo *p)
{
	ClearProxyText(p);
	if (p->IF->proxy_text)
		XmTextSetString(p->IF->proxy_text, p->editing->scheme);
	XmTextSetString(p->IF->addr_text, p->editing->address);

	if (p->editing->port)
		XmTextSetString(p->IF->port_text, p->editing->port);

	if (p->IF->alive!=NULL) {
		if (p->editing->alive)
			XmToggleButtonSetState(p->IF->alive, False, False);
		else
			XmToggleButtonSetState(p->IF->alive, True, False);
	}
}

void ClearProxyText(struct EditInfo *p)
{
	if (p->IF->proxy_text)
		XmTextSetString(p->IF->proxy_text, "");
	if (p->IF->addr_text)
		XmTextSetString(p->IF->addr_text, "");
	if (p->IF->port_text)
		XmTextSetString(p->IF->port_text, "");
	if (p->IF->alive)
		XmToggleButtonSetState(p->IF->alive, True, False);
} 

void CommitProxyInfo( Widget w, XtPointer client, XtPointer call)
{
	struct EditInfo *pEditInfo;
	struct Proxy *p;
	char *proxy, *addr, *port;
	pEditInfo = (struct EditInfo *)client;

	if (pEditInfo->IF->proxy_text) {
		proxy = XmTextGetString(pEditInfo->IF->proxy_text);
		if (proxy[0] == '\0') {

			XmxMakeErrorDialog(mo_main_window->base, COMMIT_PROXY_EMPTY_ERROR, "No Proxy Entered");
			XtManageChild (Xmx_w);
			return;
		}
	}

	addr = XmTextGetString(pEditInfo->IF->addr_text);
	if (addr[0] == '\0') {

		XmxMakeErrorDialog(mo_main_window->base, COMMIT_ADDR_EMPTY_ERROR, "No Address Entered");
		XtManageChild (Xmx_w);
		return;
	}

	/* Make sure it is all lowercase */
	{char *ptr; for (ptr=addr; ptr && *ptr; ptr++) *ptr=tolower(*ptr);}

	port = XmTextGetString(pEditInfo->IF->port_text);
	if (port[0] == '\0') {
		if (pEditInfo->fProxy) {
			XmxMakeErrorDialog(mo_main_window->base, COMMIT_PORT_EMPTY_ERROR, "No Port Entered");
			XtManageChild (Xmx_w);
			return;
		}
		XtFree(port);
		port = NULL;
	}

	if (pEditInfo->type == EDIT) {
		p = pEditInfo->editing;
	} else {
		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		pEditInfo->editing = p;
	}

	if (pEditInfo->fProxy) {
		if (p->scheme)
			p->scheme = (char *)realloc(p->scheme,strlen(proxy)+1);
		else
			p->scheme = (char *)calloc(1, strlen(proxy)+1);

		strcpy(p->scheme,proxy);
		XtFree(proxy);
	} else p->scheme = NULL;

	if (p->address)
		p->address = (char *)realloc(p->address,strlen(addr)+1);
	else
		p->address = (char *)calloc(1, strlen(addr)+1);

	strcpy(p->address, addr);
	XtFree(addr);

	if (port != NULL) {
		if (p->port)
			p->port = (char *)realloc(p->port, strlen(port)+1);
		else
			p->port = (char *)calloc(1, strlen(port)+1);
		strcpy(p->port, port);
		if (port)
			XtFree(port);
	} else {
		if (p->port)
			free(p->port);
		p->port = NULL;
	}

	if (pEditInfo->IF->alive!=NULL) {
		p->alive = (XmToggleButtonGetState(pEditInfo->IF->alive) == True) ? 0 : 1;
	}

	if ((p->transport == NULL) && (pEditInfo->fProxy)){
		p->transport = (char *)calloc(1, 5); /* strlen("http")+1 */
		strcpy(p->transport, "http");
	}

	if (pEditInfo->type == ADD) {
		AddProxyToList(pEditInfo, p);
		AppendProxy(pEditInfo, p);
	}
	ShowProxyList(pEditInfo);

	if (pEditInfo->fProxy)
		XtPopdown(EditProxyDialog);
	else
		XtPopdown(EditNoProxyDialog);
}

void DismissProxy( Widget w, XtPointer client, XtPointer call)
{
	Widget dialog = (Widget)client;

	XtPopdown(dialog);
}

void RemoveProxyInfo(Widget w, XtPointer client, XtPointer call)
{
	XmString selected_string;
	char *selected_text;
	struct EditInfo *pEditInfo = (struct EditInfo *)client;
	struct Proxy *pEditing;

	
	selected_string = GetStringFromScrolled(pEditInfo->scrolled);

	if (selected_string == NULL) {
		XmxMakeErrorDialog(mo_main_window->base, REMOVE_ERROR, "No Entry Selected");
		XtManageChild (Xmx_w);
		return;
	}
	XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, &selected_text);
	pEditing = FindProxyEntry(pEditInfo, selected_text);

	DeleteProxy(pEditInfo, pEditing);
	ShowProxyList(pEditInfo);
	XtFree(selected_text);
}

void WriteProxies(Widget w, XtPointer client, XtPointer call)
{
	struct Proxy *p;
	struct EditInfo *pEditInfo;
	FILE *fp;
	int flag;
	char msgbuf[256];

	pEditInfo = (struct EditInfo *)client;

	p = pEditInfo->proxy_list;

	if (pEditInfo->fProxy) {
                if ((fp = fopen(mMosaicProxyFileName,"w")) == NULL) {
                        sprintf(msgbuf,SAVE_ERROR, mMosaicProxyFileName);
			XmxMakeErrorDialog(mo_main_window->base, msgbuf, "Error writing file");
			XtManageChild (Xmx_w);
			return;
		}
	} else { 
                if ((fp = fopen(mMosaicNoProxyFileName,"w")) == NULL) {
                        sprintf(msgbuf,SAVE_ERROR, mMosaicNoProxyFileName);
			XmxMakeErrorDialog(mo_main_window->base, msgbuf, "Error writing file");
			XtManageChild (Xmx_w);
			return;
		}
	}

	while (p != NULL) {
		if (p->scheme)
			fprintf(fp,"%s ",p->scheme);
		if (p->address)
			fprintf(fp,"%s ",p->address);
		if (p->port)
			fprintf(fp,"%s ",p->port);
		if (p->transport)
			fprintf(fp,"%s ",p->transport);
		fprintf(fp,"\n");
		p = p->next;
	}
	if (pEditInfo->fProxy)
                sprintf(msgbuf,SAVED_AOK,mMosaicProxyFileName);
	else
                sprintf(msgbuf,SAVED_AOK,mMosaicNoProxyFileName);
	XmxMakeInfoDialog(mo_main_window->base, msgbuf, "File Saved");
	XtManageChild (Xmx_w);
	fclose(fp);
}

void DestroyDialog(Widget w, XtPointer client, XtPointer call)
{
	int *flag = (int *)client;

	*flag = 0;
}

void AppendProxy(struct EditInfo *pEditInfo, struct Proxy *p)
{
	struct Proxy *cur;

	cur = pEditInfo->proxy_list;

	p->next = NULL;
	p->prev = NULL;

	if (cur != NULL) {
		while (cur->next != NULL) 
			cur = cur->next;

		p->prev = cur;
		cur->next = p;
	} else {
		pEditInfo->proxy_list = p;
		if (pEditInfo->fProxy)
			proxy_list = p;
		else
			noproxy_list = p;
	}
}

void DeleteProxy(struct EditInfo *pEditInfo, struct Proxy *p)
{
	struct Proxy *cur;
	struct Proxy *pl;

	cur = p;

	if (pEditInfo->fProxy)
		pl = proxy_list;
	else
		pl = noproxy_list;
	/*
	** Delete proxy from list
	*/
	if (cur == pl) {
		pl = cur->next;
		if (pl == NULL) {
			pEditInfo->proxy_list = NULL;
			if (pEditInfo->fProxy)
				proxy_list = NULL;
			
			else
				noproxy_list = NULL;
		} else {
			pEditInfo->proxy_list = pl;
			if (pEditInfo->fProxy)
				proxy_list = pl;
			else
				noproxy_list = pl;
		}
	}

	if (cur->next != NULL)
		cur->next->prev = p->prev;

	if (cur->prev != NULL)
		cur->prev->next = p->next;

	/*
	** Delete allocated space from proxy entry
	*/
	if (p->scheme) {
		free(p->scheme);
		p->scheme = NULL;
	}

	if (p->address) {
		free(p->address);
		p->address = NULL;
	}

	if (p->port) {
		free(p->port);
		p->port = NULL;
	}
	if (p->transport) {
		free(p->transport);
		p->transport = NULL;
	}
	free(p);
}
