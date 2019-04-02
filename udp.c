#include "udp.h"


/***************************************************************************************************
    Função: regista o servidor de mensagens no SID

    Parâmetros: estrutura msgserv, socket UDP, serveraddr e addrlen do cliente UDP

    Return: void
****************************************************************************************************/
void join(servStruct *msgserv, int fd, struct sockaddr_in  serveraddr, int addrlen){
  char msg[128];

  sprintf(msg, "REG %s;%s;%s;%s\n", msgserv->name, msgserv->ip, msgserv->upt, msgserv->tpt);

  if(sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*) &serveraddr, addrlen) == -1){
    printf("ERRO: sendto, udp.c\n");
    fflush(stdout);
    exit(1);
  }
}


/***************************************************************************************************
    Função: inicializa o servidor UDP

    Parâmetros: porto de escuta UDP, socket UDP do servidor e serveraddr

    Return: void
****************************************************************************************************/
void setUDPserver(char* upt, int *fdUDPserver, struct sockaddr_in * serveraddr/*, struct in_addr * hostptr*/){
  int port = atoi(upt);

  serveraddr->sin_family = AF_INET;
  serveraddr->sin_port = htons((u_short)port);
  serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
}


/***************************************************************************************************
    Função: inicializa o cliente UDP

    Parâmetros: porto UDP do SID, IP do SID, serveraddr e hostptr do cliente UDP

    Return: void
****************************************************************************************************/
void setUDPclient(char *siip, char *sipt, struct sockaddr_in* serveraddr, struct hostent * hostptr){
  int port  = atoi(sipt);

  serveraddr->sin_family = AF_INET;
  serveraddr->sin_port = htons((u_short)port);

  if((hostptr = gethostbyname(siip)) == NULL){
    printf("ERRO: não encontra o SID!\n");
    fflush(stdout);
    exit(1);
  }

  serveraddr->sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
}
