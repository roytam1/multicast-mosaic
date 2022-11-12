

extern void ChildTerminated(int sig);

extern void AddChildProcessHandler(pid_t, void (*)(EditFile *, int), void *);
