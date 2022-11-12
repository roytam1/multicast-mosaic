/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

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
#include <assert.h>

#include "URLParse.h"
#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "gui.h"
#include "proxy.h"

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
	int type;  /* Is this an Edit or an Add ? */
	int domaintype;  /* Is this an Edit or an Add ? */
	Widget scrolled; 
	struct Proxy *editing;
	struct InfoFields *IF;
};

struct InfoFields {
	Widget proxy_text;
	Widget addr_text;
	Widget port_text;
	Widget alive;
};


#define BUFLEN 256
#define BLANKS " \t\n"

static char * mMosaicProxyFileName = NULL;
static char * mMosaicNoProxyFileName = NULL;

static struct Proxy * Proxy_List =NULL;
static char * Proxy_Help_File_Name = "help-proxylist.html";
static struct EditInfo *Proxy_Edit_Info=NULL;

static struct Proxy * Noproxy_List =NULL;
static char * Noproxy_Help_File_Name = "help-noproxylist.html";
static struct EditInfo *Noproxy_Edit_Info=NULL;

static Widget ProxyDialog = NULL;
static Widget NoproxyDialog = NULL;
static Widget EditProxyDialog= NULL;
static Widget EditNoproxyDialog = NULL;

static char senv[500];
static struct Proxy *http_proxy_env = NULL;
static struct Proxy *ftp_proxy_env = NULL;
static struct Proxy *gopher_proxy_env = NULL;
static struct Proxy *wais_proxy_env = NULL;

static void CommitNoproxyInfo( Widget w, XtPointer client, XtPointer call);

/* say if noproxy exist */
int GetNoProxy(char *access, char *hap)
{
	struct Proxy *p = Noproxy_List;
	char * hname;
	char *port;

	if ((access == NULL) || (hap == NULL) || (Noproxy_List == NULL))
		return 0;

	hname = strdup(hap);

	if ((port = strchr(hname,':')) != NULL) {
		*port = '\0';
	}

	while (p != NULL) {
		if (strstr(hname,p->address)) {
			free(hname);
			return 1;
		}
		p = p->next;
	}
	free(hname);
	return 0;
}

struct Proxy * GetProxy(char *acc)
{
	struct Proxy *p = Proxy_List;
	char * port;
	char * renv;
	char *hap;

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

	if (!strcmp(acc,"http")) {
		if (http_proxy_env)
			return http_proxy_env;
		http_proxy_env = (struct Proxy *) calloc(1,sizeof(struct Proxy ));
		hap = URLParse(renv, "", PARSE_HOST);
		if (port=strchr(hap, ':')) {
			*port++ = 0;
			http_proxy_env->port = port;
		} else {
			http_proxy_env->port = "80";
		}
		http_proxy_env->address = hap;
		return http_proxy_env;
	}
		
	if (!strcmp(acc,"gopher")) {
		if (gopher_proxy_env)
			return gopher_proxy_env;
		gopher_proxy_env = (struct Proxy *) calloc(1,sizeof(struct Proxy ));
		hap = URLParse(renv, "", PARSE_HOST);
		if (port=strchr(hap, ':')) {
			*port++ = 0;
			gopher_proxy_env->port = port;
		} else {
			gopher_proxy_env->port = "70";
		}
		gopher_proxy_env->address = hap;
		return gopher_proxy_env;
	}
	if (!strcmp(acc,"ftp")) { 
		if (ftp_proxy_env)
			return gopher_proxy_env;
		ftp_proxy_env = (struct Proxy *) calloc(1,sizeof(struct Proxy ));
		hap = URLParse(renv, "", PARSE_HOST);
		if (port=strchr(hap, ':')) {
			*port++ = 0;
			ftp_proxy_env->port = port;
		} else {
			ftp_proxy_env->port = "21";
		}
		ftp_proxy_env->address = hap;
		return ftp_proxy_env;
	}
	if (!strcmp(acc,"wais")) {
		if (wais_proxy_env)
			return wais_proxy_env;
		wais_proxy_env = (struct Proxy *) calloc(1,sizeof(struct Proxy ));
		hap = URLParse(renv, "", PARSE_HOST);
		if (port=strchr(hap, ':')) {
			*port++ = 0;
			wais_proxy_env->port = port;
		} else {
			wais_proxy_env->port = "210";
		}
		wais_proxy_env->address = hap;
		return wais_proxy_env;
	}
	return NULL;
}

void ReadProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur,*p;

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
			Proxy_List = head;
			return ;
		}
		p->scheme = (char *)malloc(strlen(psb)+1);
		strcpy(p->scheme, psb);

/* Read the proxy address */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			Proxy_List = head;
			return ;
		}
		p->address = (char *)malloc(strlen(psb)+1);
		strcpy(p->address, psb);

/* Read the proxy port */
		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			Proxy_List = head;
			return ;
		}
		p->port = (char *)malloc(strlen(psb)+1);
		strcpy(p->port, psb);

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
	Proxy_List = head;
}

void ReadNoProxies(char *mmosaic_root_dir)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *p;

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

/* The proxy protocol, and list are all null for no proxy. */
		p->scheme = NULL;

/* Read the proxy address */
		if ((psb = strtok(buf, BLANKS)) == NULL) {
			Noproxy_List = head;
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
	Noproxy_List = head;
}

/* ################### gui section ###################### */

/* ################### generic #################*/

static XmString GetStringFromScrolled(Widget w)
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

static void CallDismiss( Widget w, XtPointer client, XtPointer call)
{
	Widget dialog = (Widget)client;

	XtPopdown(dialog);
}

static void CallHelp(Widget w, XtPointer client, XtPointer call)
{
	char *html_file = (char*)client;
	mo_open_another_window(NULL, mo_assemble_help_url(html_file));
}

/* ############## specific ############## */

static void ClearProxyText(struct EditInfo *p)
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

static void FillProxyText(struct EditInfo *p)
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

static struct Proxy * FindProxyEntry(char *txt)
{
	struct Proxy *p;
	char proxy[30], address[50], port[8], *ptr;

	sscanf(txt,"%s %s",proxy,address); 
	ptr = strchr(address,':');
	if (ptr) {  /* which should always be true.... */
		*ptr++ = '\0';
		strcpy(port,ptr);
	}

	p = Proxy_List;

	while (p != NULL) {
		if (strcmp(p->address, address) == 0) {
			if (strcmp(p->scheme, proxy) == 0) {
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

static void AddProxyToList(struct Proxy *proxy)
{
	char *p;
	XmString string;
	Widget scrolled = Proxy_Edit_Info->scrolled;

	if ((p = (char *)malloc(256*sizeof(char))) == NULL)
		return;

	sprintf(p,"%-12.12s %s:%s",proxy->scheme, proxy->address, proxy->port);
	string = XmStringCreateSimple(p);

	XmListAddItem(scrolled, string, 0);
	free(p);
	XmStringFree(string);
}

static void ShowProxyList()
{
	Widget scrolled;
	struct Proxy *proxy;

	scrolled = Proxy_Edit_Info->scrolled;

	proxy = Proxy_List; 

	XmListDeleteAllItems(Proxy_Edit_Info->scrolled);

	while (proxy != NULL) {
		AddProxyToList(proxy);
		proxy = proxy->next;
	}
}

static void AppendProxy(struct Proxy *p)
{
	struct Proxy *cur;

	cur = Proxy_List;

	p->next = NULL;
	p->prev = NULL;

	if (cur != NULL) {
		while (cur->next != NULL) 
			cur = cur->next;
		p->prev = cur;
		cur->next = p;
	} else {
		Proxy_List = p;
	}
}


static void CommitProxyInfo( Widget w, XtPointer client, XtPointer call)
{
	struct Proxy *p;
	char *proxy=NULL, *addr, *port;

	if (Proxy_Edit_Info->IF->proxy_text) {
		proxy = XmTextGetString(Proxy_Edit_Info->IF->proxy_text);
		if (proxy[0] == '\0') {

			XmxMakeErrorDialog(mMosaicToplevelWidget,
				COMMIT_PROXY_EMPTY_ERROR, "No Proxy Entered");
			XtManageChild (Xmx_w);
			return;
		}
	}

	addr = XmTextGetString(Proxy_Edit_Info->IF->addr_text);
	if (addr[0] == '\0') {

		XmxMakeErrorDialog(mMosaicToplevelWidget,
			COMMIT_ADDR_EMPTY_ERROR, "No Address Entered");
		XtManageChild (Xmx_w);
		return;
	}

	/* Make sure it is all lowercase */
	{char *ptr; for (ptr=addr; ptr && *ptr; ptr++) *ptr=tolower(*ptr);}

	port = XmTextGetString(Proxy_Edit_Info->IF->port_text);
	if (port[0] == '\0') {
		XmxMakeErrorDialog(mMosaicToplevelWidget,
			COMMIT_PORT_EMPTY_ERROR, "No Port Entered");
		XtManageChild (Xmx_w);
		return;
	}

	if (Proxy_Edit_Info->type == EDIT) {
		p = Proxy_Edit_Info->editing;
	} else {
		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		Proxy_Edit_Info->editing = p;
	}

	if (p->scheme)
		p->scheme = (char *)realloc(p->scheme,strlen(proxy)+1);
	else
		p->scheme = (char *)calloc(1, strlen(proxy)+1);

	strcpy(p->scheme,proxy);
	XtFree(proxy);

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

	if (Proxy_Edit_Info->IF->alive!=NULL) {
		p->alive = (XmToggleButtonGetState(Proxy_Edit_Info->IF->alive) == True) ? 0 : 1;
	}

	if (Proxy_Edit_Info->type == ADD) {
		AddProxyToList(p);
		AppendProxy(p);
	}
	ShowProxyList();
	XtPopdown(EditProxyDialog);
}

static void EditProxyInfo( Widget w, XtPointer client, XtPointer call, int type)
{
	Widget text_form, form, protocol, address, port;
	Widget label;
	Widget main_form, rc, rc2;
	Widget action_area, sep, dismiss, help;
	Widget commit;
	XmString selected_string;
	char *selected_text;

/* We obtain information from the client pointer, rather than getting
** it from call->item because this routine can be called from
** a pushbutton as well as from double clicking the list.
*/

	Proxy_Edit_Info->type = type;
	if (type == EDIT) {
		selected_string = GetStringFromScrolled((Widget)Proxy_Edit_Info->scrolled);

		if (selected_string == NULL) {
			XmxMakeErrorDialog(mMosaicToplevelWidget, EDIT_ERROR, "No Entry Selected");
			XtManageChild (Xmx_w);
			return;
		}

		XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, 
					&selected_text);

		Proxy_Edit_Info->editing = FindProxyEntry(selected_text);

		XtFree(selected_text);

		if (Proxy_Edit_Info->editing == NULL) {
			assert(0);
			return; /* how did *that* happen? */
		}
	} else {
		Proxy_Edit_Info->editing = NULL;
	}

	if (EditProxyDialog) {

		if (type == EDIT) {
			FillProxyText(Proxy_Edit_Info);
		} else {
			ClearProxyText(Proxy_Edit_Info);

		}
		XtPopup(EditProxyDialog, XtGrabNone);
		return;
	}
	EditProxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, XtParent(w),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"Proxy Information",
		NULL);

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

	Proxy_Edit_Info->IF = (struct InfoFields *)calloc(1,sizeof(struct InfoFields));

	Proxy_Edit_Info->IF->proxy_text= XtVaCreateManagedWidget("proxy_text",
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

	Proxy_Edit_Info->IF->addr_text = XtVaCreateManagedWidget("addr_text",
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

	Proxy_Edit_Info->IF->port_text = XtVaCreateManagedWidget("port_text",
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

	Proxy_Edit_Info->IF->alive = XtVaCreateManagedWidget("alive",
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

        XtAddCallback(commit, XmNactivateCallback, CommitProxyInfo, Proxy_Edit_Info);
        XtAddCallback(dismiss, XmNactivateCallback, CallDismiss, EditProxyDialog);

	XtAddCallback(help, XmNactivateCallback, CallHelp, "help-proxy-edit.html");

	if (type == EDIT)
		FillProxyText(Proxy_Edit_Info);
	else
		ClearProxyText(Proxy_Edit_Info);

	XtPopup(EditProxyDialog, XtGrabNone);
}

static void CallEditProxy(Widget w, XtPointer client, XtPointer call)
{
	EditProxyInfo(w, client, call, EDIT);
}

static void CallAddProxy(Widget w, XtPointer client, XtPointer call)
{
	EditProxyInfo(w, client, call, ADD);
}

static void DeleteProxy(struct Proxy *p)
{
	struct Proxy *cur;
	struct Proxy *pl;

	cur = p;
	pl = Proxy_List;

	if (cur == pl) {
		pl = cur->next;
		if (pl == NULL) {
			Proxy_List = NULL;
		} else {
			Proxy_List = pl;
		}
	}

	if (cur->next != NULL)
		cur->next->prev = p->prev;

	if (cur->prev != NULL)
		cur->prev->next = p->next;

/* Delete allocated space from proxy entry */
	if (p->scheme) {
		free(p->scheme);
	}
	if (p->address) {
		free(p->address);
	}
	if (p->port) {
		free(p->port);
	}
	free(p);
}

static void CallRemoveProxy(Widget w, XtPointer client, XtPointer call)
{
	XmString selected_string;
	char *selected_text;
	struct Proxy *pEditing;
	
	selected_string = GetStringFromScrolled(Proxy_Edit_Info->scrolled);

	if (selected_string == NULL) {
		XmxMakeErrorDialog(mMosaicToplevelWidget, REMOVE_ERROR, "No Entry Selected");
		XtManageChild (Xmx_w);
		return;
	}
	XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, &selected_text);
	pEditing = FindProxyEntry(selected_text);

	DeleteProxy(pEditing);
	ShowProxyList();
	XtFree(selected_text);
}

static void CallWriteProxies(Widget w, XtPointer client, XtPointer call)
{
	struct Proxy *p;
	FILE *fp;
	char msgbuf[1000];

	p = Proxy_List;

        if ((fp = fopen(mMosaicProxyFileName,"w")) == NULL) {
                sprintf(msgbuf,SAVE_ERROR, mMosaicProxyFileName);
		XmxMakeErrorDialog(mMosaicToplevelWidget, msgbuf, "Error writing file");
		XtManageChild (Xmx_w);
		return;
	}

	while (p != NULL) {
		if (p->scheme)
			fprintf(fp,"%s ",p->scheme);
		if (p->address)
			fprintf(fp,"%s ",p->address);
		if (p->port)
			fprintf(fp,"%s ",p->port);
		fprintf(fp,"\n");
		p = p->next;
	}
        sprintf(msgbuf,SAVED_AOK,mMosaicProxyFileName);
	XmxMakeInfoDialog(mMosaicToplevelWidget, msgbuf, "File Saved");
	XtManageChild (Xmx_w);
	fclose(fp);
}

/* ################################################# */

void PopProxyDialog()
{
	Widget main_form, action_area;
	Widget add, edit, remove, dismiss, help;
	Widget save;
	Widget scrolled;
	int  n;
	Arg args[20];

	XFontStruct *font;
	XmFontList *fontlist=NULL;

	if (ProxyDialog) {
		ShowProxyList();
		XtPopup(ProxyDialog, XtGrabNone);
		return;
	}

/* Try and get a nice non-proportional font.  If we can't get 
 * it, then the heck with it, just use the default.
*/
	font = XLoadQueryFont(mMosaicDisplay, FONTNAME);
	if (font == NULL) {
		font = XLoadQueryFont(mMosaicDisplay, "fixed");
	}
	if (font != NULL) {
		fontlist = (XmFontList *)XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
	}

	ProxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, mMosaicToplevelWidget,
		XmNdeleteResponse, XmUNMAP,
		XmNtitle,	"Proxies",
		NULL);

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

	Proxy_Edit_Info = (struct EditInfo *)calloc(1,sizeof(struct EditInfo));
	Proxy_Edit_Info->scrolled = scrolled;

        XtAddCallback(edit, XmNactivateCallback, CallEditProxy, Proxy_Edit_Info);
	XtAddCallback(scrolled, XmNdefaultActionCallback, CallEditProxy, Proxy_Edit_Info);
	XtAddCallback(add, XmNactivateCallback, CallAddProxy, Proxy_Edit_Info);
        XtAddCallback(remove, XmNactivateCallback, CallRemoveProxy, Proxy_Edit_Info);
        XtAddCallback(dismiss, XmNactivateCallback, CallDismiss, ProxyDialog);
	XtAddCallback(save, XmNactivateCallback, CallWriteProxies, Proxy_Edit_Info);
	XtAddCallback(help, XmNactivateCallback, CallHelp, Proxy_Help_File_Name);
	XtManageChild(main_form);
	ShowProxyList();
	XtPopup(ProxyDialog, XtGrabNone);
}

/* ############# NoProxy ############# */

static void AddNoproxyToList(struct Proxy *proxy)
{
	char *p;
	XmString string;
	Widget scrolled = Noproxy_Edit_Info->scrolled;

	p = strdup(proxy->address);

	string = XmStringCreateSimple(p);
	XmListAddItem(scrolled, string, 0);
	free(p);
	XmStringFree(string);
}

static void ShowNoproxyList()
{
	Widget scrolled;
	struct Proxy *proxy;

	scrolled = Noproxy_Edit_Info->scrolled;
	proxy = Noproxy_List;
	XmListDeleteAllItems(Noproxy_Edit_Info->scrolled);
	while (proxy != NULL) {
		AddNoproxyToList(proxy);
		proxy = proxy->next;
	}
}

static void AppendNoproxy(struct Proxy *p)
{
	struct Proxy *cur;

	cur = Noproxy_List;
	p->next = NULL;
	p->prev = NULL;
	if (cur != NULL) {
		while (cur->next != NULL) 
			cur = cur->next;
		p->prev = cur;
		cur->next = p;
	} else {
		Noproxy_List = p;
	}
}

static void CommitNoproxyInfo( Widget w, XtPointer client, XtPointer call)
{
	struct Proxy *p;
	char *addr;

	addr = XmTextGetString(Noproxy_Edit_Info->IF->addr_text);
	if (addr[0] == '\0') {
		XmxMakeErrorDialog(mMosaicToplevelWidget,
			COMMIT_ADDR_EMPTY_ERROR, "No Address Entered");
		XtManageChild (Xmx_w);
		return;
	}

	/* Make sure it is all lowercase */
	{char *ptr; for (ptr=addr; ptr && *ptr; ptr++) *ptr=tolower(*ptr);}

	if (Noproxy_Edit_Info->type == EDIT) {
		p = Noproxy_Edit_Info->editing;
	} else {
		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		Noproxy_Edit_Info->editing = p;
	}

	p->scheme = NULL;

	if (p->address)
		p->address = (char *)realloc(p->address,strlen(addr)+1);
	else
		p->address = (char *)calloc(1, strlen(addr)+1);

	strcpy(p->address, addr);
	XtFree(addr);

	p->port = NULL;

	if (Noproxy_Edit_Info->IF->alive!=NULL) {
		p->alive = (XmToggleButtonGetState(Noproxy_Edit_Info->IF->alive) == True) ? 0 : 1;
	}

	if (Noproxy_Edit_Info->type == ADD) {
		AddNoproxyToList(p);
		AppendNoproxy(p);
	}
	ShowNoproxyList();
	XtPopdown(EditNoproxyDialog);
}

static struct Proxy * FindNoproxyEntry(char *txt)
{
        struct Proxy *p;
        char *ptr;

        p = Noproxy_List;
                
        while (p != NULL) {  
                if (strcmp(p->address, txt) == 0) {
			return p;
                } 
                p = p->next;           
        }
	assert(0);
        return NULL; /* whoops */
}

static void EditNoproxyInfo( Widget w, XtPointer client, XtPointer call, int type)
{
	Widget text_form, form, address, port;
	Widget main_form, rc, rc2;
	Widget action_area, sep, dismiss, help;
	Widget commit;
	XmString selected_string;
	char *selected_text;

/* We obtain information from the client pointer, rather than getting
** it from call->item because this routine can be called from
** a pushbutton as well as from double clicking the list.
*/

	Noproxy_Edit_Info->type = type;
	if (type == EDIT) {
		selected_string = GetStringFromScrolled((Widget)Noproxy_Edit_Info->scrolled);

		if (selected_string == NULL) {
			XmxMakeErrorDialog(mMosaicToplevelWidget, EDIT_ERROR, "No Entry Selected");
			XtManageChild (Xmx_w);
			return;
		}
		XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, 
					&selected_text);
		Noproxy_Edit_Info->editing = FindNoproxyEntry(selected_text);
		XtFree(selected_text);
		if (Noproxy_Edit_Info->editing == NULL) {
			assert(0);
			return; /* how did *that* happen? */
		}
	} else {
		Noproxy_Edit_Info->editing = NULL;
	}

	if (EditNoproxyDialog) {

		if (type == EDIT)
			FillProxyText(Noproxy_Edit_Info);
		else
			ClearProxyText(Noproxy_Edit_Info);

		XtPopup(EditNoproxyDialog, XtGrabNone);
		return;
	}

	EditNoproxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, XtParent(w),
		XmNdeleteResponse, XmDESTROY,
		XmNtitle,	"No_Proxy Information",
		NULL);

	main_form = XtVaCreateWidget("edit_form",
		xmFormWidgetClass, EditNoproxyDialog,
		NULL);
/* Create action area widgets */

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

	Noproxy_Edit_Info->IF = (struct InfoFields *)calloc(1,sizeof(struct InfoFields));

	Noproxy_Edit_Info->IF->proxy_text = NULL;
	Noproxy_Edit_Info->IF->alive = NULL;
		
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

	Noproxy_Edit_Info->IF->addr_text = XtVaCreateManagedWidget("addr_text",
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

	Noproxy_Edit_Info->IF->port_text = XtVaCreateManagedWidget("port_text",
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

        XtAddCallback(commit, XmNactivateCallback, CommitNoproxyInfo, Noproxy_Edit_Info);
        XtAddCallback(dismiss, XmNactivateCallback, CallDismiss, EditNoproxyDialog);
	XtAddCallback(help, XmNactivateCallback, CallHelp, "help-noproxy-edit.html");

	if (type == EDIT)
		FillProxyText(Noproxy_Edit_Info);
	else
		ClearProxyText(Noproxy_Edit_Info);

	XtPopup(EditNoproxyDialog, XtGrabNone);
}

static void CallWriteNoproxies(Widget w, XtPointer client, XtPointer call)
{
	FILE *fp;
	struct Proxy *p;
	char msgbuf[1000];

	p= Noproxy_List;

	if ((fp = fopen(mMosaicNoProxyFileName,"w")) == NULL) {
                sprintf(msgbuf,SAVE_ERROR, mMosaicNoProxyFileName);
                XmxMakeErrorDialog(mMosaicToplevelWidget, msgbuf, "Error writing file");
                XtManageChild (Xmx_w);
                return;
        }
        while (p != NULL) {
                if (p->address)
                        fprintf(fp,"%s ",p->address);
                fprintf(fp,"\n");
                p = p->next;
        }
        sprintf(msgbuf,SAVED_AOK,mMosaicNoProxyFileName);
        XmxMakeInfoDialog(mMosaicToplevelWidget, msgbuf, "File Saved");
        XtManageChild (Xmx_w);
        fclose(fp);
}
 
static void CallEditNoproxy(Widget w, XtPointer client, XtPointer call)
{
        EditNoproxyInfo(w, client, call, EDIT);
}                       
        
static void CallAddNoproxy(Widget w, XtPointer client, XtPointer call)
{ 
        EditNoproxyInfo(w, client, call, ADD);
}  

static void DeleteNoproxy(struct Proxy *p)
{
	struct Proxy *cur;
	struct Proxy *pl;

	cur = p;
	pl = Noproxy_List;

	if (cur == pl) {
		pl = cur->next;
		if (pl == NULL) {
			Noproxy_List = NULL;
		} else {
			Noproxy_List = pl;
		}
	}

	if (cur->next != NULL)
		cur->next->prev = p->prev;

	if (cur->prev != NULL)
		cur->prev->next = p->next;

/* Delete allocated space from proxy entry */

	if (p->address) {
		free(p->address);
		p->address = NULL;
	}
	free(p);
}

static void CallRemoveNoproxy(Widget w, XtPointer client, XtPointer call)
{
        XmString selected_string;
        char *selected_text;
        struct Proxy *pEditing;

        selected_string = GetStringFromScrolled(Noproxy_Edit_Info->scrolled);
                        
        if (selected_string == NULL) {
                XmxMakeErrorDialog(mMosaicToplevelWidget, REMOVE_ERROR, "No Entry Selected");   
                XtManageChild (Xmx_w); 
                return;
        }       
        XmStringGetLtoR(selected_string, XmSTRING_DEFAULT_CHARSET, &selected_text);     
        pEditing = FindNoproxyEntry(selected_text);
        
        DeleteNoproxy(pEditing);
        ShowNoproxyList();
        XtFree(selected_text);
}


void PopNoproxyDialog()
{
	Widget main_form, action_area;
	Widget add, edit, remove, dismiss, help;
	Widget save;
	Widget scrolled;
	int  n;
	Arg args[20];

	XFontStruct *font;
	XmFontList *fontlist=NULL;

	if (NoproxyDialog) {
		ShowNoproxyList();
		XtPopup(NoproxyDialog, XtGrabNone);
		return;
	}

/* Try and get a nice non-proportional font.  If we can't get 
 * it, then the heck with it, just use the default.
*/
	font = XLoadQueryFont(mMosaicDisplay, FONTNAME);
	if (font == NULL) {
		font = XLoadQueryFont(mMosaicDisplay, "fixed");
	}
	if (font != NULL) {
		fontlist = (XmFontList *)XmFontListCreate(font, XmSTRING_DEFAULT_CHARSET);
	}

	NoproxyDialog = XtVaCreatePopupShell("Proxies",
		xmDialogShellWidgetClass, mMosaicToplevelWidget,
		XmNdeleteResponse, XmUNMAP,
		XmNtitle,	"Proxies",
		NULL);

	main_form = XtVaCreateWidget("proxy_form",
		xmFormWidgetClass, NoproxyDialog,
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

	Noproxy_Edit_Info = (struct EditInfo *)calloc(1,sizeof(struct EditInfo));
	Noproxy_Edit_Info->scrolled = scrolled;

        XtAddCallback(edit, XmNactivateCallback, CallEditNoproxy, Noproxy_Edit_Info);
	XtAddCallback(scrolled, XmNdefaultActionCallback, CallEditNoproxy, Noproxy_Edit_Info);
	XtAddCallback(add, XmNactivateCallback, CallAddNoproxy, Noproxy_Edit_Info);
        XtAddCallback(remove, XmNactivateCallback, CallRemoveNoproxy, Noproxy_Edit_Info);
        XtAddCallback(dismiss, XmNactivateCallback, CallDismiss, NoproxyDialog);
	XtAddCallback(save, XmNactivateCallback, CallWriteNoproxies, Noproxy_Edit_Info);
	XtAddCallback(help, XmNactivateCallback, CallHelp, Noproxy_Help_File_Name);
	XtManageChild(main_form);
	ShowNoproxyList();
	XtPopup(NoproxyDialog, XtGrabNone);
}

