static char *hostname=0;		/* The name of this host */
const char * HTHostName(void)
{
    get_host_details();
    return hostname;
}

