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
#include "clientudpRMB.h"


/*
Função: Escolhe um servidor para o cliente aceder de entre os que estão disponíveis no SID

Parâmetros: buffer(string) com todos os servidores no SID

Return:  parâmetros do servidor escolhido ( nome, ip, ports UDP e TCP)
*/
char* chooseServer(char* allServers){

  int i = 0;
  int total = 0;
  char *aux = (char*) malloc(sizeof(char)*100);
  if(aux == NULL){
    printf("ERRO: aux=NULL, clientudpRMB\n");
    fflush(stdout);
    exit(0);
  }
  char* ptr;
  char **list = (char**) malloc(sizeof(char*)*100);
  if(list == NULL){
    printf("ERRO: list=NULL, clientudpRMB.c\n");
    fflush(stdout);
    exit(0);
  }
  for(i=0; i<100;i++){
    list[i] = (char*) malloc(sizeof(char)*100);
    if(list[i] == NULL){
      printf("ERRO: list[i]=NULL, clientudpRMB.c\n");
      fflush(stdout);
      exit(0);
    }
  }

  ptr= strtok(allServers, "\n");
  i = 0;
  while(ptr != NULL) {
    strcpy(list[i], ptr);
    ptr = strtok(NULL, "\n");
    i++;
    total++;
  }

  if(i<= 1){
    return "error";
  }
  srand(time(NULL));
  i = (rand()%(total-1))+1; // random % total gives number from 0 to total -1, we want from 2 to total4
  printf("Escolhido: \n%s\n\n", list[i]);
  fflush(stdout);

  strcpy(aux, list[i]);

  for(i = 0; i < 100; i++){
    free(list[i]);
  }
  free(list);

  return aux;
}

/*
NOT used
void setUDPclient(char *siip, char *sipt, struct sockaddr_in* serveraddr, struct hostent * hostptr){
*/

/*
Função: Envia pedido ao SID para enviar de volta a lista de servidores que estão lá guardados

Parâmetros: Ip e port do SID, estruturas e valores da socket

Return: NULL

*/
void show_servers(char *siip, char *sipt, int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr){
  char msg[100];

  serveraddr->sin_port = htons((u_short)atoi(sipt));
  hostptr = gethostbyname(siip);
  if(hostptr == NULL){
    printf("show_servers: Message server not found. \n");
    fflush(stdout);
    exit(0);
  }
  serveraddr->sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
  strcpy(msg, "GET_SERVERS");
  if(sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*) &(*serveraddr), addrlen) == -1){
    printf("ERRO: sendto, clientudpRMB\n");
    fflush(stdout);
    exit(0);
  }
}


/*
Função: faz um pedido ao servidor escolhido para publicar uma mensagem contida no input

Parâmetros: ip e port do servidor escolhido, string com comando, parâmetros da socket

Return: NULL
*/
void publish(char *ip, char *portU, char *input, int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr){

  int i ;
  static char cmd[7] = "PUBLISH";
  hostptr = gethostbyname(ip);
  if(hostptr == NULL){
    printf("Publish: Message server not found. \n");
    fflush(stdout);
    exit(0);
  }

  serveraddr->sin_port = htons((u_short)atoi(portU));
  serveraddr->sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;

  if(strlen(input) > 148){
    printf("Your message is HUMONGOUS! Max length = 140 char, Try again\n");
    return;
  }

  for(i = 0; i <= 6; i++){
    input[i] = cmd[i];
  }
  if(sendto(fd, input, strlen(input)+1, 0, (struct sockaddr*) &(*serveraddr), addrlen) == -1){
    printf("ERRO: sendto, clientudpRMB\n");
    fflush(stdout);
    exit(0);
  }
}


/*
Função: Pede ao servidor escolhido um número "x" de mensagens recentes

Parâmetros: ip e port do serv escolhido, número de msgs, parâmetros da socket

Return: NULL

*/

void showMsg(char *ip, char *portU, char *inpInfo, int fd, struct sockaddr_in * serveraddr, int addrlen, struct hostent * hostptr){
  char msg[200];

  serveraddr->sin_port = htons((u_short)atoi(portU));
  hostptr = gethostbyname(ip);
  if(hostptr == NULL){
    printf("MessageList: Message server not found. \n");
    fflush(stdout);
    exit(0);
  }
  serveraddr->sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;

  strcpy(msg, "GET_MESSAGES ");
  strcat(msg, inpInfo);

  if(sendto(fd, msg, strlen(msg)+1, 0, (struct sockaddr*) &(*serveraddr), addrlen) == -1){
    printf("ERRO: sendto, clientudpRMB\n");
    fflush(stdout);
    exit(0);
  }
}
