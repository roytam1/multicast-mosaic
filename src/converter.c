#include <stdlib.h>
#include <string.h>

/* convert plain to html */
/* replace '<' , '>' and '&' by html equivalent, insert a <PRE> and </PRE> */

void MMa2html(char **data, int len, int *len_ret)
{
	char *d;
	char *ds;
	int lenr;
	char * ptr;
	int cnt_logt = 0;
	int cnt_amp = 0;
	int cnt_oc = 0;

	ptr = *data;
	while (*ptr) {
		switch (*ptr) {
		case '<' :
		case '>' :
			cnt_logt++;
			break;
		case '&' :
			cnt_amp++;
			break;
		default:
			cnt_oc++;
			break;
		}
		ptr++;
	}

	lenr = cnt_oc + 6 + cnt_amp*5 + cnt_logt*4 + 7;
	ds = (char*) malloc(lenr+1);
	ptr = *data;
	strcpy(ds,"<PRE>\n");
	d = ds + 6;
	while(*ptr) {
		switch(*ptr) {
		case '<' :
			*d++ = '&'; *d++ = 'l'; *d++ = 't'; *d++ = ';' ;
			ptr++;
			break;
		case '>' :
			*d++ = '&'; *d++ = 'g'; *d++ = 't'; *d++ = ';' ;
			ptr++;
			break;
		case '&' :
			*d++ = '&'; *d++ = 'a'; *d++ = 'm'; *d++ = 'p'; *d++ = ';'  ;
			ptr++;
			break;
		default:
			*d++ = *ptr++;
			break;
		}
	}
	*d++ = '<'; *d++ = '/' ; *d++ = 'P' ; *d++ = 'R' ; *d++ = 'E' ; *d++ = '>' ; *d++ = '\n';
	ds[lenr] = '\0';
	free(*data);
	*data = ds;
	*len_ret = lenr ;
	return;
}
