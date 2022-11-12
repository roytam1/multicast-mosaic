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
