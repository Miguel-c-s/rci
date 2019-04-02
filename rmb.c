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

#define max(A,B) ((A)>=(B)?(A):(B))

int checkInp(int ac, char** av, char*, char*);

int main(int argc, char **argv){
  //select
  fd_set rfds;
  int maxfd, counter;

  int fdUDPclient = socket(AF_INET, SOCK_DGRAM, 0); //socket UDP cliente

  char siip[100]; // ip do servidor de identidades
  char sipt[100]; // porta udp do servidor de identidades do tipo "char"

  char input[400]; //reads input from terminal
  char inpCmd[200]; // separate cmd part of input
  char inpInfo[200]; // separate the information part of input

  char id[100]; // guarda nome de servidor de msg
  char ip[100]; // guarda ip de serv de msg
  char portU[100]; // guarda port UDP de serv de msg
  char portT[100]; // guarda port TCP de serv de msg

  char *msgSrv; // guarda um vetor com servidor de msg escolhido

  char buffer[5000]; // guarda uma string recebida no recvfrom

  int first = 0; // variável auxiliar para saber escolher um serv de msg no ínicio do programa apeas 1x

  int check = 0; // verificar se o servidor de mensagens ainda está online
  struct timeval timeout;

  unsigned int addrlenUDPclient;
  struct hostent *hostptr;
  struct sockaddr_in clientaddrUDPclient;

  // parametros opcionais do servidor de identidades
  strcpy(siip, "tejo.tecnico.ulisboa.pt");
  strcpy(sipt, "59000");
  checkInp(argc, argv, siip, sipt);
  hostptr = gethostbyname(siip); // inicializaçao
  if(hostptr == NULL){
    printf("Identity server not found. \n");
    fflush(stdout);
    exit(0);
  }

  //client UDP
  memset((void*) &clientaddrUDPclient, (int)'\0', sizeof(clientaddrUDPclient));
  clientaddrUDPclient.sin_family = AF_INET;
  addrlenUDPclient = sizeof(clientaddrUDPclient);

  // msgSrv contains name ip udp tcp port

  printf("> ");
  /*Ciclo onde vai funcionar o select, o ciclo vai estar sempre a correr, à espera que seja posto
  algum comando no terminal OU receba algo do recvfrom*/
  timeout.tv_sec = 100000;
  timeout.tv_usec = 0;
  while(1){
    if(first == 0){
      show_servers(siip, sipt, fdUDPclient, &clientaddrUDPclient, addrlenUDPclient, hostptr);
    }

    FD_ZERO(&rfds);
    FD_SET(0, &rfds);maxfd = 0;
    FD_SET(fdUDPclient, &rfds);maxfd = max(maxfd, fdUDPclient);

    counter = select(maxfd + 1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if(counter < 0)exit(1);

    if(counter == 0 && check == 1){
      printf("\n\n----------------------------------------------------------------\n");
      printf("O servidor de mensagens selecionado não respondeu à verificação!\n");
      printf("É provável que a mensagem não tenha sido enviada.\n");
      printf("Sugere-se que escolha uma das seguintes opções:\n");
      printf("  *Publicar novamente -> publish mensagem\n");
      printf("  *Escolher outro servidor -> change_server\n");
      printf("  *Terminar o programa -> exit\n");
      printf("----------------------------------------------------------------\n\n");
      printf("> ");
      fflush(stdout);
      check = 0;
      timeout.tv_sec = 100000;
    }
    if(counter == 0){
      timeout.tv_sec = 100000;
    }


    if(FD_ISSET(fdUDPclient, &rfds)){
      memset(buffer, '\0', sizeof(char)*5000); /* limpar o buffer */
      if(recvfrom(fdUDPclient, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddrUDPclient, &addrlenUDPclient) == -1){
        printf("ERRO: recvfrom, rmb.c\n");
        exit(0);
      }
      if(strstr(buffer, "SERVERS") != NULL){
        buffer[strlen(buffer)] = '\0';
        printf("%s\n", buffer);
        if(first == 0){ // se for a primeira vez , escolhemos um servidor de msg para fazer pedidos posteriormente
          msgSrv = chooseServer(buffer);
          if(strcmp(msgSrv, "error") == 0){
            printf("No servers online\n");
            exit(0);
          }
          sscanf(msgSrv, "%[^';'];%[^';'];%[^';'];%s", id, ip, portU, portT); // divide into server name and PORT , port T n interessa
          first = 1;
          free(msgSrv);
        }
        printf("> ");
        fflush(stdout);
      }
      else{
        if(check == 0){
          printf("%s\n", buffer);
          printf("> ");
          fflush(stdout);
        }
        else{
          check = 0;
        }
      }
    }

    if(FD_ISSET(0, &rfds)){
      fgets(input, 5000, stdin);
      input[strcspn(input, "\n")] = '\0'; // procura a posiçao do \n na string e substitui por \0 para o strcmp funcionar
      sscanf(input, "%s %s", inpCmd, inpInfo);

      if(strcmp(inpCmd,"show_servers") == 0 ){
        show_servers(siip, sipt, fdUDPclient, &clientaddrUDPclient, addrlenUDPclient, hostptr);
      }
      else if(strcmp(inpCmd, "publish") == 0){
        publish(ip, portU, input, fdUDPclient, &clientaddrUDPclient, addrlenUDPclient, hostptr);
        check = 1;
        timeout.tv_sec = 3;
        showMsg(ip, portU, "1", fdUDPclient, &clientaddrUDPclient, addrlenUDPclient, hostptr);
        printf("> ");
        fflush(stdout);
      }
      else if(strcmp(inpCmd, "show_latest_messages") == 0){
        check = 0;
        timeout.tv_sec = 100000;
        showMsg(ip, portU, inpInfo, fdUDPclient, &clientaddrUDPclient, addrlenUDPclient, hostptr);
        fflush(stdout);
      }
      else if(strcmp(inpCmd, "exit") == 0){
        printf("A terminar rmb...\n\n");
        fflush(stdout);
        break;
      }
      else if(strcmp(inpCmd, "change_server") == 0){
        first = 0;
      }
      else{
        printf("Comando inválido! 1) show_servers  2) publish 'text'  3) show_latest_messages 'number'  4) exit  5) change_server \n\n");
        printf("> ");
        fflush(stdout);
      }
    }
  }

  close(fdUDPclient);

  return 0;
}

/*
Função: Verifica se os argumentos com que o programa foi chamado estão corretos

Parâmetros: argc e argv, ip e port default do SID

Return: constante

*/
int checkInp(int ac, char** av, char* siip, char* sipt){

  // leitura dos argumentos
  int opt = 0;
  while((opt = getopt (ac, av, "i:p:")) != -1){
    switch(opt){
      case 'i':
        printf("siip: \"%s\"\n", optarg);
        strcpy(siip, optarg);
        break;
      case 'p':
        printf("ip: \"%s\"\n", optarg);
        strcpy(sipt, optarg);
        break;
      case '?':
        printf("rmb [-i siip] [-p sipt]\n\n");
        exit(0);
    }
  }

  if(ac != 1 && ac != 3 && ac != 5 ){
    printf("Invalid parameters. Should be:\n rmb [-i siip] [-p sipt]\n");
    exit(0);
  }else if(ac == 3){
    if((strcmp(av[1], "-i") != 0 ) && (strcmp(av[1], "-p") != 0)){
      printf("Invalid parameters. Should be:\n rmb [-i siip] [-p sipt]\n");
      exit(0);
    }
  }else if(ac == 5){
    if(((strcmp(av[1], "-i") != 0 ) && (strcmp(av[1], "-p") != 0)) ||
    ((strcmp(av[3], "-i") != 0 ) & (strcmp(av[3], "-p") != 0)) ||
     (strcmp(av[1], av[3]) == 0)){
      printf("Invalid parameters. Should be:\n rmb [-i siip] [-p sipt]\n");
      exit(0);
    }
  }
  return 0;
}
