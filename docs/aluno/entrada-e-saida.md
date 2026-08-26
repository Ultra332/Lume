# Entrada e saída

## O que vamos aprender

- perguntar algo no terminal;
- guardar a resposta;
- converter texto para número.

## Primeiro exemplo

```lume
variavel nome = leia("Qual é o seu nome? ")
escreva("Olá, " + nome + "!")
```

## O que aconteceu?

`leia` mostra o convite e espera a pessoa pressionar Enter. A resposta sempre começa como texto. Para calcular com uma idade digitada, use:

```lume
variavel idade = inteiro(leia("Sua idade: "))
escreva(idade + 1)
```

## Agora tente

Peça dois números e mostre a soma deles.

## Dica

Converta cada resposta separadamente com `inteiro`.

## Veja a execução

Use `lume --explicar entrada.lume`. O programa continuará pedindo a entrada normalmente.

## Próximo passo

Use uma resposta para tomar [decisões](condicoes.md).
