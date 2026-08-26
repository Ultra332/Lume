# Listas

## O que vamos aprender

- guardar vários valores juntos;
- acessar uma posição;
- percorrer todos os elementos.

## Primeiro exemplo

```lume
variavel frutas = ["maçã", "banana", "uva"]
escreva(frutas[0])

para fruta em frutas {
    escreva(fruta)
}
```

## O que aconteceu?

Os colchetes criam uma lista. As posições começam em zero, por isso `frutas[0]` é o primeiro item. O `para ... em` visita cada elemento.

## Agora tente

Crie uma lista de quatro notas e calcule a soma usando uma variável acumuladora.

## Dica

Comece com `variavel soma = 0` antes do laço.

## Veja a execução

Use `lume --explicar listas.lume` e observe as leituras e iterações.

## Próximo passo

Descubra como separar código em [módulos](modulos.md).
