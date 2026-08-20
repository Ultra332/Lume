#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"
#include "session.h"
#include "diagnostic.h"
#include "memory.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static void write_input(FILE *file,const char *text){fwrite(text,1U,strlen(text),file);rewind(file);}
static void test_repl(void){FILE *in=tmpfile(),*out=tmpfile();RuntimeIO io;char buffer[8192];size_t n;io.input=in;io.output=out;
 write_input(in,"variavel x = 10\nx = x + 5\nx\n10 + \"oi\"\nx\n[][0]\nx\ninteiro(\"invalido\")\nx\nfuncao dobro(n) {\n retorne n * 2\n}\ndobro(x)\nvariavel lista=[1,2,3]\nlista[1]=20\nlista\n:sair\n");
 CHECK(repl_run(io)==0);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"15\n")!=NULL);CHECK(strstr(buffer,"30\n")!=NULL);CHECK(strstr(buffer,"[1, 20, 3]\n")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"Erro de indice")!=NULL);CHECK(strstr(buffer,"Erro de conversao")!=NULL);CHECK(strstr(buffer,"... ")!=NULL);fclose(in);fclose(out);}
static void test_incomplete(void){const char *a="funcao teste() {\n",*b="escreva(\"{\")\n",*c="variavel x=[\n1,\n";CHECK(session_classify(a,strlen(a))==INPUT_INCOMPLETE);CHECK(session_classify(b,strlen(b))==INPUT_COMPLETE);CHECK(session_classify(c,strlen(c))==INPUT_INCOMPLETE);}
static size_t count_text(const char *text,const char *needle){size_t count=0U;size_t length=strlen(needle);const char *at=text;while((at=strstr(at,needle))!=NULL){count++;at+=length;}return count;}
static void test_repl_null_presentation(void){FILE *in=tmpfile(),*out=tmpfile();RuntimeIO io;char buffer[2048];size_t n;io.input=in;io.output=out;
 write_input(in,"escreva(\"Olá\")\n10 + 20\nnulo\nescreva(nulo)\n:sair\n");CHECK(repl_run(io)==0);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';
 CHECK(strstr(buffer,"Olá\n>>> ")!=NULL);CHECK(strstr(buffer,"30\n")!=NULL);CHECK(count_text(buffer,"nulo\n")==2U);CHECK(strstr(buffer,"Olá\nnulo\n")==NULL);fclose(in);fclose(out);}
static void expect_conversion_error(const char *code){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession s;ErrorList errors;Source *source=NULL;session_init(&s,io);error_list_init(&errors);CHECK(!session_execute(&s,"<teste>",code,strlen(code),true,&source,&errors));CHECK(errors.count==1U&&errors.data[0].kind==LUME_ERROR_CONVERSION);if(source){source_free(source);free(source);}error_list_free(&errors);session_free(&s);fclose(out);}
static void test_conversions(void){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession s;ErrorList errors;Source *source=NULL;char buffer[512];size_t n;session_init(&s,io);error_list_init(&errors);
 {const char *code="escreva(texto(10))\nescreva(texto(verdadeiro))\nescreva(inteiro(\"42\"))\nescreva(inteiro(10.0))\nescreva(decimal(\"3.14\"))\nescreva(decimal(10))\nescreva(tipo(verdadeiro))\n";CHECK(session_execute(&s,"<teste>",code,strlen(code),false,&source,&errors));}
 rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"10\nverdadeiro\n42\n10\n3.14\n10\nbooleano\n")!=NULL);error_list_free(&errors);session_free(&s);fclose(out);
 expect_conversion_error("inteiro(10.5)\n");expect_conversion_error("inteiro(\"42abc\")\n");expect_conversion_error("inteiro(\"999999999999999999999\")\n");expect_conversion_error("decimal(\"3.14abc\")\n");}
static void test_recursion_error_recovery_and_source(void){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession s;ErrorList errors;Source *source=NULL;char buffer[4096];size_t n;const char *definition="funcao loop(){ retorne loop() }\n";const char *call="loop()\n";const char *valid_definition="funcao dobro(n){ retorne n*2 }\n";const char *valid_call="dobro(10)\n";
 session_init(&s,io);error_list_init(&errors);CHECK(session_execute_repl(&s,"<repl:1>",definition,strlen(definition),&source,&errors));
 source=NULL;CHECK(!session_execute_repl(&s,"<repl:2>",call,strlen(call),&source,&errors));CHECK(errors.count==1U);if(source!=NULL&&errors.count==1U)diagnostic_render(out,source,&errors.data[0]);
 if(source!=NULL){source_free(source);memory_free(source);}error_list_free(&errors);error_list_init(&errors);source=NULL;
 CHECK(session_execute_repl(&s,"<repl:3>",valid_definition,strlen(valid_definition),&source,&errors));source=NULL;CHECK(session_execute_repl(&s,"<repl:4>",valid_call,strlen(valid_call),&source,&errors));rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';
 CHECK(strstr(buffer,"loop()\n^^^^^^")!=NULL);CHECK(strstr(buffer,"Limite de chamadas de funcao")!=NULL);CHECK(strstr(buffer,"20\n")!=NULL);
 error_list_free(&errors);session_free(&s);fclose(out);}
static void test_old_source_diagnostics(void){FILE *in=tmpfile(),*out=tmpfile();RuntimeIO io={in,out};char buffer[16384];size_t n;
 write_input(in,"funcao nome_antigo(){ retorne inexistente }\nnome_antigo()\nfuncao tipo_antigo(){ retorne \"x\"-1 }\ntipo_antigo()\nfuncao indice_antigo(){ retorne [1][9] }\nindice_antigo()\nfuncao fabrica_erro(){ funcao interna(){ retorne ausente }; retorne interna }\nvariavel closure_erro=fabrica_erro()\nclosure_erro()\n10+20\n:sair\n");
 CHECK(repl_run(io)==0);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"retorne inexistente")!=NULL);CHECK(strstr(buffer,"retorne \"x\"-1")!=NULL);CHECK(strstr(buffer,"retorne [1][9]")!=NULL);CHECK(strstr(buffer,"retorne ausente")!=NULL);CHECK(strstr(buffer,"Nome: 'inexistente'")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"Erro de indice")!=NULL);CHECK(strstr(buffer,"30\n")!=NULL);fclose(in);fclose(out);}
static void test_repeated_session_memory(void){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession session;ErrorList errors;Source *source=NULL;size_t i;const char *loop="{ variavel i=0; enquanto i<50 { i=i+1 } }\n";const char *import_line="importe \"tests/modules/matematica\"\n";
 session_init(&session,io);error_list_init(&errors);for(i=0U;i<100U;i++){source=NULL;CHECK(session_execute_repl(&session,"<memoria>",loop,strlen(loop),&source,&errors));CHECK(errors.count==0U);}CHECK(environment_retained_child_count(&session.environment)==0U);
 source=NULL;CHECK(session_execute_repl(&session,"<modulo>",import_line,strlen(import_line),&source,&errors));for(i=0U;i<50U;i++){source=NULL;CHECK(session_execute_repl(&session,"<modulo>",import_line,strlen(import_line),&source,&errors));}CHECK(session.modules.count==1U);CHECK(environment_retained_child_count(&session.environment)==0U);if(session.modules.count==1U)CHECK(environment_retained_child_count(&session.modules.modules[0]->environment)==0U);
 error_list_free(&errors);session_free(&session);fclose(out);}
int main(void){test_repl();test_repl_null_presentation();test_incomplete();test_conversions();test_recursion_error_recovery_and_source();test_old_source_diagnostics();test_repeated_session_memory();if(failures==0){puts("Todos os testes do REPL passaram.");return 0;}return 1;}
