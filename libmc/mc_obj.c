#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/uio.h>
#include <unistd.h>
#include <malloc.h>

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/mime.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLparse.h"
#include "../src/img.h"

#include "mc_misc.h"
#include "mc_main.h"
#include "mc_obj.h"
#include "mc_io.h"
#include "mc_session.h"

void McFillLocalOject(ObjEntry ** obj, int num, char * fname, char *aurl_wa,
	MimeHeaderStruct * mhs)
{
	int fdd;
	char * buf=NULL;
	char ces[50];

	ces[0] = '\0';
	switch(mhs->content_encoding) {
	case GZIP_ENCODING :
		sprintf(ces,"Content-Encoding: gzip\n"); 
		break;
	case COMPRESS_ENCODING:
		sprintf(ces,"Content-Encoding: compress\n");
		break;
	}

	buf = (char*)malloc(strlen(aurl_wa)+strlen(ces)+
		strlen(mhs->content_type)+strlen(mhs->last_modified)+220);
	sprintf(buf, "GET %s HTTP/1.0\n\
Content-Length: %d\n%s\
Content-Type: %s\n\
Last-Modified: %s\n\
\n\n", aurl_wa, mhs->content_length, ces, mhs->content_type,
mhs->last_modified);
	
	obj[num] = (ObjEntry *) calloc(1, sizeof(ObjEntry ));
	obj[num]->h_part = buf;
	obj[num]->h_len = strlen(buf);
	obj[num]->h_fname = NULL;
	if(obj[num]->h_len > 500)
		fprintf(stderr,"Mc: header is long\n");

        obj[num]->d_len = mhs->content_length;
	fdd = open(fname,O_RDONLY);
	buf = (char*) malloc(obj[num]->d_len);
	read(fdd,buf, obj[num]->d_len);
        obj[num]->d_part = buf;
        obj[num]->d_fname = NULL; 	/* just now ### */
	close(fdd);
}

void McCreateDocEntry(char *fname, char* aurl_wa, MimeHeaderStruct *mhs)
{                                      
        int i,n_docs ;                 
                                       
        i = mc_local_url_id;           
        n_docs = mc_local_url_id +1;   
        if ( !mc_local_docs) { /* create a tab of docs */
                mc_local_docs = (DocEntry *)malloc(sizeof(DocEntry) *n_docs );
        } else {                       
                mc_local_docs = (DocEntry *)realloc(mc_local_docs,
                         sizeof(DocEntry) * n_docs);
        }
/* #### just le HTML pour l'instant */
        mc_local_docs[i].o_tab = (ObjEntry**)calloc(1, sizeof(ObjEntry*));
        McFillLocalOject(mc_local_docs[i].o_tab, 0, fname, aurl_wa, mhs);
        mc_local_docs[i].nobj = 1;              /* nombre d'objet */
        mc_local_docs[i].o_tab[0]->o_num = 0;    /* numero de cet objet */
        gettimeofday(&mc_local_docs[i].o_tab[0]->ts,0) ;
} 

void McDocToPacket(int url_id)
{
        DocEntry doc;
        RtpPacket *ph;          /* one for header */
        RtpPacket **ptab;
        RtpPacket * deb_p;
        int s_dchunk; 
        int i, n_d, n_t, s_oh, r_b, offset, d_off;
        int p_d_l;
        int rtp_ts;
        int d_duration;
        double d_dur;
        int next_time = 10;
 
        deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
        if (deb_p) { /* il y a une file d'attente */
                /* volontary lost packet, un utilisateur va trop vite au regard
                 * de la bande passante qu'il s'autorise... */
                /* complique car il faut recuprer le seqno */   
/*              next_time = UnSchedule(deb_plist, end_plist); */
/*              deb_plist = ... */
/*              end_plist = ... */
        }               
 
/* send the header in the first packet as a standalone packet */
        s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */
        doc = mc_local_docs[url_id];
/* size of object include header */
        s_oh = doc.o_tab[0]->h_len + doc.o_tab[0]->d_len;
        if (doc.o_tab[0]->h_len <= 0 || doc.o_tab[0]->d_len<=0)  /* impossible */
                abort();
/* nombre de packet pour les donnes */ 
        n_d = (doc.o_tab[0]->d_len -1) / s_dchunk; /* division entiere */
        n_d++;                         
/* total */                            
        n_t = n_d + 1;                 
                                       
/* alloc packets include one for header . Plus one for NULL*/
        ptab = (RtpPacket **) malloc(sizeof(RtpPacket*) * (n_t+1));
        for ( i = 0; i < n_t; i++ )    
                ptab[i] = (RtpPacket *)malloc(sizeof(RtpPacket) );
        ptab[n_t] = NULL;              
                                       
        r_b = s_oh;     /* remaining byte to send */
        offset = 0;                    
        p_d_l = doc.o_tab[0]->h_len;    
        rtp_ts = McRtpTimeStamp(doc.o_tab[0]->ts); /* sample time when file come */                                       
/* fill first packets (header) */      
                                       
        d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond */        d_duration = (int) d_dur;      
        if (d_duration < 2)            
                d_duration = 2;        
        ptab[0]->next = ptab[1];       
        ptab[0]->url_id = url_id;      
        ptab[0]->o_id = 0;             
        ptab[0]->d = doc.o_tab[0]->h_part;
        ptab[0]->d_len = p_d_l;        
        ptab[0]->is_end = 0;           
        ptab[0]->offset = offset;      
                /* le temps qu'il faut pour envoyer ce packet*/
                /* dependant de la bande passante qu'on s'autorise */
        ptab[0]->rtp_ts = rtp_ts; /* sample time when file come */
        ptab[0]->duration = d_duration; /* le temps qu'il faut pour envoie
                                         * a la vitesse desiree  en millisec*/
        offset += p_d_l;               
        r_b = r_b - p_d_l;             
                                       
        if (r_b <= s_dchunk) {         
                p_d_l = r_b;            /* packet data len */
        } else {                       
                p_d_l = s_dchunk;      
        }                              
        d_off = 0;                     
        for ( i = 1; i < n_t; i++ ) {  
                d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond */                               
                d_duration = (int) d_dur;
                if (d_duration < 2)    
                        d_duration = 2;
                ptab[i]->next = ptab[i+1];
                ptab[i]->url_id = url_id;
                ptab[i]->o_id = 0;     
                ptab[i]->d_len = p_d_l;
                ptab[i]->d = &doc.o_tab[0]->d_part[d_off];
                ptab[i]->is_end = 0;   
                ptab[i]->offset = offset;
                        /* le temps qu'il faut pour envoyer ce packet*/
                        /* dependant de la bande passante qu'on s'autorise */
                ptab[i]->rtp_ts = rtp_ts; /* sample time when file come */
                ptab[i]->duration = d_duration;
                                       
                d_off += p_d_l;        
                offset += p_d_l;       
                r_b = r_b - p_d_l;     
                if (r_b <= s_dchunk) { 
                        p_d_l = r_b;            /* packet data len */
                } else {               
                        p_d_l = s_dchunk;
                }                      
        }                              
        ptab[n_t -1]->is_end = 0x80;   
        mc_rtp_packets_list = ptab[0]; 
        free(ptab);                    
}

/* allocation si necessaire d'une entree Doc */
ChunkedDocEntry * GetChkDocAndCotab(Source *s, unsigned int url_id, unsigned int o_id)
{
	ChunkedDocEntry * cdoce;
	int f, t, i;

	if (!s->chkdoc[url_id]){
		s->chkdoc[url_id] = (ChunkedDocEntry*) calloc(1, sizeof(ChunkedDocEntry));
		s->chkdoc[url_id]->url_id = url_id;
		s->chkdoc[url_id]->nobj = 0;
		s->chkdoc[url_id]->h_nobj = 0;
		s->chkdoc[url_id]->co_tab = NULL;
		/* ### all of this MUST be free when completed */
	}
	cdoce = s->chkdoc[url_id];
	if (cdoce->nobj > 0) { /* we still have parse the html part */
		if (o_id >= cdoce->nobj) { /* impossible */
			fprintf(stderr, "GetChkDocAndCotab: url_id >= nobj\n");
			return NULL; 
		}
		return cdoce;
	}
/* alloc co_tab pointer. because nobj is NULL , play with h_nobj */
	if (cdoce->h_nobj > o_id )	/* still alloc, do nothing */
		return cdoce;
	if (cdoce->h_nobj == 0) { /* first alloc, set all to NULL */
		cdoce->co_tab = (ChunkedObjEntry **) calloc( o_id+1,
			sizeof(ChunkedObjEntry *));
		cdoce->h_nobj = o_id+1;
		return cdoce;
	}
/* growing, realloc */
	f = cdoce->h_nobj;
	t = o_id;
	cdoce->co_tab = (ChunkedObjEntry **) realloc(cdoce->co_tab,
			(o_id + 1) * sizeof(ChunkedObjEntry *));
	for ( i = f; i <= t; i++) {
		cdoce->co_tab[i] = NULL;
	}
	cdoce->h_nobj = o_id+1;
	return cdoce;
}
/* o_status:    
        - complete : this packet full fill the embedded object
        - incomplete: there is missing packet
        - still_here: we have still see this data
*/  
int PutPacketInChkObj(Source *s, ChunkedObjEntry ** co_tab, int url_id,
	int o_id, int offset, char * d, int d_len, int is_end)
{
	ChunkedObjEntry * coe;
	MissRange * plmr;
	MimeHeaderStruct mhs;
	int size, status;
	PacketDataChunk *plpdc;
	PacketDataChunk *last_lpdc;

	if (! co_tab[o_id] ) {
		coe = co_tab[o_id] = (ChunkedObjEntry *) calloc(1,
					sizeof(ChunkedObjEntry ));
		coe->url_id = url_id;
		coe->o_id = o_id;
		coe->size_data = 0;
		coe->h_size = 0;
		coe->data = NULL;
		coe->lpdc =NULL;
		coe->end = NULL;
		coe->beg = NULL;
		coe->lmr = (MissRange*) malloc(sizeof(MissRange));
		coe->lmr->from = 0;
		coe->lmr->to = 0xffffffff;
		coe->lmr->next = NULL;
		coe->lmr->prev = NULL;
	}
	coe = co_tab[o_id];

	if (coe->data) { /* the size of object is know, because mime
				or is_end had been seen */
		if ( is_end && coe->end ) {
			return STILL_HERE;
		}
		status = UpdChkObj(coe, d, offset, d_len);
		return status;
	}
/* the size of object is not know, because no mime or no is_end */
/* 3 cas se presente                   
        offset = 0, c'est le header mime
        offset != 0 et !is_end packet du milieu
        offset != 0 et is_end, fin qui permet de determine la taille de l'ojet
                ( a calculer avec offset et taille du packet)
*/ 

	if (offset == 0) { /* the mime header, compute size */
		char * tmp;

		if (is_end) { /* only mime... */
			coe->data = strdup(d);
			coe->size_data = d_len;
			return COMPLETE;
		}
		tmp = strdup(d);
		ParseMimeHeader(tmp, &mhs);
		coe->data = malloc(mhs.content_length);
		coe->size_data = mhs.content_length + d_len;

		status = MergeChkObjLpdc(coe->size_data, coe, d, offset,
			d_len);
		return status;
	}
	if (is_end) {	/* last packet, but no mime header, compute size */
		size = offset + d_len;
		status = MergeChkObjLpdc(size, coe, d, offset, d_len);
		return status;
	}
	if (coe->lpdc == NULL) {
		coe->lpdc = (PacketDataChunk*)calloc(1,sizeof(PacketDataChunk));
		coe->lpdc->offset = offset;
		coe->lpdc->d_len = d_len;
		coe->lpdc->d = (char*)malloc(d_len);
		memcpy(coe->lpdc->d, d, d_len);
		return INCOMPLETE;
	}
	plpdc = coe->lpdc;
	while (plpdc) {
		if (plpdc->offset == offset)
			return STILL_HERE;
		last_lpdc = plpdc;
		plpdc = plpdc->next;
	}
	last_lpdc->next = (PacketDataChunk*)calloc(1,sizeof(PacketDataChunk));
	plpdc = last_lpdc->next;
	plpdc->offset = offset;
	plpdc->d_len = d_len;
	plpdc->d = (char*)malloc(d_len);
	memcpy(plpdc->d, d, d_len);
	return INCOMPLETE;
}

/* lpdc must be merge in data , because the size of object is know */
int MergeChkObjLpdc( int size, ChunkedObjEntry * coe, char * d , int offset, int d_len)
{
	PacketDataChunk *p, *plpdc;
	int status = INCOMPLETE;
	int f_status = INCOMPLETE;

	coe->size_data = size;
	coe->data = (char*) malloc(size + 1);
	coe->lmr->to = size - 1 ;
	status = UpdChkObj(coe, d, offset, d_len);
	plpdc = coe->lpdc;
	while (plpdc) {
		status = UpdChkObj(coe, plpdc->d, plpdc->offset,
			plpdc->d_len);
		if (status == COMPLETE)
			f_status = COMPLETE;
		p = plpdc;
		plpdc = plpdc->next;
		free(p->d);
		free(p);
	}
	coe->lpdc = NULL;
	return f_status;
}
int UpdChkObj(ChunkedObjEntry * coe, char *d, unsigned int offset, unsigned int d_len)
{
	MissRange *plmr,*fmr;
	MissRange cmr;

	fprintf(stderr,"UpdChkObj: url_id = %d, o_id = %d, offset = %d, d_len = %d, size_data = %d, h_size = %d\n",
 coe->url_id, coe->o_id, offset, d_len, coe->size_data, coe->h_size);
	if(offset + d_len > coe->size_data) { /* bug??? */
		fprintf(stderr, "Complexe BUG in UpdChkObj offset+d_len > coe->size_data\007\n");
		return INCOMPLETE;
	}
	if (offset == 0 ) { /* update h_size */
		if (coe->h_size) { /* maybe a bug */
			fprintf(stderr, "Complexe BUG in UpdChkObj coe->h_size!=0 \007\n");
			return INCOMPLETE;
		}
		coe->h_size = d_len;
	}
	plmr = coe->lmr;
	if (!plmr)
		return COMPLETE;
	while( plmr ) {
		if ( !( offset >= plmr->from && offset <= plmr->to) ) {
			plmr = plmr->next;
			continue;
		}
/* update data plmr coe->beg coe->end lmr */
		memcpy(&coe->data[offset], d, d_len);
/* now fork or merge MissRange */
		cmr = *plmr;
		if ( cmr.from == offset) {
			if (offset + d_len -1 < cmr.to) { /* adjust from*/
				plmr->from = offset + d_len;
				return INCOMPLETE;
			} 
			/* remove MissRange */
			free(plmr);
			if (cmr.next == NULL && cmr.prev == NULL){
				/* no range missing */
				coe->lmr = NULL;
				return COMPLETE;
			}
			if (cmr.next == NULL) {
				/* remove at end */
				cmr.prev->next = NULL;
				return INCOMPLETE;
			}				
			if (cmr.prev == NULL ) {
				/* remove at begin */
				coe->lmr = cmr.next;
				coe->lmr->prev = NULL;
				return INCOMPLETE;
			}
			/* remove in middle */
			cmr.prev->next = cmr.next;
			cmr.next->prev = cmr.prev;
			return INCOMPLETE;
		}
/* cas ou cmr.from < offset. Il ne peut etre > offset */
		if (offset + d_len -1 >= cmr.to ) { /* adjust to */
			plmr->to = offset -1;
			return INCOMPLETE;
		}
		/* fork this MissRange */
		fmr = (MissRange *) malloc( sizeof (MissRange ));
		fmr->next = cmr.next;
		fmr->prev = plmr;
		fmr->from = offset + d_len;
		fmr->to = plmr->to;
		plmr->next = fmr;
		plmr->to = offset -1;
		return INCOMPLETE;
	}
	return STILL_HERE;
}

int CountEO( mo_window * win, struct mark_up *mptr )
{
	int neo = 0;
	ImageInfo * picd;

	while( mptr) {
		switch (mptr->type){
		case M_IMAGE:
			if (mptr->is_end)
				break;
			picd = (ImageInfo *) malloc(sizeof(ImageInfo ));
			MMPreParseImageTag(win, picd, mptr);
/* on return : two case on fetched:
 * - fetched = True => this is an internal , width height and image are ok
 * - fetched = False => remaing field is not uptodate and need to be updated
 *       MMPreParseImageTag returns a delayimage
 */
			mptr->s_picd = picd; /* in all case display something*/
			if ( picd->fetched) { /* internal image found */
				break;
			}
			neo++;
			break;
		}
		mptr = mptr->next;
	}
	return neo;
}

void ChkObjToDocObj(Source *s, unsigned int url_id, unsigned int o_id)
{
	DocEntry *doce;
	ChunkedDocEntry *cdoce;
	ObjEntry * oe;
	ChunkedObjEntry * coe;
	int ne_obj;	/* number of embedded object */

	doce = s->doc[url_id];
	cdoce = s->chkdoc[url_id];

	if (o_id == 0) { /* parse and compute number of object */
		char * htext;
		struct mark_up *mlist;

		coe = cdoce->co_tab[o_id];
		htext = coe->data + coe->h_size;
		doce->mlist = HTMLParse(htext);
		ne_obj = CountEO(s->win, doce->mlist);
		doce->o_tab = (ObjEntry**)calloc(ne_obj + 1, sizeof(ObjEntry*));
		doce->nobj = ne_obj + 1 ;
		if (cdoce->h_nobj > doce->nobj) {
			fprintf(stderr, "\007ChkObjToDocObj: cdoce->h_nobj>doce->nobj\n");
			return;	/* #### wrong #### */
		}
		if (cdoce->h_nobj < doce->nobj) { /* realloc co_tab */
			int i,f,t;

			f = cdoce->h_nobj;
			t = doce->nobj;
			cdoce->co_tab = (ChunkedObjEntry **)realloc(cdoce->co_tab,
                        	t * sizeof(ChunkedObjEntry *));
			for ( i = f; i < t; i++) {
				cdoce->co_tab[i] = NULL;
			}
		}
		cdoce->nobj = doce->nobj;
		cdoce->h_nobj = doce->nobj;
		doce->n_miss_o = doce->nobj;
	}
	if( doce->nobj == 0 )
		return;
/* fill object in doc */
	oe = doce->o_tab[o_id] = (ObjEntry*) calloc(1,sizeof(ObjEntry));
	coe = cdoce->co_tab[o_id];

	oe->h_part = coe->data;
	oe->d_part = coe->data + coe->h_size;
	oe->h_len = coe->h_size;
	oe->d_part--;
	*oe->d_part = '\0';
	oe->d_part++;

	oe->d_len =coe->size_data - oe->h_len ;
	oe->o_num = o_id;

/* ###	oe->ts = 1; */
	coe->data = NULL;
	memset(coe, 0, sizeof(coe));
	free(coe);
	cdoce->co_tab[o_id] = NULL;
	doce->n_miss_o--;
	if ( o_id == 0 && doce->n_miss_o ) {
		/* because of html late packet, some object is maybe complete */
		unsigned int i;

		for(i = 1; i < cdoce->nobj; i++) {
			if (!cdoce->co_tab[i])
				continue;
			if (cdoce->co_tab[i]->lmr == 0)
				continue;
			ChkObjToDocObj(s, url_id, i);
		}
	}
}

/* At this point , s->doc[url_id]->o_tab[o_id] is NULL */
/* got a structure in chkdoc. */       
/* s->chkdoc[url_id]->co_tab[o_id] exist. but maybe NULL */
/* put data (if not still here) in correct PacketDataChunk (found in co_tab) */
/*   if we update a PacketDataChunk, test if obj complete */
/*      if obj complete ,test if it is HTML text AND if doc is compelete */
/* if data stil here: simply return */ 
/*   if obj not complete : simply return */  
/*      if not HMTL and doc not complete: simply return */
/* display the doc (maybe incomplete ) */
                                       
                                       
/* ### cas generale : mise a jour de la doc temporaire */
/* npdc est le numero de PacketDataChunk */
/* 
/*        if (!status) {  /* there is some missing chunck */
/*                if (!s->chkdoc[url_id]->co_tab[o_id]->lpdc[0]) {
/*                        /* reclamer le header mime rapidement */
/*                }       
/*                if (try_retrieve) { /* retrieve only if packet lost */
/*                        ScheduleAndRetrieveChunk(s->chkdoc[url_id]->co_tab[o_id]);                       
/*                }       
/*                return; 
/*        }       
/*##############
*/
