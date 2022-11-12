/* The following are flag bits which may be ORed together to form a number
 * to give the 'wanted' argument to URLParse.
 */
#define PARSE_ACCESS            16
#define PARSE_HOST               8
#define PARSE_PATH               4
#define PARSE_ANCHOR             2
#define PARSE_PUNCTUATION        1
#define PARSE_ALL               31

extern char * URLParse(const char * aName, const char * relatedName, int wanted);
extern char * UrlGuess(char *url);
extern char *mo_url_canonicalize_local (char *url);
extern char *mo_url_canonicalize(char *url, char *oldurl);
extern char *mo_url_canonicalize_keep_anchor (char *url, char *oldurl);
extern char *EscapeUrl(char *part);
extern char *UnEscapeUrl (char *str);
