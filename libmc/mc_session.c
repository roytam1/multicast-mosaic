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

unsigned int mc_local_srcid;

/* I am a receiver
 * I send feedback to sender via Unicast channel
 */
void UcRtcpWriteSdesCb(XtPointer clid, XtIntervalId * time_id)
{
	fprintf(stderr, "UcRtcpWriteSdesCb\n");
}
 
/*##########################*/

void McUpdateDataSource(Source *s, int is_end, u_int16_t seqn,
	u_int32_t rtp_ts, u_int32_t ssrc,
	u_int16_t url_id, u_int16_t o_id, u_int32_t offset, char *d,
	u_int32_t d_len)
{
	ChunkedDocEntry * cdocent;
	int try_retrieve = 0;
	int status;
	DocEntry * doce;

	if (s->mute)
		return;
	if (s->cur_seq != ((seqn -1 ) & 0xffff) ) { /* rutpure de sequence */
						/* packet lost */
		fprintf(stderr, "DO something for retrieve\n");
		try_retrieve = 1;
	} else {
		s->last_valid_seq = seqn;
	}
	s->cur_seq = seqn;

	if( !SourceAllocDocs(s, url_id)) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}
	doce = s->doc[url_id];
	if (doce->nobj > 0) { /* we still have parse the html part */
		if ( o_id >= doce->nobj ) { /* impossible */
			fprintf(stderr, "McUpdateDataSource: o_id >= nobj\n");
			return;
		}
		if (doce->o_tab[o_id]) { /* we still know that */
			return;
		}
	}
/* we work in chkdoc... good ! */
/* (re)allocation/initialisation de s->chkdoc */
/* embedded is incomplete. Do something */
	status = SourceAllocObjs(s, url_id, o_id);
	if ( status == -1) {	/*invalide object number */
		fprintf(stderr, "Invalid object number on recept\n");
		return;
	}
	if ( ! status ) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}
	cdocent = s->chkdoc[url_id];
	status = PutPacketInChkObj(s,cdocent->co_tab, url_id, o_id,
			offset, d, d_len, is_end); 
/* status:
	- complete : this packet full fill the embedded object
	- incomplete: there is missing packet
	- still_here: we have still see this data
*/
	if(status == STILL_HERE)
		return;
	if( status == INCOMPLETE) {
		if (try_retrieve ) { /* arme a timer to retrieve missing packet */
			fprintf(stderr, "Obj is INCOMPLETE and try_retrieve = 1\n");
			/* Arm a timer (a callback to retrieve packet */
			/* from coe->lmr */
			/*SendRepairFromSeqn(s, url_id, o_id, offset, d_len,
				is_end, seqn);
			*/
/* if (!s->chkdoc[url_id]->co_tab[o_id]->lpdc[0]) {  /* reclamer le header mime rapidement */
/* } */

			s->last_valid_seq = seqn;
			return;
		}
		return;
	}

/* COMPLETE */
/* at this point object is complete. put it in s->doc[url_id]->o_tab[o_id] */
/* copy de s->chkdoc[url_id]->co_tab[o_id] vers s->doc[url_id]->o_tab[o_id]);*/
/* on libere s->chkdoc[url_id] */
	ChkObjToDocObj(s, url_id, o_id);

	if(s->doc[url_id]->nobj == 0)
		return;
	if (o_id == 0) {
/* si c'est l'objet 0: c'est le html. Afficher avec ce qu'on a */
		McDoWindowText(s, url_id);	/*Display full doc */
		return;
	}
/* on a tout l'objet et ce n'est pas l'objet 0: c'est un embedded objet */
	if (s->doc[url_id]->n_miss_o) /* y a des objet qui manquent */
		return;

	ChkDocToDoc(s, url_id);
/* tous les objets sont la ... afficher */
	McDoWindowText(s, url_id);	/*Display full doc */
}
 
void McProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from)
{
	RtcpPacket rcs;
	Source *s;
	int status=1;
	int ind = 0;
	int lenp = 0;

/* we don't apply mixer's rule: FIXME### */ 
/* this does not solve the contributor source */

	while (len > 0) {
		lenp = DewrapRtcpData(&buf[ind], len, &rcs);
		if (lenp <= 0)
			return;
		ind += lenp;
		len -= lenp;
        	s = mc_rtcp_demux(rcs.ssrc, addr_from, &rcs); 
        	if ( s == NULL)
                	return; 
		switch (rcs.pt){
		case RTCP_PT_SR :
			fprintf(stderr, "RTCP_SR\n");
			break;
		case RTCP_PT_RR :
			fprintf(stderr, "RTCP_RR\n");
			break;
		case RTCP_PT_SDES :
			fprintf(stderr, "RTCP_SDES\n");
			ProcessRtcpSdes(s, &rcs);
			break;
		case RTCP_PT_BYE :
			fprintf(stderr, "RTCP_BYE\n");
			break;
		case RTCP_PT_APP:
			fprintf(stderr, "RTCP_APP\n");
			break;
		case RTCP_PT_STATR:	/* State report from sender */
			fprintf(stderr, "RTCP_STATR\n");
			McRepairFromStatr(s, &rcs);
			break;
		default:
			fprintf(stderr,"ProcessRtcpData: unknow pkt.type\n");
			break;
		}
	}
}
 
void UcProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from,
	unsigned short port_from)
{
	RtcpPacket rcs;
	Source *s;
	int status=1;
	int ind = 0;
	int lenp = 0;

/* we don't apply mixer's rule: FIXME### */ 
/* this does not solve the contributor source */

	while (len > 0) {
		lenp = DewrapRtcpData(&buf[ind], len, &rcs);
		if (lenp <= 0)
			return;
		ind += lenp;
		len -= lenp;
        	s = uc_rtcp_demux(rcs.ssrc, addr_from, port_from, &rcs); 
        	if ( s == NULL)
                	return; 
		if (s->uc_rtp_port == 0){
			s->uc_rtcp_port = port_from; /* net byte oder */
			s->uc_rtp_port = htons(ntohs(port_from-1));
			s->uc_rtp_ipaddr = addr_from;
		}
		if (s->uc_rtcp_port != port_from){
			fprintf(stderr, "BUG UcProcessRtcpData\007");
			s->uc_rtp_port =0;
			return;
		}
		switch (rcs.pt){
		case RTCP_PT_RR :
			fprintf(stderr, "RTCP_RR\n");
			break;
		case RTCP_PT_SDES :
			fprintf(stderr, "RTCP_SDES\n");
			break;
		case RTCP_PT_BYE :
			fprintf(stderr, "RTCP_BYE\n");
			break;
		case RTCP_PT_REPAIR:
			fprintf(stderr, "RTCP_PT_REPAIR\n");
			UcRtpSendData(s, &rcs);
			break;
		default:
			fprintf(stderr, "BUG UcProcessRtcpData\007\n");
			return;
		}
	}
}
