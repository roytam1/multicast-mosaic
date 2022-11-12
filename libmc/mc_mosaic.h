/* Author: Gilles Dauphin
 * Version 3.3.0 [Jan98]
 */
#ifdef MULTICAST

#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"
#include "../src/mime.h"

#include "mc_rtp.h"


#define MC_MAX_SDES_NAME_LEN 255	/* max len of a name */

/* ####################################### */

#define DATA_CHUNK_SIZE 512
#define PROTO_OVERHEAD (52)	/* IP + UDP + RTP + specific PT */
#define BAND_WIDTH (20000)      /* 20 Kb/s */

   
typedef struct _RtpPacket {     
        unsigned char is_end;
	u_int16_t seqn;
        u_int32_t rtp_ts;
	u_int32_t ssrc;
        int url_id;
        int o_id;
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
	char *aurl_wa;
	MimeHeaderStruct *mhs;
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
	char *aurl_wa;
	MimeHeaderStruct *mhs;
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


/* #######################################*/
typedef struct _GuiEntry {
	Widget form;
	Widget label;
	Widget toggle;
	int nu;
	struct _Source * source;
} GuiEntry;


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

	int huid;		/* highter url_id we hear for this source */
	int cwuid;		/* current wanted url_id doc */
	int cduid;		/* current display url_id doc */
	int last_valid_url_id;	/* the last full doc we see */

	long		lts;	/* local time stamp (unixtime) */
	DocEntry ** doc;	/* store the completed data here */
	ChunkedDocEntry ** chkdoc; /* assemble packet here */
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

extern int		mc_local_url_id;
extern int		mc_local_object_id;
extern int		mc_state_report_url_id;
extern int		mc_state_report_o_id;
extern int		mc_state_report_len;

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

extern timeval unixtime();

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
extern void McUpdateDataSource(Source *s, int is_end, u_int16_t seqn,
        u_int32_t rtp_ts, u_int32_t ssrc,
        u_int16_t url_id, u_int16_t o_id, u_int32_t offset, char *d,
        u_int32_t d_len);
extern void McProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from);
extern void UcProcessRtcpData(unsigned char *buf, int len, IPAddr addr_from,
        unsigned short port_from);
extern void ProcessRtcpSdes(Source *s, RtcpPacket* rcs);
extern void McRepairFromStatr(Source *s, RtcpPacket* rcs);

extern DocEntry *mc_local_docs;
extern void McCreateDocEntry(char *fname, char* aurl_wa, MimeHeaderStruct *mhs);
extern void McDocToPacket(int url_id);


extern int PutPacketInChkObj(Source * s, ChunkedObjEntry ** co_tab,
	int url_id, int o_id, int offset, char * d, int d_len, int is_end);

extern int IsAllObjectHere(DocEntry *doce);
extern void ChkObjToDocObj(struct _Source *s, unsigned int url_id, unsigned int o_id);
extern int UpdChkObj(ChunkedObjEntry * coe, char *d, unsigned int offset, unsigned int d_len);
extern int MergeChkObjLpdc( int size, ChunkedObjEntry * coe, char * d , int offset, int d_len);
extern void McCreateObjectEntry(char *fname, char* aurl, MimeHeaderStruct *mhs);
extern void McCreateErrorEntry(char *aurl, int status_code);
extern void McObjectToPacket(int url_id, int o_id);
extern int SourceAllocDocs(Source * s, int url_id);
extern int SourceAllocObjs(Source * s, int url_id, int o_id);
extern void ChkDocToDoc(Source * s, int url_id);

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
extern void UcRtpSendDataPacket(Source * s, RtpPacket *p);
extern void UcRtpSendData(Source * s , RtcpPacket* rcs);

extern GuiEntry * CreateMemberGuiEntry(Source *s);

u_int32_t ntptime(void);
#endif
