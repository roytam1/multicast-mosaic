/* Author: Gilles Dauphin
 * Version 3.4.0 [Jan99]
 */
#ifdef MULTICAST

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/mime.h"

#include "mc_rtp.h"

#define HTML_OBJECT_DATA_TYPE (0x02)
#define HTML_STATE_DATA_TYPE  (0)

#define MC_MAX_SDES_NAME_LEN 255	/* max len of a name */

/* ####################################### */

#define DATA_CHUNK_SIZE 512
#define PROTO_OVERHEAD (52)	/* IP + UDP + RTP + specific PT */
#define BAND_WIDTH (1000000)      /* 1 Mbits/s */

   
typedef struct _RtpPacket {     
	u_int16_t seqn;
        u_int32_t rtp_ts;
	u_int32_t ssrc;
        unsigned char is_eod;
	char *to_free;		/* when is_eod is true, free data */
	int data_type;
        int id;
        int offset;  
        char *d;
        int d_len;
				/* only for sender */
        struct _RtpPacket * next;
        int duration; 
} RtpPacket;

typedef struct _RtcpPacket {     
	u_int8_t	pt;	/* type of Rtcp */
	u_int16_t	len;	/* len in 32 bit word - 1 */
	u_int32_t	ssrc;
        char		*d;	/* PT specific */
        int		d_len;
} RtcpPacket;


typedef struct _SdesStruct {
	char cname[257];
	char name[257];
	char tool[257];
	char email[257];
	unsigned short	uc_rtp_port;	/* net byteorder */
	unsigned short	uc_rtcp_port;	/* net byteorder */
	IPAddr		uc_rtp_ipaddr;	/* net byteorder */
} SdesStruct;


/* ####################################### */

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

typedef struct _ChunkedBufStruct {
	int size_data;		/* size of data */
	char * data;		/* partial data when size is knowed */
	PacketDataChunk *lpdc;	/* list of chunck of data */
				/* order by offset. The first packet # 0 */
				/* is the MIME header */
	PacketDataChunk *beg;	/* pointer to mime packet (begin)*/
	PacketDataChunk *end;	/* pointer to is_end packet (end)*/
	PacketMiss *lpmiss; /* list of missing packet */
	MissRange * lmr;	/* range missing data */
	MimeHeaderStruct *mhs;
} ChunkedBufStruct;


typedef struct _McStateStruct {
	int statid;		/* the stateid */
	int start_moid;		/* begin with this object */
	int n_do;		/* number of depend object */
	DependObjectTab dot;	/* liste of dependant object */
	struct timeval ts;      /* timstamp (struct timeval) */

	int len_buffer;
	char * buffer;		/* buffer containing temp data (receiver)*/
	int buffer_status;	/* status of buffer (receiver) */
	ChunkedBufStruct *chkbuf; /* chunked data. must be assemble in buffer (receiver) */
	char *sdata;		/* sender data */
	int sdata_len;		/* len sender data */
} McStateStruct;

typedef struct _McObjectStruct {
	int moid;		/* multicast object id */
	int statid;		/* even if is a stateless */
	int stateless;
	int n_do;               /* number of depend object */
	DependObjectTab dot;    /* liste of dependant object */
	struct timeval ts;      /* timstamp (struct timeval) */

	int len_buffer;
	char * buffer;          /* buffer containing temp data (receiver)*/
	int buffer_status;      /* status of buffer */
	ChunkedBufStruct *chkbuf; /* chunked data. must be assemble in buffer */

        int exist;              /* Is this entry in used ? (sender) */
        char *aurl;             /* Canonical URL for this document. */
        char *fname;            /* multicast cache file name for data */
	int file_len;		/* len of all data head+body(sender) */
        time_t last_modify;
        MimeHeaderStruct * mhs; /* associated MIME info (sender)*/
} McObjectStruct;

#define EMPTY_BUFFER 1
#define CHUNKED_BUFFER 2
#define COMPLETE_BUFFER 3
#define PARSED_BUFFER 4
#define PARSED_ALL_DEPEND_BUFFER 5


/* #######################################*/
typedef struct _GuiEntry {
	Widget form;
	Widget label;
	Widget toggle;
	int nu;
	struct _Source * source;
} GuiEntry;

/* An entry in a hash bucket, containing a URL (in canonical, absolute form) */
typedef struct _McHashEntry {
        char *aurl;              /* Canonical URL for this document. */
        int moid;
        struct _McHashEntry *next;
} McHashEntry;

typedef struct _McBucket {
        McHashEntry *head;
        int count;
} McBucket;

typedef struct _Source {
	int 		mute;	/* displayed ? */
	mo_window 	*win;	/* winodw for sender */

	u_int32_t	srcid;	/* id of the source */
	IPAddr		addr;	/* IPV4 adresse */
	char *		s_srcid; /* unique string */
	int		is_sender; /* is emetting */

	unsigned short	uc_rtp_port;	/* net byteorder */
	unsigned short	uc_rtcp_port;	/* net byteorder */
	IPAddr		uc_rtp_ipaddr;	/* net byteorder */

	struct _Source 	*hlink;	/* hashcode scrid link */
	struct _Source 	*next;	/* list of source */
	int		first_seq;		/* very first seqn */
	int		cur_seq;		/* seqno */
	int		last_valid_seq;

	char sdes_cname[257];
	char sdes_tool[257];
	char sdes_email[257];
	char sdes_name[257];

	GuiEntry 	* gui_ent; /* graphique interface for this source */
				/* only for the user list */

	int cwuid;		/* current wanted url_id doc */
	int cduid;		/* current display url_id doc */
	int last_valid_state_id;	/* the last full state we see */
	int last_valid_object_id;	/* the last full object we see */
					/* sequential */
	int current_state_id_in_window; /* what state is in current window */

	long		lts;	/* local time stamp (unixtime) */

/* cache for source */
	char *source_cachedir_name;
	int source_len_cachedir_name;

	int states_tab_size;	/* number of state in states tab */
	McStateStruct *states;	/* states tab */
	int objects_tab_size;	/* number of object in object tab */
	McObjectStruct *objects;	/* objects tab */

	McBucket *hash_tab;

/* frame stuff */
	int frameset_moid;
	int frameset_dot_count;

} Source;

typedef struct _CnflcAddr {
	IPAddr addr;
	long lts;		/* local time stamp */
	struct _CnflcAddr * next;
} CnflcAddr;

/* ##################### */

                        /* sockets descriptor */
extern int		mc_fd_rtp_w;
extern int		mc_fd_rtp_w;
extern int		mc_fd_rtp_r;
extern int		mc_fd_rtcp_w;
extern int		mc_fd_rtcp_r;
extern int		uc_fd_rtp_w;
extern int		uc_fd_rtp_r;
extern int		uc_fd_rtcp_w;
extern int		uc_fd_rtcp_r;

extern unsigned short	uc_rtp_addr_port;	/* net byte order */
extern unsigned short	uc_rtcp_addr_port;	/* net byte order */

extern IPAddr		mc_local_ip_addr;
extern char		*mc_local_ip_addr_string;
extern char		mc_local_cname[];

extern int		mc_local_state_id;
extern int		mc_local_object_id;

extern int		mc_status_report_state_id;
extern int		mc_status_report_object_id;

extern McStateStruct	*mc_sender_state_tab;
extern McObjectStruct	*moid_sender_cache;

extern RtpPacket	*mc_rtp_packets_list;
extern int		mc_write_rtp_data_next_time;
extern XtIntervalId	mc_write_rtp_data_timer_id;
extern unsigned int	mc_local_srcid;	/* SSRC for me */

extern XtIntervalId	mc_rtcp_w_timer_id;
extern unsigned long    mc_rtcp_w_time;

extern int		mc_own_traffic_looped;
extern int		mc_collision_with_me;
extern int		mc_third_party_loop;

extern u_int32_t	rtp_init_time;

extern mo_window	*mc_send_win;

extern struct timeval unixtime();

extern void SendBye(int src, char *r);
extern void SendSdesCname();

extern void UcRtcpWriteSdesCb(XtPointer clid, XtIntervalId * time_id);
extern void McRtcpWriteCb(XtPointer clid, XtIntervalId * time_id);

extern u_int32_t McNewSrcid(IPAddr addr);
extern CnflcAddr * find_conflict_addr(IPAddr addr);
extern CnflcAddr * add_conflict_addr(IPAddr addr);
extern SdesStruct * parse_sdes(RtcpPacket* rcs);
extern Source* mc_rtp_demux(u_int32_t srcid, IPAddr addr_from);
extern Source* uc_rtp_demux(u_int32_t srcid, IPAddr addr_from, unsigned short port_from);
extern Source* mc_rtcp_demux(u_int32_t srcid, IPAddr addr_from, RtcpPacket* rcs);
extern Source* uc_rtcp_demux(u_int32_t srcid, IPAddr addr_from, unsigned short port_fgrom, RtcpPacket* rcs);
extern void McUpdateDataSourceWithState(Source *s, int is_end, u_int16_t seqn,
        u_int32_t rtp_ts, u_int32_t ssrc,
        u_int32_t state_id, u_int32_t offset, char *d,
        u_int32_t d_len);
extern void McUpdateDataSourceWithObject(Source *s, int is_end, u_int16_t seqn,
        u_int32_t rtp_ts, u_int32_t ssrc,
        u_int32_t object_id, u_int32_t offset, char *d,
        u_int32_t d_len);
extern void McProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from);
extern void UcProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from,
        unsigned short port_from);
extern void ProcessRtcpSdes(Source *s, RtcpPacket* rcs);
extern void McQueryRepairFromStatr(Source *s, RtcpPacket* rcs);

extern int PutPacketInChkBuf(ChunkedBufStruct *cbs,
	int is_end, int offset, char * d, int d_len);
extern int ChkBufToBuf(ChunkedBufStruct *cbs, char ** buf_ret);

extern IPAddr GetLocalIpAddr(void);
extern int McOpenRead(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McOpenWrite(IPAddr ip,unsigned short port,unsigned char ttl);
extern int UcOpenRead(IPAddr ip, unsigned short *port);
extern void McSendRtpDataTimeOutCb(XtPointer clid, XtIntervalId * id);
extern int McRead(int fd, unsigned char ** buf, IPAddr * ipfrom);
extern int McWrite( int fd, unsigned char * buf, int len);
extern int DewrapRtpData( unsigned char *buf, int len_buf, RtpPacket *rs_ret);
extern int DewrapRtcpData( unsigned char *buf, int len_buf, RtcpPacket *rcs_ret);
extern int UcRead(int fd, unsigned char ** buf, IPAddr * ipfrom,
        unsigned short* port_from);

extern GuiEntry * CreateMemberGuiEntry(Source *s);

u_int32_t ntptime(void);

/* Cache stuff */

extern void McSenderCacheInit( char * root_name);
extern int McSenderCacheFindData(char *aurl,
	char **fname_ret, MimeHeaderStruct *mhs_ret);
extern void McSenderCachePutDataInCache(char *fname, char *aurl,
	MimeHeaderStruct *mhs, int moid, DependObjectTab dot, int ndo,
	char **fname_ret, MimeHeaderStruct * mhs_ret);
extern void McSenderCachePutErrorInCache( char *aurl, int status_code,
		int moid, char ** fname_ret, MimeHeaderStruct *mhs_ret);


extern void MakeSenderState(MimeHeaderStruct *mhs, int sid);
extern void McSendOject(int moid ) ;

extern void McSendState(int stateid);
extern int McCheckStateQuery(int sid, int offset, int len);
extern int McCheckObjectQuery(int moid, int offset, int len);

extern int McRcvrSrcAllocObject(Source * s, int moid);
extern int McRcvrSrcAllocState(Source * s, int state_id);

extern void McRcvSrcScheduleCheckState( Source *s, int state_id);

extern void McStoreQueryRepair(Source *s , RtcpPacket* rcs);

extern void UcRtpSendDataPacketTo(IPAddr addr, unsigned short port , RtpPacket *p);

extern void McSourceCacheInit( Source *src, char * root_name);

extern void McSourceCachePutDataInCache(Source *s, char * body, int body_len,
        char *aurl, MimeHeaderStruct *mhs, int moid,
        char **fname_ret, MimeHeaderStruct *mhs_ret);
#endif
