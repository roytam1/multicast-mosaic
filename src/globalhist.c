/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "libhtmlw/HTML.h"
#include "mosaic.h"
#include <time.h>
#include "../libnut/system.h"
#include <memory.h>

extern char *cached_url;

#ifndef DISABLE_TRACE
extern int srcTrace;
extern int cacheTrace;
#endif

/* ---------------------------- GLOBAL HISTORY ---------------------------- */

/* We save history list out to a file (~/.mosaic-global-history) and
   reload it on entry.

   Initially the history file format will look like this:

   ncsa-mosaic-history-format-1            [identifying string]
   Global                                  [title]
   url Fri Sep 13 00:00:00 1986            [first word is url;
                                            subsequent words are
                                            last-accessed date (GMT)]
   [1-line sequence for single document repeated as necessary]
   ...

--Format 2--02/15/96--SWP
   ncsa-mosaic-history-format-2            [identifying string]
   Global                                  [title]
   url seconds                             [first word is url;
                                            subsequent word is the
					    seconds since unix birth
					    at last access]
   [1-line sequence for single document repeated as necessary]
   ...
*/

#define NCSA_HISTORY_FORMAT_COOKIE_ONE "ncsa-mosaic-history-format-1"
#define NCSA_HISTORY_FORMAT_COOKIE_TWO "ncsa-mosaic-history-format-2"
#define HASH_TABLE_SIZE 200

/* Cached data in a hash entry for a given URL; one or both
 * slots can be filled; non-filled slots will be NULL. */
typedef struct cached_data {
	void *image_data;
	char *local_name;
	int last_access;
} cached_data;

/* An entry in a hash bucket, containing a URL (in canonical,
 * absolute form) and possibly cached info (right now, an ImageInfo
 * struct for inlined images). */
typedef struct entry {
	char *url; 		/* Canonical URL for this document. */
	char *lastdate;
			/* This can be one of a couple of things:
			 * for an image, it's the ImageInfo struct;
			 * for an HDF file, it's the local filename (if any) */
	struct entry *next;
	cached_data *cache_data;
} entry;

/* A bucket in the hash table; contains a linked list of entries. */
typedef struct bucket {
	entry *head;
	int count;
} bucket;

static bucket hash_table[HASH_TABLE_SIZE];
static mo_status mo_cache_image_data (cached_data *cd, void *info);
static mo_status mo_uncache_image_data (cached_data *cd);
static int mo_kbytes_in_image_data (void *image_data);
static int notExpired(char *lastdate);
static int hash_url (char *url);
static void dump_bucket_counts (void);
static void add_url_to_bucket (int buck, char *url, char *lastdate);
static int been_here_before (char *url);
static void mo_read_global_history (char *filename);
static mo_status mo_dump_cached_cd_array (void);
static mo_status mo_init_cached_cd_array (void);
static mo_status mo_grow_cached_cd_array (void);
static int mo_sort_cd_for_qsort (const void *a1, const void *a2);
static mo_status mo_sort_cached_cd_array (void);
static mo_status mo_remove_cd_from_cached_cd_array (cached_data *cd);
static mo_status mo_add_cd_to_cached_cd_array (cached_data *cd);
static int mo_kbytes_in_image_data (void *image_data);

static int access_counter = 0;
static int dont_nuke_after_me = 0;
static int kbytes_cached = 0;

/*given a character string of time, is this older than Rdata.urlExpired?*/
static int notExpired(char *lastdate) 
{
	long expired=get_pref_int(eURLEXPIRED)*86400;
	time_t curtime=time(NULL);

	if (expired<=0)
		return(1);
	if ((curtime-atol(lastdate))>=expired) {
		if (srcTrace) {
			fprintf(stderr,"EXPIRED! [%ld] - [%ld] = [%ld] (%ld)\n",
				curtime,atol(lastdate),curtime-atol(lastdate),
				expired);
		}
		return(0);
	}
	if (srcTrace) {
		fprintf(stderr,"NOT EXPIRED! [%ld] - [%ld] = [%ld] (%ld)\n",
			curtime,atol(lastdate),curtime-atol(lastdate),
			expired);
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

/* Each bucket in the hash table maintains a count of the number of
 * entries contained within; this function dumps that information
 * out to stdout. */
static void dump_bucket_counts (void)
{
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++)
		fprintf (stdout, "Bucket %03d, count %03d\n", 
				i, hash_table[i].count);
}

/* Assume url isn't already in the bucket; add it by
 * creating a new entry and sticking it at the head of the bucket's
 * linked list of entries. */
static void add_url_to_bucket (int buck, char *url, char *lastdate)
{
	bucket *bkt = &(hash_table[buck]);
	entry *l;

	l = (entry *)malloc (sizeof (entry));
	l->url = strdup (url);
	l->lastdate=strdup(lastdate);
	l->cache_data = NULL;
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
	char ts[30];

	if (hash_table[hash].count)
		for (l = hash_table[hash].head; l != NULL; l = l->next) {
			if (!strcmp (l->url, url)) {
          			/*we need to update the date -- SWP*/ 
          			sprintf(ts,"%ld",foo); 
 
          			if (l->lastdate) {
                			free(l->lastdate);
          			}
          			l->lastdate=strdup(ts);
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
	char *curl = mo_url_canonicalize (url, "");
	mo_status status;

	if (been_here_before (curl))
		status = mo_succeed;
	else
		status = mo_fail;
	free (curl);
	return status;
}

/*
 * name:    mo_here_we_are_son
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
	char ts[30];

	sprintf(ts,"%ld",foo);
	if (!been_here_before (curl))
		add_url_to_bucket (hash_url (curl), curl, ts);
	free (curl);
	return mo_succeed;
}

/*
 * name:    mo_read_global_history (PRIVATE)
 * purpose: Given a filename, read the file's contents into the
 *          global history hash table.
 * inputs:  
 *   - char *filename: The file to read.
 * returns: 
 *   nothing
 */
static void mo_read_global_history (char *filename)
{
	FILE *fp;
	char line[MO_LINE_LENGTH];
	char *status;
	int format;

	fp = fopen (filename, "r");
	if (!fp)
		return;
	if (get_pref_boolean(eBACKUP_FILES)) {
		char *tf=NULL,retBuf[BUFSIZ];

		tf=(char *)calloc(strlen(filename)+
				strlen(".backup")+5,sizeof(char));
		sprintf(tf,"%s.backup",filename);
		if (my_copy(filename,tf,retBuf,BUFSIZ-1,1)!=SYS_SUCCESS)
			fprintf(stderr,"%s\n",retBuf);
		free(tf);
	}
	status = fgets (line, MO_LINE_LENGTH, fp);
	if (!status || !(*line))
		goto done;
  
	/* See if it's our format. */
	if (strncmp (line, NCSA_HISTORY_FORMAT_COOKIE_ONE,
	    strlen (NCSA_HISTORY_FORMAT_COOKIE_ONE))) {
		if (strncmp (line, NCSA_HISTORY_FORMAT_COOKIE_TWO,
		    strlen(NCSA_HISTORY_FORMAT_COOKIE_TWO))) {
			goto done;
		} else {
			format=2;
		}
	} else {
		format=1;
	}

	/* Go fetch the name on the next line. */
	status = fgets (line, MO_LINE_LENGTH, fp);
	if (!status || !(*line))
		goto done;
  
	/* Start grabbing url's. */
	while (1) {
		char *url;
		char *lastdate;
      
		status = fgets (line, MO_LINE_LENGTH, fp);
		if (!status || !(*line))
			break;
		url = strtok (line, " ");
		if (!url)
			break;
		/* We don't use the last-accessed date... yet. */
		/* We do now... SWP */
		lastdate = strtok (NULL, "\n");
		if (!lastdate)
			break;
		if (notExpired(lastdate) || format==1) {
			add_url_to_bucket (hash_url (url), url, lastdate);
		}
	}
done:
	fclose (fp);
	return;
}

/* name:    mo_init_global_history
 * purpose: Initialize the global history hash table.
 * inputs:  
 *   none
 * returns: 
 *   mo_succeed
 */
mo_status mo_init_global_history (void)
{
	int i;

	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		hash_table[i].count = 0;
		hash_table[i].head = 0;
	}
	return mo_succeed;
}

/*
 * name:    mo_wipe_global_history
 * purpose: Wipe out the current global history.
 * inputs:  
 *   none
 * returns: 
 *   mo_succeed
 * remarks: 
 *   Huge memory hole here.  However, we now call
 *   mo_flush_image_cache to at least clear out the image structures.
 */
mo_status mo_wipe_global_history (mo_window *win)
{
	mo_flush_image_cache (win);

	mo_init_global_history ();	/* Memory leak! @@@ */
	return mo_succeed;
}

/*
 * name:    mo_setup_global_history
 * purpose: Called on program startup to do the global history
 *          initialization stuff, including figuring out where the
 *          global history file is and reading it.
 * inputs:  
 *   none
 * returns: 
 *   mo_succeed
 */
static char *cached_global_hist_fname = NULL;
mo_status mo_setup_global_history (void)
{
	char *home = getenv ("HOME");
	char *default_filename = get_pref_string(eGLOBAL_HISTORY_FILE);
	char *filename;
	FILE *fp;

	mo_init_global_history ();

	if (!home)		 /* This shouldn't happen. */
		home = "/tmp";
  
	filename = (char *)malloc((strlen (home) +
			strlen (get_pref_string(eHISTORY_FILE)) + 8));
	sprintf (filename, "%s/%s", home, get_pref_string(eHISTORY_FILE));

	if (!(fp=fopen(filename,"r"))) {
		printf("\n\n---------------New History Format---------------\n\n");
		printf("Mosaic needs to update your history file to a new format\n");
		printf("  which will enable links to expire after %d days (see\n",get_pref_int(eURLEXPIRED));
		printf("  the resource 'Mosaic*urlExpired').\n\n");
		printf("Your current history file will still exist and will not\n");
		printf("  be modified. However, it will no longer be updated.\n");
		printf("  Instead, the file '.mosaic-x-history' will be used.\n\n");
		free(filename);
		filename = (char *)malloc(strlen (home) + 
			strlen (default_filename) + 8 );
		sprintf (filename, "%s/%s", home, default_filename);
	} else {
		fclose(fp);
	}
	cached_global_hist_fname = filename;
	mo_read_global_history (filename);
	free(filename);
	filename = (char *)malloc(strlen (home) +
		strlen (get_pref_string(eHISTORY_FILE)) + 8);
	sprintf (filename, "%s/%s", home, get_pref_string(eHISTORY_FILE));
	cached_global_hist_fname = filename;
	return mo_succeed;
}

/*
 * name:    mo_write_global_history
 * purpose: Write the global history file out to disk.
 * inputs:  
 *   none
 * returns: 
 *   mo_succeed (usually)
 * remarks: 
 *   This assigns last-read times to all the entries in the history,
 *   which is a bad thing.
 *   ---Not anymore --- SWP
 */
mo_status mo_write_global_history (void)
{
	FILE *fp;
	int i;
	entry *l;
	time_t foo = time (NULL);
	char ts[30];

	sprintf(ts,"%ld",foo);
	fp = fopen (cached_global_hist_fname, "w");
	if (!fp)
		return mo_fail;
	fprintf (fp, "%s\n%s\n", NCSA_HISTORY_FORMAT_COOKIE_TWO, "Global");
	for (i = 0; i < HASH_TABLE_SIZE; i++) {
		for (l = hash_table[i].head; l != NULL; l = l->next) {
			fprintf (fp, "%s %s\n", l->url, 
				   (isdigit(*(l->lastdate))?l->lastdate:ts));
		}
	}
	fclose (fp);
	return mo_succeed;
}

/*
 * name:    mo_fetch_cached_image_data
 * purpose: Retrieve a piece of cached data associated with a URL.
 * inputs:  
 *   - char *url: The URL.
 * returns: 
 *   The piece of cached data (void *).
 * remarks: 
 *   We do *not* do anything to the URL.  If there is a target
 *   anchor in it, fine with us.  This means the target anchor
 *   should have been stripped out someplace else if it needed to be.
 */
ImageInfo *mo_fetch_cached_image_data (char *url)
{
	int hash = hash_url (url);
	entry *l;

	if (!hash_table[hash].count)
		return NULL;
	for (l = hash_table[hash].head; l != NULL; l = l->next) {
		if (!strcmp (l->url, url)) {
			if (l->cache_data && l->cache_data->image_data) {
				if (cacheTrace)
					fprintf(stderr,
						"[mo_fetch_cached_image_data] Hit for '%s', data 0x%08x\n",
						url, l->cache_data->image_data);
				l->cache_data->last_access = access_counter++;
				return (ImageInfo *)l->cache_data->image_data;
			} 
			return NULL;
		}
	}
	return NULL;
}

/*
 * name:    mo_fetch_cached_local_name
 * purpose: Retrieve a piece of cached data associated with a URL.
 * inputs:  
 *   - char *url: The URL.
 * returns: 
 *   The piece of cached data (void *).
 * remarks: 
 *   We do *not* do anything to the URL.  If there is a target
 *   anchor in it, fine with us.  This means the target anchor
 *   should have been stripped out someplace else if it needed to be.
 */
void *mo_fetch_cached_local_name (char *url)
{
	int hash = hash_url (url);
	entry *l;

	if (hash_table[hash].count)
		for (l = hash_table[hash].head; l != NULL; l = l->next) {
			if (!strcmp (l->url, url)) {
				if (l->cache_data)
					return l->cache_data->local_name;
				else
					return NULL;
			}
		}
	return NULL;
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
	char ts[30];

	sprintf(ts,"%ld",foo);

/* First, register ourselves if we're not already registered.
 * Now, the same URL can be registered multiple times with different
 * (or, in one instance, no) internal anchor. */
	if (!been_here_before (url))
		add_url_to_bucket (hash_url (url), url,ts);

/* Then, find the right entry. */
	if (hash_table[hash].count)
		for (l = hash_table[hash].head; l != NULL; l = l->next) {
			if (!strcmp (l->url, url))
				goto found;
		}
	return mo_fail;

found:
  	if (!l->cache_data) {
		l->cache_data = (cached_data *)malloc (sizeof (cached_data));
		l->cache_data->image_data = NULL;
		l->cache_data->local_name = NULL;
		l->cache_data->last_access = 0;
	}
	if (type == 0) {
		if (cacheTrace)
			fprintf (stderr,
				"[mo_cache_data] Caching '%s', data 0x%08x\n",
				url, info);
		mo_cache_image_data (l->cache_data, info);
	} else 
		if (type == 1)
			l->cache_data->local_name = (char*)info;
	return mo_succeed;
}

mo_status mo_zap_cached_images_here (mo_window *win)
{
	char **hrefs;
	int num;
	void *ptr;

	/* Go fetch new hrefs. */
	hrefs = HTMLGetImageSrcs (win->scrolled_win, &(num));
	if (num) {
		int i;
		for (i = 0; i < num; i++) {
			char *url = mo_url_canonicalize (hrefs[i], cached_url);
			ptr = mo_fetch_cached_image_data (url);
			if (ptr)
				mo_cache_data (url, NULL, 0);
		}
		/* All done; clean up. */
		for (i = 0; i < num; i++)
			free (hrefs[i]);
		free (hrefs);
	}
	return mo_succeed;
}

/* name:    mo_flush_image_cache
 * purpose: 
 * inputs:  
 *   - mo_window *win: The current window.
 * returns: 
 *   nuthin
 * remarks: 
 */
mo_status mo_flush_image_cache (mo_window *win)
{
	entry *l;
	int hash;

	for (hash = 0; hash < HASH_TABLE_SIZE; hash++)
		for (l = hash_table[hash].head; l != NULL; l = l->next)
			if (l->cache_data)
				if (l->cache_data->image_data)
					mo_uncache_image_data (l->cache_data);
	return mo_succeed;
}

/* ------------------------- decent image caching ------------------------- */

/* CHUNK_OF_IMAGES determines the initial size of the array of cached
   pointers to image data; if more images must be cached, the array is
   grown with realloc by this amount.  It is good to keep the array as
   small as possible, as it must occasionally be sorted. */
#define CHUNK_OF_IMAGES 10

static cached_data **cached_cd_array = NULL;
static int num_in_cached_cd_array = 0;
static int size_of_cached_cd_array = 0;

static mo_status mo_dump_cached_cd_array (void)
{
  int i;
  if (!cached_cd_array) {
	if (cacheTrace)
		fprintf (stderr, "[mo_dump_cached_cd_array] No array; punting\n");
	return mo_fail;
  }
  if (cacheTrace)
	fprintf (stderr, "+++++++++++++++++++++++++\n");
  for (i = 0; i < size_of_cached_cd_array; i++) {
      if (cached_cd_array[i])
	if (cacheTrace) {
		fprintf (stderr, "  %02d data 0x%08x last_access %d\n", i,
			 cached_cd_array[i]->image_data, 
			 cached_cd_array[i]->last_access);
	} else if (cacheTrace) {
		fprintf (stderr, "  %02d NULL\n", i);
	}
  }
  if (cacheTrace)
	fprintf (stderr, "---------------------\n");
  return mo_succeed;
}

static mo_status mo_init_cached_cd_array (void)
{
	cached_cd_array = (cached_data **)malloc (sizeof (cached_data *) * 
				CHUNK_OF_IMAGES);
	size_of_cached_cd_array += CHUNK_OF_IMAGES;
	if (cacheTrace)
		fprintf (stderr, "[mo_init] Did it 0x%08x -- allocated %d pointers.\n",
			cached_cd_array, size_of_cached_cd_array);
	memset((char *)cached_cd_array, 0, 
		CHUNK_OF_IMAGES * sizeof (cached_cd_array[0]));
	return mo_succeed;
}

static mo_status mo_grow_cached_cd_array (void)
{
  
  cached_cd_array = (cached_data **)realloc 
    (cached_cd_array,
     sizeof (cached_data *) * (size_of_cached_cd_array + CHUNK_OF_IMAGES));

  if (cacheTrace)
    fprintf (stderr, "[grow] cached_cd_array 0x%08x, size_of_cached_cd_array 0x%08x, sum 0x%08x\n",
             cached_cd_array, size_of_cached_cd_array, 
             cached_cd_array + size_of_cached_cd_array);
  memset((char *)(cached_cd_array + size_of_cached_cd_array), 0,
         CHUNK_OF_IMAGES * sizeof (cached_cd_array[0]));

  size_of_cached_cd_array += CHUNK_OF_IMAGES;
  return mo_succeed;
}

static int mo_sort_cd_for_qsort (const void *a1, const void *a2)
{
	cached_data **d1 = (cached_data **)a1;
	cached_data **d2 = (cached_data **)a2;

/* NULL entries will be at end of array -- this may be good,
 * or may not be -- hmmmmmm. */
	if (!d1 || !(*d1))
		return 1;
	if (!d2 || !(*d2))
		return -1;

	if (cacheTrace)
		fprintf (stderr, "sort: hi there! %d %d\n",
			(*d1)->last_access, (*d2)->last_access);
	return ((*d1)->last_access < (*d2)->last_access ? -1 : 1);
}

static mo_status mo_sort_cached_cd_array (void)
{
	if (!cached_cd_array) {
		if (cacheTrace)
			fprintf (stderr, "[mo_sort_cached_cd_array] No array; punting\n");
		return mo_fail;
	}

	if (num_in_cached_cd_array == 0) {
		if (cacheTrace)
			fprintf (stderr, "[mo_sort_cached_cd_array] Num in array 0; punting\n");
		return mo_fail;
	}

	if (cacheTrace) {
		fprintf (stderr, "[mo_sort_cached_cd_array] Sorting 0x%08x!\n",
			cached_cd_array);
		mo_dump_cached_cd_array ();
	}

	qsort ((void *)cached_cd_array, size_of_cached_cd_array, 
			sizeof (cached_cd_array[0]), mo_sort_cd_for_qsort);

	if (cacheTrace)
		mo_dump_cached_cd_array ();
		return mo_succeed;
}

static mo_status mo_remove_cd_from_cached_cd_array (cached_data *cd)
{
	int i, freed_kbytes = 0;
  
	if (!cached_cd_array)
		return mo_fail;

	for (i = 0; i < size_of_cached_cd_array; i++) {
		if (cached_cd_array[i] == cd) {
			if (cacheTrace)
				fprintf (stderr, 
	"[mo_remove_cd_from_cached_cd_array] Found data 0x%08x, location %d\n", 
					cached_cd_array[i]->image_data, i);
			freed_kbytes = mo_kbytes_in_image_data (cached_cd_array[i]->image_data);
			mo_free_image_data (cached_cd_array[i]->image_data);
			cached_cd_array[i] = NULL;
			num_in_cached_cd_array--;
			kbytes_cached -= freed_kbytes;
			return mo_succeed;
		}
	}
	if (cacheTrace)
		fprintf (stderr,"[mo_remove_cd] UH OH, DIDN'T FIND IT!!!!!\n");
	return mo_fail;
}

static mo_status mo_add_cd_to_cached_cd_array (cached_data *cd)
{
	int i, num;
	int kbytes_in_new_image = mo_kbytes_in_image_data (cd->image_data);
  
	if (!cached_cd_array) {
		mo_init_cached_cd_array ();
		if (cacheTrace)
			fprintf (stderr, "[mo_add_cd] Init'd cached_cd_array.\n");
	} else {
			/* Maybe it's already in there. */
		for (i = 0; i < size_of_cached_cd_array; i++) {
			if (cached_cd_array[i] == cd)
				return mo_succeed;
		}
	}

/* Here's the magic part. */
	if((kbytes_cached+kbytes_in_new_image)>get_pref_int(eIMAGE_CACHE_SIZE)) {
		int num_to_remove = 0;
		if (cacheTrace)
			fprintf (stderr, "[mo_add_cd] Going to sort 0x%08x...\n", 
				cached_cd_array);
		mo_sort_cached_cd_array ();
		if (cacheTrace) {
			fprintf (stderr, "[mo_add_to] Just sorted in preparation for purging...\n");
			mo_dump_cached_cd_array ();
		}
		while((kbytes_cached + kbytes_in_new_image) > get_pref_int(eIMAGE_CACHE_SIZE)) {
			if (cacheTrace)
				fprintf (stderr, "[mo_add_cd] Trying to free another image (%d > %d).\n",
					(kbytes_cached + kbytes_in_new_image),
					get_pref_int(eIMAGE_CACHE_SIZE));

/* Try to remove one -- we rely on the fact that NULL
 * entries in cached_cd_array are at the end of the array. */
			if (num_to_remove < size_of_cached_cd_array &&
			    cached_cd_array[num_to_remove] != NULL) {
				if (cacheTrace)
					fprintf (stderr, "        ** going to try to remove %d; last_access %d < dont_nuke_after_me %d??\n",
					num_to_remove,
					cached_cd_array[num_to_remove]->last_access,
					dont_nuke_after_me);
				if (cached_cd_array[num_to_remove]->last_access < dont_nuke_after_me) {
					if (cacheTrace)
						fprintf (stderr, "        ** really removing %d\n",
							num_to_remove);
					mo_uncache_image_data (cached_cd_array[num_to_remove]);
					if (cacheTrace)
						mo_dump_cached_cd_array ();
				}
				num_to_remove++;
			} else {
				if (cacheTrace)
					fprintf (stderr, "        ** no more to remove\n");
				if (cacheTrace)
					mo_dump_cached_cd_array ();
				goto removed_em_all;
			}
		}
	}
removed_em_all:
	if (num_in_cached_cd_array == size_of_cached_cd_array) {
		if (cacheTrace)
			fprintf (stderr, "[mo_add_cd] Growing array... \n");
		num = size_of_cached_cd_array;
		mo_grow_cached_cd_array ();
	} else {
		num = -1;
		for (i = 0; i < size_of_cached_cd_array; i++) {
			if (cached_cd_array[i] == NULL) {
				num = i;
				goto got_num;
			}
		}
		if (cacheTrace)
			fprintf (stderr, "[mo_add_cd_to_cached_cd_array] UH OH couldn't find empty slot\n");
/* Try to grow array -- flow of control should never reach here, though. */
		num = size_of_cached_cd_array;
		mo_grow_cached_cd_array ();
	}
got_num:
	cached_cd_array[num] = cd;
	num_in_cached_cd_array++;
	kbytes_cached += kbytes_in_new_image;

	if (cacheTrace) {
		fprintf (stderr, "[mo_add_cd_to_cached_cd_array] Added cd, data 0x%08x, num %d\n",
			cd->image_data, num);
		fprintf (stderr, "[mo_add_cd_to_cached_cd_array] Now cached %d kbytes.\n", kbytes_cached);
		mo_dump_cached_cd_array ();
	}
	return mo_succeed;
}

static int mo_kbytes_in_image_data (void *image_data)
{
	ImageInfo *img = (ImageInfo *)image_data;
	int bytes, kbytes;

	if (!img)
		return 0;
	bytes = img->width * img->height;
	kbytes = bytes >> 10;

	if (cacheTrace)
		fprintf(stderr, "[mo_kbytes_in_image_data] bytes %d, kbytes %d\n",
			bytes, kbytes);
	if (kbytes == 0)
		kbytes = 1;
	return kbytes;
}

static mo_status mo_cache_image_data (cached_data *cd, void *info)
{
	/* Beeeeeeeeeeeeeeeeee smart! */
	if (get_pref_int(eIMAGE_CACHE_SIZE) <= 0)
		set_pref(eIMAGE_CACHE_SIZE, (void *)1);

	/* It's possible we'll be getting NULL info here, so we
		should uncache in this case... */
	if (!info)
		mo_uncache_image_data (cd);
	cd->image_data = info;
	cd->last_access = access_counter++;
	mo_add_cd_to_cached_cd_array (cd);
	return mo_succeed;
}

static mo_status mo_uncache_image_data (cached_data *cd)
{
mo_remove_cd_from_cached_cd_array (cd);
cd->image_data = NULL;
return mo_succeed;
}

mo_status mo_set_image_cache_nuke_threshold (void)
{
	if (cacheTrace)
		fprintf (stderr, "[mo_set_nuke_threshold] Setting to %d\n",
			access_counter);
	dont_nuke_after_me = access_counter;
	return mo_succeed;
}
