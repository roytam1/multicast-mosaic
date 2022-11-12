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
	fprintf(stderr, "UcRtcpWriteSdesCb ### FIXME\n");
}

int McRcvrSrcAllocState(Source * s, int state_id)
{
	int i;
	ChunkedBufStruct *cbs;

		/* initialize s->states_tab_size somewhere */
	if ( s->states_tab_size == 0 ) { /* alloc initiale */
		s->states_tab_size = state_id+1;
		s->states = (McStateStruct *)malloc(s->states_tab_size *
						sizeof(McStateStruct));
		if (!s->states) {     
                        return 0;     
                }
		for(i = 0; i<s->states_tab_size; i++) {
			s->states[i].sdata = NULL;
			s->states[i].sdata_len = 0;
			s->states[i].statid = -1;
			s->states[i].start_moid = -1;
			s->states[i].n_do = 0;
			s->states[i].dot = NULL;
			s->states[i].buffer = NULL;
			s->states[i].buffer_status = EMPTY_BUFFER;
			s->states[i].chkbuf = (ChunkedBufStruct *)calloc(1,
				sizeof(ChunkedBufStruct ));
			if(!s->states[i].chkbuf)
				return 0;
			cbs = s->states[i].chkbuf;
			cbs->size_data = 0;
			cbs->data = NULL;      
			cbs->lpdc =NULL;       
			cbs->end = NULL;       
			cbs->beg = NULL;       
			cbs->lmr = (MissRange*) malloc(sizeof(MissRange));
			cbs->lmr->from = 0;    
			cbs->lmr->to = 0xffffffff;
			cbs->lmr->next = NULL; 
			cbs->lmr->prev = NULL; 
			cbs->mhs = NULL; 
		}
	}
	if( state_id+1 >= s->states_tab_size ) { /* grow */
		int osize = s->states_tab_size;
 
		s->states_tab_size = state_id+1 + s->states_tab_size / 2;
		s->states = (McStateStruct *)realloc( s->states,
			s->states_tab_size * sizeof(McStateStruct));
		if (!s->states) {
			return 0;
		}
		for(i = osize; i<s->states_tab_size; i++) {
			s->states[i].sdata = NULL;
			s->states[i].sdata_len = 0;
			s->states[i].statid = -1;
			s->states[i].start_moid = -1;
			s->states[i].n_do = 0;
			s->states[i].dot = NULL;
			s->states[i].buffer = NULL;
			s->states[i].buffer_status = EMPTY_BUFFER;
			s->states[i].chkbuf = (ChunkedBufStruct *)calloc(1,
				sizeof(ChunkedBufStruct ));
			if(!s->states[i].chkbuf)
				return 0;
			cbs = s->states[i].chkbuf;
			cbs->size_data = 0;
			cbs->data = NULL;      
			cbs->lpdc =NULL;       
			cbs->end = NULL;       
			cbs->beg = NULL;       
			cbs->lmr = (MissRange*) malloc(sizeof(MissRange));
			cbs->lmr->from = 0;    
			cbs->lmr->to = 0xffffffff;
			cbs->lmr->next = NULL; 
			cbs->lmr->prev = NULL; 
			cbs->mhs = NULL; 
		}
	}
	return 1;
}

int McRcvrSrcAllocObject(Source * s, int moid)
{
	int i;
	ChunkedBufStruct *cbs;

		/* initialize s->objects_tab_size somewhere */
	if ( s->objects_tab_size == 0 ){
		s->objects_tab_size = moid+1;
		s->objects = (McObjectStruct *)malloc(s->objects_tab_size *
						sizeof(McObjectStruct) );
		if (!s->objects) {
			return 0;
		}
		for(i = 0; i<s->objects_tab_size; i++) {
			s->objects[i].exist = 0;
			s->objects[i].aurl = NULL;
			s->objects[i].fname = NULL;
			s->objects[i].mhs = NULL;
			s->objects[i].moid = -1; 
			s->objects[i].last_modify = 0;
			s->objects[i].statid = -1;
			s->objects[i].stateless = False;
			s->objects[i].moid = -1;
			s->objects[i].n_do = 0;
			s->objects[i].dot = NULL;
			s->objects[i].buffer = NULL;
			s->objects[i].buffer_status = EMPTY_BUFFER;
			s->objects[i].chkbuf = (ChunkedBufStruct *)calloc(1,
				sizeof(ChunkedBufStruct ));
			if(!s->objects[i].chkbuf)
				return 0;
			cbs = s->objects[i].chkbuf;
			cbs->size_data = 0;
			cbs->data = NULL;      
			cbs->lpdc =NULL;       
			cbs->end = NULL;       
			cbs->beg = NULL;       
			cbs->lmr = (MissRange*) malloc(sizeof(MissRange));
			cbs->lmr->from = 0;    
			cbs->lmr->to = 0xffffffff;
			cbs->lmr->next = NULL; 
			cbs->lmr->prev = NULL; 
			cbs->mhs = NULL; 
		}

	}
	if( moid+1 >= s->objects_tab_size ) { /* grow */
		int osize = s->objects_tab_size;
 
		s->objects_tab_size = moid+1 + s->objects_tab_size / 2;
		s->objects = (McObjectStruct *)realloc( s->objects,
			s->objects_tab_size * sizeof(McObjectStruct) );
		if (!s->objects) {
			return 0;
		}
		for(i = osize; i<s->objects_tab_size; i++) {
			s->objects[i].exist = 0;
			s->objects[i].aurl = NULL;
			s->objects[i].fname = NULL;
			s->objects[i].mhs = NULL;
			s->objects[i].moid = -1; 
			s->objects[i].last_modify = 0;
			s->objects[i].statid = -1;
			s->objects[i].stateless = False;
			s->objects[i].moid = -1;
			s->objects[i].n_do = 0;
			s->objects[i].dot = NULL;
			s->objects[i].buffer = NULL;
			s->objects[i].buffer_status = EMPTY_BUFFER;
			s->objects[i].chkbuf = (ChunkedBufStruct *)calloc(1,
				sizeof(ChunkedBufStruct ));
			if(!s->objects[i].chkbuf)
				return 0;
			cbs = s->objects[i].chkbuf;
			cbs->size_data = 0;
			cbs->data = NULL;      
			cbs->lpdc =NULL;       
			cbs->end = NULL;       
			cbs->beg = NULL;       
			cbs->lmr = (MissRange*) malloc(sizeof(MissRange));
			cbs->lmr->from = 0;    
			cbs->lmr->to = 0xffffffff;
			cbs->lmr->next = NULL; 
			cbs->lmr->prev = NULL; 
			cbs->mhs = NULL; 
		}
	}
	return 1;
}

/* parse a buffer for state . Use the mime parser. */
/* update the field of mhs concerning a State */
/* 	int statid;             /* the stateid */
/*        int start_moid;         /* begin with this object */
/*        int n_do;               /* number of depend object */
/*        DependObjectTab dot;    /* liste of dependant object */
/*        struct timeval ts;
*/
/* j'ai un petit poil dans la main. */
/* State-ID: sid
   Start-ObjectID: startid
   Depend-Object: n, moid1 ... moidn
*/
static int McRcvrParseStatesData(char *buf_in, int len_buf_in,
	MimeHeaderStruct *mhs_buf )
{
	MimeHeaderStruct tmp_mhs;
	char *lf_ptr, *beg_m, server_status[20], *lflf_ptr;

	beg_m = buf_in;
	lflf_ptr = strstr(beg_m, "\012\012");
	if (!lflf_ptr) {
		abort();	/* let me know */
	}
	lflf_ptr[1] = '\0';	/* set the last LF to '\0' */
	ParseMimeHeader(beg_m, &tmp_mhs);	/* parse mime */
	lflf_ptr[1] = '\012';
        *mhs_buf = tmp_mhs;

	return PARSED_BUFFER;
}
 
int McRcvrSrcCheckBufferStateWithData(Source *s, int is_end, int state_id,
	int offset, char * d, int d_len)
{
	int status;
	MimeHeaderStruct mhs_buf;

	if ( s->states[state_id].buffer_status == PARSED_BUFFER)
		return PARSED_BUFFER;
	if ( s->states[state_id].buffer_status == COMPLETE_BUFFER) {
		abort(); 	/* parse it */
		return PARSED_BUFFER;
	}

#ifdef DEBUG_MULTICAST
	fprintf(stderr,"McRcvrSrcCheckBufferStateWithData: receive -%s-\n", d);
#endif
	status = PutPacketInChkBuf(s->states[state_id].chkbuf, is_end, offset,
		d, d_len);
        if (status == COMPLETE_BUFFER) {
		int len;

                len = ChkBufToBuf(s->states[state_id].chkbuf, &s->states[state_id].buffer);
		s->states[state_id].chkbuf = NULL;		/* sanity */
		s->states[state_id].len_buffer = len;
                s->states[state_id].buffer_status = COMPLETE_BUFFER;
                status = McRcvrParseStatesData(s->states[state_id].buffer,
                        s->states[state_id].len_buffer, &mhs_buf);
/* mhs contient le resultat du parse pour cet etat */
                s->states[state_id].buffer_status = status;
		s->states[state_id].n_do = mhs_buf.n_do;
		s->states[state_id].dot = mhs_buf.dot;
		s->states[state_id].statid = mhs_buf.state_id; /* CHECKME ###*/
		s->states[state_id].start_moid = mhs_buf.start_object_id;

		if (status == PARSED_BUFFER) 
                        return PARSED_BUFFER;
		abort();		/* let me know */
        }          

	if ( s->states[state_id].buffer_status == CHUNKED_BUFFER) {
			/* comme il manque des datas */
                        /* demande peut etre un repair pour cet object ? */
		return CHUNKED_BUFFER;
	}
	if ( s->states[state_id].buffer_status == EMPTY_BUFFER ) {
		abort();	/* update the buffer with 'd' */
				/* status = ??? */
				/* envoyer une demande de repair ? */
				/* oui mais sous quelle condition? */
		return EMPTY_BUFFER ;	/* return apropriate status */
	}
	abort();	/* let me know */
	return status;
}

/* just check. no data is available */
/* assume transition between PARSED_BUFFER and PARSED_ALL_DEPEND_BUFFER */
int McRcvrSrcCheckBufferObject(Source *s, int moid)
{
	int status;
	int i;

	status = s->objects[moid].buffer_status;
	if (status == PARSED_ALL_DEPEND_BUFFER) {
		return PARSED_ALL_DEPEND_BUFFER;
	}
	if (status != PARSED_BUFFER) {
		return status;
	}
/* check if all other is good */
	for(i = 0 ; i < s->objects[moid].n_do; i++) {
		status = McRcvrSrcAllocObject(s, s->objects[moid].dot[i]);
                if( !status) {
                        fprintf(stderr,"Out of mem\n");
                        abort();
                }
		status = McRcvrSrcCheckBufferObject(s, s->objects[moid].dot[i]);
		if (status != PARSED_ALL_DEPEND_BUFFER)
			return PARSED_BUFFER;
	}
	s->objects[moid].buffer_status = PARSED_ALL_DEPEND_BUFFER;
	return PARSED_ALL_DEPEND_BUFFER;
}

static void ReOrderDepend(Source *s, int moid, DependObjectTab dot, int ndo)
{
	int i;

	for (i = 0; i < ndo; i++) {
		s->objects[moid].dot[i] = dot[i];
	}
}

void McUpdateDataSourceWithState(Source *s, int is_end, u_int16_t seqn,
	u_int32_t rtp_ts, u_int32_t ssrc,
	u_int32_t state_id, u_int32_t offset, char *d,
	u_int32_t d_len)
{
	int try_retrieve = 0;
	int status;
	McStateStruct st;
	int i;

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

/* A state is a string like:
 *	State-ID: stateid		(mandatory)
 *	Start-ObjectID: moid		(mandatory)
 *	Depend-Object:	n, moid1 moid2 ... moidn	(optionnal)
 *	\n\n
 */
	if( !McRcvrSrcAllocState(s, state_id)) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}
	
/* update and check: si toutes les donnees sont la pour le state_id */
/* on fait la demande de repair dans cette routine */
/* si les donnees sont completes on fait le parse dans cette routine */
	status = McRcvrSrcCheckBufferStateWithData(s, is_end, state_id, offset,
		d, d_len);
	if ( status != PARSED_BUFFER)
		return;	/* il manque des octets , une demande de repair */
			/* pour cet etat est en instance, pending */
			/* la description de l'etat n'est pas encore valide */

	st = s->states[state_id]; /* l'analyse est deja faite */ 

/* check and send possible repair for this object */
/* this may be long to repair because object may depend of object... */
	status = McRcvrSrcAllocObject(s, st.start_moid);
	if( !status) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}
	status = McRcvrSrcCheckBufferObject(s, st.start_moid);
	if (status != PARSED_ALL_DEPEND_BUFFER)
		return;
	if (st.n_do != 0) {		/* that's a Frameset */
					/* reorder depend object */
		ReOrderDepend(s, st.start_moid, st.dot, st.n_do);
	}
	for( i = 0; i < st.n_do; i++) {
		status = McRcvrSrcAllocObject(s, st.dot[i]);
		if( !status) {
			fprintf(stderr,"Out of mem\n");
			abort();
		}
		status = McRcvrSrcCheckBufferObject(s, st.dot[i]);
		if (status != PARSED_ALL_DEPEND_BUFFER)
			return;
	}
/* COMPLETE and all depend object of object are here, play with them*/
/* at this point state is complete. Display it now */
	McDoWindowText(s, state_id);	/*Display full doc */
}

/* parse a buffer: decompose in a:
	- http message (status)
	- http mime
	- body	(html data)
*/
static int McRcvrParseObjectData(char *buf_in, int len_buf_in, int *code_ret,
	char **aurl_ret, MimeHeaderStruct *mhs_buf, char **body_ret,
	int * body_len)
{
	MimeHeaderStruct tmp_mhs;
	char aurl[10000];
	char *lf_ptr, *beg_m, server_status[20], *lflf_ptr;
	int nfields;

	lf_ptr = strchr(buf_in, '\012'); /* GET http message */
	if (lf_ptr == NULL) {
		abort();	/* Bug letme know */
	}
	beg_m = lf_ptr+1;
/* http message is :
	- "GET %s HTTP/1.0\n"
	- "ERROR %d %s HTTP/1.0\n\n\n"
*/
	if (!strncmp(buf_in, "ERROR", 5) ) {
		ParseMimeHeader("", &tmp_mhs);  /* get a default mime header*/
		nfields = sscanf(buf_in, "%*s %d %s %8s", code_ret, aurl, server_status);
		if (nfields != 3) {
			abort(); /* let me know */
		}
		*aurl_ret = strdup(aurl);
		/* *code_ret = 404; */
		*body_ret = NULL;
		*body_len = 0;
		*mhs_buf  = tmp_mhs;
		return PARSED_BUFFER;
	}
	if (strncmp(buf_in, "GET", 3)) {
		abort(); 	/* bug letme know */
	}
/* GET message */
	nfields = sscanf(buf_in, "%*s %s %8s", aurl, server_status);
	if (nfields != 2) {
		abort(); /* let me know */
	}
/*char *buf_in, int len_buf_in */
	*aurl_ret = strdup(aurl);
	*code_ret = 200;

	lflf_ptr = strstr(beg_m, "\012\012");
	if (!lflf_ptr) {
		abort();	/* let me know */
	}
	lflf_ptr[1] = '\0';	/* set the last LF to '\0' */
	ParseMimeHeader(beg_m, &tmp_mhs);	/* parse mime */
	lflf_ptr[1] = '\012';
        *mhs_buf = tmp_mhs;
	*body_ret = lflf_ptr + 2;
	*body_len = len_buf_in - (lflf_ptr+2 - buf_in);

	return PARSED_BUFFER;
}

static int McRcvrSrcCheckBufferObjectWithData(Source *s, int is_end, int moid,
	int offset, char *d, int d_len)
{
	int status = EMPTY_BUFFER;
	int len;
	int code;
	char *aurl;
	MimeHeaderStruct mhs, mhs_ret;
	char *body;
	int body_len;
	char *fname_ret;

	if (s->objects[moid].buffer_status == COMPLETE_BUFFER) {
		abort();
		return COMPLETE_BUFFER;
	}
	if (s->objects[moid].buffer_status == PARSED_BUFFER)
		return PARSED_BUFFER;
	if (s->objects[moid].buffer_status == PARSED_ALL_DEPEND_BUFFER)
		return PARSED_ALL_DEPEND_BUFFER;
	status = PutPacketInChkBuf(s->objects[moid].chkbuf, is_end, offset,
		d, d_len);
	if (status == COMPLETE_BUFFER) {
		len = ChkBufToBuf(s->objects[moid].chkbuf, &s->objects[moid].buffer);
		s->objects[moid].chkbuf = NULL;		/* sanity */
		s->objects[moid].len_buffer = len;
		s->objects[moid].buffer_status = COMPLETE_BUFFER;
		status = McRcvrParseObjectData(s->objects[moid].buffer,
			s->objects[moid].len_buffer,
			&code, &aurl, &mhs, &body, &body_len);
		s->objects[moid].n_do = mhs.n_do;
		s->objects[moid].dot = mhs.dot;
		switch (code) {
		case 200:
			mhs.status_code = 200;
			break;
		case 400:		/* "Bad Request" */
		case 401:		/* "Unauthorized" */
		case 402:		/* "Payment Required" */
		case 403:		/* "Forbiden" translate to not Found */
		case 404:
			mhs.status_code =404;
			body = "404 Not Found";
			body_len = strlen(body)+1;
			break;
			abort(); /* do something with code aurl mhs & body */
		/* code is 200 when GET, or the error number(404 notfound */
		default:
			abort();	/* let me know */
		}
		/* transition de COMPLETE_BUFFER a PARSED_BUFFER */
		/* cache it . mettre tout ca dans un cache... */
		McSourceCachePutDataInCache(s, body, body_len,
			aurl, &mhs, moid, &fname_ret, &mhs_ret);
		s->objects[moid].buffer_status = status;
		free(s->objects[moid].buffer);
		s->objects[moid].buffer = NULL;	/* clean up */
		if (status == PARSED_BUFFER)
			return PARSED_BUFFER;
		abort();	/* let me know */
	}
	if (status == CHUNKED_BUFFER){
			/* schedule somethings. Verifier plus tard */
			/* si il manque des objets et les reclamer */
			/* eventuellement */
			/* il manque des datas */
			/* demande peut etre un repair pour cet object ? */
			/* si je ne prevois pas un repair ici, je ne le ferai */
			/* jamais */
			/* Finalement c'est pas vrai. je le fais quand je recois*/
			/* un STATR. Faudrait quand meme verifier la sequence*/
			/* et faire un repair en cas de rupture de sequence */
		return CHUNKED_BUFFER;
	}
	if (status == EMPTY_BUFFER)
		abort();	/* let me know */
	abort();		/* let me know */
	return status;
}

static void McRcvSrcMakeStateFromObject( Source *s, McObjectStruct *obs, int state_id)
{
	abort();
}

/* because some asynchronous event we need to check the last state at later time*/
/* checking is doing by looking for a parsed state and looking if all */
/* depend object for this state is good */
/* if this is correct, we would display this state of the source */
/* if this state is the current state, and history (multicast navigation) is not active */
typedef struct _SrcCheckStateStruct {
	Source * source;
	int sid;
} SrcCheckStateStruct;

static XtIntervalId mc_src_check_state_timer_id;
static int 	mc_src_check_state_time = 1000;	/* en millisec */
static void McSrcCheckStateCb(XtPointer clid, XtIntervalId * id)
{
	SrcCheckStateStruct *sc = (SrcCheckStateStruct *)clid;
	Source *s;
	int sid, status;
	int i;

	s= sc->source;
	sid = sc->sid;
	free(clid);	/* free memory */

/*	if (s->current_sid_displayed == sid)
/*		return;
*/
	if (s->last_valid_state_id >= sid )
		return;
/* check if state is here */
	if ( s->states[sid].buffer_status != PARSED_BUFFER ) {/* resched if not*/
		McRcvSrcScheduleCheckState(s, sid);
		return;
	}
/* check for all depend object, include the start_moid */
	status = McRcvrSrcAllocObject(s, s->states[sid].start_moid);
        if( !status) {
                fprintf(stderr,"Out of mem\n");
                abort();
        }
	status = McRcvrSrcCheckBufferObject(s, s->states[sid].start_moid);
	if (status != PARSED_ALL_DEPEND_BUFFER) {
		McRcvSrcScheduleCheckState(s, sid);
		return;
	}
	if (s->states[sid].n_do != 0) {		/* that's a Frameset */
					/* reorder depend object */
		ReOrderDepend(s, s->states[sid].start_moid, s->states[sid].dot, s->states[sid].n_do);
	}
	for(i=0; i< s->states[sid].n_do; i++) {
		status = McRcvrSrcAllocObject(s, s->states[sid].dot[i]);  
                if( !status) {
                        fprintf(stderr,"Out of mem\n");  
                        abort();
                }
		status = McRcvrSrcCheckBufferObject(s, s->states[sid].dot[i]);
                if (status != PARSED_ALL_DEPEND_BUFFER) {
			McRcvSrcScheduleCheckState(s, sid);
                        return;
		}
	}
/* all is good , do you need to display ?*/
	/* if some condition to display == True then display */
	/* condition must reflect what want the user (navigation or not) */
	/* and the hight sid display, reflecting the current sid send */
	/* if hight sid(send) change during the wait do nothing */
	McDoWindowText(s, sid);    /*Display full doc */
}
void McRcvSrcScheduleCheckState( Source *s, int state_id)
{
	SrcCheckStateStruct *scss;

	scss = (SrcCheckStateStruct *) malloc(sizeof(SrcCheckStateStruct));
	scss->source = s;
	scss->sid = state_id;

	mc_src_check_state_timer_id = XtAppAddTimeOut(mMosaicAppContext,
		mc_src_check_state_time, McSrcCheckStateCb, scss);
}

/*##########################*/
/* for each source we ave a space of object */
/* update this object */

void McUpdateDataSourceWithObject(Source *s, int is_end, u_int16_t seqn,
	u_int32_t rtp_ts, u_int32_t ssrc,
	u_int32_t moid, u_int32_t offset, char *d,
	u_int32_t d_len)
{
	int try_retrieve = 0;
	int status;
	int ostatus;
	McObjectStruct ob;

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
/* status: All depend object are here include me*/
/*if (try_retrieve ) { /* arme a timer to retrieve missing packet */
/*fprintf(stderr, "Obj is INCOMPLETE and try_retrieve = 1\n");
/* Arm a timer (a callback to retrieve packet */
/*SendRepairFromSeqn(s, url_id, o_id, offset, d_len, is_end, seqn);
/*s->last_valid_seq = seqn;
/*return; /*} /*return;
*/
	status = McRcvrSrcAllocObject(s, moid);
	if( !status) {
		fprintf(stderr,"Out of mem\n");
		abort();
	}

	ostatus = s->objects[moid].buffer_status;
	if (ostatus == PARSED_ALL_DEPEND_BUFFER )
		return;		/* schedule have been done */
	status = McRcvrSrcCheckBufferObjectWithData(s, is_end, moid, offset,
		d, d_len);
	s->objects[moid].buffer_status = status;
        if ( status != PARSED_BUFFER && status != PARSED_ALL_DEPEND_BUFFER)
                return; /* il manque des octets , une demande de repair */
                        /* pour cet oject est en instance, pending */
                        /* la description de l'oject n'est pas encore valide */
         
        ob = s->objects[moid]; /* l'analyse est deja faite */

	if (s->objects[moid].stateless == True && ostatus != PARSED_BUFFER) {
		McRcvSrcMakeStateFromObject(s, &ob, ob.statid);
	}
	status = McRcvrSrcCheckBufferObject(s, moid);
	if (status == PARSED_BUFFER  && ostatus != PARSED_BUFFER) {
		if (s->objects[moid].stateless == True) {
			McRcvSrcScheduleCheckState(s, ob.statid);
		}
		return;		/* missing depend object */
	}

	s->objects[moid].buffer_status = PARSED_ALL_DEPEND_BUFFER;

/* COMPLETE at this point object is complete including depend. */
	if (s->objects[moid].stateless == True) {
		McDoWindowText(s, ob.statid);    /*Display full doc */
	}
}

/*##########################*/
 
void McProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from)
{
	RtcpPacket rcs;
	Source *s;
	int ind = 0;
	int lenp = 0;
	u_long target_ssrc;

/* we don't apply mixer's rule: FIXME### */ 
/* this does not solve the contributor source */

	while (len > 0) {
		lenp = DewrapRtcpData(&buf[ind], len, &rcs);
		if (lenp <= 0)
			return;
		ind += lenp;
		len -= lenp;
        	s = mc_rtcp_demux(rcs.ssrc, addr_from, &rcs); 
        	if ( s == NULL) return; 
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
			McQueryRepairFromStatr(s, &rcs);
			break;
/* a receiver wish to repair a packet */
		case RTCP_PT_REPAIR:
			fprintf(stderr, "RTCP_PT_REPAIR\n");
			target_ssrc = ntohl( *((u_long*)rcs.d));
			if ( target_ssrc != mc_local_srcid) /* not for me */
				break;
			if ( !mc_send_win)	/* i am not a sender */
				break;
			/* query is for me. reply to 's' or multicast depending */
			/* of the number of query and net parameter */
			McStoreQueryRepair(s, &rcs);
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
	int ind = 0;
	int lenp = 0;
	u_long target_ssrc;

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
			target_ssrc = ntohl( *((u_long*)rcs.d));
			if ( target_ssrc != mc_local_srcid) /* not for me */
				break;
			if ( !mc_send_win)	/* i am not a sender */
				break;
			/* query is for me. reply to 's' or multicast depending */
			/* of the number of query and net parameter */
			McStoreQueryRepair(s, &rcs);
			break;
		default:
			fprintf(stderr, "BUG UcProcessRtcpData\007\n");
			return;
		}
	}
}
