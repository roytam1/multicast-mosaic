#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <malloc.h>
#include <assert.h>

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../libhtmlw/HTML.h"

#include "mc_mosaic.h"
#include "../src/img.h"
#include "mc_misc.h"
#include "mc_main.h"

/**************************************************************************/
/* SENDER PART								  */
/**************************************************************************/

static void McObjectToPacket( McObjectStruct *obs)
{
        RtpPacket **ptab;
        RtpPacket * deb_p, *end_p;
        int s_dchunk, i, n_t, r_b, offset, p_d_l;
        int rtp_ts, d_duration;
	int object_size;
	int fdd;
        double d_dur;
	char *odata;
	char *fname;
	char *aurl; 
	MimeHeaderStruct * mhs; 
	int moid;

	fname = obs->fname;
	aurl = obs->aurl;
	mhs = obs->mhs;
	moid = obs->moid;
	object_size = obs->file_len;
 
        deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
	end_p = mc_rtp_packets_list;
        if (deb_p) { /* il y a une file d'attente */
/* volontary lost packet, un utilisateur va trop vite au regard de la bande */
/* passante qu'il s'autorise. complique car il faut recuprer le seqno */   
/* next_time = UnSchedule(deb_p, end_p); deb_p=... end_p=... */
		while (end_p->next){
			end_p = end_p->next;
		}
	}

	odata = (char*) malloc(object_size +1);
	fdd = open(fname,O_RDONLY);
	read(fdd, odata, object_size);	/* append */
	odata[object_size] = '\0';
	close(fdd);
/* send header and data in one contiguous stream */
        s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */

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
                p_d_l = r_b;            /* packet data len */
        } else {                       
                p_d_l = s_dchunk;      
        }                              
        rtp_ts = McRtpTimeStamp(mhs->ts); /* sample time when file come */                                       
        for ( i = 0; i < n_t; i++ ) {  
                d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond */                               
                d_duration = (int) d_dur;
                if (d_duration < 2)    
                        d_duration = 2;
                ptab[i]->next = ptab[i+1];
                ptab[i]->id = moid;
		ptab[i]->data_type = HTML_OBJECT_DATA_TYPE;
                ptab[i]->d_len = p_d_l;
                ptab[i]->d = &odata[offset];
                ptab[i]->is_eod = 0;   
		ptab[i]->to_free = NULL;
                ptab[i]->offset = offset;
                        /* le temps qu'il faut pour envoyer ce packet*/
                        /* dependant de la bande passante qu'on s'autorise */
                ptab[i]->rtp_ts = rtp_ts; /* sample time when file come */
                ptab[i]->duration = d_duration; /* le temps qu'il faut pour envoie
                                         * a la vitesse desiree  en millisec*/
                offset += p_d_l;       
                r_b = r_b - p_d_l;     
                if (r_b <= s_dchunk) { 
                        p_d_l = r_b;            /* packet data len */
                } else {               
                        p_d_l = s_dchunk;
                }
#ifdef DEBUG_MULTICAST
		fprintf (stderr,"McObjectToPacket: pckting moid %d, offset %d, d_len %d, is_eod %d\n", moid, ptab[i]->offset, ptab[i]->d_len, ptab[i]->is_eod);
#endif
        }                              
        ptab[n_t -1]->is_eod = 1;   
#ifdef DEBUG_MULTICAST
		fprintf (stderr,"McObjectToPacket: pckting (ovewrite) moid %d, offset %d, d_len %d, is_eod %d\n", ptab[n_t -1]->id, ptab[n_t -1]->offset, ptab[n_t -1]->d_len, ptab[n_t -1]->is_eod);
#endif

	ptab[n_t -1]->to_free = odata; /* free in McSendRtpDataTimeOutCb */
	if (deb_p){
		end_p->next = ptab[0];
	} else {
        	mc_rtp_packets_list = ptab[0]; 
	}
        free(ptab);
}

void McSendOject(int moid)
{
	McObjectStruct *obs;
	int rearm_timer = mc_rtp_packets_list ? False : True;

        obs = &moid_sender_cache[moid];
	McObjectToPacket(obs);

/* a new object */
	if (rearm_timer == True){
		mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
			mc_write_rtp_data_next_time,
			McSendRtpDataTimeOutCb, NULL);
	}
}

void McStateToPacket( McStateStruct *s)
{
	RtpPacket **ptab;
	RtpPacket * deb_p, *end_p;
	int s_dchunk, i,n_t, r_b,offset, p_d_l;
	int rtp_ts, d_duration;
	double d_dur;
	char *odata;
	int object_size;

	deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
	end_p = mc_rtp_packets_list;

	if (deb_p) { /* il y a une file d'attente */
                while (end_p->next){
                        end_p = end_p->next;
                }
        }

	odata = s->sdata;	/*  don't free this  */
				/* a state is always in memory */
	object_size = s->sdata_len;

        s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */
                        
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
                p_d_l = r_b;            /* packet data len */
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
                ptab[i]->offset = offset;
                        /* le temps qu'il faut pour envoyer ce packet*/
                        /* dependant de la bande passante qu'on s'autorise */
                ptab[i]->rtp_ts = rtp_ts; /* sample time when file come */
                ptab[i]->duration = d_duration; /* le temps qu'il faut pour envoie                                         * a la vitesse desiree  en millisec*/
                offset += p_d_l;       
                r_b = r_b - p_d_l;     
                if (r_b <= s_dchunk) { 
                        p_d_l = r_b;            /* packet data len */
                } else {               
                        p_d_l = s_dchunk;
                }                      
        }                              
        ptab[n_t -1]->is_eod = 1;      
	ptab[n_t -1]->to_free = NULL; /* Dont'free in McSendRtpDataTimeOutCb */
        if (deb_p){                    
                end_p->next = ptab[0]; 
        } else {                       
                mc_rtp_packets_list = ptab[0];
        }                              
        free(ptab);                    
}

void McSendState(int stateid)
{
	McStateStruct *state;
	int rearm_timer = mc_rtp_packets_list ? False : True;

	state = &mc_sender_state_tab[stateid];
	McStateToPacket(state);
#ifdef DEBUG_MULTICAST
	fprintf(stderr, "McSendState: %s\n", state->sdata);
#endif
/* a new state */
	if (rearm_timer == True){
		mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
			mc_write_rtp_data_next_time,
			McSendRtpDataTimeOutCb, NULL);
	}
}


/**************************************************************************/
/* RECEIVER PART						          */
/**************************************************************************/

static int UpdChkBuf(ChunkedBufStruct *cbs, char *d, unsigned int offset,
	unsigned int d_len)
{
	MissRange *plmr,*fmr;
	MissRange cmr;

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"UpdChkBuf: offset = %d, d_len = %d, size_data = %d\n",
 		offset, d_len, cbs->size_data);
#endif
	if ( cbs->data) {
		if(offset + d_len > cbs->size_data) { /* bug??? */
			fprintf(stderr, "Complexe BUG in UpdChkBuf offset+d_len > coe->size_data\007\n");
			assert(0);
			return CHUNKED_BUFFER;
		}
	}

	plmr = cbs->lmr;
	if (!plmr) {
		assert(0); /* impossible */
		return COMPLETE_BUFFER;
	}
	while( plmr ) {
		if ( !( offset >= plmr->from && offset <= plmr->to) ) {
			plmr = plmr->next;
			continue;
		}
/* update data plmr coe->beg coe->end lmr */
		if (cbs->data) {
			memcpy(&cbs->data[offset], d, d_len);
		}
/* now fork or merge MissRange */
		cmr = *plmr;
		if ( cmr.from == offset) {
			if (offset + d_len -1 < cmr.to) { /* adjust from*/
				plmr->from = offset + d_len;
				return CHUNKED_BUFFER;
			} 
			/* remove MissRange */
			free(plmr);
			if (cmr.next == NULL && cmr.prev == NULL){
				/* no range missing */
				cbs->lmr = NULL;
				return COMPLETE_BUFFER;
			}
			if (cmr.next == NULL) {
				/* remove at end */
				cmr.prev->next = NULL;
				return CHUNKED_BUFFER;
			}				
			if (cmr.prev == NULL ) {
				/* remove at begin */
				cbs->lmr = cmr.next;
				cbs->lmr->prev = NULL;
				return CHUNKED_BUFFER;
			}
			/* remove in middle */
			cmr.prev->next = cmr.next;
			cmr.next->prev = cmr.prev;
			return CHUNKED_BUFFER;
		}
/* cas ou cmr.from < offset. Il ne peut etre > offset */
		if (offset + d_len -1 >= cmr.to ) { /* adjust to */
			plmr->to = offset -1;
			return CHUNKED_BUFFER;
		}
		/* fork this MissRange */
		fmr = (MissRange *) malloc( sizeof (MissRange ));
		fmr->next = cmr.next;
		fmr->prev = plmr;
		fmr->from = offset + d_len;
		fmr->to = plmr->to;
		plmr->next = fmr;
		plmr->to = offset -1;
		return CHUNKED_BUFFER;
	}
	return CHUNKED_BUFFER;
}

/* lpdc must be merge in data , because the size of object is know */
static int MergeChkBufLpdc(int size, ChunkedBufStruct *cbs, char *d, int offset,
	int d_len)
{
	PacketDataChunk *p, *plpdc;
	int status = CHUNKED_BUFFER;
	int f_status = CHUNKED_BUFFER;
	MissRange *plmr, *nlmr;

	cbs->size_data = size;
	cbs->data = (char*) malloc(size + 1);
	cbs->lmr->from = 0;
	cbs->lmr->to = size - 1 ;
	plmr = cbs->lmr->next;
	while (plmr) {	/* free old and restart */
		nlmr = plmr->next;
		free(plmr);
		plmr = nlmr;
	}
	cbs->lmr->next = NULL;

	cbs->data[size] = '\0';
	status = UpdChkBuf(cbs, d, offset, d_len);
	f_status = status;
	plpdc = cbs->lpdc;
	while (plpdc) {
		status = UpdChkBuf(cbs, plpdc->d, plpdc->offset,
			plpdc->d_len);
		if (status == COMPLETE_BUFFER)
			f_status = COMPLETE_BUFFER;
		p = plpdc;
		plpdc = plpdc->next;
		free(p->d);
		free(p);
	}
	cbs->lpdc = NULL;
	return f_status;
}

/* o_status:
        - complete : this packet full fill the embedded object
        - incomplete: there is missing packet
        - still_here: we have still see this data
*/  
int PutPacketInChkBuf(ChunkedBufStruct *cbs, int is_end, int offset,
	char * d, int d_len)
{
	int size, status;
	PacketDataChunk *plpdc;
	PacketDataChunk *last_lpdc;

	if (cbs->data) { /* the size of object is know, is_end had been seen */
		if ( is_end && cbs->end ) {
			return CHUNKED_BUFFER;
		}
		status = UpdChkBuf(cbs, d, offset, d_len);
		return status;
	}
/* the size of object is not know, because no is_end */
/* 3 cas se presentent 
        offset = 0 et is_end => 1 seul packet.
	offset = i et !is_end => packet du milieu
        offset = n et is_end, fin qui permet de determine la taille de l'ojet
                ( a calculer avec offset et taille du packet)
*/ 

	if (offset == 0 && is_end ) { /* compute size */
		cbs->data = (char*) malloc(d_len+1);
		cbs->size_data = d_len;
		memcpy(cbs->data, d, d_len);	
		cbs->data[d_len] = '\0';	/* in case of text */
		free(cbs->lmr);
		cbs->lmr = NULL; 	/* sanity */
		return COMPLETE_BUFFER;
	}
	if (is_end) {	/* last packet, compute size */
		size = offset + d_len;
		status = MergeChkBufLpdc(size, cbs, d, offset, d_len);
		return status;
	}

/* size unknow, store packet */
	if (cbs->lpdc == NULL) {
		cbs->lpdc = (PacketDataChunk*)calloc(1,sizeof(PacketDataChunk));
		cbs->lpdc->offset = offset;
		cbs->lpdc->d_len = d_len;
		cbs->lpdc->d = (char*)malloc(d_len);
		memcpy(cbs->lpdc->d, d, d_len);
		status = UpdChkBuf(cbs, d, offset, d_len);
	  /* update lmr because of UcSendRepairMcScheduleQueryRepairObjects */
		return CHUNKED_BUFFER;
	}
/* sort by offset */
	plpdc = cbs->lpdc;
	while (plpdc) {
		if (plpdc->offset == offset) {
			return CHUNKED_BUFFER; /*duplicate */
		}
		last_lpdc = plpdc;
		plpdc = plpdc->next;
	}
/* ###insert by offset order....  */
	last_lpdc->next = (PacketDataChunk*)calloc(1,sizeof(PacketDataChunk));
	plpdc = last_lpdc->next;
	plpdc->offset = offset;
	plpdc->d_len = d_len;
	plpdc->d = (char*)malloc(d_len);
	memcpy(plpdc->d, d, d_len);
	status = UpdChkBuf(cbs, d, offset, d_len);
		 /* update lmr because of UcSendRepair McScheduleQueryRepairObjects*/
	return CHUNKED_BUFFER;
}

/* input
 *	cbs: chunked data to assemble
 * output
 *	buf_ret: malloced buffer when succes
 * return:
 *	len of malloced buf_ret
 * REMARQUES: free cbs
*/
int ChkBufToBuf(ChunkedBufStruct *cbs, char ** buf_ret)
{
	int len ;

	len = cbs->size_data;
	*buf_ret = cbs->data;
	free(cbs);
	return len;
}
