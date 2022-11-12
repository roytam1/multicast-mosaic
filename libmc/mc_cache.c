/* Multicast cache stuff  G.D 22 Mars 1999 */

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
#include <assert.h>


#include "../libhtmlw/HTMLparse.h"
#include "../libhtmlw/HTML.h"
#include "../libhtmlw/HTMLP.h"
#include "../libhtmlw/HTMLPutil.h"
#include "mc_mosaic.h"

/* HASH value for sender and receiver is same */
#define HASH_TABLE_SIZE	(997)
#define INIT_N_FILE_CACHE (1024)
#define INIT_N_STATE_CACHE (1024)

#define CACHE_BUFSIZ 8192

static char * mc_root_dirname = NULL;	/* the root directory name */
static char * mc_cachedir_name = NULL;	/* lroot_dirname/mcache */
static char * mc_sender_cachedir_name=NULL; /* lroot_dirname/mcache/me */
static int mc_len_cachedir_name = 0;
static int mc_sender_len_cachedir_name = 0;
static McBucket * sender_hash_tab;

static int moid_sender_cache_size = 0;

McObjectStruct * moid_sender_cache;
McStateStruct * mc_sender_state_tab = NULL;
int mc_sender_state_tab_size = 0;

/*static int cachettl   = 4 * 24 * 3600; */	/* 4 days by default */

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
/* the creation is in the correct cache: sender or receiver cache */
static void add_url_to_bucket (McBucket * hash_tab, int buck, char *url, int moid)
{ 
        McBucket *bkt = &(hash_tab[buck]);
        McHashEntry *l;
 
        l = (McHashEntry *)calloc (1, sizeof (McHashEntry));
        l->aurl = strdup (url);
        l->moid=moid;
        l->next = bkt->head;     
        bkt->head = l;           
        bkt->count++ ;                   
}

/* initialize the cache in ~/.mMosaic */
/* check for the dirname/mcache/me directory */
/* the default dirname is ~/.mMosaic */
/* else create */
void McSenderCacheInit( char * root_name)
{
	int i, l;
	struct stat s;
	char *ocwd;
	DIR *dirp;
	struct dirent *direntp;

	mc_root_dirname = strdup(root_name);	/* ~/.mMosaic must exit */

	l = strlen(mc_root_dirname);
	mc_cachedir_name = (char*) calloc(l+12, 1);
	strcpy(mc_cachedir_name,mc_root_dirname);
	strcat(mc_cachedir_name,"/mcache");
	mc_len_cachedir_name = strlen(mc_cachedir_name);

	mc_sender_cachedir_name = (char*) malloc(mc_len_cachedir_name + 12);
	strcpy(mc_sender_cachedir_name,mc_cachedir_name);
	strcat(mc_sender_cachedir_name,"/me");
	mc_sender_len_cachedir_name = strlen(mc_sender_cachedir_name);

	if ( stat(mc_root_dirname,&s) != 0 ) {
		fprintf (stderr, "Multicast Cache directory not found.\n");
		fprintf (stderr, "This is mandatory in Multicast context. Aborting...\n");
		free(mc_root_dirname);
		mc_root_dirname = NULL;
		assert(0);
	}

/* read the cache or make one */
	if(stat(mc_cachedir_name,&s) != 0) { /* create dircache ~mMosaic/mcache*/
		mkdir(mc_cachedir_name,0755);
	}
	if(stat(mc_sender_cachedir_name,&s) != 0) { /* create dircache ~mMosaic/mcache/me */
		mkdir(mc_sender_cachedir_name,0755);
	}

/* alloc an initiale moid_cache */
	moid_sender_cache = (McObjectStruct *)calloc(INIT_N_FILE_CACHE, sizeof(McObjectStruct));
	moid_sender_cache_size = INIT_N_FILE_CACHE;

/* create a hash-code table and a reverse moid table */
	for( i=0 ; i<moid_sender_cache_size; i++){
		moid_sender_cache[i].moid = -1;
	}
	sender_hash_tab = (McBucket *) calloc(HASH_TABLE_SIZE, sizeof(McBucket));

	/* read the dir cache remove files */

	ocwd = getcwd(NULL,MAXPATHLEN);
	chdir(mc_sender_cachedir_name);
	dirp = opendir( mc_sender_cachedir_name );
	while ( (direntp = readdir( dirp )) != NULL ) {
		unlink(direntp->d_name);
	}
	(void)closedir( dirp );
	chdir(ocwd);
	free(ocwd);

	mc_sender_state_tab_size = INIT_N_STATE_CACHE;
	mc_sender_state_tab = (McStateStruct *)calloc(mc_sender_state_tab_size,
					sizeof(McStateStruct));
	for(i = 0; i<mc_sender_state_tab_size; i++) {
		mc_sender_state_tab[i].statid = -1;
		mc_sender_state_tab[i].start_moid = -1;
	}
}

/* 
 * aurl :    cannon url. The reference to find in cache
 * fname_ret : file name where to find data
 * mhs_ret : a Struct to fill
 * return 0/1 : Notfound/Found
 */
int McSenderCacheFindData(char *aurl, char **fname_ret, MimeHeaderStruct *mhs_ret)
{
	int h;
	McHashEntry * deb;
	char smoid[40];
	int moid;
	char * fname;

	h = hash_url(aurl);
	deb = sender_hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->aurl) == 0) {
			moid = deb->moid;
			sprintf(smoid,"%ld",moid);
			fname = (char *) malloc(mc_sender_len_cachedir_name +
						strlen(smoid)+ 2);
			sprintf(fname, "%s/%s", mc_sender_cachedir_name, smoid); /* the filename */
			(*mhs_ret) = *(moid_sender_cache[moid].mhs);
			*fname_ret = fname ;
			return 1;
		}
		deb = deb->next;
	}
	mhs_ret->moid_ref = -1;
	return 0;
}

/* - input :
	aurl, the cannon url. Reference for cache
	status_code, the error code (kind of mime header struct)
	moid, Multicast ID
*/
void McSenderCachePutErrorInCache( char *aurl, int status_code, int moid,
	char ** fname_ret, MimeHeaderStruct *mhs_ret)
{
	McHashEntry * deb;
	char smoid[40];
	int h, i;
	int exist = 0;
	char *fname_w;
	int fdw;
	struct timeval tv;
	char * buf;
	int object_size;

/* look if an entry still exist, if yes abort */
	h = hash_url(aurl);
	deb = sender_hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->aurl) == 0) {
			assert(0);	/* incoherence */
		}
		deb = deb->next;
	}

	if ( moid >= moid_sender_cache_size) {
		int omoid = moid_sender_cache_size;

		moid_sender_cache_size = moid_sender_cache_size * 2;
		moid_sender_cache = (McObjectStruct *) realloc(moid_sender_cache,
			moid_sender_cache_size * sizeof(McObjectStruct));
		for(i = omoid; i< moid_sender_cache_size; i++){
			moid_sender_cache[i].exist = 0;
			moid_sender_cache[i].aurl = NULL;
			moid_sender_cache[i].fname = NULL;
			moid_sender_cache[i].file_len = 0;
			moid_sender_cache[i].mhs = NULL;
			moid_sender_cache[i].moid = -1;
			moid_sender_cache[i].last_modify = 0;
		}
	}
	assert(! moid_sender_cache[moid].aurl); 	/* incoherence */

	add_url_to_bucket(sender_hash_tab, h, aurl, moid);
	sprintf(smoid,"%ld",moid); 
	fname_w = (char *) malloc(mc_sender_len_cachedir_name +
		strlen(smoid)+ 2);
	sprintf(fname_w, "%s/%s", mc_sender_cachedir_name, smoid); /* the filename */
	moid_sender_cache[moid].aurl = strdup(aurl);
	moid_sender_cache[moid].fname = fname_w;
	moid_sender_cache[moid].moid = moid;
	moid_sender_cache[moid].last_modify = time(NULL);
	moid_sender_cache[moid].exist = 1;

	moid_sender_cache[moid].mhs = (MimeHeaderStruct *) calloc(1,
				sizeof(MimeHeaderStruct));
	moid_sender_cache[moid].mhs->content_encoding = 0;
	moid_sender_cache[moid].mhs->content_length = 0;
	moid_sender_cache[moid].mhs->status_code = status_code;
	moid_sender_cache[moid].mhs->cache_control = 0;
	moid_sender_cache[moid].mhs->content_type = NULL;
	moid_sender_cache[moid].mhs->expires = NULL;
	moid_sender_cache[moid].mhs->last_modified = NULL;
	moid_sender_cache[moid].mhs->location = NULL;
        moid_sender_cache[moid].mhs->moid_ref = moid;
        moid_sender_cache[moid].mhs->is_stateless = 0;
        moid_sender_cache[moid].mhs->state_id = 0;
        moid_sender_cache[moid].mhs->n_do = 0;
        moid_sender_cache[moid].mhs->dot = NULL;
	gettimeofday(&tv, 0);
        moid_sender_cache[moid].mhs->ts = tv; /* time stamp for multicast */

        buf = (char*)malloc(strlen(aurl) + 220);
        sprintf(buf, "ERROR %d %s HTTP/1.0\n\n\n", status_code, aurl);
        object_size = strlen(buf);    
	moid_sender_cache[moid].file_len = object_size;

	fdw = open(fname_w,O_WRONLY | O_CREAT | O_TRUNC,0744);
	write(fdw, buf, object_size+1);
	close (fdw);
	free(buf);
	*fname_ret = fname_w;
	(*mhs_ret) = *(moid_sender_cache[moid].mhs);
	return ;
}

/* input:
	fname_r : file to read and copy
	aurl :	ref url
	mhs
	dot
	ndo
*/
void McSenderCachePutDataInCache(char *fname_r, char *aurl, MimeHeaderStruct *mhs,
		int moid, DependObjectTab dot, int ndo, char **fname_ret,
		MimeHeaderStruct *mhs_ret)
{
	char iobuf[CACHE_BUFSIZ];
	McHashEntry * deb;
	char smoid[40];
	int h, i;
	int exist = 0;
	char *fname_w;
	int fdr, fdw;
	struct timeval tv;
	int len_state_buf, len_do_buf; 
	int data_size, header_size, object_size;
	char state_buf[50];            
	char *buf=NULL;
	char *do_buf=NULL;
	char *header;                  
	char ces[50];
	MimeHeaderStruct *cmhs;

/* look if an entry still exist, if yes abort */
	h = hash_url(aurl);
	deb = sender_hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->aurl) == 0) {
			assert(0);	/* incoherence */
		}
		deb = deb->next;
	}

	if ( moid >= moid_sender_cache_size) {
		int omoid = moid_sender_cache_size;

		moid_sender_cache_size = moid_sender_cache_size * 2;
		moid_sender_cache = (McObjectStruct *) realloc(moid_sender_cache,
			moid_sender_cache_size * sizeof(McObjectStruct));
		for(i = omoid; i< moid_sender_cache_size; i++){
			moid_sender_cache[i].exist = 0;
			moid_sender_cache[i].aurl = NULL;
			moid_sender_cache[i].fname = NULL;
			moid_sender_cache[i].file_len = 0;
			moid_sender_cache[i].mhs = NULL;
			moid_sender_cache[i].moid = -1;
			moid_sender_cache[i].last_modify = 0;
		}
	}

	if (moid_sender_cache[moid].aurl) {
		assert(0);	/* incoherence */
	}
	add_url_to_bucket(sender_hash_tab, h, aurl, moid);
	sprintf(smoid,"%ld",moid); 
	fname_w = (char *) malloc(mc_sender_len_cachedir_name +
		strlen(smoid)+ 2);
	sprintf(fname_w, "%s/%s", mc_sender_cachedir_name, smoid); /* the filename */
	moid_sender_cache[moid].aurl = strdup(aurl);
	moid_sender_cache[moid].fname = fname_w;
	moid_sender_cache[moid].moid = moid;
	moid_sender_cache[moid].last_modify = time(NULL);
	moid_sender_cache[moid].exist = 1;

	moid_sender_cache[moid].mhs = (MimeHeaderStruct *) calloc(1,
				sizeof(MimeHeaderStruct));
	moid_sender_cache[moid].mhs->content_encoding = mhs->content_encoding;
	moid_sender_cache[moid].mhs->content_length = mhs->content_length;
	moid_sender_cache[moid].mhs->status_code = mhs->status_code;
	moid_sender_cache[moid].mhs->cache_control = mhs->cache_control;
	moid_sender_cache[moid].mhs->content_type = strdup(mhs->content_type);
	moid_sender_cache[moid].mhs->expires = strdup(mhs->expires);
	moid_sender_cache[moid].mhs->last_modified = strdup(mhs->last_modified);
	moid_sender_cache[moid].mhs->location = NULL;
	if(mhs->location)
		moid_sender_cache[moid].mhs->location = strdup(mhs->location);
        moid_sender_cache[moid].mhs->moid_ref = moid;
        moid_sender_cache[moid].mhs->is_stateless = 0;
        moid_sender_cache[moid].mhs->state_id = 0;
        moid_sender_cache[moid].mhs->n_do = ndo;
        moid_sender_cache[moid].mhs->dot = dot;
	gettimeofday(&tv, 0);
        moid_sender_cache[moid].mhs->ts = tv; /* time stamp for multicast */
	cmhs = moid_sender_cache[moid].mhs;

        len_state_buf =0;              
/* FIXME : y a un pb ici : ceci n'est pas cachable ######## */
/*        if (mhs->is_stateless){        
 *                sprintf(state_buf,"Stateless-Stateid: %d\n", mhs->state_id);
 *                len_state_buf = strlen(state_buf);
 *        }                              
*/
        len_do_buf = 0;                
        if (cmhs->n_do >0) {            
                char tmp_buf[50];      
                                       
                do_buf = (char*) malloc(cmhs->n_do * 20 + 20);
                sprintf(do_buf,"Depend-Object: %d,", cmhs->n_do);
                for( i=0 ; i < cmhs->n_do ; i++){
                        sprintf(tmp_buf," %d", cmhs->dot[i]);
                        strcat(do_buf,tmp_buf);
                }                      
                strcat(do_buf,"\n");   
                len_do_buf = strlen(do_buf);
        }                              
                                       
/* build a multicast mime header */    
        ces[0] = '\0';                 
        switch(cmhs->content_encoding) {
        case GZIP_ENCODING :           
                sprintf(ces,"Content-Encoding: gzip\n");
                break;                 
        case COMPRESS_ENCODING:        
                sprintf(ces,"Content-Encoding: compress\n");
                break;                 
        }                              
                                       
        buf = (char*)malloc(strlen(aurl)+strlen(ces)+
                strlen(cmhs->content_type)+strlen(cmhs->last_modified)+
                len_state_buf + len_do_buf + 220);
        sprintf(buf, "GET %s HTTP/1.0\n\
Content-Length: %d\n%sContent-Type: %s\n\
Last-Modified: %s\n",                  
                aurl,                  
                cmhs->content_length, ces, cmhs->content_type,
                        cmhs->last_modified); 
        if (len_state_buf){            
                strcat(buf, state_buf);
        }                              
        if (len_do_buf) {              
                strcat(buf, do_buf);   
        }                              
        strcat(buf,"\n");            
        header = buf;                  
        header_size = strlen(header);  
        data_size = cmhs->content_length;        /* that's the size of file */
        object_size = data_size + header_size;

        if (len_do_buf)                
                free(do_buf);          
/* send header and data in one contiguous stream */

	moid_sender_cache[moid].file_len = object_size;
	fdr = open(fname_r,O_RDONLY);

	fdw = open(fname_w,O_WRONLY | O_CREAT | O_TRUNC,0744);
	write(fdw, header, header_size);	/* write mime */
        free(header);                     

	while ( (i = read(fdr,iobuf,CACHE_BUFSIZ)) >0)	/* write body */
		write(fdw, iobuf, i);
	close (fdw);
	close (fdr);
	*fname_ret = fname_w;
	(*mhs_ret) = *(moid_sender_cache[moid].mhs);
	return ;
}

int McCheckObjectQuery(int moid, int offset, int len)
{
	if( moid >= moid_sender_cache_size)
		return 0;
	if( moid_sender_cache[moid].fname == NULL)
		return 0;
	if( !moid_sender_cache[moid].exist)
		return 0;
	if ( moid_sender_cache[moid].file_len <= offset)
		return 0;
	return 1;
}

/* Reset the cache. Delete all cached data */
/*void MMCacheClearCache(void)
 *{
 *	char * ocwd;
 *	int i;
 *	HashEntry * deb, *next;
 *	DIR *dirp;
 *	struct dirent *direntp;
 *
 *       ocwd = getcwd(NULL,MAXPATHLEN);        
 *      chdir(lcachedir_name); 
 *     dirp = opendir( lcachedir_name );
 *    while ( (direntp = readdir( dirp )) != NULL ) {
 *           unlink(direntp->d_name);
 *  }
 * (void)closedir( dirp );
 *        chdir(ocwd);
 *       free(ocwd);
 *	lnext_cid =1;
 *
 *	for( i=0 ; i<MAX_N_FILE_CACHE; i++){
 *		cid_cache[i].exist = 0;
 *		if (cid_cache[i].url)
 *			free(cid_cache[i].url);
 *		cid_cache[i].url = NULL;
 *		cid_cache[i].size = 0;
 *		cid_cache[i].last_modify = 0;
 *		deb = hash_tab[i].head;
 *		hash_tab[i].count = 0;
 *		hash_tab[i].head = NULL;
 *		while ( deb != NULL) {
 *			next = deb->next;
 *			free (deb->url);
 *                      free (deb);
 *			deb = next;
 *		}
 *	}
 *	MMCacheWriteCache();
 *}
*/

/* mhs: describe objet realazing this state */
/* sid: the stateid to cache (to put in memory) */
void MakeSenderState( MimeHeaderStruct *mhs, int sid)
{
	McStateStruct *s;
	McStateStruct *st;
	struct timeval tv;      /* timstamp (struct timeval) */
	int len_do_buf;
	char *do_buf, *buf;
	int object_size;

	if (sid >= mc_sender_state_tab_size) {  /*grow the tab */
		int osize = mc_sender_state_tab_size;
		int i;

		mc_sender_state_tab_size = mc_sender_state_tab_size * 2;
		mc_sender_state_tab = (McStateStruct *)realloc(
			mc_sender_state_tab,
			mc_sender_state_tab_size * sizeof(McStateStruct));
		for(i = osize; i<mc_sender_state_tab_size; i++) {
			mc_sender_state_tab[i].statid = -1;
			mc_sender_state_tab[i].start_moid = -1;
			mc_sender_state_tab[i].n_fdo = 0;
			mc_sender_state_tab[i].fdot = NULL;
			mc_sender_state_tab[i].sdata = NULL;
			mc_sender_state_tab[i].sdata_len = 0;
		}
	}

	s = mc_sender_state_tab;
	s[sid].statid = sid;
	s[sid].start_moid = mhs->moid_ref;
	s[sid].n_fdo = mhs->n_do;
	s[sid].fdot = mhs->dot;
	gettimeofday(&tv, 0);
	s[sid].ts = tv;

	st = &s[sid];
        len_do_buf = 0;
        if (st->n_fdo >0) {
                char tmp_buf[50];
		int i;
         
                do_buf = (char*) malloc(st->n_fdo * 20 + 20);
                sprintf(do_buf,"Depend-Object: %d,", st->n_fdo);
                for( i=0 ; i < st->n_fdo ; i++){
                        sprintf(tmp_buf," %d", st->fdot[i]);
                        strcat(do_buf,tmp_buf);
                }
                strcat(do_buf,"\n");
                len_do_buf = strlen(do_buf);
        }
        buf = (char*) malloc( len_do_buf + 220);
        sprintf(buf,"State-ID: %d\nStart-ObjectID: %d\n",
                st->statid, st->start_moid);
        if (len_do_buf) {
                strcat(buf, do_buf);
                free(do_buf);
        }
        strcat(buf,"\n");
        object_size = strlen(buf);
	mc_sender_state_tab[sid].sdata = buf;
	mc_sender_state_tab[sid].sdata_len = object_size;
}

int McCheckStateQuery(int sid, int offset, int len)
{
	if( sid >= mc_sender_state_tab_size)
		return 0;
	if( mc_sender_state_tab[sid].sdata == NULL)
		return 0;
	if( offset >= mc_sender_state_tab[sid].sdata_len)
		return 0;
	return 1;
}

/* initialize the cache in ~/.mMosaic */
/* check for the dirname/mcache/srcid directory */
/* the default dirname is ~/.mMosaic */
/* else create */
void McSourceCacheInit( Source *src, char * root_name)
{
	int i, l;
	struct stat s;
	char *ocwd;
	DIR *dirp;
	struct dirent *direntp;
	char number[20];
	char *src_root_dirname;
	char *src_cachedir_name;
	int src_len_cachedir_name;
	char *src_source_cachedir_name;
	int src_source_len_cachedir_name;

	src_root_dirname = strdup(root_name);	/* ~/.mMosaic must exit */

	l = strlen(src_root_dirname);
	src_cachedir_name = (char*) malloc(l + 12);
	strcpy(src_cachedir_name,src_root_dirname);
	strcat(src_cachedir_name,"/mcache");
	src_len_cachedir_name = strlen(src_cachedir_name);

	src_source_cachedir_name = (char*) malloc(src_len_cachedir_name + 20);
	strcpy(src_source_cachedir_name,src_cachedir_name);
	sprintf(number,"/%d", src->srcid);
	strcat(src_source_cachedir_name, number);
	src_source_len_cachedir_name = strlen(src_source_cachedir_name);

	if ( stat(src_root_dirname,&s) != 0 ) {
		fprintf (stderr, "Multicast Cache directory not found.\n");
		fprintf (stderr, "This is mandatory in Multicast context. Aborting...\n");
		free(src_root_dirname);
		src_root_dirname = NULL;
		assert(0);
	}

/* read the cache or make one */
	if(stat(src_cachedir_name,&s) != 0) { /* create dircache ~mMosaic/mcache*/
		mkdir(src_cachedir_name,0755);
	}
	if(stat(src_source_cachedir_name,&s) != 0) { /* create dircache ~mMosaic/mcache/srcid */
		mkdir(src_source_cachedir_name,0755);
	}

	src->source_cachedir_name = src_source_cachedir_name;
	src->source_len_cachedir_name = src_source_len_cachedir_name;
/* alloc an initiale moid_cache */
	src->objects = (McObjectStruct *)calloc(INIT_N_FILE_CACHE , sizeof(McObjectStruct));
	src->objects_tab_size = 0;
	McRcvrSrcAllocObject(src, INIT_N_FILE_CACHE);

/* create a hash-code table and a reverse moid table */
	src->hash_tab = (McBucket *) calloc(HASH_TABLE_SIZE , sizeof(McBucket));
	for( i=0 ; i<HASH_TABLE_SIZE; i++){
		src->hash_tab[i].head = NULL;
		src->hash_tab[i].count = 0;
	}

	/* read the dir cache remove files */

	ocwd = getcwd(NULL,MAXPATHLEN);
	chdir(src_source_cachedir_name);
	dirp = opendir( src_source_cachedir_name );
	while ( (direntp = readdir( dirp )) != NULL ) {
		unlink(direntp->d_name);
	}
	(void)closedir( dirp );
	chdir(ocwd);
	free(ocwd);

	src->states_tab_size = 0;
	McRcvrSrcAllocState(src, INIT_N_STATE_CACHE );
}

/* input:
	s	: Source where we cache data
	obdata	: the full data message mime and body
	len_obdata : size of obdata
	body	: pointer to body data
	body_len: len of body data
	aurl :	ref url
	mhs
	moid
output:
	fname_ret: the file cache. where we put the data
	mhs_ret : update mhs???
*/
void McSourceCachePutDataInCache(Source *s, char * body, int body_len,
	char *aurl, MimeHeaderStruct *mhs, int moid,
	char **fname_ret, MimeHeaderStruct *mhs_ret)
{
	McHashEntry * deb;
	char smoid[40];
	int h;
	int exist = 0;
	char *fname_w;
	int fdw;
	struct timeval tv;
	int data_size;
	char *buf=NULL;
	char *do_buf=NULL;
	MimeHeaderStruct *cmhs;

/* look if an entry still exist, if yes abort */
	h = hash_url(aurl);
	deb = s->hash_tab[h].head;
	while (deb != NULL){
		if (strcmp(aurl,deb->aurl) == 0) {
			assert(0);	/* incoherence */
		}
		deb = deb->next;
	}

	McRcvrSrcAllocObject(s, moid);

	if (s->objects[moid].aurl) {
		assert(0);	/* incoherence (we still have it) */
	}
	add_url_to_bucket(s->hash_tab, h, aurl, moid);
	sprintf(smoid,"%ld",moid); 

	fname_w = (char *) malloc(s->source_len_cachedir_name +
		strlen(smoid)+ 2);
	sprintf(fname_w, "%s/%s", s->source_cachedir_name, smoid); /* the filename */
	s->objects[moid].aurl = strdup(aurl);
	s->objects[moid].fname = fname_w; /* file name for body data */
	s->objects[moid].moid = moid;
	s->objects[moid].last_modify = time(NULL);
	s->objects[moid].exist = 1;

	s->objects[moid].mhs = (MimeHeaderStruct *) calloc(1,
				sizeof(MimeHeaderStruct));
	s->objects[moid].mhs->content_encoding = mhs->content_encoding;
	s->objects[moid].mhs->content_length = mhs->content_length;
	s->objects[moid].mhs->status_code = mhs->status_code;
	s->objects[moid].mhs->cache_control = mhs->cache_control;
	s->objects[moid].mhs->content_type = strdup(mhs->content_type);
	s->objects[moid].mhs->expires = strdup(mhs->expires);
	s->objects[moid].mhs->last_modified = strdup(mhs->last_modified);
	s->objects[moid].mhs->location = NULL;
	if(mhs->location)
		s->objects[moid].mhs->location = strdup(mhs->location);
        s->objects[moid].mhs->moid_ref = moid;
        s->objects[moid].mhs->is_stateless = 0;
        s->objects[moid].mhs->state_id = 0;
        s->objects[moid].mhs->n_do = mhs->n_do;
        s->objects[moid].mhs->dot = mhs->dot;
	gettimeofday(&tv, 0);
        s->objects[moid].mhs->ts = tv; /* time stamp for multicast */
	cmhs = s->objects[moid].mhs;

/* FIXME : y a un pb ici : ceci n'est pas cachable ######## */
/*        if (mhs->is_stateless){        
 *                sprintf(state_buf,"Stateless-Stateid: %d\n", mhs->state_id);
 *                len_state_buf = strlen(state_buf);
 *        }                              
*/

        data_size = body_len;
	s->objects[moid].file_len = data_size;
	fdw = open(fname_w,O_WRONLY | O_CREAT | O_TRUNC,0744);
	write(fdw, body, data_size);	/* write body */
	close (fdw);
	*fname_ret = fname_w;
	(*mhs_ret) = *(s->objects[moid].mhs);
	return ;
}
