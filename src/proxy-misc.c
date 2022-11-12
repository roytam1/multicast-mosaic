/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "proxy.h"

extern struct Proxy *proxy_list;

#define BUFLEN 256
#define BLANKS " \t\n"

struct ProxyDomain *AddProxyDomain();

struct Proxy * ReadProxies(char *filename)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;
	struct ProxyDomain *pCurList, *pNewDomain;
	extern void FreeProxy();
		
	if ((fp = fopen(filename,"r")) == NULL)
		return NULL;

	head = NULL;
	cur = NULL;

	/*
	** read entries from the proxy list
	**
	** These malloc()s should be checked for returning NULL
	*/
	while (fgets(buf, BUFLEN, fp) != 0) {

		p = (struct Proxy *)calloc(1,sizeof(struct Proxy));
		
		p->next = NULL;
		p->prev = NULL;

		/*
		** Read the proxy scheme
		*/

		if ((psb = strtok(buf, BLANKS)) == NULL)
			return head;

		p->scheme = (char *)malloc(strlen(psb)+1);
		strcpy(p->scheme, psb);

		/*
		** Read the proxy address
		*/

		if ((psb = strtok(NULL, BLANKS)) == NULL)
			return head;
		p->address = (char *)malloc(strlen(psb)+1);
		strcpy(p->address, psb);

		/*
		** Read the proxy port
		*/

		if ((psb = strtok(NULL, BLANKS)) == NULL)
			return head;
		p->port = (char *)malloc(strlen(psb)+1);
		strcpy(p->port, psb);

		/*
		** Read the transport mechanism
		*/
		if ((psb = strtok(NULL, BLANKS)) == NULL)
			return head;
		p->transport = (char *)malloc(strlen(psb)+1);
		strcpy(p->transport, psb);

		p->alive = 0;

		if (strcmp(p->transport,"CCI") == 0)
			p->trans_val = TRANS_CCI;
		else
			p->trans_val = TRANS_HTTP;

		/*
		** Read the domain
		*/
		p->list = NULL;

		if ((psb = strtok(NULL, BLANKS)) != NULL) {

			p->list = NULL;
			AddProxyDomain(psb, &p->list);

			pCurList = p->list;

			while ((psb = strtok(NULL, BLANKS)) != NULL) {
				if (psb[0] == '\\') {
					if (fgets(buf, BUFLEN, fp) == 0) {
						return head;
					}
					psb = strtok(buf, BLANKS);
					if (psb == NULL)
						return head;
				}
				if (AddProxyDomain(psb, &pCurList) == NULL)
					return head;
			}
		}

		if (cur == NULL) {
			head = p;
			cur = p;
		} else {
			p->prev = cur;
			cur->next = p;
			cur = p;
		}
		if (feof(fp) != 0)
			break;
	}

	return(head);
}

struct Proxy * ReadNoProxies(char *filename)
{
	FILE *fp;
	char buf[BUFLEN], *psb;
	struct Proxy *head, *cur, *next, *p;
	extern void FreeProxy();
		
	if ((fp = fopen(filename,"r")) == NULL)
		return NULL;

	head = NULL;
	cur = NULL;

	/*
	** read entries from the proxy list
	**
	** These malloc()s should be checked for returning NULL
	*/
	while (fgets(buf, BUFLEN, fp) != 0) {

		p = (struct Proxy *)calloc(1, sizeof(struct Proxy));
		
		p->next = NULL;
		p->prev = NULL;

		/*
		** The proxy protocol, transport, and list
		** are all null for no proxy.
		*/
		p->scheme = NULL;
		p->transport = NULL;
		p->list = NULL;

		/*
		** Read the proxy address
		*/

		if ((psb = strtok(buf, BLANKS)) == NULL)
			return head;
		p->address = (char *)malloc(strlen(psb)+1);
		strcpy(p->address, psb);

		/*
		** Read the proxy port
		*/

		if ((psb = strtok(NULL, BLANKS)) == NULL) {
			p->port = NULL;
		} else {
			p->port = (char *)malloc(strlen(psb)+1);
			strcpy(p->port, psb);
		}

		if (cur == NULL) {
			head = p;
			cur = p;
		} else {
			p->prev = cur;
			cur->next = p;
			cur = p;
		}
		if (feof(fp) != 0)
			break;
	}

	return(head);
}

struct ProxyDomain *
AddProxyDomain(char *sbDomain, struct ProxyDomain **pdList)
{
	struct ProxyDomain *pNewDomain;

	pNewDomain = (struct ProxyDomain *)malloc(sizeof(struct ProxyDomain));
	if (pNewDomain == NULL)
		return NULL;

	pNewDomain->domain = (char *)malloc(strlen(sbDomain)+1);
	strcpy(pNewDomain->domain, sbDomain);
	if (*pdList == NULL) {
		*pdList = pNewDomain;
		(*pdList)->next = NULL;
		(*pdList)->prev = NULL;
	} else {
		struct ProxyDomain *p;

		p = *pdList;
		while (p->next != NULL)
			p = p->next;
		pNewDomain->prev = p;
		pNewDomain->next = NULL;
		p->next = pNewDomain;
	}
	return pNewDomain;
}

void DeleteProxyDomain(struct ProxyDomain *p)
{
	struct ProxyDomain *cur;

	cur = p;
	if (cur->next !=NULL)
		cur->next->prev =  p->prev;
	if (cur->prev !=NULL)
		cur->prev->next =  p->next;
	if (p->domain) {
		free(p->domain);
		p->domain = NULL;
	}
	free(p);
	p = NULL;
}

/* Returns true if there is at least one fallback proxy for the specified
 * protocol (means more than one proxy server specified).
 * --SWP
 */
int has_fallbacks(char *protocol)
{
	int protocol_len;
	struct Proxy *ptr;

      if (!protocol || !*protocol || !proxy_list)
              return(0);

      protocol_len=strlen(protocol);
      ptr=proxy_list;

      while (ptr) {
              if (ptr->scheme && !strncmp(ptr->scheme,protocol,protocol_len))
                      return(1);
              ptr=ptr->next;
      }
      return(0);
}
