# Modos educacionais

Os três modos respondem perguntas diferentes e usam a mesma AST e a mesma
semântica da execução normal:

| Comando | Executa o programa? | Pergunta respondida |
|---|---:|---|
| `lume --explicar arquivo.lume` | sim | o que aconteceu e quais valores participaram |
| `lume --passo arquivo.lume` | sim, com pausas | o que foi observado em cada etapa da execução |
| `lume --analisar arquivo.lume` | não | o que pode ser apontado antes de executar |

As explicações são determinísticas: vêm da AST e dos valores observados pelo
interpretador. Elas não tentam adivinhar a intenção da pessoa, não atribuem nota
ao programa e não usam um segundo interpretador.

## Explicar a execução automaticamente

Considere `aprendizado.lume`:

```lume
variavel idade = 16
variavel minimo = 18
variavel calculo = 10 + 5 * 2

se idade >= minimo {
    escreva("Permitido")
} senao {
    escreva("Nao permitido")
}

funcao dobro(numero) {
    retorne numero * 2
}

variavel resultado = dobro(calculo)
```

Execute:

```powershell
lume --explicar aprendizado.lume
```

Trechos reais da saída são:

```text
5. Avaliando `5 * 2`: 5 * 2 = 10.
6. Avaliando `10 + 5 * 2`: 10 + 10 = 20.
7. Criando a variavel "calculo".
    Expressao: `10 + 5 * 2`
    Valor inicial: 20.
...
10. Avaliando `idade >= minimo`: 16 >= 18 = falso.
11. Verificando a condicao `idade >= minimo`. Resultado: falso. O bloco 'se' sera ignorado; o ramo 'senao', se existir, sera usado.
...
16. Chamando a funcao "dobro".
    Argumentos:
      20
17.   Entrando no escopo da funcao "dobro".
    Parametros neste escopo:
      numero = 20
...
20.   A funcao "dobro" retornou 40.
```

A numeração acompanha a ordem real dos acontecimentos. Expressões binárias e
unárias mostram operandos e resultado; operadores lógicos também indicam quando
o operando direito não foi avaliado por curto-circuito. Declarações mostram a
expressão e o valor inicial, enquanto atribuições mostram valor anterior,
expressão e novo valor. Condições explicam o ramo escolhido, laços mostram suas
iterações e funções relacionam argumentos, parâmetros e retorno.

`--explicar` realmente executa o programa. Portanto, escritas no terminal,
leitura de entrada e operações de arquivo continuam tendo seus efeitos normais.
Use `--analisar` quando precisar examinar código sem efeitos colaterais.

## Acompanhar gradualmente com `--passo`

Execute o mesmo arquivo com:

```powershell
lume --passo aprendizado.lume
```

O começo da sessão segue este formato:

```text
Passo 1
Acontecimento observado:
Inicio do programa.
[Enter=proximo, v=variaveis, p=pilha, c=continuar, q=sair] >
```

Nos passos associados ao código, o cabeçalho também mostra arquivo, linha e a
linha-fonte correspondente. Os comandos disponíveis são:

| Entrada | Ação |
|---|---|
| `Enter` | avança um acontecimento |
| `v` | mostra as variáveis visíveis, agrupadas por escopo |
| `p` | mostra a pilha lógica de chamadas |
| `c` | continua automaticamente, sem novas pausas |
| `q` | encerra a execução educacional |

A pilha apresentada é lógica e contém as funções Lume ativas; ela não expõe a
pilha C. A visualização de variáveis percorre somente os escopos visíveis no
ponto atual e não mostra endereços ou outras estruturas internas.

Enquanto você usa `Enter`, cada evento é apresentado. Depois de `c`, a execução
passa a usar a mesma política de compactação de `--explicar`.

## Analisar sem executar

O arquivo `analise.lume` abaixo possui uma saída, mas também uma variável não
utilizada:

```lume
variavel resultado = 10
escreva("Isto nao sera impresso pela analise")
```

Execute:

```powershell
lume --analisar analise.lume
```

A saída contém o diagnóstico e o resumo estrutural, não o texto passado a
`escreva`:

```text
Analisando: analise.lume

Aviso de simbolo em analise.lume:1:10

variavel resultado = 10
         ^^^^^^^^^

A variavel 'resultado' foi declarada, mas nunca utilizada.

Resumo:
  1 aviso(s)
  0 erro(s)
```

O analyzer possui escopos e símbolos próprios; ele não reutiliza valores do
runtime. Entre as verificações conservadoras estão símbolos não utilizados,
sombreamento, condições constantes e código inalcançável depois de `retorne`,
`pare` ou `continue` no mesmo fluxo direto. Avisos não se tornam erros e não
bloqueiam a execução normal.

## Saída limitada para programas grandes

Os eventos são renderizados em streaming e não ficam guardados até o fim da
execução. `--explicar` detalha até 160 eventos. Depois disso, preserva uma janela
pequena de encerramentos estruturais, contabiliza os eventos intermediários
omitidos e informa o total resumido ao terminar. O programa continua sendo
executado integralmente; somente a apresentação é compactada.

Valores também usam uma representação centralizada e limitada:

- textos mostram até 72 bytes, sem cortar uma sequência UTF-8 no meio;
- listas mostram até seis elementos e informam o tamanho total quando truncadas;
- listas aninhadas são detalhadas até dois níveis;
- funções e módulos aparecem por nome, nunca por endereço interno.

Por exemplo, uma lista de dez inteiros é mostrada como:

```text
[0, 1, 2, 3, 4, 5, ...] (10 elementos)
```

Esses limites são apenas visuais: os valores usados pelo programa não são
alterados, e a renderização não cria alocações permanentes por evento.

## Diagnósticos já são educacionais

O comando normal pode acrescentar contexto seguro aos erros. Um nome próximo e
inequívoco pode produzir:

```text
Nome: 'quantdade'

Talvez voce quisesse usar 'quantidade'.
```

Uma operação entre tipos incompatíveis pode incluir:

```text
Tipos recebidos:
  Esquerda: inteiro
  Direita: texto
```

As sugestões são deliberadamente conservadoras: nomes muito curtos,
identificadores distantes e empates não geram substituição. Como o renderer de
diagnósticos normal já apresenta esse contexto, a v0.3.0 não adiciona uma flag
`--explicar-erros`.
