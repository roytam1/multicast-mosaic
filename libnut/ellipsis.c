#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int compact_string(char *main_string, char *ellipsis_string, 
		    int num_chars, int mode, int eLength) 
{
	int string_len;
	int feem, puff, i;
	int right_side, left_side;

	num_chars--;
	string_len = strlen(main_string);
	if(string_len <= num_chars) {
		strcpy(ellipsis_string, main_string);
		return(1);
	}
	switch(mode) {
	case 1:
		puff = num_chars - eLength;
		feem = string_len - puff;
		strcpy(ellipsis_string,".");
		for (i = 0; i < eLength; i++) {
			ellipsis_string [i] = '.';
		}
		ellipsis_string [i] = '\0';
		strncat(ellipsis_string, main_string + feem, puff);
		break;
	case 2:
		puff = num_chars - eLength;
		left_side = puff >> 1;
		right_side = puff - left_side;
		strncpy (ellipsis_string, main_string, left_side);
		for (i = left_side; i < num_chars-right_side; i++) {
			ellipsis_string [i] = '.';
		}
		ellipsis_string [num_chars - right_side] = '\0';
		strncat (ellipsis_string, main_string + (string_len - right_side),
				right_side);
		break;
	case 3:
		puff = num_chars - eLength;
		feem = string_len - puff;
		strncpy(ellipsis_string, main_string, puff);
		for (i = puff; i < num_chars; i++) {
			ellipsis_string [i] = '.';
		}
		break;
	default:
		return(-1);
	}
	ellipsis_string[num_chars]='\0';
	return(1);
}
