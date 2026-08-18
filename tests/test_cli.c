#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define CHANGE_DIR(path) _chdir(path)
#define GET_CWD(buffer,size) _getcwd((buffer),(int)(size))
#else
#include <unistd.h>
#define CHANGE_DIR(path) chdir(path)
#define GET_CWD(buffer,size) getcwd((buffer),(size))
#endif
#include "cli.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static size_t read_output(FILE *file,char *buffer,size_t capacity){size_t n;rewind(file);n=fread(buffer,1U,capacity-1U,file);buffer[n]='\0';return n;}
static void test_commands(void){RuntimeIO io;char buffer[4096];FILE *out=tmpfile();char *version[]={"lume","--versao"};char *help[]={"lume","--ajuda"};char *bad[]={"lume","--desconhecida"};char *explain[]={"lume","--explicar","exemplos/variaveis.lume"};io.input=stdin;io.output=out;
 CHECK(cli_run(2,version,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Lume 0.1.0")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(2,help,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"arquivo.lume")!=NULL);CHECK(strstr(buffer,"--expr")!=NULL);
 CHECK(strstr(buffer,"lume novo")!=NULL);CHECK(strstr(buffer,"lume executar")!=NULL);CHECK(strstr(buffer,"lume verificar")!=NULL);CHECK(strstr(buffer,"lume resolver")!=NULL);CHECK(strstr(buffer,"lume testar")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(2,bad,io)==2);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Opcao desconhecida")!=NULL);
 freopen(NULL,"w+",out);CHECK(cli_run(3,explain,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Declarou a variavel")!=NULL);CHECK(strstr(buffer,"Chamou a nativa 'escreva'")!=NULL);fclose(out);}
static void test_default_dispatch(void){char cwd[4096],buffer[4096];FILE *in=tmpfile(),*out=tmpfile();RuntimeIO io={in,out};char *args[]={"lume"};fputs(":sair\n",in);rewind(in);CHECK(cli_run(1,args,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Modo interativo")!=NULL);fclose(out);out=tmpfile();io.output=out;CHECK(GET_CWD(cwd,sizeof(cwd))!=NULL);CHECK(CHANGE_DIR("tests/projects/basic")==0);CHECK(cli_run(1,args,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"30")!=NULL);CHECK(strstr(buffer,"Modo interativo")==NULL);CHECK(CHANGE_DIR(cwd)==0);fclose(in);fclose(out);}
static void test_scripts_and_project_tests(void){char buffer[8192];const char *first,*second;FILE *out=tmpfile();RuntimeIO io={stdin,out};char *script[]={"lume","exemplos/ola.lume"};char *tests[]={"lume","testar","tests/projects/cli_tests"};char *missing[]={"lume","testar","tests/projects/basic"};char *failing[]={"lume","testar","tests/projects/cli_tests_fail"};CHECK(cli_run(2,script,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Olá, mundo!")!=NULL);freopen(NULL,"w+",out);CHECK(cli_run(3,tests,io)==0);read_output(out,buffer,sizeof(buffer));first=strstr(buffer,"a_primeiro.lume");second=strstr(buffer,"z_ultimo.lume");CHECK(first!=NULL&&second!=NULL&&first<second);CHECK(strstr(buffer,"2 teste(s) aprovado(s).")!=NULL);CHECK(strstr(buffer,"SAIDA OCULTA")==NULL);freopen(NULL,"w+",out);CHECK(cli_run(3,missing,io)==0);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"Nenhum teste encontrado.")!=NULL);freopen(NULL,"w+",out);CHECK(cli_run(3,failing,io)==1);read_output(out,buffer,sizeof(buffer));CHECK(strstr(buffer,"falhou")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"1 falha(s).")!=NULL);fclose(out);}
int main(void){test_commands();test_default_dispatch();test_scripts_and_project_tests();if(failures==0){puts("Todos os testes da CLI passaram.");return 0;}return 1;}
