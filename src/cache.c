#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <sys/param.h>		/* MAXPATHLEN is defined here */
#include <unistd.h>
#include <dirent.h>

#include "../libhtmlw/HTMLparse.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../libhtmlw/HTMLPutil.h"
#include "mosaic.h"
#include "mime.h"
#include "cache.h"
#include "util.h"
#include "paf.h"

typedef struct _CacheEntry {
	char *url;              /* Canonical URL for this document. */
	int exist;		/* Is this entry in used ? */
	time_t last_modify;
	char *content_type;
	int content_encoding;
	int size;
} CacheEntry;

/* An entry in a hash bucket, containing a URL (in canonical, absolute form) */
typedef struct _HashEntry {
	char *url;              /* Canonical URL for this document. */
	int cid;
	struct _HashEntry *next;
} HashEntry;

typedef struct _Bucket {         
        HashEntry *head;              
        int count;
} Bucket;

#define MAX_N_FILE_CACHE	1000
#define HASH_TABLE_SIZE	MAX_N_FILE_CACHE

static char * lroot_dirname = NULL;	/* the root directory name */
static char * lcachedb_name = NULL;	/* lroot_dirname/cache.db */
static char * lcachedir_name = NULL;	/* lroot_dirname/cache */
static int llen_cachedir_name = 0;
static int lfd_cachedb = -1;		/* fd for cache.db */
static int lcachettl   = 30 * 24 * 3600;	/* 30 days by default */
static CacheEntry * cid_cache;
static Bucket * hash_tab;
static int lnext_cid = 0;


/* Given a URL, return the hash value */
static int hash_url (char *url)
{       
        int len, i, val; 
        
        if (!url)
                return 0;
        len = strlen (url);
        val = 0;
        for (i = 0; i < 10; i++) 
                val += url[(i * val + 7) % len];
        return val % HASH_TABLE_SIZE;
}

/* if url isn't already in the bucket; add it by creating a new entry. */     
static void add_url_to_bucket (int buck, char *url, int cid)
{ 
        Bucket *bkt = &(hash_tab[buck]);
        HashEntry *l;
 
        l = (HashEntry *)calloc (1, sizeof (HashEntry));
        l->url = strdup (url);
        l->cid=cid;
        l->next = NULL;
        if (bkt->head == NULL)
                bkt->head = l;
        else {
                l->next = bkt->head;     
                bkt->head = l;           
        }                                
        bkt->count++ ;                   
}

/* assume the url exist in the list */

static void remove_url_from_bucket(int buck, char * url)
{
        Bucket *bkt = &(hash_tab[buck]);
	HashEntry *prev, *deb, *next;

	deb = hash_tab[buck].head;
	prev = NULL;
	while (deb != NULL){
		if (strcmp(url,deb->url) == 0) { /* remove the entry */
			next = deb->next;
			if (prev == NULL){
				hash_tab[buck].head = next;
			} else {
				prev->next = next;
			}
			free (deb->url);
			free (deb);
			bkt->count--;
			return;
		}
		prev = deb;
		deb = deb->next;
	}
}
	
/* initialize the cache in ~/.mMosaic */
/* check for dirname/cache.db file and the dirname/cache directory */
/* the default dirname is ~/.mMosaic */
/* else create */
void MMCacheInit( char * root_dirname)
{
	int i, l, hcid, cid, status;
	struct stat s;
	char * data= NULL;
	char * href, *scid;
	char *ocwd;
	time_t t;
	struct mark_up * mlist;
	struct mark_up * mptr;
	DIR *dirp;
	struct dirent *direntp;

	lroot_dirname = strdup(root_dirname);

	l = strlen(root_dirname);
	lcachedb_name = (char*) malloc(l + 12);
	strcpy(lcachedb_name,lroot_dirname);
	strcat(lcachedb_name,"/cache.db");

	lcachedir_name = (char*) malloc(l + 12);
	strcpy(lcachedir_name,lroot_dirname);
	strcat(lcachedir_name,"/cache");
	llen_cachedir_name = strlen(lcachedir_name);

	if ( stat(lroot_dirname,&s) != 0 ) {
		fprintf (stderr, "Cache directory not found.  Not caching.\n");
		free(lroot_dirname);
		lroot_dirname = NULL;
		return;
	}

/* read the cache or make one */
	if ( stat(lcachedir_name,&s) != 0 ) { /* create dir cache */
		mkdir(lcachedir_name,0755);
	}
	if (stat(lcachedb_name,&s) !=0 ) { /* create empty cache.db */
		lfd_cachedb = creat(lcachedb_name, 0644);
		write(lfd_cachedb,"<html></html>\n",15 );
		close(lfd_cachedb);
	}
	lfd_cachedb = open(lcachedb_name, O_RDWR , 0644);
	stat(lcachedb_name, &s);

	data = (char*) malloc(s.st_size+1);
	l = read(lfd_cachedb, data, s.st_size);
	data[s.st_size] = '\0';
	close(lfd_cachedb);

	/* read the data base cache.db */
	mptr = mlist = HTMLLexem(data);
	t = time(NULL);
	hcid =0;

/* alloc and look at max number of file MAX_N_FILE_CACHE */
	cid_cache = (CacheEntry *) calloc( MAX_N_FILE_CACHE , sizeof(CacheEntry));

/* create a hash-code table and a reverse cid table */
	hash_tab = (Bucket *) calloc( MAX_N_FILE_CACHE , sizeof(Bucket));
	for( i=0 ; i<MAX_N_FILE_CACHE; i++){
		cid_cache[i].exist = 0;
		cid_cache[i].url = NULL;
		hash_tab[i].head = NULL;
		hash_tab[i].count = 0;
	}
	while (mptr != NULL) {
		char * fname;
		char * content_type;
		char * scontent_encoding;
		int content_encoding;
		int h;

		if (mptr->type == M_ANCHOR && !mptr->is_end) {
			href = ParseMarkTag(mptr->start, MT_ANCHOR, "href");
			scid = ParseMarkTag(mptr->start, MT_ANCHOR, "cid");
			content_type = ParseMarkTag(mptr->start, MT_ANCHOR, "content_type");
			scontent_encoding = ParseMarkTag(mptr->start, MT_ANCHOR, "content_encoding");
			if(!scontent_encoding || !content_type || !scid || !href) {
				mptr = mptr->next;
				continue;
			}
			fname = (char*) malloc(llen_cachedir_name +
				strlen(scid) + 2);
			sprintf(fname,"%s/%s",lcachedir_name,scid);
			cid = atoi(scid);
			content_encoding = atoi(scontent_encoding);
			status = stat(fname, &s);
			if ( status || cid >= MAX_N_FILE_CACHE || cid <= 0 ) { /* nofile */
				unlink(fname);
				free(fname);
				free(scid);
				free(href);
				free(scontent_encoding);
				free(content_type);
				mptr = mptr->next;
				continue;
			}
			if ( t - s.st_mtime  > lcachettl) { /* look at  date; */
				unlink(fname);
				free(fname);
				free(scid);
				free(href);
				free(scontent_encoding);
				free(content_type);
				mptr = mptr->next;
				continue;
			}
			/* date of file is in ttl */
			hcid = hcid > cid ? hcid : cid;	/* hightest cid */
			cid_cache[cid].url = strdup(href);
			cid_cache[cid].exist = 1;
			cid_cache[cid].last_modify = s.st_mtime;
			cid_cache[cid].size = s.st_size;
			cid_cache[cid].content_type = content_type;
			cid_cache[cid].content_encoding = content_encoding;

			h = hash_url(href);
			add_url_to_bucket(h, href, cid);

			free(fname);
			free(scid);
			free(href);
		}
		mptr = mptr->next;
	}

	FreeMarkUpList(mlist);	
	free(data);

	/* read the dir cache and look for valid entry. */
	/* else remove the file (no cid found in cache.db). */
	/* Check for consistency . */

	ocwd = getcwd(NULL,MAXPATHLEN);
	chdir(lcachedir_name);
	dirp = opendir( lcachedir_name );
	while ( (direntp = readdir( dirp )) != NULL ) {
		cid = atoi(direntp->d_name);
		if (cid <=0 || cid >= MAX_N_FILE_CACHE || ! cid_cache[cid].exist ){
			unlink(direntp->d_name);
		}
	}
	(void)closedir( dirp );
	chdir(ocwd);
	free(ocwd);

	lnext_cid = hcid + 1;
	if (lnext_cid <= 0 || lnext_cid >= MAX_N_FILE_CACHE)
		lnext_cid = 1;
}

/* A suggest from mjr@pc29.dfg.de*/
static int IsCacheableUrl( char * aurl_wa)
{
	int len;

/* URL with '?' */
	if( (char *)strchr(aurl_wa, '?' ) != NULL ) {
		return 0;
	}
/* minimum len for url : 
 * 	proto : 3
 * 	://h/ : 5
 * total : 8
 */
	len = strlen(aurl_wa);
	if (len < 8)
		return 0;
	if ( !strcasecmp( aurl_wa + len - 4, ".cgi") )
		return 0;
	return 1;
}

/* aurl_wa : full url
 * aurl :    cannon url. The reference to find in cache
 * fdw : file des to write
 * mhs : a Struct to fill
 * return 0/1 : Notfound/Found
 */
int MMCacheFindData(char *aurl_wa, char *aurl, int fdw, MimeHeaderStruct *mhs)
{
#define CACHE_BUFSIZ 8192
	char buf[CACHE_BUFSIZ];
	int h,i;
	HashEntry * deb;
	/* time_t t = time(NULL); */
	char scid[40];
	int cid;
	int fdr;
	char * fname;

	if (! IsCacheableUrl(aurl_wa))	/* dont't find uncacheable url */
		return 0;
	
	h = hash_url(aurl);
	deb = hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->url) == 0) {
			cid = deb->cid;
			sprintf(scid,"%ld",cid);
			fname = (char *) malloc(llen_cachedir_name +
						strlen(scid)+ 2);
			sprintf(fname, "%s/%s", lcachedir_name, scid); /* the filename */
			mhs->content_length = cid_cache[cid].size; 
			mhs->content_encoding = cid_cache[cid].content_encoding;
			if(mhs->content_type) free(mhs->content_type);
			mhs->content_type = strdup(cid_cache[cid].content_type);
			if(mhs->last_modified) free(mhs->last_modified);
			mhs->last_modified = strdup( 
				rfc822ctime(cid_cache[cid].last_modify));
			if(mhs->expires) free(mhs->expires);
			mhs->expires = strdup("never");  /* ### FIXME */
			if(mhs->location) free(mhs->location);
			mhs->location = NULL;
			mhs->status_code = HTTP_STATUS_INTERNAL_CACHE_HIT;
			fdr = open(fname, O_RDONLY);
			while ( (i = read(fdr,buf,CACHE_BUFSIZ)) >0)
				write(fdw, buf, i);
			close (fdr);
			free(fname);
			return 1;
		}
		deb = deb->next;
	}
	return 0;
}

/* - input :
	fname_r, file to read.
	aurl_wa, the full url (with optional query)
	aurl, the cannon url. Reference for cache
	mhs, mime header struct
*/
void MMCachePutDataInCache(char *fname_r, char *aurl_wa, char *aurl,
	MimeHeaderStruct * mhs)
{
	char buf[CACHE_BUFSIZ];
	HashEntry * deb;
	char scid[40];
	int cid, h, i;
	time_t t = time(NULL);
	int exist = 0;
	char *fname_w;
	int fdr, fdw;

	if (! IsCacheableUrl(aurl_wa))	/* dont't cache uncacheable url */
		return ;

	if ( mhs->cache_control & CACHE_CONTROL_NO_CACHE)
		return;

/* look if an entry still exist */
	h = hash_url(aurl);
	deb = hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->url) == 0) {
			exist = 1;
			break;
		}
		deb = deb->next;
	}
	if ( !exist ){
		h = hash_url(aurl);
		cid = lnext_cid;
		if (cid_cache[cid].url) {
			remove_url_from_bucket(hash_url(cid_cache[cid].url), cid_cache[cid].url);
			free(cid_cache[cid].url);
		}
		lnext_cid++;
		if ( lnext_cid >= MAX_N_FILE_CACHE)
			lnext_cid = 1;
		add_url_to_bucket(h, aurl, cid);
		sprintf(scid,"%ld",cid); 
		fname_w = (char *) malloc(llen_cachedir_name +
			strlen(scid)+ 2);
		sprintf(fname_w, "%s/%s", lcachedir_name, scid); /* the filename */
		cid_cache[cid].url = strdup(aurl);
		cid_cache[cid].size = mhs->content_length;
		cid_cache[cid].last_modify = t;
		if(cid_cache[cid].content_type) free(cid_cache[cid].content_type);
		cid_cache[cid].content_type = strdup(mhs->content_type);
		cid_cache[cid].content_encoding = mhs->content_encoding;
		cid_cache[cid].exist = 1;
		fdr = open(fname_r,O_RDONLY);
		fdw = open(fname_w,O_WRONLY | O_CREAT | O_TRUNC,0744);
		while ( (i = read(fdr,buf,CACHE_BUFSIZ)) >0)
			write(fdw, buf, i);
		close (fdw);
		close (fdr);
		free(fname_w);
		return ;
	}
/* there is still data in cache: force the remplacment */
	h = hash_url(aurl);     
	deb = hash_tab[h].head;
	while (deb != NULL){               
		if (strcmp(aurl,deb->url) == 0) {
			cid = deb->cid;
			sprintf(scid,"%ld",cid); 
			fname_w = (char *) malloc(llen_cachedir_name +
				strlen(scid)+ 2);
			sprintf(fname_w, "%s/%s", lcachedir_name, scid); /* the filename */
				/* cid_cache[cid].url = strdup(url); */
			cid_cache[cid].size = mhs->content_length;
			cid_cache[cid].last_modify = t;
			cid_cache[cid].content_type = strdup(mhs->content_type);
			cid_cache[cid].content_encoding = mhs->content_encoding;
			cid_cache[cid].exist = 1;
			fdr = open(fname_r,O_RDONLY);
			fdw = open(fname_w,O_WRONLY | O_CREAT | O_TRUNC,0744);
			while ( (i = read(fdr,buf,CACHE_BUFSIZ)) >0)
				write(fdw, buf, i);
			close (fdw);
			close (fdr);
			free(fname_w);
			return ;
		}
		deb = deb->next;
	}
/* NEVER GOES HERE !!! */
	fprintf(stderr, "This a Bug. Please report\n");
	fprintf(stderr, "MMCachePutDataInCache :Problem in cache\n");
	fprintf(stderr, "Aborting ...\n");
	abort();
	return ;
}

/* Reset the cache. Delete all cached data */
void MMCacheClearCache(void)
{
	char * ocwd;
	int i;
	HashEntry * deb, *next;
	DIR *dirp;
	struct dirent *direntp;

        ocwd = getcwd(NULL,MAXPATHLEN);        
        chdir(lcachedir_name); 
        dirp = opendir( lcachedir_name );
        while ( (direntp = readdir( dirp )) != NULL ) {
                unlink(direntp->d_name);
        }
        (void)closedir( dirp );
        chdir(ocwd);
        free(ocwd);
	lnext_cid =1;

	for( i=0 ; i<MAX_N_FILE_CACHE; i++){
		cid_cache[i].exist = 0;
		if (cid_cache[i].url)
			free(cid_cache[i].url);
		if(cid_cache[i].content_type)
			free(cid_cache[i].content_type);
		cid_cache[i].url = NULL;
		cid_cache[i].size = 0;
		cid_cache[i].last_modify = 0;

		deb = hash_tab[i].head;
		hash_tab[i].count = 0;
		hash_tab[i].head = NULL;
		while ( deb != NULL) {
			next = deb->next;
			free (deb->url);
                        free (deb);
			deb = next;
		}
	}
	memset(cid_cache,0,MAX_N_FILE_CACHE*sizeof(CacheEntry));
	memset(hash_tab,0,MAX_N_FILE_CACHE*sizeof(Bucket));
	MMCacheWriteCache();
}

/* flush the cache.db */
void MMCacheWriteCache(void )
{
	FILE *fp;
	int i;

	if (!(fp = fopen(lcachedb_name, "w")) ) {                                
		return;
	}

	fprintf (fp, "<html>\n");      
	fprintf (fp, "<head><title>mMosaic Cache Index</title></head>\n");
	fprintf (fp, "<body>\n");      
	fprintf (fp, "<ul>\n");        

	 for( i=0 ; i<MAX_N_FILE_CACHE; i++){
		if (! cid_cache[i].url)
			continue;
		fprintf (fp, "<li><a content_type=%s content_encoding=%d size=%u cid=%d mdate=%lu href=\"%s\">%s</a>\n",
			cid_cache[i].content_type,
			cid_cache[i].content_encoding,
			cid_cache[i].size,
			i,
			cid_cache[i].last_modify,
			cid_cache[i].url,
			cid_cache[i].url );
	}
	fprintf (fp, "</ul>\n");       
	fprintf (fp, "</body>\n");     
	fprintf (fp, "</html>\n");     
	fclose(fp);                    
}
