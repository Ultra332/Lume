#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "analyzer.h"
#include "cli.h"
#include "interpreter.h"
#include "lexer.h"
#include "parser.h"

static int failures = 0;
#define CHECK(c) do { if (!(c)) { fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #c); failures++; } } while (0)

typedef struct {
    Source source; TokenArray tokens; ErrorList errors; Program *program;
    Environment environment; FILE *output; bool ok;
} Run;

static Run run_text(const char *text) {
    Run run; RuntimeIO io;
    source_init(&run.source); token_array_init(&run.tokens); error_list_init(&run.errors);
    run.program = NULL; environment_init(&run.environment, NULL);
    run.output = tmpfile(); run.ok = run.output != NULL && source_from_bytes(&run.source, "estabilidade.lume", text, strlen(text));
    if (run.ok) run.ok = lexer_scan(&run.source, &run.tokens, &run.errors);
    if (run.ok) run.ok = parser_parse_program(&run.tokens, &run.program, &run.errors);
    io.input = stdin; io.output = run.output;
    if (run.ok) run.ok = interpreter_execute_program_with_io(run.program, &run.environment, &io, &run.errors);
    return run;
}
static void run_free(Run *run) {
    if (run->output != NULL) fclose(run->output);
    environment_free(&run->environment); program_free(run->program); error_list_free(&run->errors);
    token_array_free(&run->tokens); source_free(&run->source);
}
static void expect_failure(const char *text, LumeErrorKind kind) {
    Run run = run_text(text); CHECK(!run.ok); CHECK(run.errors.count > 0U);
    if (run.errors.count > 0U) { CHECK(run.errors.data[0].kind == kind); CHECK(run.errors.data[0].message != NULL); }
    run_free(&run);
}
static bool integer_binding(Run *run, const char *name, int64_t expected) {
    SourceSpan span = {{0U,1U,1U},{0U,1U,1U},NULL}; Value value = value_null();
    bool ok = environment_get(&run->environment, name, strlen(name), &value, span, &run->errors);
    ok = ok && value.type == VALUE_INTEGER && value.as.integer == expected; value_free(&value); return ok;
}
static void test_empty_comments_and_incomplete_inputs(void) {
    Run run = run_text(""); CHECK(run.ok); run_free(&run);
    run = run_text("// somente comentario\n// segunda linha\n"); CHECK(run.ok); run_free(&run);
    run = run_text("variavel vazia = \"\"\n"); CHECK(run.ok); run_free(&run);
    expect_failure("\"texto sem fim", LUME_ERROR_LEXICAL);
    expect_failure("variavel x = (1 + 2\n", LUME_ERROR_SYNTAX);
    expect_failure("se verdadeiro {\n variavel x=1\n", LUME_ERROR_SYNTAX);
    expect_failure("variavel xs=[1,2\n", LUME_ERROR_SYNTAX);
    expect_failure("funcao f(a,b { retorne a }\n", LUME_ERROR_SYNTAX);
}
static void test_runtime_boundaries(void) {
    expect_failure("variavel x=10/0\n", LUME_ERROR_NUMERIC);
    expect_failure("variavel x=\"a\"-1\n", LUME_ERROR_TYPE);
    expect_failure("variavel xs=[]\nvariavel x=xs[0]\n", LUME_ERROR_INDEX);
    expect_failure("variavel x=9223372036854775807+1\n", LUME_ERROR_NUMERIC);
    { Run run=run_text("variavel xs=[]\npara i de 1 ate 500 { adicione(xs,i) }\nvariavel total=tamanho(xs)\n"); CHECK(run.ok); CHECK(integer_binding(&run,"total",500)); run_free(&run); }
}
static void test_large_source(void) {
    size_t index, used = 0U, capacity = 65536U; char *code = malloc(capacity); Run run;
    CHECK(code != NULL); if (code == NULL) return;
    for (index=0U; index<300U; index++) used += (size_t)snprintf(code+used,capacity-used,"variavel v%zu=%zu\n",index,index);
    used += (size_t)snprintf(code+used,capacity-used,"variavel resultado=v0+v299\n");
    CHECK(used < capacity); run=run_text(code); CHECK(run.ok); CHECK(integer_binding(&run,"resultado",299)); run_free(&run); free(code);
}
static void test_large_string_and_analyzer(void) {
    static const char *prefix="variavel texto_grande=\"";static const char *suffix="\"\nvariavel total=tamanho(texto_grande)\n";size_t length=8192U,index,prefix_length=strlen(prefix),suffix_length=strlen(suffix);char *code=malloc(prefix_length+length+suffix_length+1U);Run run;Source source;TokenArray tokens;ErrorList errors;Program *program=NULL;AnalysisResult result;bool ok;
    CHECK(code!=NULL);if(code==NULL)return;memcpy(code,prefix,prefix_length);for(index=0U;index<length;index++)code[prefix_length+index]='a';memcpy(code+prefix_length+length,suffix,suffix_length+1U);
    run=run_text(code);CHECK(run.ok);CHECK(integer_binding(&run,"total",(int64_t)length));run_free(&run);
    source_init(&source);token_array_init(&tokens);error_list_init(&errors);analysis_result_init(&result);ok=source_from_bytes(&source,"analise-grande.lume",code,strlen(code));if(ok)ok=lexer_scan(&source,&tokens,&errors);if(ok)ok=parser_parse_program(&tokens,&program,&errors);if(ok)ok=analyzer_analyze(program,&result);CHECK(ok);CHECK(result.metrics.variables==2U);analysis_result_free(&result);program_free(program);error_list_free(&errors);token_array_free(&tokens);source_free(&source);free(code);
}
static char *nested_expression(size_t depth,char open,char close) {
    const char *prefix="variavel resultado=";size_t prefix_length=strlen(prefix),index;char *code=malloc(prefix_length+depth*2U+3U);if(code==NULL)return NULL;memcpy(code,prefix,prefix_length);for(index=0U;index<depth;index++)code[prefix_length+index]=open;code[prefix_length+depth]='1';for(index=0U;index<depth;index++)code[prefix_length+depth+1U+index]=close;code[prefix_length+depth*2U+1U]='\n';code[prefix_length+depth*2U+2U]='\0';return code;
}
static void test_parser_depth_limit(void) {
    char *code=nested_expression(128U,'(',')');Run run;CHECK(code!=NULL);if(code==NULL)return;run=run_text(code);CHECK(run.ok);run_free(&run);free(code);
    code=nested_expression(129U,'(',')');CHECK(code!=NULL);if(code==NULL)return;run=run_text(code);CHECK(!run.ok);CHECK(run.errors.count==1U);if(run.errors.count==1U)CHECK(strstr(run.errors.data[0].message,"profundamente")!=NULL);run_free(&run);free(code);
    code=nested_expression(128U,'[',']');CHECK(code!=NULL);if(code==NULL)return;run=run_text(code);CHECK(run.ok);run_free(&run);free(code);
    code=nested_expression(129U,'[',']');CHECK(code!=NULL);if(code==NULL)return;run=run_text(code);CHECK(!run.ok);CHECK(run.errors.count==1U);run_free(&run);
    {FILE *file;RuntimeIO io={stdin,tmpfile()};char *argv[]={"lume","--analisar","tests/.tmp-parser-depth.lume"};CHECK(io.output!=NULL);file=fopen(argv[2],"wb");CHECK(file!=NULL);if(file!=NULL){fwrite(code,1U,strlen(code),file);fclose(file);CHECK(cli_run(3,argv,io)==1);remove(argv[2]);}if(io.output!=NULL)fclose(io.output);}free(code);
}
static void test_temporary_environment_ownership(void) {
    Run run = run_text(
        "variavel contador=0\n"
        "enquanto contador<10000 { contador=contador+1 }\n");
    CHECK(run.ok); CHECK(integer_binding(&run,"contador",10000));
    CHECK(environment_retained_child_count(&run.environment)==0U); run_free(&run);

    run = run_text(
        "funcao identidade(valor){ retorne valor }\n"
        "variavel i=0\n"
        "enquanto i<10000 { identidade(i); i=i+1 }\n");
    CHECK(run.ok); CHECK(environment_retained_child_count(&run.environment)==0U); run_free(&run);

    run = run_text(
        "variavel i=0\n"
        "enquanto i<2000 { variavel xs=[i,i+1]; variavel texto=\"temporario\"; i=i+1 }\n");
    CHECK(run.ok); CHECK(environment_retained_child_count(&run.environment)==0U); run_free(&run);

    run = run_text(
        "funcao fabrica(valor){ funcao interna(){ retorne valor }; retorne interna }\n"
        "variavel closure=fabrica(42)\nvariavel resultado=closure()\n");
    CHECK(run.ok); CHECK(integer_binding(&run,"resultado",42));
    CHECK(environment_retained_child_count(&run.environment)>0U); run_free(&run);
}
int main(void) {
    test_empty_comments_and_incomplete_inputs(); test_runtime_boundaries(); test_large_source(); test_large_string_and_analyzer(); test_parser_depth_limit(); test_temporary_environment_ownership();
    if (failures==0) { puts("Todos os testes de estabilidade passaram."); return 0; }
    fprintf(stderr,"%d teste(s) de estabilidade falharam.\n",failures); return 1;
}
