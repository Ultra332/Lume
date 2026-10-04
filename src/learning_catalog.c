#include "learning_catalog.h"

#include <string.h>

#define CONCEPT(slug_value, title_value, definition_value, example_value, explanation_value, uses_value, related_value, lesson_value) \
    {slug_value, title_value, definition_value, example_value, explanation_value, uses_value, related_value, lesson_value}

static const LearningConcept CONCEPTS[] = {
    CONCEPT("variavel", "Variável", "Uma variável guarda um valor usando um nome e pode receber outro valor depois.",
        "variavel idade = 18\nidade = 19", "O nome é 'idade'. Primeiro ele guarda 18 e depois passa a guardar 19.",
        "Para lembrar informações que mudam durante o programa.", "valor, tipo, atribuição, escopo", "02 — Variáveis"),
    CONCEPT("constante", "Constante", "Uma constante associa um nome a um valor que não pode ser substituído.",
        "constante DIAS_DA_SEMANA = 7", "O nome ajuda a explicar o valor e protege essa associação contra atribuições posteriores.",
        "Para valores que devem permanecer iguais durante aquela execução.", "variável, valor, atribuição, escopo", "02 — Variáveis"),
    CONCEPT("atribuicao", "Atribuição", "Uma atribuição coloca um novo valor em uma variável já existente.",
        "variavel pontos = 0\npontos = pontos + 1", "A segunda linha calcula um valor e o guarda novamente no nome 'pontos'.",
        "Para atualizar contadores, estados e resultados parciais.", "variável, expressão, valor", "02 — Variáveis"),
    CONCEPT("valor", "Valor", "Um valor é uma informação que o programa pode guardar, comparar ou transformar.",
        "42\n\"Olá\"\nverdadeiro\n[1, 2, 3]", "Os exemplos são valores de tipos diferentes: número, texto, booleano e lista.",
        "Valores aparecem em expressões, variáveis, argumentos e retornos.", "tipo, expressão, variável", "02 — Variáveis"),
    CONCEPT("tipo", "Tipo", "O tipo descreve a forma de um valor e quais operações fazem sentido para ele.",
        "variavel nome = \"Ana\"\nvariavel idade = 18", "'nome' guarda texto; 'idade' guarda um número inteiro.",
        "Para entender por que algumas operações aceitam certos valores e outras não.", "valor, operador, lista", "02 — Variáveis"),
    CONCEPT("operador", "Operador", "Um operador combina ou transforma valores para produzir outro valor.",
        "total + 1\nidade >= 18\nnao terminou", "Os operadores realizam soma, comparação e negação.",
        "Em cálculos, comparações e condições.", "expressão, condição, tipo", "04 — Decisões com se"),
    CONCEPT("expressao", "Expressão", "Uma expressão é uma parte do código que produz um valor.",
        "preco * quantidade\nidade >= 18", "Cada linha combina nomes, valores e operadores e produz um resultado.",
        "Em atribuições, condições, argumentos e retornos.", "valor, operador, atribuição", "02 — Variáveis"),
    CONCEPT("condicao", "Condição", "Uma condição produz verdadeiro ou falso e permite ao programa escolher um caminho.",
        "se idade >= 18 {\n    escreva(\"Maior\")\n}", "A comparação decide se o bloco será executado.",
        "Em decisões e repetições controladas.", "booleano, comparação, operador, bloco", "04 — Decisões com se"),
    CONCEPT("bloco", "Bloco", "Um bloco reúne instruções que pertencem à mesma parte do programa.",
        "se pronto {\n    escreva(\"Começar\")\n}", "As chaves delimitam as instruções controladas pela condição.",
        "Em condições, laços e funções.", "condição, repetição, função, escopo", "04 — Decisões com se"),
    CONCEPT("repeticao", "Repetição", "Uma repetição executa um bloco várias vezes segundo uma regra.",
        "para numero em numeros {\n    escreva(numero)\n}", "O bloco é executado uma vez para cada elemento da lista.",
        "Para percorrer coleções e repetir tarefas.", "condição, bloco, lista, iterador", "05 — Repetições"),
    CONCEPT("funcao", "Função", "Uma função reúne uma tarefa sob um nome para que ela possa ser utilizada quando necessário.",
        "funcao dobro(numero) {\n    retorne numero * 2\n}\nescreva(dobro(5))", "A função recebe um número, calcula o dobro e devolve o resultado.",
        "Para organizar, reutilizar e testar partes do programa.", "parâmetro, argumento, retorno, escopo", "06 — Funções"),
    CONCEPT("parametro", "Parâmetro", "Um parâmetro é o nome que uma função usa para receber um valor.",
        "funcao dobro(numero) {\n    retorne numero * 2\n}", "'numero' é o parâmetro disponível dentro da função.",
        "Para escrever funções que trabalham com valores diferentes.", "função, argumento, retorno, escopo", "06 — Funções"),
    CONCEPT("argumento", "Argumento", "Um argumento é o valor entregue a uma função no momento da chamada.",
        "dobro(5)", "O valor 5 é o argumento recebido pelo parâmetro da função.",
        "Para fornecer dados a uma função.", "função, parâmetro, chamada", "06 — Funções"),
    CONCEPT("retorno", "Retorno", "Um retorno encerra a função e entrega um valor para quem a chamou.",
        "funcao dobro(numero) {\n    retorne numero * 2\n}", "O resultado da expressão volta ao ponto em que a função foi chamada.",
        "Para produzir resultados e encerrar antecipadamente uma função.", "função, expressão, argumento", "06 — Funções"),
    CONCEPT("lista", "Lista", "Uma lista guarda vários valores em uma ordem definida.",
        "variavel nomes = [\"Ana\", \"Bia\"]", "A variável guarda uma lista com dois elementos de texto.",
        "Para agrupar, acessar e percorrer dados relacionados.", "índice, elemento, repetição, tipo", "07 — Listas"),
    CONCEPT("indice", "Índice", "Um índice indica a posição de um elemento em uma lista; a primeira posição é zero.",
        "variavel nomes = [\"Ana\", \"Bia\"]\nescreva(nomes[0])", "O índice 0 acessa o primeiro elemento, 'Ana'.",
        "Para consultar ou substituir uma posição específica.", "lista, elemento, valor", "07 — Listas"),
    CONCEPT("escopo", "Escopo", "O escopo é a região do programa onde um nome está disponível.",
        "variavel fora = 1\n{\n    variavel dentro = 2\n    escreva(fora)\n}", "O bloco acessa 'fora', mas 'dentro' deixa de existir ao terminar o bloco.",
        "Para organizar nomes e evitar que detalhes locais escapem.", "bloco, variável, função, módulo", "06 — Funções"),
    CONCEPT("modulo", "Módulo", "Um módulo organiza funcionalidades em outro arquivo ou na biblioteca padrão.",
        "importe \"lume/matematica\" como matematica", "O programa passa a acessar os recursos públicos do módulo pelo nome 'matematica'.",
        "Para dividir programas e reutilizar funcionalidades.", "import, escopo, função", "Material sobre módulos"),
    CONCEPT("import", "Import", "Um import torna disponível no arquivo atual um módulo definido em outro lugar.",
        "importe \"util\" como util", "A instrução localiza o módulo e cria o nome 'util' para acessar seus exports.",
        "Para usar módulos locais, dependências e a biblioteca padrão.", "módulo, escopo, export", "Material sobre módulos")
};

static const TransitionTopic PYTHON_TOPICS[] = {
    {"primeiro-programa", "Primeiro programa", "escreva(\"Olá\")", "print(\"Olá\")",
     "A função de saída se chama print em Python.", "Nos dois casos uma chamada mostra texto para a pessoa."},
    {"variaveis", "Variáveis", "variavel idade = 18", "idade = 18",
     "Python não usa a palavra 'variavel' nessa atribuição.", "O nome 'idade' continua associado ao valor 18."},
    {"entrada-saida", "Entrada e saída", "variavel nome = leia(\"Nome: \")\nescreva(nome)", "nome = input(\"Nome: \")\nprint(nome)",
     "As funções se chamam input e print; Python também omite 'variavel'.", "A entrada retorna texto e a saída mostra esse texto."},
    {"condicoes", "Condições", "se idade >= 18 {\n    escreva(\"Maior\")\n} senao {\n    escreva(\"Menor\")\n}", "if idade >= 18:\n    print(\"Maior\")\nelse:\n    print(\"Menor\")",
     "Python escreve if/else e delimita blocos pela indentação, sem chaves.", "A comparação e a escolha entre dois caminhos permanecem."},
    {"repeticoes", "Repetições", "para nome em nomes {\n    escreva(nome)\n}", "for nome in nomes:\n    print(nome)",
     "Python usa for/in e indentação para o corpo.", "Cada elemento da lista ainda é processado uma vez."},
    {"funcoes", "Funções", "funcao dobro(numero) {\n    retorne numero * 2\n}", "def dobro(numero):\n    return numero * 2",
     "Python escreve def/return e usa indentação.", "Função, parâmetro, cálculo e retorno conservam seus papéis."},
    {"listas", "Listas", "variavel nomes = [\"Ana\", \"Bia\"]\nescreva(nomes[0])", "nomes = [\"Ana\", \"Bia\"]\nprint(nomes[0])",
     "A declaração e a saída mudam; o literal e o índice têm forma semelhante.", "A lista mantém ordem e o primeiro índice continua sendo zero."},
    {"modulos", "Módulos básicos", "importe \"util\" como util", "import util",
     "A resolução, os exports e os pacotes seguem regras diferentes nas duas linguagens.", "Ambas permitem organizar funcionalidades fora do arquivo atual; os sistemas não são equivalentes internamente."}
};

static const TransitionLanguage LANGUAGES[] = {
    {"python", "Python",
     "Python é a primeira ponte oficial da Lume. O objetivo não é traduzir toda a linguagem, mas reconhecer conceitos já aprendidos em uma nova sintaxe.",
     PYTHON_TOPICS, sizeof(PYTHON_TOPICS) / sizeof(PYTHON_TOPICS[0])}
};

size_t learning_concept_count(void) { return sizeof(CONCEPTS) / sizeof(CONCEPTS[0]); }
const LearningConcept *learning_concept_at(size_t index) {
    return index < learning_concept_count() ? &CONCEPTS[index] : NULL;
}
const LearningConcept *learning_concept_find(const char *slug) {
    size_t index;
    if (slug == NULL) return NULL;
    if (strcmp(slug, "variável") == 0) slug = "variavel";
    else if (strcmp(slug, "atribuição") == 0) slug = "atribuicao";
    else if (strcmp(slug, "expressão") == 0) slug = "expressao";
    else if (strcmp(slug, "condição") == 0) slug = "condicao";
    else if (strcmp(slug, "repetição") == 0 || strcmp(slug, "laço") == 0 || strcmp(slug, "laco") == 0) slug = "repeticao";
    else if (strcmp(slug, "função") == 0) slug = "funcao";
    else if (strcmp(slug, "parâmetro") == 0) slug = "parametro";
    else if (strcmp(slug, "índice") == 0) slug = "indice";
    else if (strcmp(slug, "módulo") == 0) slug = "modulo";
    for (index = 0U; index < learning_concept_count(); index++)
        if (strcmp(CONCEPTS[index].slug, slug) == 0) return &CONCEPTS[index];
    return NULL;
}
size_t transition_language_count(void) { return sizeof(LANGUAGES) / sizeof(LANGUAGES[0]); }
const TransitionLanguage *transition_language_at(size_t index) {
    return index < transition_language_count() ? &LANGUAGES[index] : NULL;
}
const TransitionLanguage *transition_language_find(const char *slug) {
    size_t index;
    if (slug == NULL) return NULL;
    for (index = 0U; index < transition_language_count(); index++)
        if (strcmp(LANGUAGES[index].slug, slug) == 0) return &LANGUAGES[index];
    return NULL;
}
