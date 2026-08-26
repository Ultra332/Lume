# Módulos

## O que vamos aprender

- separar responsabilidades em arquivos;
- exportar uma função;
- importar e usar um módulo.

## Primeiro exemplo

Crie `matematica.lume`:

```lume
exporte funcao dobro(numero) {
    retorne numero * 2
}
```

Ao lado, crie `principal.lume`:

```lume
importe "matematica"
escreva(matematica.dobro(8))
```

## O que aconteceu?

`exporte` torna a função disponível para outro arquivo. `importe` carrega o módulo; o ponto acessa o que ele publicou.

## Agora tente

Adicione uma função `triplo` ao módulo e use as duas no programa principal.

## Dica

Os caminhos de imports são relativos ao arquivo que faz a importação.

## Veja a execução

Use `lume --analisar principal.lume` antes de executar e depois compare com `--explicar`.

## Próximo passo

Monte um [pequeno projeto](projetos.md).
