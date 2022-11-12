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
#include "../src/mo-www.h"

#include "mc_rtp.h"
#include "mc_defs.h"
#include "mc_misc.h"
#include "mc_sockio.h"
#include "mc_dispatch.h"

IPAddr		mc_local_ip_addr;

#ifdef IPV6 
static const IPAddr anyaddr = IPV6ADDR_ANY_INIT;
#endif  
 
#ifdef IPV6

static struct sockaddr_in6 addr_r;
static struct sockaddr_in6 rtcp_addr_r;
static struct sockaddr_in6 addr_w;
static struct sockaddr_in6 rtcp_addr_w;
#else

static struct sockaddr_in addr_r;
static struct sockaddr_in rtcp_addr_r;
static struct sockaddr_in addr_w;
static struct sockaddr_in rtcp_addr_w;
#endif

static int addr_r_len;
static int rtcp_addr_r_len;
static unsigned char emit_buf[MC_MAX_BUF_SIZE];
static unsigned char recv_buf[MC_MAX_BUF_SIZE];
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

int McOpenRead (IPAddr ip,unsigned short port,unsigned char ttl)
{
#ifdef IPV6
        struct ipv6_mreq mreq;
	struct sockaddr_in6 fakesockaddr;
#else
        struct ip_mreq mreq;
	struct sockaddr_in fakesockaddr;
#endif

        int one = 1;
	int fd=-1;
	char hostname[MAXHOSTNAMELEN];
	struct hostent *hp;
	int sd_len;
	int fakesock;

#ifdef IPV6
        if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) <0 ) {
#else
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) <0 ) {
#endif
                perror ("McOpenRead: socket");
                exit(1);
        }
        /* so that more than one process can bind to the same
           SOCK_DGRAM UDP port ( must be placed BEFORE bind() ) */
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one))){
		perror ("McOpenRead: setsockopt SO_REUSEADDR");
		exit(1);
	}
	addr_r_len = sizeof(addr_r);
        memset(&addr_r,0,sizeof(addr_r));
#ifdef IPV6
        addr_r.sin6_family = AF_INET6;
        addr_r.sin6_port = port;
        addr_r.sin6_addr = anyaddr;
#else
        addr_r.sin_family = AF_INET;
        /*addr_r.sin_port = htons(port);*/
        addr_r.sin_port = port;
        addr_r.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
        if(bind(fd, (struct sockaddr *)&addr_r, sizeof(addr_r)) <0) {
                perror ("McOpenRead: bind:");
                exit(1);
        }
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
                perror ("McOpenRead: setsockopt IP_ADD_MEMBERSHIP:");
                exit(1);
        }

#ifdef IPV6
        /* This bogosity is to find the IP address of the local host! */
        if ((fakesock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("McOpenRead: gethostname " );
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
        mc_local_ip_addr = fakesockaddr.sin6_addr;
        close(fakesock);
#else
        /* This bogosity is to find the IP address of the local host! */
        if ((fakesock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("McOpenRead: gethostname " );
                exit(1);
        }
        hp = gethostbyname(hostname);
        memcpy((char *)&fakesockaddr.sin_addr, hp->h_addr, hp->h_length);
        fakesockaddr.sin_port = htons(0);
        bind(fakesock, (struct sockaddr *) &fakesockaddr, sizeof(fakesockaddr));
        sd_len = sizeof(fakesockaddr);
        getsockname(fakesock, (struct sockaddr *)&fakesockaddr, &sd_len);
        mc_local_ip_addr = ntohl(fakesockaddr.sin_addr.s_addr);
        close(fakesock);
#endif

	return fd;
}

int McOpenRtcpRead (IPAddr ip,unsigned short port,unsigned char ttl)
{
#ifdef IPV6     
        struct ipv6_mreq mreq;
        struct sockaddr_in6 fakesockaddr;
#else
        struct ip_mreq mreq;
        struct sockaddr_in fakesockaddr;
#endif 
        int one = 1;
	int fd=-1;
	char hostname[MAXHOSTNAMELEN];
	struct hostent *hp;
	int sd_len;
	int fakesock;

#ifdef IPV6
        if ((fd = socket(AF_INET6, SOCK_DGRAM, 0)) <0 ) {
#else                                 
        if ((fd = socket(AF_INET, SOCK_DGRAM, 0)) <0 ) {
#endif
                perror ("McOpenRtcpRead: socket");
                exit(1);
        }
        /* so that more than one process can bind to the same
           SOCK_DGRAM UDP port ( must be placed BEFORE bind() ) */
        if(setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, (char*)&one, sizeof(one))){
		perror ("McOpenRtcpRead: setsockopt SO_REUSEADDR");
		exit(1);
	}
	rtcp_addr_r_len = sizeof(rtcp_addr_r);
        memset(&rtcp_addr_r,0,sizeof(rtcp_addr_r));
#ifdef IPV6                           
        rtcp_addr_r.sin6_family = AF_INET6;
        rtcp_addr_r.sin6_port = port;      
        rtcp_addr_r.sin6_addr = anyaddr;   
#else                                 
        rtcp_addr_r.sin_family = AF_INET;
        /*addr_r.sin_port = htons(port);*/
        rtcp_addr_r.sin_port = port;
        rtcp_addr_r.sin_addr.s_addr = htonl(INADDR_ANY);
#endif
        if(bind(fd, (struct sockaddr *)&rtcp_addr_r, sizeof(rtcp_addr_r)) <0) {
                perror ("McOpenrtcp_Read: bind:");
                exit(1);
        }
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
                perror ("McOpenrtcp_Read: setsockopt IP_ADD_MEMBERSHIP:");
                exit(1);
        }

#ifdef IPV6                           
        /* This bogosity is to find the IP address of the local host! */
        if ((fakesock = socket(AF_INET6, SOCK_DGRAM, 0)) < 0) {
                perror("socket");     
                exit(1);              
        }                             
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("McOpenRead: gethostname " );
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
        mc_local_ip_addr = fakesockaddr.sin6_addr;
        close(fakesock);              
#else
        /* This bogosity is to find the IP address of the local host! */
        if ((fakesock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
                perror("socket");
                exit(1);
        }
        if ( gethostname(hostname, MAXHOSTNAMELEN) != 0) {
                perror("McOpenrtcp_Read: gethostname " );
                exit(1);
        }
        hp = gethostbyname(hostname);
        memcpy((char *)&fakesockaddr.sin_addr, hp->h_addr, hp->h_length);
        fakesockaddr.sin_port = htons(0);
        bind(fakesock, (struct sockaddr *) &fakesockaddr, sizeof(fakesockaddr));
        sd_len = sizeof(fakesockaddr);
        getsockname(fakesock, (struct sockaddr *)&fakesockaddr, &sd_len);
        mc_local_ip_addr = ntohl(fakesockaddr.sin_addr.s_addr);
        close(fakesock);
#endif

	return fd;
}

int McOpenWrite(IPAddr ip,unsigned short port,unsigned char ttl)
{
	int fd;
	unsigned int t6 = ttl;
 
#ifdef IPV6
	fd = socket(AF_INET6, SOCK_DGRAM, 0);
#else
	fd = socket(AF_INET, SOCK_DGRAM, 0);
#endif
	if(fd<0) {
		perror("McOpenWrite: socket");
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
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		       (char*)&(ttl), sizeof(ttl)) < 0) {
		perror("McOpenWrite:setsockopt IP_MULTICAST_TTL:");
		exit(1);
	}
	memset(&addr_w,0,sizeof(addr_w));
	addr_w.sin_family = AF_INET;
	addr_w.sin_port = port;
	addr_w.sin_addr.s_addr = ip;
#endif
	return fd;
}

int McOpenRtcpWrite(IPAddr ip,unsigned short port,unsigned char ttl)
{
	int fd;
	unsigned int t6 = ttl; 
#ifdef IPV6
	fd = socket(AF_INET6, SOCK_DGRAM, 0);
#else
	fd = socket(PF_INET, SOCK_DGRAM, 0);
#endif
	if(fd<0) {
		perror("McOpenRtcpWrite: socket");
		exit(1);
	}  
#ifdef IPV6
	if (setsockopt(fd, IPPROTO_IPV6, IPV6_MULTICAST_HOPS,
		       (char*)&(t6), sizeof(t6)) < 0) {
		perror("McOpenRtcpWrite:setsockopt IP_MULTICAST_HOPS:");
		exit(1);
	}
	memset(&rtcp_addr_w,0,sizeof(rtcp_addr_w));
	rtcp_addr_w.sin6_family = AF_INET6;
	rtcp_addr_w.sin6_port = port;
	rtcp_addr_w.sin6_addr = ip;
#else
	if (setsockopt(fd, IPPROTO_IP, IP_MULTICAST_TTL,
		       (char*)&(ttl), sizeof(ttl)) < 0) {
		perror("McOpenRtcpWrite:setsockopt IP_MULTICAST_TTL:");
		exit(1);
	}
	memset(&rtcp_addr_w,0,sizeof(rtcp_addr_w));
	rtcp_addr_w.sin_family = AF_INET;
	rtcp_addr_w.sin_port = port;
	rtcp_addr_w.sin_addr.s_addr = ip;
#endif
	return fd;
}

/*
 * return number of byte read and the static recv_buf 
 */
int McGetRecvBuf(unsigned char ** buf, IPAddr * ipfrom)
{
	int cnt;
  
	cnt = recvfrom(mc_fdread, (char*)recv_buf, MC_MAX_BUF_SIZE,0, 
			(struct sockaddr *)&addr_r, &addr_r_len);
	if (cnt <= 0 ) {
		printf("error in McGetRecvBuf\n");
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
	printf("McGetRecvBuf: cnt=%d, recv_buf[12]=%ud\n",cnt,recv_buf[12]);
#endif
	return cnt;
}

/* return the number of byte read or 0 if probleme */

int McCheckCursorPos( unsigned char *buf, int len_buf, McRtpCursorPosDataStruct *cp, IPAddr ipfrom)
{
	unsigned int i;
	unsigned char * p = buf;

/*################## this code need to be revisited #### unuse padding*/
	if ( len_buf < (17+3) ){ 
		fprintf(stderr,"Error receiving CURSOR_POS: n = %d\n", len_buf);
		return 0;
	}

			/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	cp->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                                ((unsigned long) p[1]      ) );

	if (cp->rh_flags != (u_int16_t) RTP_CONST_HPT_WEB){
		fprintf(stderr,"Error receiving CURSOR_POS: proto error\n");
		return 0;
	}
	cp->url_id =  (u_int32_t) (	((unsigned long) p[9] << 16)|
						((unsigned long) p[2] << 8) |
						((unsigned long) p[3]      ));
	cp->gmt_send_time=(u_int32_t)(     ((unsigned long) p[4] << 24) |
					        ((unsigned long) p[5] << 16) |
					        ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	cp->ipaddr = 	ipfrom;

	cp->ssrc = (u_int8_t)	p[8];
	cp->pid =  (u_int16_t)(            ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]      ) );
	cp->code = (u_int8_t)	p[12];

	cp->posx = (int16_t) (			((unsigned long) p[13] << 8) |
					        ((unsigned long) p[14] ) );
	cp->posy = (int16_t) (			((unsigned long) p[15] << 8) |
					        ((unsigned long) p[16] ) );
 
/*################## this code need to be revisited #### unuse padding*/
	return 17 +3;
}

/* return the number of byte read or 0 if probleme */

int McCheckGotoId( unsigned char *buf, int len_buf, McRtpGotoIdDataStruct *hgid,
		IPAddr ipfrom)
{
	unsigned int i;
	unsigned char * p = buf;

/*################## this code need to be revisited #### unuse padding*/
	if ( len_buf < (16 + 4) ){ 
		fprintf(stderr,"Error receiving GOTO_ID: n = %d\n", len_buf);
		return 0;
	}
	
			/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	hgid->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                                ((unsigned long) p[1]      ) );

	if (hgid->rh_flags != (u_int16_t) RTP_CONST_HPT_WEB){
		fprintf(stderr,"Error receiving GOTO_ID: proto error\n");
		return 0;
	}
	hgid->url_id =  (u_int32_t) (	((unsigned long) p[9] << 16)|
						((unsigned long) p[2] << 8) |
						((unsigned long) p[3]      ));
	hgid->gmt_send_time=(u_int32_t)(     ((unsigned long) p[4] << 24) |
					        ((unsigned long) p[5] << 16) |
					        ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	hgid->ipaddr = ipfrom;

	hgid->ssrc = (u_int8_t)	p[8];
	hgid->pid =  (u_int16_t)(            ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]      ) );
	hgid->code = (u_int8_t)	p[12];

	hgid->html_goto_id = (u_int32_t) (	((unsigned long) p[13] << 16) |
					        ((unsigned long) p[14] << 8 ) |
					        ((unsigned long) p[15]      ) );
 
	return 16 + 4 ;
}

/* return the number of byte read or 0 if probleme */

int McCheckAllData( unsigned char *buf, int len_buf, Mcs_alldata *alldata,
		IPAddr ipfrom)
{
	unsigned int i;
	unsigned char * p = buf;

	if ( len_buf < 24 ){ 
		fprintf(stderr,"Error receiving ALLDATA: n = %d\n", len_buf);
		return 0;
	}
	
			/* T:2 P:1 X:1 CC:4 M:1 PT:7 */
	alldata->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                                ((unsigned long) p[1]      ) );

	if (alldata->rh_flags != (u_int16_t) RTP_CONST_HPT_WEB){
		fprintf(stderr,"Error receiving ALLDATA: proto error\n");
		return 0;
	}
	alldata->url_id =  (u_int32_t) (	((unsigned long) p[9] << 16)|
						((unsigned long) p[2] << 8) |
						((unsigned long) p[3]      ));
	alldata->gmt_send_time=(u_int32_t)(     ((unsigned long) p[4] << 24) |
					        ((unsigned long) p[5] << 16) |
					        ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	alldata->ipaddr = ipfrom;

	alldata->ssrc = (u_int8_t)	p[8];
	alldata->pid =  (u_int16_t)(            ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]      ) );
	alldata->code = (u_int8_t)	p[12];

	alldata->seo = (u_int32_t) (		((unsigned long) p[13] << 16) |
					        ((unsigned long) p[14] << 8 ) |
					        ((unsigned long) p[15]      ) );
	alldata->nombre_eo = (u_int16_t) ( 	((unsigned long) p[16] << 8 ) |
					        ((unsigned long) p[17]      ) );
	alldata->num_eo = (u_int16_t) (		((unsigned long) p[18] << 8 ) |
					        ((unsigned long) p[19]      ) );
	alldata->nombre_packet=(u_int16_t)(	((unsigned long) p[20] << 8 ) |
					        ((unsigned long) p[21]      ) );
	alldata->num_packet= (u_int16_t ) (   	((unsigned long) p[22] << 8 ) |
					        ((unsigned long) p[23]      ) );
	p = &buf[24];
	alldata->len_data = len_buf - 24;

	if (alldata->len_data > MC_PACKET_SIZE){
		fprintf(stderr,"Error receiving ALLDATA: len_data = %d\n",
					alldata->len_data);
		return 0;
	}
        for (i = 0;i< alldata->len_data; i++)
                alldata->data[i] = *p++;
        alldata->data[alldata->len_data] = '\0'; /* juste if it is texte */
 
        if (mc_debug)
                printf("McCheckAllData: len_data = %d\n",alldata->len_data);

	return p-buf;
}


void McSendData( unsigned char * buf, int len)
{
	int cnt;

	cnt = sendto(mc_fdwrite, (char*)buf, len, 0,
			(struct sockaddr *)&addr_w, sizeof(addr_w));
	if(cnt != len){
		perror("McSendData:sendto:");
	}
/*	mc_send_cnt++; */
}

void McSendRtpCursorPos( unsigned char code, unsigned short pid, 
	unsigned int url_id, unsigned int gmt_send_time,
	int x, int y)
{
	unsigned int len_buf;

	emit_buf[0] = (RTP_CONST_HPT_WEB >> 8) & 0xff;	/* web header*/
	emit_buf[1] = RTP_CONST_HPT_WEB & 0xff;

	emit_buf[9] = (url_id >> 16) & 0xff;		/* url_id */
	emit_buf[2] = (url_id >> 8) & 0xff;
	emit_buf[3] =  url_id & 0xff;

	emit_buf[4] =  (gmt_send_time >> 24) & 0xff;    /* MSB gmt_send_time */
	emit_buf[5] = (gmt_send_time >> 16) & 0xff;
	emit_buf[6] = (gmt_send_time >> 8) & 0xff;
	emit_buf[7] =  gmt_send_time & 0xff;

	emit_buf[8] = 0x01;

	emit_buf[10] = (pid >> 8) & 0xff; 	/* MSB pid */
	emit_buf[11] = pid & 0xff;

	emit_buf[12] = code ;			/* code */

	emit_buf[13] = (x >> 8) & 0xff; 
	emit_buf[14] = x & 0xff;
	emit_buf[15] = (y >> 8) & 0xff; 
	emit_buf[16] = y & 0xff;

/*################## this code need to be revisited #### unuse padding*/
	emit_buf[17] = emit_buf[18] = emit_buf[19] =0;
	len_buf = 17 + 3;
        McSendData(emit_buf, len_buf);
}

void McSendRtpGotoId( unsigned char code, unsigned short pid, 
	unsigned int url_id, unsigned int gmt_send_time,
	unsigned int html_goto_id)
{
	unsigned int len_buf;

	emit_buf[0] = (RTP_CONST_HPT_WEB >> 8) & 0xff;	/* web header*/
	emit_buf[1] = RTP_CONST_HPT_WEB & 0xff;

	emit_buf[9] = (url_id >> 16) & 0xff;		/* url_id */
	emit_buf[2] = (url_id >> 8) & 0xff;
	emit_buf[3] =  url_id & 0xff;

	emit_buf[4] =  (gmt_send_time >> 24) & 0xff;    /* MSB gmt_send_time */
	emit_buf[5] = (gmt_send_time >> 16) & 0xff;
	emit_buf[6] = (gmt_send_time >> 8) & 0xff;
	emit_buf[7] =  gmt_send_time & 0xff;

	emit_buf[8] = 0x01;

	emit_buf[10] = (pid >> 8) & 0xff; 	/* MSB pid */
	emit_buf[11] = pid & 0xff;

	emit_buf[12] = code ;			/* code */

	emit_buf[13] = (html_goto_id >> 16) & 0xff; 
	emit_buf[14] = (html_goto_id >> 8) & 0xff; 
	emit_buf[15] = html_goto_id & 0xff;

/*################## this code need to be revisited #### unuse padding*/
	emit_buf[16] = emit_buf[17] = emit_buf[18] = emit_buf[19] =0;
	len_buf = 16 + 4;
        McSendData(emit_buf, len_buf);
}

void McSendPacket( unsigned char code,
	unsigned short pid, unsigned int url_id, unsigned int gmt_send_time,
	unsigned int nombre_eo,
	unsigned int num_eo , unsigned int seo , unsigned int nombre_packet ,
	unsigned int num_packet , char * data, unsigned int len )
{
	unsigned int len_buf;

	emit_buf[0] = (RTP_CONST_HPT_WEB >> 8) & 0xff;	/* web header*/
	emit_buf[1] = RTP_CONST_HPT_WEB & 0xff;

	emit_buf[9] = (url_id >> 16) & 0xff;		/* url_id */
	emit_buf[2] = (url_id >> 8) & 0xff;
	emit_buf[3] =  url_id & 0xff;

	emit_buf[4] =  (gmt_send_time >> 24) & 0xff;    /* MSB gmt_send_time */
	emit_buf[5] = (gmt_send_time >> 16) & 0xff;
	emit_buf[6] = (gmt_send_time >> 8) & 0xff;
	emit_buf[7] =  gmt_send_time & 0xff;

	emit_buf[8] = 0x01;

	emit_buf[10] = (pid >> 8) & 0xff; 	/* MSB pid */
	emit_buf[11] = pid & 0xff;

	emit_buf[12] = code ;			/* code */

	emit_buf[13] = (seo >> 16) & 0xff; 
	emit_buf[14] = (seo >> 8) & 0xff; 
	emit_buf[15] = seo & 0xff;

	emit_buf[16] = (nombre_eo >> 8) & 0xff; 
	emit_buf[17] = nombre_eo & 0xff;

	emit_buf[18] = (num_eo >> 8) & 0xff; 
	emit_buf[19] = num_eo & 0xff;

	emit_buf[20] = (nombre_packet >> 8) & 0xff; 
	emit_buf[21] = nombre_packet & 0xff;

	emit_buf[22] = (num_packet >> 8) & 0xff; 
	emit_buf[23] = num_packet & 0xff;

	len_buf = 24 + len;
	memcpy(&emit_buf[24], data, len);
        McSendData(emit_buf, len_buf);
}

	/*d->id = ??? id de l'url ;     unsigned int     */
        /*d->url = l'url;               char *          */
        /*d->text = l'hypertext;        char *          */
        /*d->neo = nombre d'objet;      unsigned int     */
        /*d->seo = tableau de taile d'objet; unsigned int *      */
        /*d->eos= tableau d'objet       char ** ou d->eos[i] = ???       */
                                /* eo[neo]. Each element is data of eo*/
                                /* but :                                */
                                /*    eo[0] <=> URL                     */
                                /*    eo[1] <=> text                    */
 
void McSendRtcpData( unsigned char * buf, int len)
{
	int cnt;

	cnt = sendto(mc_rtcp_fdwrite, (char*)buf, len, 0,
			(struct sockaddr *)&rtcp_addr_w, sizeof(rtcp_addr_w));
	if(cnt != len){
		perror("McSendRtcpData:sendto:");
	}
}

/* Old NACK_ALL packet format(obsolete):
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
 * Il manque tout l'objet num_eo
 */
/* NACK_ALL packet format:            
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|ST=NACK_A| PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |  ssrc         |    0          |       pid                     |
 * |===============================================================|
 * | sender ssrc   |        hurlid |  sender pid                   |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              lurl_id          |           num_eo              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        first sender ipaddr [0]                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        first sender ipaddr [1]                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        first sender ipaddr [2]                |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |                        first sender ipaddr [3] (IPv4)         |
 * |===============================================================|
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * Il manque tout l'objet num_eo
 * ipaddr is 128 bits wide for IPv6 
 * ipaddr[0..2] = 0 if IPv4 
 */

void McSendRtcpNackAll( IPAddr uip_addr, unsigned short upid, 
			unsigned int uurl_id, unsigned short num_eo,
			unsigned int ussrc)
{
	unsigned int len_buf;

	emit_buf[0] = 0x80 + LRMP_NACK_ALL;	/* V=2 p=0 SC=LRMP_NACK_ALL */
	emit_buf[1] = (unsigned char) RTCP_PT_LRMP;

	emit_buf[4] = 0x01;
	emit_buf[5] = 0x00; 

	emit_buf[6] = (mc_my_pid >> 8) & 0xff; 
	emit_buf[7] = mc_my_pid & 0xff;		/* LSB my_pid */

	emit_buf[8] = (u_int8_t)ussrc;

	emit_buf[10] = (upid >> 8) & 0xff; 
	emit_buf[11] = upid & 0xff;		/* LSB pid */

	emit_buf[9] = (uurl_id >> 16) & 0xff; 
	emit_buf[12] = (uurl_id >> 8) & 0xff; 
	emit_buf[13] = uurl_id & 0xff;		/* LSB  */

	emit_buf[14] = (num_eo >> 8) & 0xff; 
	emit_buf[15] = num_eo & 0xff;		/* LSB  */

#ifdef IPV6
	emit_buf[16] = uip_addr.s6_addr[0];
	emit_buf[17] = uip_addr.s6_addr[1];
	emit_buf[18] = uip_addr.s6_addr[2];
	emit_buf[19] = uip_addr.s6_addr[3];
	emit_buf[20] = uip_addr.s6_addr[4];
	emit_buf[21] = uip_addr.s6_addr[5];
	emit_buf[22] = uip_addr.s6_addr[6];
	emit_buf[23] = uip_addr.s6_addr[7];
	emit_buf[24] = uip_addr.s6_addr[8];
	emit_buf[25] = uip_addr.s6_addr[9];
	emit_buf[26] = uip_addr.s6_addr[10];
	emit_buf[27] = uip_addr.s6_addr[11];

	emit_buf[28] = uip_addr.s6_addr[12];
	emit_buf[29] = uip_addr.s6_addr[13];
	emit_buf[30] = uip_addr.s6_addr[14];
	emit_buf[31] = uip_addr.s6_addr[15];
#else
	emit_buf[16] = 0;
	emit_buf[17] = 0;
	emit_buf[18] = 0;
	emit_buf[19] = 0;
	emit_buf[20] = 0;
	emit_buf[21] = 0;
	emit_buf[22] = 0;
	emit_buf[23] = 0;
	emit_buf[24] = 0;
	emit_buf[25] = 0;
	emit_buf[26] = 0;
	emit_buf[27] = 0;

	emit_buf[28] = (uip_addr >> 24) & 0xff;	/* MSB ip_addr */
	emit_buf[29] = (uip_addr >> 16) & 0xff; 
	emit_buf[30] = (uip_addr >> 8) & 0xff; 
	emit_buf[31] = uip_addr & 0xff;		/* LSB  */

#endif
        len_buf = 32 ;
        McSendRtcpData(emit_buf, len_buf);
}

/* return the number of byte read or 0 if probleme */

int McCheckRtcpLrmpNackAll( unsigned char *buf, int len_buf,
	McRtcpLrmpNackAllDataStruct *rlnad, IPAddr ipfrom)
{
	unsigned char * p = buf;

	if ( len_buf < 32 ){ 
		fprintf(stderr,"Error McCheckRtcpLrmpNackAll: n = %d\n", len_buf);
		return 0;
	}
			/* T:2 P:1 SC:LRMP_NACK_ALL PT:RTCP_PT_LRMP */
	rlnad->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                        ((unsigned long) p[1]      ) );
	if (rlnad->rh_flags != (u_int16_t) (0x8500 + RTCP_PT_LRMP)){
		fprintf(stderr,"Error receiving rlnad: proto error\n");
		return 0;
	}
	rlnad->ipaddr = ipfrom;
	rlnad->pid =  (u_int16_t)(              ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	rlnad->s_ssrc = p[8];
	rlnad->s_pid = (u_int16_t)(            ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]     ) );
	rlnad->url_id = (u_int32_t)	(       ((unsigned long) p[9] << 16) |
					        ((unsigned long) p[12] << 8 ) |
					        ((unsigned long) p[13]      ) );
	rlnad->num_eo = (u_int16_t)(            ((unsigned long) p[14] << 8 ) |
					        ((unsigned long) p[15]     ) );
	rlnad->s_ipaddr.s6_addr[0] = p[16];
	rlnad->s_ipaddr.s6_addr[1] = p[17];
	rlnad->s_ipaddr.s6_addr[2] = p[18];
	rlnad->s_ipaddr.s6_addr[3] = p[19];
	rlnad->s_ipaddr.s6_addr[4] = p[20];
	rlnad->s_ipaddr.s6_addr[5] = p[21];
	rlnad->s_ipaddr.s6_addr[6] = p[22];
	rlnad->s_ipaddr.s6_addr[7] = p[23];
	rlnad->s_ipaddr.s6_addr[8] = p[24];
	rlnad->s_ipaddr.s6_addr[9] = p[25];
	rlnad->s_ipaddr.s6_addr[10] = p[26];
	rlnad->s_ipaddr.s6_addr[11] = p[27];
	rlnad->s_ipaddr.s6_addr[12] = p[28];
	rlnad->s_ipaddr.s6_addr[13] = p[29];
	rlnad->s_ipaddr.s6_addr[14] = p[30];
	rlnad->s_ipaddr.s6_addr[15] = p[31];

/*
	rlnad->s_ipaddr = (u_int32_t)	(       ((unsigned long) p[8] << 24) |
					        ((unsigned long) p[9] << 16) |
					        ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]      ) );
*/
	p = &buf[32];
	return p-buf;
}

static IPAddr current_nack_uip_addr;
static unsigned int current_nack_upid;
static unsigned int current_nack_uurl_id;
static unsigned int current_nack_num_eo;
static unsigned int current_nack_ussrc;
static int packet_tab[17];
static int nombre_packet=0;

void McInitNackPacketData(IPAddr uip_addr, unsigned short upid,
			unsigned int uurl_id, unsigned short num_eo,
			unsigned int ussrc)
{
	int i;

	current_nack_uip_addr = uip_addr;
	current_nack_upid = upid;
	current_nack_uurl_id = uurl_id;
	current_nack_num_eo = num_eo;
	current_nack_ussrc = ussrc;
	nombre_packet = 0;
	for(i=0; i<17; i++)
		packet_tab[i] = -1;
}

void McPushNackPacketData(IPAddr  uip_addr, unsigned short upid,
			unsigned int uurl_id, unsigned short num_eo,
			unsigned short num_pkt, unsigned int ussrc)
{
	if ( nombre_packet >= 17)
		McFlushNackPacketData();
	packet_tab[nombre_packet] = num_pkt;
	nombre_packet++;
}
void McFlushNackPacketData(void)
{
	int i,j,n;
	unsigned int fsl, blp;

	if(nombre_packet == 0)
		return;
	if (nombre_packet == 1) {	/* just one */
		McSendRtcpNack(current_nack_uip_addr, current_nack_upid,
			current_nack_uurl_id, current_nack_num_eo,
		 	(unsigned short)(0xffff & packet_tab[0]), 0 ,
			current_nack_ussrc);
		nombre_packet = 0;
		packet_tab[0] = -1;
		return;
	}
/* more than one */
	fsl = packet_tab[0];
	n = 1;
	blp = 0;
	for(i = 1; i< nombre_packet; i++){ /* flush the stack. Emit Nack*/
		if ( (packet_tab[i] - fsl) > 16) { /* ca rentre pas */
			int k = 0;

			McSendRtcpNack(current_nack_uip_addr, current_nack_upid,
			         current_nack_uurl_id, current_nack_num_eo,
		 	         (unsigned short)(0xffff & fsl), blp ,
			         current_nack_ussrc);
			/* packet_tab[0] = packet_tab[i]; */
			for(j = i ; j < nombre_packet; j++){
				packet_tab[k] = packet_tab[j];
				k++;
			}
			nombre_packet = k;
			McFlushNackPacketData();
			return;
		}
/* ca rentre */
		blp = blp | (0x0001 << (i-1));
		n++;
	}
	if (blp){
		McSendRtcpNack(current_nack_uip_addr, current_nack_upid,
		         current_nack_uurl_id, current_nack_num_eo,
		         (unsigned short)(0xffff & fsl), blp ,
		         current_nack_ussrc);
	}
}
			
		

/* NACK packet format:
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P| ST=NACK | PT=RTCP_LRMP  |           length              |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              idem NACK_ALL    ...                             |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |              FSL              |              BLP              |
 * |===============================================================|
 * |                         (next SSRC ...)                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * FSL: first seqno lost for this num_eo. (First packet number for this num_eo)
 * BLP: bitmask of the following lost packets.
 */

void McSendRtcpNack( IPAddr  uip_addr, unsigned short upid, 
			unsigned int uurl_id, unsigned short num_eo,
			unsigned short num_pkt, unsigned short blp,
			unsigned int ussrc)
{
	unsigned int len_buf;

	emit_buf[0] = 0x80 + LRMP_NACK ;	/* V=2 p=0 SC=LRMP_NACK */
	emit_buf[1] = (unsigned char) RTCP_PT_LRMP;
	emit_buf[4] = 0x01;
	emit_buf[5] = 0x00; 

	emit_buf[6] = (mc_my_pid >> 8) & 0xff; 
	emit_buf[7] = mc_my_pid & 0xff;		/* LSB my_pid */

	emit_buf[8] = (u_int8_t) ussrc;

	emit_buf[10] = (upid >> 8) & 0xff; 
	emit_buf[11] = upid & 0xff;		/* LSB pid */

	emit_buf[9] = (uurl_id >> 16) & 0xff; 
	emit_buf[12] = (uurl_id >> 8) & 0xff; 
	emit_buf[13] = uurl_id & 0xff;		/* LSB  */

	emit_buf[14] = (num_eo >> 8) & 0xff; 
	emit_buf[15] = num_eo & 0xff;		/* LSB  */

	emit_buf[16] = (num_pkt >> 8) & 0xff; 
	emit_buf[17] = num_pkt & 0xff;		/* LSB  */

	emit_buf[18] = (blp >> 8) & 0xff; 	/* bitwise */
	emit_buf[19] = blp & 0xff;		/* LSB  */

#ifdef IPV6
        emit_buf[20] = uip_addr.s6_addr[0];
        emit_buf[21] = uip_addr.s6_addr[1];
        emit_buf[22] = uip_addr.s6_addr[2];
        emit_buf[23] = uip_addr.s6_addr[3];
        emit_buf[24] = uip_addr.s6_addr[4];
        emit_buf[25] = uip_addr.s6_addr[5];
        emit_buf[26] = uip_addr.s6_addr[6];
        emit_buf[27] = uip_addr.s6_addr[7];
        emit_buf[28] = uip_addr.s6_addr[8];
        emit_buf[29] = uip_addr.s6_addr[9]; 
        emit_buf[30] = uip_addr.s6_addr[10];
        emit_buf[31] = uip_addr.s6_addr[11];
                                      
        emit_buf[32] = uip_addr.s6_addr[12];
        emit_buf[33] = uip_addr.s6_addr[13];
        emit_buf[34] = uip_addr.s6_addr[14];
        emit_buf[35] = uip_addr.s6_addr[15];
#else
        emit_buf[20] = 0;
        emit_buf[21] = 0;
        emit_buf[22] = 0;
        emit_buf[23] = 0;
        emit_buf[24] = 0;
        emit_buf[25] = 0;
        emit_buf[26] = 0;
        emit_buf[27] = 0;
        emit_buf[28] = 0;
        emit_buf[29] = 0;
        emit_buf[30] = 0;
        emit_buf[31] = 0;
	emit_buf[32] = (uip_addr >> 24) & 0xff;	/* MSB ip_addr */
	emit_buf[33] = (uip_addr >> 16) & 0xff; 
	emit_buf[34] = (uip_addr >> 8) & 0xff; 
	emit_buf[35] = uip_addr & 0xff;		/* LSB  */

#endif
        len_buf = 36 ;
        McSendRtcpData(emit_buf, len_buf);
}

/* return the number of byte read or 0 if probleme */

int McCheckRtcpLrmpNack( unsigned char *buf, int len_buf,
	McRtcpLrmpNackDataStruct *rlnd, IPAddr ipfrom)
{
	unsigned char * p = buf;

	if ( len_buf < 36 ){ 
		fprintf(stderr,"Error McCheckRtcpLrmpNack: n = %d\n", len_buf);
		return 0;
	}
			/* T:2 P:1 SC:LRMP_NACK PT:RTCP_PT_LRMP */
	rlnd->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                        ((unsigned long) p[1]      ) );
	if (rlnd->rh_flags != (u_int16_t) (0x8300 + RTCP_PT_LRMP)){
		fprintf(stderr,"Error receiving rlnd: proto error\n");
		return 0;
	}
	rlnd->ipaddr = ipfrom;
	rlnd->pid =  (u_int16_t)(              ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
        rlnd->s_ssrc = p[8];
        rlnd->s_pid = (u_int16_t)(            ((unsigned long) p[10] << 8 ) |
                                              ((unsigned long) p[11]     ) );
        rlnd->url_id = (u_int32_t)     (      ((unsigned long) p[9] << 16) |
                                              ((unsigned long) p[12] << 8 ) |
                                              ((unsigned long) p[13]      ) );
        rlnd->num_eo = (u_int16_t)(           ((unsigned long) p[14] << 8 ) |
                                              ((unsigned long) p[15]     ) );
	rlnd->fpno = (u_int16_t)(            ((unsigned long) p[16] << 8 ) |
					        ((unsigned long) p[17]     ) );

	rlnd->blp = (u_int16_t)(            ((unsigned long) p[18] << 8 ) |
					        ((unsigned long) p[19]     ) );
	rlnd->s_ipaddr.s6_addr[0] = p[20];
	rlnd->s_ipaddr.s6_addr[1] = p[21];
	rlnd->s_ipaddr.s6_addr[2] = p[22];
	rlnd->s_ipaddr.s6_addr[3] = p[23];
	rlnd->s_ipaddr.s6_addr[4] = p[24];
	rlnd->s_ipaddr.s6_addr[5] = p[25];
	rlnd->s_ipaddr.s6_addr[6] = p[26];
	rlnd->s_ipaddr.s6_addr[7] = p[27];
	rlnd->s_ipaddr.s6_addr[8] = p[28];
	rlnd->s_ipaddr.s6_addr[9] = p[29];
	rlnd->s_ipaddr.s6_addr[10] = p[30];
	rlnd->s_ipaddr.s6_addr[11] = p[31];
	rlnd->s_ipaddr.s6_addr[12] = p[32];
	rlnd->s_ipaddr.s6_addr[13] = p[33];
	rlnd->s_ipaddr.s6_addr[14] = p[34];
	rlnd->s_ipaddr.s6_addr[15] = p[35];
/*
	rlnd->s_ipaddr = (u_int32_t)	(       ((unsigned long) p[8] << 24) |
					        ((unsigned long) p[9] << 16) |
					        ((unsigned long) p[10] << 8 ) |
					        ((unsigned long) p[11]      ) );
*/
	p = &buf[36];
	return p-buf;
}

/* a RTCP BYE  packet/
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    SC   |      PT=203   |             length            | header
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |  SSRC         | 0             |     pid                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
 */
void McSendRtcpBye(void) 
{
	unsigned int len_buf;

	emit_buf[0] = 0x80;	/* V=2 p=0 SC=0 */
	emit_buf[1] = (unsigned char) RTCP_PT_BYE;
	emit_buf[2] = (unsigned char) (( 0 >> 8) & 0xff);
	emit_buf[3] = (unsigned char) (1 & 0xff);
	emit_buf[4] = 0x01;
	emit_buf[5] = 0x0;

	emit_buf[6] = (mc_my_pid >> 8) & 0xff; 
	emit_buf[7] = mc_my_pid & 0xff;		/* LSB my_pid */

        len_buf = 8  ;
        McSendRtcpData(emit_buf, len_buf);
}

/* return the number of byte read or 0 if probleme */

int McCheckRtcpBye( unsigned char *buf, int len_buf,
	McRtcpByeDataStruct *rbye, IPAddr ipfrom)
{
	unsigned int i;
	unsigned char * p = buf;

	if ( len_buf < 8 ){ 
		fprintf(stderr,"Error receiving McRtcpByeDataStruct: n = %d\n", len_buf);
		return 0;
	}
	
			/* T:2 P:1 SC:5 PT:8 */
	rbye->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                        ((unsigned long) p[1]      ) );

	if (rbye->rh_flags != (u_int16_t) (0x8000 + RTCP_PT_BYE)){
		fprintf(stderr,"Error receiving rbye: proto error\n");
		return 0;
	}
	rbye->ipaddr = ipfrom;
	rbye->ssrc = p[4];
	rbye->pid =  (u_int16_t)(            ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	return 8;
}

/* a RTCP SDES CNAME packet/
 *  0                   1                   2                   3
 *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 * |V=2|P|    SC   |      PT       |             length            | header
 * +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
 * |  SSRC         | 0             |     pid                       |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+   1
 * |  CNAME        |  len          | ... data ...                  |
 * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
 */
void McSendRtcpSdesCname(void) 
{
	unsigned int len_buf;
	unsigned int cnt;

	emit_buf[0] = 0x80;	/* V=2 p=0 SC=0 */
	emit_buf[1] = (unsigned char) RTCP_PT_SDES;
	emit_buf[4] = 0x01;
	emit_buf[5] = 0x0;

	emit_buf[6] = (mc_my_pid >> 8) & 0xff; 
	emit_buf[7] = mc_my_pid & 0xff;		/* LSB my_pid */

	emit_buf [8] = RTCP_SDES_CNAME;
	emit_buf[9] = mc_len_alias;
	memcpy(&emit_buf[10], mc_alias_name, mc_len_alias);

        len_buf = 10 + mc_len_alias ;
	cnt = (len_buf + 3)/4;
	emit_buf[2] = (unsigned char) (( cnt >> 8) & 0xff);
	emit_buf[3] = (unsigned char) (cnt & 0xff);
        McSendRtcpData(emit_buf, len_buf);
}

/* return the number of byte read or 0 if probleme */

int McCheckRtcpSdesCname( unsigned char *buf, int len_buf,
	McRtcpSdesCnameDataStruct *rscd, IPAddr ipfrom)
{
	unsigned int i;
	unsigned char * p = buf;

	if ( len_buf < 10 ){ 
		fprintf(stderr,"Error receiving McRtcpSdesCnameDataStruct: n = %d\n", len_buf);
		return 0;
	}
	
			/* T:2 P:1 SC:5 PT:8 */
	rscd->rh_flags = (u_int16_t) (	((unsigned long) p[0] << 8 ) |
                                        ((unsigned long) p[1]      ) );

	if (rscd->rh_flags != (u_int16_t) (0x8000 + RTCP_PT_SDES)){
		fprintf(stderr,"Error receiving rscd: proto error\n");
		return 0;
	}
	rscd->ipaddr = ipfrom;
	rscd->ssrc = p[4];
	rscd->pid =  (u_int16_t)(            ((unsigned long) p[6] << 8 ) |
					        ((unsigned long) p[7]      ) );
	rscd->code = (u_int8_t)	p[8];
	if (rscd->code != RTCP_SDES_CNAME){
		fprintf(stderr,"Error receiving McRtcpSdesCnameDataStruct: n = %d\n", len_buf);   
                return 0;
        }                

	p = &buf[9];
	rscd->len_alias = *p++;
	if ((rscd->len_alias > MC_MAX_ALIAS_SIZE) || (rscd->len_alias == 0)){
		fprintf(stderr,"Error receiving RtcpSdesCname: len_alias = %d\n",
					rscd->len_alias);
		return 0;
	}
	if ( p + rscd->len_alias - buf != len_buf){
		fprintf(stderr,"Error receiving RtcpSdesCname:len buf= %d, n=%d\n"
					, p+rscd->len_alias - buf,len_buf);
		return 0 ;
	}
	for (i = 0; i< rscd->len_alias; i++)
		rscd->alias[i] = *p++;
	rscd->alias[rscd->len_alias] = '\0';

	return p-buf;
}

/*
 * return number of byte read and the static recv_buf 
 */
int McGetRtcpRecvBuf(unsigned char ** buf, IPAddr * ipfrom)
{
	int cnt;
  
	cnt = recvfrom(mc_rtcp_fdread, (char*)recv_buf, MC_MAX_BUF_SIZE,0, 
			(struct sockaddr *)&rtcp_addr_r, &rtcp_addr_r_len);
	if (cnt <= 0 ) {
		perror ("recvfrom");
		/* exit(1); */
	}
	*buf = recv_buf;
#ifdef IPV6
	*ipfrom = rtcp_addr_r.sin6_addr;
#else
	*ipfrom = rtcp_addr_r.sin_addr.s_addr;
#endif
	return cnt;
}

#endif /* MULTICAST */
