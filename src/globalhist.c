/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <time.h>
#include <memory.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "../libhtmlw/HTMLparse.h"
#include "../libmc/mc_defs.h"
#include "libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../libhtmlw/HTMLPutil.h"
#include "mosaic.h"
#include "../libnut/system.h"

#define HASH_TABLE_SIZE 200

/* An entry in a hash bucket, containing a URL (in canonical,
 * absolute form)  */

typedef struct entry {
	char *url; 		/* Canonical URL for this document. */
	int last_visited;
	struct entry *next;
} entry;

/* A bucket in the hash table; contains a linked list of entries. */
typedef struct bucket {
	entry *head;
	int count;
} bucket;

static bucket hhash_table[HASH_TABLE_SIZE];
static int been_here_before (char *url);

/*given a character string of time, is this older than urlExpired?*/
static int notExpired(char *lastdate) 
{
	long expired= mMosaicAppData.urlExpired*86400;
	time_t curtime=time(NULL);

	if (expired<=0)
		return(1);
	if ((curtime-atol(lastdate))>=expired) {
		return(0);
	}
	return(1);
}

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
 * creating a new entry and sticking it at the head of the bucket's
 * linked list of entries. */
static void add_url_to_bucket (int buck, char *url, int lastdate)
{
	bucket *bkt = &(hhash_table[buck]);
	entry *l;

	l = (entry *)malloc (sizeof (entry));
	l->url = strdup (url);
	l->last_visited=lastdate;
	l->next = NULL;
	if (bkt->head == NULL)
		bkt->head = l;
	else {
		l->next = bkt->head;
		bkt->head = l;
	}
	bkt->count++ ;
}

/* This is the internal predicate that takes a URL, hashes it,
 * does a search through the appropriate bucket, and either returns
 * 1 or 0 depending on whether we've been there. */
static int been_here_before (char *url)
{
	int hash = hash_url (url);
	entry *l;
	time_t foo = time (NULL);

	for (l = hhash_table[hash].head; l != NULL; l = l->next) {
		if (!strcmp (l->url, url)) {
       			l->last_visited=foo;
			return 1;
		}
	}
	return 0;
}

/* name:    mo_been_here_before_huh_dad
 * purpose: Predicate to determine if we've visited this URL before.
 * inputs:  
 *   - char *url: The URL.
 * returns: 
 *   mo_succeed if we've been here before; mo_fail otherwise
 * remarks: 
 *   We canonicalize the URL (stripping out the target anchor, 
 *   if one exists).
 */
mo_status mo_been_here_before_huh_dad (char *url)
{
	mo_status status;

	if (been_here_before (url))
		status = mo_succeed;
	else
		status = mo_fail;
	return status;
}

/* name:    mo_here_we_are_son
 * purpose: Add a URL to the global history, if it's not already there.
 * inputs:  
 *   - char *url: URL to add.
 * returns: 
 *   mo_succeed
 * remarks: 
 *   We canonicalize the URL (stripping out the target anchor, 
 *   if one exists).
 */
mo_status mo_here_we_are_son (char *url)
{
	char *curl = mo_url_canonicalize (url, "");
	time_t foo = time (NULL);

	if (!been_here_before (curl))
		add_url_to_bucket (hash_url (curl), curl, foo);
	free (curl);
	return mo_succeed;
}

/* name:    MMInitHistory
 * purpose: Initialize the global history hash table.
 * inputs:  
 *   ~/.mMosaic
 */
static char * lroot_dirname = NULL;     /* the root directory name */
static char * lhistory_name = NULL;     /* lroot_dirname/cache.db */
static int lfd_history = -1;            /* fd for cache.db */
static int lhistoryttl = 0;		/* in number of second */

void MMInitHistory (char* mmosaic_root_dir)
{
	int l, i;
	struct stat s;
	char *data = NULL;
	struct mark_up * mlist;
	struct mark_up * mptr;
	time_t t, ldate;
	char *href, *sldate;

/* creer le fichier ~/.mMosaic/history.html */

        lroot_dirname = strdup(mmosaic_root_dir);

        l = strlen(mmosaic_root_dir);
        lhistory_name = (char*) malloc(l + 15);
        strcpy(lhistory_name,lroot_dirname);
        strcat(lhistory_name,"/history.html");
	if (stat(lhistory_name,&s) !=0 ) { /* create empty history.html */
		lfd_history = creat(lhistory_name, 0644);
		write(lfd_history,"<html></html>\n",15 );
		close(lfd_history);
	}
	lfd_history = open(lhistory_name, O_RDWR , 0644);
	stat(lhistory_name, &s);
	data = (char*) malloc(s.st_size+1);
	l = read(lfd_history, data, s.st_size);
	data[s.st_size] = '\0';
	close(lfd_history);

/* read the data base */
	mptr = mlist = HTMLParse(data);
	t = time(NULL);
	lhistoryttl = mMosaicAppData.urlExpired*86400;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hhash_table[i].count = 0;
		hhash_table[i].head = 0;
	}
	while (mptr != NULL) {
		if (mptr->type == M_ANCHOR && !mptr->is_end) {
			href = ParseMarkTag(mptr->start, MT_ANCHOR, "href");
			sldate = ParseMarkTag(mptr->start, MT_ANCHOR, "last_visited");
			ldate = atoi(sldate);		/* en second */
			if( !(t - ldate > lhistoryttl)) { /* history expired? */
				add_url_to_bucket (hash_url(href), href, ldate);
			}
			free(href);
			free(sldate);
		}
		mptr = mptr->next;
	}
	FreeMarkUpList(mlist);
	free(data);
}

void MMWriteHistory()
{
	FILE *fp;
	int i; 
	entry *l;

	if (!(fp = fopen(lhistory_name, "w")) ) {
		return;
	}
	fprintf (fp, "<html>\n");
	fprintf (fp, "<head><title>mMosaic History Index</title></head>\n");
	fprintf (fp, "<body>\n");
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		for (l = hhash_table[i].head; l != NULL; l = l->next) {
			fprintf (fp, "<a href=%s last_visited=%d>%s</a>\n",
				l->url,
				l->last_visited,
				l->url);
		}
	}
	fprintf (fp, "</body>\n");
	fprintf (fp, "</html>\n");
	fclose(fp);
}

/* name:    mo_wipe_global_history
 * purpose: Wipe out the current global history.
 * inputs:  
 *   win
 * remarks: 
 *   Huge memory hole here. 
 */
void mo_wipe_global_history (mo_window *win)
{
	MMInitHistory (mMosaicRootDirName);	/* Memory leak! @@@ */
}

/****************************************************************************
 * name:    mo_cache_data
 * purpose: Cache a piece of data associated with a given URL.
 * inputs:  
 *   - char  *url: The URL.
 *   - void *info: The piece of data to cache (currently either
 *                 an ImageInfo struct for an image named as SRC
 *                 in an IMG tag, or the filename corresponding to the
 *                 local copy of a remote HDF file).
 *   - int   type: The type of data to cache (currently either
 *                 0 for an ImageInfo struct or 1 for a local name).
 * returns: 
 *   mo_succeed, unless something goes badly wrong
 * remarks: 
 *   We do *not* do anything to the URL.  If there is a target
 *   anchor in it, fine with us.  This means the target anchor
 *   should have been stripped out someplace else if it needed to be.
 ****************************************************************************/
mo_status mo_cache_data (char *url, void *info, int type)
{
	int hash = hash_url (url);
	entry *l;
	time_t foo = time (NULL);


/* First, register ourselves if we're not already registered.
 * Now, the same URL can be registered multiple times with different
 * (or, in one instance, no) internal anchor. */
	if (!been_here_before (url))
		add_url_to_bucket (hash_url (url), url,foo);

/* Then, find the right entry. */
	if (hhash_table[hash].count)
		for (l = hhash_table[hash].head; l != NULL; l = l->next) {
			if (!strcmp (l->url, url))
				return mo_succeed;
		}
	return mo_fail;
}
