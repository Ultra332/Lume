# Condições

## O que vamos aprender

- comparar valores;
- escolher entre dois caminhos;
- testar casos de limite.

## Primeiro exemplo

```lume
variavel idade = inteiro(leia("Sua idade: "))

se idade >= 18 {
    escreva("Você é maior de idade.")
} senao {
    escreva("Você é menor de idade.")
}
```

## O que aconteceu?

A expressão `idade >= 18` produz `verdadeiro` ou `falso`. O bloco depois de `se` executa no primeiro caso; o bloco de `senao`, no segundo.

## Agora tente

Peça uma nota e mostre “aprovado” quando ela for pelo menos 7.

## Dica

Teste valores abaixo, iguais e acima do limite. Isso ajuda a encontrar comparações erradas.

## Veja a execução

Use `lume --explicar idade.lume` para ver o resultado da condição e o caminho escolhido.

## Próximo passo

Aprenda a [repetir instruções](repeticoes.md).
