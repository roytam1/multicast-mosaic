#include <stdio.h>
#include <time.h>
#include <ctype.h>
#include <string.h>

#include "util.h"

static char *const days[7]= {"Sun","Mon", "Tue", "Wed", "Thu", "Fri", "Sat"};
static char *const month_snames[12] = {
	"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};

static char ts[50];
static struct tm *gmt;

char * rfc822ctime( time_t t)
{
	gmt = gmtime(&t);
	sprintf(ts, "%s, %.2d %s %d %.2d:%.2d:%.2d GMT",
		days[gmt->tm_wday],
		gmt->tm_mday, month_snames[gmt->tm_mon], gmt->tm_year + 1900,
		gmt->tm_hour, gmt->tm_min, gmt->tm_sec);
	return ts;
}

/* Simply loop through a string and convert all newlines to spaces. */
/* We now remove leading whitespace as well */
/* and trailing whitesapce as well */
char *mo_convert_newlines_to_spaces (char *str)
{
	int i;
	char *tptr;
	char * ptr;
 
	if (!str)
		return NULL;
	for (i = 0; i < strlen (str); i++)
		if (str[i] == '\n')
	str[i] = ' ';
	tptr = str;
	while ((*tptr != '\0')&&(isspace((int)(*tptr))))
		tptr++;
	if (tptr != str)
		memcpy(str, tptr, (strlen(tptr) + 1));
	for(ptr=(str + strlen(str) -1); ptr && *ptr == ' '; ptr--)
		*ptr = '\0';
	return str;
}


