/*
 * Generate a random 32-bit quantity
 *
 * RFC 1889
 */
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/utsname.h>
#ifdef SOLARIS
#include <sys/systeminfo.h>
#endif
#include "md5-global.h"		/* from RFC 1321 */
#include "md5.h"		/* from RFC 1321 */

#define MD_CTX MD5_CTX
#define MDInit MD5Init
#define MDUpdate MD5Update
#define MDFinal MD5Final

unsigned int random32(int);

static u_long md_32(char *string, int length)
{
	MD_CTX context;
	union {
		char	c[16];
		u_long	x[4];
	} digest;
	u_long r;
	int i;

	MDInit(&context);
	MDUpdate(&context, string, length);
	MDFinal((unsigned char *)&digest, &context);
	r = 0;
	for (i = 0; i < 3; i++) {
		r ^= digest.x[i];
	}
	return r;
}

/*
 * Return random unsigned 32-bit quantity. Use 'type' argument if you
 * need to generate several different values in close succession.
 */
unsigned int random32(int type)
{
	struct {
		int	type;
		struct	timeval tv;
		clock_t	cpu;
		pid_t	pid;
		u_long	hid;
		uid_t	uid;
		gid_t	gid;
		struct	utsname name;
	} s;
	char buf[32];

	gettimeofday(&s.tv, 0);
	uname(&s.name);
	s.type	= type;
	s.cpu	= clock();
	s.pid	= getpid();
#ifdef SOLARIS
	sysinfo(SI_HW_SERIAL, buf, sizeof(buf));
	s.hid	= atoi(buf);
#else
	s.hid	= gethostid();
#endif
	s.uid	= getuid();
	s.gid	= getgid();

	return md_32((char *)&s, sizeof(s));
}
