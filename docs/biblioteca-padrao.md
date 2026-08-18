# Biblioteca padrão da Lume 0.1

Os módulos oficiais usam o namespace reservado `lume/*`. São módulos nativos
da versão instalada, não dependências, e não aparecem em `lume.lock`.

## `lume/matematica`

- `absoluto(numero)`: preserva inteiro/decimal; `INT64_MIN` produz erro.
- `minimo(a, b)` e `maximo(a, b)`: retornam inteiro para dois inteiros e
  decimal quando há promoção.
- `potencia(base, expoente)`: retorna decimal e rejeita resultado não finito.
- `raiz(numero)`: retorna decimal; valores negativos são erro numérico.
- `piso(numero)`, `teto(numero)`: retornam decimal.
- `arredonde(numero)`: retorna decimal, arredondando metades para longe de zero.
- `PI` e `E`: constantes decimais imutáveis.

```lume
importe "lume/matematica"
escreva(matematica.raiz(81)) // 9
```

Todas exigem números e produzem erro de tipo para outros valores.

## `lume/texto`

- `maiusculo(texto)` e `minusculo(texto)`: convertem somente ASCII e preservam
  intactos os demais bytes UTF-8.
- `contem(texto, trecho)`, `comeca_com` e `termina_com`: comparação exata de
  sequências UTF-8.
- `aparar(texto)`: remove nas extremidades somente espaço ASCII, tab, CR, LF,
  form feed e tab vertical.
- `subtexto(texto, inicio, fim)`: índices por ponto de código; `fim` é exclusivo.
- `substitua(texto, busca, novo)`: substitui todas as ocorrências não
  sobrepostas; busca vazia é erro.
- `separe(texto, separador)`: retorna lista; separador vazio é erro.
- `junte(lista, separador)`: exige que todos os itens sejam textos; lista vazia
  retorna texto vazio.

Não há normalização Unicode, locale ou conversão implícita de valores.

## `lume/arquivo`

- `existe(caminho)` retorna booleano.
- `leia(caminho)` retorna o conteúdo textual.
- `escreva(caminho, texto)` sobrescreve e retorna `nulo`.
- `adicione(caminho, texto)` acrescenta e retorna `nulo`.
- `remova(caminho)` remove e retorna `nulo`.

Em projetos, caminhos relativos partem da raiz do projeto. Em scripts isolados,
partem do diretório do script principal. A API usa filesystem diretamente, sem
shell ou sandbox, e trabalha apenas com bytes de conteúdo textual.

## `lume/tempo`

- `timestamp()` retorna segundos Unix como inteiro.
- `agora()` retorna hora local em `AAAA-MM-DD HH:MM:SS`, sem formatação locale.
- `durma(ms)` suspende sem busy-wait; aceita inteiro de 0 a 86.400.000 ms.

Falhas de relógio, tipos e limites são diagnósticos Lume.
