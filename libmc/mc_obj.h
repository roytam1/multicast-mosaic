#ifdef MULTICAST

#define STILL_HERE 1
#define INCOMPLETE 2
#define COMPLETE 3

typedef struct _ObjEntry {
        char * h_part;          /* header before data */
        int h_len;
        char *h_fname;
        char *d_part;           /* data */
        int d_len;
        char *d_fname;
        struct timeval ts;      /* timstamp (struct timeval) */
        int o_num;              /* object number */
} ObjEntry;

typedef struct _DocEntry {
        int url_id;
        int nobj;  	/* dimension of o_tab */
			/* we know the number of object in parsing o_tab[0]->d_part */
        ObjEntry **o_tab;	/* tab of pointer to ObjEntry */
				/* all pointer is NULL at init. */
	unsigned int n_miss_o;	/* number of missing object */
			/* this decrease when ChkToObj */
	struct mark_up * mlist;
} DocEntry;

typedef struct _PacketDataChunk {
	int offset;
	int d_len;
	char * d;
	struct _PacketDataChunk * next;
	struct _PacketDataChunk * prev;
} PacketDataChunk;

typedef struct _PacketMiss {
	int offset;
	int d_len;
} PacketMiss;

typedef struct _MissRange {
	unsigned int from;
	unsigned int to;
	struct _MissRange *next;
	struct _MissRange *prev;
} MissRange ;

typedef struct _ChunkedObjEntry {
	int url_id;
	int o_id;	/* object id. if 0, this is html part */
	int size_data;	/* size of data */
	int h_size;  /* len of mime header + request */
	char * data;		/* partial data when size is knowed */
	PacketDataChunk *lpdc;	/* list of chunck of data */
				/* order by offset. The first packet # 0 */
				/* is the MIME header */
	PacketDataChunk *beg;	/* pointer to mime packet (begin)*/
	PacketDataChunk *end;	/* pointer to is_end packet (end)*/
	PacketMiss *lpmiss; /* list of missing packet */
	MissRange * lmr;	/* range missing data */
} ChunkedObjEntry;

typedef struct _ChunkedDocEntry {	/* need to assemble packet */
					/* data struct for PacketToDoc */
	int url_id;
	int nobj;			/* number of object for this doc */
					/* we know the number when we have */
					/* the html part */
	int h_nobj;			/* temporary number of nobj, because */
					/* some time the html part come late */
	ChunkedObjEntry **co_tab;	/* tempory space to assemble object */
} ChunkedDocEntry;

extern DocEntry *mc_local_docs;
extern void McCreateDocEntry(char *fname, char* aurl_wa, MimeHeaderStruct *mhs);
extern void McDocToPacket(int url_id);

typedef struct _Source *Sourcep;

extern int PutPacketInChkObj(Sourcep s, ChunkedObjEntry ** co_tab,
	int url_id, int o_id, int offset, char * d, int d_len, int is_end);

extern int IsAllObjectHere(DocEntry *doce);
extern void ChkObjToDocObj(struct _Source *s, unsigned int url_id, unsigned int o_id);
extern int UpdChkObj(ChunkedObjEntry * coe, char *d, unsigned int offset, unsigned int d_len);
extern int MergeChkObjLpdc( int size, ChunkedObjEntry * coe, char * d , int offset, int d_len);

#endif
