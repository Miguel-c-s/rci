#include "messages.h"


/***************************************************************************************************
    Função: alocação de memória para o array onde são guardadas as mensagens

    Parâmetros: número máximo de mensagens que podem ser guardas no servidor

    Return: msgArray: array onde são guardadas as mensagens
****************************************************************************************************/
msgStruct** createMsgArray(int size){
  int i = 0;

  /* alocação de memória para o array de estruturas do tipo msgStruct */
  msgStruct** msgArray;
  msgArray = (msgStruct**) malloc(sizeof(msgStruct*)*size);
  if(msgArray == NULL){
    printf("ERRO: msgArray=NULL, messages.c\n");
    fflush(stdout);
    exit(1);
  }
  for(i = 0; i < size; i++){
    msgArray[i] = (msgStruct*) malloc(sizeof(msgStruct));
    if(msgArray[i] == NULL){
      printf("ERRO: msgArray[i]=NULL, messages.c\n");
      fflush(stdout);
      exit(1);
    }
  }

  /* relógio lógico inicializado a -1 */
  for(i = 0; i < size; i++){
    msgArray[i]->time = -1;
  }

  return msgArray;
}


/***************************************************************************************************
    Função: inserir uma mensagem no array de mensagens depois de ser recebida por UDP de um cliente
            RMB

    Parâmetros: estrutura de array de mensagens, número máximo de mensagens, buffer com o conteúdo
                recebido por UDP

    Return: void
****************************************************************************************************/
void insertMsg(msgStruct** msgArray, int size, char* buffer){
  char *ptr;
  int i = 0;

  ptr = &buffer[8];  /* ponteiro para o início da mensagem a seguir a PUBLISH */

  /* procura uma posição livre no array de mensagens */
  for(i = 0; i < size; i++){
    if(msgArray[i]->time != -1){  /* continua se já está ocupada */
      continue;
    }
    else{
      if(i == 0){
        msgArray[i]->time = 0;  /* 1ª mensagem a ser guardada */
      }
      else{
        msgArray[i]->time = msgArray[i-1]->time + 1;  /* restantes casos */
      }
      strcpy(msgArray[i]->msg, ptr);
      break;
    }
  }
}


/***************************************************************************************************
    Função: cria a string com a lista de mensagens guardada que vai ser enviada para o cliente RMB

    Parâmetros: array de mensagens, número máximo de mensagens, buffer que contém o número de
                mensagens que o cliente quer receber

    Return: ptr: ponteiro para uma string com a lista de mensagens pronta a ser enviada
****************************************************************************************************/
char* getMessages(msgStruct** msgArray, int size, char* buffer){
  /* variáveis auxiliares */
  char n[100];
  char command[141];
  int nInt = 0; /* número de mensagens pedidas pelo cliente no RMB */
  int i = 0;
  int nMsg = 0; /* número de mensagens já guardadas */
  char *msgList = (char*)malloc(sizeof(char)*5000);
  if(msgList == NULL){
    printf("ERRO: msgList=NULL, messages.c\n");
    fflush(stdout);
    exit(0);
  }
  char *aux;
  aux = &msgList[0];

  /* leitura do número de mensagens pedidas */
  sscanf(buffer, "%s %s", command, n);
  nInt = atoi(n);

	/* coloca uma mensagem em cada linha */
  sprintf(aux, "MESSAGES\n");
  aux = &msgList[strlen(msgList)];
  for(i = 0; i < size; i++){
    if(msgArray[i]->time != -1){
      nMsg++;
    }
  }

  /* se o número de mensagens pedido pelo cliente for superior ao máximo, esse valor é reduzido */
  if(nInt > nMsg){
    nInt = nMsg;
  }

  /* construção da string com a lista de mensagens para ser enviada */
  for(i = nMsg - nInt; i < nMsg; i++){
    if(msgArray[i]->time != -1){
      sprintf(aux, "%s\n", msgArray[i]->msg);
      aux = &msgList[strlen(msgList)];
      nInt--;
    }
    if(nInt == 0){
      break;
    }
  }

  return msgList;
}


/***************************************************************************************************
    Função: envia as mensagens para outros servidores de mensagens

    Parâmetros: descritor do socket TCP, array de mensagens, número máximo de mensagens,
                mode = 0 -> envia todas as mensagens, mode = 1 -> envia apenas a última mensagem

    Return: void
****************************************************************************************************/
void sendMessages(int sd, msgStruct** msgArray, int size, int mode){
  /* variáveis auxiliares */
  int nw = 0;
  int i = 0;
  int nleft = 0;
  int nbytes = 0;
  char msgList[5000];
  char *ptr;
  char *aux;
  aux = &msgList[0];

  sprintf(aux, "SMESSAGES\n");
  aux = &msgList[strlen(msgList)];

  /* envia apenas a mensagem nova */
  if(mode == 0){
    for(i = 0; i < size; i++){
      if(msgArray[i]->time != -1){
        sprintf(aux, "%d;%s\n", msgArray[i]->time, msgArray[i]->msg);
        aux = &msgList[strlen(msgList)];
      }
      else{
        break;
      }
    }
  }
  /* envia todas as mensagens */
  else if(mode == 1){
    for(i = size-1; i >= 0; i--){
      if(msgArray[i]->time != -1){
        sprintf(aux, "%d;%s\n", msgArray[i]->time, msgArray[i]->msg);
        aux = &msgList[strlen(msgList)];
        break;
      }
    }
  }

  /* \n final do protocolo */
  sprintf(aux, "\n");

  /* envio das mensagens para um ou vários servidores, dependendo de mode */
  ptr = &msgList[0];
  nbytes = strlen(msgList);
  nleft = nbytes;
  while (nleft > 0){
    nw = write(sd, msgList, strlen(msgList));
    if(nw < 0){
      printf("ERRO: nw<0, messages.c\n");
      fflush(stdout);
      exit(1);
    }
    nleft -= nw;
    ptr += nw;
  }
}


/***************************************************************************************************
    Função: guarda as mensagens enviadas por outros servidores de mensagens

    Parâmetros: array de mensagens, número máximo de mensagens, buffer com a lista de mensagens
                recebida

    Return: void
****************************************************************************************************/
void receiveMessages(msgStruct** msgArray, int size, char* buffer){
  int i = 0;
  int isEmpty = 1; /* flag para verificar se o array de mensagens está vazio */
  char *ptr;
  int lc = 0; /* relógio lógico */
  int total = 0;

  /* alocação de memória para um array de strings auxiliar */
  char **msg = (char**) malloc(sizeof(char*)*(size+1));
  if(msg == NULL){
    printf("ERRO: msg=NULL, messages.c\n");
    fflush(stdout);
    exit(0);
  }
  for(i = 0; i < (size+1); i++){
    msg[i] = (char*) malloc(sizeof(char)*200);
    if(msg[i] == NULL){
      printf("ERRO: msg[i]=NULL, messages.c\n");
      fflush(stdout);
      exit(0);
    }
  }

  /* detectar se ainda não há mensagens guardadas */
  for(i = 0; i < size; i++){
    if(msgArray[i]->time != -1){
      isEmpty = 0;
      break;
    }
  }

  /* separação das mensagens do buffer */
  ptr = strtok(buffer, "\n");
  i = 0;
  while(ptr != NULL) {
    strcpy(msg[i], ptr);
    ptr = strtok(NULL, "\n");
    i++;
    total++;
    if(i == size+1){
      break;
    }
  }

  /* guardar as mensagens no array com o tempo lógico correcto */
  if(isEmpty){
    for(i = 1; i < total; i++){
      sscanf(msg[i], "%d", &lc);
      ptr = strstr(msg[i], ";");
      ptr++;
      strcpy(msgArray[i-1]->msg, ptr);
      msgArray[i-1]->time = lc;
    }
  }
  else{
    for(i = 0; i < size; i++){
      if(msgArray[i]->time == -1){
        sscanf(msg[1], "%d", &lc);
        ptr = strstr(msg[1], ";");
        ptr++;
        strcpy(msgArray[i]->msg, ptr);
        msgArray[i]->time = max(lc, msgArray[i-1]->time) + 1;
        break;
      }
    }
  }

  /* libertar memória alocada */
  for(i = 0; i < (size+1); i ++){
    free(msg[i]);
  }
  free(msg);
}


/***************************************************************************************************
    Função: imprime no terminal as mensagens guardadas no servidor de mensagens

    Parâmetros: array de mensagens, número máximo de mensagens

    Return: void
****************************************************************************************************/
void showMessages(msgStruct** msgArray, int size){
  int i = 0;

  for(i = 0; i < size; i++){
    if(msgArray[i]->time != -1){
      printf("%d;%s\n", msgArray[i]->time, msgArray[i]->msg);
    }
  }
}


/***************************************************************************************************
    Função: liberta a memória de msgArray

    Parâmetros: array de mensagens, número máximo de mensagens

    Return: void
****************************************************************************************************/
void freeMsgArray(msgStruct** msgArray, int size){
  int i = 0;

  for(i = 0; i < size; i++){
    free(msgArray[i]);
  }
  free(msgArray);
}
