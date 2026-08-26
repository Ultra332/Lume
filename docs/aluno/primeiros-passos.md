# Seu primeiro programa

## O que vamos aprender

- o que é um arquivo de programa;
- como mostrar uma mensagem;
- como executar o arquivo no terminal.

## Primeiro exemplo

Crie um arquivo chamado `ola.lume`:

```lume
escreva("Olá, mundo!")
```

Abra o terminal na pasta desse arquivo e execute:

```sh
lume ola.lume
```

## O que aconteceu?

`escreva` é uma função que mostra um valor. O texto fica entre aspas; os parênteses indicam o valor enviado para a função. O terminal é apenas a janela em que você digita comandos e vê resultados.

Se `lume --versao` ainda não funcionar, siga a [instalação no Windows](../referencia/instalacao-windows.md).

## Agora tente

Troque a mensagem pelo seu nome e execute de novo.

## Dica

Não retire as aspas nem os parênteses.

## Veja a execução

```sh
lume --explicar ola.lume
```

Você também pode abrir a trilha offline com `lume aprender`.

## Próximo passo

Aprenda a [guardar valores em variáveis](variaveis.md).
