/* Author: Gilles Dauphin [Jan98]
 *
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 */


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
#include "../src/mime.h"
#include "../src/gui-documents.h"

#include "mc_main.h"
#include "mc_obj.h"
#include "mc_defs.h"
#include "mc_io.h"
#include "mc_session.h"
#include "mc_gui.h"

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
IPAddr		local_ip_addr;
char		*local_ip_addr_string;

IPAddr		mc_rtp_addr_ip_group;
unsigned short	mc_rtp_addr_port;
IPAddr		mc_rtcp_addr_ip_group;
unsigned short	mc_rtcp_addr_port;
IPAddr		uc_rtp_addr_ip;
unsigned short	uc_rtp_addr_port;
IPAddr		uc_rtcp_addr_ip;
unsigned short	uc_rtcp_addr_port;

			/* delay timer */
unsigned long uc_rtcp_w_sdes_time = 10000; /* in millisec */
unsigned long mc_rtcp_w_sdes_time = 2000; /* in millisec */

			/* Events */
XtInputId		mc_rtp_r_input_id;
XtInputId		mc_rtcp_r_input_id;
XtInputId		uc_rtp_r_input_id;
XtIntervalId		uc_rtcp_w_sdes_timer_id;

XtInputId		uc_rtcp_r_input_id;
XtIntervalId		mc_rtcp_w_sdes_timer_id;

		/* callbacks */
static void McRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void UcRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void McRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id);
static void UcRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id);

static void UcRtpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{ abort();}
static void UcRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{ abort();}

		/* Actions */
static void McSendNewDoc(char *fname, char *aurl_wa, MimeHeaderStruct *mhs);
static void McSendNewObject(char *fname, char *aurl_wa, MimeHeaderStruct *mhs);


DocEntry *mc_local_docs = NULL;

int mc_len_alias = 0;

int mc_local_url_id = -1;
int mc_local_object_id = -1;

/*
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/param.h>
#include <signal.h>

#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../src/gui-documents.h"
#include "mc_rtp.h"
#include "mc_misc.h"
#include "mc_action.h"
#include "vir_cursor.xbm"

static XtIntervalId 		mc_send_all_data_in_bd_time_out_id;
static XtIntervalId 		mc_send_hear_beat_time_out_id;
static XtIntervalId 		mc_send_goto_id_time_out_id;
XtIntervalId		mc_check_senderstime_time_out_id;
XtIntervalId		mc_check_rcvstime_time_out_id;
Pixmap			VirCursorPix;
GC			gc_vc;
*/

/*
McGlobalEo *		mc_global_embedded_object_tab= NULL;
unsigned int		mc_global_eo_count = 0 ;
int			mc_fdwrite;
int			mc_rtcp_fdwrite;
*/

/* initialisation of multicast datas structure */
/* called in gui.c after we open the first Mosaic window */
/* this start the multicast in listen mode only */
void McInit(mo_window * win)
{
	XGCValues gcv;
	unsigned long gcm;
	unsigned long fg;
	unsigned long bg;
	char * s;
	unsigned short port;
	struct in_addr ina;

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

	if (mMosaicAppData.mc_debug)
		printf("dest/port: %s/%d, ttl=%d\n",
			mMosaicAppData.mc_dest, port,
			mMosaicAppData.mc_ttl);
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
/*	mMosaicAppData.mc_debug; */
/*	mMosaicAppData.mc_sess_name; */
/*	mMosaicAppData.mc_media_name; */
/*	mMosaicAppData.mc_ttl; */
/*	mMosaicAppData.mc_alias_name; */
/*mc_dest become mc_rtp_addr_ip_group and mc_rtp_addr_port */
	if( mMosaicAppData.mc_alias_name == NULL ){
		mMosaicAppData.mc_alias_name = (char*) malloc(MC_MAX_SDES_NAME_LEN + 1);
		strncpy(mMosaicAppData.mc_alias_name,mMosaicAppData.author_email,
			MC_MAX_SDES_NAME_LEN);
	}
	if (strlen(mMosaicAppData.mc_alias_name) >= MC_MAX_SDES_NAME_LEN )
		mMosaicAppData.mc_alias_name[MC_MAX_SDES_NAME_LEN] = '\0';
	mc_len_alias = strlen(mMosaicAppData.mc_alias_name);
	win->mc_callme_on_new_doc = NULL;
	win->mc_callme_on_new_object = NULL;

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

	local_ip_addr = GetLocalIpAddr();
	ina.S_un.S_addr = local_ip_addr;
	local_ip_addr_string = strdup(inet_ntoa(ina));
	mc_fd_rtp_r = McOpenRead(mc_rtp_addr_ip_group,mc_rtp_addr_port,
		mMosaicAppData.mc_ttl);
	mc_rtcp_addr_ip_group = mc_rtp_addr_ip_group;
	mc_fd_rtcp_r = McOpenRead(mc_rtcp_addr_ip_group,mc_rtcp_addr_port,
		mMosaicAppData.mc_ttl);

	uc_rtp_addr_ip = local_ip_addr;		/* net byte order */
	uc_fd_rtp_r = UcOpenRead(uc_rtp_addr_ip, &uc_rtp_addr_port);
/* open and bind */
	uc_rtcp_addr_ip = local_ip_addr;
	uc_fd_rtcp_r = UcOpenRead(uc_rtcp_addr_ip, &uc_rtcp_addr_port);
	uc_fd_rtcp_w = uc_fd_rtcp_r; /* we dont know yet where to send */
				/* we will know when a sender send cname */
/* kind of close */
	uc_fd_rtcp_r = -1;

/* choose an SSRC */
	McNewSrcid(local_ip_addr);

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
}


void McStartSender(mo_window * main_win)
{
	mc_send_win = main_win;
/* for collecting data */
	main_win->mc_callme_on_new_doc = McSendNewDoc;
	main_win->mc_callme_on_new_object = McSendNewObject;
	mc_local_url_id = -1;
	mc_local_docs = NULL;	/* table of doc to send */
/* initialise sender part */
	mc_fd_rtp_w = McOpenWrite(mc_rtp_addr_ip_group,mc_rtp_addr_port,
		mMosaicAppData.mc_ttl);
	mc_fd_rtcp_w =McOpenWrite(mc_rtcp_addr_ip_group, mc_rtcp_addr_port,
		mMosaicAppData.mc_ttl);
/* timer for SDES and SR */
	mc_rtcp_w_sdes_timer_id = XtAppAddTimeOut(mMosaicAppContext,
		mc_rtcp_w_sdes_time, McRtcpWriteSdesCb, NULL);
	uc_fd_rtp_w = uc_fd_rtp_r;
	uc_fd_rtcp_r = uc_fd_rtcp_w;

/* add a callback on uc_fd_rtcp_r */
	uc_rtcp_r_input_id = XtAppAddInput(mMosaicAppContext, uc_fd_rtcp_r,
                (XtPointer)XtInputReadMask, UcRtcpReadCb, NULL);
/* we reload, and via McSendNewDoc and McSendNewObject , on est prevenu
 * qu'une nouvelle doc ou objet est chargee */
	mo_reload_document (main_win->scrolled_win, (XtPointer) main_win, NULL);
#ifdef MDEBUG
	printf(" in McStartSendHyperText ... setting is_send... 0\n");
#endif
/* Remarques:
	All socket for multicast are open
	A new callback is added
*/
}

/* we load a new doc in the sender window */
/* copy and cache data for multicast. re-arrange data */
static void McSendNewDoc(char *fname, char *aurl_wa, MimeHeaderStruct *mhs)
{
/* a new doc or reloading */
	mc_local_url_id++;
	mc_local_object_id = 0;
	McCreateDocEntry(fname, aurl_wa, mhs);
	McDocToPacket(mc_local_url_id);	/* start to send this doc */
				/* remember to re-schedule other timer */
	mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
                                mc_write_rtp_data_next_time, McSendRtpDataTimeOutCb,
                                NULL);
}


static void McSendNewObject(char *fname, char *aurl_wa, MimeHeaderStruct *mhs)
{
	abort();
/*	if( IsObjStillInCurrentDoc(aurl) ) { /* we still send it in same doc */
/*		return;			/* don't resend */
/*	}
/* a new object */
/*
	mc_local_object_id++;
	McCreateObjectEntry(fname, aurl, mhs);
/*	if ( obj_entry = IsObjStillSent(aurl,mhs) ) { /* we still sent this obj*/
/*			/* just reschedule SR. Un client est capable de
			 * determine si cette objet est dans son cache ou pas */
			/* send the header to help the client because */
			/* maybe we reload and modify this object */
/*		McScheduleSendHeaderOject(mc_local_object_id);
		return;
	}
	McScheduleSendOject(mc_local_object_id);
*/
}
/*
	mc_send_hear_beat_time_out_id = XtAppAddTimeOut(
                                mMosaicAppContext,
                                MC_SEND_HEAR_BEAT_TIME_OUT,
                                McSendHearBeatTimeOutCb,
                                NULL);
	mc_send_goto_id_time_out_id = XtAppAddTimeOut(
                                mMosaicAppContext,
                                MC_SEND_GOTO_ID_TIME_OUT,
                                McSendGotoIdTimeOutCb,
                                NULL);

/*####### case of navigation ########### */
/* navigation in history never modify a doc or object... */
/* juste update the 'RTPtimestamp' in SR. Back to the futur... */
/*McSendOldDoc(...)
{
	DocEntry *doc_entry = NULL;

	doc_entry = IsDocStillSend(aurl, mhs);
	if ( doc_entry && ! reloading ) { /* dont resend */
/*		McRescheduleSR(doc_entry);
		return;
	}
}

void McStopSendHyperText(mo_window * win)
{
	mc_send_win = NULL;
/* initialise sender part */
/*	mc_fd_rtp_w = ???;
	mc_fd_rtcp_w = ???;
	uc_fd_rtp_w = ???;
	uc_fd_rtcp_r = ???;
####
	mc_data_send_data_struct.id = 0;
	mc_local_url_id++;
	mc_data_send_data_struct.text = NULL;
	mc_data_send_data_struct.neo = 0;
	mc_data_send_data_struct.is_send = 0;
	mc_data_send_data_struct.win = NULL;
	XtRemoveTimeOut(mc_send_hear_beat_time_out_id);
	XtRemoveTimeOut(mc_send_goto_id_time_out_id);
}
*/

void RegisterSender(Source * s, RtpPacket *rs)
{                       
        fprintf(stderr, "RegisterSender\n");
                        
        s->is_sender = True;
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
		fprintf(stderr, "error in RtpData Packet\n");
                return;
	}

	s = demux( rs.ssrc, addr_from, rs.seqn);

/* do nothing until the source is well know */
/* and not in collision */
	if ( s == NULL)
		return;
	if (s->is_sender == False) {
		RegisterSender(s, &rs);
	}

/* Show the source data in the window */
	McUpdateDataSource(s, rs.is_end, rs.seqn, rs.rtp_ts, rs.ssrc,
		rs.url_id, rs.o_id, rs.offset, rs.d, rs.d_len);
}
 
static void McRtcpReadCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len;
	int status;
        unsigned char * buf;
	IPAddr addr_from;
	Source * s;

        len = McRead(mc_fd_rtcp_r, &buf, &addr_from);
	if (len <= 0 )
		return;
	ProcessRtcpData(buf, len, addr_from);
}

/* ###########
void McSendHearBeatTimeOutCb(XtPointer clid, XtIntervalId * id)
{
	unsigned int code;
	unsigned long gmt_send_time;
	unsigned int np;

#ifdef MDEBUG
	printf("in McSendHearBeatTimeOutCb \n");
#endif
	mc_send_hear_beat_time_out_id = XtAppAddTimeOut(
                                mMosaicAppContext,
                                MC_SEND_HEAR_BEAT_TIME_OUT,
                                McSendHearBeatTimeOutCb,
                                NULL);
	if (mc_data_send_data_struct.is_send == 0)
		return;	/* is it send Once ? No*/
/*
	code = MCR_HEARBEAT;
	gmt_send_time = McDate();

	np = 0;
}

void McSendAPacketCB(XtPointer clid, XtIntervalId * id)
{
#ifdef MDEBUG
	printf(" in McSendAPacketCB\n");
#endif
	if(nu < NPacketToSend){
		McSendPacket(
			Packets[nu].packet_size);
		nu++;
		mc_send_all_data_in_bd_time_out_id = XtAppAddTimeOut(
                        mMosaicAppContext,
                        _mc_timer_interval,
                        McSendAPacketCB,
                        (XtPointer) nu);
		return;
	}
	mc_data_send_data_struct.is_send = 1;	/* is it send Once ? Yes*/
/*
}

void McSendAllDataInBandWidth(McSendDataStruct * d)
{
	unsigned int nfpt,npt,lpst, seo, nombre_full_packet, nombre_packet,
			last_packet_size;
	unsigned char code;
	IPAddr 	 ipaddr;
	unsigned int url_id, nombre_eo, num_eo, num_packet, packet_size;
	unsigned long gmt_send_time;
	unsigned short pid;
	unsigned int np;
	char * data;
	int i,j;

	_mc_timer_interval = ((MC_PACKET_SIZE + 80)*8*1000) / BAND_WIDTH;
				/* time interval */
				/* en milli second */
				/* si MC_PACKET_SIZE + 80 =~ 500 octets */
/*				
	mc_send_all_data_in_bd_time_out_id = XtAppAddTimeOut(
                                mMosaicAppContext,
                                _mc_timer_interval,
                                McSendAPacketCB,
                                0);
}

void McSendAllDataOnlyOnce(McSendDataStruct * d)
{
	int mask, omask;
	int i;

/* #### don't work yet #### */
/*
#ifdef SVR4
	if( sighold(SIGUSR1) != 0){
		perror("error in sig hold: ");
	}
#else
	mask = sigmask(SIGUSR1);
	omask = sigblock(mask);
#endif

	if (d->id < mc_local_url_id){  /* ### *MUST* be based on time * fill only if it's new */
/*		McFillData( d,  mc_send_win);
		if(Packets){
			for(i = 0; i< NPacketToSend; i++)
				free(Packets[i].data);
			free(Packets);
		}
		Packets = NULL;
		NPacketToSend = 0;
	}
	d->id = mc_local_url_id;
	d->alias = mc_alias_name;
	d->url = mc_local_url;
	d->is_send = 0;
#ifdef MDEBUG
	printf(" in McSendAllDataTimeOutCb ... setting is_send... 0\n");
#endif
	McSendAllDataInBandWidth(d);
#ifdef SVR4
	if( sigrelse(SIGUSR1) != 0) {
		perror("error in sigrelse: ");
	}
#else
	sigsetmask(omask);
#endif
}

/* send a SDES packet just CNAME */
/*
static void McSendRtcpSdesCnameTimeOutCb(XtPointer clid, XtIntervalId * id)
{                       
        int mask, omask;
                                
#ifdef SVR4     
        if( sighold(SIGUSR1) != 0){
                perror("error in sig hold: ");
        }                    
#else           
        mask = sigmask(SIGUSR1);
        omask = sigblock(mask); 
#endif                          
                                
        McSendRtcpSdesCname();         /* send my cname user@host */
 /*       mc_send_rtcp_sdes_cname_time_out_id = XtAppAddTimeOut(
			mMosaicAppContext, 
			MC_SEND_RTCP_SDES_CNAME_TIME_OUT,
			McSendRtcpSdesCnameTimeOutCb,
			NULL);
#ifdef SVR4             
        if( sigrelse(SIGUSR1) != 0) {
                perror("error in sig relse: ");
        }               
#else                   
        sigsetmask(omask);             
#endif          
}       


/*############################################################# */
/*fd write multicast socket ###### */

/* now active timeout callback for receiver mode */
/* send my cname member */
/*	mc_send_rtcp_sdes_cname_time_out_id = XtAppAddTimeOut( mMosaicAppContext,
			MC_SEND_RTCP_SDES_CNAME_TIME_OUT,
			McSendRtcpSdesCnameTimeOutCb,
			NULL);
	McCreateWUserlist( win);

	mc_check_rcvstime_time_out_id = XtAppAddTimeOut( mMosaicAppContext,
			MC_CHECK_RCVSTIME_TIME_OUT,
			McCheckRcvstimeTimeOutCb,
			NULL);
	mc_check_senderstime_time_out_id = XtAppAddTimeOut( mMosaicAppContext,
			MC_CHECK_SENDERSTIME_TIME_OUT,
			McCheckSenderstimeTimeOutCb,
			NULL);
/* create the virtual cursor */
/*
	hw = (HTMLWidget) win->scrolled_win;
	fg = hw->html.foreground_SAVE;
	bg = hw->html.background_SAVE;
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
       	gcv.foreground = /*fg ^ bg*/ /* 0;
/*       	gcv.plane_mask = AllPlanes;
       	gcv.background = 0;
	gcv.subwindow_mode = IncludeInferiors;

	gc_vc = XCreateGC(mMosaicDisplay,
			XtWindow(mMosaicToplevelWidget),gcm,&gcv);
*/
/* #### */
/*
void McSetCursorPos(Widget w, int x, int y)
{
	HTMLWidget hw = (HTMLWidget) w;
	XCopyArea(mMosaicDisplay,VirCursorPix,XtWindow(hw->html.view),
		gc_vc,0,0,vir_cursor_width,vir_cursor_height,
		x,y);
}

*/
