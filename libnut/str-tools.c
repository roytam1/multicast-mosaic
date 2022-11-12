/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define outofmem(file, func) \
 { fprintf(stderr, "%s %s: out of memory.\nProgram aborted.\n", file, func); \
  exit(1);}

char *getFileName(char *file_src)
{
	char *ptr;

	if (!file_src || !*file_src)
		return(NULL);
	ptr=strrchr(file_src,'/');
	if (!ptr || !*ptr)
		return(file_src);
	if (*ptr=='/' && *(ptr+1))
		ptr++;
	return(ptr);
}

/* Will casefully search forward through a string for a character.
 * Must be a null-terminated string.
 *
 * SWP
 */
char *strcasechr(char *src, char srch)
{
	char *ptr=NULL;
	char tmp;

	if (!src || !*src)
		return(NULL);
	tmp=toupper(srch);
	for (ptr=src; (*ptr && toupper(*ptr)!=tmp); ptr++);
/*
* At this point, either *ptr == \0 (failure) or toupper(*ptr) is
*   == to tmp (success). Return accordingly.
*/
	if (*ptr)
		return(ptr);
	return(NULL);
}

/* Will casefully search backward through a string for a character.
 * Must be a null-terminated string.
 *
 * SWP
 */  
char *strrcasechr(char *src, char srch)
{
	char *ptr=NULL;
	char tmp;

	if (!src || !*src) {
		return(NULL);
	}
	tmp=toupper(srch);
	for (ptr=(src+strlen(src)-1); (ptr>src && toupper(*ptr)!=tmp); ptr--);
/*
* At this point we have either found toupper(*ptr) == to tmp, or we
*   are at the very begining of the string. So, if ptr is != to src,
*   we found a match...or...we need to test to make sure the first
*   char in the string is not the match. Return accordingly.
*/
	if (ptr!=src || toupper(*ptr)==tmp) {
		return(ptr);
	}       
	return(NULL);
}        
 
/*      Allocate a new copy of a string, and returns it */
 
char * HTSACopy(char **dest, const char *src)
{                                     
  if (!dest)                          
        return NULL;                  
  if (*dest) free(*dest);             
  if (! src)                          
    *dest = NULL;                     
  else {                              
    *dest = (char *) malloc (strlen(src) + 1);
    if (*dest == NULL) outofmem(__FILE__, "HTSACopy");
    strcpy (*dest, src);              
  }                                   
  return *dest;                       
}                                     
      
                                      
/*      String Allocate and Concatenate */
                                      
char * HTSACat (char **dest, const char *src)
{                                     
  if (src && *src) {                  
    if (*dest) {                      
      int length = strlen (*dest);    
      *dest = (char *) realloc (*dest, length + strlen(src) + 1);
      if (*dest == NULL) outofmem(__FILE__, "HTSACat");
      strcpy (*dest + length, src);   
    } else {                          
      *dest = (char *) malloc (strlen(src) + 1);
      if (*dest == NULL) outofmem(__FILE__, "HTSACat");
      strcpy (*dest, src);            
    }                                 
  }                                   
  return *dest;                       
}                                     


/* Simple string sorting support, thanks to qsort(). */
/* this is a copy of libwww2 HTSort */

#define SIZE_OF_HUNK 100

static char **hunk = NULL;
static int size_of_hunk;
static int count;

void HTSortInit (void)
{
	count = 0;
	if (!hunk) {
		size_of_hunk = SIZE_OF_HUNK;
		hunk = (char **)malloc (sizeof (char *) * size_of_hunk);
	}
}

static void expand_hunk (void)
{
  /* Make hunk bigger by SIZE_OF_HUNK elements. */
  size_of_hunk += SIZE_OF_HUNK;
  hunk = (char **)realloc (hunk, sizeof (char *) * size_of_hunk);
}

void HTSortAdd (char *str)
{
  /* If we don't have room, expand. */
  if (count == size_of_hunk)
    expand_hunk ();

  hunk[count++] = str;
}

static int dsortf (char **s1, char **s2)
{
  return (strcmp (*(char **)s1, *(char **)s2));
}

void HTSortSort (void)
{
  qsort ((void *)hunk, 
	 count, 
	 sizeof (char *), 
	 (int (*)(const void*,const void*))dsortf);
}

char *HTSortFetch (int i)
{
  if (i < count)
    return hunk[i];
  else
    return NULL;
}
