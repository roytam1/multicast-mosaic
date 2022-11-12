typedef int	ListenAddress;

extern ListenAddress NetServerInitSocket(int portNumber);
extern PortDescriptor *NetServerAccept(ListenAddress socketFD);
extern int NetRead(PortDescriptor *c,char *buffer,int bufferSize);
extern int NetServerWrite(PortDescriptor *c,char *buffer,int bufferSize);
extern void NetCloseConnection(PortDescriptor *c);
extern int NetIsThereInput(PortDescriptor *p);
void NetCloseAcceptPort(int s);
int NetIsThereAConnection(int socketFD);
