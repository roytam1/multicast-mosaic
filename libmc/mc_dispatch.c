/* mc_dispatch.c
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

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <signal.h>
#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../src/mo-www.h"
#include "../src/mosaic.h"
#include "mc_rtp.h"
#include "mc_defs.h"
#include "mc_misc.h"
#include "mc_sockio.h"
#include "mc_dispatch.h"
#include "mc_action.h"
#include "vir_cursor.xbm"

McSendDataStruct 	mc_data_send_data_struct;
XtIntervalId    	mc_send_rtcp_sdes_cname_time_out_id;
mo_window * 	mc_send_win;
int      	mc_multicast_enable;
int      	mc_send_enable;

extern Widget toplevel;

typedef struct _packet_struct {
	unsigned char code;
	IPAddr ipaddr;
	unsigned short pid;
	unsigned int url_id;
	unsigned int gmt_send_time;
	unsigned int nombre_eo;
	unsigned int num_eo;
	unsigned int seo;
	unsigned int nombre_packet;
	unsigned int num_packet;
	char * data;
	unsigned int packet_size;
	unsigned int nrecover;
} PacketStruct;

typedef int PacketsIndStruct ;

static XtInputId		mc_read_socket_id;
static XtInputId		mc_rtcp_read_socket_id;
static int 			NPacketToSend = 0;
static PacketStruct * 		Packets = NULL;
static PacketsIndStruct * 	Packets_ind = NULL;
static int			Number_of_Packets_ind = 0;
static XtIntervalId 		mc_send_all_data_in_bd_time_out_id;
static XtIntervalId 		mc_send_hear_beat_time_out_id;
static XtIntervalId 		mc_send_goto_id_time_out_id;
XtIntervalId		mc_check_senderstime_time_out_id;
XtIntervalId		mc_check_rcvstime_time_out_id;
Pixmap			VirCursorPix;
GC			gc_vc;

static unsigned long 		_mc_timer_interval=1000;

#define BAND_WIDTH (100000)	/* 100 Kb/s */

McGlobalEo *		mc_global_embedded_object_tab= NULL;
unsigned int		mc_global_eo_count = 0 ;
int			mc_fdwrite;
int			mc_rtcp_fdwrite;

void McSendAllDataOnlyOnce(McSendDataStruct * d);
static void McSendRtcpSdesCnameTimeOutCb(XtPointer clid, XtIntervalId * id);
static void McSendHearBeatTimeOutCb(XtPointer clid, XtIntervalId * id);
static void McSendGotoIdTimeOutCb(XtPointer clid, XtIntervalId * id);
static void McGetEos( McSendDataStruct * d);
static void McAdjustText(McSendDataStruct * d);

void McFillData(McSendDataStruct * d, mo_window * win);

static char * html_text_part = NULL;

void McSetHtmlTexte( char *txt)
{
	if(html_text_part != NULL){
		free(html_text_part);
		html_text_part = NULL;
	}
	if (txt == NULL)
		return;
	html_text_part = strdup(txt);
}

McGlobalEo * McGetEmptyGlobalObject()
{
	McGlobalEo * new_geo;
	int i;

	mc_global_eo_count++;
	new_geo = (McGlobalEo *) malloc(mc_global_eo_count * sizeof(McGlobalEo));
	for(i = 0; i< (mc_global_eo_count -1); i++){
		new_geo[i] = mc_global_embedded_object_tab[i];
	}
	if (mc_global_eo_count != 1)
		free(mc_global_embedded_object_tab);
	mc_global_embedded_object_tab = new_geo;
	return &mc_global_embedded_object_tab[mc_global_eo_count-1] ;
}



/* initialisation of multicast datas structure */
/* called in gui.c after we open the first Mosaic window */
/* this start the multicast in listen mode only */
void McInit(mo_window * win)
{
	XGCValues gcv;
	unsigned long gcm;
        HTMLWidget hw;
	unsigned long fg;
	unsigned long bg;

	mc_send_enable = 0;	/* On demare en mode lecture */
        mc_send_win = NULL;
        if ( mc_multicast_enable ){
                mc_read_socket_id = XtAppAddInput(app_context, mc_fdread,
                        (XtPointer)XtInputReadMask,
                        McReadSocketCb, NULL);
		XtVaSetValues(win->scrolled_win, 
                                WbNmctype, MC_MO_TYPE_MAIN, 
                                NULL);
                mc_rtcp_read_socket_id=XtAppAddInput(app_context,mc_rtcp_fdread,
                        (XtPointer)XtInputReadMask,
                        McReadRtcpSocketCb, NULL);

/*fd write multicast socket*/
		mc_fdwrite = McOpenWrite(mc_addr_ip_group,mc_port,mc_ttl);
		mc_rtcp_fdwrite = McOpenRtcpWrite(mc_addr_ip_group,
					mc_rtcp_port,mc_ttl);

/* now active timeout callback for receiver mode */
/* send my cname member */
		mc_send_rtcp_sdes_cname_time_out_id = XtAppAddTimeOut(
				app_context,
				MC_SEND_RTCP_SDES_CNAME_TIME_OUT,
				McSendRtcpSdesCnameTimeOutCb,
				NULL);
		McCreateWUserlist( win);

		mc_check_rcvstime_time_out_id = XtAppAddTimeOut(
				app_context,
				MC_CHECK_RCVSTIME_TIME_OUT,
				McCheckRcvstimeTimeOutCb,
				NULL);
		mc_check_senderstime_time_out_id = XtAppAddTimeOut(
				app_context,
				MC_CHECK_SENDERSTIME_TIME_OUT,
				McCheckSenderstimeTimeOutCb,
				NULL);
/* create the virtual cursor */
		hw = (HTMLWidget) win->scrolled_win;
		fg = hw->html.foreground_SAVE;
		bg = hw->html.background_SAVE;
        	VirCursorPix = XCreatePixmapFromBitmapData (XtDisplay(toplevel),
                        DefaultRootWindow(XtDisplay(toplevel)),
                        (char*)vir_cursor_bits, vir_cursor_width,
                        vir_cursor_height, fg^bg, 0, 
			DefaultDepth( XtDisplay(toplevel),
			              DefaultScreen( XtDisplay(toplevel) ) 
			            ) );
		gcm = GCFunction | GCForeground | GCPlaneMask | GCBackground |
			GCSubwindowMode ;
        	gcv.function = GXxor;
        	gcv.foreground = /*fg ^ bg*/0;
        	gcv.plane_mask = AllPlanes;
        	gcv.background = 0;
		gcv.subwindow_mode = IncludeInferiors;

		gc_vc = XCreateGC(XtDisplay(toplevel),
				XtWindow(toplevel),gcm,&gcv);
        }
}
void McSetCursorPos(Widget w, int x, int y)
{
	HTMLWidget hw = (HTMLWidget) w;
	XCopyArea(XtDisplay(toplevel),VirCursorPix,XtWindow(hw->html.view),
		gc_vc,0,0,vir_cursor_width,vir_cursor_height,
		x,y);
}

void McStartSendHyperText(mo_window * main_win)
{
/* Collect the data */
	mc_send_win = main_win;
	mo_reload_window_text (main_win, 0);
	mc_data_send_data_struct.id = 0;
	mc_local_url_id++;
	mc_data_send_data_struct.text = NULL;
	mc_data_send_data_struct.neo = 0;
#ifdef MDEBUG
	printf(" in McStartSendHyperText ... setting is_send... 0\n");
#endif
	mc_data_send_data_struct.is_send = 0;	/* is it send Once ? */
	mc_data_send_data_struct.win = mc_send_win;
	McFillData(&mc_data_send_data_struct,main_win);

	McSendAllDataOnlyOnce(&mc_data_send_data_struct);
	mc_send_hear_beat_time_out_id = XtAppAddTimeOut(
                                app_context,
                                MC_SEND_HEAR_BEAT_TIME_OUT,
                                McSendHearBeatTimeOutCb,
                                NULL);
	mc_send_goto_id_time_out_id = XtAppAddTimeOut(
                                app_context,
                                MC_SEND_GOTO_ID_TIME_OUT,
                                McSendGotoIdTimeOutCb,
                                NULL);
}

void McStopSendHyperText(mo_window * win)
{
	mc_data_send_data_struct.id = 0;
	mc_local_url_id++;
	mc_data_send_data_struct.text = NULL;
	mc_data_send_data_struct.neo = 0;
	mc_data_send_data_struct.is_send = 0;
	mc_data_send_data_struct.win = NULL;
	mc_send_win = NULL;
	XtRemoveTimeOut(mc_send_hear_beat_time_out_id);
	XtRemoveTimeOut(mc_send_goto_id_time_out_id);
}

/* il faut remplir une structure de donnee pour pouvoir
 * les multicaster plus tard...
 * Ceci est fait par McSendAllDataTimeOutCb
 * Cette structure doit etre rempli quand on a eu l'hypertexte
 * de facon 'normal' (par une connexion unicast) et qu'il est complet
 * (on a aussi les donnes 'images' ainsi que ses dimensions) c.a.d.
 * tout pour eviter une connexion unicast par le client qui recoit
 * les donnes multicasts. Au besoin modifier l'hyper-texte
 * pour completer les donnees manquantes (comme la taille d'une image GIF)
 */
void McFillData(McSendDataStruct * d, mo_window * win)
{
	int i;

	/* l'hypertext doit etre stocke en global qqe part */
	/* faut le trouver ... */
	/* HTMainText stocke l'hypertexte */
	/* Mosaic stocke le texte dans HTMainText. */
	/* 	struct _HText {
	 *		char *expandedAddress;
	 *		char *simpleAddress;
	 *		char *htmlSrc;   * a 'parser'. se termine par '\0' *
	 *		char *htmlSrcHead;     * This is what we should free.*
	 *		int srcalloc;    * amount of space allocated *
	 *		int srclen;      * amount of space used(len of htmlSrc *
	 *	};
	 * le texte est dans HTMainText->htmlSrc;
	 */

/*	HTMLPart * htmlptr = McGetInternalHtmlPart( win->scrolled_win); */

	/* parcourir htmlptr et selection de ce qui interesse */
	/* les mettre dans 'd'. Il suffit apres de balancer la puree */
	/* voir dans ImageResolve si on peut pas recuprer des donnes */
	/* liberer l'espace si alloue */

	/*d->id = ??? id de l'url ;	unsigned int	 */
	/*d->alias = l'email du mec ;	char *	 	*/
	/*d->url = l'url;		char *	 	*/
	/*d->text = l'hypertext;	char *	 	*/
	/*d->neo = nombre d'objet;	unsigned int 	 */
	/*d->seo = tableau de taile d'objet; unsigned int *	 */
	/*d->eos= tableau d'objet	char ** ou d->eos[i] = ???	 */
				/* eo[neo]. Each element is data of eo*/
                                /* but :                                */
                                /*    eo[0] <=> alias                   */
                                /*    eo[1] <=> URL                     */
                                /*    eo[1] <=> text                    */


/*###### */
	d->id = mc_local_url_id;
	d->alias = mc_alias_name;
	d->url = mc_local_url;
#ifdef MDEBUG
	printf(" in McFillData ... setting is_send... 0\n");
#endif
	d->is_send = 0;
	/* ajuste l'hyper texte pour en donner un autre avec les infos */
	/* en notre possession et connues (exemple IMG) */
	/* le resultat est dans d->text */
	/*if ( d->text != NULL) free(d->text);*/

	if (html_text_part){
		McAdjustText ( d);
		McGetEos( d);
	}

	if(Packets){
		for(i = 0; i< NPacketToSend; i++)
			free(Packets[i].data);
		free(Packets);
	}
	if(Packets_ind){
		free(Packets_ind);
	}
	Packets_ind =NULL;
	Number_of_Packets_ind = 0;
	Packets = NULL;
	NPacketToSend = 0;
}

static void McAdjustText( McSendDataStruct * d)
{
	if (html_text_part == NULL){
		d->text = strdup("<P>\nSender Multicast a wrong URL\n</P>");
		return;
	}
	d->text = strdup(html_text_part);
}


static void McGetEos( McSendDataStruct * d)
{
	int i,j;

	if (d->neo != 0){
		free(d->seo);
		for(i = 0; i< d->neo; i++){
			free(d->eos[i]);
			d->eos[i] = NULL;
		}

		free(d->eos);
		d->seo = NULL;
		d->eos = NULL;
	}
	
	d->neo = 2+ mc_global_eo_count;	/* 3 is juste to say it's a minimum */
#ifdef MDEBUG
	printf("McGetEos: d->neo = %d\n",d->neo);
#endif
	d->seo = (unsigned int *) malloc( d->neo * sizeof(unsigned int ));
	d->eos = (char **) malloc(d->neo * sizeof(char*));

	d->eos[0] = strdup(mc_local_url);
	d->seo[0] = mc_len_local_url;
	d->eos[1] = strdup(d->text);
	d->seo[1] = strlen(d->text);

	/* fill with the global_eo */
	for(i=2, j=0; j < mc_global_eo_count; i++, j++){
		d->seo[i] = mc_global_embedded_object_tab[j].len_eo;
		d->eos[i] = mc_global_embedded_object_tab[j].orig_data;
	}
}

void McReadSocketCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len,lend,lencur;
        unsigned char * buf;
        int ibuf = 0;           /* index in buf */
	Mcs_gotodata gotodata;
	Mcs_alldata alldata;
	McRtpGotoIdDataStruct html_goto_id_data;
	McRtpCursorPosDataStruct cursor_pos_data;
	unsigned int rtp_code;
	IPAddr ipfrom;

        len = McGetRecvBuf(&buf, &ipfrom);
        if (len <= 0 ) 
                return;

        lencur = len;
	if ( len < 16)		/* minimu rtp header for mMosaic */
		return;
	if ( ((buf[0] << 8 ) |buf[1]) != RTP_CONST_HPT_WEB) /* T:2 P:1 X:1 CC:4 M:1 PT:7 */
		return;
        while(len > ibuf){
		rtp_code = buf[ibuf+12];

#ifdef MDEBUG
			printf("len of data = %d\n",lend);
		       printf("receive from %d.%d.%d.%d\n", (ipfrom >> 24) & 0xff,
				(ipfrom >> 16) & 0xff,
				(ipfrom >> 8) & 0xff,
				(ipfrom ) & 0xff);
#endif
                switch(rtp_code){
                case MCR_ALLDATA:
			lend = McCheckAllData(&buf[ibuf],lencur, &alldata,ipfrom);
			if(!lend) return;
			ibuf += lend;
			lencur -= lend;
			McActionAllData(&alldata);
			break;
		case MCR_HEARBEAT:
			lend = McCheckAllData(&buf[ibuf],lencur, &alldata,ipfrom);
			if(!lend) return;
			ibuf += lend;
			lencur -= lend;
			McActionHearBeatData(&alldata);
			break;
		case MCR_HTML_GOTO_ID:
			lend = McCheckGotoId(&buf[ibuf],lencur, &html_goto_id_data,ipfrom);
			if(!lend) return;
			ibuf += lend;
			lencur -= lend;
			McActionGotoIdData(&html_goto_id_data);
			break;
		case MCR_CURSOR_POS:
			lend = McCheckCursorPos(&buf[ibuf],lencur, &cursor_pos_data,ipfrom);
			if(!lend) return;
			ibuf += lend;
			lencur -= lend;
			McActionCursorPosData(&cursor_pos_data);
			break;

                default:
                        fprintf(stderr,"RTPError code reading buffer\n");
                        return;
                } /* switch */
        }/* while */
}

static void McSendGotoIdTimeOutCb(XtPointer clid, XtIntervalId * id)
{
	unsigned char code;
	unsigned long gmt_send_time;
	unsigned int html_goto_id;

#ifdef MDEBUG
	printf("in McSendGotoIdTimeOutCb \n");
#endif
	mc_send_goto_id_time_out_id = XtAppAddTimeOut(
                                app_context,
                                MC_SEND_GOTO_ID_TIME_OUT,
                                McSendGotoIdTimeOutCb,
                                NULL);
	if (mc_data_send_data_struct.is_send == 0)
		return;	/* is it send Once ? No*/

#ifdef MDEBUG
	printf("McSendGotoIdTimeOutCb ... sending ...\n");
#endif
	code = MCR_HTML_GOTO_ID;
	gmt_send_time = McDate();

/* get the html_goto_id */

	html_goto_id = HTMLPositionToId(mc_send_win->scrolled_win, 0, 3);

        McSendRtpGotoId( code, mc_my_pid, mc_local_url_id,
              gmt_send_time, html_goto_id);

}

void McSendHearBeatTimeOutCb(XtPointer clid, XtIntervalId * id)
{
	unsigned int code;
	unsigned long gmt_send_time;
	unsigned int np;

#ifdef MDEBUG
	printf("in McSendHearBeatTimeOutCb \n");
#endif
	mc_send_hear_beat_time_out_id = XtAppAddTimeOut(
                                app_context,
                                MC_SEND_HEAR_BEAT_TIME_OUT,
                                McSendHearBeatTimeOutCb,
                                NULL);
	if (mc_data_send_data_struct.is_send == 0)
		return;	/* is it send Once ? No*/

#ifdef MDEBUG
	printf("McSendHearBeatTimeOutCb ... sending ...\n");
#endif
	code = MCR_HEARBEAT;
	gmt_send_time = McDate();

	np = 0;

	/* L'URL */

/*	Packets[np].code = code; */
/*	Packets[np].ipaddr = ipaddr;*/
/*	Packets[np].pid = pid;*/
/*	Packets[np].url_id = url_id;*/
	Packets[np].gmt_send_time = gmt_send_time;
/*	Packets[np].nombre_eo = nombre_eo;*/
/*	Packets[np].num_eo = num_eo;*/
/*	Packets[np].seo = seo;*/
/*	Packets[np].nombre_packet = nombre_packet;*/
/*	Packets[np].num_packet = num_packet;*/
/*	Packets[np].data = strdup(data);*/
/*	Packets[np].packet_size = seo;*/
	Packets[np].nrecover = 0;	/* number of recover request */
/* si il y a un nouvelle arrivant il va demande l'integralite des datas*/
/* par un packet NACK_ALL */
        McSendPacket(
              code,
              Packets[np].pid,
              Packets[np].url_id,
              Packets[np].gmt_send_time,
              Packets[np].nombre_eo,
              Packets[np].num_eo,
              Packets[np].seo,
              Packets[np].nombre_packet,
              Packets[np].num_packet,
              Packets[np].data,
              Packets[np].packet_size);

}

void McSendAPacketCB(XtPointer clid, XtIntervalId * id)
{
	int nu = (int) clid;		/* numero du packet a envoyer */

#ifdef MDEBUG
	printf(" in McSendAPacketCB\n");
#endif
	if(nu < NPacketToSend){
		McSendPacket(
			Packets[nu].code,
			Packets[nu].pid,
			Packets[nu].url_id,
			Packets[nu].gmt_send_time,
			Packets[nu].nombre_eo,
			Packets[nu].num_eo,
			Packets[nu].seo,
			Packets[nu].nombre_packet,
			Packets[nu].num_packet,
			Packets[nu].data,
			Packets[nu].packet_size);
		nu++;
		mc_send_all_data_in_bd_time_out_id = XtAppAddTimeOut(
                        app_context,
                        _mc_timer_interval,
                        McSendAPacketCB,
                        (XtPointer) nu);
#ifdef MDEBUG
		printf(" in McSendAPacketCB ... sending ...\n");
#endif
		return;
	}
#ifdef MDEBUG
	printf(" in McSendAPacketCB ... setting is_send... 1\n");
#endif
	mc_data_send_data_struct.is_send = 1;	/* is it send Once ? Yes*/
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

/* compute a bandwidth */
/* calcul la taille des donnees */
/* calcul exact du nombre de packet */
	NPacketToSend = 1;			/* l'url */
	if(d->neo== 1){
		return;		/* ##### pour l'instant */
	}

	nfpt = npt = (strlen(d->text)+1)/MC_PACKET_SIZE;
	lpst = (strlen(d->text)+1) % MC_PACKET_SIZE;
	if(lpst)
		npt++;

	NPacketToSend = NPacketToSend + npt;		/* le texte */

	nombre_packet = 0;
	for(j=2; j< d->neo; j++){	/* les EOs */
		seo = d->seo[j];
		nombre_full_packet = seo / MC_PACKET_SIZE ;
		nombre_packet = nombre_packet + nombre_full_packet;
		last_packet_size = seo % MC_PACKET_SIZE;
		if ( last_packet_size != 0)
			nombre_packet++;
	}
	NPacketToSend = NPacketToSend + nombre_packet;

/* copie de la structure 'd' */
/* rearrangement dans un tableau de packet */
	if(!Packets){ /* alloc et copie */
		Packets = (PacketStruct *) malloc(sizeof(PacketStruct) *
						NPacketToSend);
		Packets_ind = (PacketsIndStruct *) malloc(sizeof(PacketsIndStruct) * d->neo);
		Number_of_Packets_ind = d->neo;
		code = MCR_ALLDATA;
		ipaddr = mc_local_ip_addr;
		pid = mc_my_pid; 
		url_id = mc_local_url_id;
		gmt_send_time = McDate();
		nombre_eo = d->neo;     

		np = 0;

		/* L'URL */
		num_eo = 0;             /* l'url */     
		seo = mc_len_local_url;                 
		nombre_packet = 1;                      
		num_packet = 0; 
		data = mc_local_url;                    

		Packets_ind[num_eo]= np;
		Packets[np].code = code;
		Packets[np].ipaddr = ipaddr;
		Packets[np].pid = pid;
		Packets[np].url_id = url_id;
		Packets[np].gmt_send_time = gmt_send_time;
		Packets[np].nombre_eo = nombre_eo;
		Packets[np].num_eo = num_eo;
		Packets[np].seo = seo;
		Packets[np].nombre_packet = nombre_packet;
		Packets[np].num_packet = num_packet;
		Packets[np].data = strdup(data);
		Packets[np].packet_size = seo;
		Packets[np].nrecover = 0;	/* number of recover request */
		np++;
        
        	if(d->neo < 2)                          
                	return;                         
                                                
		num_eo = 1;             /* le texte */  
		seo = strlen(d->text)+1;
		nombre_full_packet = nombre_packet = seo / MC_PACKET_SIZE ;
		packet_size = MC_PACKET_SIZE;           
		last_packet_size = seo % MC_PACKET_SIZE;
		if ( last_packet_size != 0)
			nombre_packet ++;
		data = d->text;
		num_packet = 0;
		Packets_ind[num_eo]= np;
        	for (i = 0 ; i < nombre_full_packet; i++){
			Packets[np].code = code;
			Packets[np].ipaddr = ipaddr;
			Packets[np].pid = pid;
			Packets[np].url_id = url_id;
			Packets[np].gmt_send_time = gmt_send_time;
			Packets[np].nombre_eo = nombre_eo;
			Packets[np].num_eo = num_eo;
			Packets[np].seo = seo;
			Packets[np].nombre_packet = nombre_packet;
			Packets[np].num_packet = num_packet;
			Packets[np].data = (char*)malloc(packet_size);
			memcpy(Packets[np].data, data, packet_size);
			Packets[np].packet_size = packet_size;
			Packets[np].nrecover = 0; /* number of recover request */

                	data = data + MC_PACKET_SIZE;
                	num_packet++;          
			np++;
		}
		if ( last_packet_size != 0){   
			Packets[np].code = code;
			Packets[np].ipaddr = ipaddr;
			Packets[np].pid = pid;
			Packets[np].url_id = url_id;
			Packets[np].gmt_send_time = gmt_send_time;
			Packets[np].nombre_eo = nombre_eo;
			Packets[np].num_eo = num_eo;
			Packets[np].seo = seo;
			Packets[np].nombre_packet = nombre_packet;
			Packets[np].num_packet = num_packet;
			Packets[np].data = (char*)malloc(last_packet_size);
			memcpy(Packets[np].data, data, last_packet_size);
			Packets[np].packet_size = last_packet_size;
			Packets[np].nrecover = 0; /* number of recover request */
			np++;
		}
                                       
		for(j=2; j< nombre_eo; j++){            /* le EO */
			seo = d->seo[j];       
			nombre_full_packet = nombre_packet = seo /MC_PACKET_SIZE ;
			last_packet_size = seo % MC_PACKET_SIZE;
			if ( last_packet_size != 0)  
				nombre_packet ++;
			data = d->eos[j];      
			num_packet = 0;        
			Packets_ind[j]= np;
			for (i = 0 ; i < nombre_full_packet; i++){
				Packets[np].code = code;
				Packets[np].ipaddr = ipaddr;
				Packets[np].pid = pid;
				Packets[np].url_id = url_id;
				Packets[np].gmt_send_time = gmt_send_time;
				Packets[np].nombre_eo = nombre_eo;
				Packets[np].num_eo = j;
				Packets[np].seo = seo;
				Packets[np].nombre_packet = nombre_packet;
				Packets[np].num_packet = num_packet;
				Packets[np].data = (char*)malloc(MC_PACKET_SIZE);
				memcpy(Packets[np].data, data, MC_PACKET_SIZE);
				Packets[np].packet_size = MC_PACKET_SIZE;
				Packets[np].nrecover = 0;/*# of recover request */

                        	num_packet++;  
                        	data = data + MC_PACKET_SIZE;
				np++;
			}                      
			if ( last_packet_size != 0){ 
				Packets[np].code = code;
				Packets[np].ipaddr = ipaddr;
				Packets[np].pid = pid;
				Packets[np].url_id = url_id;
				Packets[np].gmt_send_time = gmt_send_time;
				Packets[np].nombre_eo = nombre_eo;
				Packets[np].num_eo = j;
				Packets[np].seo = seo;
				Packets[np].nombre_packet = nombre_packet;
				Packets[np].num_packet = num_packet;
				Packets[np].data =(char*)malloc(last_packet_size);
				memcpy(Packets[np].data, data, last_packet_size);
				Packets[np].packet_size = last_packet_size;
				Packets[np].nrecover =0;/* # of recover request */
				np++;
                	}                      
        	}              
	}
	
	_mc_timer_interval = ((MC_PACKET_SIZE + 80)*8*1000) / BAND_WIDTH;
				/* time interval */
				/* en milli second */
				/* si MC_PACKET_SIZE + 80 =~ 500 octets */
				/* si BAND_WIDTH = 100kbit/second */
				/* alors _mc_timer_interval = 40 milli second */
				
	mc_send_all_data_in_bd_time_out_id = XtAppAddTimeOut(
                                app_context,
                                _mc_timer_interval,
                                McSendAPacketCB,
                                0);
}

void McSendAllDataOnlyOnce(McSendDataStruct * d)
{
	int mask, omask;
	int i;

/* #### don't work yet #### */

#ifdef SVR4
	if( sighold(SIGUSR1) != 0){
		perror("error in sig hold: ");
	}
#else
	mask = sigmask(SIGUSR1);
	omask = sigblock(mask);
#endif

	if (d->id < mc_local_url_id){  /* ### *MUST* be based on time * fill only if it's new */
		McFillData( d,  mc_send_win);
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
        mc_send_rtcp_sdes_cname_time_out_id = XtAppAddTimeOut(
			app_context, 
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
                
void McReadRtcpSocketCb(XtPointer clid, int * fd, XtInputId * input_id)
{
        int len,lend,lencur;
        unsigned char * buf;
        int ibuf = 0;           /* index in buf */
	int rtcp_code;
	int lrmp_code;
	McRtcpSdesCnameDataStruct rtcp_sdes_cname_data;
	McRtcpLrmpNackDataStruct rtcp_lrmp_nack_data;
	McRtcpLrmpNackAllDataStruct rtcp_lrmp_nack_all_data;
	McRtcpByeDataStruct rbye;
	IPAddr ipfrom;

        len = McGetRtcpRecvBuf(&buf,&ipfrom);
        if (len <= 0 ) 
                return;

        lencur = len;

        while(len > ibuf){
		if (len <= ibuf+1)
			return;
		rtcp_code = buf[ibuf+1];
#ifdef MDEBUG
			printf("len of data = %d\n",lend);
#endif

                switch(rtcp_code){
                case RTCP_PT_SDES:
/* ### just now we check only the cname */
			lend = McCheckRtcpSdesCname(&buf[ibuf], lencur, &rtcp_sdes_cname_data, ipfrom );
			if(!lend) return;
			ibuf += lend;
			lencur -= lend;
/* ##############################################################*/
			McActionRtcpSdesCnameData(&rtcp_sdes_cname_data);
			break;
		case RTCP_PT_LRMP:
			lrmp_code = buf[ibuf] & 0x1f;
			switch (lrmp_code){
			case LRMP_ECHO:
			case LRMP_ECHO_ACK:
			case LRMP_SYNC:
			case LRMP_SYNC_ERROR:
				printf("LRMP echo echo_ack sync sync_error:not yet def.\n");
				return;
			case LRMP_NACK:
				lend = McCheckRtcpLrmpNack(&buf[ibuf], lencur, &rtcp_lrmp_nack_data, ipfrom );
				if(!lend) return;
				ibuf += lend;
				lencur -= lend;
				McActionRtcpLrmpNackData(&rtcp_lrmp_nack_data);
				break;
			case LRMP_NACK_ALL:
				lend = McCheckRtcpLrmpNackAll(&buf[ibuf], lencur, &rtcp_lrmp_nack_all_data, ipfrom );
				if(!lend) return;
				ibuf += lend;
				lencur -= lend;
/* ##############################################################*/
				McActionRtcpLrmpNackAllData(&rtcp_lrmp_nack_all_data);
				break;

			default:
				fprintf(stderr,"LRMP Error code\n");
                        	return;
			}
			break;

		case RTCP_PT_BYE:
                        lend = McCheckRtcpBye(&buf[ibuf], lencur, &rbye , ipfrom);
                        if(!lend) return;
                        ibuf += lend;
                        lencur -= lend;
                        McActionRtcpByeData(&rbye);
			printf("RTCP receive BYE.\n"); 
                        break;
		case RTCP_PT_SR:
		case RTCP_PT_RR:
		case RTCP_PT_APP:
			printf("RTCP receive SR or RR or APP: not yet def.\n");
			return;
                default:
                        fprintf(stderr,"RTCP Error code reading buffer\n");
                        return;
                } /* switch */
        }/* while */
}
void McSendFastAllEoData(int r_num_eo)
{
	int deb;
	int n;
	int i;

	if ( r_num_eo >= Number_of_Packets_ind){
		fprintf(stderr,"Error McSendFastAllEoData (Pirate?)\n");
		return;
	}
	deb = Packets_ind[r_num_eo];
	n = Packets[deb].nombre_packet;

	for(i=0; i<n; i++){
		McSendPacket(
                        Packets[deb].code,
              		Packets[deb].pid,
              		Packets[deb].url_id,
              		Packets[deb].gmt_send_time,
              		Packets[deb].nombre_eo,
              		Packets[deb].num_eo,
              		Packets[deb].seo,  
              		Packets[deb].nombre_packet,
              		Packets[deb].num_packet, 
              		Packets[deb].data,
              		Packets[deb].packet_size); 
		deb++;
	}
}

void McSendFastPacketEoData(int r_num_eo,int r_fpno)
{
	int n;

	if ( r_num_eo >= Number_of_Packets_ind){
		fprintf(stderr,"Error McSendFastPacketEoData (Pirate?)\n");
		return;
	}
	n = Packets_ind[r_num_eo] + r_fpno;

	if( r_fpno >= Packets[Packets_ind[r_num_eo]].nombre_packet){
		fprintf(stderr,"Error McSendFastPacketEoData/n (Pirate?)\n");
		return;
	}

	McSendPacket(
                Packets[n].code,
       		Packets[n].pid,
       		Packets[n].url_id,
       		Packets[n].gmt_send_time,
       		Packets[n].nombre_eo,
       		Packets[n].num_eo,
       		Packets[n].seo,  
       		Packets[n].nombre_packet,
       		Packets[n].num_packet, 
       		Packets[n].data,
       		Packets[n].packet_size); 
}

#endif /* MULTICAST */
