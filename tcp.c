#include "tcp.h"


/***************************************************************************************************
    Função: inicializar o servidor TCP

    Parâmetros: porto TCP, descritor do socket do servidor TCP

    Return: void
****************************************************************************************************/
void setTCPserver(char *tpt, struct sockaddr_in* serveraddr){
  int port = atoi(tpt);

  serveraddr->sin_family = AF_INET;
  serveraddr->sin_addr.s_addr = htonl(INADDR_ANY);
  serveraddr->sin_port = htons((u_short)port);
}


/***************************************************************************************************
    Função: criar o array que guarda os descritores de cada conexão TCP

    Parâmetros: número máximo de ligações

    Return: array tcpConec
****************************************************************************************************/
tcpStruct** createTCParray(int max){
  int i = 0;

  /* alocação de memória para o array de estruturas do tipo tcpStruct */
  tcpStruct** tcpConec = (tcpStruct**) malloc(sizeof(tcpStruct*)*max);
  if(tcpConec == NULL){
    printf("ERRO: tcpConec=NULL, tcp.c\n");
    fflush(stdout);
    exit(1);
  }
  for(i = 0; i < max; i++){
    tcpConec[i] = (tcpStruct*) malloc(sizeof(tcpStruct));
    if(tcpConec[i] == NULL){
      printf("ERRO: tcpConec[i]=NULL, tcp.c\n");
      fflush(stdout);
      exit(1);
    }
  }

  /* inicialização dos descritores e portos com -1 e ip em branco */
  for(i = 0; i < max; i++){
    tcpConec[i]->fd = -1;
    strcpy(tcpConec[i]->ip, "");
    tcpConec[i]->port = -1;
  }

  return tcpConec;
}


/***************************************************************************************************
    Função: mostrar no terminal a lista de servidores de mensagens com os quais se tem uma
    conexão TCP estabelecida

    Parâmetros: array tcpConec, máximo de conexões

    Return: void
****************************************************************************************************/
void showServers(tcpStruct** tcpConec, int max){
  int i = 0;

  for(i = 0; i < max; i++){
    if(tcpConec[i]->fd != -1){
      printf("%s:%d\n", tcpConec[i]->ip, tcpConec[i]->port);
    }
  }
}


/***************************************************************************************************
    Função: escolhe a qual servidor de mensagens pede a lista de mensagens

    Parâmetros: estrutura msgserv, lista com todos os servidores, array de conexões TCP

    Return: mensagem de erro
****************************************************************************************************/
char* chooseServer(servStruct *msgserv, char* allServers, tcpStruct** tcpConec){
  /*variáveis auxiliares */
  int i = 0;
  int n = 0;
  int total = 0; /* número total de servidores na lista recebida */
  char* ptr;
  char buffer[5000];

  /* identificar o servidor actual na lista de servidores */
  int this = -1;
  char thisServer[200];

  /* dados de cada servidor */
  char id[100];
  char ip[100];
  char portU[100];
  char portT[100];

  /* variáveis e estruturas para operações relaciondas com as ligações TCP */
  struct hostent *hostptr;
  struct sockaddr_in serveraddr;
  int nleft = 0;
  int nbytes = 0;
  int nw = 0;
  int nTCP = 0; /* número de ligações TCP efectuadas com sucesso */
  int fd = 0;

  /* alocação de memória para separar os servidores recebidos através do SID */
  char **list = (char**) malloc(sizeof(char*)*100);
  if(list == NULL){
    printf("ERRO: list = NULL, tcp.c\n");
  }
  for(i=0; i<100;i++){
    list[i] = (char*) malloc(sizeof(char)*100);
    if(list[i] == NULL){
      printf("ERRO: list[i] = NULL, tcp.c\n");
    }
  }

  /* guardar em thisServer os dados do servidor actual */
  sprintf(thisServer, "%s;%s;%s;%s",  msgserv->name, msgserv->ip, msgserv->upt, msgserv->tpt);

  /* guardar cada servidor em separado no array list */
  ptr = strtok(allServers, "\n");
  i = 0;
  while(ptr != NULL) {
    strcpy(list[i], ptr);
    ptr = strtok(NULL, "\n");
    if(strstr(list[i], thisServer) != NULL){
      this = i;
    }
    i++;
    total++;
  }

  /* pode acontecer que o registo do servidor actual enviado para o SID ainda não ter chegado e ele ser o primeiro */
  if(i <= 1){
    for(i = 0; i < 100; i++){
      free(list[i]);
    }
    free(list);
    return "";
  }

  /* o registo já está no SID e é o único servidor registado */
  if(total-1 == 1 && this == 1){
    for(i = 0; i < 100; i++){
      free(list[i]);
    }
    free(list);
    return "first";
  }

  printf("Servidores de mensagens:\n");
  /* tenta estabelecer ligação TCP com os outros servidores de mensagens */
  for(i = 1; i < total; i++){
    if(i == this){
      continue;
    }
    sscanf(list[i], "%[^';'];%[^';'];%[^';'];%s", id, ip, portU, portT);
    printf("  *%s;%s;%s;%s -> ", id, ip, portU, portT);
    for(n = 0; n < 200; n++) {
      /* procura uma posição livre */
      if(tcpConec[n]->fd == -1 ){
        fd=socket(AF_INET,SOCK_STREAM,0);
        if((hostptr=gethostbyname(ip)) == NULL){
          printf("não encontrado\n");
          fflush(stdout);
          break;
        }

        memset((void*)&serveraddr,(int)'\0', sizeof(serveraddr));
        serveraddr.sin_family = AF_INET;
        serveraddr.sin_addr.s_addr = ((struct in_addr *)(hostptr->h_addr_list[0]))->s_addr;
        serveraddr.sin_port = htons((u_short)atoi(portT));

        /* tenta estabelecer conexão */
        if(connect(fd,(struct sockaddr*)&serveraddr, sizeof(serveraddr)) == -1){
          printf("não conectado\n");
          fflush(stdout);
          break;
        }

        /* guarda o descritor, o ip e o porto TCP no array tcpConec */
        tcpConec[n]->fd = fd;
        strcpy(tcpConec[n]->ip, ip);
        tcpConec[n]->port = atoi(portT);
        nTCP++;
        printf("TCP OK\n");
        break;
      }
    }
  }

  /* pode acontecer não se ligar a nenhum servidor */
  if(nTCP == 0){
    for(i = 0; i < 100; i++){
      free(list[i]);
    }
    free(list);
    return "error: não se ligou a nenhum MSGSERV";
  }

  /* escolha aleatória de um servidor de mensagens com ligação TCP estabelecida
  de modo a pedir a lista de mensagens */
  srand(time(NULL));
  i = (rand()%(nTCP));

  ptr = NULL;
  ptr = strcpy(buffer, "SGET_MESSAGES\n"); /* ponteiro para a primeira posição do vetor buffer */
  nbytes = strlen(buffer);
  nleft = nbytes;

  /* envio do pedido das mensagens */
  while(nleft > 0){
    nw = write(tcpConec[i]->fd, ptr, nleft);
    if(nw<0){
      printf("ERRO: nw<0, tcp.c %s\n", tcpConec[i]->ip);
      fflush(stdout);
      exit(1);
    }
    nleft -= nw;
    ptr += nw;
  }

  printf("\nEscolhido: %s:%d\n", tcpConec[i]->ip, tcpConec[i]->port);

  for(i = 0; i < 100; i++){
    free(list[i]);
  }
  free(list);

  return "ok";
}


/***************************************************************************************************
    Função: liberta a memória do array de conexões TCP

    Parâmetros: array tcpConec, máximo de conexões

    Return: void
****************************************************************************************************/
void freeTCPconec(tcpStruct** tcpConec, int max){
  int i = 0;

  for(i = 0; i < max; i++){
    free(tcpConec[i]);
  }
  free(tcpConec);
}
