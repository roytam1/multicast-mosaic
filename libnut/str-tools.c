/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

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

char *my_strndup(char *str, int num)
{
	char *nstr = NULL;

	if(!str || !*str )
		return NULL;

	nstr = (char*)malloc(sizeof(char) * (num + 1));

	strncpy(nstr, str, num);
	nstr[num] = '\0'; /* shouldn't strcpy do this ?? */
	return nstr;
}

char *my_chop(char *str)
{
	char *ptr;

	if(!str || !*str)
		return str;
 
/* Remove blank space from end of string. */
	ptr = str + strlen(str) - 1;
	while((ptr >= str) && isspace(*ptr)) {
		*ptr = '\0';
		ptr--;
	}
 
/* Remove blank space from start of string. */
	ptr = str;
	while(isspace(ptr[0]))
		ptr++;
 
/*
** If there was blank space at start of string then move string back to the
** beginning. This prevents memory freeing problems later if pointer is
** moved. memmove is used because it is safe for overlapping regions.
*/
	if (ptr != str)
#ifdef SUNOS
		memcpy (str, ptr, strlen (ptr) + 1); /* no memmove in SUNOS */
#else
		memmove (str, ptr, strlen (ptr) + 1);
#endif
	return str;
}
