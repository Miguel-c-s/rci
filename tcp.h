#ifndef tcpHeader
#define tcpHeader

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

/* definição do tipo de estrutura tcpStruct que guarda o descritor da ligação TCP, IP e porto TCP */
typedef struct tcpStruct{
  int fd;
  char ip[100];
  int port;
}tcpStruct;

tcpStruct** createTCParray(int max);
void showServers(tcpStruct** tcpConec, int max);
void setTCPserver(char *tpt, struct sockaddr_in* serveraddr);
char* chooseServer(servStruct *msgserv, char* allServers, tcpStruct** tcpConec);
void freeTCPconec(tcpStruct** tcpConec, int max);

#endif
