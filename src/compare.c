#include "compare.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#include "ast.h"
#include "diagnostic.h"
#include "error.h"
#include "lexer.h"
#include "memory.h"
#include "parser.h"
#include "source.h"
#include "token.h"

typedef enum {
    FOUND_VARIABLE, FOUND_CONSTANT, FOUND_ASSIGNMENT, FOUND_EXPRESSION,
    FOUND_CONDITION, FOUND_COMPARISON, FOUND_LOOP, FOUND_FUNCTION,
    FOUND_PARAMETER, FOUND_ARGUMENT, FOUND_RETURN, FOUND_LIST,
    FOUND_INDEX, FOUND_INPUT, FOUND_OUTPUT, FOUND_COUNT
} FoundConcept;

typedef struct {
    FILE *output;
    bool supported;
    bool found[FOUND_COUNT];
    size_t function_depth;
} PythonRenderer;

static void indent(FILE *output, size_t depth) {
    size_t index;
    for (index = 0U; index < depth; index++) fputs("    ", output);
}

static const char *binary_operator(BinaryOperator operator_type) {
    switch (operator_type) {
        case BINARY_ADD: return "+";
        case BINARY_SUBTRACT: return "-";
        case BINARY_MULTIPLY: return "*";
        case BINARY_DIVIDE: return "/";
        case BINARY_REMAINDER: return "%";
        case BINARY_EQUAL: return "==";
        case BINARY_NOT_EQUAL: return "!=";
        case BINARY_LESS: return "<";
        case BINARY_LESS_EQUAL: return "<=";
        case BINARY_GREATER: return ">";
        case BINARY_GREATER_EQUAL: return ">=";
        case BINARY_LOGICAL_AND: return "and";
        case BINARY_LOGICAL_OR: return "or";
    }
    return "?";
}

static int expression_precedence(const Expr *expression) {
    if (expression == NULL) return 100;
    if (expression->type == EXPR_BINARY) {
        switch (expression->as.binary.operator_type) {
            case BINARY_LOGICAL_OR: return 10;
            case BINARY_LOGICAL_AND: return 20;
            case BINARY_EQUAL: case BINARY_NOT_EQUAL: return 30;
            case BINARY_LESS: case BINARY_LESS_EQUAL:
            case BINARY_GREATER: case BINARY_GREATER_EQUAL: return 40;
            case BINARY_ADD: case BINARY_SUBTRACT: return 50;
            case BINARY_MULTIPLY: case BINARY_DIVIDE: case BINARY_REMAINDER: return 60;
        }
    }
    if (expression->type == EXPR_UNARY) return 70;
    return 80;
}

static bool render_expression(PythonRenderer *renderer, const Expr *expression, int parent_precedence);

static bool render_literal(FILE *output, const Value *value) {
    char *formatted;
    size_t length;
    switch (value->type) {
        case VALUE_NULL: fputs("None", output); return true;
        case VALUE_BOOLEAN: fputs(value->as.boolean ? "True" : "False", output); return true;
        case VALUE_INTEGER: fprintf(output, "%" PRId64, value->as.integer); return true;
        case VALUE_DECIMAL: fprintf(output, "%.15g", value->as.decimal); return true;
        case VALUE_STRING:
            if (!value_format_nested(value, &formatted, &length)) return false;
            (void)fwrite(formatted, 1U, length, output);
            memory_free(formatted);
            return true;
        default: return false;
    }
}

static bool render_call(PythonRenderer *renderer, const Expr *expression) {
    const Expr *callee = expression->as.call.callee;
    const char *name;
    size_t index;
    if (callee->type != EXPR_IDENTIFIER) return false;
    name = callee->as.identifier.name;
    if (strcmp(name, "escreva") == 0) {
        fputs("print", renderer->output);
        renderer->found[FOUND_OUTPUT] = true;
    } else if (strcmp(name, "leia") == 0) {
        fputs("input", renderer->output);
        renderer->found[FOUND_INPUT] = true;
    } else if (strcmp(name, "tamanho") == 0) {
        fputs("len", renderer->output);
    } else {
        fputs(name, renderer->output);
    }
    fputc('(', renderer->output);
    for (index = 0U; index < expression->as.call.argument_count; index++) {
        if (index > 0U) fputs(", ", renderer->output);
        if (!render_expression(renderer, expression->as.call.arguments[index], 0)) return false;
        renderer->found[FOUND_ARGUMENT] = true;
    }
    fputc(')', renderer->output);
    return true;
}

static bool render_expression(PythonRenderer *renderer, const Expr *expression, int parent_precedence) {
    size_t index;
    int precedence;
    bool parentheses;
    if (expression == NULL) return false;
    renderer->found[FOUND_EXPRESSION] = true;
    precedence = expression_precedence(expression);
    parentheses = precedence < parent_precedence;
    if (parentheses) fputc('(', renderer->output);
    switch (expression->type) {
        case EXPR_LITERAL:
            if (!render_literal(renderer->output, &expression->as.literal)) return false;
            break;
        case EXPR_IDENTIFIER:
            fputs(expression->as.identifier.name, renderer->output);
            break;
        case EXPR_UNARY:
            if (expression->as.unary.operator_type == UNARY_NOT) fputs("not ", renderer->output);
            else fputc(expression->as.unary.operator_type == UNARY_NEGATIVE ? '-' : '+', renderer->output);
            if (!render_expression(renderer, expression->as.unary.operand, precedence)) return false;
            break;
        case EXPR_BINARY:
        {
            bool comparison = expression->as.binary.operator_type >= BINARY_EQUAL &&
                              expression->as.binary.operator_type <= BINARY_GREATER_EQUAL;
            if (comparison)
                renderer->found[FOUND_COMPARISON] = true;
            if (!render_expression(renderer, expression->as.binary.left,
                                   comparison ? precedence + 1 : precedence)) return false;
            fprintf(renderer->output, " %s ", binary_operator(expression->as.binary.operator_type));
            if (!render_expression(renderer, expression->as.binary.right, precedence + 1)) return false;
            break;
        }
        case EXPR_GROUPING:
            fputc('(', renderer->output);
            if (!render_expression(renderer, expression->as.grouping.expression, 0)) return false;
            fputc(')', renderer->output);
            break;
        case EXPR_CALL:
            if (!render_call(renderer, expression)) return false;
            break;
        case EXPR_LIST:
            renderer->found[FOUND_LIST] = true;
            fputc('[', renderer->output);
            for (index = 0U; index < expression->as.list.count; index++) {
                if (index > 0U) fputs(", ", renderer->output);
                if (!render_expression(renderer, expression->as.list.elements[index], 0)) return false;
            }
            fputc(']', renderer->output);
            break;
        case EXPR_INDEX:
            renderer->found[FOUND_INDEX] = true;
            if (!render_expression(renderer, expression->as.index.target, precedence)) return false;
            fputc('[', renderer->output);
            if (!render_expression(renderer, expression->as.index.index, 0)) return false;
            fputc(']', renderer->output);
            break;
        case EXPR_MEMBER:
            return false;
    }
    if (parentheses) fputc(')', renderer->output);
    return true;
}

static bool render_statement(PythonRenderer *renderer, const Stmt *statement, size_t depth);

static bool render_block(PythonRenderer *renderer, const Stmt *block, size_t depth) {
    size_t index;
    if (block == NULL || block->type != STMT_BLOCK) return false;
    if (block->as.block.statements.count == 0U) {
        indent(renderer->output, depth);
        fputs("pass\n", renderer->output);
        return true;
    }
    for (index = 0U; index < block->as.block.statements.count; index++)
        if (!render_statement(renderer, block->as.block.statements.data[index], depth)) return false;
    return true;
}

static bool render_if(PythonRenderer *renderer, const Stmt *statement, size_t depth, bool alternate) {
    indent(renderer->output, depth);
    fputs(alternate ? "elif " : "if ", renderer->output);
    if (!render_expression(renderer, statement->as.if_statement.condition, 0)) return false;
    fputs(":\n", renderer->output);
    if (!render_block(renderer, statement->as.if_statement.then_branch, depth + 1U)) return false;
    if (statement->as.if_statement.else_branch != NULL) {
        if (statement->as.if_statement.else_branch->type == STMT_IF)
            return render_if(renderer, statement->as.if_statement.else_branch, depth, true);
        indent(renderer->output, depth);
        fputs("else:\n", renderer->output);
        return render_block(renderer, statement->as.if_statement.else_branch, depth + 1U);
    }
    return true;
}

static bool render_statement(PythonRenderer *renderer, const Stmt *statement, size_t depth) {
    size_t index;
    if (statement == NULL) return false;
    switch (statement->type) {
        case STMT_EXPRESSION:
            indent(renderer->output, depth);
            if (!render_expression(renderer, statement->as.expression.expression, 0)) return false;
            fputc('\n', renderer->output);
            return true;
        case STMT_VARIABLE_DECLARATION:
        case STMT_CONSTANT_DECLARATION:
            renderer->found[statement->type == STMT_VARIABLE_DECLARATION ? FOUND_VARIABLE : FOUND_CONSTANT] = true;
            indent(renderer->output, depth);
            fputs(statement->as.declaration.name, renderer->output);
            fputs(" = ", renderer->output);
            if (!render_expression(renderer, statement->as.declaration.initializer, 0)) return false;
            if (statement->type == STMT_CONSTANT_DECLARATION)
                fputs("  # convenção; Python não torna este nome imutável", renderer->output);
            fputc('\n', renderer->output);
            return true;
        case STMT_ASSIGNMENT:
            renderer->found[FOUND_ASSIGNMENT] = true;
            indent(renderer->output, depth);
            fputs(statement->as.assignment.name, renderer->output);
            fputs(" = ", renderer->output);
            if (!render_expression(renderer, statement->as.assignment.value, 0)) return false;
            fputc('\n', renderer->output);
            return true;
        case STMT_BLOCK:
            return render_block(renderer, statement, depth);
        case STMT_IF:
            renderer->found[FOUND_CONDITION] = true;
            return render_if(renderer, statement, depth, false);
        case STMT_WHILE:
            renderer->found[FOUND_LOOP] = true;
            indent(renderer->output, depth);
            fputs("while ", renderer->output);
            if (!render_expression(renderer, statement->as.while_statement.condition, 0)) return false;
            fputs(":\n", renderer->output);
            return render_block(renderer, statement->as.while_statement.body, depth + 1U);
        case STMT_FOR:
            renderer->found[FOUND_LOOP] = true;
            indent(renderer->output, depth);
            fprintf(renderer->output, "for %s in range(", statement->as.for_statement.iterator_name);
            if (!render_expression(renderer, statement->as.for_statement.start, 0)) return false;
            fputs(", (", renderer->output);
            if (!render_expression(renderer, statement->as.for_statement.end, 0)) return false;
            fputs(") + 1):\n", renderer->output);
            return render_block(renderer, statement->as.for_statement.body, depth + 1U);
        case STMT_FOR_EACH:
            renderer->found[FOUND_LOOP] = true;
            indent(renderer->output, depth);
            fprintf(renderer->output, "for %s in ", statement->as.for_each_statement.iterator_name);
            if (!render_expression(renderer, statement->as.for_each_statement.iterable, 0)) return false;
            fputs(":\n", renderer->output);
            return render_block(renderer, statement->as.for_each_statement.body, depth + 1U);
        case STMT_BREAK:
            indent(renderer->output, depth); fputs("break\n", renderer->output); return true;
        case STMT_CONTINUE:
            indent(renderer->output, depth); fputs("continue\n", renderer->output); return true;
        case STMT_FUNCTION:
            if (renderer->function_depth > 0U) return false;
            renderer->found[FOUND_FUNCTION] = true;
            if (statement->as.function.parameter_count > 0U) renderer->found[FOUND_PARAMETER] = true;
            indent(renderer->output, depth);
            fprintf(renderer->output, "def %s(", statement->as.function.name);
            for (index = 0U; index < statement->as.function.parameter_count; index++) {
                if (index > 0U) fputs(", ", renderer->output);
                fputs(statement->as.function.parameters[index], renderer->output);
            }
            fputs("):\n", renderer->output);
            renderer->function_depth++;
            if (!render_block(renderer, statement->as.function.body, depth + 1U)) {
                renderer->function_depth--;
                return false;
            }
            renderer->function_depth--;
            return true;
        case STMT_RETURN:
            renderer->found[FOUND_RETURN] = true;
            indent(renderer->output, depth);
            fputs("return", renderer->output);
            if (statement->as.return_statement.value != NULL) {
                fputc(' ', renderer->output);
                if (!render_expression(renderer, statement->as.return_statement.value, 0)) return false;
            }
            fputc('\n', renderer->output);
            return true;
        case STMT_INDEX_ASSIGNMENT:
            renderer->found[FOUND_ASSIGNMENT] = true;
            renderer->found[FOUND_INDEX] = true;
            indent(renderer->output, depth);
            if (!render_expression(renderer, statement->as.index_assignment.target, 80)) return false;
            fputc('[', renderer->output);
            if (!render_expression(renderer, statement->as.index_assignment.index, 0)) return false;
            fputs("] = ", renderer->output);
            if (!render_expression(renderer, statement->as.index_assignment.value, 0)) return false;
            fputc('\n', renderer->output);
            return true;
        case STMT_IMPORT:
            return false;
    }
    return false;
}

static void render_concepts(FILE *output, const bool found[FOUND_COUNT]) {
    static const char *names[FOUND_COUNT] = {
        "variável", "constante", "atribuição", "expressão", "condição",
        "comparação", "repetição", "função", "parâmetro", "argumento",
        "retorno", "lista", "índice", "entrada", "saída"
    };
    size_t index;
    fputs("\nConceitos encontrados:\n", output);
    for (index = 0U; index < FOUND_COUNT; index++)
        if (found[index]) fprintf(output, "- %s\n", names[index]);
}

static void copy_stream(FILE *from, FILE *to) {
    char buffer[1024];
    size_t count;
    rewind(from);
    while ((count = fread(buffer, 1U, sizeof(buffer), from)) > 0U)
        (void)fwrite(buffer, 1U, count, to);
}

static int compare_program(const Source *source, const Program *program, RuntimeIO io) {
    PythonRenderer renderer;
    size_t index;
    memset(&renderer, 0, sizeof(renderer));
    renderer.output = io.output;
    renderer.supported = true;
    fprintf(io.output, "Comparação educacional: Lume → Python\n\nLume:\n");
    (void)fwrite(source->bytes, 1U, source->length, io.output);
    if (source->length == 0U || source->bytes[source->length - 1U] != '\n') fputc('\n', io.output);
    fputs("\nPython (representação educacional):\n", io.output);
    for (index = 0U; index < program->statements.count; index++) {
        FILE *temporary = tmpfile();
        bool statement_supported;
        if (temporary == NULL) {
            fputs("Não foi possível preparar a comparação educacional.\n", io.output);
            return 1;
        }
        renderer.output = temporary;
        statement_supported = render_statement(&renderer, program->statements.data[index], 0U);
        renderer.output = io.output;
        if (statement_supported) {
            copy_stream(temporary, io.output);
        } else {
            renderer.supported = false;
            fputs("# Esta construção ainda não possui comparação educacional com Python.\n", io.output);
        }
        (void)fclose(temporary);
    }
    render_concepts(io.output, renderer.found);
    fputs("\nO que mudou:\n- palavras e delimitação de blocos podem mudar; Python usa indentação;\n- escreva/leia cumprem papéis semelhantes a print/input nos exemplos suportados.\n", io.output);
    fputs("\nO que continuou igual:\n- valores, nomes, expressões e fluxo continuam representando os mesmos conceitos fundamentais.\n", io.output);
    if (!renderer.supported)
        fputs("\nLimite: esta comparação é parcial e não inventa equivalência para construções ainda não suportadas.\n", io.output);
    fputs("\nEsta saída é uma ponte de aprendizagem, não um transpiler nem uma garantia de equivalência completa.\n", io.output);
    return 0;
}

int compare_run_file(const char *path, const char *language, RuntimeIO io) {
    Source source;
    TokenArray tokens;
    ErrorList errors;
    Program *program = NULL;
    bool ok;
    int result;
    if (language == NULL || strcmp(language, "python") != 0) {
        fprintf(io.output, "A linguagem '%s' não está disponível para comparação. Use: lume comparar arquivo.lume --com python\n",
                language == NULL ? "" : language);
        return 1;
    }
    source_init(&source);
    token_array_init(&tokens);
    error_list_init(&errors);
    ok = source_load_file(&source, path);
    if (!ok) {
        fprintf(io.output, "Não foi possível ler '%s'.\n", path);
        result = 1;
    } else {
        ok = lexer_scan(&source, &tokens, &errors);
        if (ok) ok = parser_parse_program(&tokens, &program, &errors);
        if (!ok) {
            if (errors.count > 0U) diagnostic_render(io.output, &source, &errors.data[0]);
            result = 1;
        } else {
            result = compare_program(&source, program, io);
        }
    }
    program_free(program);
    error_list_free(&errors);
    token_array_free(&tokens);
    source_free(&source);
    return result;
}
