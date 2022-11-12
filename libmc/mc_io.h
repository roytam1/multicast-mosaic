/* Author: G.Dauphin
 * Version 3.3.0 [Jan98]
 */

#ifdef MULTICAST

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

extern RtpPacket	*mc_rtp_packets_list;
extern int		mc_write_rtp_data_next_time;
XtIntervalId		mc_write_rtp_data_timer_id;


extern IPAddr GetLocalIpAddr(void);
extern int McOpenRead(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McOpenWrite(IPAddr ip,unsigned short port,unsigned char ttl);
extern int UcOpenRead(IPAddr ip, unsigned short *port);
extern void McSendRtpDataTimeOutCb(XtPointer clid, XtIntervalId * id);
extern int McRead(int fd, unsigned char ** buf, IPAddr * ipfrom);
extern int DewrapRtpData( unsigned char *buf, int len_buf, RtpPacket *rs_ret);
extern int DewrapRtcpData( unsigned char *buf, int len_buf, RtcpPacket *rcs_ret);

#endif
