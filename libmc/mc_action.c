/* mc_action.c
 * Author: Gilles Dauphin
 * Version 2.7b4m3 [May96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 * Bug report :  dauphin@sig.enst.fr dax@inf.enst.fr
 */

#ifdef MULTICAST

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <ctype.h>
#include <sys/utsname.h>
#include <pwd.h>
#include <Xm/XmAll.h>
#include <signal.h>

#include "../libnut/mipcf.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../src/mosaic.h"
#define __SRC__
#include "../libwww2/HTAAUtil.h"

#include "mc_rtp.h"
#include "mc_defs.h"
#include "mc_misc.h"
#include "mc_sockio.h"
#include "mc_dispatch.h"
#include "mc_action.h"

#define CLIP_TRAILING_NEWLINE(url) \
  if (url[strlen (url) - 1] == '\n') \
    url[strlen (url) - 1] = '\0';

IPAddr  	mc_addr_ip_group;
unsigned short 	mc_port;
unsigned short 	mc_rtcp_port;
int 		mc_debug;
time_t 		mc_init_gmt;
unsigned short 	mc_my_pid;
unsigned char 	mc_ttl;
time_t 		mc_init_local_time;


extern Widget toplevel;

extern void McUpdateWidgetObject();

void McRemoveMoWin(McUser * u);
void ClearRcvData(McUser * d);

McUserList mc_ulist=NULL;

Widget mc_list_top_w = NULL;
Widget mc_list_main_rc_w = NULL;

typedef struct _mc_w_u_lst {
	Widget form;
	Widget toggle;
	Widget label;
	McUser * user;
/*	mo_window * win; */
} McWULst;

McWULst * mc_wulst;
int mc_wulst_cnt = 0;


static McUser * SolveUser(IPAddr uip_addr, unsigned short upid);

/*########### remove the user too #########*/

/* When click again on user in member list. I don't want listen to this
 * user. So remove the user's mo_window */
/* ##### c'est a faire... ##### */
void McRemoveMoWin(McUser * u)
{
	mo_window *win;
	mo_node *node ;

	win = u->win;

	if(win)
		mo_delete_window(win);
	u->win = NULL;
	u->rcv_enable = 0;
	ClearRcvData(u);
	XtVaSetValues(mc_wulst[u->mc_list_number].toggle,
				XmNset, False,
				NULL);
	
}

void McDeleteUser(McUser *u, int i)
{
        McUser * uu;
	int j;
	int nuu;

	XtDestroyWidget(mc_wulst[u->mc_list_number].form);
	if ((u->prev == NULL) && (u->next == NULL)){
		mc_ulist = NULL;
		free(u);
	} else if (u->prev == NULL){
		mc_ulist = u->next;
		mc_ulist->prev = NULL;
		free(u);
	} else if (u->next == NULL){
		u->prev->next = NULL;
		free(u);
	} else {
		u->prev->next = u->next;
		u->next->prev = u->prev;
		free(u);
	}
		
	for(j=i+1; j < mc_wulst_cnt; j++){
		uu = mc_wulst[j].user;
		nuu = uu->mc_list_number;
		mc_wulst[j-1] = mc_wulst[j];
		uu->mc_list_number = nuu - 1;
	}
	mc_wulst_cnt-- ;
}
 

/*
 * Bye: Un participant quite.
 */
void McActionRtcpByeData(McRtcpByeDataStruct * rbye)
{
	unsigned int i;
	McUser * user;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        IPAddr uip_addr;
        unsigned short upid;
	char * tmp;
	time_t t;
	int u_is_upd = 0;


        if(ADDRCMP(rbye->ipaddr, mc_local_ip_addr) && (rbye->pid==mc_my_pid)){
#ifdef MDEBUG
		printf("McActionBye: It's Me\n");
#endif
                return;			/* It's Me */
	}
	uip_addr = rbye->ipaddr;
	upid =  rbye->pid;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionByeData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */

	t = time(NULL);
        user->last_rcv_time= user->last_sender_time = t; /*Last time i hear him */
	win = user->win;
	if(win) 
		McRemoveMoWin(user);
	McDeleteUser(user, user->mc_list_number);
}
/* cyclique check if rcv is alive */
 
void McCheckRcvstimeTimeOutCb(XtPointer clid, XtIntervalId * id)
{       
        McUser * u;
        time_t t;
        int i;

        t = time(NULL);
        
        for(i = 1; i<mc_wulst_cnt; i++){ /* start at 1 . # 0 is Me */
                u = mc_wulst[i].user;
                if( (t - u->last_rcv_time) > (MC_CHECK_RCVSTIME_TIME_OUT/1000)){
                        /* remove the member */
                        if(u->win) 
                                McRemoveMoWin(u); 
			McDeleteUser(u, i);
                }
        }
	mc_check_rcvstime_time_out_id = XtAppAddTimeOut(
		app_context,
		MC_CHECK_RCVSTIME_TIME_OUT,
		McCheckRcvstimeTimeOutCb,
		NULL);
}                                      
                                       
/* cyclique check if member is alive as sender */
                                       
void McCheckSenderstimeTimeOutCb(XtPointer clid, XtIntervalId * id)
{                                      
        McUser * u;
        time_t t; 
	int i;
 
        t = time(NULL); 
 
        for(i = 1; i<mc_wulst_cnt; i++){
                u = mc_wulst[i].user;
                if (u->url_id == 0) continue; 
                if( (t - u->last_sender_time) > (MC_CHECK_SENDERSTIME_TIME_OUT/1000)){
                        /* the sender member become a reveiver */ 
                        if(u->win) 
                                McRemoveMoWin(u); 
                        u->url[0] = '\0'; 
                        XmxAdjustLabelText(mc_wulst[u->mc_list_number].label,u->url);                     
                } 
        }
	mc_check_senderstime_time_out_id = XtAppAddTimeOut(
		app_context,
		MC_CHECK_SENDERSTIME_TIME_OUT,
		McCheckSenderstimeTimeOutCb,
		NULL); 
}   

void McCreateWUserlist(mo_window * win)
{
	Arg args[2];
	Widget sw;

	XtSetArg(args[0], XmNallowShellResize, True);
	XtSetArg(args[1], XmNtitle, "mMosaic - Members");
	mc_list_top_w = XtCreatePopupShell("Members",
				topLevelShellWidgetClass, toplevel, args, 2);
	sw = XtVaCreateManagedWidget ("scrolled_w",
		xmScrolledWindowWidgetClass, mc_list_top_w,
		XmNwidth,           200,
		XmNheight,          200,
		XmNscrollingPolicy, XmAUTOMATIC,
		NULL);

/* RowColumn is the work window for the widget */
	mc_list_main_rc_w = XtVaCreateWidget ("mc_main_rc", 
			xmRowColumnWidgetClass, sw,
			XmNorientation, XmVERTICAL,
			XmNpacking, XmPACK_COLUMN,
			NULL);

/* for my entry, create a form containing a toggle button and a label */
	mc_wulst = (McWULst*) malloc(sizeof(McWULst));
	mc_wulst[0].form = XtVaCreateWidget("mc_form", xmFormWidgetClass,
				mc_list_main_rc_w, 
				XmNborderWidth,      1,
				NULL);
	mc_wulst[0].toggle = XtVaCreateManagedWidget(mc_alias_name, 
			xmToggleButtonWidgetClass, mc_wulst[0].form,
			XmNborderWidth,	     1,
			XmNalignment,        XmALIGNMENT_BEGINNING,
			XmNtopAttachment,    XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment,   XmATTACH_FORM,
			XmNrightAttachment,  XmATTACH_NONE,
			XmNsensitive,	     False,
			NULL);
	mc_wulst[0].label = XtVaCreateManagedWidget(mc_local_url, 
			xmLabelWidgetClass, mc_wulst[0].form,
			XmNborderWidth,	     1,
			XmNalignment,        XmALIGNMENT_BEGINNING,
			XmNtopAttachment,    XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment,   XmATTACH_WIDGET,
			XmNleftWidget,       mc_wulst[0].toggle,
			XmNrightAttachment,  XmATTACH_NONE,
			NULL);
        XtManageChild (mc_wulst[0].form);
        mc_wulst_cnt++;

	XtManageChild (mc_list_main_rc_w);
	XtManageChild (mc_list_top_w);
	XtRealizeWidget (mc_list_top_w);
	XtPopdown(mc_list_top_w);
	mc_wulst[0].user = NULL;	/* it's me */
}
void McCreateMoWin(McUser * u)
{
	mo_window * win;
	mo_node *node ;

	win = mo_make_window(NULL, MC_MO_TYPE_RCV_ALL);
	u->win = win;
	win->mc_user = u;

/*	node= (mo_node *)malloc (sizeof (mo_node)); */
/*	node->title=NULL;	*/
/*	node->url=NULL;     /* c'est l'url de reference sous forme canonique*/
/*	node->last_modified=NULL;
	node->expires=NULL;
	node->ref=NULL;
	node->text=NULL;
	node->texthead=NULL;
	node->position=0;
	node->annotation_type=0;
	node->docid=1;
	node->cached_stuff=NULL;
	node->authType=0;
	node->previous=NULL;
	node->next=NULL;
*/
#ifdef TO_DO
/* ...faut comprendre ce que ca fait tous ces champs... */
/* node-> char *title; */
/* node-> char *last_modified; */
/* node-> char *expires; */
/* node-> char *ref; */
/* node-> char *text; */
/* node-> char *texthead; */
/* node-> int position; */
/* node-> int annotation_type; */
/* node-> int docid; */
/* node-> void *cached_stuff; */
/* node-> int authType; */
#endif
}

void PopUpOrDownMosaicMcUser(Widget w, XtPointer clid, XtPointer calld)
{
	McUser * u = (McUser *) clid;

	if (u->rcv_enable == 0) {	/* create an mo_window for user */
		McCreateMoWin(u);
		u->rcv_enable = 1;
		if (u->neo == 0) return;
		if (!u->eos) return;
		if (!u->eos[1]) return;
                mo_do_window_text(     
                        u->win,            /* an mo_window */
                        u->url,      /* url */
                        u->eos[1],   /* le texte */
                        "",             /* txthead */
                        False,           /* register_visit */
                        NULL,           /* ref */
                        NULL,           /* last_modified */
                        NULL); 
	} else {			/* delete the user's mo_window */
		McRemoveMoWin(u);
	}
}

void McUpdMcList( McUser * u)
{
	int nu;

	if (u->mc_list_number == -1) { /* add an entry */
		mc_wulst = (McWULst*) realloc(mc_wulst,
				sizeof(McWULst) * (mc_wulst_cnt + 1));
		nu = u->mc_list_number = mc_wulst_cnt;
		mc_wulst_cnt++;
		mc_wulst[nu].form = XtVaCreateWidget(NULL, xmFormWidgetClass,
				mc_list_main_rc_w, NULL);
		mc_wulst[nu].toggle = XtVaCreateManagedWidget(u->alias, 
			xmToggleButtonWidgetClass, mc_wulst[nu].form,
			XmNalignment,        XmALIGNMENT_BEGINNING,
			XmNtopAttachment,    XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment,   XmATTACH_FORM,
			XmNrightAttachment,  XmATTACH_NONE,
			NULL);
		mc_wulst[nu].label = XtVaCreateManagedWidget(u->url, 
			xmLabelGadgetClass, mc_wulst[nu].form,
			XmNalignment,        XmALIGNMENT_BEGINNING,
			XmNtopAttachment,    XmATTACH_FORM,
			XmNbottomAttachment, XmATTACH_FORM,
			XmNleftAttachment,   XmATTACH_WIDGET,
			XmNleftWidget,	     mc_wulst[nu].toggle,
			XmNrightAttachment,  XmATTACH_NONE,
			NULL);
		mc_wulst[nu].user = u;
		XtManageChild (mc_wulst[nu].form);
		XtManageChild (mc_list_main_rc_w);
		XtRealizeWidget(mc_wulst[nu].form);
		XtAddCallback(mc_wulst[nu].toggle,XmNvalueChangedCallback,
			PopUpOrDownMosaicMcUser,u);
		return;
	}
	nu = u->mc_list_number;
	XmxAdjustLabelText(mc_wulst[nu].toggle,u->alias);
	XmxAdjustLabelText(mc_wulst[nu].label,u->url);
}

void McInitUser( McUser * u)
{
        u->url[0] = '\0';
        u->url_id = 0;
        u->last_rcv_time = u->last_sender_time = 0;
        u->player_goto_time = 0;
        u->my_goto_time = 0;
        u->player_fdata_time = 0;
        u->my_fdata_time = 0;
        u->len_alias = 0;
        u->alias[0] = '\0';
        u->mc_list_number = -1;
        u->rcv_enable = 0;
        u->text = NULL;
        u->neo = 0;
        u->seo = NULL;
        u->eos = NULL;
	u->filename = NULL;
        u->pds = NULL;
        u->win = NULL;
	u->oldx = -100;
	u->oldy = -100;
	u->next = NULL;
	u->prev = NULL;
}

static McUser * SolveUser(IPAddr uip_addr, unsigned short upid)
{
	McUser * user;
	McUser * puser;

	if (mc_ulist == NULL){		/* create the first */
		user = (McUser *) malloc(sizeof(McUser));
		McInitUser(user);
		user->next = NULL;
		user->prev = NULL;
		user->ip_addr = uip_addr;
		user->pid = upid;
		mc_ulist = user;
		return user;
	}
	user = mc_ulist;
	while(user != NULL){
		if( ADDRCMP(user->ip_addr, uip_addr) && (user->pid == upid) )
			return user;
		puser = user;
		user = user->next;
	}
	user = (McUser *) malloc(sizeof(McUser));
	McInitUser(user);
	puser->next = user;
	user->next = NULL;
	user->prev = puser;
	user->ip_addr = uip_addr;
	user->pid = upid;
#ifdef IPV6
	sprintf(user->alias,"Unknow@ipv6.domain");
#else
	sprintf(user->alias,"Unknow@%d.%d.%d.%d",
		(uip_addr >> 24) & 0xFF, (uip_addr >> 16) & 0xFF,
		(uip_addr >> 8 ) & 0xFF, uip_addr & 0xFF);
#endif
	user->len_alias = strlen(user->alias);
	McUpdMcList(user);	/* MAJ de la mc_liste dans le cas d'un*/
				/* petit nouveau */
	return user;
}

void ClearRcvData(McUser * d)
{
	int i,j;

	if(d->neo == 0) return;

	for(i = 0; i< d->neo;i++){
		if( d->eos[i] != NULL) {
			free(d->eos[i]);
			d->eos[i] = NULL;
		}
		if ( d->filename[i] != NULL){
			unlink(d->filename[i]);
			free(d->filename[i]);
			d->filename[i] = NULL;
		}
	}
	free(d->seo);
	free(d->eos);
	free(d->filename);
	d->eos=NULL;
	d->filename=NULL;
	d->seo=NULL;
	for(i=0; i< d->neo; i++){
		if (d->pds[i].nalloc == 0)
			continue;
		for(j=0; j<d->pds[i].nalloc; j++){
			free(d->pds[i].pdata[j]);
			d->pds[i].pdata[j] = NULL;
		}
		free(d->pds[i].pdata);
		d->pds[i].pdata = NULL;
	}

	free (d->pds);
	d->pds = NULL;
	d->neo = 0;
	d->url[0] = '\0';
	d->url_id = 0;
	d->oldx = -100;
	d->oldy = -100;
}

/*
 * C'est pareil que McActionAllData, sauf qu'on a que le premier
 * packet (Packets[0]). C'est comme si on perd tous les autres
 * il va falloir demander les packets manquants (voir LRMP)
 */
void McActionHearBeatData(Mcs_alldata* alldata)
{
	unsigned int i,j, num_eo,seo,nombre_packet,num_packet,len_data;
	char * data;
	McUser * user;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        unsigned int uurl_id;
        IPAddr  uip_addr;
        unsigned short upid;
	unsigned int ussrc;
	char * tmp;
	time_t t;
	int u_is_upd = 0;

        if(ADDRCMP(alldata->ipaddr, mc_local_ip_addr) && (alldata->pid==mc_my_pid))
                 return;                        /* It's Me */
#ifdef MDEBUG
	printf("RTP have a hearbeat\n");
#endif
	ugmts = alldata->gmt_send_time;
	uurl_id = alldata->url_id;
	uip_addr = alldata->ipaddr;
	upid =  alldata->pid;
	ussrc = alldata->ssrc;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionHearBeatData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */
	/* So the url_id can only grown up */

	if((user->url_id < alldata->url_id) && 	/* a new text  */
	   user->rcv_enable)
		ClearRcvData(user);		/*clean the struct & free block */

	/* now fill the 'user' structure with the 'alldata' struct */

	t = time(NULL);
	user->url_id = alldata->url_id;
        user->last_rcv_time = user->last_sender_time = t; /* Last time i hear him */
	user->my_fdata_time = t;   	/* Last msg rcv by this user*/
	user->player_fdata_time = alldata->gmt_send_time;/* Player last upd gmt*/

	if(strcmp(user->url,alldata->data)){
		strcpy(user->url,alldata->data);
		McUpdMcList(user);	/* MAJ de la mc_liste */
	}
        win = user->win;                /* la mo_window du user */
        if(user->neo == 0){     /* allocate space in 'user'*/
                user->neo = alldata->nombre_eo;
                user->seo = (unsigned int *) malloc(user->neo *
                                                        sizeof(unsigned int));
                user->eos = (char**) malloc(user->neo * sizeof(char *));
		user->filename = (char**) malloc(user->neo * sizeof(char *));
                user->pds = (McPacketData*) malloc(user->neo *
                                                        sizeof(McPacketData));
                for(i=0; i < user->neo; i++){
                        user->seo[i] = 0;
                        user->eos[i] = NULL;
                        user->filename[i] = NULL;
                        user->pds[i].nalloc = 0;
                        user->pds[i].nbre_rcv = 0;
                        user->pds[i].pdata = NULL;
                }                      
        }                              
	if (!user->rcv_enable) return;

	user->eos[0] = strdup(user->url);
	for(i=1; i < user->neo; i++){ /* look at what is missing */
		if (user->eos[i] != NULL )          /* on a deja tout eu */
                	continue; 
		if (user->pds[i].pdata == NULL){ /* il manque tout l'objet i*/
					/*Le reclammer */
			McSendRtcpNackAll( uip_addr, upid, uurl_id,i,
				ussrc);
				/* l'envoie est imediat ### revoir LRMP */
				/* pour mettre des delais */
			continue;
		}
		/* sinon regarder tous les packets de l'objet */
		McInitNackPacketData(uip_addr, upid, uurl_id,i,ussrc);
		for(j = 0; j < user->pds[i].nalloc; j++){
			if (user->pds[i].pdata[j] == NULL) {
				/*reclamer ce packet ######## */
				McPushNackPacketData( uip_addr, upid, uurl_id,i,j,
					ussrc);
				/* l'envoie est imediat ### revoir LRMP */
				/* pour mettre des delais */
				continue;
			}
				/* sinon on l'a */
			continue;	/* ne rien faire */
		}
		McFlushNackPacketData();
	}
}

/*
 * affiche l' hypertexte dans la fenetre idoine
 */
void McActionAllData(Mcs_alldata* alldata)
{
	unsigned int i, num_eo,seo,nombre_packet,num_packet,len_data;
	char * data;
	McUser * user;
	mo_window *win;
        unsigned int ugmts;
        unsigned int uurl_id;
        IPAddr  uip_addr;
        unsigned short upid;
	char * tmp;
	time_t t;
	int u_is_upd = 0;

/* Process a directive that we received by multicast. */
        if(ADDRCMP(alldata->ipaddr, mc_local_ip_addr) && (alldata->pid==mc_my_pid))
                 return;			/* It's Me */
	ugmts = alldata->gmt_send_time;
	uurl_id = alldata->url_id;
	uip_addr = alldata->ipaddr;
	upid =  alldata->pid;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionAllData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */
	/* So the url_id can only grown up */

	if((user->url_id < alldata->url_id) && 	/* a new text  */
	   user->rcv_enable)
		ClearRcvData(user);		/*clean the struct & free block */

	/* now fill the 'user' structure with the 'alldata' struct */

	t = time(NULL);
	/*user->ip_addr; 		set by SolveUser()*/
	/*user->pid; 			set by SolveUser()*/
	/*user->rcv_enable;		set by SolveUser and a switch */
        /*user-> mc_list_number;       	set by SolveUser() */
        /*user->next;
        /*user->prev;
	/*user->url[MC_MAX_URL_SIZE+1];	set when i rcv url ot goto */
	/*user->player_goto_time;	"	"		   */
	/*user->my_goto_time;		"	"		   */
	/*user->len_alias;		set when i rcv alias */
	/*user->alias[MC_MAX_ALIAS_SIZE+1];  "		"    */
        /*user->win ;			set to NULL byt SolveUser() */
	user->url_id = alldata->url_id;
        user->last_rcv_time = user->last_sender_time = t;/*Last time i hear him */
	user->my_fdata_time = t;   	/* Last msg rcv by this user*/
	user->player_fdata_time = alldata->gmt_send_time;/* Player last upd gmt*/

	if(user->url[0] == '\0'){
		strcpy(user->url,"Url Path Not Available Yet");
		u_is_upd++;
	}

	num_eo = alldata->num_eo;
	if( num_eo == 0){		/* URL */
		strcpy(user->url,alldata->data);
		McUpdMcList(user);	/* MAJ de la mc_liste */
		return;
	}
	if(u_is_upd)
		McUpdMcList(user);	/* MAJ de la mc_liste dans le cas d'un*/
					/* petit nouveau */

	win = user->win;		/* la mo_window du user */
	if(user->neo == 0){	/* allocate space in 'user'*/
		user->neo = alldata->nombre_eo;
		user->seo = (unsigned int *) malloc(user->neo * 
							sizeof(unsigned int));
		user->eos = (char**) malloc(user->neo * sizeof(char *));
		user->filename = (char**) malloc(user->neo * sizeof(char *));
		user->pds = (McPacketData*) malloc(user->neo * 
							sizeof(McPacketData));
		for(i=0; i < user->neo; i++){
			user->seo[i] = 0;
			user->eos[i] = NULL;
			user->filename[i] = NULL;
			user->pds[i].nalloc = 0;
			user->pds[i].nbre_rcv = 0;
			user->pds[i].pdata = NULL;
		}
	}
	seo = alldata->seo;
	nombre_packet = alldata->nombre_packet;
	num_packet = alldata->num_packet;
	data = alldata->data;
	len_data = alldata->len_data;
	if (user->eos[num_eo] != NULL ) 	 /* on a deja tout eu */
		return;
	if(user->pds[num_eo].pdata == NULL ){ /* initialise pointeur */
		user->pds[num_eo].pdata = (char **) malloc(nombre_packet *
						sizeof(char*));
		for(i=0; i< nombre_packet;i++)
			user->pds[num_eo].pdata[i]=NULL;
		user->pds[num_eo].nalloc = nombre_packet;
		user->seo[num_eo] = seo;
	}
#ifdef MDEBUG
	printf ("rcv data for num_eo %d, num_packet %d\n",
		num_eo,num_packet);
#endif
	if (user->pds[num_eo].pdata[num_packet] != NULL) /* we have it */
		return;
	/* update the data and test if complete */
	user->pds[num_eo].pdata[num_packet] = (char*) malloc (MC_PACKET_SIZE);
	memcpy(user->pds[num_eo].pdata[num_packet],data,len_data);
	user->pds[num_eo].nbre_rcv++;
#ifdef MDEBUG
	printf("num_eo = %d , nbre_rvc/nalloc = %d/%d\n",
		num_eo, user->pds[num_eo].nbre_rcv,user->pds[num_eo].nalloc);
#endif
	if( user->pds[num_eo].nbre_rcv < user->pds[num_eo].nalloc )
		return;			/* not enought data */
	if (user->pds[num_eo].nbre_rcv > user->pds[num_eo].nalloc ){
		printf("Ca merde dans le nombre de packet\007\n");
		return;
	}

	/* Assemble le packet et faire le necessaire dans Mosaic */
	switch (num_eo) {
	case 1:		/* le texte */
		user->eos[1] =(char*)malloc(user->pds[1].nalloc * MC_PACKET_SIZE);
		tmp = user->eos[1];
		for(i = 0; i< user->pds[1].nalloc; i++){ /* reassemble packet */
			memcpy(tmp, user->pds[1].pdata[i],MC_PACKET_SIZE);
			tmp = tmp + MC_PACKET_SIZE;
			free(user->pds[1].pdata[i]);
			user->pds[1].pdata[i] = NULL;
		}
		free(user->pds[1].pdata);
		user->pds[1].pdata = NULL;
		user->pds[1].nalloc = user->pds[1].nbre_rcv = 0;
		/* in user->eos[1] we have the texte . put in Mosaic */
		if (!user->rcv_enable) return;
		mo_do_window_text(
			win,		/* an mo_window */
			user->url,	/* url */
			user->eos[1],	/* le texte */
			"",		/* txthead */
			False,		/* register_visit */
			NULL,		/* ref */
			NULL,		/* last_modified */
			NULL);		/* expires */
		break;
	default:		/* the embedded oject */
#ifdef MDEBUG
		printf("OBJECT is complete...! num_eo = %d\n",num_eo);
#endif
		if ( user->eos[1] == NULL ) { /* need to have the text before */
			printf("Have object but not the TEXT\n");
			return;
		}
		user->eos[num_eo] = (char*) malloc(user->pds[num_eo].nalloc * 
							MC_PACKET_SIZE);
		tmp = user->eos[num_eo];
		for(i=0; i< user->pds[num_eo].nalloc; i++){/* reassemble packet */
                        memcpy(tmp, user->pds[num_eo].pdata[i],MC_PACKET_SIZE);
                        tmp = tmp + MC_PACKET_SIZE;
			free(user->pds[num_eo].pdata[i]);
			user->pds[num_eo].pdata[i] = NULL;
                }
		free(user->pds[num_eo].pdata);
		user->pds[num_eo].pdata = NULL;
		user->pds[num_eo].nalloc = user->pds[num_eo].nbre_rcv = 0;
		if (!user->rcv_enable) return;
		mo_do_window_text(win, user->url, user->eos[1], 
                        "", False, NULL, NULL, NULL);     
                break;
	}
}

/*
 * CursorPosition: change la position du curseur virtuel
 */
void McActionCursorPosData(McRtpCursorPosDataStruct * cp)
{
	unsigned int i;
	McUser * user;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        IPAddr uip_addr;
        unsigned short upid;
	char * tmp;
	time_t t;
	int u_is_upd = 0;


        if(ADDRCMP(cp->ipaddr, mc_local_ip_addr) && (cp->pid==mc_my_pid)){
#ifdef MDEBUG
		printf("McActionCursorPosData: It's Me\n");
#endif
                return;			/* It's Me */
	}
	uip_addr = cp->ipaddr;
	upid =  cp->pid;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionCursorPosData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */

	t = time(NULL);
        user->last_rcv_time= user->last_sender_time = t; /*Last time i hear him */
	if (!user->rcv_enable) return;
	win = user->win;
	if(!win) return;

	if((user->url_id != cp->url_id) && 	/* a new text  */
	   user->rcv_enable)
		return;
	if ( user->eos[1] == NULL ) { /* need to have the text before */
		return;
	}
/* now position the virtual cursor */
	McSetCursorPos(win->scrolled_win, user->oldx, user->oldy);
	McSetCursorPos(win->scrolled_win, cp->posx, cp->posy);
	user->oldx = cp->posx;
	user->oldy = cp->posy;
}

/*
 * GotoId: change la valeur de la scrollbar et place le texte
 */
void McActionGotoIdData(McRtpGotoIdDataStruct * hgid)
{
	unsigned int i;
	McUser * user;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        IPAddr uip_addr;
        unsigned short upid;
	char * tmp;
	time_t t;
	int u_is_upd = 0;


        if(ADDRCMP(hgid->ipaddr, mc_local_ip_addr) && (hgid->pid==mc_my_pid)){
#ifdef MDEBUG
		printf("McActionGotoIdData: It's Me\n");
#endif
                return;			/* It's Me */
	}
	uip_addr = hgid->ipaddr;
	upid =  hgid->pid;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionGotoIdData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */

	t = time(NULL);
        user->last_rcv_time= user->last_sender_time = t; /*Last time i hear him */
	if (!user->rcv_enable) return;
	win = user->win;
	if(!win) return;

	if((user->url_id != hgid->url_id) && 	/* a new text  */
	   user->rcv_enable)
		return;
	if ( user->eos[1] == NULL ) { /* need to have the text before */
		return;
	}
/* now goto */
	HTMLGotoId(win->scrolled_win, hgid->html_goto_id,0);
}

/*
 * affiche le membre  dans la fenetre idoine
 */
void McActionRtcpSdesCnameData(McRtcpSdesCnameDataStruct * rscd)
{
	unsigned int i;
	char * data;
	McUser * user;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        IPAddr uip_addr;
        unsigned short upid;
	char * tmp;
	char b[MC_MAX_ALIAS_SIZE+1];
	time_t t;
	int u_is_upd = 0;


        if(ADDRCMP(rscd->ipaddr, mc_local_ip_addr) && (rscd->pid==mc_my_pid)){
#ifdef MDEBUG
		printf("McActionRtcpSdesCnameData: It's Me\n");
#endif
                return;			/* It's Me */
	}
	uip_addr = rscd->ipaddr;
	upid =  rscd->pid;
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionRtcpSdesCnameData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */
	/* now fill the 'user' structure with the 'rscd' struct */

	t = time(NULL);
        user->last_rcv_time = t;   		/* Last time i hear him */

	/*user->ip_addr; 		set by SolveUser()*/
	/*user->pid; 			set by SolveUser()*/
	/*user->rcv_enable;		set by SolveUser and a switch */
        /*user-> mc_list_number;       	set by SolveUser() */
        /*user->next;
        /*user->prev;
	/*user->url[MC_MAX_URL_SIZE+1];	set when i rcv url ot goto */
	/*user->player_goto_time;	"	"		   */
	/*user->my_goto_time;		"	"		   */
	/*user->len_alias;		set when i rcv alias */
	/*user->alias[MC_MAX_ALIAS_SIZE+1];  "		"    */
        /*user->win ;			set to NULL byt SolveUser() */
	/*user->my_fdata_time = t;   	/* Last msg rcv by this user*/

	if (strcmp(user->alias, rscd->alias) == 0)
		return;			/* c'est la meme chose */

	strcpy(user->alias,rscd->alias);
	user->len_alias = rscd->len_alias;

	McUpdMcList(user);	/* MAJ de la mc_liste */
}

void McActionRtcpLrmpNackAllData(McRtcpLrmpNackAllDataStruct * rlnad)
{
        IPAddr  uip_addr;
	IPAddr6 s_uip_addr;
	unsigned int i;
	char * data;
	McUser * user;
        int mask, omask;
	mo_window *win;
        unsigned int ugmts;
        unsigned short upid;
	char * tmp;
	time_t t;
	unsigned int r_url_id,r_num_eo;
	unsigned int s_upid;

        if(ADDRCMP(rlnad->ipaddr, mc_local_ip_addr) && (rlnad->pid==mc_my_pid)){
                return;			/* It's Me */
	}
#ifdef IPV6
	if( !(ADDRCMP(rlnad->s_ipaddr, mc_local_ip_addr) && ( rlnad->s_pid == mc_my_pid)) ){ /* this NACK is not for me */
		return;
	}
#else
	if( !(ADDRCMP64(rlnad->s_ipaddr, mc_local_ip_addr) && ( rlnad->s_pid == mc_my_pid)) ){ /* this NACK is not for me */
		return;
	}
#endif
	uip_addr = rlnad->ipaddr;
	upid =  rlnad->pid;
	s_uip_addr = rlnad->s_ipaddr;
	s_upid =  rlnad->s_pid;
	
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionRtcpLrmpNackAllData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */
	/* now fill the 'user' structure with the 'rscd' struct */

	t = time(NULL);
        user->last_rcv_time = t;   		/* Last time i hear him */

	r_url_id = rlnad->url_id;
	r_num_eo = rlnad->num_eo;

/* sigblock critical section */

#ifdef SVR4
        if( sighold(SIGUSR1) != 0){
                perror("error in sig hold: ");
        }
#else
        mask = sigmask(SIGUSR1);
        omask = sigblock(mask);
#endif
	if ((r_url_id != mc_local_url_id) || (mc_data_send_data_struct.id !=r_url_id) ){
#ifdef SVR4
	        if( sigrelse(SIGUSR1) != 0) {
                	perror("error in sigrelse: ");
        	}
#else
        	sigsetmask(omask);      
#endif 
		return;
	}

	McSendFastAllEoData(r_num_eo);

#ifdef SVR4
        if( sigrelse(SIGUSR1) != 0) {
                perror("error in sigrelse: ");
        }
#else
        sigsetmask(omask);      
#endif 
}
void McActionRtcpLrmpNackData(McRtcpLrmpNackDataStruct * rlnd)
{
        IPAddr uip_addr;
	IPAddr6 s_uip_addr;
	unsigned int i;
	char * data;
	McUser * user;
        int mask, omask;

	/* Process a directive that we received by multicast. */
	mo_window *win;
        unsigned int ugmts;
        unsigned short upid;
	char * tmp;
	time_t t;
	unsigned int r_url_id,r_num_eo,r_fpno;
	unsigned int s_upid;
	unsigned int r_blp;

        if(ADDRCMP(rlnd->ipaddr, mc_local_ip_addr) && (rlnd->pid==mc_my_pid)){
                return;			/* It's Me */
	}
#ifdef IPV6
	if( !(ADDRCMP(rlnd->s_ipaddr, mc_local_ip_addr) && ( rlnd->s_pid == mc_my_pid)) ){ /* this NACK is not for me */
		return;
	}
#else
	if( !(ADDRCMP64(rlnd->s_ipaddr, mc_local_ip_addr) && ( rlnd->s_pid == mc_my_pid)) ){ /* this NACK is not for me */
		return;
	}
#endif
	uip_addr = rlnd->ipaddr;
	upid =  rlnd->pid;
	s_uip_addr = rlnd->s_ipaddr;
	s_upid =  rlnd->s_pid;
	
	user = SolveUser(uip_addr,upid);	/* get user struct*/
	if (user == NULL){
		fprintf(stderr,"McActionRtcpLrmpNackData: PB getting user\n");
		return;
	}
	/* We know on Unix system that ip_addr/pid is unique in World */
	/* now fill the 'user' structure with the 'rscd' struct */

	t = time(NULL);
        user->last_rcv_time = t;   		/* Last time i hear him */

	r_url_id = rlnd->url_id;
	r_num_eo = rlnd->num_eo;
	r_fpno = rlnd->fpno;
	r_blp = rlnd->blp & 0x0000ffff;

/* sigblock critical section */

#ifdef SVR4
        if( sighold(SIGUSR1) != 0){
                perror("error in sig hold: ");
        }
#else
        mask = sigmask(SIGUSR1);
        omask = sigblock(mask);
#endif
	if ((r_url_id != mc_local_url_id) || (mc_data_send_data_struct.id !=r_url_id) ){
#ifdef SVR4
	        if( sigrelse(SIGUSR1) != 0) {
                	perror("error in sigrelse: ");
        	}
#else
        	sigsetmask(omask);      
#endif 
		return;
	}

	McSendFastPacketEoData(r_num_eo,r_fpno);
	while(r_blp){
		r_fpno++;
		if ( (r_blp & 0x0001) ){
			McSendFastPacketEoData(r_num_eo,r_fpno);
		}
		r_blp = r_blp >> 1;
	}

#ifdef SVR4
        if( sigrelse(SIGUSR1) != 0) {
                perror("error in sigrelse: ");
        }
#else
        sigsetmask(omask);      
#endif 
}

#endif /* MULTICAST */