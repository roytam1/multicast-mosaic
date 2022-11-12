#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <assert.h>

#include <Xm/XmAll.h>

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
	u_int32_t	sid;	/* C'est le dernier etat complet que l'emetteur */
				/* a envoyer , c'est celui qu'il fauy affiche */
	u_int32_t	oid;	/* C'est le dernier objet complet que l'emetteur*/
				/* a envoyer */
	u_int32_t	c_sid;	/* current state id in transfer. C'est l'etat */
				/* en cours de transfert , cote emetteur */
				/* si le transfert est fini alors c_sid=sid */
				/* sinon c_sid > sid */
	u_int32_t	rtp_otime; /* RTP sample time of oid */
	u_int32_t	reserved;  /* extension ... */
} RtcpStard;

static void McScheduleQueryRepairState(Source *s, int sid)
{
	/* from = send repair from this state */
	ChunkedBufStruct *cbs;
	MissRange * plmr;
	int offset;
	int len;

	switch (s->states[sid].buffer_status) {
	case CHUNKED_BUFFER:
		cbs = s->states[sid].chkbuf;
		plmr = cbs->lmr;
		while(plmr){
			offset=plmr->from;
			len = plmr->to - offset +1;
			if (plmr->to == 0xffffffff)
				len = 0xffffffff;
			
			UcSendRepair(s, HTML_STATE_DATA_TYPE, sid,
				offset , len);
			plmr = plmr->next;
		}
		break;
	case EMPTY_BUFFER:
		UcSendRepair(s, HTML_STATE_DATA_TYPE, sid,
			0, 0xffffffff);	/* request all */
		break;
	default:
		assert(0);
	}
}

static void McScheduleQueryRepairObject(Source *s, int oid)
{
	/* send query repair to Source s */
	/* oid = send repair from this object */
	ChunkedBufStruct *cbs;
	MissRange * plmr;
	int offset;
	int len;

	if (s->objects[oid].buffer_status == PARSED_BUFFER ||
	    s->objects[oid].buffer_status == COMPLETE_BUFFER ||
	    s->objects[oid].buffer_status == PARSED_ALL_DEPEND_BUFFER) {
		return;
	}
	switch (s->objects[oid].buffer_status) {
	case CHUNKED_BUFFER:
		cbs = s->objects[oid].chkbuf;
		plmr = cbs->lmr;
		while(plmr){
			offset=plmr->from;
			len = plmr->to - offset +1;
			if (plmr->to == 0xffffffff)
				len = 0xffffffff;
			
			UcSendRepair(s, HTML_OBJECT_DATA_TYPE, oid,
				offset , len);
			plmr = plmr->next;
		}
		break;
	case EMPTY_BUFFER:
		UcSendRepair(s, HTML_OBJECT_DATA_TYPE, oid,
			0, 0xffffffff);	/* request all */
		break;
	default:
		assert(0);
	}
}

static void McScheduleQueryRepairObjectTree( Source *s, int moid)
{
	int status;
	int i;
	int no;

	status = s->objects[moid].buffer_status;
	if (status == PARSED_ALL_DEPEND_BUFFER ){
		return;
	}
	if ( !(status == PARSED_BUFFER || status == COMPLETE_BUFFER) ) {
		McScheduleQueryRepairObject(s, moid);
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
		McScheduleQueryRepairObjectTree(s, s->objects[moid].dot[i]);
	}	
}

void McQueryRepairFromStatr(Source *s, RtcpPacket* rcs)
{
	RtcpStard *psrd = (RtcpStard *) rcs->d;
	int cf_sid;	/* etat en cours de formation */
	int dec_sid;	/* dernier etat complet */
	int dec_oid;	/* dernier objet complet */
	int rtp_otime;
	McBufferStatus status;
	int do_return = 0;
	McStateStruct st;
	int i;

	cf_sid = ntohl (psrd->c_sid);
	rtp_otime = ntohl (psrd->rtp_otime);

	dec_sid = ntohl (psrd->sid);	/* c'est lui que je dois afficher*/
	dec_oid = ntohl (psrd->oid);

/* la premiere fois que la source est vue, on cree sa structure */
	assert(s->states_tab_size);

/* reallocation de l'espace state_id , si il grandi */
	McRcvrSrcAllocState(s, cf_sid);
	McRcvrSrcAllocState(s, dec_sid);

/* check and alloc enought object. repair missing object */
/* this may be long to repair because object may depend of object... */
        McRcvrSrcAllocObject(s, dec_oid);

/* On recoit un numero d'etat sans ses donnees: */
/*      - soit l'etat est incomplet parcequ'il manque qqes objets       */
/*        auquel cas on les demande.                                    */
/*      - soit l'etat est complet. Dans ce cas                          */
/*              - soit l'etat affiche est le meme-> return              */
/*              - soit il est different, et il faut l'afficher          */

	if ( s->states[dec_sid].state_status == STATE_COMPLETED ) {
                                /* l'etat est complet */
                if (s->current_view_state == dec_sid ) {
                                /* l'etat recu est celui affiche... */
                        return;       
                }                     
/* state is COMPLETE and all depend object of object are here, play with them*/
/* Display it now */                  
                McDisplayWindowText(s, dec_sid);    /*Display full doc */
                return;               
        }

/* l'etat n'est pas complet:
	- soit les donnees de l'etat ne sont pas la
	- soit il manque des objets
   Faire un repair!
*/

/* teste le cas ou les donnees d'etat ne sont pas la */
	if ( s->states[dec_sid].buffer_status != PARSED_BUFFER){
		McScheduleQueryRepairState(s, dec_sid); 
		return;
	}

/* les donnees d'etat sont la ! */
	
	st = s->states[dec_sid]; /* l'analyse est deja faite */

/* on commence par le start_moid */
	McRcvrSrcAllocObject(s, st.start_moid);

/* on teste seulement les OBJETS (pas l'etat) dependant d'autre objets */
/* parcequ'un frameset a une dependance variable ... */

	status = McRcvrSrcCheckBufferObject(s, st.start_moid);
	if (status != PARSED_ALL_DEPEND_BUFFER) {       /* cas html */
			/* le principal objet n'est pas la. faire un repair */
		McScheduleQueryRepairObjectTree(s, st.start_moid);
		return;
	}

/* le start_moid est la */

/* Traite le cas html */
	if (st.n_fdo == 0 ) {
		s->states[dec_sid].state_status = STATE_COMPLETED;
		McDisplayWindowText(s, dec_sid);    /*Display full doc */
                return;               
        }

/* traite le cas d'un frameset */

	for( i = 0; i < st.n_fdo; i++) {
		McRcvrSrcAllocObject(s, st.fdot[i]);

		status = McRcvrSrcCheckBufferObject(s, st.fdot[i]);
		if (status != PARSED_ALL_DEPEND_BUFFER) {
			McScheduleQueryRepairObjectTree(s, st.fdot[i]);
			do_return =1;
		}
	}

	if (do_return)
		return;

	s->states[dec_sid].state_status = STATE_COMPLETED;
	McDisplayWindowText(s, dec_sid);
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
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UcSendRepair to %08x %04x\n",
		ntohl(s->uc_rtp_ipaddr), ntohs(s->uc_rtcp_port));
	fprintf(stderr,"UcSendRepair: %d %d %d %d\n",
		type, id, offset, lend);
#endif
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

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McSendStateRepairAnswerCb\n");
#endif
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
			assert(0);
		}
		if (query_len < 0 ) {
			query_len = data_size - query_offset;
		}
		if ( query_offset + query_len > data_size) { /* adjust len */
#ifdef DEBUG_MULTICAST
			fprintf(stderr, "BUG len adjust \007\n");
#endif
			query_len = data_size - query_offset;
		}
		if (query_len <= 0 ) {
			assert(0);
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
#ifdef DEBUG_MULTICAST
				fprintf(stderr,"McSendStateRepairAnswerCb: send unicast state\n%s\n", ptab[j]->d);
				fprintf(stderr,"McSendStateRepairAnswerCb: calling UcRtpSendDataPacketTo\n");
#endif
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
#ifdef DEBUG_MULTICAST
			fprintf(stderr,"McSendStateRepairAnswerCb: send Multicast AppendTo mc_rtp_packets_list\n");
#endif
		}
		pp =p;
		p = p->next;
		free(pp);
		mc_queue_state_repair_query = p;
	}
/* rearm if necessary */
        if (rearm_timer == True && have_send_multi == 1){      
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McSendStateRepairAnswerCb: Rearm mc_write_rtp_data_timer_id\n");
#endif
                mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
                        mc_write_rtp_data_next_time,
                        McSendRtpDataTimeOutCb, NULL);
        }
#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McSendStateRepairAnswerCb: return\n");
#endif
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
#ifdef DEBUG_MULTICAST
void printQueueStateRepairEntry(McQueueStateRepairQuery *p)
{
	fprintf(stderr,"printQueueStateRepairEntry: State id %d, offset %d, len %d, addr_ip %d.%d.%d.%d, port %d, source_count %d\n",
		p->id,
		p->offset,
		p->len,
		(p->addr_ip >> 24)&0xff, (p->addr_ip >> 16)&0xff, (p->addr_ip>>8)&0xff, (p->addr_ip)&0xff,
		p->port,
		p->source_count);
}
#endif

static void McScheduleStateRepairAnswer( Source *s, u_int32_t id,
	u_int32_t offset, int len)
{
/* for State, the data is in memory */
	McQueueStateRepairQuery *head = mc_queue_state_repair_query;
	McQueueStateRepairQuery *p, *pp;

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McScheduleStateRepairAnswer\n");
#endif
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
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McScheduleStateRepairAnswer: Create Queue\n");
		printQueueStateRepairEntry(head);
#endif
		return;
	}

	/* look in queue if query still exist */
	p = head;
	pp = NULL;
	while (p) {
		if ( p->id == id && p->offset == offset && p->len == len) {
			p->source_count++;
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McScheduleStateRepairAnswer: Old In Queue source_count++\n");
		printQueueStateRepairEntry(p);
#endif
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
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McScheduleStateRepairAnswer: New In Queue Appending\n");
		printQueueStateRepairEntry(p);
#endif
	return;
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
                        assert(0);
                }
                if (query_len < 0 ) {
                        query_len = data_size - query_offset;
                }
                if ( query_offset + query_len > data_size) { /* adjust len */
#ifdef DEBUG_MULTICAST
                        fprintf(stderr, "BUG len adjust \007\n");
#endif
                        query_len = data_size - query_offset;
                }
                if (query_len <= 0 ) {
                        assert(0);
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
#ifdef DEBUG_MULTICAST
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

#ifdef DEBUG_MULTICAST
	fprintf(stderr, "McStoreQueryRepair\n");
#endif
	queryed_ssrc = s->srcid;

	my_ssrc = ntohl(psrd->target_ssrc);	/* that is me */
	bits = ntohl(psrd->bits);
	id = ntohl(psrd->id);
	offset = ntohl(psrd->offset);
	len = ntohl(psrd->len);
	qtype = bits & 0x02;	/* query type */

	switch(qtype) {
	case HTML_STATE_DATA_TYPE:
		if( !McCheckStateQuery(id, offset, len) ) {
#ifdef DEBUG_MULTICAST
			fprintf(stderr, "McStoreQueryRepair: StateQuery NOT exist return\n");
#endif
			return;
		}
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McStoreQueryRepair: receive a query for State id %d offset %d len %d\n", id , offset, len);
		fprintf(stderr,"Calling McScheduleStateRepairAnswer\n");
#endif
		McScheduleStateRepairAnswer(s, id, offset, len);
		break;
	case HTML_OBJECT_DATA_TYPE:
		if( !McCheckObjectQuery(id, offset, len) ) {
#ifdef DEBUG_MULTICAST
			fprintf(stderr, "McStoreQueryRepair: ObjectQuery NOT exist return\n");
#endif
			return;
		}
#ifdef DEBUG_MULTICAST
		fprintf(stderr,"McStoreQueryRepair: receive a query for Object id %d offset %d len %d\n", id , offset, len);
		fprintf(stderr,"Calling McScheduleObjectRepairAnswer\n");
#endif
		McScheduleObjectRepairAnswer(s, id, offset, len);
		break;
	default:
		fprintf(stderr,"McStoreQueryRepair: invlaid repair type\n");
		assert(0);
		return;
	}
#ifdef DEBUG_MULTICAST
	fprintf(stderr, "McStoreQueryRepair: return\n");
#endif
}
