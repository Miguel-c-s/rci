#ifndef messagesHeader
#define messagesHeader

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

#define max(A,B) ((A)>=(B)?(A):(B))

/* definição do tipo de estrutura msgStruct que guarda um inteiro para o tempo lógico
e uma string para a mensagem */
typedef struct msgStruct{
  int time;
  char msg[141];
}msgStruct;

msgStruct** createMsgArray(int size);
void insertMsg(msgStruct** msgArray, int size, char* buffer);
char* getMessages(msgStruct** msgArray, int size, char* buffer);
void sendMessages(int sd, msgStruct** msgArray, int size, int mode);
void receiveMessages(msgStruct** msgArray, int size, char* buffer);
void showMessages(msgStruct** msgArray, int size);
void freeMsgArray(msgStruct** msgArray, int size);

#endif
