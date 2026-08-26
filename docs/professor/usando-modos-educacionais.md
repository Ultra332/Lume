# Usando os modos educacionais

## `--explicar`: discutir causa e efeito

Execute o programa enquanto a Lume descreve valores, decisões, iterações, chamadas e retornos. Use depois de pedir que a turma preveja o resultado.

```sh
lume --explicar exemplo.lume
```

## `--passo`: controlar o ritmo

Avança sobre a mesma execução. Enter continua, `v` mostra variáveis, `p` mostra a pilha lógica, `c` segue sem pausas e `q` encerra. É útil para tabelas de acompanhamento e chamadas de função.

```sh
lume --passo exemplo.lume
```

## `--analisar`: conversar antes de executar

Percorre o código sem produzir seus efeitos. Aponta, por exemplo, nomes não utilizados, sombras e fluxo obviamente inalcançável. Avisos não impedem a execução normal.

```sh
lume --analisar exemplo.lume
```

## Cuidados didáticos

- não transforme a quantidade de avisos em nota;
- não mostre uma explicação longa antes de o aluno formular hipótese;
- use programas pequenos nas primeiras demonstrações;
- diferencie erro da linguagem, erro de raciocínio e escolha válida diferente da esperada.

Detalhes de cada evento estão na [referência dos modos educacionais](../referencia/modos-educacionais.md).
