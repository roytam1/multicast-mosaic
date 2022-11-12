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
	Widget translation; 
	struct Proxy *editing;
	struct InfoFields *IF;
	struct ProxyDomain  *editingDomain;
	struct Proxy *proxy_list;
};


#define BUFLEN 256
#define BLANKS " \t\n"

struct ProxyDomain *AddProxyDomain();

char * mMosaicProxyFileName = NULL;
char * mMosaicNoProxyFileName = NULL;

static struct Proxy * noproxy_list =NULL;
static struct Proxy * proxy_list =NULL;
static struct ProxyDomain *pdList = NULL;

Widget ProxyDialog, EditProxyDialog, EditNoProxyDialog, EditProxyDomainDialog;

void AppendProxy(struct EditInfo *pEditInfo, struct Proxy *p);

struct InfoFields {
	Widget proxy_text;
	Widget addr_text;
	Widget port_text;
	Widget alive;
	Widget domain_text;
	Widget trans_menu;
};



void AddProxyToList(struct EditInfo *pEditInfo, struct Proxy *proxy);
void ShowProxyList(struct EditInfo *pEditInfo);
void EditProxyInfo( Widget w, XtPointer client, XtPointer call, int type);
void CommitProxyInfo( Widget w, XtPointer client, XtPointer call);
void DismissProxy( Widget w, XtPointer client, XtPointer call);
void ClearProxyText(struct EditInfo *p);
void FillProxyText(struct EditInfo *p);
void WriteProxies(Widget w, XtPointer client, XtPointer call);
void RemoveProxyInfo(Widget w, XtPointer client, XtPointer call, int type);
void EditProxyDomainInfo( Widget w, XtPointer client, XtPointer call, int type);
void ShowProxyDomainList(struct EditInfo *pEditInfo);
void CommitProxyDomainInfo(Widget w, XtPointer client, XtPointer call);
void CallEdit(Widget w, XtPointer client, XtPointer call);
void CallEditDomain(Widget w, XtPointer client, XtPointer call);
void CallAdd(Widget w, XtPointer client, XtPointer call) ;
void CallAddDomain(Widget w, XtPointer client, XtPointer call);
void CallRemoveProxy(Widget w, XtPointer client, XtPointer call);
void CallRemoveProxyDomain(Widget w, XtPointer client, XtPointer call);
void DestroyDialog(Widget w, XtPointer client, XtPointer call);
void PopProxyDialog(mo_window *win, struct Proxy *list, int fProxy);
void DeleteProxy(struct EditInfo *pEditInfo, struct Proxy *p);
void EditNoProxyInfo( Widget w, XtPointer client, XtPointer call, int type);
void CenterDialog(Widget dialog, XtPointer client, XtPointer call);
void ProxyHelpWindow(Widget w, XtPointer client, XtPointer call);
void HelpWindow(Widget w, XtPointer client, XtPointer call);


#ifdef OTHER_TRANSPORT
void	SetOptionMenuButtonLabel();
#endif

struct Proxy * FindProxyEntry(struct EditInfo *pEditInfo, char *txt);
struct ProxyDomain * FindProxyDomainEntry(struct ProxyDomain *pDomain,char *txt);

static mo_window *mo_main_window;

void ReadProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;
	struct ProxyDomain *pCurList, *pNewDomain;

	mMosaicProxyFileName = (char*) malloc(strlen (mmosaic_root_dir) +
                                strlen ("proxy") + 8);
	sprintf (mMosaicProxyFileName, "%s/proxy", mmosaic_root_dir);


	if ((fp = fopen(mMosaicProxyFileName,"r")) == NULL)
		return; 

	head = NULL;
	cur = NULL;

	/* read entries from the proxy list
	** These malloc()s should be checked for returning NULL
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

		if (strcmp(p->transport,"CCI") == 0)
			p->trans_val = TRANS_CCI;
		else
			p->trans_val = TRANS_HTTP;

		/* Read the domain */
		p->list = NULL;

		if ((psb = strtok(NULL, BLANKS)) != NULL) {

			p->list = NULL;
			AddProxyDomain(psb, &p->list);

			pCurList = p->list;

			while ((psb = strtok(NULL, BLANKS)) != NULL) {
				if (psb[0] == '\\') {
					if (fgets(buf, BUFLEN, fp) == 0) {
						proxy_list = head;
						return ;
					}
					psb = strtok(buf, BLANKS);
					if (psb == NULL){
						proxy_list = head;
						return ;
					}
				}
				if (AddProxyDomain(psb, &pCurList) == NULL) {
					proxy_list = head;
					return ;
				}
			}
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
	proxy_list = head;
}

void ReadNoProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;
	extern void FreeProxy();
		
	mMosaicNoProxyFileName = (char*) malloc(strlen (mmosaic_root_dir) +
                                strlen ("noproxy") + 8);
	sprintf (mMosaicNoProxyFileName, "%s/noproxy", mmosaic_root_dir);
	if ((fp = fopen(mMosaicNoProxyFileName,"r")) == NULL)
		return ;

	head = NULL;
	cur = NULL;

	/* read entries from the proxy list
	** These malloc()s should be checked for returning NULL
	*/
	while (fgets(buf, BUFLEN, fp) != 0) {

		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		
		p->next = NULL;
		p->prev = NULL;

/* The proxy protocol, transport, and list are all null for no proxy. */
		p->scheme = NULL;
		p->transport = NULL;
		p->list = NULL;

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

struct ProxyDomain *
AddProxyDomain(char *sbDomain, struct ProxyDomain **pdList)
{
	struct ProxyDomain *pNewDomain;

	pNewDomain = (struct ProxyDomain *)malloc(sizeof(struct ProxyDomain));
	if (pNewDomain == NULL)
		return NULL;

	pNewDomain->domain = (char *)malloc(strlen(sbDomain)+1);
	strcpy(pNewDomain->domain, sbDomain);
	if (*pdList == NULL) {
		*pdList = pNewDomain;
		(*pdList)->next = NULL;
		(*pdList)->prev = NULL;
	} else {
		struct ProxyDomain *p;

		p = *pdList;
		while (p->next != NULL)
			p = p->next;
		pNewDomain->prev = p;
		pNewDomain->next = NULL;
		p->next = pNewDomain;
	}
	return pNewDomain;
}

void DeleteProxyDomain(struct ProxyDomain *p)
{
	struct ProxyDomain *cur;

	cur = p;
	if (cur->next !=NULL)
		cur->next->prev =  p->prev;
	if (cur->prev !=NULL)
		cur->prev->next =  p->next;
	if (p->domain) {
		free(p->domain);
		p->domain = NULL;
	}
	free(p);
	p = NULL;
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

	/*
	** Try and get a nice non-proportional font.  If we can't get 
	** it, then the heck with it, just use the default.
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

	/*
	** Create action area widgets
	*/
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
        XtAddCallback(remove, XmNactivateCallback, CallRemoveProxy, pEditInfo);
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

	mo_open_another_window(mo_main_window, mo_assemble_help_url(html_file),
			NULL, NULL);
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

	if (pdList != NULL) {
		p = pdList; 
		while (p != NULL) {
			next = p->next;
			DeleteProxyDomain(p);
			p = next;
		}
		pdList = NULL; 
	}

	if (pEditInfo->fProxy)
		EditProxyInfo(w, client, call, ADD);
	else
		EditNoProxyInfo(w, client, call, ADD);
}

void CallRemoveProxy(Widget w, XtPointer client, XtPointer call)
{
	RemoveProxyInfo(w, client, call, PROXY);
}

void CallRemoveProxyDomain(Widget w, XtPointer client, XtPointer call)
{
	RemoveProxyInfo(w, client, call, PROXY_DOMAIN);
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
	pEditInfo->IF->domain_text = NULL;
	pEditInfo->IF->trans_menu = NULL;
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

#ifdef OTHER_TRANSPORT
	int trans_val;
#endif
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
#ifdef OTHER_TRANSPORT
			SetOptionMenuButtonLabel(pEditInfo->IF->trans_menu, pEditInfo->editing->transport);
#endif
		} else {
			ClearProxyText(pEditInfo);

#ifdef OTHER_TRANSPORT
			SetOptionMenuButtonLabel(pEditInfo->IF->trans_menu, "http");
#endif
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

#ifdef OTHER_TRANSPORT
	trans_string = XmStringCreateSimple("Transport Method");
	http_string = XmStringCreateSimple("http");
	cci_string = XmStringCreateSimple("cci");

	/*
	** If we're editing, start option menu with the value specified
	** otherwise, we're adding, so default to TRANS_HTTP.
	*/

	if (type == EDIT)
		trans_val = pEditInfo->editing->trans_val;
	else
		trans_val = TRANS_HTTP;

	pEditInfo->IF->trans_menu = XmVaCreateSimpleOptionMenu(rc2, "trans_menu",
		trans_string, 'T', trans_val, NULL,
		XmVaPUSHBUTTON, http_string, 'H', NULL, NULL,
		XmVaPUSHBUTTON, cci_string, 'C', NULL, NULL,
		NULL);


	XmStringFree(trans_string);
	XmStringFree(http_string);
	XmStringFree(cci_string);
		

	XtManageChild(pEditInfo->IF->trans_menu);
#endif
	XtManageChild(rc2);
	XtManageChild(text_form);

	trans_rc = XtVaCreateWidget("trans_rc",
		xmRowColumnWidgetClass, rc,
		XmNorientation, XmVERTICAL, 
		NULL);

	label = XtVaCreateManagedWidget("Scheme Info",
		xmLabelWidgetClass, trans_rc,
		NULL);
	
	pEditInfo->translation = XmCreateScrolledList(trans_rc, "trans_info", NULL, 0);

	XtVaSetValues(pEditInfo->translation,
		XmNwidth,	150,
		XmNvisibleItemCount,	3,
		XmNmargin,		1,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNwidth,	150,
		XmNvisibleItemCount,	5,
		XmNmargin,		1,
		NULL);

	XtManageChild(pEditInfo->translation);

	rc3 = XtVaCreateWidget("rowcolumn3",
		xmRowColumnWidgetClass, trans_rc,
		XmNtopAttachment, XmATTACH_WIDGET,
		XmNtopWidget,	pEditInfo->translation,
		XmNorientation,	XmHORIZONTAL,
		NULL);

	add = XtVaCreateManagedWidget("Add",
		xmPushButtonWidgetClass, rc3,
		NULL);
		
	remove = XtVaCreateManagedWidget("Remove",
		xmPushButtonWidgetClass, rc3,
		NULL);
		
	edit = XtVaCreateManagedWidget("Edit",
		xmPushButtonWidgetClass, rc3,
		NULL);

	XtManageChild(rc3);
	XtManageChild(trans_rc);
	XtManageChild(rc);
	XtManageChild(main_form);

        XtAddCallback(edit, XmNactivateCallback, CallEditDomain, pEditInfo);
	XtAddCallback(pEditInfo->translation, XmNdefaultActionCallback, CallEditDomain, pEditInfo);

	XtAddCallback(add, XmNactivateCallback, CallAddDomain, pEditInfo);
	XtAddCallback(remove, XmNactivateCallback, CallRemoveProxyDomain, pEditInfo);

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

void CallEditDomain(Widget w, XtPointer client, XtPointer call)
{
	EditProxyDomainInfo(w, client, call, EDIT);
}

void CallAddDomain(Widget w, XtPointer client, XtPointer call)
{
	EditProxyDomainInfo(w, client, call, ADD);
}

void EditProxyDomainInfo( Widget w, XtPointer client, XtPointer call, int type)
{
	Widget main_form, action_area, sep, dismiss, commit, help;
	Widget rc, form, domain;

	XmString selected_string;
	char *selected_text;

	struct EditInfo *pEditInfo;

	static int fEditProxyDomainDialog = 0;

	/*
	** We obtain information from the client pointer, rather than getting
	** it from call->item because this routine can be called from
	** a pushbutton as well as from double clicking the list.
	*/

	pEditInfo = (struct EditInfo *)client;
	pEditInfo->domaintype = type;

	if (type == EDIT) {
		selected_string = GetStringFromScrolled((Widget)pEditInfo->translation);

		if (selected_string == NULL) {

			XmxMakeErrorDialog(mo_main_window->base, EDIT_ERROR, "No Entry Selected");
			XtManageChild (Xmx_w);
			return;
		}

		XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, 
					&selected_text);

		if (pdList)
			pEditInfo->editingDomain = FindProxyDomainEntry(pdList, selected_text);
		else
			pEditInfo->editingDomain = FindProxyDomainEntry(pEditInfo->editing->list, selected_text);

		XtFree(selected_text);
	}

	if (fEditProxyDomainDialog) {
		if (type == EDIT)
			XmTextSetString(pEditInfo->IF->domain_text, pEditInfo->editingDomain->domain);
		else
			XmTextSetString(pEditInfo->IF->domain_text, "");

		XtPopup(EditProxyDomainDialog, XtGrabNone);
		return;
	}

	EditProxyDomainDialog = XtVaCreatePopupShell("Proxy Domain",
		xmDialogShellWidgetClass, XtParent(w),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"Proxy Domain Information",
		NULL);

	fEditProxyDomainDialog = 1;

	main_form = XtVaCreateWidget("edit_form",
		xmFormWidgetClass, EditProxyDomainDialog,
		NULL);
	/*
	** Create action area widgets
	*/
	action_area = XtVaCreateWidget("action",
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

	rc = XtVaCreateWidget("rowcolumn",
		xmRowColumnWidgetClass, main_form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_FORM,
		XmNrightAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_WIDGET,
		XmNbottomWidget, action_area,
		NULL);

	form = XtVaCreateWidget("form1",
		xmFormWidgetClass, rc,
		XmNfractionBase, 10,
		NULL);

	domain = XtVaCreateManagedWidget("Scheme Info",
		xmLabelWidgetClass, form,
		XmNtopAttachment, XmATTACH_FORM,
		XmNbottomAttachment, XmATTACH_FORM,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 0,
		XmNalignment, XmALIGNMENT_END,
		NULL);

	pEditInfo->IF->domain_text= XtVaCreateManagedWidget("domain_text",
		xmTextFieldWidgetClass, form,
		XmNleftAttachment, XmATTACH_POSITION,
		XmNleftPosition, 4,
		XmNrightAttachment, XmATTACH_FORM,
		NULL);

	if (type == EDIT)
		XmTextSetString(pEditInfo->IF->domain_text, pEditInfo->editingDomain->domain);
	else
		XmTextSetString(pEditInfo->IF->domain_text, "");

	XtManageChild(form);
	XtManageChild(rc);

	XtManageChild(action_area);
	XtManageChild(main_form);

        XtAddCallback(dismiss, XmNactivateCallback, DismissProxy, EditProxyDomainDialog);
	XtAddCallback(commit, XmNactivateCallback, CommitProxyDomainInfo, pEditInfo);
	XtAddCallback(EditProxyDomainDialog, XmNdestroyCallback, DestroyDialog, &fEditProxyDomainDialog);
	XtAddCallback(EditProxyDomainDialog, XmNpopupCallback, CenterDialog, NULL);
 
        XtAddCallback(help, XmNactivateCallback, HelpWindow, "help-proxy-domain-edit.html");

	XtPopup(EditProxyDomainDialog, XtGrabNone);
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


void ClearTempBongedProxies() {

struct Proxy *p = proxy_list;

	while (p!=NULL) {
		if (p->alive==2) {
			p->alive=0;
		}
		p=p->next;
	}

	return;
}


struct Proxy * GetProxy(char *proxy, char *access, int fMatchEnd)
{
	struct Proxy *p = proxy_list;
	struct ProxyDomain *pd;

	if ((access == NULL) || (proxy == NULL))
		return NULL;

	while (p != NULL) {

		if (strcmp(p->scheme, proxy) != 0 || p->alive) {
			p = p->next;
			continue;
		}

		/* found a matching proxy */

		/*
		** If the access list is empty, that's a match on
		** everything.  Bale out here.
		*/
		if (p->list == NULL)
			return p;	
		
		pd = p->list;
		
		while (pd != NULL) {
			char *ptr;

			ptr = strstr(access, pd->domain);

			if (ptr) {
				if (fMatchEnd) {
					/* at the end? */
					if (strlen(ptr) == strlen(pd->domain)) 
						break;
				} else {
					/* at beginning? */
					if (ptr == access) 
						break;
				}
			}
			pd = pd->next;
		}

		if (pd == NULL) {
			p = p->next;
			continue; /* We didn't match... look for another */
		}

		return p; /* we found a match on access and proxy */
	}
	return NULL;
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

struct ProxyDomain * FindProxyDomainEntry(struct ProxyDomain *pDomain, char *txt)
{
	struct ProxyDomain *p;

	p = pDomain;
	while (p != NULL) {
		if (strcmp(p->domain,txt))
			return p;
		p = p->next;
	}
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

	if (p->editing->list == NULL) 
		return;
	ShowProxyDomainList(p);
}

void ShowProxyDomainList(struct EditInfo *pEditInfo)
{

	XmString string;
	struct ProxyDomain *p;

	XmListDeleteAllItems(pEditInfo->translation);

	/*
	** Fill in the translation domain list
	*/

	p = NULL;
	if (pdList != NULL)
		p = pdList;
	else if (pEditInfo->editing != NULL)
		p = pEditInfo->editing->list;
	while (p != NULL) {
		if (p->domain) {
			string = XmStringCreateSimple(p->domain);

			XmListAddItem(pEditInfo->translation, string, 0);
			XmStringFree(string);
		}
		p = p->next;
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
	if (p->translation)
		XmListDeleteAllItems(p->translation);
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

#ifdef OTHER_TRANSPORT
	label = XmOptionButtonGadget(pEditInfo->IF->trans_menu);
	XtVaGetValues(label,
		XmNlabelString, &label_string,
		NULL);

	XmStringGetLtoR(label_string, XmSTRING_DEFAULT_CHARSET, &trans);
	if (p->transport)
		p->transport = (char *)realloc(p->transport, strlen(trans)+1);
	else
		p->transport = (char *)calloc(1, strlen(trans)+1);
	strcpy(p->transport, trans);
	XtFree(trans);
#else
	if ((p->transport == NULL) && (pEditInfo->fProxy)){
		p->transport = (char *)calloc(1, 5); /* strlen("http")+1 */
		strcpy(p->transport, "http");
	}
#endif

	if (pEditInfo->type == ADD) {
		AddProxyToList(pEditInfo, p);
		AppendProxy(pEditInfo, p);
	}
	if (pdList != NULL) {
		pEditInfo->editing->list = pdList;
		pdList = NULL;
	}
	ShowProxyList(pEditInfo);

	if (pEditInfo->fProxy)
		XtPopdown(EditProxyDialog);
	else
		XtPopdown(EditNoProxyDialog);
}

void CommitProxyDomainInfo(Widget w, XtPointer client, XtPointer call)
{
	char *domain;
	struct EditInfo *p = (struct EditInfo *)client;

	domain = XmTextGetString(p->IF->domain_text);
	if (domain[0] == '\0') {
		
		XmxMakeErrorDialog(mo_main_window->base, COMMIT_DOMAIN_EMPTY_ERROR, "No Entry Selected");
		
		XtManageChild (Xmx_w);
		return;
	}

	/* Make sure it is all lowercase */
	{char *ptr; for (ptr=domain; ptr && *ptr; ptr++) *ptr=tolower(*ptr);}

	if (p->domaintype == ADD) {
		struct ProxyDomain *pd;

		if (pdList == NULL) {
		/*
		** Temporary list is null
		*/
			if (p->editing == NULL) /* scheme being used yet? */
				pd = NULL; /* no */
			else
				pd = p->editing->list; /* yes! use it */
		} else
			pd = pdList;
		if (pd == NULL) { /* this will be the first thing in list */
			AddProxyDomain(domain, &pd);
			if (p->editing)
				p->editing->list = pd;
			else
				pdList = pd;
		} else {
			while (pd->next != NULL)
				pd = pd->next;
			AddProxyDomain(domain, &pd);
		}
	} else {
		p->editingDomain->domain = 
                    (char *)realloc(p->editingDomain->domain,strlen(domain)+1);
		strcpy(p->editingDomain->domain, domain);
	}
	
	ShowProxyDomainList(p);
	XtPopdown(EditProxyDomainDialog);
	return;
}

void DismissProxy( Widget w, XtPointer client, XtPointer call)
{
	Widget dialog = (Widget)client;

	XtPopdown(dialog);
}

#ifdef OTHER_TRANSPORT
void SetOptionMenuButtonLabel( Widget w, char *s)
{
	Widget label;
	XmString label_string;


	label = XmOptionButtonGadget(w);

	label_string = XmStringCreateSimple(s);

	XtVaSetValues(label,
		XmNlabelString, label_string,
		NULL);

	XmStringFree(label_string);

}
#endif

void RemoveProxyInfo(Widget w, XtPointer client, XtPointer call, int type)
{
	XmString selected_string;
	char *selected_text;
	struct EditInfo *pEditInfo = (struct EditInfo *)client;
	
	if (type == PROXY)
		selected_string = GetStringFromScrolled(pEditInfo->scrolled);
	else
		selected_string = GetStringFromScrolled(pEditInfo->translation);

	if (selected_string == NULL) {
		XmxMakeErrorDialog(mo_main_window->base, REMOVE_ERROR, "No Entry Selected");
		XtManageChild (Xmx_w);
		return;
	}

	XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, &selected_text);
	if (type == PROXY) {
		struct Proxy *pEditing = FindProxyEntry(pEditInfo, selected_text);


		DeleteProxy(pEditInfo, pEditing);
		ShowProxyList(pEditInfo);
	} else { /* PROXY_DOMAIN */
		struct ProxyDomain *pdEntry;

		if (pdList != NULL)
 			pdEntry = FindProxyDomainEntry(pdList, selected_text);
		else
 			pdEntry = FindProxyDomainEntry(pEditInfo->editing->list, selected_text);
		
		if (pdList != NULL) {
			if (pdEntry == pdList)
				pdList = pdEntry->next;
		} else {
			if (pdEntry == pEditInfo->editing->list)
				pEditInfo->editing->list = pdEntry->next;
		}
		DeleteProxyDomain(pdEntry);
		ShowProxyDomainList(pEditInfo);
	}

	XtFree(selected_text);
}

void WriteProxies(Widget w, XtPointer client, XtPointer call)
{
	struct Proxy *p;
	struct ProxyDomain *pd;
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
		
		pd = p->list;
		if (pd != NULL) {
			flag = 0;
			while (pd != NULL) {
				if (flag)
					fprintf(fp,"\\\n");
				else
					flag = 1;
				fprintf(fp, "%s ",pd->domain);
				pd = pd->next;
			}
		}
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
