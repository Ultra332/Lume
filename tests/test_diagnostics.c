#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diagnostic.h"
#include "memory.h"
#include "session.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
static const char *fonte="variavel x = 10\nx + \"oi\"\n";
/* Regressao: o diagnostico e exibido depois de session_execute liberar a AST.
   O nome precisa sobreviver a esse free (era um heap-use-after-free). */
static void test_subject_survives_program_free(void){LumeSession session;ErrorList errors;Source *source=NULL;FILE *out=tmpfile();char buffer[1024];size_t n;RuntimeIO io;const char *codigo="escreva(xyz)\n";
 io.input=stdin;io.output=out;session_init(&session,io);error_list_init(&errors);
 CHECK(session_execute(&session,"regressao.lume",codigo,strlen(codigo),false,&source,&errors)==false);
 CHECK(errors.count>0U);
 if(errors.count>0U)diagnostic_render(out,source,&errors.data[0]);
 rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';
 CHECK(strstr(buffer,"Nome: 'xyz'")!=NULL);
 if(source!=NULL){source_free(source);memory_free(source);}
 error_list_free(&errors);session_free(&session);fclose(out);}
int main(void){Source source;LumeError error;FILE *out=tmpfile();char buffer[1024];size_t n;source_init(&source);CHECK(source_from_bytes(&source,"exemplo.lume",fonte,strlen(fonte)));error.kind=LUME_ERROR_TYPE;error.span.start.offset=20U;error.span.start.line=2U;error.span.start.column=4U;error.span.end.offset=21U;error.span.end.line=2U;error.span.end.column=5U;error.span.source=NULL;error.message="tipos incompativeis";error.suggestion="Use valores do mesmo tipo.";error.subject=NULL;error.subject_length=0U;diagnostic_render(out,&source,&error);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"exemplo.lume:2:4")!=NULL);CHECK(strstr(buffer,"x + \"oi\"")!=NULL);CHECK(strstr(buffer,"^")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"Dica:")!=NULL);fclose(out);source_free(&source);test_subject_survives_program_free();if(failures==0){puts("Todos os testes de diagnosticos passaram.");return 0;}fprintf(stderr,"%d teste(s) falharam.\n",failures);return 1;}
