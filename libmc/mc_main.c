/* G.D. [Jan2000] */

#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/gui-documents.h"

#include "mc_mosaic.h"
#include "mc_main.h"
#include "mc_gui.h"
#include "mc_misc.h"

#include "vir_cursor.xbm"

mo_window * 	mc_send_win;		/* if != NULL we are sending... */
int      	mc_multicast_enable;	/* mMosaic called with mc options */

			/* sockets */
int		mc_fd_rtp_w;
int		mc_fd_rtp_r;
int		mc_fd_rtcp_w;
int		mc_fd_rtcp_r;
int		uc_fd_rtp_w;
int		uc_fd_rtp_r;
int		uc_fd_rtcp_w;
int		uc_fd_rtcp_r;

			/* address and port . Net byte order*/
IPAddr		mc_local_ip_addr;
char		*mc_local_ip_addr_string;
char		mc_local_cname[257];

IPAddr		mc_rtp_addr_ip_group;
unsigned short	mc_rtp_addr_port;
IPAddr		mc_rtcp_addr_ip_group;
unsigned short	mc_rtcp_addr_port;
IPAddr		uc_rtp_addr_ip;
unsigned short	uc_rtp_addr_port = 0;
IPAddr		uc_rtcp_addr_ip;
unsigned short	uc_rtcp_addr_port = 0;

			/* delay timer */
unsigned long	uc_rtcp_w_sdes_time = 20000; 	/* in millisec */
unsigned long	mc_rtcp_w_time = 1000;		/* in millisec */

			/* Events */
XtInputId	mc_rtp_r_input_id;
XtInputId	mc_rtcp_r_input_id;
XtInputId	uc_rtp_r_input_id;
XtIntervalId	uc_rtcp_w_sdes_timer_id;

XtInputId	uc_rtcp_r_input_id;
XtIntervalId	mc_rtcp_w_timer_id;

XtIntervalId	mc_write_rtp_data_timer_id;

u_int32_t	rtp_init_time;

		/* callbacks */
static void McRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void UcRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void McRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void UcRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id);

		/* Actions */
static void McSendNewObject(char *fname, char *aurl, MimeHeaderStruct *mhs,
	DependObjectTab dot, int ndo, int *moid_ret);
static void McSendNewErrorObject(char *aurl, int status_code,
	int * moid_ret);
static void McSendNewState(mo_window * win, int moid_ref, DependObjectTab dot, int ndo);


int mc_len_alias = 0;

int mc_local_state_id = -1;
int mc_local_object_id = -1;

int mc_status_report_state_id = -1;
int mc_status_report_object_id = -1;

Pixmap			VirCursorPix;
GC			gc_vc;
XGCValues gcv;
unsigned long gcm;
unsigned long fg;
unsigned long bg;

/* initialisation of multicast datas structure */
/* called in gui.c after we open the first Mosaic window */
/* this start the multicast in listen mode only */
void McInit(mo_window * win)
{
	char * s;
	unsigned short port;
	struct in_addr ina;
	Widget hw;

	rtp_init_time = ntptime();
        mc_multicast_enable = 0;             
	if (mMosaicAppData.mc_dest == NULL)
		return;
	s = strchr(mMosaicAppData.mc_dest, '/');
	if (! *s){
		fprintf(stderr,"invalid Multicast addr/port\n");
		fprintf(stderr,"Multicast is disable\n");
		return;
	}

	*s = '\0';
	port = atoi(s+1);
	if (port%2!=0)
		port--;
	mc_rtp_addr_port = htons(port);
	mc_rtcp_addr_port = htons(port + 1);

#ifdef DEBUG_MULTICAST
	printf("dest/port: %s/%d, ttl=%d\n", mMosaicAppData.mc_dest, port,
			mMosaicAppData.mc_ttl);
#endif
#ifdef IPV6
/* inet_pton () */
	if( ascii2addr(AF_INET6,mMosaicAppData.mc_dest,&mc_rtp_addr_ip_group) == -1){
		fprintf(stderr,"invalid IPV6Multicast addr\n");
		fprintf(stderr,"Multicast is disable\n");
		return;
	}
#else   
	mc_rtp_addr_ip_group = inet_addr(mMosaicAppData.mc_dest);
#endif                                 
	mc_multicast_enable = 1;
	win->mc_type = MC_MO_TYPE_MAIN;
	XmxRSetSensitive (win->menubar, (XtPointer)mo_multicast_send_tog,
		XmxSensitive);
	XmxRSetSensitive(win->menubar, (XtPointer)mo_multicast_show_participant,
		XmxSensitive);
        mc_send_win = NULL;
	

/* initialise global variable for Multicast */
/*	mMosaicAppData.mc_sess_name; */
/*	mMosaicAppData.mc_media_name; */
/*	mMosaicAppData.mc_ttl; */
/*mc_dest become mc_rtp_addr_ip_group and mc_rtp_addr_port */
	if( mMosaicAppData.mc_alias_name == NULL ){
		mMosaicAppData.mc_alias_name = (char*) malloc(MC_MAX_SDES_NAME_LEN + 1);
		strncpy(mMosaicAppData.mc_alias_name,mMosaicAppData.author_email,
			MC_MAX_SDES_NAME_LEN);
	}
	if (strlen(mMosaicAppData.mc_alias_name) >= MC_MAX_SDES_NAME_LEN )
		mMosaicAppData.mc_alias_name[MC_MAX_SDES_NAME_LEN] = '\0';
	mc_len_alias = strlen(mMosaicAppData.mc_alias_name);
	win->mc_callme_on_new_object = NULL;
	win->mc_callme_on_error_object = NULL;

/* open useable socket. There are 8 sockets we use:
 * - mc_fd_rtp_w:	(Multicast)
 *			We are a sender, emit data throught this channel
 *			At Init we are a listener, don't open at Init.
 *			Only sender open this channel.
 * - mc_fd_rtp_r:	(Multicast)
 *			We are a listener, data come from this channel.
 *			Play with data if we want them.
 * - uc_fd_rtp_w:	(Unicast)
 *			We are a sender, emit recover data throught this channel.
 *			This is a Unicast recover channel. When a listener
 *			ask for a recover, we send the data throught this channel.
 *			At Init we are a listener. Don't open...
 *			Only sender open this channel.
 * - uc_fd_rtp_r:	(Unicast)
 *			We are a listener, the recover data come from this channel
 *			A sender send those repair data only for me
 * - mc_fd_rtcp_w:	(Multicast)
 *			We are a sender, Send SR and some control throught
 *			this channel. A listener don't send report via multicast,
 *			A listener send report throught Unicast.
 *			At Init we are a listener. Don't open...
 *			Only sender open this channel.
 * - mc_fd_rtcp_r:	(Multicast)
 *			Only Sender Report come from this channel.
 *			Check all data.
 * - uc_fd_rtcp_w:	(Unicast)
 *			We are a listener. We send our (receiver) report throught
 *			this chanel. A sender that is not a receiver Never send
 *			data throught this channel.
 *			a listener that listen nothing send minimal data throught
 *			this channel (SDES-CNAME).
 * - uc_fd_rtcp_r:	(Unicast)
 *			We are a Sender. We receive report/repair from listener
 *			throught this channel.
 *			Only sender open this channel. At Init we are a listener.
 *			Don't open...
 * SUMARY:
 * A listener open:
 *	mc_fd_rtp_r	data from a sender
 *	uc_fd_rtp_r	data from a sender (Unicast) case of repair.
 *	mc_fd_rtcp_r	Sender Report from a sender.
 *	uc_fd_rtcp_w	Listener send report and ask for repair.
 * A listener DONT open:	 because
 *	mc_fd_rtp_w	We dont send data ...
 *	mc_fd_rtcp_w	Report via Unicast ...
 *	uc_fd_rtp_w	We dont send data ...
 *	uc_fd_rtcp_r	We dont receive report, because we are not a sender ...
 */
	mc_fd_rtp_w = -1;
	mc_fd_rtcp_w = -1;
	uc_fd_rtp_w = -1;
	uc_fd_rtcp_r = -1;

	mc_local_ip_addr = GetLocalIpAddr();
	ina.s_addr = mc_local_ip_addr;
	mc_local_ip_addr_string = strdup(inet_ntoa(ina));
	mc_fd_rtp_r = McOpenRead(mc_rtp_addr_ip_group,mc_rtp_addr_port,
		mMosaicAppData.mc_ttl);
	mc_rtcp_addr_ip_group = mc_rtp_addr_ip_group;
	mc_fd_rtcp_r = McOpenRead(mc_rtcp_addr_ip_group,mc_rtcp_addr_port,
		mMosaicAppData.mc_ttl);

	uc_rtp_addr_ip = mc_local_ip_addr;		/* net byte order */
	uc_fd_rtp_r = UcOpenRead(uc_rtp_addr_ip, &uc_rtp_addr_port);
/* open and bind */
	uc_rtcp_addr_ip = mc_local_ip_addr;
	uc_rtcp_addr_port = htons((ntohs(uc_rtp_addr_port)+1));
	uc_fd_rtcp_r = UcOpenRead(uc_rtcp_addr_ip, &uc_rtcp_addr_port);
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McInit: Unicast listen port, rtp %d, rtcp %d\n",
		ntohs(uc_rtp_addr_port), ntohs(uc_rtcp_addr_port));
#endif
	uc_fd_rtcp_w = uc_fd_rtcp_r; /* we dont know yet where to send */
				/* we will know when a sender send cname */
/* kind of close */
	uc_fd_rtcp_r = -1;

/* choose an SSRC */
	mc_local_srcid = McNewSrcid(mc_local_ip_addr);
	sprintf(mc_local_cname,"%s@%s/%d/%u",mMosaicAppData.author_name,
		mc_local_ip_addr_string,ntohs(uc_rtp_addr_port),rtp_init_time);
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McInit: mc_local_cname %s\n",
		mc_local_cname);
#endif

/* create the gui */
	McCreateMemberlist();
	mc_rtp_r_input_id = XtAppAddInput(mMosaicAppContext, mc_fd_rtp_r,
		(XtPointer)XtInputReadMask, McRtpReadCb, NULL);
	uc_rtp_r_input_id = XtAppAddInput(mMosaicAppContext, uc_fd_rtp_r,
		(XtPointer)XtInputReadMask, UcRtpReadCb, NULL);
	mc_rtcp_r_input_id= XtAppAddInput(mMosaicAppContext,mc_fd_rtcp_r,
		(XtPointer)XtInputReadMask, McRtcpReadCb, NULL);
	uc_rtcp_w_sdes_timer_id = XtAppAddTimeOut(mMosaicAppContext,
		uc_rtcp_w_sdes_time, UcRtcpWriteSdesCb, NULL);
/*
REMARQUES:
	At Init time , we could receive packet from mc_rtp_r or mc_rtcp_r
	It is impossible to receive packet from uc_rtp_r.
	The UcRtcpWriteSdesCb callback will send data when he know
	where to send (if we know some sender target host)
*/

/* create the virtual cursor */
	hw = win->scrolled_win;
	fg = 0;	/*hw->html.foreground_SAVE; */
	bg = 1; /*hw->html.background_SAVE; */
      	VirCursorPix = XCreatePixmapFromBitmapData (mMosaicDisplay,
                     DefaultRootWindow(mMosaicDisplay),
                       (char*)vir_cursor_bits, vir_cursor_width,
                    vir_cursor_height, fg^bg, 0, 
		DefaultDepth( mMosaicDisplay,
		              DefaultScreen( mMosaicDisplay ) 
		            ) );
	gcm = GCFunction | GCForeground | GCPlaneMask | GCBackground |
		GCSubwindowMode ;
      	gcv.function = GXxor;
     	gcv.foreground = /*fg ^ bg*/  0;
      	gcv.plane_mask = AllPlanes;
    	gcv.background = 0;
	gcv.subwindow_mode = IncludeInferiors;

	gc_vc = XCreateGC(mMosaicDisplay,
			XtWindow(mMosaicToplevelWidget),gcm,&gcv);
}

void McMoveVirtualCursor(Source *s, int x, int y)
{
	Widget hw;

	if (s->mute)
		return;

	hw = s->win->scrolled_win;

	XCopyArea(mMosaicDisplay,VirCursorPix,XtWindow(hw),
		gc_vc,0,0,vir_cursor_width,vir_cursor_height,
		s->old_cur_pos_x, s->old_cur_pos_y);

	XCopyArea(mMosaicDisplay,VirCursorPix,XtWindow(hw),
		gc_vc,0,0,vir_cursor_width,vir_cursor_height,
		x,y);
	s->old_cur_pos_x = x;
	s->old_cur_pos_y = y;
}

void McStartSender(mo_window * main_win)
{
	mc_send_win = main_win;
/* for collecting data */
	main_win->mc_callme_on_new_object = McSendNewObject; /* html or image */
	main_win->mc_callme_on_error_object = McSendNewErrorObject;
	main_win->mc_callme_on_new_state = McSendNewState; /* new state */
	mc_local_state_id = -1;

/* Initialize multicast cache for sender */
	McSenderCacheInit(mMosaicRootDirName);

/* initialise sender part */
	mc_fd_rtp_w = McOpenWrite(mc_rtp_addr_ip_group,mc_rtp_addr_port,
		mMosaicAppData.mc_ttl);
	mc_fd_rtcp_w =McOpenWrite(mc_rtcp_addr_ip_group, mc_rtcp_addr_port,
		mMosaicAppData.mc_ttl);
/* timer for writing in mc_rtcp socket: SDES and SR... */
	mc_rtcp_w_timer_id = XtAppAddTimeOut(mMosaicAppContext,
		mc_rtcp_w_time, McRtcpWriteCb, NULL);
	uc_fd_rtp_w = uc_fd_rtp_r;
	uc_fd_rtcp_r = uc_fd_rtcp_w;

/* add a callback on uc_fd_rtcp_r */
	uc_rtcp_r_input_id = XtAppAddInput(mMosaicAppContext, uc_fd_rtcp_r,
                (XtPointer)XtInputReadMask, UcRtcpReadCb, NULL);
/* we reload, and via McSendNewState and McSendNewObject , on est prevenu
 * qu'une nouvelle doc ou objet est chargee */
	mo_reload_document (main_win->scrolled_win, (XtPointer) main_win, NULL);

	XmxAdjustLabelText(mc_gui_member_list[0].label, "Xmit On");
#ifdef DEBUG_MULTICAST
	printf(" in McStartSendHyperText ... setting is_send... 0\n");
#endif
/* Remarques:
	All socket for multicast are open
	A new callback is added
*/
}

void McStopSendHyperText(mo_window * main_win)
{
	XmxAdjustLabelText(mc_gui_member_list[0].label, "Xmit Off");
}

static void McSendNewState(mo_window * win, int moid_ref, DependObjectTab dot, int ndo)
{
	MimeHeaderStruct mhs;
	int stateid =0;

	mc_local_state_id++;
	stateid = mc_local_state_id;
/* complete la partie mhs et creer un nouvelle etat */
/* L'etat est dans un cache: peut etre recupere par un nouvel arrivant */
/* qui demande un repair de cet etat */
	mhs.state_id = stateid;
	mhs.moid_ref = moid_ref;
	mhs.n_do = ndo;
	mhs.dot = dot;
/* mhs_ret.is_stateless = True;*/

	MakeSenderState(&mhs, stateid);
	McSendState(stateid);
}

/* Un nouvel objet est charge dans la fenetre d'emission */
/* fname contient les donnees recu via le protocol HTTP TCP/IP */
/* mhs contient le MIME associe a ce fichier (partie head de HTTP) */
/* aurl contient une url absolue avec les ponctuations, mais sans anchor */

static void McSendNewObject(char *fname, char *aurl, MimeHeaderStruct *mhs,
	DependObjectTab dot, int ndo, int *moid_ret)
{
	int cache_found;
	int moid = 0;
	int stateid =0;
	MimeHeaderStruct mhs_ret;
	char * fname_ret;

	cache_found = McSenderCacheFindData(aurl, &fname_ret, &mhs_ret);
	*moid_ret = mhs_ret.moid_ref;	/* un object a toujours un moid */

/* ### CAS d'une IMAGE par exemple */
/* cet objet n'a pas de dependance (objet atomique)  */
/* il ne decrit pas un etat */
/* lui attribue un MOID si il n'en a pas deja un dans le cache multicast*/
/* sinon le mettre dans le cache multicast avec son MOID */
/* et son entete mhs(multicast) dans un fichier separe. */
/* il faut faire ceci avant la decompression ou transformation du fichier
/* ### CAS d'une page HTML sans frameset => stateless = True  */
/* peut avoir des dependance */
/* ### CAS d'une page HTML dans un frame => stateless = False */
/* peut avoir des dependance */
/* ### cas d'un FRAMESET => stateless = True et dot est != NULL */
/* A obligatoirement au moins une dependance */

	if (!cache_found) {
		mc_local_object_id++;
		moid = mc_local_object_id;
		McSenderCachePutDataInCache(fname,aurl,mhs, moid, dot, ndo,
			&fname_ret, &mhs_ret);
		*moid_ret = mhs_ret.moid_ref;
	}


/* Si objet pas dans le cache , on peut maintenant envoyer l'objet */
	if (!cache_found) {
		McSendOject(moid);
	}
}


void RegisterSender(Source * s, RtpPacket *rs)
{                       
#ifdef DEBUG_MULTICAST
        fprintf(stderr, "RegisterSender\n");
#endif
        s->is_sender = True;
}

static void UcRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len;
	int status;
        unsigned char * buf;
	IPAddr addr_from;
	unsigned short port_from;
	RtpPacket rs;
	Source * s;

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UcRtpReadCb\n");
#endif
        len = UcRead(uc_fd_rtp_r, &buf, &addr_from, &port_from);
	status = DewrapRtpData(buf, len, &rs);
        if (status <= 0 ) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "error in RtpData Packet\n");
#endif
                return;
	}

	s = uc_rtp_demux( rs.ssrc, addr_from, port_from);

/* do nothing until the source is well know */
/* and not in collision */
	if ( s == NULL) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"UcRtpReadCb: NULL source\n");
#endif
		return;
	}
	if (s->is_sender == False) {
		RegisterSender(s, &rs);
	}

/* Show the source data in the window */
	switch (rs.data_type){
	case HTML_STATE_DATA_TYPE:
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"UcRtpReadCb: calling McUpdateDataSourceWithState\n");
#endif
		McUpdateDataSourceWithState(s, rs.is_eod, rs.seqn=0,
			rs.rtp_ts, rs.ssrc, rs.id, rs.offset, rs.d, rs.d_len);
		break;
	case HTML_OBJECT_DATA_TYPE:
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"UcRtpReadCb: calling McUpdateDataSourceWithObject\n");
#endif
		McUpdateDataSourceWithObject(s, rs.is_eod, rs.seqn=0,
                        rs.rtp_ts, rs.ssrc, rs.id, rs.offset, rs.d, rs.d_len);
		break;
	default:
		abort();        /* let me know !!! */
	}
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UcRtpReadCb: return\n");
#endif
}

static void McRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len;
	int status;
        unsigned char * buf;
	IPAddr addr_from;
	RtpPacket rs;
	Source * s;

        len = McRead(mc_fd_rtp_r, &buf, &addr_from);
	status = DewrapRtpData(buf, len, &rs);
        if (status <= 0 ) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "McRtpReadCb:error in RtpData Packet\n");
#endif
                return;
	}
	s = mc_rtp_demux( rs.ssrc, addr_from);

/* do nothing until the source is well know */
/* and not in collision */
	if ( s == NULL) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "McRtpReadCb: NULL source\n");
#endif
		return;
	}
	if (s->is_sender == False) {
		RegisterSender(s, &rs);
	}

	if (rs.pt == 0x63 ){	/* curseur */
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "McRtpReadCb: calling McMoveVirtualCursor x = %d, y=%d\n", rs.cur_pos_x, rs.cur_pos_y);
#endif
		McMoveVirtualCursor(s, rs.cur_pos_x, rs.cur_pos_y);
		return;
	}
/* Show the source data in the window */
	switch (rs.data_type){
	case HTML_STATE_DATA_TYPE:
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "McRtpReadCb: calling McUpdateDataSourceWithState\n");
#endif
		McUpdateDataSourceWithState(s, rs.is_eod, rs.seqn=0,
			rs.rtp_ts, rs.ssrc, rs.id, rs.offset, rs.d, rs.d_len);
		break;
	case HTML_OBJECT_DATA_TYPE:
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "McRtpReadCb: calling McUpdateDataSourceWithObject\n");
#endif
		McUpdateDataSourceWithObject(s, rs.is_eod, rs.seqn=0,
                        rs.rtp_ts, rs.ssrc, rs.id, rs.offset, rs.d, rs.d_len);
		break;
	default:
		abort();	/* let me know !!! */
	}
#ifdef DEBUG_MULTICAST
	fprintf(stderr, "McRtpReadCb: returning\n");
#endif
}
 
static void UcRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len;
	int status;
        unsigned char * buf;
	IPAddr addr_from;
	unsigned short port_from;
	Source * s;

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UcRtcpReadCb\n");
#endif
        len = UcRead(uc_fd_rtcp_r, &buf, &addr_from, &port_from);
	if (len <= 0 ) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"UcRtcpReadCb: read nothing\n");
#endif
		return;
	}
#ifdef DEBUG_MULTICAST
        fprintf(stderr,"UcRtcpReadCb: calling UcProcessRtcpData\n");
#endif
	UcProcessRtcpData(buf, len, addr_from, port_from);
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UcRtcpReadCb: return\n");
#endif
}

static void McRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len;
	int status;
        unsigned char * buf;
	IPAddr addr_from;
	Source * s;

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McRtcpReadCb\n");
#endif
        len = McRead(mc_fd_rtcp_r, &buf, &addr_from);
	if (len <= 0 ) {
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McRtcpReadCb: read nothing\n");
#endif
		return;
	}
	McProcessRtcpData(buf, len, addr_from);
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McRtcpReadCb: return\n");
#endif
}

static void McSendNewErrorObject(char *aurl, int status_code,
	int * moid_ret)
{
	int cache_found;
	char * fname_ret;
	MimeHeaderStruct mhs_ret;
	int moid;

        cache_found = McSenderCacheFindData(aurl, &fname_ret, &mhs_ret);
	*moid_ret = mhs_ret.moid_ref;   /* un object a toujours un moid */
					/* meme les erreurs !!! */
/* ### CAS d'une erreur */
/* cet objet n'a pas de dependance (objet atomique)  */
	if (!cache_found) {
		mc_local_object_id++;
		moid = mc_local_object_id;
		McSenderCachePutErrorInCache(aurl, status_code, moid,
			&fname_ret, &mhs_ret);
		*moid_ret = mhs_ret.moid_ref;
	}
/* Si objet pas dans le cache , on peut maintenant envoyer l'objet */
	if (!cache_found) {            
		McSendOject(moid);
	}                              
}

void McEmitCursor(mo_window * win, XEvent * ev)
{
	XMotionEvent * xmev;
        struct timeval tv;
	int rtp_ts;

	if ( ev->type != MotionNotify)
		return;

	gettimeofday(&tv, 0);
        rtp_ts = McRtpTimeStamp(tv); /* sample time when file come */

	xmev = &ev->xmotion;
#ifdef DEBUG_MULTICAST
	fprintf(stderr, "--------------------\n");
	fprintf(stderr,"window = %08x root = %08x subwin = %08x x = %d y = %d x_root = %d y_root = %d state = %08x is_hint = %d \n",
		xmev->window,
		xmev->root,
		xmev->subwindow,
		xmev->x,
		xmev->y,
		xmev->x_root,
		xmev->y_root,
		xmev->state,
		xmev->is_hint);
#endif
	McSendRtpCursorPosition(rtp_ts, xmev->x, xmev->y);
}
/* ###########
/*void McSendAllDataOnlyOnce(McSendDataStruct * d)
/*{
/*	int mask, omask;
/*	int i;
/*#ifdef SVR4
/*	if( sighold(SIGUSR1) != 0){
/*		perror("error in sig hold: ");
/*	}
/*#else
/*	mask = sigmask(SIGUSR1);
/*	omask = sigblock(mask);
/*#endif
/*	McSendAllDataInBandWidth(d);
/*#ifdef SVR4
/*	if( sigrelse(SIGUSR1) != 0) {
/*		perror("error in sigrelse: ");
/*	}
/*#else
/*	sigsetmask(omask);
/*#endif
/*}

/*####### case of navigation ########### */
/* navigation in history never modify a doc or object... */
/* juste update the 'RTPtimestamp' in SR. Back to the futur... */
