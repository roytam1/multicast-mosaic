#include <stdio.h>

extern unsigned int random32(int);

main(int argc, char ** argv)
{
	if (argc == 1) { printf("arg needed\n"); exit(1); }
	printf("%08x\n", random32(atoi(argv[1])));
}
