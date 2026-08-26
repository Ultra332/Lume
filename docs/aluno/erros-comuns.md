# Erros comuns

Um erro é uma pista sobre o que o programa encontrou. Leia a linha destacada, a categoria e a dica antes de mudar o código.

## Variável não definida

```lume
escreva(pontuacao)
```

Lume informa que o nome não foi definido e pode sugerir um nome próximo. Confira a grafia e se a declaração acontece antes do uso.

## Tipo incompatível

```lume
escreva("idade: " + 18)
```

Texto e número não são somados automaticamente. Use `"idade: " + texto(18)`.

## Índice inválido

```lume
variavel nomes = ["Ana"]
escreva(nomes[1])
```

A primeira posição é zero; essa lista só possui a posição 0. Antes de acessar, pense em `tamanho(nomes)`.

## Recursão sem caso base

```lume
funcao repetir() {
    retorne repetir()
}
repetir()
```

Uma função recursiva precisa de uma condição que pare as chamadas. A Lume limita a profundidade para evitar encerrar inesperadamente.

## `pare` fora de repetição

```lume
pare
```

`pare` só tem significado dentro de `para` ou `enquanto`. Mova-o para o laço que deseja encerrar.

## Import incorreto

```lume
importe "utilidades"
```

Confira se `utilidades.lume` existe no local esperado e se o caminho é relativo ao arquivo importador. Em projetos, use `lume verificar` para analisar sem executar.

## Como investigar

1. reduza o programa até manter apenas o erro;
2. confira a linha destacada;
3. use `lume --analisar arquivo.lume`;
4. quando o código executar, use `--explicar` ou `--passo` para acompanhar valores.

Volte para [Pequenos projetos](projetos.md).
