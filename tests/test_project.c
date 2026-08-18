#include <stdio.h>
#include <string.h>
#ifdef _WIN32
#include <direct.h>
#define REMOVE_DIR(path) _rmdir(path)
#else
#include <unistd.h>
#define REMOVE_DIR(path) rmdir(path)
#endif
#include "cli.h"
#include "project.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static void output(FILE *f,char *b,size_t n){size_t got;rewind(f);got=fread(b,1U,n-1U,f);b[got]='\0';}
static void test_manifest(void){LumeProject p;project_init(&p);CHECK(project_load(&p,"tests/projects/basic"));CHECK(strcmp(p.name,"basic")==0);CHECK(strcmp(p.version,"0.1.0")==0);CHECK(strstr(p.entry_path,"src/principal.lume")!=NULL);project_free(&p);
 project_init(&p);CHECK(!project_load(&p,"tests/projects/invalid/missing"));CHECK(strstr(p.message,"obrigatorias")!=NULL);project_free(&p);
 project_init(&p);CHECK(!project_load(&p,"tests/projects/invalid/duplicate"));CHECK(strstr(p.message,"mais de uma vez")!=NULL);project_free(&p);
 project_init(&p);CHECK(!project_load(&p,"tests/projects/invalid/unknown"));CHECK(strstr(p.message,"Voce quis dizer 'entrada'")!=NULL);project_free(&p);
 project_init(&p);CHECK(!project_load(&p,"tests/projects/invalid/version"));CHECK(strstr(p.message,"X.Y.Z")!=NULL);project_free(&p);}
static void test_commands(void){RuntimeIO io;char b[8192];FILE *f=tmpfile();char *run[]={"lume","executar","tests/projects/basic"};char *paths[]={"lume","executar","tests/projects/module_path"};char *verify[]={"lume","verificar","tests/projects/no_run"};io.input=stdin;io.output=f;CHECK(cli_run(3,run,io)==0);output(f,b,sizeof(b));CHECK(strstr(b,"30")!=NULL);freopen(NULL,"w+",f);CHECK(cli_run(3,paths,io)==0);output(f,b,sizeof(b));CHECK(strstr(b,"42")!=NULL);freopen(NULL,"w+",f);CHECK(cli_run(3,verify,io)==0);output(f,b,sizeof(b));CHECK(strstr(b,"NAO EXECUTAR")==NULL);CHECK(strstr(b,"Verificando projeto")!=NULL);fclose(f);}
static void test_create(void){char message[512];CHECK(project_create("tests","projeto_temporario_lume",message,sizeof(message)));CHECK(!project_create("tests","projeto_temporario_lume",message,sizeof(message)));CHECK(remove("tests/projeto_temporario_lume/src/principal.lume")==0);CHECK(remove("tests/projeto_temporario_lume/lume.projeto")==0);CHECK(REMOVE_DIR("tests/projeto_temporario_lume/src")==0);CHECK(REMOVE_DIR("tests/projeto_temporario_lume")==0);}
static void test_cycle(void){RuntimeIO io;char b[4096];FILE *f=tmpfile();char *args[]={"lume","verificar","tests/projects/cycle"};io.input=stdin;io.output=f;CHECK(cli_run(3,args,io)==1);output(f,b,sizeof(b));CHECK(strstr(b,"Import circular")!=NULL);fclose(f);}
int main(void){test_manifest();test_commands();test_create();test_cycle();if(failures==0){puts("Todos os testes de projeto passaram.");return 0;}return 1;}
