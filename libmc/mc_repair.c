#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/XmAll.h>
#include "../libnut/mipcf.h"
#include "../src/mosaic.h"

#include "mc_mosaic.h"
#include "mc_rtp.h"
#include "mc_gui.h"
#include "mc_main.h"

void UcSendRepair(Source *s, int url_id, int o_id, int offset, int len);
void McRepairWithChk(Source *s, int url_id, int o_id);

/* 
	url_id | o_id
	len
*/
typedef struct _RtcpStard {
	u_int16_t	url_id;
	u_int16_t	o_id;
	u_int32_t	len;
} RtcpStard;

void McRepairFromStatr(Source *s, RtcpPacket* rcs)
{
	RtcpStard *psrd = (RtcpStard *) rcs->d;
	int url_id;
	int o_id;
	int lvu,lvo;
	int i,j;
	DocEntry * doce;
	int to_oid;

	if(s->mute)
		return;
	url_id = ntohs (psrd->url_id);
	o_id = ntohs (psrd->o_id);
/*	len = ntohl(psrd->len); ### faire qqes chose avec ca */

/*
	lvu = s->last_valid_url_id;	
	lvo = s->last_valid_o_id;
*/
	lvu = 0;
	lvo = 0;
	if( !SourceAllocDocs(s, url_id)) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}

	for ( i = lvu; i <= s->huid; i++) { /* Analyse and send repair... */
		doce = s->doc[i];
		if (doce->nobj == 0) {
			/* no html yet. repair with chunkObject */
			McRepairWithChk(s, i, 0); /* work with chkobj */
			continue;
		}
		if ( i == url_id) {	/* do not send paste the o_id from STATR */
			to_oid = o_id;
		} else {
			to_oid = doce->nobj -1;
		}
		for(j = 1; j<= to_oid; j++){
			if (doce->o_tab[j])
				continue;
			McRepairWithChk(s, i, j); /* work with chkobj */
		}
	}
}

void McRepairWithChk(Source *s, int url_id, int o_id)
{
	int len;
	int offset;
	int status;
	MissRange * plmr;
	ChunkedDocEntry * cdocent;
	ChunkedObjEntry * coe;

	status = SourceAllocObjs(s, url_id, o_id); 
	if ( status == -1) {    /*invalide object number */ 
		fprintf(stderr, "Invalid object number on recept\n"); 
		return;  
	} 
	if ( ! status ) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}                              
	cdocent = s->chkdoc[url_id];
	coe = cdocent->co_tab[o_id];
	if (coe->lpdc != NULL) {
		/* request mime */
		UcSendRepair(s, url_id, o_id, offset=0, len=1);
		return;
	}
	plmr = coe->lmr;
	while(plmr){
		offset=plmr->from;
		len = plmr->to - offset +1;
		if (plmr->to == 0xffffffff)
			len = 0xffffffff;
		UcSendRepair(s, url_id, o_id, offset,len);
		plmr=plmr->next;
	}
}

void UcSendRepair(Source *s, int url_id, int o_id, int offset, int lend)
{
	struct sockaddr_in addr_w;
	unsigned char buf[20];
	int cnt;
	int len;

	if (!s->uc_rtcp_port)	/* no unicast rctp port yet */
		return;

	memset(&addr_w,0,sizeof(addr_w));
	addr_w.sin_family = AF_INET;
	addr_w.sin_port = s->uc_rtcp_port; /* netbyte order */
	addr_w.sin_addr.s_addr = s->uc_rtp_ipaddr;   /* net byteorder */


	buf[0] = 0x80 ;	 /* V[0:1] , P[2], reserved[3:7] */
	buf[1] = RTCP_PT_REPAIR; /* PT = RTCP_PT_REPAIR */
	buf[2] = 0;
	buf[3] = 4;	/* 5 - 1 Word */

	buf[4] = (mc_local_srcid >> 24) & 0xff;    /* SSRC */  
	buf[5] = (mc_local_srcid >> 16) & 0xff;
	buf[6] = (mc_local_srcid >> 8) & 0xff;
	buf[7] =  mc_local_srcid & 0xff;

        buf[8] = ( url_id >> 8) & 0xff;        /* url_id */
        buf[9] = url_id & 0xff;
                                      
        buf[10] = (o_id >> 8) & 0xff;   /* object_id */
        buf[11] = o_id & 0xff;
                                      
        buf[12] = ( offset >> 24) & 0xff;       /* offset */
        buf[13] = ( offset >> 16) & 0xff;
        buf[14] = ( offset >> 8) & 0xff;
        buf[15] = offset & 0xff;

        buf[16] = (lend >> 24) & 0xff;   /* lend */
        buf[17] = ( lend >> 16) & 0xff;
	buf[18] = ( lend >> 8) & 0xff;
	buf[19] = lend & 0xff;

	len =20;
	cnt = sendto(uc_fd_rtcp_w, (char*)buf, len, 0,
		(struct sockaddr *) &addr_w, sizeof(addr_w));
	if(cnt != len){
		perror("UcSendRepair:sendto:");
	}
	fprintf(stderr,"UcSendRepair to %08x %04x\n",
		ntohl(s->uc_rtp_ipaddr), ntohs(s->uc_rtcp_port));
	fprintf(stderr,"UcSendRepair: %d %d %d %d\n",
		url_id, o_id, offset, lend);
}

typedef struct _RtcpRepaird {
	u_int16_t	url_id;
	u_int16_t	o_id;
	u_int32_t	offset;
	u_int32_t	len;
} RtcpRepaird;

void UcRtpSendData(Source *s , RtcpPacket* rcs)
{
	RtcpRepaird *psrd = (RtcpRepaird *) rcs->d;
	int url_id, o_id, len;
	int offset;
	int p_d_l, s_dchunk, d_off,s_oh;
	DocEntry doc;
	RtpPacket pkt;

	url_id = ntohs (psrd->url_id);
	o_id = ntohs (psrd->o_id);
	offset = ntohl(psrd->offset);
	len = ntohl(psrd->len);

	if (mc_local_url_id < url_id ) { /* request an impossible url_id */
		fprintf(stderr, "***BUG UcRtpSendData url_id***\007\n");
		fprintf(stderr," mc_local_url_id = %d, url_id = %d\n",
			mc_local_url_id,url_id);
		return;
	}
	if (mc_local_docs[url_id].nobj <= o_id) { /* request an impossible o_id */
		fprintf(stderr, "*****BUG UcRtpSendData o_id\007*****\n");
		fprintf(stderr,"mc_local_docs[%d].nobj = %d, o_id = %d\n",
			url_id, mc_local_docs[url_id].nobj, o_id);
		return;
	}
/* size of object include header */
	doc = mc_local_docs[url_id];
	s_oh = doc.o_tab[o_id]->h_len + doc.o_tab[o_id]->d_len;
	if (s_oh <= offset ) { 		/* request an impossible offset */
		fprintf(stderr, "BUG UcRtpSendData offset \007\n");
		return;
	}
	if ( offset + len > s_oh) { /* adjust len */
		fprintf(stderr, "BUG UcRtpSendData len adjust \007\n");
		len = s_oh - offset;
	}
	if (offset == 0) { /* mime is requested */
		pkt.d = doc.o_tab[o_id]->h_part;
		pkt.d_len = doc.o_tab[o_id]->h_len;
		pkt.offset = 0;
		pkt.is_end = doc.o_tab[o_id]->d_part==NULL ? 0x80 : 0;
		pkt.url_id = url_id;
		pkt.o_id = o_id;
		UcRtpSendDataPacket(s, &pkt);	/* send one packet via Unicast */
		return;
	}
	if (offset < doc.o_tab[o_id]->h_len) { /* offset in a middle of mime */
		fprintf(stderr, "BUG UcRtpSendData mid mime \007\n");
		return;
	}
	d_off = offset - doc.o_tab[o_id]->h_len;
	s_dchunk = DATA_CHUNK_SIZE;
	p_d_l = s_dchunk;
	while ( offset < s_oh) {
		pkt.url_id = url_id;
		pkt.o_id = o_id;
		pkt.d = &doc.o_tab[o_id]->d_part[d_off];
		pkt.offset = offset;
		if ( offset + s_dchunk >= s_oh) {
			pkt.d_len = s_oh - offset;
			pkt.is_end = 0x80;
			UcRtpSendDataPacket(s, &pkt);
			break;
		}
		pkt.d_len = s_dchunk;
		pkt.is_end = 0;
		UcRtpSendDataPacket(s, &pkt);

		d_off = d_off + s_dchunk;
		offset = offset + s_dchunk;
		len = len - s_dchunk;
	}
}
