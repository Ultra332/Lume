#include <stdio.h>
#include <string.h>
#include "cli.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static size_t read_output(FILE *file,char *buffer,size_t capacity){size_t n;rewind(file);n=fread(buffer,1U,capacity-1U,file);buffer[n]='\0';return n;}
static void test_commands(void){RuntimeIO io;char buffer[4096];FILE *out=tmpfile();char *version[]={"lume","--versao"};char *help[]={"lume","--ajuda"};char *bad[]={"lume","--desconhecida"};char *explain[]={"lume","--explicar","exemplos/variaveis.lume"};io.input=stdin;io.output=out;
 CHECK(cli_run(2,version,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Lume 0.1.0")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(2,help,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"arquivo.lume")!=NULL);CHECK(strstr(buffer,"--expr")!=NULL);
 CHECK(strstr(buffer,"lume novo")!=NULL);CHECK(strstr(buffer,"lume executar")!=NULL);CHECK(strstr(buffer,"lume verificar")!=NULL);CHECK(strstr(buffer,"lume resolver")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(2,bad,io)==2);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Opcao desconhecida")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(3,explain,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Declarou a variavel")!=NULL);CHECK(strstr(buffer,"Chamou a nativa 'escreva'")!=NULL);fclose(out);}
int main(void){test_commands();if(failures==0){puts("Todos os testes da CLI passaram.");return 0;}return 1;}
