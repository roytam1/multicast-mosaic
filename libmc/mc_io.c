/* sockio.c
 * Author: Gilles Dauphin
 * Version 3.1.1 [May97]
 *
 * Copyright (C) 1997 - G.Dauphin, P.Dax
 *
 * See the file "license.mMosaic" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES. 
 *
 * Bug report : dauphin@sig.enst.fr dax@inf.enst.fr
 */

#ifdef MULTICAST

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/param.h>
#include <Xm/XmAll.h>



#if defined(__QNX__) || defined(ultrix)
/* From frank@ctcqnx4.ctc.cummins.com Mon Apr 28 02:33:01 1997
/* QNX tcp/ip is not as complete, and I have to modify "libmc/mc_sockio.c"
/* to make it compile. I am wondering IP multicast is supported, I will
/* check with QNX to see ..  
/* I will let you know, maybe we will have to disable this for QNX.  */

/*those are borrowed from Linux */
/*QNX's <netinet/in.h> is far from complete, some stuffs are not supported */

#define IP_MULTICAST_TTL        0x11    /* set/get IP multicast timetolive */
#define IP_ADD_MEMBERSHIP       0x13    /* add  an IP group membership     */
/*
 * Argument structure for IP_ADD_MEMBERSHIP and IP_DROP_MEMBERSHIP.
 */
struct ip_mreq {
        struct in_addr  imr_multiaddr;  /* IP multicast address of group */
        struct in_addr  imr_interface;  /* local IP address of interface */
};
#endif /* __QNX__ */

#include "../libnut/mipcf.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../src/mosaic.h"

#include "mc_mosaic.h"
#include "mc_main.h"
#include "mc_rtp.h"
#include "mc_misc.h"

IPAddr		mc_local_ip_addr;

RtpPacket *mc_rtp_packets_list =NULL;
int mc_write_rtp_data_next_time = 10;		/* en millisec */

#ifdef IPV6 
static const IPAddr anyaddr = IPV6ADDR_ANY_INIT;
#endif  
 
#define MC_MAX_BUF_SIZE		32767	/* max io buf size for socket */

static unsigned char emit_buf[MC_MAX_BUF_SIZE+1];
static unsigned char recv_buf[MC_MAX_BUF_SIZE+1];
static unsigned int mc_send_cnt = 0;
static unsigned int mc_my_upd_time ;

#ifdef SOLARIS
#ifdef  __cplusplus
extern "C" {
#endif

int gethostname(char *name, int namelen); /* because solaris 2.5 include bug */

#ifdef  __cplusplus
}
#endif
#endif

static int		noloopback_broken_ = 1;

IPAddr GetLocalIpAddr(void)
{
#ifdef IPV6         
        struct sockaddr_in6 fakesockaddr;
#else               
        struct sockaddr_in fakesockaddr;
#endif
	IPAddr local_addr;
        char hostname[MAXHOSTNAMELEN];
        struct hostent *hp;
        int sd_len; 
        int fakesock;

        /* This bogosity is to find the IP address of the local host! */
#ifdef IPV6
        if ((fakesock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("gethostname " );
                exit(1);
        }
/*        hp = gethostbyname2(hostname,AF_INET6); */
        hp = hostname2addr(hostname,AF_INET6);
	if (!hp) {
                fprintf(stderr,"IPV6 gasp no hosts \n");
                exit(1) ;
        }
        memcpy((char *)&fakesockaddr.sin6_addr, hp->h_addr, hp->h_length);
        fakesockaddr.sin6_port = 0;
        bind(fakesock, (struct sockaddr *) &fakesockaddr, sizeof(fakesockaddr));
        sd_len = sizeof(fakesockaddr);
        getsockname(fakesock, (struct sockaddr *)&fakesockaddr, &sd_len);
        local_addr = fakesockaddr.sin6_addr;
#else
        if ((fakesock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("gethostname " );
                exit(1);
        }
        hp = gethostbyname(hostname);
        memcpy((char *)&fakesockaddr.sin_addr, hp->h_addr, hp->h_length);
        fakesockaddr.sin_port = htons(0);
        bind(fakesock, (struct sockaddr *) &fakesockaddr, sizeof(fakesockaddr));
        sd_len = sizeof(fakesockaddr);
        getsockname(fakesock, (struct sockaddr *)&fakesockaddr, &sd_len);
        local_addr = fakesockaddr.sin_addr.s_addr;
#endif
        close(fakesock);
	return local_addr;
}

int  UcOpenRead(IPAddr ip, unsigned short *port)
{
#ifdef IPV6         
        struct sockaddr_in6 sin;
#else               
        struct sockaddr_in sin;
#endif
	IPAddr local_addr = ip;
        int sd_len; 
	int on = 1;
        int s;

#ifdef IPV6
        if ((s = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0){
		perror("SO_REUSEPORT");
		exit(1);
	}
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin6_family = AF_INET6;
        sin.sin6_addr.s_addr= local_addr;
        sin.sin6_port = *port;
        if( bind(s, (struct sockaddr *) &sin, sizeof(sin) < 0) {
		perror("bind");
		exit(1);
	}
	*port = sin.sin6_port;
#else
        if ((s = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
	if(setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof(on)) < 0){
		perror("SO_REUSEPORT");
		exit(1);
	}
	memset((char *)&sin, 0, sizeof(sin));
	sin.sin_family = AF_INET;
        sin.sin_addr.s_addr = local_addr;
/*        sin.sin_addr.s_addr = INADDR_ANY;*/
        sin.sin_port = *port;
        if( bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("bind");
		exit(1);
	}
	sd_len = sizeof(sin);
	if ( getsockname(s, (sockaddr*)&sin, &sd_len) < 0 ) {
		perror("getsockname");
		exit(1);
	}
	*port = sin.sin_port;
#endif
	return s;
}


int McOpenRead(IPAddr ip,unsigned short port,unsigned char ttl)
{
#ifdef IPV6
        struct ipv6_mreq mreq;
	struct sockaddr_in6 addr_r;
#else
        struct ip_mreq mreq;
	struct sockaddr_in addr_r;
#endif
        int one = 1;
	int fd=-1;

#ifdef IPV6
        if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) <0 )
#else
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) <0 )
#endif
							{
                perror ("McOpenRead: socket");
                exit(1);
        }
        /* so that more than one process can bind to the same
           SOCK_DGRAM UDP port ( must be placed BEFORE bind() ) */
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one))){
		perror ("setsockopt SO_REUSEADDR");
		exit(1);
	}
        memset(&addr_r,0,sizeof(addr_r));
#ifdef IPV6
        addr_r.sin6_family = AF_INET6;
        addr_r.sin6_port = port;
        addr_r.sin6_addr = ip;
#else
        addr_r.sin_family = AF_INET;
        addr_r.sin_port = port;
        addr_r.sin_addr.s_addr = ip;
#endif
/* Comment from VAT: Try to bind the multicast address as the socket dest
 * address. On many systems this won't work so fall back to a destination
 * of INADDR_ANY if the first bind fails.
 */ 
        if(bind(fd, (struct sockaddr *)&addr_r, sizeof(addr_r)) <0) {
#ifdef IPV6
		addr_r.sin6_addr = anyaddr;
#else
		addr_r.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
		if(bind(fd, (struct sockaddr *)&addr_r, sizeof(addr_r)) <0) {
                	perror ("McOpenRead: bind:");
                	exit(1);
		}
        }
/* Comment from VAT: XXX This is bogus multicast setup that really shouldn't
 * have to be done (group membership should be implicit in the IP class D address,
 * route should contain ttl & no loopback flag, etc.). Steve Deering has promised
 * to fix this for the 4.4bsd release.  We're all waiting with bated breath. 
 */
#ifdef IPV6
        mreq.ipv6mr_multiaddr = ip;
        mreq.ipv6mr_interface = anyaddr;
        if (setsockopt(fd, IPPROTO_IPV6, IPV6_ADD_MEMBERSHIP,
#else
        /*mreq.imr_multiaddr.s_addr = htonl(ip);*/
        mreq.imr_multiaddr.s_addr = ip;
        mreq.imr_interface.s_addr = htonl(INADDR_ANY);
        if (setsockopt(fd, IPPROTO_IP, IP_ADD_MEMBERSHIP,
#endif
            (char *)&mreq, sizeof(mreq)) < 0) {
                perror ("IP_ADD_MEMBERSHIP:");
                exit(1);
        }
	return fd;
}

int McOpenWrite(IPAddr ip,unsigned short port,unsigned char ttl)
{
#ifdef IPV6
	struct sockaddr_in6 addr_w;
#else
	struct sockaddr_in addr_w;
#endif
	char off =0;
	int fd;
	unsigned int t6 = ttl;
 
#ifdef IPV6
	fd = socket(AF_INET6, SOCK_DGRAM, 0);
#else
	fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif
	if(fd<0) {
		perror("socket");
		exit(1);
	}  
#ifdef IPV6
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       (char*)&(t6), sizeof(t6)) < 0) {
		perror("McOpenWrite:setsockopt IP_MULTICAST_HOPS:");
		exit(1);
	}
	memset(&addr_w,0,sizeof(addr_w));
	addr_w.sin6_family = AF_INET6;
	addr_w.sin6_port = port;
	addr_w.sin6_addr = ip;
#else
	memset(&addr_w,0,sizeof(addr_w));
	addr_w.sin_family = AF_INET;
	addr_w.sin_port = port;
	addr_w.sin_addr.s_addr = ip;
	if (connect(fd, (struct sockaddr *)&addr_w, sizeof(addr_w)) < 0) {
		perror("connect");
		exit(1);
	}

/* turn off loopback */
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_LOOP, &off, 1) <0){
/* Comment from VAT:If we cannot turn off loopback (Like on the Microsoft TCP/IP
 * stack), then declare this option broken so that our packets can be
 * filtered on the recv path.
 */
		noloopback_broken_ = 1;
	}
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		       (char*)&(ttl), sizeof(ttl)) < 0) {
		perror("IP_MULTICAST_TTL:");
		exit(1);
	}
#endif
	return fd;
}

/*
 * return number of byte read and the static recv_buf 
 */
int McRead(int fd, unsigned char ** buf, IPAddr * ipfrom)
{
	int cnt;
#ifdef IPV6
	struct sockaddr_in6 addr_r;
#else
	struct sockaddr_in addr_r;
#endif
	int addr_r_len = sizeof(addr_r);
  
	cnt = recvfrom(fd, (char*)recv_buf, MC_MAX_BUF_SIZE,0, 
			(struct sockaddr *)&addr_r, &addr_r_len);
	if (cnt <= 0 ) {
		perror ("recvfrom");
		/* exit(1); */
	}
	*buf = recv_buf;
#ifdef IPV6
	*ipfrom = addr_r.sin6_addr;
#else
	*ipfrom = addr_r.sin_addr.s_addr;
#endif
#ifdef MDEBUG
	printf("McRead: cnt=%d, recv_buf[12]=%u\n",cnt,recv_buf[12]);
#endif
	return cnt;
}

/*
 * return number of byte read and the static recv_buf 
 */
int UcRead(int fd, unsigned char ** buf, IPAddr * ipfrom, 
	unsigned short* port_from)
{
	int cnt;
#ifdef IPV6
	struct sockaddr_in6 addr_r;
#else
	struct sockaddr_in addr_r;
#endif
	int addr_r_len = sizeof(addr_r);
  
	cnt = recvfrom(fd, (char*)recv_buf, MC_MAX_BUF_SIZE,0, 
			(struct sockaddr *)&addr_r, &addr_r_len);
	if (cnt <= 0 ) {
		perror ("recvfrom");
		/* exit(1); */
	}
	*buf = recv_buf;
#ifdef IPV6
	*ipfrom = addr_r.sin6_addr;
	*port_from = addr_r.sin6_port ;
#else
	*ipfrom = addr_r.sin_addr.s_addr;
	*port_from = addr_r.sin_port ;
#endif
#ifdef MDEBUG
	printf("UcRead: cnt=%d, recv_buf[12]=%u\n",cnt,recv_buf[12]);
#endif
	return cnt;
}

int McWrite( int fd, unsigned char * buf, int len)
{
	int cnt;

	cnt = write(fd, (char*)buf, len);
	if(cnt != len){
		perror("write:");
	}
	return cnt;
}


/* return the number of byte read or 0 if probleme */

int DewrapRtpData( unsigned char *buf, int len_buf,
	RtpPacket *rs_ret)
{
	unsigned int rh_flags;
	unsigned int m_flag;
	unsigned int i;
	unsigned char * p = buf;

	if ( len_buf < 20 ){ 
		fprintf(stderr,"Error receiving Rtp data: n = %d\n", len_buf);
		return 0;
	}
	
			/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	m_flag = p[1] & 0x80;
	rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                        ((unsigned long) (p[1] & 0x7f) ) );

	if (rh_flags != (u_int16_t) 0x8062){
		return 0;
	}
	rs_ret->is_end = m_flag;

	rs_ret->seqn =  (u_int16_t)( ((u_int16_t) p[2] << 8) |
				     ((u_int16_t) p[3]      ));

	rs_ret->rtp_ts  =   (u_int32_t)( ((u_int32_t) p[4] << 24) |
				     ((u_int32_t) p[5] << 16) |
				     ((u_int32_t) p[6] << 8 ) |
				     ((u_int32_t) p[7]      ) );

	rs_ret->ssrc = (u_int32_t)( ((u_int32_t) p[8] << 24) |
				    ((u_int32_t) p[9] << 16) |
				    ((u_int32_t) p[10] << 8) |
				    ((u_int32_t) p[11]      ) );

	rs_ret->url_id = (u_int16_t)(((u_int16_t) p[12] << 8) |
				     ((u_int16_t) p[13]      ));

	rs_ret->o_id = (u_int16_t)( ((u_int16_t) p[14] << 8) |
				    ((u_int16_t) p[15]      ));

	rs_ret->offset = (u_int32_t)( ((u_int32_t) p[16] << 24) |
				      ((u_int32_t) p[17] << 16) |
				      ((u_int32_t) p[18] << 8 ) |
				      ((u_int32_t) p[19]      ) );
	rs_ret->d = (char*) &buf[20];
	rs_ret->d_len = len_buf - 20;
	rs_ret->d[rs_ret->d_len] = '\0';
	return 1;
}
 
/*                  
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                           timestamp                           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |           synchronization source (SSRC) identifier            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
   |            contributing source (CSRC) identifiers             |
   |                             ....                              |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   P = 0 (padding)  
   X = 0 (extension)
   CC = 0 (contributor count)
   M = 0 (Marker: use as End Of Object.)
   PT = 98
*/
                    
static int mc_seq_number = 0; 

void McSendRtpDataTimeOutCb(XtPointer clid, XtIntervalId * id)
{
        RtpPacket *p;
        int len_buf;
        int next_time;
	int cnt;
 
        p = mc_rtp_packets_list;
        
        emit_buf[0] = 0x80;     /* V,P,CC */
        emit_buf[1] = p->is_end | 0x62;        /* M, PT=98 */
                                       
        emit_buf[2] = (mc_seq_number >> 8) & 0xff;      /* sequence number */
        emit_buf[3] = mc_seq_number & 0xff; 
        mc_seq_number++;               
                                       
        emit_buf[4] = (p->rtp_ts >> 24) & 0xff;    /* timestamp */
        emit_buf[5] = (p->rtp_ts >> 16) & 0xff;
        emit_buf[6] = (p->rtp_ts >> 8) & 0xff;
        emit_buf[7] =  p->rtp_ts & 0xff;

        emit_buf[8] = (mc_local_srcid >> 24) & 0xff;    /* SSRC */
        emit_buf[9] = (mc_local_srcid >> 16) & 0xff;
        emit_buf[10] = (mc_local_srcid >> 8) & 0xff;
        emit_buf[11] =  mc_local_srcid & 0xff;
                                       
/* payload                             
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             url_id            |           object_id           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             offset                            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*/                                     
                                       
        emit_buf[12] = ( p->url_id >> 8) & 0xff;        /* url_id */
        emit_buf[13] = p->url_id & 0xff;
                                       
        emit_buf[14] = (p->o_id >> 8) & 0xff;   /* object_id */
        emit_buf[15] = p->o_id & 0xff; 
                                       
        emit_buf[16] = ( p->offset >> 24) & 0xff;       /* offset */
        emit_buf[17] = ( p->offset >> 16) & 0xff;
        emit_buf[18] = ( p->offset >> 8) & 0xff;
        emit_buf[19] = p->offset & 0xff;
                                       
        len_buf = 20 + p->d_len;       
        memcpy(&emit_buf[20], p->d, p->d_len);
	fprintf(stderr, "McSendRtpDataTimeOutCb: m=%d, seq=%d, ts=%u, srcid=%u, url_id=%u, o_id=%u, offset=%u, d_len=%u\n",
		p->is_end, mc_seq_number, p->rtp_ts, mc_local_srcid,
		p->url_id, p->o_id, p->offset,p->d_len);
        cnt = McWrite(mc_fd_rtp_w, emit_buf, len_buf);  
	if (p->is_end) {
        	mc_state_report_url_id = p->url_id;
        	mc_state_report_o_id = p->o_id;
        	mc_state_report_len = p->offset + p->d_len;
	}

        mc_rtp_packets_list = p->next; 
        mc_write_rtp_data_next_time = p->duration; 
        free(p);                       
        if( !mc_rtp_packets_list)      
                return;                
/* rearme a timer for the next packet */
        mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
                mc_write_rtp_data_next_time, McSendRtpDataTimeOutCb,
                NULL);                 
} 

void UcRtpSendDataPacket(Source *s, RtpPacket *p)
{
        int len_buf;
        int next_time;
	int cnt;
	struct sockaddr_in addr_w;

        memset(&addr_w,0,sizeof(addr_w));
        addr_w.sin_family = AF_INET;
        addr_w.sin_port = s->uc_rtp_port;  /* net byteorder */
        addr_w.sin_addr.s_addr = s->uc_rtp_ipaddr; /* net byteorder */
 
        emit_buf[0] = 0x80;     /* V,P,CC */
        emit_buf[1] = p->is_end | 0x62;        /* M, PT=98 */
                                       
        emit_buf[2] = (0 >> 8) & 0xff;      /* sequence number */
        emit_buf[3] = 0 & 0xff; 
  /*      mc_seq_number++;                */
                                       
        emit_buf[4] = (p->rtp_ts >> 24) & 0xff;    /* timestamp */
        emit_buf[5] = (p->rtp_ts >> 16) & 0xff;
        emit_buf[6] = (p->rtp_ts >> 8) & 0xff;
        emit_buf[7] =  p->rtp_ts & 0xff;

        emit_buf[8] = (mc_local_srcid >> 24) & 0xff;    /* SSRC */
        emit_buf[9] = (mc_local_srcid >> 16) & 0xff;
        emit_buf[10] = (mc_local_srcid >> 8) & 0xff;
        emit_buf[11] =  mc_local_srcid & 0xff;
                                       
/* payload                             
    0                   1                   2                   3
    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |             url_id            |           object_id           |
   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
   |                             offset                            |
   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
*/                                     
                                       
        emit_buf[12] = ( p->url_id >> 8) & 0xff;        /* url_id */
        emit_buf[13] = p->url_id & 0xff;
                                       
        emit_buf[14] = (p->o_id >> 8) & 0xff;   /* object_id */
        emit_buf[15] = p->o_id & 0xff; 
                                       
        emit_buf[16] = ( p->offset >> 24) & 0xff;       /* offset */
        emit_buf[17] = ( p->offset >> 16) & 0xff;
        emit_buf[18] = ( p->offset >> 8) & 0xff;
        emit_buf[19] = p->offset & 0xff;
                                       
        len_buf = 20 + p->d_len;       
        memcpy(&emit_buf[20], p->d, p->d_len);
	fprintf(stderr, "UcRtpSendDataPacket: m=%d, seq=%d, ts=%u, srcid=%u, url_id=%u, o_id=%u, offset=%u, d_len=%u\n",
		p->is_end, mc_seq_number, p->rtp_ts, mc_local_srcid,
		p->url_id, p->o_id, p->offset,p->d_len);

	cnt = sendto(uc_fd_rtp_w, (char*)emit_buf, len_buf, 0,
		(struct sockaddr *) &addr_w, sizeof(addr_w));
        if(cnt != len_buf ){
		perror("UcRtpSendDataPacket:sendto:");
	}
/*      cnt = McWrite(mc_fd_rtp_w, emit_buf, len_buf);  
/*        mc_rtp_packets_list = p->next; 
/*        mc_write_rtp_data_next_time = p->duration; 
/*        free(p);                       
/*        if( !mc_rtp_packets_list)      
/*                return;                
/* rearme a timer for the next packet */
/*        mc_write_rtp_data_timer_id = XtAppAddTimeOut( mMosaicAppContext,
/*                mc_write_rtp_data_next_time, McSendRtpDataTimeOutCb,
/*                NULL);                 
*/
} 


/* return 1 good or 0 if probleme */

int DewrapRtcpData( unsigned char *buf, int len_buf,
	RtcpPacket *rcs_ret)
{
	unsigned int rh_flags;
	unsigned int i;
	unsigned char * p = buf;
	int len8 = 0;

	if ( len_buf < 8 ){ 
		fprintf(stderr,"Error receiving Rtcp data: n = %d\n", len_buf);
		return 0;
	}
			/* V:2 P:1 RC:5 PT:8 */
	if ( p[0] != 0x80){
		return 0;
	}
	rcs_ret->pt = p[1];
	rcs_ret->len =  (u_int16_t)( ((u_int16_t) p[2] << 8) |
				     ((u_int16_t) p[3]      ));

	len8 = (rcs_ret->len + 1) * 4;
	if ( len8 > len_buf)
		return 0;
	rcs_ret->ssrc  =   (u_int32_t)( ((u_int32_t) p[4] << 24) |
				     ((u_int32_t) p[5] << 16) |
				     ((u_int32_t) p[6] << 8 ) |
				     ((u_int32_t) p[7]      ) );
	rcs_ret->d = (char*) &buf[8];
	rcs_ret->d_len = len8 - 8;
	return len8;
}
#endif /* MULTICAST */
