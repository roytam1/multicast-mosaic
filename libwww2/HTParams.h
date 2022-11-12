/* Created : 10 Sep 1997
		Gilles Dauphin
*/

/* Parameters for libwww2 */

typedef struct _HTParams {
	int trace;		/* trace enable in libwww */
	char *global_xterm_str ;
	char *uncompress_program ;
	char *gunzip_program ;
	int tweak_gopher_types ;
	int max_wais_responses ;
	int ftp_timeout_val ;
	int ftpRedial;
	int ftpFilenameLength;
	int ftpEllipsisLength;
	int ftpEllipsisMode;
	int sendAgent;
	int use_default_extension_map;
	int twirl_increment;
	char *personal_type_map;
	int use_default_type_map;
	char *personal_extension_map;
	int reloading;
} HTParams;

extern HTParams wWWParams;
