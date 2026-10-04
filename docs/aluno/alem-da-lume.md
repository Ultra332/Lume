# Além da Lume

## Você não está começando do zero

Programar não é decorar palavras-chave. Quando você guarda um valor, toma uma decisão, percorre uma lista ou cria uma função, está usando conceitos presentes em muitas linguagens.

```lume
variavel idade = 18
```

```python
idade = 18
```

Nos dois exemplos, `idade` está associado ao valor 18. Python não usa a palavra `variavel`; isso é uma diferença de sintaxe. As linguagens também têm regras próprias, portanto uma não é simples tradução da outra.

## Consultar conceitos

```text
lume aprender conceitos
lume aprender conceito variavel
lume aprender conceito funcao
```

As lições ensinam pela prática. O catálogo serve como consulta: definição, exemplo, uso, conceitos relacionados e lição correspondente.

## Primeira ponte: Python

Python foi escolhido como primeiro destino educacional da v0.5.0 por permitir exemplos iniciais curtos e por ser amplamente usado em educação e diferentes áreas. Isso não significa que seja a melhor linguagem para todas as pessoas ou projetos.

```text
lume aprender transicao
lume aprender transicao python
```

A trilha compara primeiro programa, variáveis, entrada e saída, condições, repetições, funções, listas e módulos básicos. Cada tópico explica o que mudou e o que continuou igual.

## Comparar um programa pequeno

```text
lume comparar meu-programa.lume --com python
```

O comando usa a AST real da Lume e reconhece um subconjunto: literais, variáveis, constantes, atribuições, operadores, condições, laços, funções simples, retorno, listas, índices, `escreva` e `leia`.

Ele não executa Python e não precisa que Python esteja instalado. Closures complexas, acesso a membros, módulos e APIs de terminal não recebem código inventado: a ferramenta informa que ainda não há comparação educacional.

Essa saída é uma ponte para leitura e discussão, não um transpiler nem garantia de equivalência completa.

## Próximo passo

1. identifique os conceitos que já conhece;
2. observe a nova sintaxe;
3. anote diferenças reais de comportamento;
4. reescreva programas pequenos;
5. consulte a documentação da nova linguagem em vez de presumir equivalência.

Pratique com o [exemplo da ponte Python](../../exemplos/v050/ponte-python.lume).
