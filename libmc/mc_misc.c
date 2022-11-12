#include <stdio.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <Xm/XmAll.h>

#include "../libnut/mipcf.h"
#include "../src/mosaic.h"

#include "mc_mosaic.h"
#include "mc_rtp.h"
#include "mc_misc.h"

 
/* Copyright (c) 1995 The Regents of the University of California.
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
 *      This product includes software developed by the Network Research
 *      Group at Lawrence Berkeley National Laboratory.
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
 * @(#) $Header: ntp-time.h,v 1.5 96/03/22 08:31:27 van Exp $ (LBL)
 */

/*            
 * convert microseconds to fraction of second * 2^32 (i.e., the lsw of
 * a 64-bit ntp timestamp).  This routine uses the factorization
 * 2^32/10^6 = 4096 + 256 - 1825/32 which results in a max conversion
 * error of 3 * 10^-7 and an average error of half that.
 */           
static u_int32_t usec2ntp(u_int32_t usec)
{             
        u_int32_t t = (usec * 1825) >> 5;
        return ((usec << 12) + (usec << 8) - t);
}
 
/*
 * Number of seconds between 1-Jan-1900 and 1-Jan-1970
 */
#define GETTIMEOFDAY_TO_NTP_OFFSET 2208988800
 
/*
 * Return a 64-bit ntp timestamp (UTC time relative to Jan 1, 1970).
 * gettimeofday conveniently gives us the correct reference -- we just
 * need to convert sec+usec to a 64-bit fixed point (with binary point
 * at bit 32).
 */
ntp64 ntp64time(struct timeval tv)
{
        ntp64 n;
        n.upper = (u_int32_t)tv.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
        n.lower = usec2ntp((u_int32_t)tv.tv_usec);
        return (n);
}
 
u_int32_t tv2ntptime(struct timeval t)
{
        u_int32_t s = (u_int32_t)t.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
        return (s << 16 | usec2ntp((u_int32_t)t.tv_usec) >> 16);
}

u_int32_t ntptime(void)
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return (tv2ntptime(tv));
}

struct timeval unixtime()
{
	struct timeval tv;
	gettimeofday(&tv, 0);
	return tv;
}
 
/*
u_int32_t McRtpTimeStamp(struct timeval ts)
{
	u_int32_t sec;
	u_int32_t frac;
	u_int32_t t;

	sec = (u_int32_t)ts.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
	frac = usec2ntp((u_int32_t)ts.tv_usec);
	t = (u_int32_t) ( 	((sec & 0xffff) << 16) |
				 (frac >> 16) & 0xffff) ;
	return t;
}
*/

/* should be relative to GMT. FIXME */
u_int32_t McRtpTimeStamp(struct timeval ts)
{
	return (u_int32_t)ts.tv_sec + GETTIMEOFDAY_TO_NTP_OFFSET;
}
