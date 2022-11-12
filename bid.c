#include <stdio.h>
#include <stdlib.h>


main()
{
	char * renv;

	renv = getenv ("http_proxy");
	printf("%s\n", renv);
}
	
