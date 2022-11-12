#define SYS_SUCCESS 0
#define SYS_NO_COMMAND 1
#define SYS_FORK_FAIL 2
#define SYS_PROGRAM_FAILED 3
#define SYS_NO_RETBUF 4
#define SYS_FCNTL_FAILED 5
#define SYS_NO_SRC_FILE 6
#define SYS_NO_DEST_FILE 7
#define SYS_DEST_EXISTS 8
#define SYS_NO_MEMORY 9
#define SYS_SRC_OPEN_FAIL 10
#define SYS_DEST_OPEN_FAIL 11
#define SYS_READ_FAIL 12
#define SYS_WRITE_FAIL 13
#define SYS_INTERNAL_FAIL 14

extern int my_system(char *cmd, char *retBuf, int bufsize);
extern int my_move(char *src, char *dest, char *retBuf, int bufsize, int overwrite);
extern char *my_strerror(int errornum);
extern char **buildArgv(char *cmd, int *new_argc);
extern int my_copy(char *, char *, char *, int, int);
extern int get_home(char **);
extern int compact_string(char *main_string, char *ellipsis_string,
                    int num_chars, int mode, int eLength);
extern char *strcasechr(char *src, char srch);
extern char *strrcasechr(char *src, char srch);

extern char *HTSortFetch (int i);
extern void HTSortSort (void);
extern void HTSortAdd (char *str);
extern void HTSortInit (void);
