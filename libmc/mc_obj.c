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
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLparse.h"

#include "mc_mosaic.h"
#include "../src/img.h"
#include "mc_misc.h"
#include "mc_main.h"

void McFillLocalObject(ObjEntry ** obj, int num, char * fname, char *aurl_wa,
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

void McFillLocalErrorObject(ObjEntry ** obj, int j, char *aurl, int status_code)
{
	char * buf=NULL;

	buf = (char*)malloc(strlen(aurl)+ 100);
	sprintf(buf, "GET %s HTTP/1.0\nContent-Length: 0\nStatus-Code: 404\n\n\n", aurl);

	obj[j] = (ObjEntry *) calloc(1, sizeof(ObjEntry ));
	obj[j]->h_part = buf;
	obj[j]->h_len = strlen(buf);
	obj[j]->h_fname = NULL;
	if(obj[j]->h_len > 500)
		fprintf(stderr,"Mc: header is long\n");

        obj[j]->d_len = 0;
        obj[j]->d_part = NULL;
        obj[j]->d_fname = NULL; 	/* just now ### */
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
        McFillLocalObject(mc_local_docs[i].o_tab, 0, fname, aurl_wa, mhs);
        mc_local_docs[i].nobj = 1;              /* nombre d'objet */
        mc_local_docs[i].o_tab[0]->o_num = 0;    /* numero de cet objet */
        gettimeofday(&mc_local_docs[i].o_tab[0]->ts,0) ;
} 

void McCreateErrorEntry(char *aurl, int status_code)
{
        int i, j, n_obj;

        i = mc_local_url_id;           
	j = mc_local_object_id;
	n_obj = mc_local_object_id +1;
/* une entre pour la doc existe deja */

/* #### just le HTML pour l'instant */
        mc_local_docs[i].o_tab = (ObjEntry**)realloc(mc_local_docs[i].o_tab,
				n_obj * sizeof(ObjEntry*));

	McFillLocalErrorObject(mc_local_docs[i].o_tab, j, aurl, status_code);

        mc_local_docs[i].nobj = n_obj;              /* nombre d'objet */
        mc_local_docs[i].o_tab[j]->o_num = j;    /* numero de cet objet */
        gettimeofday(&mc_local_docs[i].o_tab[j]->ts,0) ;
}

void McCreateObjectEntry(char *fname, char* aurl, MimeHeaderStruct *mhs)
{                                      
        int i, j, n_obj;
                                       
        i = mc_local_url_id;           
	j = mc_local_object_id;
	n_obj = mc_local_object_id +1;
/* une entre pour la doc existe deja */

/* #### just le HTML pour l'instant */
        mc_local_docs[i].o_tab = (ObjEntry**)realloc(mc_local_docs[i].o_tab,
				n_obj * sizeof(ObjEntry*));
        McFillLocalObject(mc_local_docs[i].o_tab, j, fname, aurl, mhs);
        mc_local_docs[i].nobj = n_obj;              /* nombre d'objet */
        mc_local_docs[i].o_tab[j]->o_num = j;    /* numero de cet objet */
        gettimeofday(&mc_local_docs[i].o_tab[j]->ts,0) ;
} 

void McObjectToPacket(int url_id, int o_id)
{
        DocEntry doc;
        RtpPacket *ph;          /* one for header */
        RtpPacket **ptab;
        RtpPacket * deb_p, *end_p;
        int s_dchunk; 
        int i, n_d, n_t, s_oh, r_b, offset, d_off;
        int p_d_l;
        int rtp_ts;
        int d_duration;
        double d_dur;
        int next_time = 10;
 
        deb_p = mc_rtp_packets_list; /* pointe au debut de la liste  */
	end_p = mc_rtp_packets_list;
        if (deb_p) { /* il y a une file d'attente */
                /* volontary lost packet, un utilisateur va trop vite au regard
                 * de la bande passante qu'il s'autorise... */
                /* complique car il faut recuprer le seqno */   
/*              next_time = UnSchedule(deb_plist, end_plist); */
/*              deb_plist = ... */
/*              end_plist = ... */
		while (end_p->next){
			end_p = end_p->next;
		}
        }               
 
/* send the header in the first packet as a standalone packet */
        s_dchunk = DATA_CHUNK_SIZE;     /* size of data chunck ~512 */
        doc = mc_local_docs[url_id];
/* size of object include header */
        s_oh = doc.o_tab[o_id]->h_len + doc.o_tab[o_id]->d_len;
        if (doc.o_tab[o_id]->h_len <= 0 || doc.o_tab[o_id]->d_len<0)  /* impossible */
                abort();
/* nombre de packet pour les donnes */ 
        n_d = (doc.o_tab[o_id]->d_len -1) / s_dchunk; /* division entiere */
        if (doc.o_tab[o_id]->d_len > 0)
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
        p_d_l = doc.o_tab[o_id]->h_len;    
        rtp_ts = McRtpTimeStamp(doc.o_tab[o_id]->ts); /* sample time when file come */                                       
/* fill first packets (header) */      
                                       
        d_dur = ((p_d_l + PROTO_OVERHEAD) * 8000)/BAND_WIDTH; /* en millisecond */        d_duration = (int) d_dur;      
        if (d_duration < 2)            
                d_duration = 2;        
        ptab[0]->next = ptab[1];       
        ptab[0]->url_id = url_id;      
        ptab[0]->o_id = o_id;             
        ptab[0]->d = doc.o_tab[o_id]->h_part;
        ptab[0]->d_len = p_d_l;        
        ptab[0]->is_end = n_t == 1 ? 0x80 : 0;           
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
                ptab[i]->o_id = o_id;     
                ptab[i]->d_len = p_d_l;
                ptab[i]->d = &doc.o_tab[o_id]->d_part[d_off];
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
	if (deb_p){
		end_p->next = ptab[0];
	} else {
        	mc_rtp_packets_list = ptab[0]; 
	}
        free(ptab);                    
}

void McDocToPacket( int url_id)
{
	McObjectToPacket(url_id, 0);
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
	int size, status;
	PacketDataChunk *plpdc;
	PacketDataChunk *last_lpdc;

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
		char * pmh;

		tmp = strdup(d);
		coe->mhs = (MimeHeaderStruct *) malloc(sizeof(MimeHeaderStruct));
		pmh = strchr(tmp,'\n');
		ParseMimeHeader(pmh, coe->mhs);
		status = MergeChkObjLpdc(coe->mhs->content_length+d_len, coe, d,
			offset, d_len);
		free(tmp);
		if (is_end) { /* only mime... */
		/*	coe->data = strdup(d); */
		/*	coe->size_data = d_len; */
			if (status != COMPLETE) { /* bug */
				abort();
			}
			return COMPLETE;
		}
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
	coe->data[size] = '\0';
	status = UpdChkObj(coe, d, offset, d_len);
	f_status = status;
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

void ChkDocToDoc(Source *s, int url_id)
{
	char * fname;
	int fd;
	char buf[8192];
	struct mark_up *mlist,*mptr;
	DocEntry *doce;
	ChunkedDocEntry *cdoce;
	ObjEntry * oe;
	int i;
	int rneo ;	/* remaining number of embedded object */
	int o_id;
	ImageInfo * picd;

	doce = s->doc[url_id];
	cdoce = s->chkdoc[url_id];
	rneo = doce->nobj -1;
	for(i = 0; i < cdoce->nobj; i++) {
		if (!cdoce->co_tab[i])
			continue;
		if (cdoce->co_tab[i]->lmr != NULL){
			fprintf(stderr,"BUG in ChkDocToDoc: obj is INCOMPLETE\n");
			continue;
		}
		if ( cdoce->co_tab[i]->data == NULL) {
			continue;
		}
/* No missing range and data exist => obj is COMPLETE */
/* fill object in doc */
		fprintf(stderr, "Y a un pepin...ChkDocToDoc\n");
	}
/* update embedded object in mlist */
	mptr = mlist = doce->mlist;
	o_id = 1;
	while(mptr && ( rneo > 0 ) ){
		switch (mptr->type){
		case M_IMAGE:
			if (mptr->is_end)
				break;
			picd = mptr->s_picd;
			if ( picd->fetched) { /* internal image found */
				break;
			}
			oe = doce->o_tab[o_id];
/* oe->d_part oe->d_len oe->aurl_wa oe->h_part oe->h_len */
			mptr->s_picd->src = oe->aurl_wa;
			fname = NULL;
			if (oe->d_part) {
				fname = tempnam (mMosaicTmpDir,"mMo");
				fd = open(fname, O_WRONLY | O_CREAT | O_TRUNC, 0644);
				write(fd, oe->d_part, oe->d_len);
				close(fd);
			}
/* mettre l'image dans picd a partir de oe */
			MMPreloadImage(s->win, mptr, NULL /*oe->h_part*/, fname);
			if (oe->d_part){
				unlink(fname);
				free(fname);
			}
			rneo--;
			o_id++;
			break;
		}
		mptr = mptr->next;
	}
}

void ChkObjToDocObj(Source *s, unsigned int url_id, unsigned int o_id)
{
	char buf[8192];
	DocEntry *doce;
	ChunkedDocEntry *cdoce;
	ObjEntry * oe;
	ChunkedObjEntry * coe;
	int ne_obj;	/* number of embedded object */
	int status;

	doce = s->doc[url_id];
	cdoce = s->chkdoc[url_id];

	if (o_id == 0) { /* parse and compute number of object */
		char * htext;
		struct mark_up *mlist;

		coe = cdoce->co_tab[o_id];
		htext = coe->data + coe->h_size;
		doce->mlist = HTMLParseRepair(htext);
		ne_obj = CountEO(s->win, doce->mlist);
		doce->o_tab = (ObjEntry**)calloc(ne_obj + 1, sizeof(ObjEntry*));
		doce->nobj = ne_obj + 1 ;
		status = SourceAllocObjs(s, url_id, ne_obj); /* ne_obj is equivalent to max_o_id */
		if ( status == -1) {    /*invalide object number */
			fprintf(stderr, "Invalid object number on recept\n");
			return;
		}
		if ( ! status ) {
			fprintf(stderr,"Out of mem\n");
			abort();
		}
		cdoce->nobj = ne_obj + 1;
		cdoce->h_nobj = ne_obj + 1;

		doce->n_miss_o = doce->nobj;
	}
	if( doce->nobj == 0 )
		return;
/* fill object in doc */
	oe = doce->o_tab[o_id] = (ObjEntry*) calloc(1,sizeof(ObjEntry));
	coe = cdoce->co_tab[o_id];

	sscanf(coe->data,"%*s %s", buf);
	oe->aurl_wa = strdup(buf);
	oe->h_part = (char*)malloc(coe->h_size+1);
	memcpy(oe->h_part,coe->data, coe->h_size);
	oe->h_part[coe->h_size] = '\0';
	oe->h_len = coe->h_size;

	oe->d_part = NULL;
	oe->d_len =coe->size_data - oe->h_len ;
	if (oe->d_len){	
		oe->d_part = (char*)malloc(oe->d_len + 1);
		memcpy(oe->d_part, coe->data + coe->h_size, oe->d_len);
	}
	free(coe->data);
	coe->data = NULL;
	oe->o_num = o_id;
	oe->mhs = coe->mhs;

/* ###	oe->ts = 1; */
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
			if ((cdoce->co_tab[i]->lmr == NULL) && cdoce->co_tab[i]->data != NULL) { /* No missing range and data exist => obj is COMPLETE */
				ChkObjToDocObj(s, url_id, i);
			}
		}
	}
}

int SourceAllocDocs(Source *s, int url_id)
{
	DocEntry * doc;
	ChunkedDocEntry * cdoc;
	int i, from, to;

	if (s->huid >= url_id)		/* still alloc */
		return 1;
	if (s->huid == -1) {		/* initial alloc */
		from = 0;
		to = url_id;
		s->doc = (DocEntry **)calloc(url_id +1, sizeof(DocEntry *));
		if (!s->doc) return 0;
		s->chkdoc = (ChunkedDocEntry **)calloc(url_id +1,
				sizeof(ChunkedDocEntry *));
		if(!s->chkdoc) return 0;
		s->huid = url_id;
	} else { 			/* realloc space. */
		from = s->huid+1;
		to = url_id;
		s->doc = (DocEntry **) realloc( s->doc,
			sizeof(DocEntry *) * (url_id+1));
		if (!s->doc) return 0;
		s->chkdoc = (ChunkedDocEntry **) realloc( s->chkdoc,
			sizeof(ChunkedDocEntry *) * (url_id+1));
		if(!s->chkdoc) return 0;
		s->huid = url_id;
	}

	for(i = from; i <= to; i++) { /* init all doc entry */
		s->doc[i] = (DocEntry *) calloc(1, sizeof(DocEntry ));
		if (!s->doc[i])
			return 0;
		doc = s->doc[i];
		doc->url_id = url_id;
		doc->nobj = 0;
		doc->o_tab = NULL;
		doc->n_miss_o = 0xffffffff;

	/* ### all of this MUST be free when completed */
		s->chkdoc[i] = (ChunkedDocEntry*) calloc(1,
				sizeof(ChunkedDocEntry));
		if (!s->chkdoc[i])
			return 0;
		cdoc = s->chkdoc[i]; 
		cdoc->url_id = url_id;
		cdoc->nobj = 0;
		cdoc->h_nobj = 0;
		cdoc->co_tab = NULL;
	}
	return 1;	
}

int SourceAllocObjs(Source *s, int url_id, int o_id)
{
	ChunkedObjEntry * coe;
	ChunkedDocEntry * cdoce;
	int f, t, i;

	cdoce = s->chkdoc[url_id];
	if (cdoce->nobj > 0) { /* we still have parse the html part */
		if (o_id >= cdoce->nobj) { /* impossible */
			fprintf(stderr, "SourceAllocObjs: url_id >= nobj\n");
			return -1; 
		}
		return 1;
	}
/* alloc co_tab pointer. because nobj is NULL , play with h_nobj */
	if (cdoce->h_nobj > o_id )	/* still alloc, do nothing */
		return 1;
	if (cdoce->h_nobj == 0) { /* first alloc, set all to NULL */
		f = 0;
		t = o_id;
		cdoce->co_tab = (ChunkedObjEntry **) calloc( o_id+1,
			sizeof(ChunkedObjEntry *));
		if (!cdoce->co_tab) return 0;
		cdoce->h_nobj = o_id+1;
	} else { 		/* growing, realloc */
		f = cdoce->h_nobj;
		t = o_id;
		cdoce->co_tab = (ChunkedObjEntry **) realloc(cdoce->co_tab,
			(o_id + 1) * sizeof(ChunkedObjEntry *));
		if (!cdoce->co_tab) return 0;
		cdoce->h_nobj = o_id+1;
	}
	for ( i = f; i <= t; i++) {
		coe = cdoce->co_tab[i] = (ChunkedObjEntry *) calloc(1,
					sizeof(ChunkedObjEntry ));
		if (! coe) return 0;
		coe->url_id = url_id;
		coe->o_id = i;
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
		coe->mhs = NULL;
	}
	return 1;
}
