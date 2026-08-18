# Testes

`test_lexer.c` é uma suíte unitária C sem framework externo. Ela cobre tokens,
palavras-chave, funções nativas como identificadores, números, strings, escapes,
comentários, localização, erros, BOM, arquivo completo e fronteiras de EOF.

`test_expression.c` cobre formato da AST, precedência, associatividade,
aritmética, promoção numérica, booleanos, strings, short-circuit e erros.

`test_program.c` cobre declarações, constantes, atribuição, terminadores,
integração de expressões, escopo, sombreamento, bindings externos e erros.

`test_control_flow.c` cobre condições booleanas, ramos, loops, limites
inclusivos, escopo e imutabilidade de iteradores, aninhamento e erros.

`test_functions.c` cobre AST e execução de funções, aridade, chamadas
encadeadas, retorno, recursão, hoisting mútuo, closures e funções nativas.

`test_cli.c`, `test_repl.c` e `test_diagnostics.c` cobrem argumentos e exit
codes, persistência, multilinha, conversões, recuperação após erro e o renderer
com localização, fonte, categoria e caret.

`test_lists.c` cobre literais, índices, aliases, mutação, crescimento, remoção,
listas aninhadas, formatter, funções, closures, UTF-8, igualdade e ciclos.

`test_education.c` cobre a sequência estruturada de eventos, decisões, laços,
recursão, listas, renderização numerada, resumo de execuções longas e o modo
passo a passo com entrada/saída simulada.

`test_analyzer.c` cobre símbolos não usados, parâmetros, funções referenciadas,
closures, listas, atribuições, código inalcançável, condições constantes, laços
vazios, sombreamento, sugestões conservadoras e ausência de execução pela CLI.

`test_modules.c` cobre imports e exports, privacidade, cache, estado persistente,
closures, listas, caminhos relativos e aninhados, ciclos, módulos e membros
inexistentes, REPL, analyzer e tracing educacional.

`test_project.c` cobre manifesto válido, campos obrigatórios, duplicatas,
chaves desconhecidas e sugestões, versões, criação sem sobrescrita, execução,
verificação sem efeitos, módulos internos, raízes adicionais, caminhos Windows
e ciclos.

`test_dependencies.c` cobre dependência direta, submódulo, transitividade,
isolamento, nome incompatível, caminho/manifesto ausente, ciclo entre projetos,
execução, verificação sem efeitos e lockfile transitivo determinístico.

`test_stdlib.c` cobre módulos nativos, matemática, texto e UTF-8, arquivos
temporários, tempo, erros, analyzer, projetos, REPL, namespace reservado,
compatibilidade de nativas e ausência no lockfile.

Execute as quinze suítes na raiz com `make test`.

Na integração contínua, o mesmo alvo é executado em Linux com GCC. Antes de uma
release Windows, `scripts/build-windows.sh` repete o build e todas as suítes
antes de copiar o executável; nenhum teste específico é omitido no pacote.

`test_cli.c` também valida o despacho contextual de `lume`, execução direta de
scripts, ajuda e `lume testar`: descoberta direta em `tests/`, ordenação
lexicográfica, ausência de testes e propagação de falhas.
