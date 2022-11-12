#include "mc_mosaic.h"

int mc_third_party_loop;
int mc_own_traffic_looped;
int mc_collision_with_me;


u_int32_t McNewSrcid(IPAddr addr)
{
        u_int32_t srcid ;
        struct timeval tv;

        gettimeofday(&tv, 0);
        srcid = (u_int32_t) (tv.tv_sec + tv.tv_usec);
        srcid += (u_int32_t)getuid();
        srcid += (u_int32_t)getpid();
        srcid += addr;
        mc_local_srcid = srcid;
        return srcid;
}

#define SOURCE_HASH 1024
static Source *hashtab_[SOURCE_HASH];
#define SHASH(a) ((int)((((a) >> 20) ^ ((a) >> 10) ^ (a)) & (SOURCE_HASH-1)))
 
static Source * sources_ = NULL;
static int nsources_ = 0;
   
static CnflcAddr *conflic_add_list = NULL;
CnflcAddr * add_conflict_addr(IPAddr addr)
{
	CnflcAddr * ca = (CnflcAddr *) malloc(sizeof(CnflcAddr ));

	ca->addr = addr;
	ca->next = conflic_add_list;
	conflic_add_list = ca;
	return ca;
}

CnflcAddr *find_conflict_addr( IPAddr addr)
{
	CnflcAddr *p = conflic_add_list;
	while(p){
		if (p->addr == addr)
			return p;
	}
	return NULL;
}
	
void remove_from_hashtable(Source* s)
{                                     
        /* delete the source from hash table */
        u_int32_t srcid = s->srcid; 
        int h = SHASH(srcid);         
        Source** p = &hashtab_[h];    
        while (*p != s)               
                p = &(*p)->hlink;    
        *p = (*p)->hlink;            
}
 
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

        ina.s_addr = addr;        

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
                                       
        s->sdes_cname[0] = '\0';                
	s->sdes_tool[0] = '\0';
	s->sdes_email[0] = '\0';
	s->sdes_name[0] = '\0';

        s->gui_ent = CreateMemberGuiEntry(s);
                                       
        s->cwuid = -1;
        s->cduid = -1;                 
        s->last_valid_object_id = -1;     
        s->last_valid_state_id = -1;     
	s->current_state_id_in_window = -1;
        s->states = NULL;
        s->objects = NULL;              
	s->states_tab_size = 0;
	s->objects_tab_size = 0;

	s->old_cur_pos_x = -100;
	s->old_cur_pos_y= -100;

	McSourceCacheInit(s,mMosaicRootDirName); /* create a cache for it */
        return s;
}

/* Try to find an entry in the source table with the same cname 
 * (i.e.,  a "duplicate entry") but possibly different srcid.
 * As a side effect, refile the source under the new srcid. 
 *                       
 * The idea here is to gracefully handle sites that restart (with
 * a new srcid).  If we assume that it takes a couple seconds to
 * restart      
 */                     
Source* lookup_duplicate(u_int32_t srcid, u_int32_t addr, char* cname)
{               
        /* could use hashing here, but this is rarely called. */      
        register Source* s;
        for (s = sources_; s != 0; s = s->next) {
                /*
                 * if cname match, take old entry if:
                 *  - it sent a 'done', or
                 *  - it hasn't sent any data for 2 seconds and
                 *    and any control for 30 seconds.
We always get old entry because a match on a cname=user@ip/port/rtptime.
                 */
                if (strcmp(s->sdes_cname , cname) == 0 ) {
			break;
                }                     
        }                             
        if (s) {
                int h;
                remove_from_hashtable(s);
                s->srcid = srcid;
		s->addr = addr;
                h = SHASH(srcid);
                s->hlink = hashtab_[h];
                hashtab_[h] = s; 
        }
        return (s);
}                                     

Source* mc_rtcp_demux(u_int32_t srcid, IPAddr addr_from, RtcpPacket* rcs)
{
	Source* s;
	SdesStruct * sdes;
	char * cname;
	CnflcAddr *cnflc_addr;
	int newsrcid;

	if ( rcs->pt != RTCP_PT_SDES) { /* do same things as mc_rtp_demux */
		s = mc_rtp_demux(srcid, addr_from);
		return s;
	}

/* RTCP_PT_SDES analyse */
	sdes = parse_sdes(rcs);
	if (!sdes || *sdes->cname == '\0')
		return NULL;
	cname = sdes->cname;
/* a cname is manage by software, not by user, so it is unique because
 * allocation of udp port is not REUSEABLE */
	if ( srcid == mc_local_srcid && addr_from == mc_local_ip_addr &&
	     strcmp(cname, mc_local_cname) == 0 ) {
		/* it is REALLY me via loopback. don't process myself */
		/* other srcid==mc_local_srcid is loopback or collision */
		return NULL;
	}
	
	if (srcid == mc_local_srcid ) { /* loop or collision */
		if (addr_from == mc_local_ip_addr){
			fprintf(stderr,"Sorry I must restart because of ssrc/addr collision (local)\n");
			return NULL;
			/* abort(); */
		}
		cnflc_addr = find_conflict_addr(addr_from);
		if (cnflc_addr ) { 
/* the soure transport addr is found in conflicting (data or control) source
   transport address */
			/* a collision or loop of the participant's own packets */
			if ( strcmp(cname,mc_local_cname) == 0){ /* loop */
				mc_own_traffic_looped++;
				fprintf(stderr, "Traffic loop(rctp-cname): ip = %08x\n", addr_from);
				cnflc_addr->lts = unixtime().tv_sec;
				return NULL;
			}
			/* case: the source id is not from RTCP SDES CNAME */
			mc_own_traffic_looped++;
			fprintf(stderr, "Traffic loop(rctp): ip = %08x\n", addr_from);
			/* mark current time in conflicting adress list entry */
			cnflc_addr->lts = unixtime().tv_sec;
			/* abort processing */
			return NULL;
		}
/* not in list of conflict or CNAME is not participant's own */
		fprintf(stderr, "Traffic collision(rctp): ip = %08x\n", addr_from);
		mc_collision_with_me++;
		cnflc_addr = add_conflict_addr(addr_from);
		cnflc_addr->lts = unixtime().tv_sec;
		if (mc_fd_rtp_w != -1) { /* i am a sender == True */
			char * reason = "Collision detected";
			SendBye(mc_local_srcid,reason);
			/* don't realloc port . stay with the same cname */
			/* if a user quit then then a bye and realloc a (new) port*/
/*### a cname = user@ip/port/ntptime (32bits)*/
		}
		newsrcid = McNewSrcid(mc_local_ip_addr);
		while(consult(newsrcid) !=0)
			newsrcid = McNewSrcid(mc_local_ip_addr);
		mc_local_srcid = newsrcid;
		SendSdesCname(); /* send a new srcid/addr/cname as fast as it posiible. a receiver need to resync on new source */
		s = newSource(srcid, addr_from);
		strcpy(s->sdes_cname, cname);
		enter(s);
		s->lts = unixtime().tv_sec;
		s->first_seq=0;
		s->cur_seq=0;
		return s;
	}

	s = consult(srcid);

	if ( s == NULL)	{	/* create a new entry */
/* 
 * Heuristic: handle application re-starts
 * gracefully.  We have a new source that's
 * not via a mixer.  Try to find an existing
 * entry from the same host. 
 */
/*we can do it because:
### a cname = user@ip/port/ntptime (32bits)
*/
/* recover gracefully (via cname) a collised ssrc */
		s = lookup_duplicate(srcid,addr_from,cname);
		if (s != NULL)
			return s;
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
		enter(s);
		s->lts = unixtime().tv_sec;
		s->first_seq=0;
		s->cur_seq=0;
		return s;	/* we process without a cname */
	}

	if (s->addr == addr_from) {
		s->lts = unixtime().tv_sec;
		s->cur_seq = 0;
		return s;
	}
/* collision or loop detection (third party) */
	fprintf(stderr, "third party loop(rctp)\n");
	mc_third_party_loop++;
	return NULL;
}
 


/* comment from vat:
 * Demux data packet to its source table entry.  (We don't want an extra
 * SSRC arg here because CSRC's via a mixer don't have their own data
 * packets.)
 */

Source * mc_rtp_demux(u_int32_t srcid, IPAddr addr_from)
{
	CnflcAddr *cnflc_addr;
	Source* s;
	int newsrcid;

	if ( srcid == mc_local_srcid && addr_from == mc_local_ip_addr) {
		/* it is me via loopback. don't process myself */
		return NULL;
	}
	if (srcid == mc_local_srcid ) {
		cnflc_addr = find_conflict_addr(addr_from);
		if (cnflc_addr ) { 
/* the soure transport addr is found in conflicting (data or control) source
   transport address */
			/* a collision or loop of the participant's own packets */
			/* case: the source id is not from RTCP SDES CNAME */
			mc_own_traffic_looped++;
			fprintf(stderr, "Traffic loop(rtp): ip = %08x\n", addr_from);
			/* mark current time in conflicting adress list entry */
			cnflc_addr->lts = unixtime().tv_sec;
			/* abort processing */
			return NULL;
		}
/* not in list of conflict or CNAME is not participant's own */
		fprintf(stderr, "Traffic collision(rtp): ip = %08x\n", addr_from);
		mc_collision_with_me++;
		cnflc_addr = add_conflict_addr(addr_from);
		cnflc_addr->lts = unixtime().tv_sec;
/* collision is recover via a cname */
/* don't send bye. choose a new src. let the receiver recover my source from
 * a cname */
/* SendBye(mc_local_srcid); I am a receiver just change scrid*/
/*		if (am_i_a_sender == True){
### a cname = user@ip/port/ntptime (32bits)
			return NULL;
		}
*/
		newsrcid = McNewSrcid(mc_local_ip_addr);
		while(consult(newsrcid) !=0)
			newsrcid = McNewSrcid(mc_local_ip_addr);
		mc_local_srcid = newsrcid;
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
		enter(s);
		s->lts = unixtime().tv_sec;
/*
		s->first_seq=seq;
		s->cur_seq=seq;
*/
		return s;
	}

	s = consult(srcid);

	if ( s == NULL)	{	/* create a new entry */
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
		enter(s);
		s->lts = unixtime().tv_sec;
		s->first_seq=0;
		s->cur_seq=0;
		return s;	/* we process without a cname */
	}

	if (s->addr == addr_from) {
		s->lts = unixtime().tv_sec;
/*
		s->cur_seq = seq;
*/
		return s;
	}
/* collision or loop detection (third party) */
	fprintf(stderr, "third party loop(rtp)\n");
	mc_third_party_loop++;
	return NULL;
}

Source* uc_rtcp_demux(u_int32_t srcid, IPAddr addr_from, unsigned short port_from, RtcpPacket* rcs)
{
	CnflcAddr *cnflc_addr;
	Source* s;
	int newsrcid;

	if ( srcid == mc_local_srcid && addr_from == mc_local_ip_addr) {
		/* it is me via loopback. don't process myself */
		return NULL;
	}
	if (srcid == mc_local_srcid ) {
		cnflc_addr = find_conflict_addr(addr_from);
		if (cnflc_addr ) { 
/* the soure transport addr is found in conflicting (data or control) source
   transport address */
			/* a collision or loop of the participant's own packets */
			/* case: the source id is not from RTCP SDES CNAME */
/*			uc_own_traffic_looped++; */
			fprintf(stderr, "Traffic loop(rtcp): ip = %08x\n", addr_from);
			/* mark current time in conflicting adress list entry */
			cnflc_addr->lts = unixtime().tv_sec;
			/* abort processing */
			return NULL;
		}
/* not in list of conflict or CNAME is not participant's own */
		fprintf(stderr, "Traffic collision(rtp): ip = %08x\n", addr_from);
/*		uc_collision_with_me++; */
		cnflc_addr = add_conflict_addr(addr_from);
		cnflc_addr->lts = unixtime().tv_sec;
/* collision is recover via a cname */
/* don't send bye. choose a new src. let the receiver recover my source from
 * a cname */
/* SendBye(mc_local_srcid); I am a receiver just change scrid*/
/*		if (am_i_a_sender == True){
### a cname = user@ip/port/ntptime (32bits)
			return NULL;
		}
*/
		newsrcid = McNewSrcid(mc_local_ip_addr);
		while(consult(newsrcid) !=0)
			newsrcid = McNewSrcid(mc_local_ip_addr);
		mc_local_srcid = newsrcid;
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
		enter(s);
		s->lts = unixtime().tv_sec;
/*
		s->first_seq=seq;
		s->cur_seq=seq;
*/
		return s;
	}
	s = consult(srcid);
	if ( s == NULL)	{	/* create a new entry */
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
                enter(s);
		s->uc_rtcp_port = port_from;
		s->uc_rtp_port = htons(ntohs(port_from -1));
		s->uc_rtp_ipaddr = addr_from;
                s->lts = unixtime().tv_sec;
                s->first_seq=0;
                s->cur_seq=0;
                return s;       /* we process without a cname */
	}

	if (s->uc_rtp_ipaddr == addr_from && s->uc_rtcp_port == port_from) {
		s->lts = unixtime().tv_sec;
		return s;
	}
/* collision or loop detection (third party) */
	fprintf(stderr, "third party loop(uc_rtp)\n");
/*	uc_third_party_loop++; */
	return NULL;
}

/* comment from vat:
 * Demux data packet to its source table entry.  (We don't want an extra
 * SSRC arg here because CSRC's via a mixer don't have their own data
 * packets.)
 */

Source * uc_rtp_demux(u_int32_t srcid, IPAddr addr_from, unsigned short port_from)
{
	CnflcAddr *cnflc_addr;
	Source* s;
	int newsrcid;

	if ( srcid == mc_local_srcid && addr_from == mc_local_ip_addr) {
		/* it is me via loopback. don't process myself */
		return NULL;
	}
	if (srcid == mc_local_srcid ) {
		cnflc_addr = find_conflict_addr(addr_from);
		if (cnflc_addr ) { 
/* the soure transport addr is found in conflicting (data or control) source
   transport address */
			/* a collision or loop of the participant's own packets */
			/* case: the source id is not from RTCP SDES CNAME */
/*			uc_own_traffic_looped++;*/
			fprintf(stderr, "Traffic loop(rtp): ip = %08x\n", addr_from);
			/* mark current time in conflicting adress list entry */
			cnflc_addr->lts = unixtime().tv_sec;
			/* abort processing */
			return NULL;
		}
/* not in list of conflict or CNAME is not participant's own */
		fprintf(stderr, "Traffic collision(rtp): ip = %08x\n", addr_from);
		mc_collision_with_me++;
		cnflc_addr = add_conflict_addr(addr_from);
		cnflc_addr->lts = unixtime().tv_sec;
/* collision is recover via a cname */
/* don't send bye. choose a new src. let the receiver recover my source from
 * a cname */
/* SendBye(mc_local_srcid); I am a receiver just change scrid*/
/*		if (am_i_a_sender == True){
### a cname = user@ip/port/ntptime (32bits)
			return NULL;
		}
*/
		newsrcid = McNewSrcid(mc_local_ip_addr);
		while(consult(newsrcid) !=0)
			newsrcid = McNewSrcid(mc_local_ip_addr);
		mc_local_srcid = newsrcid;
		s = newSource(srcid, addr_from);
		s->sdes_cname[0] = '\0';
		enter(s);
		s->lts = unixtime().tv_sec;
/*
		s->first_seq=seq;
		s->cur_seq=seq;
*/
		return s;
	}
	s = consult(srcid);
	if ( s == NULL)	{	/* create a new entry */
		return NULL;	/* never heard from mc_rtp!!! */
	}

	if (s->uc_rtp_ipaddr == addr_from && s->uc_rtp_port == port_from) {
		s->lts = unixtime().tv_sec;
		return s;
	}
/* collision or loop detection (third party) */
	fprintf(stderr, "third party loop(uc_rtp)\n");
/*	uc_third_party_loop++; */
	return NULL;
}

void ProcessRtcpSdes(Source * s, RtcpPacket* rcs)
{
	SdesStruct * sdes;

	sdes = parse_sdes(rcs);
	if (!sdes)
		return;
/* setup NAME EMAIL ... */
	if (*sdes->cname ) {
		if (strcmp(s->sdes_cname, sdes->cname) != 0) {
			strcpy(s->sdes_cname, sdes->cname);
			s->uc_rtp_port = sdes->uc_rtp_port;
			s->uc_rtcp_port = sdes->uc_rtcp_port;
			s->uc_rtp_ipaddr = sdes->uc_rtp_ipaddr;
		}
#ifdef DEBUG_MULTICAST
fprintf(stderr,"ProcessRtcpSdes: cname = %s, rtp_port = %d, rtcp_port = %d, rtp_ipaddr = %d\n", s->sdes_cname, s->uc_rtp_port, s->uc_rtcp_port, s->uc_rtp_ipaddr);
#endif
	}
	if (*sdes->tool)
		strcpy(s->sdes_tool, sdes->tool);
	if (*sdes->email)
		strcpy(s->sdes_email, sdes->email);
	if (*sdes->name){
		if (strcmp(s->sdes_name, sdes->name) !=0){
			strcpy(s->sdes_name, sdes->name);
			UpdGuiMemberName(s);
		}
	}
	return;
}
