/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#define FONTNAME "-adobe-courier-medium-r-normal-*-17-*-*-*-*-*-iso8859-1"

#define TRANS_HTTP 0
#define TRANS_CCI 1

#define PROXY 0
#define PROXY_DOMAIN 1

struct ProxyDomain {
	struct ProxyDomain *next;
	struct ProxyDomain *prev;
	char *domain;
};
	

struct Proxy {
	struct Proxy *next;
	struct Proxy *prev;
	char *scheme;
	char *address;
	char *port;
	char *transport;
	int trans_val;
	int alive;
	struct ProxyDomain *list;
};

/* added function prototypes - DXP */

void AddProxyToList(), ShowProxyList(), EditProxyInfo(), CommitProxyInfo(),
        DismissProxy(), ClearProxyText(), FillProxyText(),  WriteProxies(),
        RemoveProxyInfo(), EditProxyDomainInfo(), DisplayErrorMessage(), 
        ShowProxyDomainList(), CommitProxyDomainInfo(),
        CallEdit(), CallAdd(), CallEditDomain(), CallAddDomain(), 
        CallRemoveProxy(), DestroyDialog(), PopProxyDialog(), DeleteProxy(),
        EditNoProxyInfo(), CenterDialog(), ProxyHelpWindow(), HelpWindow();

struct Proxy *ReadProxies(char *filename);

struct Proxy *ReadNoProxies(char *filename);

struct ProxyDomain *AddProxyDomain(char *sbDomain, struct ProxyDomain **pdList);

void DeleteProxyDomain(struct ProxyDomain *p);
void ClearTempBongedProxies();
