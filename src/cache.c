/* G.Dauphin 25/7/97 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/uio.h>
#include <unistd.h>
#include <dirent.h>

#include "../libhtmlw/HTMLparse.h"
#include "../libmc/mc_defs.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../libhtmlw/HTMLPutil.h"
#include "mo-www.h"
#include "cache.h"

typedef struct _CacheEntry {
	char *url;              /* Canonical URL for this document. */
	int exist;		/* Is this entry in used ? */
	time_t last_modify;
	int size;
} CacheEntry;

/* An entry in a hash bucket, containing a URL (in canonical,
 * absolute form)
*/
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


/* Given a URL, hash it and return the hash value, mod'd by the size
 * of the hash table. */
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

/* Assume url isn't already in the bucket; add it by
 * creating a new entry. */     
static void add_url_to_bucket (int buck, char *url, int cid)
{ 
        Bucket *bkt = &(hash_tab[buck]);
        HashEntry *l;
 
        l = (HashEntry *)malloc (sizeof (HashEntry));
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

static void remove_url_to_bucket(int buck, char * url)
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
			}
			prev->next = next;
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
	mptr = mlist = HTMLParse(data);
	t = time(NULL);
	hcid =0;
/*	allouer et regarder le nombre de fichier max pour le cache (1000)
	allouer la structure. cache_entry[MAX_N_FILE_CACHE]
*/
	cid_cache = (CacheEntry *) malloc( MAX_N_FILE_CACHE * sizeof(CacheEntry));
/*	creer une table hash-code pour les url et donnant le cid. */

	hash_tab = (Bucket *) malloc( MAX_N_FILE_CACHE * sizeof(Bucket));
	for( i=0 ; i<MAX_N_FILE_CACHE; i++){
		cid_cache[i].exist = 0;
		cid_cache[i].url = NULL;
		hash_tab[i].head = NULL;
		hash_tab[i].count = 0;
	}
	while (mptr != NULL) {
		char * fname;
		int h;

		if (mptr->type == M_ANCHOR && !mptr->is_end) {
			href = ParseMarkTag(mptr->start, MT_ANCHOR, "href");
			scid = ParseMarkTag(mptr->start, MT_ANCHOR, "cid");
			fname = (char*) malloc(llen_cachedir_name +
				strlen(scid) + 2);
			sprintf(fname,"%s/%s",lcachedir_name,scid);
			cid = atoi(scid);
			status = stat(fname, &s);
			if ( status || cid >= MAX_N_FILE_CACHE || cid <= 0 ) { /* nofile */
				unlink(fname);
				free(fname);
				free(scid);
				free(href);
				continue;
			}
			if ( t - s.st_mtime  > lcachettl) { /* look at  date; */
				unlink(fname);
				free(fname);
				free(scid);
				free(href);
				continue;
			}
			/* date of file is in ttl */
			hcid = hcid > cid ? hcid : cid;	/* hightest cid */
			cid_cache[cid].url = strdup(href);
			cid_cache[cid].exist = 1;
			cid_cache[cid].last_modify = s.st_mtime;
			cid_cache[cid].size = s.st_size;

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

	ocwd = getcwd(NULL,0);
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

/* -input :
	the absolute url to fetch in the cache
   - output :
	fname_ret is the filename containing the data.
	fname_ret is allocated
   - return the size of the file
*/
int MMCacheFetchCachedData(char * url , char ** fname_ret)
{
	int status = 0;
	int h;
	HashEntry * deb;
	time_t t = time(NULL);
	char scid[40];
	int cid;

	h = hash_url(url);
	deb = hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(url,deb->url) == 0) {
			cid = deb->cid;
			sprintf(scid,"%ld",cid);
			*fname_ret = (char *) malloc(llen_cachedir_name +
						strlen(scid)+ 2);
			sprintf(*fname_ret, "%s/%s", lcachedir_name, scid); /* the filename */
			status = cid_cache[cid].size; 
			return status;
		}
		deb = deb->next;
	}
	*fname_ret = NULL;
	return 0;
}

/* - input :
	data, the data to cache in a file.
	len, the len of data.
	url, the absolute url.
   - output :
	fname_ret, the name of the file containing the cached data.
   - return the len of the file (same as len or 0)
*/
int MMCachePutDataInCache(char * data, int len, char * url, char ** fname_ret)
{
	HashEntry * deb;
	char scid[40];
	int cid, lenget, h, fd;
	time_t t = time(NULL);

	lenget = MMCacheFetchCachedData(url,fname_ret);
	if (lenget == 0){
		h = hash_url(url);
		cid = lnext_cid;
		if (cid_cache[cid].url) {
			remove_url_to_bucket(hash_url(cid_cache[cid].url), cid_cache[cid].url);
			free(cid_cache[cid].url);
		}
		lnext_cid++;
		if ( lnext_cid >= MAX_N_FILE_CACHE)
			lnext_cid = 1;
		add_url_to_bucket(h, url, cid);
		sprintf(scid,"%ld",cid); 
		*fname_ret = (char *) malloc(llen_cachedir_name +
			strlen(scid)+ 2);
		sprintf(*fname_ret, "%s/%s", lcachedir_name, scid); /* the filename */
		cid_cache[cid].url = strdup(url);
		cid_cache[cid].size = len;
		cid_cache[cid].last_modify = t;
		cid_cache[cid].exist = 1;
		fd = open(*fname_ret,O_WRONLY | O_CREAT | O_TRUNC,0744);
		write(fd,data,len);
		close (fd);
		return len;
	}
/* there is still data in cache: force the remplacment */
	h = hash_url(url);     
	deb = hash_tab[h].head;
	while (deb != NULL){               
		if (strcmp(url,deb->url) == 0) {
			cid = deb->cid;
			sprintf(scid,"%ld",cid); 
			*fname_ret = (char *) malloc(llen_cachedir_name +
			strlen(scid)+ 2);
			sprintf(*fname_ret, "%s/%s", lcachedir_name, scid); /* the filename */
				/* cid_cache[cid].url = strdup(url); */
			cid_cache[cid].size = len;
			cid_cache[cid].last_modify = t;
			cid_cache[cid].exist = 1;
			fd = open(*fname_ret,O_WRONLY | O_CREAT | O_TRUNC,0744);
			write(fd,data,len);
			close (fd);
			return len;
		}
		deb = deb->next;
	}
/* NEVER GOES HERE !!! */
	abort();
	*fname_ret = NULL;
	return 0;
}

/*
   Force the load and cache the data

   - input :
	url, get the data from this url throught libwww.
   - output :
	fname_ret, the data go in this file
	ldata_ret, the size of the file.
   - return a status like 'mo_pull_er_over_virgin'
*/
int MMCacheGetDataAndCache(char * url, char ** fname_ret, int * ldata_ret, mo_window * win)
{
	int len_ret, cid, rv;
	struct stat s;

/* put temp data to create a file and entry in cache */
	len_ret = MMCachePutDataInCache("", 1, url, fname_ret);

/* fname is now the file where to put data */
	rv = mo_pull_er_over_virgin(url, *fname_ret, win);

/* now adjust the size */
	stat(*fname_ret, &s);
	cid = atoi( strrchr(*fname_ret,'/') + 1);
	*ldata_ret = cid_cache[cid].size = s.st_size;
	return rv;
}

/* Reset the cache. Delete all cached data */
void MMCacheClearCache(void)
{
	char * ocwd;
	int i;
	HashEntry * deb, *next;
	DIR *dirp;
	struct dirent *direntp;

        ocwd = getcwd(NULL,0);        
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
		fprintf (fp, "<li><a size=%u cid=%d mdate=%u href=\"%s\">%s</a>\n",
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
