#ifndef defsHeader
#define defsHeader

/* definição do tipo de estrutura servStruct que guarda todos os argumentos de entrada
quando o programa é executado */
typedef struct servStruct{
  char name[128];
  char ip[128];
  char upt[128];
  char tpt[128];
  char siip[128];
  char sipt[128];
  char r[128];
  char m[128];
}servStruct;

#endif
