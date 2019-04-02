#ifndef clientudpRMBHeader
#define clientudpRMBHeader

#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <netdb.h>

char* chooseServer(char* allServers);
void show_servers(char *siip, char *sipt,  int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr);
void publish(char *ip, char *portU, char *inpInfo, int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr);
void showMsg(char *ip, char *portU, char *inpInfo, int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr);

#endif
