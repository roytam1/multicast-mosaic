/* Please read copyright.ncsa. Don't remove next line */
#include "../Copyrights/copyright.ncsa"

#define FONTNAME "-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-iso8859-1"

struct Proxy {
	struct Proxy *next;
	struct Proxy *prev;
	char *scheme;
	char *address;
	char *port;
	char *transport;
	int alive;
};

extern char * mMosaicProxyFileName;
extern char * mMosaicNoProxyFileName;

/* added function prototypes - DXP */

extern void ReadProxies(char *rootdir);
extern void ReadNoProxies(char *rootdir);
extern struct Proxy * GetProxy(char *acc);
extern struct Proxy * GetNoProxy(char *access, char *site);
