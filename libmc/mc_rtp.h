/* see Copyright at end */

/**
 * RTCP packet format.
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    SC   |      PT       |             length            | header
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                             SSRC                              | chunk
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
 *
 * where:
 *   V - version (2 bits). 
 *   P - padding (1 bit).
 *   SC - count (5 bits).
 *   PT - payload type (8 bits);
 *   lenght - packet lenght (16 bits) = (total octets)/4 - 1.
 *
 *   all time variables are expressed in milliseconds, except for
 *   transmission where time is converted to the required format.
 */

/* RR SR as RFC
 * extension = URL_ID for this SSRC
 */

/* BYE as RFC */

/* No APP packet */
/* SDES as RFC user@host (CNAME) */


/*
 * RTP packet format :
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      |       sequence number         |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |           synchronization source (SSRC) identifier            |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |            contributing source (CSRC) identifiers             |
 * |                             ....                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Meanings:
 *   V - version (2 bits). 
 *   P - padding (1 bit).
 *   X - extension (1 bit).
 *   CC - CSRC count (4 bits).
 *   M - marker (1 bit, data block boundary). 1 -> end, 0 -> more.
 *   PT - payload type (7 bits).
 *
 * RTP packet format for mMosaic:
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      | sequence number=lurlid[15-0]  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | SSRC          |hurlid[23-16]  |    pid                        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |    code       |     size of eo = seo                          |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    nombre_eo                  |      num_eo                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    nombre_packet              |      num_packet               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  data       ....                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Meanings:
 *   V - version (2 bits).  	= 2
 *   P - padding (1 bit).	= 0
 *   X - extension (1 bit).	= 0
 *   CC - CSRC count (4 bits).	= 0
 *   M - marker (1 bit, data block boundary). 1 -> end, 0 -> more.	= 0
 *   PT - payload type (7 bits). = RTP_PT_WEB    (48)
 *   sequence number		= url_id[15-0]
 *   timestamp			= timestamp[31-10] second [9-0] 1/1024 sec
 *				  timestamp is near millisecond
 *   SSRC			= 0x01
 *   hurlid			= url_id[23-16]
 *   pid			= 16bits process pid 
 * NO CSRC but in place:
 *   code			= 8bits code
 *   seo			= size of this eo in byte (max 16Mb)
 *   nombre_eo			= nombre de eo pour cet urlid
 *   num_eo			= numero du eo
 *   nombre_packet		= nombre_packet pour cette eo
 *   num_packet			= numero du packet
 *
 * OLD RTP packet format for mMosaic (obsolete):
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|X|  CC   |M|     PT      | sequence number=hurlid[23-8]  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                           timestamp                           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | synchronization source (SSRC)     identifier IPv4 Addr        |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |    pid                        | code          | lurlid        |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    nombre_eo                  |      num_eo                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |    nombre_packet              |      num_packet               |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                 size of eo = seo                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                  data       ....                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Meanings:
 *   V - version (2 bits).  	= 2
 *   P - padding (1 bit).	= 0
 *   X - extension (1 bit).	= 0
 *   CC - CSRC count (4 bits).	= 0
 *   M - marker (1 bit, data block boundary). 1 -> end, 0 -> more.	= 0
 *   PT - payload type (7 bits). = RTP_PT_WEB    (48)
 *   hurlid[23-8]		= 23-8 bits of urlid
 *   timestamp			= timestamp[31-10] second [9-0] 1/1024 sec
 *				  timestamp is near millisecond
 *   SSRC			= IPv4 Addr
 * NO CSRC but in place:
 *   pid			= 16bits pid
 *   code			= 8bits code
 *   lurlid			= 8 last bit of urlid
 *   nombre_eo			= nombre de eo pour cet urlid
 *   num_eo			= numero du eo
 *   nombre_packet		= nombre_packet pour cette eo
 *   num_packet			= numero du packet
 *   seo			= size of this eo in byte
 */


#ifndef vic_rtp_h
#define vic_rtp_h

#include <sys/param.h>
#include <sys/types.h>
#include <sys/time.h>

#include "rtp-config.h"

/* RTP standard content encodings for audio */
#define RTP_PT_PCMU		0
#define RTP_PT_CELP		1
#define RTP_PT_GSM		3
#define RTP_PT_DVI		5
#define RTP_PT_LPC		7

#define RTP_PT_BVC		22	/* Berkeley video codec */

/* RTP standard content encodings for video */
#define RTP_PT_RGB8		23 	/* 8-bit dithered RGB */
#define RTP_PT_HDCC		24 	/* SGI proprietary */
#define RTP_PT_CELLB		25 	/* Sun CellB */
#define RTP_PT_JPEG		26	/* JPEG */
#define RTP_PT_CUSEEME		27	/* Cornell CU-SeeMe */
#define RTP_PT_NV		28	/* Xerox PARC nv */
#define RTP_PT_PICW		29	/* BB&N PictureWindow */
#define RTP_PT_CPV		30	/* Concept/Bolter/Viewpoint codec */
#define RTP_PT_H261		31	/* ITU H.261 */
#define RTP_PT_MPEG		32 	/* MPEG-I & MPEG-II */
#define RTP_PT_MP2T		33 	/* MPEG-II either audio or video */

#define RTP_PT_WEB		(48)	/* web payload type */

/* backward compat hack for decoding RTPv1 ivs streams */
#define RTP_PT_H261_COMPAT 	127



/* Offset from UNIX's epoch to the NTP epoch in seconds (NTP's JAN_1970) */
#define RTP_EPOCH_OFFSET	2208988800UL
#define RTP_VERSION 2

#define RTP_M	0x0080	/* Marker: significant event <e.g. frame boundary> */
#define RTP_P	0x2000	/* Padding is present */
#define RTP_X	0x1000	/* Extension Header is present */

/* Basic RTP header */
struct rtphdr {
	u_int16_t rh_flags;	/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	u_int16_t rh_seqno;	/* sequence number */
	u_int32_t rh_ts;	/* media-specific time stamp */
	u_int32_t rh_ssrc;	/* synchronization src id */
	/* data sources follow per cc */
};

struct rtcphdr {
	u_int16_t rh_flags;	/* T:2 P:1 CNT:5 PT:8 */
	u_int16_t rh_len;	/* length of message (in bytes) */
	u_int32_t rh_ssrc;	/* synchronization src id */
};

typedef struct {
	u_int32_t upper;	/* more significant 32 bits */
	u_int32_t lower;	/* less significant 32 bits */
} ntp64;

/*
 * Sender report.
 */
struct rtcp_sr {
	ntp64 sr_ntp;		/* 64-bit ntp timestamp */
	u_int32_t sr_ts;	/* reference media timestamp */
	u_int32_t sr_np;	/* no. packets sent */
	u_int32_t sr_nb;	/* no. bytes sent */
};

/*
 * Receiver report.
 * Time stamps are middle 32-bits of ntp timestamp.
 */
struct rtcp_rr {
	u_int32_t rr_srcid;	/* sender being reported */
	u_int32_t rr_loss;	/* loss stats (8:fraction, 24:cumulative)*/
	u_int32_t rr_ehsr;	/* ext. highest seqno received */
	u_int32_t rr_dv;	/* jitter (delay variance) */
	u_int32_t rr_lsr;	/* orig. ts from last rr from this src  */
	u_int32_t rr_dlsr;	/* time from recpt of last rr to xmit time */
};

#define RTCP_SDES_END	0
#define	RTCP_SDES_CNAME	1	/* official name (mandatory) */
#define	RTCP_SDES_NAME	2	/* personal name (optional) */
#define	RTCP_SDES_EMAIL	3	/* e-mail addr (optional) */
#define	RTCP_SDES_PHONE	4	/* telephone # (optional) */
#define	RTCP_SDES_LOC	5	/* geographical location */
#define	RTCP_SDES_TOOL	6	/* name/(vers) of app */
#define	RTCP_SDES_NOTE	7	/* transient messages */
#define	RTCP_SDES_PRIV	8	/* private SDES extensions */

#define RTCP_PT_SR	200	/* sender report */
#define RTCP_PT_RR	201	/* receiver report */
#define RTCP_PT_SDES	202	/* source description */
#define RTCP_PT_BYE	203	/* end of participation */
#define RTCP_PT_APP	204	/* application specific functions */

#define RTCP_PT_LRMP	206	/* Light-weight Reliable Multicast Protocol */


#define		RTCP_SDES_MIN	0
#define		RTCP_SDES_MAX	8

/*
 * Parameters controling the RTCP report rate timer.
 */
#define RTCP_SESSION_BW_FRACTION 	(0.05)
#define RTCP_MIN_RPT_TIME 		(5.)
#define RTCP_SENDER_BW_FRACTION 	(0.25)
#define RTCP_RECEIVER_BW_FRACTION 	(1. - RTCP_SENDER_BW_FRACTION)
#define RTCP_SIZE_GAIN 			(1./8.)

/*
 * Largest (user-level) packet size generated by our rtp applications.
 * Individual video formats may use smaller mtu's.
 */
#define RTP_MTU 512

#define MAXHDR 24

/*
 * Motion JPEG encapsulation.
 *
 * 0                   1                   2                   3
 * 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      MBZ      |                frag offset                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |      Type     |       Q       |     Width     |     Height    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * 
 * Type = Index into a table of predefined JPEG parameters
 * Width = Width of input in 8-pixel blocks
 * Height = Height of input in 8-pixel blocks
 * Q = Quality factor (0-100 = std, >100 = custom)
 * Frag offset = The byte offset into the frame for the data in 
 * this packet
 */
struct jpeghdr {
	u_int32_t off;		/* fragment offset */
	u_int8_t type;		/* id of jpeg decoder params */
	u_int8_t q;		/* quantization factor (or table id) */
	u_int8_t width;		/* 1/8 frame width */
	u_int8_t height;	/* 1/8 frame height */
};

/*
 * NV encapsulation.
 */
struct nvhdr {
	u_int16_t width;
	u_int16_t height;
	/* nv data */
};

/*
 * CellB encapsulation.
 */
struct cellbhdr {
	u_int16_t x;
	u_int16_t y;
	u_int16_t width;
	u_int16_t height;
	/* cells */
};

/*
 * H.261 encapsulation.
 * See Internet draft.
 *
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |SBIT |EBIT |I|V| GOBN  |  MBAP   |  QUANT  |  HMVD   |  VMVD   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
#ifdef notdef
struct h261hdr {
	u_int16_t	flags;
	u_int16_t off;
};
#endif

struct bvchdr {
	u_int8_t version;
	u_int8_t pad;
	u_int8_t width;
	u_int8_t height;
	u_int32_t quant;
	u_int8_t sbit;
	u_int8_t ebit;
	u_int16_t blkno;
};


/*                                    
 * LRMP as an extension to RTCP.      
 * New packet types:                  
 *     o ECHO to measure the round-trip time.
 *     o ECHO ACK.                    
 *     o NACK.                        
 *     o NACK_ALL.                        
 *     o SYNC to inform the current seqno. 
 *     o SYNC ERROR to report any reception failure.
 *                                    
 * NACK packet format:                
 *                                    
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P| ST=NACK | PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              SSRC                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * |===============================================================|
 * |                        first sender SSRC                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              pid              |              url_id           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              url_id           |              num_eo           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              FSL              |              BLP              |
 * |===============================================================|
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * FSL: first seqno lost for this num_eo. (First packet number for this num_eo) 
 * BLP: bitmask of the following lost packets.
 *
 * NACK_ALL packet format:                
 *                                    
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|ST=NACK_A| PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                              SSRC                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * |===============================================================|
 * |                        first sender SSRC                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              pid              |              url_id           |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              url_id           |           num_eo              |
 * |===============================================================|
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 *
 * Il manque tout l'objet num_eo
 *                                    
 * ECHO packet format:                
 * 
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P| ST=ECHO | PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             SSRC                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                     first destination SSRC                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                            timestamp                          |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * timestamp - middle  32 bits of NTP time. 
 *                                    
 * ECHO_ACK packet format:            
 *                                    
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|ST=ECHO_A| PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             SSRC                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                   first SSRC of ECHO sender                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |        pid                    |     unused                    |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    timestamp of ECHO packet                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    delay since received ECHO packet           |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * delay is in units of fraction of 1/65536 seconds.
 *                                    
 * SYNC packet format:                
 *                                    
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P| ST=SYNC | PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             SSRC                              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                    Next Seqno to be sent                      |
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 *                                    
 * Only low 16 bits of seqno are meaningful.
 *  o sources may have already or never sent data.
 *  o for a source sent data, this seqno tells the next seqno to be used
 *    in sending. If a receiver has not received till this seqno, loss
 *    needs to be repaired.           
 *  o for a source never send data, it tells the starting seqno.
 *                                    
 * SYNC_ERROR packet format:          
 *                                    
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|ST=SYNC_E| PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                             SSRC                              |
 * |===============================================================|
 * |                        first sender SSRC                      |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * | CA  |        NL               |              FSL              |
 * |===============================================================|
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * Unrecoverable sequence error.      
 *   CA: cause, 3 bits.               
 *   NL: number of lost packets, 13 bits.
 *   FSL: first seqno lost, 16 bits.  
 */                                   
/**                                   
 * implements the Light-weight Reliable Multicast Protocol.
 */ 
#define LRMP_ECHO  		0
#define LRMP_ECHO_ACK  		1
#define LRMP_SYNC  		2
#define LRMP_NACK  		3
#define LRMP_SYNC_ERROR		4
#define LRMP_NACK_ALL		5
#define RTP_HEADER_LEN 	 	12
#define RTP_DATA_SIZE  		(RTP_MTU - RTP_HEADER_LEN)


#define RTP_CONST_HPT_WEB (0x8030)	/* PT=48 */


#endif
/*-
 * Copyright (c) 1993-1994 The Regents of the University of California.
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. All advertising materials mentioning features or use of this software
 *    must display the following acknowledgement:
 *      This product includes software developed by the University of
 *      California, Berkeley and the Network Research Group at
 *      Lawrence Berkeley Laboratory.
 * 4. Neither the name of the University nor of the Laboratory may be used
 *    to endorse or promote products derived from this software without
 *    specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE REGENTS AND CONTRIBUTORS ``AS IS'' AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
 * ARE DISCLAIMED.  IN NO EVENT SHALL THE REGENTS OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS
 * OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
 * HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 * LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
 * OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
 * SUCH DAMAGE.
 */

/* retified by Gilles Dauphin for mMosaic 21 Jan 1997 */
/* inspired de weboard from inria ((Tie.Liao@inria.fr) */
/* RTP Control Protocol version 2 (RFC 1889). */

/*********************************************************************
 * This Software is copyright INRIA. 1997.
 *  
 * INRIA holds all the ownership rights on the Software. The scientific
 * community is asked to use the SOFTWARE in order to test and evaluate
 * it.
 *      
 * INRIA freely grants the right to use the Software. Any use or
 * reproduction of this Software to obtain profit or for commercial ends
 * being subject to obtaining the prior express authorization of INRIA.
 *   
 * INRIA authorizes any reproduction of this Software
 *  
 *  - in limits defined in clauses 9 and 10 of the Berne agreement for
 * the protection of literary and artistic works respectively specify in
 * their paragraphs 2 and 3 authorizing only the reproduction and quoting
 * of works on the condition that :
 *      
 *  - "this reproduction does not adversely affect the normal
 * exploitation of the work or cause any unjustified prejudice to the
 * legitimate interests of the author".
 *      
 *  - that the quotations given by way of illustration and/or tuition
 * conform to the proper uses and that it mentions the source and name of
 * the author if this name features in the source",
 *      
 *  - under the condition that this file is included with any
 * reproduction.
 *  
 * Any commercial use made without obtaining the prior express agreement
 * of INRIA would therefore constitute a fraudulent imitation.
 *   
 * The Software beeing currently developed, INRIA is assuming no
 * liability, and should not be responsible, in any manner or any case,
 * for any direct or indirect dammages sustained by the user.
 *                                    
 * Furthermore, INRIA grants for free the right to adapt, modify or integrate
 * the SOFTWARE in an other programme, provided that any derived work
 * should be distributed in the same conditions as the SOFTWARE.
 *********************************************************************
 */                                   
/*                                    
 * LRMP.java - Light-weight Reliable Multicast Protocol.
 * Author:  Tie Liao (Tie.Liao@inria.fr). 
 * Created: 17 September 1996.        
 * Updated: no.                       
 */      
