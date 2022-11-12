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
#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"          
#include "../libhtmlw/HTML.h"         
#include "../libhtmlw/HTMLP.h"        
#include "../src/mosaic.h"            

#include "mc_mosaic.h"                  
#include "mc_rtp.h"                   
#include "mc_misc.h"                  

#ifdef DEBUG
#define DEBUG_MULTICAST
#endif

#define MC_MAX_BUF_SIZE         32767   /* max io buf size for socket */

static unsigned char ebuf[MC_MAX_BUF_SIZE+1];

/* I am a sender:
 * I send SDES packet thru multicast channel via mc_fd_rtcp_w 
 
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|    SC   |  PT=SDES=202  |             length            | header
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |                          SSRC/CSRC_1                          | chunk
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
   |                           SDES items                          |
   |                              ...                              |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
p = 0 
SC = 0 
PT = 202 
lenght = len of this RTCP packet in 32 bits words minus one, include the header
	 and padding.
SDES item: 
        CNAME (mandatory)       : user@ipquad 
        NAME                    : GCOS field 
        EMAIL                   : from mMosaic ressource 
	PHONE
	LOC
        TOOL                    : mMosaic-version 
 */     
   
#define NITEMS 4
typedef struct _SdesItem {
	int type;
	char d[256];
	int len;
} SdesItem;

#define RTCP_SDES_CNAME 1
#define RTCP_SDES_NAME 2
#define RTCP_SDES_EMAIL 3
#define RTCP_SDES_TOOL 6


void McRtcpWriteSdes()
{
	SdesItem itm[NITEMS];
	int len;
	int pad;
	int n,i,j;

/* CNAME */
	itm[0].type = RTCP_SDES_CNAME;
	sprintf(itm[0].d, "%s@%s/%d/%d",
		mMosaicAppData.author_name, mc_local_ip_addr_string,
		ntohs(uc_rtp_addr_port), rtp_init_time);
	itm[0].len= strlen(itm[0].d);

/* NAME */
	itm[1].type = RTCP_SDES_NAME;
	strcpy(itm[1].d, mMosaicAppData.author_full_name);
	itm[1].len= strlen(itm[1].d);

/* EMAIL */
	itm[2].type = RTCP_SDES_EMAIL;
	strcpy(itm[2].d, mMosaicAppData.author_email);
	itm[2].len= strlen(itm[2].d);
	
/* TOOL */
	itm[3].type = RTCP_SDES_TOOL;
	strcpy(itm[3].d, "mMosaic-3.4.6");
	itm[3].len= strlen(itm[3].d);

        ebuf[0] = 0x80;     /* V,P,SC */
        ebuf[1] = 202;      /* PT=202 (SDES) */
                                       
        ebuf[4] = (mc_local_srcid >> 24) & 0xff;    /* SSRC */
        ebuf[5] = (mc_local_srcid >> 16) & 0xff;
        ebuf[6] = (mc_local_srcid >> 8) & 0xff;
        ebuf[7] =  mc_local_srcid & 0xff;

	n = 8;
	for(i = 0; i < NITEMS; i++) {
		ebuf[n++] = itm[i].type;
		ebuf[n++] = itm[i].len;
		for(j = 0; j < itm[i].len; j++) {
			ebuf[n++] = itm[i].d[j];
		}
	}

	len = n;
	pad = 4 - (len & 3);
	len += pad;
	len = (len >> 2) - 1;
	while (--pad >= 0)
		ebuf[n++] = 0;

        ebuf[2] = (len >> 8) & 0xff;      /* length */
        ebuf[3] = len & 0xff; 

	McWrite(mc_fd_rtcp_w, ebuf, n);
}

void SendSdesCname(void)
{
	McRtcpWriteSdes(); /* ###FIXME*/
}

SdesStruct * parse_sdes(RtcpPacket* rcs)
{
	static SdesStruct sdes;
	unsigned char * psdesd = (unsigned char *) rcs->d;
	int d_len = rcs->d_len;
	unsigned int h1, h2, h3, h4;
	unsigned int h;
	int i;
	char user[257];
	unsigned short port;

	memset(&sdes, 0, sizeof(SdesStruct));
	while(d_len >3 && (psdesd[1] +2 < d_len)) {
		switch (psdesd[0]) {
		case RTCP_SDES_CNAME:
			strncpy(sdes.cname,(char*)&psdesd[2], psdesd[1]);
			d_len = d_len -2 - psdesd[1] ;
			psdesd = psdesd + psdesd[1] + 2;
			i = sscanf((char*)sdes.cname,"%[^@]@%d.%d.%d.%d/%hu/%u",
				user,
				&h1, &h2, &h3, &h4,
				&port, &rtp_init_time);
/* &bid, &s->uc_rtcp_port);*/
			if (i != 7)
				return NULL;
			h = (h1 << 24) | (h2 << 16) | (h3 << 8) | h4;
			sdes.uc_rtp_ipaddr = htonl(h);
			sdes.uc_rtp_port = htons(port);
			sdes.uc_rtcp_port = htons(port +1);
/*			d_len = d_len -2 - psdesd[1] ;
			psdesd = psdesd + psdesd[1] + 2;
*/
			break;
		case RTCP_SDES_NAME:
			strncpy(sdes.name, (char*)&psdesd[2], psdesd[1]);
			d_len = d_len -2 - psdesd[1] ;
			psdesd = psdesd + psdesd[1] + 2;
			break;
		case RTCP_SDES_EMAIL:
			strncpy(sdes.email, (char*)&psdesd[2], psdesd[1]);
			d_len = d_len -2 - psdesd[1] ;
			psdesd = psdesd + psdesd[1] + 2;
			break;
		case RTCP_SDES_TOOL:
			strncpy(sdes.tool, (char*)&psdesd[2], psdesd[1]);
			d_len = d_len -2 - psdesd[1] ;
			psdesd = psdesd + psdesd[1] + 2;
			break;
		case 0:
			return &sdes;
		default:
			fprintf(stderr, "Unknow SDES item...\n");
			return NULL;
		}
	}
	return &sdes;
}

static void McRtcpWriteStateReport()
{
	int len = 7 - 1;

	if(mc_status_report_state_id == -1 || mc_status_report_object_id == -1)
		return;

        ebuf[0] = 0x80;     /* V,P,RC */
        ebuf[1] = 200;      /* PT=200 (SR) */

	ebuf[2] = (len >> 8) & 0xff;	/* length */
	ebuf[3] = len & 0xff;
                                       
        ebuf[4] = (mc_local_srcid >> 24) & 0xff;    /* SSRC */
        ebuf[5] = (mc_local_srcid >> 16) & 0xff;
        ebuf[6] = (mc_local_srcid >> 8) & 0xff;
        ebuf[7] =  mc_local_srcid & 0xff;

	ebuf[8] = ebuf[9] = ebuf[10] = ebuf[11] = 0;   /* NTP Msb*/
	ebuf[12] = ebuf[13] = ebuf[14] = ebuf[15] = 0; /* NTP Lsb */
	ebuf[16] = ebuf[17] = ebuf[18] = ebuf[19] = 0; /* RTP ts */
	ebuf[20] = ebuf[21] = ebuf[22] = ebuf[23] = 0; /* sender's pckt count*/
	ebuf[24] = ebuf[25] = ebuf[26] = ebuf[27] = 0; /* sender's octet count*/

/* State Report */
	len = 7 -1;
	ebuf[28] = 0x80;     /* V,P,Reserved[3:7] */
	ebuf[29] = RTCP_PT_STATR;	/* mMosaic specific State Report */

	ebuf[30] = (len >> 8) & 0xff;	/* length */
	ebuf[31] = len & 0xff;
                                       
        ebuf[32] = ebuf[4];    /* SSRC */
        ebuf[33] = ebuf[5];
        ebuf[34] = ebuf[6];
        ebuf[35] = ebuf[7];

	ebuf[36] = ( mc_status_report_state_id >> 24) & 0xff; /* state_id */
	ebuf[37] = ( mc_status_report_state_id >> 16) & 0xff; /* state_id */
	ebuf[38] = ( mc_status_report_state_id >> 8) & 0xff; /* state_id */
	ebuf[39] = mc_status_report_state_id & 0xff;

	ebuf[40] = ( mc_status_report_object_id >> 24) & 0xff; /* ojectid */
	ebuf[41] = ( mc_status_report_object_id >> 16) & 0xff;
	ebuf[42] = ( mc_status_report_object_id >> 8) & 0xff;
	ebuf[43] = mc_status_report_object_id & 0xff;

	ebuf[44] = ( mc_local_state_id >> 24) & 0xff; /* current state_id */
	ebuf[45] = ( mc_local_state_id >> 16) & 0xff;
	ebuf[46] = ( mc_local_state_id >> 8) & 0xff;
	ebuf[47] = mc_local_state_id & 0xff;

/* we load this o_id at this time (use for maintaining state of FRAME)*/
	ebuf[48]= ebuf[49]= ebuf[50]= ebuf[51]= 0; /*RTP sample time of o_id */
	ebuf[52]= ebuf[53]= ebuf[54]= ebuf[55]=0;  /* reserved */

	McWrite(mc_fd_rtcp_w, ebuf, 56);
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"State Report: statid %d objid %d cuurent statid %d\n",
		mc_status_report_state_id, mc_status_report_object_id,
		mc_local_state_id);
#endif
}

void McRtcpWriteCb(XtPointer clid, XtIntervalId * time_id)
{
	McRtcpWriteSdes();
	McRtcpWriteStateReport();

	mc_rtcp_w_timer_id = XtAppAddTimeOut(mMosaicAppContext,
		mc_rtcp_w_time, McRtcpWriteCb, NULL);
}

void SendBye(int srcid, char * reason)
{
	return;		/* FIXME */
}

#endif
