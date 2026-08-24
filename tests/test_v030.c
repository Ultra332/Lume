#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "analyzer.h"
#include "diagnostic.h"
#include "education.h"
#include "environment.h"
#include "interpreter.h"
#include "lexer.h"
#include "list.h"
#include "module.h"
#include "parser.h"

static int failures = 0;

#define CHECK(condition) do { \
    if (!(condition)) { \
        fprintf(stderr, "FALHA %s:%d: %s\n", __FILE__, __LINE__, #condition); \
        failures++; \
    } \
} while (0)

typedef struct {
    Source source;
    TokenArray tokens;
    ErrorList errors;
    Program *program;
} ParsedProgram;

static bool parsed_program_init(ParsedProgram *parsed, const char *name,
                                const char *code) {
    bool ok;
    source_init(&parsed->source);
    token_array_init(&parsed->tokens);
    error_list_init(&parsed->errors);
    parsed->program = NULL;
    ok = source_from_bytes(&parsed->source, name, code, strlen(code));
    if (ok) ok = lexer_scan(&parsed->source, &parsed->tokens, &parsed->errors);
    if (ok) ok = parser_parse_program(&parsed->tokens, &parsed->program,
                                      &parsed->errors);
    return ok;
}

static void parsed_program_free(ParsedProgram *parsed) {
    program_free(parsed->program);
    error_list_free(&parsed->errors);
    token_array_free(&parsed->tokens);
    source_free(&parsed->source);
}

static size_t read_output(FILE *file, char *buffer, size_t capacity) {
    size_t count;
    rewind(file);
    count = fread(buffer, 1U, capacity - 1U, file);
    buffer[count] = '\0';
    return count;
}

static bool name_is(const TraceEvent *event, const char *expected) {
    size_t length = strlen(expected);
    return event->name != NULL && event->name_length == length &&
        memcmp(event->name, expected, length) == 0;
}

static bool integer_is(const Value *value, int64_t expected) {
    return value != NULL && value->type == VALUE_INTEGER &&
        value->as.integer == expected;
}

typedef struct {
    size_t next_position;
    size_t multiply_position;
    size_t outer_add_position;
    size_t call_position;
    size_t enter_position;
    size_t return_position;
    bool call_mapping_ok;
    bool enter_mapping_ok;
    bool return_value_ok;
    bool return_span_ok;
} EventProbe;

static void probe_event(void *context, const TraceEvent *event) {
    EventProbe *probe = context;
    size_t position = probe->next_position++;
    if (event->type == TRACE_BINARY_EXPRESSION && event->expression != NULL &&
            event->expression->type == EXPR_BINARY) {
        BinaryOperator operator_type = event->expression->as.binary.operator_type;
        if (operator_type == BINARY_MULTIPLY && integer_is(event->left, 5) &&
                integer_is(event->right, 2) && integer_is(event->after, 10))
            probe->multiply_position = position;
        if (operator_type == BINARY_ADD && integer_is(event->left, 10) &&
                integer_is(event->right, 10) && integer_is(event->after, 20))
            probe->outer_add_position = position;
    }
    if (event->type == TRACE_FUNCTION_CALL && name_is(event, "combinar")) {
        probe->call_position = position;
        probe->call_mapping_ok = event->statement != NULL &&
            event->statement->type == STMT_FUNCTION &&
            event->statement->as.function.parameter_count == 2U &&
            event->argument_count == 2U && integer_is(&event->arguments[0], 20) &&
            integer_is(&event->arguments[1], 3) &&
            event->statement->as.function.parameter_lengths[0] == 1U &&
            event->statement->as.function.parameters[0][0] == 'a' &&
            event->statement->as.function.parameter_lengths[1] == 1U &&
            event->statement->as.function.parameters[1][0] == 'b';
    }
    if (event->type == TRACE_FUNCTION_ENTER && name_is(event, "combinar")) {
        probe->enter_position = position;
        probe->enter_mapping_ok = event->call_depth == 1U &&
            event->argument_count == 2U && integer_is(&event->arguments[0], 20) &&
            integer_is(&event->arguments[1], 3);
    }
    if (event->type == TRACE_FUNCTION_RETURN && name_is(event, "combinar")) {
        probe->return_position = position;
        probe->return_value_ok = event->call_depth == 1U &&
            integer_is(event->after, 23);
        probe->return_span_ok=event->span.start.line==1U&&event->statement!=NULL&&
            event->statement->type==STMT_RETURN;
    }
}

static void test_structured_events_and_order(void) {
    const char *code =
        "funcao combinar(a, b) { retorne a + b }\n"
        "variavel resultado = 10 + 5 * 2\n"
        "variavel chamada = combinar(resultado, 3)\n";
    ParsedProgram parsed;
    Environment environment;
    RuntimeIO io;
    RuntimeTrace trace;
    EventProbe probe;
    FILE *output = tmpfile();
    bool ok;
    CHECK(output != NULL);
    if (output == NULL) return;
    memset(&probe, 0, sizeof(probe));
    probe.multiply_position = SIZE_MAX;
    probe.outer_add_position = SIZE_MAX;
    probe.call_position = SIZE_MAX;
    probe.enter_position = SIZE_MAX;
    probe.return_position = SIZE_MAX;
    ok = parsed_program_init(&parsed, "tests/v030_eventos.lume", code);
    environment_init(&environment, NULL);
    io.input = stdin;
    io.output = output;
    trace.callback = probe_event;
    trace.context = &probe;
    trace.stop_requested = false;
    if (ok) ok = interpreter_execute_program_with_trace(parsed.program,
        &environment, &io, &trace, &parsed.errors);
    CHECK(ok);
    CHECK(probe.multiply_position < probe.outer_add_position);
    CHECK(probe.outer_add_position < probe.call_position);
    CHECK(probe.call_position < probe.enter_position);
    CHECK(probe.enter_position < probe.return_position);
    CHECK(probe.call_mapping_ok);
    CHECK(probe.enter_mapping_ok);
    CHECK(probe.return_value_ok);
    CHECK(probe.return_span_ok);
    environment_free(&environment);
    parsed_program_free(&parsed);
    fclose(output);
}

typedef struct {
    size_t rendered;
    size_t omitted;
} ExplainStats;

static bool explain_code(const char *name, const char *code, FILE *output,
                         ExplainStats *stats) {
    ParsedProgram parsed;
    Environment environment;
    RuntimeIO io;
    RuntimeTrace trace;
    EducationRenderer renderer;
    ModuleRegistry registry;
    LumeModule current;
    bool ok = parsed_program_init(&parsed, name, code);
    environment_init(&environment, NULL);
    io.input = stdin;
    io.output = output;
    trace.callback = education_trace_callback;
    trace.context = &renderer;
    trace.stop_requested = false;
    education_renderer_init(&renderer, &io, &parsed.source, &trace,
                            EDUCATION_EXPLAIN);
    module_registry_init(&registry, &io, &trace);
    memset(&current, 0, sizeof(current));
    current.path = (char *)name;
    if (ok) ok = interpreter_execute_program_with_modules(parsed.program,
        &environment, &io, &trace, &registry, &current, &parsed.errors);
    if (stats != NULL) {
        stats->rendered = renderer.rendered;
        stats->omitted = renderer.omitted;
    }
    environment_free(&environment);
    module_registry_free(&registry);
    parsed_program_free(&parsed);
    return ok;
}

static void test_conceptual_renderer(void) {
    const char *code =
        "constante BASE = 2\n"
        "variavel total = 10 + 5 * BASE\n"
        "se total > 100 { total = 0 } senao { total = total + 1 }\n"
        "funcao dobro(valor) { retorne valor * 2 }\n"
        "variavel calculado = dobro(total)\n"
        "variavel nomes = [\"Ana\", \"Bia\", \"Caio\"]\n"
        "para item em nomes {\n"
        "  se item == \"Ana\" { continue }\n"
        "  pare\n"
        "}\n"
        "variavel primeiro = nomes[0]\n"
        "nomes[0] = \"Lia\"\n";
    FILE *output = tmpfile();
    char buffer[65536];
    CHECK(output != NULL);
    if (output == NULL) return;
    CHECK(explain_code("tests/v030_conceitos.lume", code, output, NULL));
    (void)read_output(output, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "Criando a constante \"BASE\"") != NULL);
    CHECK(strstr(buffer, "5 * 2 = 10") != NULL);
    CHECK(strstr(buffer, "10 + 10 = 20") != NULL);
    CHECK(strstr(buffer, "Resultado: falso") != NULL);
    CHECK(strstr(buffer, "ramo 'senao'") != NULL);
    CHECK(strstr(buffer, "Chamando a funcao \"dobro\"") != NULL);
    CHECK(strstr(buffer, "valor = 21") != NULL);
    CHECK(strstr(buffer, "A funcao \"dobro\" retornou 42") != NULL);
    CHECK(strstr(buffer, "Criando uma lista com 3 elemento(s)") != NULL);
    CHECK(strstr(buffer, "'continue' encerra esta iteracao") != NULL);
    CHECK(strstr(buffer, "'pare' encerra o laco atual") != NULL);
    CHECK(strstr(buffer, "Acessando o indice 0") != NULL);
    CHECK(strstr(buffer, "Alterando o indice 0") != NULL);
    fclose(output);
}

static void test_bounded_value_rendering(void) {
    char bytes[200];
    char buffer[4096];
    FILE *output = tmpfile();
    Value text = value_null();
    Value list_value;
    LumeList *list;
    size_t index;
    CHECK(output != NULL);
    if (output == NULL) return;
    memset(bytes, 'a', sizeof(bytes));
    CHECK(value_string_copy(bytes, sizeof(bytes), &text));
    education_print_value(output, &text);
    fputc('\n', output);
    value_free(&text);
    list = list_new();
    CHECK(list != NULL);
    if (list == NULL) {
        fclose(output);
        return;
    }
    for (index = 0U; index < 10U; index++) {
        Value item = value_integer((int64_t)index);
        CHECK(list_append(list, &item));
    }
    list_value = value_list(list);
    education_print_value(output, &list_value);
    fputc('\n', output);
    value_free(&list_value);
    (void)read_output(output, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "...\" (200 bytes)") != NULL);
    CHECK(strstr(buffer, "[0, 1, 2, 3, 4, 5, ...] (10 elementos)") != NULL);
    CHECK(strlen(buffer) < 512U);
    fclose(output);
}

static size_t occurrence_count(const char *text,const char *needle) {
    size_t count=0U,length=strlen(needle);
    while((text=strstr(text,needle))!=NULL){count++;text+=length;}
    return count;
}

static void test_step_cancel_prevents_pending_native_effect(void) {
    const char *code="escreva(\"EFEITO_NATIVO\")\n";
    ParsedProgram parsed;
    Environment environment;
    RuntimeIO io;
    RuntimeTrace trace;
    EducationRenderer renderer;
    FILE *input=tmpfile(),*output=tmpfile();
    char buffer[8192];
    bool ok;
    CHECK(input!=NULL&&output!=NULL);
    if(input==NULL||output==NULL){if(input!=NULL)fclose(input);if(output!=NULL)fclose(output);return;}
    fputs("\nq\n",input);rewind(input);
    ok=parsed_program_init(&parsed,"tests/v030_cancelamento.lume",code);
    environment_init(&environment,NULL);
    io.input=input;io.output=output;
    trace.callback=education_trace_callback;trace.context=&renderer;trace.stop_requested=false;
    education_renderer_init(&renderer,&io,&parsed.source,&trace,EDUCATION_STEP);
    if(ok)ok=interpreter_execute_program_with_trace(parsed.program,&environment,&io,&trace,
        &parsed.errors);
    CHECK(!ok);CHECK(trace.stop_requested);
    (void)read_output(output,buffer,sizeof(buffer));
    CHECK(occurrence_count(buffer,"EFEITO_NATIVO")==1U);
    environment_free(&environment);parsed_program_free(&parsed);
    fclose(input);fclose(output);
}

static void test_million_iteration_explanation_is_bounded(void) {
    const char *code =
        "para i de 1 ate 1000000 { }\n";
    FILE *output = tmpfile();
    ExplainStats stats;
    char buffer[32768];
    long output_size;
    CHECK(output != NULL);
    if (output == NULL) return;
    CHECK(explain_code("tests/v030_milhao.lume", code, output, &stats));
    output_size = ftell(output);
    CHECK(output_size >= 0L);
    CHECK(output_size < 100000L);
    CHECK(stats.rendered <= 165U);
    CHECK(stats.omitted > 900000U);
    (void)read_output(output, buffer, sizeof(buffer));
    CHECK(strstr(buffer, "eventos intermediarios foram resumidos") != NULL);
    CHECK(strstr(buffer, "Fim do programa") != NULL);
    fclose(output);
}

static size_t analysis_count(const AnalysisResult *result, AnalysisCode code) {
    size_t index, count = 0U;
    for (index = 0U; index < result->count; index++)
        if (result->diagnostics[index].code == code) count++;
    return count;
}

static void test_analyzer_regressions(void) {
    const char *code =
        "funcao fluxo(usado, ignorado) {\n"
        "  variavel local = 1\n"
        "  retorne usado\n"
        "  variavel depois = 2\n"
        "}\n"
        "variavel solta = 3\n"
        "enquanto verdadeiro { pare; variavel apos_pare = 4 }\n";
    ParsedProgram parsed;
    AnalysisResult result;
    bool ok = parsed_program_init(&parsed, "tests/v030_analyzer.lume", code);
    analysis_result_init(&result);
    if (ok) ok = analyzer_analyze(parsed.program, &result);
    CHECK(ok);
    CHECK(analysis_count(&result, ANALYSIS_UNUSED_VARIABLE) >= 2U);
    CHECK(analysis_count(&result, ANALYSIS_UNUSED_PARAMETER) == 1U);
    CHECK(analysis_count(&result, ANALYSIS_UNUSED_FUNCTION) == 1U);
    CHECK(analysis_count(&result, ANALYSIS_UNREACHABLE) == 2U);
    CHECK(result.errors == 0U);
    analysis_result_free(&result);
    parsed_program_free(&parsed);
}

static void execute_error_case(const char *code, bool name_case) {
    ParsedProgram parsed;
    Environment environment;
    RuntimeIO io;
    FILE *output = tmpfile();
    char buffer[4096];
    bool ok;
    CHECK(output != NULL);
    if (output == NULL) return;
    ok = parsed_program_init(&parsed, "tests/v030_diagnostico.lume", code);
    environment_init(&environment, NULL);
    io.input = stdin;
    io.output = output;
    if (ok) ok = interpreter_execute_program_with_io(parsed.program, &environment,
        &io, &parsed.errors);
    CHECK(!ok);
    CHECK(parsed.errors.count == 1U);
    if (parsed.errors.count == 1U) {
        LumeError *error = &parsed.errors.data[0];
        diagnostic_render(output, &parsed.source, error);
        if (name_case) {
            CHECK(error->kind == LUME_ERROR_NAME);
            CHECK(error->replacement != NULL);
            CHECK(error->replacement_length == strlen("quantidade"));
            CHECK(memcmp(error->replacement, "quantidade",
                         error->replacement_length) == 0);
        } else {
            CHECK(error->kind == LUME_ERROR_TYPE);
            CHECK(error->left_type != NULL &&
                strcmp(error->left_type, "inteiro") == 0);
            CHECK(error->right_type != NULL &&
                strcmp(error->right_type, "texto") == 0);
        }
    }
    (void)read_output(output, buffer, sizeof(buffer));
    if (name_case)
        CHECK(strstr(buffer, "Talvez voce quisesse usar 'quantidade'.") != NULL);
    else {
        CHECK(strstr(buffer, "Tipos recebidos:") != NULL);
        CHECK(strstr(buffer, "Esquerda: inteiro") != NULL);
        CHECK(strstr(buffer, "Direita: texto") != NULL);
    }
    environment_free(&environment);
    parsed_program_free(&parsed);
    fclose(output);
}

static void test_diagnostic_details(void) {
    execute_error_case(
        "variavel quantidade = 10\nescreva(quantdade)\n", true);
    execute_error_case("variavel erro = 1 + \"x\"\n", false);
}

static void test_stdlib_modules_and_aliases(void) {
    const char *code =
        "importe \"lume/matematica\" como mat\n"
        "importe \"lume/terminal\" como term\n"
        "variavel raiz = mat.raiz(81)\n"
        "variavel estilo = term.estilize(\"ok\", \"branco\", \"preto\")\n";
    FILE *output = tmpfile();
    char buffer[32768];
    CHECK(output != NULL);
    if (output == NULL) return;
    CHECK(explain_code("tests/v030_modulos.lume", code, output, NULL));
    (void)read_output(output, buffer, sizeof(buffer));
    CHECK(strstr(buffer,
        "Importando o modulo \"lume/matematica\" como \"mat\"") != NULL);
    CHECK(strstr(buffer, "Modulo \"matematica\" carregado") != NULL);
    CHECK(strstr(buffer,
        "Importando o modulo \"lume/terminal\" como \"term\"") != NULL);
    CHECK(strstr(buffer, "Modulo \"terminal\" carregado") != NULL);
    CHECK(strstr(buffer, "Chamando a funcao nativa \"raiz\"") != NULL);
    CHECK(strstr(buffer, "Chamando a funcao nativa \"estilize\"") != NULL);
    fclose(output);
}

int main(void) {
    test_structured_events_and_order();
    test_conceptual_renderer();
    test_bounded_value_rendering();
    test_step_cancel_prevents_pending_native_effect();
    test_million_iteration_explanation_is_bounded();
    test_analyzer_regressions();
    test_diagnostic_details();
    test_stdlib_modules_and_aliases();
    if (failures == 0) {
        puts("Todos os testes educacionais da Lume v0.3.0 passaram.");
        return 0;
    }
    fprintf(stderr, "%d teste(s) falharam.\n", failures);
    return 1;
}
