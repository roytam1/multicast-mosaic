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

extern unsigned int mc_local_ip_addr;

extern int McOpenRead(unsigned long ip,unsigned short port,unsigned char ttl);
extern int McOpenRtcpRead(unsigned long ip,unsigned short port,unsigned char ttl);
extern int McOpenWrite(unsigned long ip,unsigned short port,unsigned char ttl);
extern int McOpenRtcpWrite(unsigned long ip,unsigned short port,unsigned char ttl);
extern int McGetRecvBuf(unsigned char **, u_int32_t *);
extern int McGetRtcpRecvBuf(unsigned char **, u_int32_t *);


extern int McCheckAllData( unsigned char *buf, int len_buf, Mcs_alldata *alldata, unsigned int ipfrom);
extern int McCheckRtcpSdesCname( unsigned char *buf, int len_buf, McRtcpSdesCnameDataStruct *rtcp_sdes_cname_data, unsigned int ipfrom);

extern void McSendPacket(unsigned char code, unsigned int ipaddr,
		unsigned short pid, unsigned int url_id,
		unsigned int gmt_send_time, unsigned int nombre_eo,
		unsigned int num_eo, unsigned int seo, unsigned int nombre_packet,
		unsigned int num_packet , char * data, unsigned int len );
extern void McSendRtcpSdesCname(void);
extern void McSendRtcpNackAll( unsigned int uip_addr, unsigned short upid,  
                        unsigned int uurl_id, unsigned short num_eoi,
			unsigned int ussrc);
extern void McSendRtcpNack(unsigned int uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo, 
                        unsigned short num_pkt, unsigned short blp,
			unsigned int ussrc);

extern int McCheckRtcpLrmpNackAll( unsigned char *buf, int len_buf,
        McRtcpLrmpNackAllDataStruct *rlnad, u_int32_t ipfrom);
extern int McCheckRtcpLrmpNack( unsigned char *buf, int len_buf,
        McRtcpLrmpNackDataStruct *rlnd, u_int32_t ipfrom);

extern void McSendRtpGotoId( unsigned char code, unsigned short pid,
        unsigned int url_id, unsigned int gmt_send_time,
        unsigned int html_goto_id);

extern int McCheckGotoId( unsigned char *buf, int len_buf, 
		McRtpGotoIdDataStruct *hgid, u_int32_t ipfrom);
extern int McCheckCursorPos( unsigned char *buf, int len_buf, McRtpCursorPosDataStruct *cp,
                u_int32_t ipfrom);
extern void McSendRtpCursorPos( unsigned char code, unsigned short pid,
        unsigned int url_id, unsigned int gmt_send_time,
        int x, int y);
extern void McSendRtcpBye(void);
extern int McCheckRtcpBye( unsigned char *buf, int len_buf,
        McRtcpByeDataStruct *rbye, u_int32_t ipfrom);

extern void McInitNackPacketData(unsigned int uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo,
                        unsigned int ussrc);
extern void McPushNackPacketData(unsigned int uip_addr, unsigned short upid,
                        unsigned int uurl_id, unsigned short num_eo,
                        unsigned short num_pkt, unsigned int ussrc);
extern void McFlushNackPacketData(void);
