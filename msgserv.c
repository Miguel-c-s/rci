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
#include <signal.h>
#include "defs.h"
#include "messages.h"
#include "udp.h"
#include "tcp.h"

#define max(A,B) ((A)>=(B)?(A):(B))
#define MAX_SERVERS 200
#define MAX_BUFFER 5000

void readArgs(int argc, char **argv, servStruct *msgserv);

/* msgserv –n name –j ip -u upt –t tpt [-i siip] [-p sipt] [–m m] [–r r] */

int main(int argc, char **argv){
  void (*old_handler)(int);
  if((old_handler=signal(SIGPIPE,SIG_IGN))==SIG_ERR)exit(1);

  /* variáveis auxiliares */
  int x = 0;
  int i = 0;
  int sd = 0;
  int n = 0;
  int firstJoin = 0; /* indica se o utilizador já se fez o registo no SID pela primeira vez */
  char command[128];
  char buffer[MAX_BUFFER];
  char *msgList;
  char *ptr;

  /* variáveis para o select */
  fd_set rfds;
  int maxfd, counter;
  struct timeval timeout;

  /* sockets */
  int fdUDPserver = socket(AF_INET, SOCK_DGRAM, 0); /* socket UDP servidor */
  int fdUDPclient = socket(AF_INET, SOCK_DGRAM, 0); /* socket UDP cliente */
  tcpStruct** tcpConec = createTCParray(MAX_SERVERS); /* descritores das conexões TCP */
  int fdTCPmaster = socket(AF_INET, SOCK_STREAM, 0); /* socket TCP escuta */

  /* mensagens */
  servStruct *msgserv = (servStruct*) malloc(sizeof(servStruct)); /* estrutura do servidor que guarda os argumentos de entrada */
  if(msgserv == NULL){
    printf("ERRO: msgsev=NULL, msgserv.c\n");
    fflush(stdout);
    exit(0);
  }
  msgStruct** msgArray; /* estrutura que guarda todas as mensagens recebidas */

  /* leitura dos argumentos de entrada */
  readArgs(argc, argv, msgserv);

  /* cria o array onde vão ser guardadas todas as mensagens */
  msgArray = createMsgArray(atoi(msgserv->m));

  /* incialização do servidor e cliente UDP */
  unsigned int addrlenUDPserver, addrlenUDPclient;
  struct hostent *hostptr;
  hostptr = NULL;
  struct sockaddr_in serveraddrUDPserver, clientaddrUDPserver, serveraddrUDPclient;

  /* servidor UDP */
  memset((void*) &serveraddrUDPserver, (int)'\0', sizeof(serveraddrUDPserver));
  setUDPserver(msgserv->upt, &fdUDPclient, &serveraddrUDPserver);
  if(bind(fdUDPserver, (struct sockaddr*) &serveraddrUDPserver, sizeof(serveraddrUDPserver)) == -1){
    printf("ERRO: bindUDPserver=-1, msgserv.c\n");
    fflush(stdout);
    exit(1);
  }
  addrlenUDPserver = sizeof(clientaddrUDPserver);

  /* cliente UDP */
  memset((void*) &serveraddrUDPclient, (int)'\0', sizeof(serveraddrUDPclient));
  setUDPclient(msgserv->siip, msgserv->sipt, &serveraddrUDPclient, hostptr);
  addrlenUDPclient = sizeof(serveraddrUDPclient);

  /* incialização do servidor TCP */
  struct sockaddr_in serveraddrTCPmaster, clientaddrTCPmaster;
  unsigned int clientlenTCPmaster;

  memset((void*)&serveraddrTCPmaster,(int)'\0',sizeof(serveraddrTCPmaster));
  setTCPserver(msgserv->tpt, &serveraddrTCPmaster);
  if(bind(fdTCPmaster,(struct sockaddr*)&serveraddrTCPmaster,sizeof(serveraddrTCPmaster))){
    printf("ERRO: bindTCPmaster, msgserv.c\n");
    fflush(stdout);
    exit(1);
  }
  if(listen(fdTCPmaster, MAX_SERVERS) == -1){
    printf("ERRO: listen, msgserv.c\n");
    fflush(stdout);
    exit(1);
  }


  printf("\n> ");
  fflush(stdout);
  timeout.tv_sec = atoi(msgserv->r);
  timeout.tv_usec = 0;
  while(1){
    FD_ZERO(&rfds);
    FD_SET(0, &rfds);maxfd = 0;
    FD_SET(fdUDPclient, &rfds);maxfd = max(maxfd, fdUDPclient);
    FD_SET(fdUDPserver, &rfds);maxfd = max(maxfd, fdUDPserver);
    FD_SET(fdTCPmaster, &rfds);maxfd = max(maxfd, fdTCPmaster);
    /*FD_SET para todos os descritores das ligações TCP */
    for(i = 0; i < MAX_SERVERS ; i++){
      sd = tcpConec[i]->fd;
      if(sd > 0){
        FD_SET(sd, &rfds);
      }
      if(sd > maxfd){
        maxfd = sd;
      }
    }

    counter = select(maxfd + 1, &rfds, (fd_set*) NULL, (fd_set*) NULL, &timeout);
    if(counter < 0){
      printf("ERRO: counter<0, msgserv.c\n");
      fflush(stdout);
      exit(1);
    }

    /* refresh do registo no SID */
    if(counter == 0){
      timeout.tv_sec = atoi(msgserv->r);
      /* só faz refresh se o utilizador tiver feito join */
      if(firstJoin){
        join(msgserv, fdUDPclient, serveraddrUDPclient, addrlenUDPclient);
      }
    }

    /* socket de escuta do TCP */
    if(FD_ISSET(fdTCPmaster, &rfds)){
      clientlenTCPmaster = sizeof(clientaddrTCPmaster);
      for(i = 0; i < MAX_SERVERS; i++) {
        /* verifica se a posição do array está livre */
        if(tcpConec[i]->fd == -1 ){
          if((tcpConec[i]->fd = accept(fdTCPmaster,(struct sockaddr*)&clientaddrTCPmaster, &clientlenTCPmaster)) == -1){
            printf("ERRO: accept=-1, msgserv.c\n");
            fflush(stdout);
            exit(1);
          }
          /* se a conexão foi estabelecida com sucesso guarda o IP, porto TCP e descritor no array tcpConec */
          strcpy(tcpConec[i]->ip, inet_ntoa(clientaddrTCPmaster.sin_addr));
          tcpConec[i]->port = clientaddrTCPmaster.sin_port;
          printf("TCP OK\n");
          printf("Ligação estabelecida com %s:%d\n", tcpConec[i]->ip, tcpConec[i]->port);
          break;
        }
      }
      printf("\n> ");
      fflush(stdout);
    }

    /* FD_ISSET para cada uma das conexões TCP guardadas no array tcpConec */
    for(i = 0; i < MAX_SERVERS; i++){
      sd = tcpConec[i]->fd;
      if(FD_ISSET(sd , &rfds)){
        ptr = &buffer[strlen(buffer)];
        n = read(sd, ptr, 1);
        if(n < 0){
          printf("ERRO: read<0, msgserv.c n = %d %s \n", n, tcpConec[i]->ip);
          fflush(stdout);
          exit(1);
        }
        /* verifica o comando de PROTOCOLO recebido: SGET_MESSAGES ou SMESSAGES */
        if(strstr(buffer, "SGET_MESSAGES\n") != NULL){
          sendMessages(sd, msgArray, atoi(msgserv->m), 0); /* envia a lista de mensagens para outro servidor */
          memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
        }
        else if(strstr(buffer, "SMESSAGES\n") != NULL){
          /* detectar se já foi lido todo o buffer verificando se já foi lido \n\n */
          for(x = 0; x < sizeof(buffer) - 1; x ++){
            if(buffer[x] == '\n' && buffer[x+1] == '\n'){
              receiveMessages(msgArray, atoi(msgserv->m), buffer); /* guardar as mensagens recebidas */
              memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
              break;
            }
          }
        }
        /* quando uma conexão TCP é fechada pelo outro interveniente read retorna 0 */
        if(n == 0){
          /* remover o descritor de tcpConec e fazer FD_CLR */
          close(tcpConec[i]->fd);
          FD_CLR(tcpConec[i]->fd, &rfds);
          printf("%s:%d desligou-se\n", tcpConec[i]->ip, tcpConec[i]->port);
          printf("\n> ");
          fflush(stdout);

          tcpConec[i]->fd = -1;
          strcpy(tcpConec[i]->ip, " ");
          tcpConec[i]->port = -1;
        }
      }
    }

    /* FD_ISSET para o servidor UDP */
    if(FD_ISSET(fdUDPserver, &rfds)){
      memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
      if(recvfrom(fdUDPserver, buffer, sizeof(buffer), 0, (struct sockaddr*) &clientaddrUDPserver, &addrlenUDPserver) == -1){
        printf("ERRO: recvfrom, msgserv.c\n");
        fflush(stdout);
        exit(1);
      }

      if(strstr(buffer,"PUBLISH") != NULL){
        insertMsg(msgArray, atoi(msgserv->m), buffer);
        for(i = 0; i < MAX_SERVERS; i++){
          if(tcpConec[i]->fd != -1 ){
            sendMessages(tcpConec[i]->fd, msgArray, atoi(msgserv->m), 1);
          }
        }
      }
      else if(strstr(buffer,"GET_MESSAGES") != NULL){
        msgList = getMessages(msgArray, atoi(msgserv->m), buffer);
        if(sendto(fdUDPserver, msgList, strlen(msgList)+1, 0, (struct sockaddr*) &clientaddrUDPserver, addrlenUDPserver) == -1){
          printf("ERRO: sendto, msgserv.c\n");
          fflush(stdout);
          exit(1);
        }
        free(msgList);
      }
      memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
    }

    /* FD_ISSET para o cliente UDP */
    if(FD_ISSET(fdUDPclient, &rfds)){
      memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
      if(recvfrom(fdUDPclient, buffer, sizeof(buffer), 0, (struct sockaddr*) &serveraddrUDPclient, &addrlenUDPclient) == -1){
        printf("ERRO: recvfrom, msgserv.c\n");
        fflush(stdout);
        exit(1);
      }
      if(strstr(buffer, "SERVERS") != NULL){
        printf("%s\n", buffer);
        chooseServer(msgserv, buffer, tcpConec);
        /* regista-se no SID */
        join(msgserv, fdUDPclient, serveraddrUDPclient, addrlenUDPclient);
        printf("Registado no SID!\n\n");
        firstJoin = 1; /* coloca a flag a 1 para o programa saber que tem de fazer refresh ao registo */
        printf("> ");
        fflush(stdout);

      }
      memset(buffer, '\0', sizeof(char)*MAX_BUFFER); /* limpar o buffer */
    }

    /* FD_ISSET para o terminal UDP */
    if(FD_ISSET(0, &rfds)){
      fgets(command, 100, stdin);
      sscanf(command, "%s", command);
      if(strcmp(command, "join") == 0){
        /* pede a lista de servidores ao SID */
        if(sendto(fdUDPclient, "GET_SERVERS", strlen("GET_SERVERS")+1, 0, (struct sockaddr*) &serveraddrUDPclient, addrlenUDPclient) == -1){
          printf("ERRO: sendto, msgserv.c\n");
          fflush(stdout);
          exit(1);
        }
      }
      else if(strcmp(command, "show_servers") == 0){
        showServers(tcpConec, MAX_SERVERS); /* imprime a lista de servidores aos quais tem uma conexão TCP estabelecida */
      }
      else if(strcmp(command, "show_messages") == 0){
        showMessages(msgArray, atoi(msgserv->m)); /* imprime a lista de mensagens guardadas no servidor */
      }
      else if(strcmp(command, "exit") == 0){
        break; /* sai do loop para o programa poder ser encerrado */
      }
      else{
        printf("Comando inválido! 1) show_servers  2) show_messages  3) exit\n");
        fflush(stdout);
      }

      printf("\n> ");
      fflush(stdout);
    }
  }

  /* fechar todos os sockets UDP e TCP */
  close(fdUDPserver);
  close(fdUDPclient);
  close(fdTCPmaster);
  for(i = 0; i < MAX_SERVERS; i++){
    if(tcpConec[i]->fd >= 0){
      close(tcpConec[i]->fd);
    }
  }

  /* libertar a memória */
  freeMsgArray(msgArray, atoi(msgserv->m));
  freeTCPconec(tcpConec, MAX_SERVERS);
  free(msgserv);

  printf("A terminar msgserv...\n");
  fflush(stdout);
  exit(0);
}


/***************************************************************************************************
    Função: leitura dos argumentos de entrada

    Parâmetros: argc, argv, estrutura msgserv

    Return: void
****************************************************************************************************/
void readArgs(int argc, char **argv, servStruct *msgserv){
  int opt = 0;

  /* ponteiros auxiliares para detectar se os argumentos obrigatórios foram escritos */
  char *namePtr = NULL;
  char *ipPtr = NULL;
  char *uptPtr = NULL;
  char *tptPtr = NULL;

  /* argumentos opcionais com os valores por defeito */
  strcpy(msgserv->siip, "tejo.tecnico.ulisboa.pt");
  strcpy(msgserv->sipt, "59000");
  strcpy(msgserv->m, "200");
  strcpy(msgserv->r, "10");

  /* ciclo para ler os argumentos de entrada */
  while((opt = getopt (argc, argv, "n:j:u:t:i:p:m:r:")) != -1){
    switch(opt){
      case 'n':
        strcpy(msgserv->name, optarg);
        namePtr = &msgserv->name[0];
        break;
      case 'j':
        strcpy(msgserv->ip, optarg);
        ipPtr = &msgserv->ip[0];
        break;
      case 'u':
        strcpy(msgserv->upt, optarg);
        uptPtr = &msgserv->upt[0];
        break;
      case 't':
        strcpy(msgserv->tpt, optarg);
        tptPtr = &msgserv->tpt[0];
        break;
      case 'i':
        strcpy(msgserv->siip, optarg);
        break;
      case 'p':
        strcpy(msgserv->sipt, optarg);
        break;
      case 'm':
        strcpy(msgserv->m, optarg);
        break;
      case 'r':
        strcpy(msgserv->r, optarg);
        break;
      case '?':
        printf("msgserv –n name –j ip -u upt –t tpt [-i siip] [-p sipt] [–m m] [–r r]\n\n");
        exit(0);
    }
  }

  /* detectar se os argumentos obrigatórios foram escritos */
  if(namePtr == NULL || ipPtr == NULL || uptPtr == NULL || tptPtr == NULL){
    printf("Faltam parâmetros obrigatórios!\n");
    printf("msgserv –n name –j ip -u upt –t tpt [-i siip] [-p sipt] [–m m] [–r r]\n\n");
    exit(0);
  }
}
