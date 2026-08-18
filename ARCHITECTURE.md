# Arquitetura da Lume

## 1. Fluxo planejado

```text
arquivo UTF-8
    -> Source
    -> Lexer
    -> TokenArray
    -> Parser
    -> Program (Statements + Expressions)
       |-> Interpreter -> Environment -> Value / saída
       |-> Educational Trace
       `-> Static Analyzer -> diagnósticos e métricas
```

Lexer, parser e interpretador são fronteiras independentes. O parser nunca
executa código, e o interpretador não depende dos detalhes do lexer. Uma futura
rota `AST -> bytecode compiler -> VM` poderá coexistir com o interpretador de
AST.

Na Fase 6, a mesma linguagem ganhou CLI definitiva, REPL persistente,
diagnósticos visuais, conversões e tracing educacional.

## 2. Módulos

- `main`: ponto de entrada mínimo que delega para `cli`.
- `cli`: argumentos, exit codes e despacho dos comandos.
- `repl`: prompts, comandos e acumulação de entrada multilinha.
- `session`: ambiente persistente e ownership das unidades interativas.
- `diagnostic`: apresentação terminal de erros estruturados, fonte e caret.
- `trace`: eventos educacionais estruturados e callback opcional.
- `education`: renderer numerado, inspeção dos escopos ativos, pilha lógica e
  controle do modo passo a passo sobre `RuntimeIO` injetável.
- `analyzer`: travessia estática da AST, escopos e símbolos próprios,
  diagnósticos educacionais com severidade e resumo estrutural.
- `module`: resolução portátil de caminhos locais, `LumeModule`, registry/cache,
  exports, detecção de ciclos e validação multi-arquivo sem execução.
- `project`: parser do manifesto, validação, caminhos e criação segura da
  estrutura inicial de um projeto.
- `dependency`: grafo de projetos locais, isolamento, ciclos e lockfile.
- `lume_stdlib`: catálogo e implementação dos módulos nativos `lume/*`.
- `source`: possui bytes, comprimento explícito e nome lógico.
- `token`: tipos, intervalos e vetor dinâmico de tokens.
- `lexer`: percorre `Source` uma vez e produz tokens ou o primeiro erro.
- `ast`: expressões, statements (incluindo `if`, `while` e `for`) e `Program`,
  com destruição recursiva.
- `parser`: mantém APIs separadas para expressão isolada e programa.
- `value`: valores escalares, chamáveis e referências para objetos compostos.
- `list`: lista mutável, armazenamento dinâmico e contagem de referências.
- `callable`: objeto chamável referenciado por valores, para funções Lume e nativas.
- `runtime_io`: canais de entrada e saída injetáveis para CLI e testes.
- `environment`: tabela hash de bindings, encadeamento e arena de escopos capturados.
- `interpreter`: avalia expressões e executa statements, blocos e controle de
  fluxo.
- `error`: diagnóstico estruturado, localização e apresentação amigável.
- `memory`: alocação verificada e crescimento seguro de arrays.

## 3. Decisões arquiteturais

### Recursive Descent e precedência por níveis

Statements serão analisados por recursive descent. Expressões usarão funções de
precedência (uma por nível da EBNF), uma forma simples e didática de Pratt/precedence
climbing. A tabela poderá migrar para um Pratt parser completo se operadores
extensíveis justificarem isso. A AST permanece igual em ambos os casos.

### Statements e expressions

Expressions produzem valores; statements controlam execução e escopo. Separar
essas categorias torna `retorne`, declarações e blocos semanticamente claros e
permite compilar a mesma AST para bytecode no futuro.

### Fronteiras da Fase 2

```text
TokenArray -> Parser -> Expr -> Evaluator -> Value
```

O parser é o único componente que traduz `TokenType` para os enums semânticos
`UnaryOperator` e `BinaryOperator`. Assim, a AST não depende do lexer. O
evaluator conhece apenas AST, valores e diagnósticos.

### Localização da fonte

`SourceSpan` guarda localizações inicial e final com offset, linha e coluna.
Tokens guardam a referência à fonte e referenciam fatias por offsets. O objeto `Source` deve viver até
o fim do uso dos tokens e diagnósticos. A coluna é inicialmente
medida em bytes UTF-8; a camada de apresentação calculará posição visual quando
necessário. Isso evita o lexer fingir suporte Unicode completo.

### Valores

`Value` será uma união discriminada: nulo, booleano, `int64_t`, `double`, texto e,
quando funções forem implementadas, objeto chamável. Strings de runtime serão
objetos alocados com comprimento explícito; nunca dependerão apenas de `strlen`.
Na etapa inicial, cópia/ownership explícito será preferido a um GC.

Cada nó AST possui seus filhos. Um literal textual pertence ao nó, e
`expr_free` libera recursivamente toda a árvore. Avaliar um literal copia seu
valor para que o resultado sobreviva independentemente da AST; o chamador
libera o resultado com `value_free`.

`Program` possui seu vetor de `Stmt`; cada statement possui nomes copiados,
expressões e blocos filhos. `program_free` percorre e libera toda a árvore. Como
identificadores e textos literais são copiados durante parsing, um `Program`
concluído não depende mais do `TokenArray` ou da `Source`.

Essa AST poderá alimentar análise semântica, compilador de bytecode e tooling
educacional sem reutilizar detalhes internos do parser.

### Listas e objetos compostos

`VALUE_LIST` guarda um ponteiro para `LumeList`, que contém contador de
referências, vetor de `Value`, tamanho e capacidade. `value_copy` retém e
`value_free` libera a referência. Assim bindings, parâmetros, retornos,
closures e o REPL compartilham mutações sem cópia profunda.

Acesso é O(1), append cresce de forma amortizada com os helpers seguros e
remoção é O(n). Remover transfere ownership do slot para o resultado; substituir
copia primeiro e só então libera o valor anterior.

Contagem de referências não coleta ciclos. `list` pesquisa referências
aninhadas antes de append ou substituição e recusa a operação se ela criaria
ciclo direto ou indireto. Isso evita vazamentos cíclicos sem introduzir GC.

Chamadas e índices formam uma única cadeia postfix no parser, permitindo
`funcoes[0](10)`, `criar()[0]` e `matriz[1][0]`.

### Ambientes e closures

`Environment` é uma tabela hash própria, apontando para um ambiente pai.
Bindings guardam valor, mutabilidade e localização da declaração. Na Fase 5,
ambientes criados durante a execução pertencem a uma arena do ambiente global e
vivem até o encerramento daquela execução. Closures mantêm ponteiros não
proprietários para esses frames; chamáveis usam contagem de referências.

A arena foi escolhida em vez de contagem de referências nos ambientes porque o
grafo `ambiente -> função local -> ambiente capturado` contém ciclos legítimos.
Liberar o grafo inteiro no fim evita ciclos retidos e acessos após liberação sem
introduzir um coletor de lixo. O custo deliberado nesta fase é reter frames até
o término do programa, inclusive frames transitórios de laços e chamadas.

Na Fase 3, cada `Environment` usa open addressing com sondagem linear e cresce
antes de atingir 75% de ocupação. Não há remoção individual de bindings; o
ambiente inteiro é destruído ao sair do escopo. Nomes e valores são copiados,
portanto não dependem da `Source` ou da AST. O ponteiro para o ambiente pai é
emprestado e o filho deve ser destruído primeiro.

Bindings guardam nome, valor, mutabilidade e span de declaração. Lookup e
atribuição percorrem pais; definição consulta somente o escopo atual. Conflitos
são validados antes do inicializador para evitar efeitos futuros desnecessários.

### Controle de fluxo

`STMT_IF` possui condição, ramo principal e ramo alternativo opcional; um
`senao se` é outro `STMT_IF` no ramo alternativo. `STMT_WHILE` reavalia sua
condição antes de cada iteração. Ambos exigem booleanos e delegam o escopo aos
blocos já existentes.

`STMT_FOR` avalia seus limites uma vez e cria um ambiente filho exclusivo para
o iterador imutável. Cada execução do corpo cria abaixo dele o ambiente normal
do bloco. Uma API interna do Environment substitui somente esse binding para o
avanço do runtime; atribuições Lume continuam respeitando a imutabilidade. O
incremento ocorre apenas quando o valor atual ainda é menor que o limite final.

### Retorno antecipado

O interpretador devolverá um resultado discriminado (`ok`, `erro`, `retorno`)
em vez de usar globais ou `longjmp`. Assim cada caminho libera temporários e
propaga `retorne` com ownership explícito.

### Erros

`LumeError` separa categoria, mensagem, sugestão e `SourceSpan`. Lexer, parser e
runtime produzem dados; uma camada de diagnóstico os renderiza. O modo
`--educacional` poderá anexar eventos a uma interface de tracing opcional, sem
misturar textos educativos com a semântica.

### Sessão interativa e tracing

`LumeSession` reutiliza o ambiente global e sua arena durante toda a sessão.
Unidades comuns são liberadas após executar. Uma unidade que contém declaração
de função retém AST e Source até o encerramento, pois `Callable` referencia o
corpo na AST e seus spans continuam úteis. Isso mantém closures seguras sem
reter toda entrada digitada.

`RuntimeTrace` recebe eventos estruturados de início/fim, declaração,
atribuição, decisão, laços, chamada, retorno, nativas e operações de listas. Os
campos apontados são empréstimos válidos somente durante o callback síncrono;
consumidores que retenham dados precisam copiá-los. O runtime normal usa callback
nulo. `--explicar` instala um renderer não interativo e `--passo` acrescenta
pausas dirigidas pela mesma abstração `RuntimeIO` usada nos testes.

A pilha exibida é lógica: a profundidade é mantida pelo interpretador ao entrar
e sair de funções Lume, sem examinar a pilha C. A visualização percorre somente o
ambiente ativo recebido no evento e seus pais; frames antigos retidos pela arena
para closures não aparecem. O renderer limita explicações longas a 200 eventos,
mas o programa continua sendo executado integralmente.

### Análise estática educacional

O analyzer não inclui nem chama o interpreter e não reutiliza `Environment`.
Cada `Scope` encadeado possui símbolos de variável, constante, parâmetro,
função e iterador, com span, leituras, escritas e estado do último valor. Funções
são pré-declaradas no escopo para preservar hoisting, recursão e referências
mútuas; funções aninhadas resolvem nomes nos pais, cobrindo closures.

Os diagnósticos têm severidade (`erro`, `aviso`, `informação`), código de
categoria e `SourceSpan`. A análise de fluxo é conservadora: inalcançabilidade é
propagada por `retorne` direto ou por `se` cujos dois ramos retornam;
sobrescritas são apontadas apenas em sequência linear segura. Condições são
avaliadas somente para booleanos literais, `nao` e comparações numéricas puras.
Sugestões de nome consideram apenas símbolos visíveis com até 64 bytes e
distância de edição máxima 2.

### Módulos e múltiplos arquivos

`ModuleRegistry` pertence à execução ou à sessão REPL e indexa módulos pelo
caminho lexical normalizado. Caminhos são relativos ao arquivo importador,
aceitam `/` e `\` internamente, removem segmentos `.`/`..` e acrescentam
`.lume` somente quando ausente. O registry mantém estados `LOADING`, `LOADED` e
`FAILED`; encontrar novamente um módulo `LOADING` identifica um ciclo.

Cada `LumeModule` possui caminho, nome, `Source`, AST, `Environment` global e
tabela explícita de exports. O registry possui módulos, que possuem seus
environments, sources e ASTs. `VALUE_MODULE` é uma referência não proprietária,
válida enquanto o registry da sessão existir. Na destruição, o ambiente do
importador é liberado antes do registry; dentro de cada módulo, o environment é
liberado antes da AST, preservando closures.

Imports executam a inicialização no ambiente próprio uma única vez e só criam o
binding imutável no importador depois do carregamento bem-sucedido. Acesso por
membro consulta exclusivamente a tabela de exports. O analyzer usa a mesma
resolução e cache, mas apenas carrega, parseia e valida ASTs, sem executar código.
Tracing recebe eventos de início e conclusão do import, e chamadas exportadas
continuam usando o ambiente capturado no módulo de origem.

### Projetos formais

`LumeProject` possui todas as strings do manifesto: raiz, manifesto, nome,
versão, entrada, fonte e o vetor de caminhos adicionais. `project_free` libera
recursivamente esses dados. O parser aceita somente `chave = "texto"` e a lista
de textos de `caminhos_modulos`; chaves duplicadas, desconhecidas, obrigatórias
ausentes e valores de tipo incorreto são erros de projeto.

O `ProjectLoader` resolve caminhos relativos à raiz e valida nome, versão
`X.Y.Z`, entrada e diretório fonte. A CLI cria uma única `LumeSession` para a
execução. Seu `ProjectContext` é emprestado ao `ModuleRegistry`, sem globais e
sem transferir ownership. O resolvedor consulta, em ordem, o diretório do
importador, a raiz `fonte` e `caminhos_modulos`; scripts usam contexto nulo e
mantêm a resolução relativa da Fase 10. `lume verificar` reutiliza lexer,
parser, registry e analyzer, mas não cria nem chama o interpreter.

### Dependências locais

`ProjectDependency` possui nome declarado, caminho declarado/resolvido, versão
lida e índice do projeto-alvo. `DependencyResolver` carrega recursivamente cada
`LumeProject`, valida correspondência de nomes e constrói um `DependencyGraph`
com estados não visitado, visitando e resolvido. Reentrada em um nó visitando
produz erro de dependência circular distinto de import circular.

O `ModuleRegistry` recebe o grafo por empréstimo. A origem do importador define
seu `ProjectContext`; somente módulos e dependências diretas desse projeto são
consultados. Assim dependências transitivas não vazam e módulos internos com o
mesmo caminho permanecem distintos pela identidade completa do arquivo/projeto.

`lume.lock` é saída gerada, versão 1, ordenada por nome e caminho, sem timestamp.
Registra nome, versão e caminho lexical normalizado de todos os nós transitivos.
`lume resolver` é o único comando que o escreve. O cache `.lume/` continua
reservado: não foi necessário nesta fase e não participa da correção.

### Biblioteca padrão

Imports `lume/*` são interceptados antes do filesystem e das dependências.
`lume_stdlib` cria um `LumeModule` normal, com `Environment`, exports explícitos
e `Callable` de callback genérico. Assim member lookup, `VALUE_MODULE`, cache,
REPL e tracing reutilizam a arquitetura existente. O registry possui o módulo
nativo e libera ambiente, callables e contexto de filesystem no encerramento.

Callbacks recebem argumentos, `RuntimeIO`, contexto, span e `ErrorList`, sem
conhecer o interpretador. `lume/arquivo` guarda como contexto a raiz do projeto
ou diretório do script. Validação multi-arquivo cria apenas metadata/export
tables; nenhuma função padrão é chamada pelo analyzer.

## 4. Ownership e memória

Convenção pretendida para APIs C:

- funções `*_new`, `*_copy` e `*_take` indicam aquisição de ownership;
- funções `*_free` aceitam `NULL` e liberam recursivamente o que possuem;
- parâmetros sem `take` são empréstimos válidos apenas durante a chamada;
- vetores têm `data`, `count` e `capacity`, crescem com verificação de overflow;
- falha de alocação vira erro controlado no limite da execução; não se usa
  buffer fixo para texto arbitrário;
- AST possui seus filhos; tokens possuem fatias da `Source` com lifetime
  documentado; ambiente possui bindings e valores;
- sanitizers/Valgrind serão usados quando disponíveis, sem introduzir APIs
  específicas de sistema operacional no núcleo.

`Source` possui nome e bytes; ambos deixam de ser válidos em `source_free`.
`TokenArray` possui o vetor, mas não a fonte. `ErrorList` possui seu vetor;
mensagem e sugestão são textos estáticos nesta fase.

Falhas de alocação são propagadas por retorno booleano. O lexer tenta registrar
`LUME_ERROR_MEMORY`; se isso também falhar, retorna `false` sem diagnóstico e o
chamador trata explicitamente esse caso.

## 5. Arrays dinâmicos

`memory` fornecerá crescimento validado. Cada domínio poderá ter um vetor
tipado pequeno (`TokenArray`, `ExprArray`, `StmtArray`), mas a aritmética de
capacidade e `realloc` ficará centralizada. Macros genéricas complexas serão
evitadas até haver repetição concreta suficiente.

## 6. Portabilidade e build

O núcleo usa C11 e a biblioteca padrão. CI futuro deve compilar ao menos com
GCC/Clang em Linux e MinGW/MSVC no Windows. O `Makefile` cobre ambientes Make;
um gerador adicional poderá ser incluído quando houver CI, sem substituir esse
caminho simples.

## 7. Evolução para bytecode

A fronteira é o `Program` da AST. Um compilador futuro emitirá instruções como
carregar constante, operar, saltar, chamar e retornar. Ele compartilhará
`Value`, diagnósticos e especificação sem depender do interpretador. Nenhuma VM
é implementada antes de a semântica do interpretador estar testada.

## 8. Riscos conhecidos

- UTF-8: colunas por byte e identificadores ASCII são deliberadamente limitados;
  diagnóstico visual precisa tratar caracteres multibyte.
- closures: ownership de ambientes deve estar definido antes de funções para
  impedir use-after-free e ciclos futuros.
- números: overflow inteiro deve ser verificado de forma portável, sem depender
  de comportamento indefinido.
- separadores por quebra de linha: o lexer precisa emitir `NEWLINE` e o parser
  precisa definir continuação com testes, evitando ambiguidades.
- Make no Windows não é universal; suporte MSVC/CMake pode ser necessário mais
  adiante.
- concatenação não converte números implicitamente; a biblioteca precisará de
  conversões explícitas antes de exemplos mistos como `"idade: " + idade`.

## 9. Superfície pública da v0.1.0

A versão é definida em `src/common.h` e consumida pela CLI e pelo gerador de
projetos. O `Makefile` continua sendo a interface canônica de build e testes.
`scripts/build-windows.sh` somente orquestra essa interface e deposita o
artefato ignorado em `dist/`; publicação, assinatura e criação de tags exigem
revisão humana.

A automação em `.github/workflows/ci.yml` valida GCC em Linux. Ela não substitui
os testes locais no Windows, importantes para caminhos, módulos e empacotamento.
