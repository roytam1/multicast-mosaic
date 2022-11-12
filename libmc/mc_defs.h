/* Author: Gilles Dauphin
 * Version 3.3.0 [Jan98]
 */

#ifdef MULTICAST

#ifndef MC_DEFS_H
#define MC_DEFS_H

#define MC_MAX_SDES_NAME_LEN 	255	/* max len of a name */

#if 0

typedef struct _McRtcpByeDataStruct {
	unsigned short rh_flags; /* (0x8000 + RTCP_PT_BYE) */
	IPAddr ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* SSRC */
	unsigned short pid;	/* user's pid */
} McRtcpByeDataStruct;

typedef struct _McRtcpSdesCnameDataStruct {
	unsigned short rh_flags; /* (0x8000 + RTCP_PT_SDES) */
	IPAddr ipaddr;	/* user's ip addr */
	unsigned int ssrc;	/* SSRC */
	unsigned short pid;	/* user's pid */
	unsigned char code;	/* RTCP_SDES_CNAME */
	unsigned char len_alias;	/* length of alias name */
	char alias[MC_MAX_ALIAS_SIZE+1]; /* alias de l'utilisateur  */
} McRtcpSdesCnameDataStruct;


#endif
#endif /* MC_DEFS_H */

#endif /* MULTICAST */
