#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/mime.h"

#include "mc_rtp.h"
#include "mc_io.h"
#include "mc_obj.h"
#include "mc_session.h"
#include "mc_gui.h"

unsigned int mc_local_srcid;

static Source* rtcp_demux(u_int32_t srcid, IPAddr addr);

/* I am a receiver
 * I send feedback to sender via Unicast channel
 */
void UcRtcpWriteSdesCb(XtPointer clid, XtIntervalId * time_id)
{
	fprintf(stderr, "UcRtcpWriteSdesCb\n");
}
 
void McNewSrcid(IPAddr addr)
{
        u_int32_t srcid ;
        struct timeval tv;

        gettimeofday(&tv, 0);
        srcid = (u_int32_t) (tv.tv_sec + tv.tv_usec);
        srcid += (u_int32_t)getuid();
        srcid += (u_int32_t)getpid();
        srcid += addr;
        mc_local_srcid = srcid;
}
 
#define SOURCE_HASH 1024
static Source *hashtab_[SOURCE_HASH];
#define SHASH(a) ((int)((((a) >> 20) ^ ((a) >> 10) ^ (a)) & (SOURCE_HASH-1)))

static Source * sources_ = NULL;
static int nsources_ = 0;

Source* consult(u_int32_t srcid)
{       
        int h = SHASH(srcid);
	Source* s;

        for ( s = hashtab_[h]; s != 0; s = s->hlink) {
                u_int32_t id = s->srcid;
                if (id == srcid)
                        return (s);
        }
        return (0);
}
        
/*##########################*/
/* Source from VAT. */
Source* enter(Source* s)
{         
	int h;

        s->next = sources_;
        sources_ = s;                 
          
        h = SHASH(s->srcid);    
        s->hlink = hashtab_[h];      
        hashtab_[h] = s;              

        ++nsources_;                  
        return (s);                   
}

Source * newSource(u_int32_t srcid, IPAddr addr)
{
	Source * s = (Source*)calloc(1, sizeof(Source));
	char * a_ad;
	struct in_addr ina;

	ina.S_un.S_addr = addr;

	a_ad = inet_ntoa(ina);
	s->mute = True;
	s->win = NULL;

	s->srcid = srcid;
	s->addr = addr;
	s->s_srcid = (char*) malloc(strlen(a_ad) + 30);
	sprintf(s->s_srcid,"%u/%s", s->srcid, a_ad);
	s->is_sender = False;

	s->hlink = NULL;
	s->next = NULL;
	s->first_seq = -1;
	s->cur_seq = -1;

	s->sdes = NULL;

	s->gui_ent = CreateMemberGuiEntry(s);

	s->huid = -1;
	s->cwuid = -1;
	s->cduid = -1;

	s->doc = NULL;
	s->chkdoc = NULL;
	return s;
}

/* Demux data packet to its source table entry.  (We don't want an extra
 * SSRC arg here because CSRC's via a mixer don't have their own data
 * packets.)  If we haven't seen this source yet, allocate it but
 * wait until we see two in-order packets accepting the flow.
 */                                    
                                     
static Source* rtcp_demux(u_int32_t srcid, IPAddr addr)
{                                      
        Source* s;

	if (srcid == mc_local_srcid) { /* collision with my srcid... */
		/* if i am a receiver simply choose a new srcid */
		/* if i am a sender , send a bye and choose new srcid */
		fprintf(stderr, "COLLISION with ME\n");
		return NULL;
	}
	s = consult(srcid);    /* consult by srcid */

        if (s == 0) {	/* maybe a new one */
/* we take the very first packet from this source */
		s = newSource(srcid, addr);
 		enter(s);
                s->first_seq=0;
                s->cur_seq=0;            
                return (s);            
        }
	if (s->addr != addr) { /* collision */
/* collision or loop detection (third party) */
/* check for a srcid conflict or loop:
*  - believe the new guy if the old guy said he's done.
*  - otherwise, don't believe the new guy if we've heard
*    from the old guy 'recently'. */                    
		fprintf(stderr, "third party collision\n");
		return NULL;
/*
		u_int32_t t = s->lts_done().tv_sec;
		if (t == 0) {  
			t = s->lts_data().tv_sec;
			u_int32_t now = unixtime().tv_sec;
			if (t && int(now - t) <= 2)
				return (0);
			t = s->lts_ctrl().tv_sec;
			if (t && int(now - t) <= 30)
				return (0);
		}              
		s->addr(addr); 
		s->clear_counters();
		s->lost(0);    
*/
	}
/* nocollision or loop */
	return (s);
}

/* Demux data packet to its source table entry.  (We don't want an extra
 * SSRC arg here because CSRC's via a mixer don't have their own data
 * packets.)  If we haven't seen this source yet, allocate it but
 * wait until we see two in-order packets accepting the flow.
 */                                    
                                     
Source* demux(u_int32_t srcid, IPAddr addr, u_int16_t seq)
{                                      
        Source* s;

	if (srcid == mc_local_srcid) { /* collision with my srcid... */
		/* if i am a receiver simply choose a new srcid */
		/* if i am a sender , send a bye and choose new srcid */
		fprintf(stderr, "COLLISION with ME\n");
		return NULL;
	}
	s = consult(srcid);    /* consult by srcid */

        if (s == 0) {	/* maybe a new one */
/* we take the very first packet from this source */
		s = newSource(srcid, addr);
 		enter(s);
                s->first_seq=seq;
                s->cur_seq=seq;            
                return (s);            
        }
	if (s->addr != addr) { /* collision */
/* collision or loop detection (third party) */
/* check for a srcid conflict or loop:
*  - believe the new guy if the old guy said he's done.
*  - otherwise, don't believe the new guy if we've heard
*    from the old guy 'recently'. */                    
		fprintf(stderr, "third party collision\n");
		return NULL;
/*
		u_int32_t t = s->lts_done().tv_sec;
		if (t == 0) {  
			t = s->lts_data().tv_sec;
			u_int32_t now = unixtime().tv_sec;
			if (t && int(now - t) <= 2)
				return (0);
			t = s->lts_ctrl().tv_sec;
			if (t && int(now - t) <= 30)
				return (0);
		}              
		s->addr(addr); 
		s->clear_counters();
		s->lost(0);    
*/
	}
/* nocollision or loop */
	s->cur_seq = seq; 
	return (s);
}

 /*
 * Try to find an entry in the source table with the same network
 * address (i.e.,  a "duplicate entry") but possibly different srcid.
 * As a side effect, refile the source under the new srcid.
 *                                     
 * The idea here is to gracefully handle sites that restart (with
 * a new srcid).  If we assume that it takes a couple seconds to
 * restart                             
 *                                     
 */                                    
/*
Source* SourceManager::lookup_duplicate(u_int32_t srcid, u_int32_t addr)
{                                      
        /*XXX - should eventually be conditioned on cname not ipaddr */
        /*                             
         * could use hashing here, but this is rarely called.
         */                            
/*        register Source* s;            
        for (s = sources_; s != 0; s = s->next_) {
                /*                     
                 * if addresses match, take old entry if:
                 *  - it sent a 'done', or  
                 *  - it hasn't sent any data for 2 seconds and
                 *    and any control for 30 seconds.
                 */                    
/*                if (s->addr() == addr) {
                        if (s->lts_done().tv_sec != 0)
                                break; 
                        u_int32_t now = unixtime().tv_sec;
                        u_int32_t t = s->lts_data().tv_sec;
                        if (t == 0 || int(now - t) > 2) {
                                t = s->lts_ctrl().tv_sec;
                                if (t == 0 || int(now - t) > 30)
                                        break;
                        }              
                }                      
        }                              
        if (s) {                       
                remove_from_hashtable(s);
                s->srcid(srcid);       
                s->ssrc(srcid);        
                int h = SHASH(srcid);  
                s->hlink = hashtab_[h];
                hashtab_[h] = s;       
                s->clear_counters();   
                s->lost(0);            
        }                              
        return (s);                    
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

	fprintf(stderr, "Update data source \n");
	if (s->mute)
		return;
	if (s->cur_seq != ((seqn -1 ) & 0xffff) ) { /* rutpure de sequence */
						/* packet lost */
		fprintf(stderr, "retrieve\n");
		try_retrieve = 1;
	}
	s->cur_seq = seqn;

	if ( s->huid < url_id ) { /* a new texte */
/* url_id vs huid */
/* (re)allocation/initialisation de s->doc et de s->chkdoc */
/* alloc space for a sender */
		s->doc = (DocEntry **)calloc(0xffff, sizeof(DocEntry *));
		s->chkdoc = (ChunkedDocEntry **)calloc(0xffff,sizeof(ChunkedDocEntry *));
		s->huid = 0xffff;
	}
/* allocate space for this doc */
	if (!s->doc[url_id]) {
		s->doc[url_id] = (DocEntry *) calloc(1, sizeof(DocEntry ));
		s->doc[url_id]->url_id = url_id;
		s->doc[url_id]->nobj = -1;
		s->doc[url_id]->o_tab = NULL;
		s->doc[url_id]->n_miss_o = 0xffffffff;
	}
	doce = s->doc[url_id];
	if (doce->nobj > 0) { /* we still have parse the html part */
		if ( o_id >= doce->nobj ) { /* impossible */
			fprintf(stderr, "McUpdateDataSource: url_id >= nobj\n");
			return;
		}
		if (doce->o_tab[o_id]) { /* we still know that */
			return;
		}
	}
/* we work in chkdoc... good ! */
/* (re)allocation/initialisation de s->chkdoc */
/* embedded is incomplete. Do something */
	cdocent = GetChkDocAndCotab(s, url_id, o_id);
	if(!cdocent)
		return;
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
		/* arme a timer to retrieve missing packet */
		return;
	}

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

/* tous les objets sont la ... afficher */
	McDoWindowText(s, url_id);	/*Display full doc */
}
 
void ProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from)
{
	RtcpPacket rcs;
	Source *s;
	int status;

/* we don't apply mixer's rule: FIXME### */ 
/* this does not solve the contributor source */

	status = DewrapRtcpData(buf, len, &rcs);
	if (status <= 0)
		return;
        s = rtcp_demux(rcs.ssrc, addr_from); 
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
			break;
		case RTCP_PT_BYE :
			fprintf(stderr, "RTCP_BYE\n");
			break;
		case RTCP_PT_APP:
			fprintf(stderr, "RTCP_APP\n");
			break;
		case RTCP_PT_RMP:		/* reliable ... */
/*
			switch(...) {
			case RTCP_RMP_NACK :
			case RTCP_RMP_... :
			}
*/
			break;
	}
}
