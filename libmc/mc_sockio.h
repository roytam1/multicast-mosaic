/*
 * mc_sockio.h
 * Author: G.Dauphin
 * Version 2.0 [Marb96]
 *
 * Copyright (C) 1996 - G.Dauphin, P.Dax (ENST)
 *
 * THIS PROGRAM IS DISTRIBUTED WITHOUT ANY WARRANTY
 * COMMERCIAL USE IS FORBIDDEN WITHOUT EXPLICIT AUTHORIZATION
 *
 * Bug report :
 * 
 * dax@inf.enst.fr
 * dauphin@sig.enst.fr
 */

extern IPAddr mc_local_ip_addr;

extern int McOpenRead(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McOpenRtcpRead(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McOpenWrite(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McOpenRtcpWrite(IPAddr ip,unsigned short port,unsigned char ttl);
extern int McGetRecvBuf(unsigned char **, IPAddr *);
extern int McGetRtcpRecvBuf(unsigned char **, IPAddr *);

extern int McCheckCursorPos(unsigned char *buf, int len, 
			McRtpCursorPosDataStruct *cp, IPAddr ipfrom);
extern int McCheckGotoId( unsigned char *buf, int len, 
			McRtpGotoIdDataStruct *hgid, IPAddr ipfrom);
extern int McCheckAllData( unsigned char *buf, int len,
			Mcs_alldata *alldata, IPAddr ipfrom);
extern int McCheckRtcpSdesCname(unsigned char *buf, int len,
			McRtcpSdesCnameDataStruct *rtcp_scd, IPAddr ipfrom);
extern int McCheckRtcpLrmpNackAll(unsigned char *buf, int len,
        		McRtcpLrmpNackAllDataStruct *rlnad, IPAddr ipfrom);
extern int McCheckRtcpLrmpNack(unsigned char *buf, int len,
        		McRtcpLrmpNackDataStruct *rlnd, IPAddr ipfrom);
extern int McCheckRtcpBye(unsigned char *buf, int len,
		        McRtcpByeDataStruct *rbye, IPAddr ipfrom);

extern void McSendRtpCursorPos(unsigned char code, unsigned short pid,
        		unsigned int url_id, unsigned int gmt_send_time,
        		int x, int y);
extern void McSendRtpGotoId(unsigned char code, unsigned short pid,
        		unsigned int url_id, unsigned int gmt_send_time,
        		unsigned int html_goto_id);

extern void McSendPacket(unsigned char code, unsigned short pid, 
			unsigned int url_id, unsigned int gmt_send_time, 
			unsigned int nombre_eo, unsigned int num_eo, 
			unsigned int seo, unsigned int nombre_packet,
			unsigned int num_packet , char * data, unsigned int len );

extern void McSendRtcpNackAll(IPAddr uip_addr, unsigned short upid,  
                        unsigned int uurl_id, unsigned short num_eoi,
			unsigned int ussrc);
extern void McSendRtcpNack(IPAddr uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo, 
                        unsigned short num_pkt, unsigned short blp,
			unsigned int ussrc);
extern void McSendRtcpSdesCname(void);
extern void McSendRtcpBye(void);

extern void McInitNackPacketData(IPAddr uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo,
                        unsigned int ussrc);
extern void McPushNackPacketData(IPAddr uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo,
                        unsigned short num_pkt, unsigned int ussrc);
extern void McFlushNackPacketData(void);
