# Começando com Lume

Este guia pressupõe que `lume --versao` já funciona. Você pode experimentar no
REPL executando apenas `lume` em uma pasta sem `lume.projeto`, ou salvar os
exemplos em arquivos `.lume`. Dentro de um projeto, `lume` executa o programa
principal automaticamente.

## Valores e nomes

```lume
constante CURSO = "Lume"
variavel aulas = 3
variavel ativo = verdadeiro
escreva(CURSO)
escreva(aulas)
escreva(ativo)
```

Constantes não podem receber outro valor. Variáveis podem. Nomes existem no bloco em que foram declarados e em blocos internos.

## Decisões

```lume
variavel nota = 8
se nota >= 7 {
    escreva("aprovado")
} senao {
    escreva("continue estudando")
}
```

## Repetição

```lume
para numero de 1 ate 5 {
    escreva(numero)
}

variavel restante = 3
enquanto restante > 0 {
    escreva(restante)
    restante = restante - 1
}
```

## Funções e listas

```lume
funcao dobro(valor) {
    retorne valor * 2
}

variavel numeros = [2, 4, 6]
escreva(dobro(numeros[0]))
```

Continue com o [guia de uso](guia-de-uso.md) e [algoritmos](algoritmos.md),
consulte a referência em [LANGUAGE.md](../LANGUAGE.md) e experimente os arquivos
em `exemplos/basico`.
