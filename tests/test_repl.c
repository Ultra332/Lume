#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "repl.h"
#include "session.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static void write_input(FILE *file,const char *text){fwrite(text,1U,strlen(text),file);rewind(file);}
static void test_repl(void){FILE *in=tmpfile(),*out=tmpfile();RuntimeIO io;char buffer[8192];size_t n;io.input=in;io.output=out;
 write_input(in,"variavel x = 10\nx = x + 5\nx\n10 + \"oi\"\nx\nfuncao dobro(n) {\n retorne n * 2\n}\ndobro(x)\nvariavel lista=[1,2,3]\nlista[1]=20\nlista\n:sair\n");
 CHECK(repl_run(io)==0);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"15\n")!=NULL);CHECK(strstr(buffer,"30\n")!=NULL);CHECK(strstr(buffer,"[1, 20, 3]\n")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"... ")!=NULL);fclose(in);fclose(out);}
static void test_incomplete(void){const char *a="funcao teste() {\n",*b="escreva(\"{\")\n",*c="variavel x=[\n1,\n";CHECK(session_classify(a,strlen(a))==INPUT_INCOMPLETE);CHECK(session_classify(b,strlen(b))==INPUT_COMPLETE);CHECK(session_classify(c,strlen(c))==INPUT_INCOMPLETE);}
static void expect_conversion_error(const char *code){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession s;ErrorList errors;Source *source=NULL;session_init(&s,io);error_list_init(&errors);CHECK(!session_execute(&s,"<teste>",code,strlen(code),true,&source,&errors));CHECK(errors.count==1U&&errors.data[0].kind==LUME_ERROR_CONVERSION);if(source){source_free(source);free(source);}error_list_free(&errors);session_free(&s);fclose(out);}
static void test_conversions(void){FILE *out=tmpfile();RuntimeIO io={stdin,out};LumeSession s;ErrorList errors;Source *source=NULL;char buffer[512];size_t n;session_init(&s,io);error_list_init(&errors);
 {const char *code="escreva(texto(10))\nescreva(texto(verdadeiro))\nescreva(inteiro(\"42\"))\nescreva(inteiro(10.0))\nescreva(decimal(\"3.14\"))\nescreva(decimal(10))\nescreva(tipo(verdadeiro))\n";CHECK(session_execute(&s,"<teste>",code,strlen(code),false,&source,&errors));}
 rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"10\nverdadeiro\n42\n10\n3.14\n10\nbooleano\n")!=NULL);error_list_free(&errors);session_free(&s);fclose(out);
 expect_conversion_error("inteiro(10.5)\n");expect_conversion_error("inteiro(\"42abc\")\n");expect_conversion_error("inteiro(\"999999999999999999999\")\n");expect_conversion_error("decimal(\"3.14abc\")\n");}
int main(void){test_repl();test_incomplete();test_conversions();if(failures==0){puts("Todos os testes do REPL passaram.");return 0;}return 1;}
