/*
 * Copyright (c) 1995 The Regents of the University of California.
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
 * 	This product includes software developed by the Network Research
 * 	Group at Lawrence Berkeley National Laboratory.
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
 *
 * @(#) $Header: config.h,v 1.2 96/05/16 05:26:51 van Exp $ (LBL)
 */

#ifndef rtp_config_h
#define rtp_config_h

#if defined(sun)||defined(_AIX)
#if defined(__cplusplus)
extern "C" {
#endif
#if !defined(__SunOS_5_7) && !defined(__SunOS_5_8)
int srandom(int);
int random(void);
#endif
#if defined(__cplusplus)
}
#endif
#endif

#ifdef sgi
#include <math.h>
#endif

#include <stdlib.h>
#include <time.h>		/* For clock_t */
#ifndef WIN32
#include <unistd.h>
#if defined(__cplusplus)
extern "C" {
#endif
#include <netinet/in.h>
#include <arpa/inet.h>
int strcasecmp(const char *, const char *);
clock_t clock(void);
#if !defined(sco) && !defined(sgi) && !defined(__bsdi__) && !defined(__FreeBSD__) && !defined(linux) && !defined(__alpha)
long int gethostid(void);
#endif
time_t time(time_t *);
char *ctime(const time_t *);
#if defined(__cplusplus)
}
#endif
#endif

#ifdef WIN32

#include <winsock.h>

#define MAXHOSTNAMELEN	256

#define _SYS_NMLN	9
struct utsname {
	char sysname[_SYS_NMLN];
	char nodename[_SYS_NMLN];
	char release[_SYS_NMLN];
	char version[_SYS_NMLN];
	char machine[_SYS_NMLN];
};

typedef char *caddr_t;

struct iovec {
	caddr_t iov_base;
	int	    iov_len;
};

struct timezone {
	int tz_minuteswest;
	int tz_dsttime;
};

typedef int pid_t;
typedef int uid_t;
typedef int gid_t;
    
#if defined(__cplusplus)
extern "C" {
#endif

int uname(struct utsname *);
int getopt(int, char * const *, const char *);
int strcasecmp(const char *, const char *);
int srandom(int);
int random(void);
int gettimeofday(struct timeval *p, struct timezone *z);
int gethostid(void);
int getuid(void);
int getgid(void);
int getpid(void);
int nice(int);
int sendmsg(int, struct msghdr*, int);
time_t time(time_t *);

#if defined(__cplusplus)
}
#endif

#define ECONNREFUSED	WSAECONNREFUSED
#define ENETUNREACH	WSAENETUNREACH
#define EHOSTUNREACH	WSAEHOSTUNREACH
#define EWOULDBLOCK	WSAEWOULDBLOCK

#define M_PI		3.14159265358979323846

#endif /* WIN32 */
  
#endif
