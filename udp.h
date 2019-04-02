#ifndef udpHeader
#define udpHeader

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include "defs.h"
#include "messages.h"

void join(servStruct *msgserv, int fd, struct sockaddr_in  serveraddr, int addrlen);
void setUDPserver(char * upt, int *fdUDPserver, struct sockaddr_in * serveraddr/*, struct in_addr * hostptr*/);
void setUDPclient(char *siip, char *sipt, struct sockaddr_in* serveraddr, struct hostent * hostptr);

#endif
