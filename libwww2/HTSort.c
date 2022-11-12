/* Simple string sorting support, thanks to qsort(). */

#include "HTUtils.h"
#include <string.h>

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
	 (int (*)(void*,void*))dsortf);
}

char *HTSortFetch (int i)
{
  if (i < count)
    return hunk[i];
  else
    return NULL;
}
