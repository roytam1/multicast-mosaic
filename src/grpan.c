/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include "../libhtmlw/HTML.h"
#include "mosaic.h"
#include "mo-www.h"
#if defined(__hpux) || defined(__sgi)
#include <time.h>
#endif


/* -------------------------- Group Annotations --------------------------- */
#define NCSA_GROUP_ANNOTATION_FORMAT_ONE \
  "<ncsa-group-annotation-format-1>"

/* If Rdata.annotation_server is non-NULL, then every time we retrieve
   a document from any remote system we ask the annotation server
   about any annotations corresonding to that document.

   The annotation server either returns NULL (none exist) or a block
   of HTML-format text suitable for appending at the end of a document
   that contain anchors that link to the annotations themselves.

   The external interface to this module is as follows:

   char *mo_fetch_grpan_links (char *url)
     Return an HTML-format hyperlink table to be appended to
     the document text, or NULL if no annotations exist.
     If a non-NULL result is returned, the caller must free it.
     
   mo_status mo_new_grpan (char *url, char *title, char *author,
                           char *text)
     Add a new annotation for document named by 'url' to the
     list of annotations for that document on the annotation server.
     Return mo_succeed if successful, else mo_fail.

   All functions herein behave properly if Rdata.annotation_server
   is NULL; therefore, they can be freely called as desired.
*/


/* Escape quotes by preceding them with backslash.
 * Also escape backslash with a preceding backslash.
 */
static char * EscapeStuff(char *title)
{
	char *ret;
	char *ptr;
	char *tptr;
	int ecnt;

	if (title == NULL)
		return(title);

	ecnt = 0;
	tptr = title;
	while (*tptr != '\0') {
		if ((*tptr == '\"')||(*tptr == '\\')) {
			ecnt++;
		}
		tptr++;
	}

	if (ecnt == 0) {
		ret = strdup(title);
		return(ret);
	}

	ret = (char *)malloc(strlen(title) + ecnt + 1);
	ptr = ret;
	tptr = title;
	while (*tptr != '\0') {
		if ((*tptr == '\"')||(*tptr == '\\')) {
			*ptr++ = '\\';
			*ptr++ = *tptr++;
		} else {
			*ptr++ = *tptr++;
		}
	}
	*ptr = '\0';
	return(ret);
}

/****************************************************************************
 * name:    mo_fetch_grpan_links
 * purpose: Fetch the list of group annotations for this document.
 * inputs:  
 *   - char *url: The URL for which to fetch.
 * returns: 
 *   An HTML block with the list of group annotations, or NULL.
 * remarks: 
 *   Right now the server constructs the list for us.  Later we will want 
 *   to construct it ourselves to be able to support local kill lists, 
 *   and the like.
 ****************************************************************************/
char *mo_fetch_grpan_links (char *url)
{
	char *request;
	char *ttxt, *ttxthead;
	char *post_data, *status_ptr;

	if (! get_pref_string(eANNOTATION_SERVER)) /* No annotation server */ {
		return NULL;
	} else {
/* Go get the anchor in the URL, if any. */
		char *anch = mo_url_extract_anchor (url);
      
/* If there is one and it doesn't start with "hdfref",
 * then clip it off.  WHAT ABOUT PERSONAL ANNOTATIONS??? */
		if (anch && strncmp (anch, "hdfref", 6))
			url = mo_url_canonicalize (url, "");
      
/* Sanity check. */
		if (!url)
			return NULL;

/* amb */
		post_data = (char *)malloc(strlen(url) + 1024);
		sprintf(post_data, "cmd=an_get&format=html&url=%s", url);
		ttxt = mo_post_pull_er_over (get_pref_string(eANNOTATION_SERVER),
				"application/x-www-form-urlencoded", 
				post_data, &ttxthead);
		free(post_data);

/* check if status=200 was returned */
		status_ptr=strstr(ttxt, "status=");
/*check for null... SWP*/
		if (!status_ptr || (status_ptr && strncmp(status_ptr, "status=200", 10)!=0)) {
			return(NULL);
		}
		if ((ttxt == NULL)||(*ttxt == '\0')) { /* No annotations */
			return(NULL);
		} else {
			return(ttxt);
		}
	}
}

/****************************************************************************
 * name:    mo_is_editable_grpan
 * purpose: Discover if a block of HTML is an editable group annotation.
 * inputs:  
 *   - char *text: Text block to check.
 * returns: 
 *   mo_succeed if it is, mo_fail if not.
 * remarks: 
 *   
 ****************************************************************************/
mo_status mo_is_editable_grpan (char *text)
{
	if (!text)
		return mo_fail;
	if (!strncmp (text, NCSA_GROUP_ANNOTATION_FORMAT_ONE,
	    strlen (NCSA_GROUP_ANNOTATION_FORMAT_ONE)))
		return mo_succeed;
	else
		return mo_fail;
}

/****************************************************************************
 * name:    mo_new_grpan
 * purpose: Send a new group annotation to the server.
 * inputs:  
 *   - char    *url: The URL of the document being annotated.
 *   - char  *title: Title of the new annotation.
 *   - char *author: Author of the new annotation.
 *   - char   *text: Text of the new annotation.
 * returns: 
 *   mo_succeed if the annotation was registered; mo_fail if not.
 * remarks: 
 *   Send a new group annotation to the server.  The data (text) can actually
 *   be binary, but then you need to compose what you send with something other
 *   than strcat, and you may need to call a function other than
 *   mo_pull_er_over to send the binary request unmunged.
 ****************************************************************************/
mo_status mo_new_grpan (char *url, char *title, char *author, char *text)
{
	if (! get_pref_string(eANNOTATION_SERVER)) /* No annotation server */ {
		return mo_fail;
	} else {
		char *post_data;
		char *request;
		char *ttxt, *ttxthead;
		time_t foo = time (NULL);
		char *ts = ctime (&foo);
		char *Etitle, *Euser;
		char *esc_text;
      
		ts[strlen(ts)-1] = '\0';
		Etitle = EscapeStuff(title);
		Euser = EscapeStuff(author);

/* amb */
		post_data = (char *)malloc(strlen(url) + strlen(Etitle) + strlen(Euser) 
				+ strlen(ts) + strlen(text) + 256);
		if (post_data == NULL)
			return mo_fail;
		esc_text = mo_escape_part(text);
		sprintf(post_data,"cmd=an_post&url=%s&title=%s&author=%s&text=%s",
				url, title, author, esc_text);
		ttxt = mo_post_pull_er_over (get_pref_string(eANNOTATION_SERVER),
				"application/x-www-form-urlencoded", 
				post_data, &ttxthead);
		free(post_data);
		if (Etitle != NULL)
			free(Etitle);
		if (Euser != NULL)
			free(Euser);
		return mo_succeed;
	}
}

/****************************************************************************
 * name:    mo_audio_grpan
 * purpose: Send a new audio group annotation to the server.  
 * inputs:  
 *   - char    *url: The URL of the document being annotated.
 *   - char  *title: Title of the new annotation.
 *   - char *author: Author of the new annotation.
 *   - char   *data: Binary data of the new annotation.
 *   - int      len: 
 * returns: 
 *   mo_succeed if everything's OK, mo_fail else.
 * remarks: 
 *   The data is sound, either .au, or .aiff format.
 ****************************************************************************/
mo_status mo_audio_grpan (char *url, char *title, char *author, 
                          char *data, int len)
{
	if (! get_pref_string(eANNOTATION_SERVER)) /* No annotation server */ {
		return mo_fail;
	} else {
		char *request;
		char *ttxt, *ttxthead;
		time_t foo = time (NULL);
		char *ts = ctime (&foo);
		char *Etitle, *Euser;

		ts[strlen(ts)-1] = '\0';
		Etitle = EscapeStuff(title);
		Euser = EscapeStuff(author);
      
		request = (char *)malloc(strlen(url) + strlen(Etitle) + strlen(Euser) +
				strlen(ts) + 256);
		if (request == NULL)
			return mo_fail;
		sprintf(request,
#ifdef __sgi
			"grpan://%s/url=\"%s\";title=\"%s\";user=\"%s\";date=%s;audio=aiff;length=%d;=",
#else /* sun or hp */
			"grpan://%s/url=\"%s\";title=\"%s\";user=\"%s\";date=%s;audio=au;length=%d;=",
#endif
			get_pref_string(eANNOTATION_SERVER), url, Etitle, Euser, ts, len);
		ttxt = grpan_doit("ANN_SET ", request, data, len, &ttxthead);
		free(request);
		if (Etitle != NULL)
			free(Etitle);
		if (Euser != NULL)
			free(Euser);
		return mo_succeed;
	}
}

/****************************************************************************
 * name:    mo_modify_grpan
 * purpose: Alter the content of an existing group annotation.
 * inputs:  
 * inputs:  
 *   - char    *url: The URL of the annotation being changed.
 *   - char  *title: Title of the new (modified) annotation.
 *   - char *author: Author of the new (modified) annotation.
 *   - char   *text: Text of the new (modified) annotation.
 * returns: 
 *   mo_succeed if the annotation changes were registered; mo_fail if not.
 * remarks: 
 *
 ****************************************************************************/
mo_status mo_modify_grpan (char *url, char *title, char *author, char *text)
{
	if (! get_pref_string(eANNOTATION_SERVER)) /* No annotation server */ {
		return mo_fail;
	} else {
		char *request;
		char *ttxt, *ttxthead;
		time_t foo = time (NULL);
		char *ts = ctime (&foo);
		char *Etitle, *Euser;
      
		ts[strlen(ts)-1] = '\0';
		Etitle = EscapeStuff(title);
		Euser = EscapeStuff(author);
      
		request = (char *)malloc(strlen(url) + strlen(Etitle) + strlen(Euser) +
				strlen(ts) + strlen(text) + 256);
		if (request == NULL) {
			return mo_fail;
		}
		sprintf(request,
			"grpan://%s/url=\"%s\";title=\"%s\";user=\"%s\";date=%s;length=%d;=",
			get_pref_string(eANNOTATION_SERVER), url, Etitle, Euser, ts, strlen(text));
		ttxt = grpan_doit("ANN_CHANGE ", request, text, (strlen(text) + 1), &ttxthead);
		free(request);
		if (Etitle != NULL)
			free(Etitle);
		if (Euser != NULL)
			free(Euser);
		return mo_succeed;
	}
}

/****************************************************************************
 * name:    mo_delete_grpan
 * purpose: Delete the annotation whose url is passed.
 * inputs:  
 *   - char *url: URL of annotation to be deleted.
 * returns: 
 *   mo_succeed, if everything went OK.
 * remarks: 
 *   
 ****************************************************************************/
mo_status mo_delete_grpan (char *url)
{
	if (! get_pref_string(eANNOTATION_SERVER)) /* No annotation server */ {
		return mo_fail;
	} else {
		char *request;
		char *ttxt, *ttxthead;
      
		request = (char *)malloc(strlen(url) + 256);
		sprintf (request, "grpan://%s/url=\"%s\";=", 
			get_pref_string(eANNOTATION_SERVER), url);
		ttxt = grpan_doit("ANN_DELETE ", request, (char *)NULL, 0, &ttxthead);
		free(request);
		return(mo_succeed);
	}
}

/****************************************************************************
 * name:    mo_grok_grpan_pieces
 * purpose: Read pieces of out an annotation.
 * inputs:  
 *   - char     *url: URL of the annotation.
 *   - char       *t: Text of the annotation.
 *   - char  **title: Return title of the annotation.
 *   - char **author: Return author of the annotation.
 *   - char   **text: Return text (body) of the annotation.
 *   - int       *id: Return ID of the annotation.
 *   - char     **fn: Unused.
 * returns: 
 *   mo_succeed if the return pointers were set to the right data;
 *   mo_fail if something went wrong.
 * remarks: 
 *   
 ****************************************************************************/
mo_status mo_grok_grpan_pieces (char *url, char *t,
                                char **title, char **author, char **text,
                                int *id, char **fn)
{
	char *txt;
	char *tptr;
	char *head;

/* Fail if there isno annotation text */
	txt = t;
	if ((txt == NULL)||(*txt == '\0'))
		return mo_fail;

/* Fail if this is not a group annotation */
	if (strncmp (txt, NCSA_GROUP_ANNOTATION_FORMAT_ONE,
	    strlen (NCSA_GROUP_ANNOTATION_FORMAT_ONE)) != 0)
		return mo_fail;
  
/* Skip the magic cookie */
	tptr = txt;
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* Skip the title line */
	tptr++;
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* skip to the beginning of the title after the header tag */
	while (*tptr != '>') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
	tptr++;
	head = tptr;
  
/* skip to the end of the title before the close header tag */
	while (*tptr != '<') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
	*tptr = '\0';
	*title = strdup(head); /* snarf out the title */
	*tptr = '<';
  
/* skip to the end of the header line. */
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* skip to the beginning of the author after the address tag */
	while (*tptr != '>') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
	tptr++;
	head = tptr;
  
/* skip to the end of the author before the close address tag */
	while (*tptr != '<') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
	*tptr = '\0';
	*author = strdup(head); /* snarf the author name */
	*tptr = '<';
  
/* skip to the end of the author line. */
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* skip to the end of the date line. */
	tptr++;
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* skip to the end of the ___ line. */
	tptr++;
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
  
/* skip to the end of the pre line. */
	tptr++;
	while (*tptr != '\n') {
		if (*tptr == '\0') {
			return mo_fail;
		}
		tptr++;
	}
	tptr++;
	*text = strdup(tptr); /* snarf the remaining text */
  
/*
* Find the annotation file name at the end of the url, and strip
* the id number out of it.
*/
	tptr = strrchr(url, '/');
	if (tptr == NULL) {
		int hash, val;
		if (sscanf(url, "%d-%d.html", &hash, &val) != 2) {
			return mo_fail;
		}
		*id = val;
	} else {
		int hash, val;
      
		tptr++;
		if (sscanf(tptr, "%d-%d.html", &hash, &val) != 2) {
			return mo_fail;
		}
		*id = val;
	}
	return mo_succeed;
}
