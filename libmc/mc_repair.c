#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
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
#include "mc_misc.h"

void UcSendRepair(Source *s, int type, int id, int offset, int lend);

/* 
	state_id    id of state change when clicking on a frame
		    a new state describe how is the new view of frame
	object_id
	rtp_otime RTP sample time of objectid
	reserved
*/
typedef struct _RtcpStard {
	u_int32_t	sid;	/* Hightest state_id report */
	u_int32_t	oid;	/* Hightest object id report */
	u_int32_t	c_sid;	/* current state id */
	u_int32_t	rtp_otime; /* RTP sample time of oid */
	u_int32_t	reserved;  /* extension ... */
} RtcpStard;

static void McScheduleQueryRepairStates(Source *s, int from, int to)
{
	/* from = send repair from this state */
	/* to =   until this state */
	ChunkedBufStruct *cbs;
	MissRange * plmr;
	int offset;
	int len;
	int i;

	for ( i=from; i<= to; i++) { /* for each incomplete state */
		if (s->states[i].buffer_status == PARSED_BUFFER) {
			if (i - s->last_valid_state_id == 1)
				s->last_valid_state_id = i;
			continue;
		}
		switch (s->states[i].buffer_status) {
		case CHUNKED_BUFFER:
			cbs = s->states[i].chkbuf;
			plmr = cbs->lmr;
			while(plmr){
				offset=plmr->from;
				len = plmr->to - offset +1;
				if (plmr->to == 0xffffffff)
					len = 0xffffffff;
				
				UcSendRepair(s, HTML_STATE_DATA_TYPE, i,
					offset , len);
				plmr = plmr->next;
			}
			break;
		case EMPTY_BUFFER:
			UcSendRepair(s, HTML_STATE_DATA_TYPE, i,
				0, 0xffffffff);	/* request all */
			break;
		default:
			abort();	/*le me know */
		}
	}
}

static void McScheduleQueryRepairObjects(Source *s, int from, int to)
{
	/* send query repair to Source s */
	/* from = send repair from this object */
	/* to =   until this object */
	ChunkedBufStruct *cbs;
	MissRange * plmr;
	int offset;
	int len;
	int i;

	for ( i=from; i<= to; i++) { /* for each incomplete state */
		if (s->objects[i].buffer_status == PARSED_BUFFER ||
		    s->objects[i].buffer_status == COMPLETE_BUFFER ||
		    s->objects[i].buffer_status == PARSED_ALL_DEPEND_BUFFER) {
			if (i - s->last_valid_object_id == 1)
				s->last_valid_object_id = i;
			continue;
		}
		switch (s->objects[i].buffer_status) {
		case CHUNKED_BUFFER:
			cbs = s->objects[i].chkbuf;
			plmr = cbs->lmr;
			while(plmr){
				offset=plmr->from;
				len = plmr->to - offset +1;
				if (plmr->to == 0xffffffff)
					len = 0xffffffff;
				
				UcSendRepair(s, HTML_OBJECT_DATA_TYPE, i,
					offset , len);
				plmr = plmr->next;
			}
			break;
		case EMPTY_BUFFER:
			UcSendRepair(s, HTML_OBJECT_DATA_TYPE, i,
				0, 0xffffffff);	/* request all */
			break;
		default:
			abort();	/*le me know */
		}
	}
}

static void McScheduleQueryRepairObjectsTree( Source *s, int moid)
{
	int status;
	int i;
	int no;

	status = s->objects[moid].buffer_status;
	if (status == PARSED_ALL_DEPEND_BUFFER ){
		return;
	}
	if ( !(status == PARSED_BUFFER || status == COMPLETE_BUFFER) ) {
		McScheduleQueryRepairObjects(s, moid, moid);
		return;
	}

/* le buffer est bon et il n'y a pas d'objet dependant => promu a PARSED_ALL_DEPEND_BUFFER */
	if ( (status == PARSED_BUFFER) && (s->objects[moid].n_do == 0) ) {
		s->objects[moid].buffer_status = PARSED_ALL_DEPEND_BUFFER;
		return;
	}

	for (i = 0; i< s->objects[moid].n_do; i++) {
		no = s->objects[moid].dot[i];
		status = s->objects[no].buffer_status;
		if (status == PARSED_ALL_DEPEND_BUFFER ){
			continue;
		}
/* if ( !(status == PARSED_BUFFER || status == COMPLETE_BUFFER) )  */
/* l'arbre de depandance ne doit pas boucler */
		McScheduleQueryRepairObjectsTree(s, s->objects[moid].dot[i]);
	}	
}

void McQueryRepairFromStatr(Source *s, RtcpPacket* rcs)
{
	RtcpStard *psrd = (RtcpStard *) rcs->d;
	int c_sid;
	int sid;
	int oid;
	int lvo, lvs;
	int rtp_otime;
	int status;

	sid = ntohl (psrd->sid);
	c_sid = ntohl (psrd->c_sid);
	oid = ntohl (psrd->oid);
	rtp_otime = ntohl (psrd->rtp_otime);

	if (c_sid != s->c_sid && sid >= c_sid) {
		s->c_sid = c_sid;
		UpdGuiMemberPage(s);
	}


/* check and alloc enought object. repair missing object */
/* this may be long to repair because object may depend of object... */
        status = McRcvrSrcAllocObject(s, oid);
        if( !status) {                
                fprintf(stderr,"Out of mem\n");
                abort();
        }

/* check and alloc for enought state */
	if( !McRcvrSrcAllocState(s, sid)) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}
	if (c_sid > sid){
		if( !McRcvrSrcAllocState(s, c_sid)) {
			fprintf(stderr,"Out of mem\n");
			abort();
		}
		return;
	}

	if(s->mute)
		return;

	lvo = s->last_valid_object_id;
/*	if (lvo < oid ) { /* schedule une demande de repair pour tous les objets manquant */
/*		/* look for missing object, missing packet in object etc..*/
/*		McScheduleQueryRepairObjects(s, lvo+1, oid);
/*		/* an object will be complete, so check state */
/*		McRcvSrcScheduleCheckState(s, sid);
/*	}
*/

	lvs = s->last_valid_state_id;
	if ( lvs < sid) { /* schedule une demande de repair pour ces etats */
		McScheduleQueryRepairStates(s, lvs+1, sid); 
/*		McScheduleQueryRepairStates(s, sid, sid); */
		/* schedule a check for sid */
		/* when check is good we display the valid state */
		McRcvSrcScheduleCheckState(s, sid);
		return;
	}

		/* FIXME: si tous les obj sont la pour l'etat,
		/* s->states[sid]/start_moid/n_do/dot/ */
		/* alors afficher */
	if (lvo < oid ) {
		int do_return = 0;
		int i;

		status=  McRcvrSrcCheckBufferObject(s, s->states[c_sid].start_moid);
		if (status != PARSED_ALL_DEPEND_BUFFER) {
			McScheduleQueryRepairObjectsTree(s, s->states[c_sid].start_moid);
			McRcvSrcScheduleCheckState(s, c_sid);
			return;
		}
		for (i = 0; i<s->states[c_sid].n_do; i++){
			status=  McRcvrSrcCheckBufferObject(s, s->states[c_sid].dot[i]);
			if (status != PARSED_ALL_DEPEND_BUFFER) {
				McScheduleQueryRepairObjectsTree(s, s->states[c_sid].dot[i]);
				do_return =1;
			}
		}
		if (do_return){
			McRcvSrcScheduleCheckState(s, c_sid);
			return;
		}
	}
	if (lvs < c_sid)	/* because c_sid not transmit yet */
		return;
	if (c_sid == s->current_state_id_in_window )
		return;
	McDoWindowText(s, c_sid);
	return;
}

/* input
	- s	: where to send unicast repair
	- type	: type of repair STATE or OBJECT
			HTML_OBJECT_DATA_TYPE or HTML_STATE_DATA_TYPE
	- id	: state_id or moid
	- offset
	- lend  : number of missing byte;
*/

void UcSendRepair(Source *s, int type, int id, int offset, int lend)
{
	struct sockaddr_in addr_w;
	unsigned char buf[28];
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

	buf[4] = (mc_local_srcid >> 24) & 0xff;    /* SSRC (me) */  
	buf[5] = (mc_local_srcid >> 16) & 0xff;
	buf[6] = (mc_local_srcid >> 8) & 0xff;
	buf[7] =  mc_local_srcid & 0xff;

	buf[8] = (s->srcid >> 24) & 0xff;    /* SSRC target to be asked SSRC */
	buf[9] = (s->srcid >> 16) & 0xff;
	buf[10] = (s->srcid >> 8) & 0xff;
	buf[11] =  s->srcid & 0xff;

	buf[12] = buf[13] = buf[14] = 0;
	buf[15] = type;			/* type of repair */

        buf[16] = ( id >> 24) & 0xff;       /* id */
        buf[17] = ( id >> 16) & 0xff;
        buf[18] = ( id >> 8) & 0xff;
        buf[19] = id & 0xff;

        buf[20] = ( offset >> 24) & 0xff;       /* offset */
        buf[21] = ( offset >> 16) & 0xff;
        buf[22] = ( offset >> 8) & 0xff;
        buf[23] = offset & 0xff;

        buf[24] = (lend >> 24) & 0xff;   /* lend */
        buf[25] = ( lend >> 16) & 0xff;
	buf[26] = ( lend >> 8) & 0xff;
	buf[27] = lend & 0xff;

	len =28;
	cnt = sendto(uc_fd_rtcp_w, (char*)buf, len, 0,
		(struct sockaddr *) &addr_w, sizeof(addr_w));
	if(cnt != len){
		perror("UcSendRepair:sendto:");
	}
	fprintf(stderr,"UcSendRepair to %08x %04x\n",
		ntohl(s->uc_rtp_ipaddr), ntohs(s->uc_rtcp_port));
	fprintf(stderr,"UcSendRepair: %d %d %d %d\n",
		type, id, offset, lend);
}

typedef struct _McQueueStateRepairQuery {
	struct _McQueueStateRepairQuery * next;
	int id;		/* sid part of query */
	int offset;	/* offset part */
	int len;	/* len part */
	IPAddr addr_ip;	/* ip addr of queryier */
	unsigned short port;	/* unicast port of the queryier */
	int source_count;	/* number of querier */
} McQueueStateRepairQuery;

static McQueueStateRepairQuery * mc_queue_state_repair_query = NULL;

/* time to answer to query */
static void McSendStateRepairAnswerCb(XtPointer clid, XtIntervalId * id)
{
	McStateStruct *s;
	McQueueStateRepairQuery * p = mc_queue_state_repair_query;
	McQueueStateRepairQuery *pp= NULL;
	char *sdata;
	int data_size, query_offset;
	int query_len;
        int rearm_timer = mc_rtp_packets_list ? False : True;
	RtpPacket * deb_p, *end_p;
	int s_dchunk;
	char *odata;
	int object_size,n_t, r_b, offset, p_d_l, i;
	double d_dur;
	RtpPacket **ptab;
	int rtp_ts, d_duration;
	int have_send_multi = 0;

	deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
        end_p = mc_rtp_packets_list;
 
        if (deb_p) { /* il y a une file d'attente */
                while (end_p->next){
                        end_p = end_p->next;
                }
        }

	while(p) {
		s = &mc_sender_state_tab[p->id];
		sdata = s->sdata;
		data_size = s->sdata_len;
		query_offset = p->offset;
		query_len = p->len;
		if (query_offset >= data_size) { /*request an impossible offset */
			abort();
		}
		if (query_len < 0 ) {
			query_len = data_size - query_offset;
		}
		if ( query_offset + query_len > data_size) { /* adjust len */
			fprintf(stderr, "BUG len adjust \007\n");
			query_len = data_size - query_offset;
		}
		if (query_len <= 0 ) {
			abort();	/* bug let me know */
		}
        	s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */
	        odata = &sdata[query_offset];       /*  don't free this  */
                                     /* a state is always in memory */
		object_size = query_len;

/* nombre total de packet pour l'objet */
	        n_t = (object_size -1) / s_dchunk; /* division entiere */
	        if (object_size > 0)
	       	         n_t++;
/* alloc packets. Plus one for NULL*/
	        ptab = (RtpPacket **) malloc(sizeof(RtpPacket*) * (n_t+1));
        	for ( i = 0; i < n_t; i++ )    
                	ptab[i] = (RtpPacket *)malloc(sizeof(RtpPacket) );
        	ptab[n_t] = NULL;              
                                       
        	r_b = object_size;     /* remaining byte to send */
        	offset = 0;                    
        	if (r_b <= s_dchunk) { 
                	p_d_l = r_b;   /* packet data len */
        	} else {
                	p_d_l = s_dchunk;
        	}
        	rtp_ts = McRtpTimeStamp(s->ts); /* sample time when file come */
	 	for ( i = 0; i < n_t; i++ ) {
                	d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond */
                	d_duration = (int) d_dur;
                	if (d_duration < 2)    
                        	d_duration = 2;
                	ptab[i]->next = ptab[i+1];  
                	ptab[i]->id = s->statid;
                	ptab[i]->data_type = HTML_STATE_DATA_TYPE;
                	ptab[i]->d_len = p_d_l;
                	ptab[i]->d = &odata[offset];
                	ptab[i]->is_eod = 0;   
                	ptab[i]->to_free = NULL;
                	ptab[i]->offset = offset + query_offset;
                        /* le temps qu'il faut pour envoyer ce packet*/
                        /* dependant de la bande passante qu'on s'autorise */
	                ptab[i]->rtp_ts = rtp_ts; /* sample time when file come*/
                	ptab[i]->duration = d_duration; /* le temps qu'il faut pour envoie
					/* a la vitesse desiree  en millisec*/
	                offset += p_d_l;
	                r_b = r_b - p_d_l;
	                if (r_b <= s_dchunk) {
	                        p_d_l = r_b;            /* packet data len */
	                } else {
	                        p_d_l = s_dchunk;
	                }
	        }
		if ( ptab[n_t -1]->offset + s_dchunk >= data_size) {
	        	ptab[n_t -1]->is_eod =  1;      
	        	ptab[n_t -1]->to_free = NULL; /* Dont'free in McSendRtpDataTimeOutCb */
		}
/*voir si on envoie par UNI ou MULTI et si on rearme le timer multicast... */
/* FIXME: compute something to choose between UNICAST or MULTIUCAST */
		if ( p->source_count == 1) { /* unicast */
			int j;

			for (j = 0; j< n_t ; j++) { /* FIXME: not a full bw */
fprintf(stderr,"McSendStateRepairAnswerCb: sending unicast data -%s-\n", ptab[j]->d);
				UcRtpSendDataPacketTo(p->addr_ip,p->port, ptab[j]);
				free(ptab[j]);
			}
	        	free(ptab);
		} else { /* multicast */
/* add at end ########## */
		        if (deb_p){                    
	                	end_p->next = ptab[0]; 
	        	} else {                       
	                	mc_rtp_packets_list = ptab[0];
	        	}                              
	        	free(ptab);
			have_send_multi = 1;
		}
		pp =p;
		p = p->next;
		free(pp);
		mc_queue_state_repair_query = p;
	}
/* rearm if necessary */
        if (rearm_timer == True && have_send_multi == 1){      
                mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
                        mc_write_rtp_data_next_time,
                        McSendRtpDataTimeOutCb, NULL);
        }

}


typedef struct _RtcpRepaird {
	u_int32_t	target_ssrc;
	u_int32_t	bits;
	u_int32_t	id;
	u_int32_t	offset;
	u_int32_t	len;
} RtcpRepaird;

/* we schedule data packet . we had been asked before */
/* rebuild the data and send packet via unicast or multicast */
/* depending of some algo... */

static XtIntervalId mc_queue_state_repair_query_timer_id;
static int mc_queue_state_repair_query_time = 100;

static void McScheduleStateRepairAnswer( Source *s, u_int32_t id,
	u_int32_t offset, int len)
{
/* for State, the data is in memory */
	McQueueStateRepairQuery *head = mc_queue_state_repair_query;
	McQueueStateRepairQuery *p, *pp;

	if (!head) {
		head = (McQueueStateRepairQuery *)malloc(
				sizeof(McQueueStateRepairQuery));
		head->next = NULL;
		head->addr_ip= s->uc_rtp_ipaddr;
		head->port=s->uc_rtp_port;

		head->source_count = 1;
		head->id = id;
		head->offset = offset;
		head->len = len;
		mc_queue_state_repair_query = head ;
		mc_queue_state_repair_query_timer_id = XtAppAddTimeOut(
			mMosaicAppContext, mc_queue_state_repair_query_time,
			McSendStateRepairAnswerCb, NULL);
		return;
	}

	/* look in queue if query still exist */
	p = head;
	pp = NULL;
	while (p) {
		if ( p->id == id && p->offset == offset && p->len == len) {
			p->source_count++;
			return;
		}
		pp = p;	
		p = p->next;
	}
/* add at end , if query doesn't still exist */
	p = (McQueueStateRepairQuery *)malloc(
                                sizeof(McQueueStateRepairQuery));
	p->next = NULL;
	p->addr_ip= s->uc_rtp_ipaddr;
	p->port=s->uc_rtp_port;

	p->source_count = 1;
	p->id = id;
	p->offset = offset;
	p->len = len;
	pp->next = p;
/*###	maybe recompute time if toomany query ###*/
}


typedef struct _McQueueObjectRepairQuery {
        struct _McQueueObjectRepairQuery * next;
        int id;         /* sid part of query */
        int offset;     /* offset part */
        int len;        /* len part */
        IPAddr addr_ip; /* ip addr of queryier */
        unsigned short port;    /* unicast port of the queryier */
        int source_count;       /* number of querier */
} McQueueObjectRepairQuery;

static McQueueObjectRepairQuery * mc_queue_object_repair_query = NULL;

/* time to answer to query */
static void McSendObjectRepairAnswerCb(XtPointer clid, XtIntervalId * id)
{
        McObjectStruct *ob;
        McQueueObjectRepairQuery * p = mc_queue_object_repair_query;
        McQueueObjectRepairQuery *pp= NULL;
        char *obdata;
        int data_size, query_offset;
        int query_len;
        int rearm_timer = mc_rtp_packets_list ? False : True;
        RtpPacket * deb_p, *end_p;
        int s_dchunk;
        char *odata;
        int object_size,n_t, r_b, offset, p_d_l, i;
        double d_dur;
        RtpPacket **ptab;
        int rtp_ts, d_duration;
        int have_send_multi = 0;
	char *obfname;
	int fdr;

        deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
        end_p = mc_rtp_packets_list;

        if (deb_p) { /* il y a une file d'attente */
                while (end_p->next){
                        end_p = end_p->next;
                }
        }

        while(p) {
                ob = &moid_sender_cache[p->id];;
                query_offset = p->offset;
                query_len = p->len;
		obfname = ob->fname;
		data_size = ob->file_len;
		fdr = open(obfname, O_RDONLY);
                if (query_offset >= data_size) { /*request an impossible offset */
                        abort();
                }
                if (query_len < 0 ) {
                        query_len = data_size - query_offset;
                }
                if ( query_offset + query_len > data_size) { /* adjust len */
                        fprintf(stderr, "BUG len adjust \007\n");
                        query_len = data_size - query_offset;
                }
                if (query_len <= 0 ) {
                        abort();        /* bug let me know */
                }

                obdata = (char*) malloc(query_len+1);
		lseek(fdr, query_offset, SEEK_SET);
		read(fdr, obdata, query_len);
		close(fdr);
                s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */

                odata = obdata; /*free this is an object in a file*/
                object_size = query_len;

/* nombre total de packet pour l'objet */
                n_t = (object_size -1) / s_dchunk; /* division entiere */
                if (object_size > 0)
                         n_t++;
/* alloc packets. Plus one for NULL*/
                ptab = (RtpPacket **) malloc(sizeof(RtpPacket*) * (n_t+1));
                for ( i = 0; i < n_t; i++ )
                        ptab[i] = (RtpPacket *)malloc(sizeof(RtpPacket) );
                ptab[n_t] = NULL;

                r_b = object_size;     /* remaining byte to send */
                offset = 0;      
                if (r_b <= s_dchunk) {
                        p_d_l = r_b;   /* packet data len */
                } else {
                        p_d_l = s_dchunk;
                }
                rtp_ts = McRtpTimeStamp(ob->ts); /* sample time when file come */
                for ( i = 0; i < n_t; i++ ) {
                        d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond
 */
                        d_duration = (int) d_dur;
                        if (d_duration < 2)
                                d_duration = 2;
                        ptab[i]->next = ptab[i+1];
                        ptab[i]->id = ob->moid;
                        ptab[i]->data_type = HTML_OBJECT_DATA_TYPE;
                        ptab[i]->d_len = p_d_l;
                        ptab[i]->d = &odata[offset];
                        ptab[i]->is_eod = 0;
                        ptab[i]->to_free = NULL;
                        ptab[i]->offset = offset + query_offset;
                        /* le temps qu'il faut pour envoyer ce packet*/
                        /* dependant de la bande passante qu'on s'autorise */
                        ptab[i]->rtp_ts = rtp_ts; /* sample time when file come*/
                        ptab[i]->duration = d_duration; /* le temps qu'il faut pour envoie
                    
                                        /* a la vitesse desiree  en millisec*/
                        offset += p_d_l;
                        r_b = r_b - p_d_l;
                        if (r_b <= s_dchunk) {
                                p_d_l = r_b;            /* packet data len */
                        } else {
                                p_d_l = s_dchunk;
                        }
#ifdef DEBUG_MULTICAST                
                fprintf (stderr,"McSendObjectRepairAnswerCb: pckting moid %d, offset %d, d_len %d, is_eod %d\n", ptab[i]->id, ptab[i]->offset, ptab[i]->d_len, ptab[i]->is_eod);
#endif 
                }
		ptab[n_t -1]->to_free = obdata; /* free in McSendRtpDataTimeOutCb */
                if ( ptab[n_t -1]->offset + s_dchunk >= data_size) {
                        ptab[n_t -1]->is_eod =  1;
#ifdef DEBUG_MULTICAST                
                fprintf (stderr,"McSendObjectRepairAnswerCb: pckting (ovewrite) moid %d, offset %d, d_len %d, is_eod %d\n", ptab[n_t -1]->id, ptab[n_t -1]->offset, ptab[n_t -1]->d_len, ptab[n_t -1]->is_eod);
#endif 
                        ptab[n_t -1]->to_free = obdata; /* free in McSendRtpDataTimeOutCb */
                }
#ifdef DEBUG_MULTICAST
		fprintf(stderr, "offset %d, s_dchunk %d, data_size %d\n",
			offset , s_dchunk, data_size);
#endif
/*voir si on envoie par UNI ou MULTI et si on rearme le timer multicast... */
/* FIXME: compute something to choose between UNICAST or MULTIUCAST */
                if ( p->source_count == 1) { /* unicast */
                        int j;

                        for (j = 0; j< n_t ; j++) { /* FIXME: not a full bw */
                                UcRtpSendDataPacketTo(p->addr_ip,p->port, ptab[j]);
                                free(ptab[j]);
                        }
                        free(ptab);
			free(obdata);
                } else { /* multicast */
/* add at end ########## */
                        if (deb_p){
                                end_p->next = ptab[0];
                        } else { 
                                mc_rtp_packets_list = ptab[0];
                        }        
                        free(ptab);
                        have_send_multi = 1;
                }
                pp =p;
                p = p->next;
                free(pp);
		mc_queue_object_repair_query = p;
        }
/* rearm if necessary */
        if (rearm_timer == True && have_send_multi == 1){
                mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
                        mc_write_rtp_data_next_time,
                        McSendRtpDataTimeOutCb, NULL);
        }
#ifdef MULTICAST
fprintf(stderr, "McSendObjectRepairAnswerCb: DESACTIVATE timer\n");
#endif
}

static XtIntervalId mc_queue_object_repair_query_timer_id;
static int mc_queue_object_repair_query_time = 100;

/* for Object, data is in file */
static void McScheduleObjectRepairAnswer( Source *s, u_int32_t id,
	u_int32_t offset, int len)
{
        McQueueObjectRepairQuery *head = mc_queue_object_repair_query;
        McQueueObjectRepairQuery *p, *pp;


        if (!head) {
                head = (McQueueObjectRepairQuery *)malloc(
                                sizeof(McQueueObjectRepairQuery));
                head->next = NULL;
                head->addr_ip= s->uc_rtp_ipaddr;
                head->port=s->uc_rtp_port;

                head->source_count = 1;
                head->id = id;
                head->offset = offset;
                head->len = len;
                mc_queue_object_repair_query = head ;
                mc_queue_object_repair_query_timer_id = XtAppAddTimeOut(
                        mMosaicAppContext, mc_queue_object_repair_query_time,
                        McSendObjectRepairAnswerCb, NULL);
#ifdef DEBUG_MULTICAST
fprintf(stderr, "McScheduleObjectRepairAnswer: Activate McSendObjectRepairAnswerCb for id %d, offset %d, len %d\n", id , offset, len);
#endif
                return;
        }

#ifdef DEBUG_MULTICAST
fprintf(stderr, "McScheduleObjectRepairAnswer: QUEUING for id %d, offset %d, len %d\n", id , offset, len);
#endif
        /* look in queue if query still exist */
        p = head;
        pp = NULL;
        while (p) {
                if ( p->id == id && p->offset == offset && p->len == len) {
                        p->source_count++;
                        return;
                }
                pp = p;
                p = p->next;
        }
/* add at end , if query doesn't still exist */
        p = (McQueueObjectRepairQuery *)malloc(
                                sizeof(McQueueObjectRepairQuery));
        p->next = NULL;
        p->addr_ip= s->uc_rtp_ipaddr;
        p->port=s->uc_rtp_port;

        p->source_count = 1;
        p->id = id;
        p->offset = offset;
        p->len = len;
        pp->next = p;
/*###   maybe recompute time if toomany query ###*/
}

/* i am a sender. I send some packet and a receiver need to repair some */
/* packet. this receiver send me a query to repair its packet */
/* query is for me. I reply to 's'(unicast) or multicast depending */
/* of the number of query and net parameter */
/* reply with data. But before we need to analyze the query and check */
/* for scalability (Hum!dream...) */

void McStoreQueryRepair(Source *s , RtcpPacket* rcs)
{
	RtcpRepaird *psrd = (RtcpRepaird *) rcs->d;
	u_int32_t my_ssrc ;
	u_int32_t queryed_ssrc;
	u_int32_t id, len, offset,bits;
	int qtype;

	queryed_ssrc = s->srcid;

	my_ssrc = ntohl(psrd->target_ssrc);	/* that is me */
	bits = ntohl(psrd->bits);
	id = ntohl(psrd->id);
	offset = ntohl(psrd->offset);
	len = ntohl(psrd->len);
	qtype = bits & 0x02;	/* query type */

	switch(qtype) {
	case HTML_STATE_DATA_TYPE:
		if( !McCheckStateQuery(id, offset, len) )
			return;
		McScheduleStateRepairAnswer(s, id, offset, len);
		break;
	case HTML_OBJECT_DATA_TYPE:
		if( !McCheckObjectQuery(id, offset, len) )
			return;
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McStoreQueryRepair: receive a query for id %d offset %d len %d\n", id , offset, len);
#endif
		McScheduleObjectRepairAnswer(s, id, offset, len);
		break;
	default:
		fprintf(stderr,"McStoreQueryRepair: invlaid repair type\n");
		abort();	/* let me know */
		return;
	}
}
