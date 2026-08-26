# Pequenos projetos

## O que vamos aprender

- combinar conceitos em um programa útil;
- dividir o problema em partes;
- testar casos diferentes.

## Primeiro exemplo

Comece por um programa de média: guarde notas numa lista, some-as em um laço e mostre o resultado. Há uma versão executável em [exemplos/algoritmos/media.lume](../../exemplos/algoritmos/media.lume).

Para criar uma pasta formal de projeto:

```sh
lume novo meu_projeto
cd meu_projeto
lume
```

## O que aconteceu?

Um projeto guarda o programa principal em `src/principal.lume` e suas configurações em `lume.projeto`. Você pode crescer aos poucos e adicionar módulos e testes quando precisar.

## Agora tente

Escolha um projeto: calculadora de operações básicas, quiz de três perguntas ou jogo de adivinhação.

## Dica

Faça primeiro uma versão pequena que funciona. Depois extraia funções e cuide de entradas inválidas.

## Veja a execução

Use `lume verificar` para analisar o projeto, `lume testar` para seus testes e `lume --explicar arquivo.lume` para um script isolado.

## Próximo passo

Explore [algoritmos e exercícios](algoritmos.md), [jogos no terminal](projetos-no-terminal.md) ou veja [o que estudar depois](depois-da-lume.md).
