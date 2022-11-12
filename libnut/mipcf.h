/* Author: Gilles Dauphin
 * Version 3.1.1 [May97]
 *
 * Copyright (C) 1997 - G.Dauphin, P.Dax
 *
 * configuration for IPv6 or IPv4
 */

#ifndef mipcf_h
#define mipcf_h

#ifdef _HPUX_SOURCE
/* mjr: arrrgggghhhhhh!!!!!!!!!!!! HPUX: High Prices, Uncomfortable uniX*/
# include <sys/types.h>
#endif

#if defined(NETBSD) || defined(FreeBSD)
#include <db.h>
#endif

#include <netinet/in.h>

#include "mosaic-types.h"

#ifdef IPV6
typedef struct in6_addr IPAddr;
#else
typedef u_int32_t 	IPAddr;
#endif

#define ADDRCMP66(a,b)	( \
	( a.s6_addr[0] == b.s6_addr[0]) && \
	( a.s6_addr[1] == b.s6_addr[1]) && \
	( a.s6_addr[2] == b.s6_addr[2]) && \
	( a.s6_addr[3] == b.s6_addr[3]) && \
	( a.s6_addr[4] == b.s6_addr[4]) && \
	( a.s6_addr[5] == b.s6_addr[5]) && \
	( a.s6_addr[6] == b.s6_addr[6]) && \
	( a.s6_addr[7] == b.s6_addr[7]) && \
	( a.s6_addr[8] == b.s6_addr[8]) && \
	( a.s6_addr[9] == b.s6_addr[9]) && \
	( a.s6_addr[10] == b.s6_addr[10]) && \
	( a.s6_addr[11] == b.s6_addr[11]) && \
	( a.s6_addr[12] == b.s6_addr[12]) && \
	( a.s6_addr[13] == b.s6_addr[13]) && \
	( a.s6_addr[14] == b.s6_addr[14]) && \
	( a.s6_addr[15] == b.s6_addr[15]) \
	)

#define ADDRCMP44(a,b)	( a == b )

#define ADDRCMP64(a,b)	( \
	( a.s6_addr[0] == 0) && \
	( a.s6_addr[1] == 0) && \
	( a.s6_addr[2] == 0) && \
	( a.s6_addr[3] == 0) && \
	( a.s6_addr[4] == 0) && \
	( a.s6_addr[5] == 0) && \
	( a.s6_addr[6] == 0) && \
	( a.s6_addr[7] == 0) && \
	( a.s6_addr[8] == 0) && \
	( a.s6_addr[9] == 0) && \
	( a.s6_addr[10] == 0) && \
	( a.s6_addr[11] == 0) && \
	( a.s6_addr[12] == ( ((u_int32_t)(b & 0xff000000)) >> 24) ) && \
	( a.s6_addr[13] == ( (b & 0x00ff0000) >> 16) ) && \
	( a.s6_addr[14] == ( (b & 0x0000ff00) >> 8 ) ) && \
	( a.s6_addr[15] == ( b & 0xff ) ) \
	)

#ifdef IPV6
#define ADDRCMP(a,b)	ADDRCMP66(a,b)
#else
#define ADDRCMP(a,b)	ADDRCMP44(a,b)
#endif

struct __mcmo_in6_addr__ {
	u_int8_t addr[16];	/* compatible Ipv6 struct */
};

typedef struct __mcmo_in6_addr__ IPAddr6 ;
typedef u_int32_t	IPAddr4;

#endif /* mipcf_h */
