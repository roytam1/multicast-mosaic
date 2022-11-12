/* Please read copyright.ncsa. Don't remove next line */
#include "copyright.ncsa"

/* Note that this code is taken almost straight from the httpd cgi-src
	directory.*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char *makeword(char *line, char stop)
{
    int x = 0,y;
    char *word = (char *) malloc(sizeof(char) * (strlen(line) + 1));

    for(x=0;((line[x]) && (line[x] != stop));x++)
        word[x] = line[x];

    word[x] = '\0';
    if(line[x]) ++x;
    y=0;

    while(line[y++] = line[x++]);
    return word;
}
