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
com localização, fonte, categoria e caret. As regressões da v0.3.0 também cobrem
sugestões inequívocas de nomes e a apresentação dos tipos recebidos em operações
incompatíveis.

`test_lists.c` cobre literais, índices, aliases, mutação, crescimento, remoção,
listas aninhadas, formatter, funções, closures, UTF-8, igualdade e ciclos.

`test_education.c` cobre a sequência estruturada de eventos, leituras de nomes,
expressões unárias e binárias, decisões, laços, recursão, listas, renderização
numerada, resumo de execuções longas e o modo passo a passo com entrada/saída
simulada.

`test_analyzer.c` cobre símbolos não usados, parâmetros, funções referenciadas,
closures, listas, atribuições, código inalcançável, condições constantes, laços
vazios, sombreamento, sugestões conservadoras e ausência de execução pela CLI.
Também verifica inalcançabilidade direta depois de `pare` e `continue`, rejeição
de sugestões ambíguas ou curtas e uso externo de símbolos exportados.

`test_modules.c` cobre imports e exports, aliases, colisões com nativas,
privacidade, cache, estado persistente, entradas malformadas, closures, listas,
caminhos relativos e aninhados, ciclos, módulos e membros inexistentes, REPL,
analyzer e tracing educacional.

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

`test_stability.c` reúne regressões e limites de uso comum: entradas vazias e
incompletas, diagnósticos léxicos/sintáticos, erros numéricos, de tipo e índice,
listas maiores, textos longos, muitas declarações e análise de uma AST maior sem
executá-la. Também cobre a fronteira de 128 níveis do parser, a primeira entrada
recusada e o caminho de `--analisar` sem crash. Os testes de ownership executam
milhares de atribuições, blocos, chamadas, listas e textos e confirmam que nenhum
ambiente temporário fica na arena. Uma closure que escapa confirma o caso oposto:
seu ambiente precisa e continua sendo preservado.

`test_v020.c` cobre `leia` com e sem convite, validação de aridade/tipo, os
módulos `lume/aleatorio` e `lume/terminal`, intervalos aleatórios, lista vazia,
sequências ANSI, tamanho e leitura de tecla com `RuntimeIO` simulado.
`test_control_flow.c` também cobre `para ... em`, fotografia diante de mutação,
`pare`, `continue`, laços aninhados e usos inválidos fora de repetição.

`test_v030.c` cobre os payloads estruturados e a ordem real de avaliação das
expressões, mapeamento entre argumentos e parâmetros, explicações conceituais de
variáveis, constantes, condições, funções, listas e controle de laço, além da
representação limitada de textos e listas. A suíte também reúne regressões de
diagnósticos de nome e tipo, analyzer, aliases de módulos e biblioteca padrão.
Um programa de **1.000.000 de iterações** confirma que a explicação permanece em
streaming: a saída fica limitada, os eventos omitidos são contabilizados e o fim
estrutural ainda é apresentado sem armazenar o histórico completo.

`test_repl.c` também verifica diagnósticos associados a Sources antigas para
erros de nome, tipo, índice e closures, seguidos por execução válida na sessão.
Execuções repetidas de blocos e imports validam que REPL e módulos não acumulam
ambientes transitórios entre unidades. Uma função publicada antes de um erro na
mesma entrada continua válida depois da recuperação, protegendo o lifetime de sua
AST e Source.

Execute as dezoito suítes na raiz com `make test`.

Na integração contínua, o mesmo alvo é executado em Linux com GCC. Antes de uma
release Windows, `scripts/build-windows.sh` repete o build e todas as suítes
antes de copiar o executável; nenhum teste específico é omitido no pacote.

`test_cli.c` também valida o despacho contextual de `lume`, execução direta de
scripts, ajuda e `lume testar`: descoberta direta em `tests/`, ordenação
lexicográfica, ausência de testes e propagação de falhas.
