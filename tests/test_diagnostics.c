#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "diagnostic.h"
static int failures=0;
#define CHECK(c) do{if(!(c)){fprintf(stderr,"FALHA %s:%d: %s\n",__FILE__,__LINE__,#c);failures++;}}while(0)
int main(void){Source source;LumeError error;FILE *out=tmpfile();char buffer[1024];size_t n;source_init(&source);CHECK(source_from_bytes(&source,"exemplo.lume","variavel x = 10\nx + \"oi\"\n",28U));error.kind=LUME_ERROR_TYPE;error.span.start.offset=20U;error.span.start.line=2U;error.span.start.column=4U;error.span.end.offset=21U;error.span.end.line=2U;error.span.end.column=5U;error.message="tipos incompativeis";error.suggestion="Use valores do mesmo tipo.";error.subject=NULL;error.subject_length=0U;diagnostic_render(out,&source,&error);rewind(out);n=fread(buffer,1U,sizeof(buffer)-1U,out);buffer[n]='\0';CHECK(strstr(buffer,"exemplo.lume:2:4")!=NULL);CHECK(strstr(buffer,"x + \"oi\"")!=NULL);CHECK(strstr(buffer,"^")!=NULL);CHECK(strstr(buffer,"Erro de tipo")!=NULL);CHECK(strstr(buffer,"Dica:")!=NULL);fclose(out);source_free(&source);if(failures==0){puts("Todos os testes de diagnosticos passaram.");return 0;}return 1;}
