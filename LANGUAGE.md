# Especificação da linguagem Lume 0.1

Este documento define a sintaxe pretendida para o primeiro interpretador. Itens
marcados como futuros não fazem parte da implementação inicial.

## 1. Texto-fonte

- Arquivos usam UTF-8. Um BOM UTF-8, se presente no início, poderá ser ignorado.
- Palavras-chave são ASCII, minúsculas e sem acentos, para serem fáceis de
  digitar em qualquer teclado.
- A linguagem diferencia maiúsculas de minúsculas: `idade` e `Idade` são nomes
  distintos; `Variavel` não é a palavra-chave `variavel`.
- Quebra de linha encerra uma instrução quando a construção está completa.
  Ponto e vírgula (`;`) também encerra uma instrução e permite várias na mesma
  linha. Quebras de linha são ignoradas dentro de `(` e depois de operadores
  que exigem um operando.
- Blocos são delimitados por `{` e `}`; indentação é apenas visual.

## 2. Palavras-chave da 0.1

| Palavra | Papel |
| --- | --- |
| `variavel` | declara uma variável mutável |
| `constante` | declara um vínculo imutável |
| `se` | inicia uma condição |
| `senao` | ramo alternativo, inclusive `senao se` |
| `enquanto` | laço condicional |
| `para`, `de`, `ate` | laço inclusivo por intervalo |
| `funcao` | declara uma função |
| `retorne` | retorna de uma função |
| `importe` | carrega um módulo local no nível do arquivo |
| `exporte` | torna uma declaração global acessível a outros módulos |
| `verdadeiro`, `falso` | literais booleanos |
| `nulo` | literal nulo |
| `e`, `ou`, `nao` | operadores lógicos |

`escreva`, `leia`, `texto`, `inteiro`, `decimal`, `tipo`, `tamanho`, `adicione`
e `remova` são funções nativas,
não palavras-chave. Assim, chamadas
seguem uma única regra sintática e a biblioteca nativa poderá crescer sem
alterar a gramática. Nomes nativos predefinidos não podem ser redeclarados no
escopo global na 0.1.

Palavras reservadas para versões futuras não serão bloqueadas antes de ganhar
semântica. `em`, `estrutura` e `modulo` ainda podem ser identificadores.
Na Fase 10, `importe` e `exporte` tornaram-se palavras-chave; esta é uma quebra
explícita de compatibilidade aceita durante o desenvolvimento da versão 0.1.

## 3. Identificadores

Na 0.1, um identificador corresponde a:

```text
[A-Za-z_][A-Za-z0-9_]*
```

Caracteres acentuados são aceitos em strings e comentários, mas não em nomes.
Isso evita regras Unicode incompletas no primeiro lexer. Uma revisão futura
poderá adotar identificadores Unicode com normalização definida.

## 4. Comentários

Comentários de linha começam com `//` e terminam antes da quebra de linha:

```lume
variavel total = 10 // comentário em UTF-8
```

Não existem comentários de bloco na 0.1. A escolha elimina aninhamento e erros
por terminador ausente; documentação multilinha poderá usar várias linhas `//`.

## 5. Literais e valores

### Inteiros

Inteiros são decimais, sem sinal no token (o sinal é operador unário), e serão
representados por `int64_t`. Exemplos: `0`, `18`, `9223372036854775807`.
Overflow na leitura ou em operações deve produzir erro de execução; não pode
ocorrer wraparound silencioso. Bases hexadecimal/binária e separadores ficam
para versões futuras.

### Decimais

Decimais exigem dígitos dos dois lados do ponto: `3.14`, `0.5`. Formas `.5`,
`1.`, expoentes e vírgula decimal não fazem parte da 0.1. A representação será
`double` (IEEE 754 quando oferecido pela plataforma). Divisão por zero é erro,
inclusive para decimais, para manter comportamento pedagógico consistente.

### Strings

Strings usam aspas duplas e armazenam bytes UTF-8. Escapes iniciais:

| Escape | Valor |
| --- | --- |
| `\\n` | nova linha |
| `\\r` | retorno de carro |
| `\\t` | tabulação |
| `\\"` | aspas |
| `\\\\` | barra invertida |

Escape desconhecido e string não terminada são erros léxicos. Strings não
atravessam uma quebra de linha física na 0.1. Índices, interpolação e escapes
Unicode explícitos são futuros.

### Booleano e nulo

`verdadeiro`, `falso` e `nulo` são valores distintos. Na versão 0.1, operadores
lógicos exigem booleanos explicitamente; não existe truthiness implícita.

### Listas

Listas são heterogêneas, mutáveis e usam semântica de referência. `[]` cria
uma lista vazia e `[1, "Ana", verdadeiro]` cria três elementos. Vírgula final
não é aceita. Listas podem conter outras listas e funções.

Copiar uma lista copia sua referência: se `b = a`, alterar `b[0]` também altera
o objeto observado por `a`. Duas listas são iguais somente se são o mesmo
objeto; `[1] == [1]` é falso. Mutações que criariam ciclos diretos ou indiretos
são recusadas, pois a implementação usa contagem de referências sem GC.

## 6. Operadores e precedência

Da menor para a maior precedência:

| Nível | Operadores | Associatividade |
| --- | --- | --- |
| 1 | `ou` | esquerda |
| 2 | `e` | esquerda |
| 3 | `==`, `!=` | esquerda |
| 4 | `<`, `<=`, `>`, `>=` | esquerda |
| 5 | `+`, `-` | esquerda |
| 6 | `*`, `/`, `%` | esquerda |
| 7 | `nao`, `-`, `+` unários | direita |
| 8 | chamada `(...)`, índice `[...]`, membro `.` | esquerda |
| 9 | literal, identificador, agrupamento | — |

`+` soma números ou concatena quando ambos os operandos são texto. Não haverá
conversão implícita texto/número; para concatenação mista use `texto(valor)`.
Operadores aritméticos aceitam números; comparação
de ordem aceita apenas números. Inteiro com decimal promove o inteiro para decimal.
`==` e `!=` aceitam qualquer par; tipos diferentes são desiguais, exceto a
comparação numérica inteiro/decimal pelo valor.

`e` e `ou` exigem booleanos, fazem curto-circuito e retornam booleano. Comparações encadeadas como
`1 < x < 10` não têm significado especial e devem ser escritas
`1 < x e x < 10`.

### Semântica numérica

- `+`, `-` e `*` entre dois inteiros retornam inteiro e detectam overflow.
- Se um operando for decimal, o outro é promovido e o resultado é decimal.
- `/` aceita números e sempre retorna decimal: `5 / 2` resulta em `2.5`.
- `%` aceita somente inteiros.
- Divisão e resto por zero são erros de execução.
- `+` e `-` unários aceitam números; `nao` aceita somente booleano.

Igualdade compara textos pelo conteúdo, booleanos pelo valor e `nulo` apenas
com `nulo`. Inteiro e decimal podem ser iguais pelo valor numérico, como
`10 == 10.0`. Tipos incompatíveis são simplesmente diferentes.

Decimais são convertidos sem alterar o locale global. A sintaxe sempre usa
ponto. Valores extremamente grandes são rejeitados; precisão segue as
limitações do tipo C `double`.

## 7. Declarações, atribuição e escopo

Variáveis exigem declaração explícita e inicializador:

```lume
variavel nome = "Ana"
constante PI = 3.14159
nome = "Bia"
```

Atribuir a um nome inexistente ou a uma constante é erro. Não há hoisting de
variáveis. Cada bloco cria escopo léxico; uma função cria um escopo cujo pai é o
ambiente em que foi declarada. Um nome pode sombrear outro de escopo externo,
mas redeclará-lo no mesmo escopo é erro. Parâmetros pertencem ao escopo da
função e não podem ser duplicados.

O inicializador é obrigatório e é avaliado antes de o binding ser criado. Um
conflito de redeclaração ou nome nativo global é detectado antes da avaliação do
inicializador. Bindings nativos são reservados somente no escopo
global; um bloco local pode sombrear esses nomes.

Um bloco isolado cria um ambiente filho. Leituras e atribuições procuram do
escopo atual em direção aos pais; uma atribuição modifica o primeiro binding
encontrado. Ao sair do bloco seus bindings deixam de ser visíveis, mas o runtime
pode conservar o frame até o fim da execução quando necessário para closures.

Em programas, a quebra após um operador continua a expressão. Uma quebra depois
de expressão completa encerra o statement, evitando ambiguidade com um operador
unário no começo da linha seguinte. Dentro de parênteses, quebras são ignoradas.

Funções declaradas em um bloco ficam disponíveis naquele bloco desde o início
dele, permitindo recursão e referência mútua entre funções do mesmo bloco. Esse
hoisting é exclusivo de declarações de função.

## 8. Controle de fluxo

Condições de `se` e `enquanto` precisam produzir booleano. Números, textos e
`nulo` não são convertidos implicitamente para verdadeiro ou falso. Os corpos
são sempre blocos e, portanto, criam seus próprios escopos.

`senao se` é composição de `senao` com outro `se`:

```lume
se nota >= 7 {
    escreva("aprovado")
} senao se nota >= 5 {
    escreva("recuperacao")
} senao {
    escreva("reprovado")
}
```

`enquanto` testa antes de cada iteração. O `para` inicial percorre um intervalo
inclusivo, com passo `+1`; limites são avaliados uma vez e precisam ser inteiros:

```lume
para i de 1 ate 5 {
    escreva(i)
}
```

Se o início for maior que o fim, o corpo executa zero vezes. `i` é uma variável
imutável local ao laço. `pare` e `continue` não fazem parte da 0.1.

Os dois limites são avaliados uma única vez e precisam ser inteiros. O laço
possui um ambiente próprio para o iterador, que pode sombrear um nome externo;
o bloco do corpo é filho desse ambiente. O runtime avança o binding imutável por
uma operação interna que não está disponível ao código Lume. Após executar o
limite final, o laço termina antes de incrementar, evitando overflow.

## 9. Funções

Funções possuem aridade fixa e são valores chamáveis internos, embora a 0.1
não prometa sintaxe de função anônima:

```lume
funcao somar(a, b) {
    retorne a + b
}
```

Argumentos são avaliados da esquerda para a direita. `retorne` sem expressão
retorna `nulo`; alcançar o fim também retorna `nulo`. Usar `retorne` fora de uma
função é erro de sintaxe. Funções capturam o ambiente léxico (closures), decisão
necessária para manter semântica consistente quando funções são aninhadas.

### Conversões e tipos

`texto(valor)` usa a mesma representação de `escreva` e do REPL; funções são
representadas como `<funcao nome>`. `inteiro` aceita inteiro, decimal sem parte
fracionária ou texto integral completo, detectando overflow. `decimal` aceita
números e texto decimal completo. `tipo(valor)` retorna o nome do tipo. Essas
funções não introduzem conversão booleana nem truthiness.

### Operações de listas

`lista[indice]` acessa em O(1), com índices inteiros iniciados em zero. Índices
negativos e fora da faixa são erros. `lista[indice] = valor` altera o objeto
compartilhado e avalia alvo, índice e valor uma vez cada.

`tamanho(lista)` retorna elementos; para texto, conta pontos de código UTF-8,
não bytes nem grapheme clusters. `adicione(lista, valor)` acrescenta e retorna
`nulo`. `remova(lista, indice)` remove e retorna o valor. `texto(lista)` usa
strings entre aspas e `tipo(lista)` retorna `"lista"`.

## 10. Gramática (EBNF)

### Módulos

O prefixo `lume/` é reservado à biblioteca padrão fornecida pelo runtime. Esses
módulos têm precedência absoluta sobre arquivos, caminhos de projeto e
dependências; não possuem manifesto nem entrada no lockfile.

`importe "util/texto"` carrega `util/texto.lume` relativamente ao arquivo que
contém o import. A extensão pode ser escrita ou omitida, e `.` e `..` são
normalizados. Imports só aparecem no nível do módulo. Cada caminho normalizado é
carregado e executado uma vez por sessão.

O binding imutável usa o último componente do caminho. Apenas variáveis,
constantes e funções globais marcadas com `exporte` são públicas. O acesso usa
`modulo.membro`; estado privado e closures permanecem no ambiente próprio do
módulo. Não existem alias ou reexports nesta versão. `tipo(modulo)` retorna
`"modulo"` e `texto(modulo)` produz `<modulo nome>`.

`NEWLINE` e `;` formam terminadores conforme as regras da seção 1. O parser
aceita terminadores extras entre instruções.

```ebnf
programa       = terminadores, { declaracao, terminadores }, EOF ;
declaracao     = importacao | declaracao_exportada | decl_variavel
               | decl_constante | decl_funcao | instrucao ;
importacao     = "importe", STRING ;
declaracao_exportada = "exporte", ( decl_variavel | decl_constante | decl_funcao ) ;
decl_variavel  = "variavel", IDENT, "=", expressao ;
decl_constante = "constante", IDENT, "=", expressao ;
decl_funcao    = "funcao", IDENT, "(", [ parametros ], ")", bloco ;
parametros     = IDENT, { ",", IDENT } ;

instrucao      = instr_se | instr_enquanto | instr_para | instr_retorne
               | bloco | atribuicao_ou_expressao ;
instr_se       = "se", expressao, bloco,
                 [ "senao", ( instr_se | bloco ) ] ;
instr_enquanto = "enquanto", expressao, bloco ;
instr_para     = "para", IDENT, "de", expressao, "ate", expressao, bloco ;
instr_retorne  = "retorne", [ expressao ] ;
bloco          = "{", terminadores,
                 { declaracao, terminadores }, "}" ;
atribuicao_ou_expressao = [ IDENT, "=" ], expressao ;

expressao      = ou ;
ou             = e, { "ou", e } ;
e              = igualdade, { "e", igualdade } ;
igualdade      = comparacao, { ( "==" | "!=" ), comparacao } ;
comparacao     = termo, { ( ">" | ">=" | "<" | "<=" ), termo } ;
termo          = fator, { ( "+" | "-" ), fator } ;
fator          = unario, { ( "*" | "/" | "%" ), unario } ;
unario         = ( "nao" | "-" | "+" ), unario | postfix ;
postfix        = primario, { "(", [ argumentos ], ")" | "[", expressao, "]"
                           | ".", IDENT } ;
argumentos     = expressao, { ",", expressao } ;
primario       = INTEIRO | DECIMAL | STRING | "verdadeiro" | "falso"
               | "nulo" | IDENT | "(", expressao, ")" | lista ;
lista          = "[", [ expressao, { ",", expressao } ], "]" ;
terminadores   = { NEWLINE | ";" } ;
```

Na regra `atribuicao_ou_expressao`, o prefixo opcional representa apenas a forma
`IDENT = expressao`; o parser fará lookahead para distingui-la de uma expressão.

## 11. Diagnósticos

Tokens e nós relevantes carregam arquivo, linha e coluna (contados a partir de
1) por meio de um `SourceSpan`. Erros devem incluir categoria, localização,
trecho e indicação visual quando a fonte estiver disponível. O núcleo separa o
dado estruturado do erro de sua apresentação, permitindo um modo educacional
mais detalhado sem mudar lexer ou parser.

Linhas e colunas começam em 1. Na versão 0.1, colunas contam bytes desde o
início da linha, não caracteres Unicode visuais.

O lexer emite um token `NEWLINE` para cada byte `\n`; espaço, tab e `\r` são
ignorados. A continuação de expressões entre linhas será decisão do parser.
Ele para no primeiro erro e não acrescenta `EOF` à sequência inválida.

## 12. Regras léxicas concretas

- Tokens estruturais: `(`, `)`, `{`, `}`, `[`, `]`, `.`, `,`, `:`, `;`, `NEWLINE` e `EOF`.
- Operadores: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `!=`, `<`, `<=`, `>` e `>=`.
- Literais e nomes: `INTEGER`, `DECIMAL`, `STRING` e `IDENTIFIER`.
- Todas as funções nativas continuam sendo `IDENTIFIER`.
- `123abc` é erro léxico, não dois tokens.
- `1.`, `.5` e `1..2` são erros; decimais exigem dígitos dos dois lados.
- `!` isolado é erro; a negação é escrita `nao`.
- `:` é reconhecido e reservado como pontuação, embora ainda não apareça em uma
  produção gramatical da 0.1.
