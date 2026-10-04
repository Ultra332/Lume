# Transição da Lume para Python

## Objetivo pedagógico

A ponte reduz a sensação de que o aluno precisa começar novamente. Ela não pretende ensinar Python inteiro nem apresentar equivalência formal entre as linguagens.

O foco é separar três perguntas:

1. qual conceito resolve este problema?
2. como a Lume expressa esse conceito?
3. como Python expressa o mesmo conceito e quais regras mudam?

## Sequência sugerida

1. O aluno resolve um problema pequeno em Lume.
2. Identifica variável, expressão, condição, repetição ou função presentes.
3. Consulta `lume aprender conceito nome`.
4. A turma prevê como o programa poderia aparecer em Python.
5. O professor abre `lume aprender transicao python` ou usa `lume comparar`.
6. A turma discute sintaxe, indentação e diferenças semânticas.
7. O aluno reescreve e testa uma versão pequena em um ambiente Python separado.

## Atividade curta

Use uma lista de nomes. Peça ao aluno para percorrê-la em Lume, nomear os conceitos, prever a forma Python e explicar por que papéis semelhantes não tornam os runtimes idênticos.

## Limites importantes

- `lume comparar` não valida nem executa Python;
- módulos Lume e pacotes Python não são equivalentes internamente;
- constantes em Lume são protegidas pelo runtime; nomes em maiúsculas em Python são convenção;
- closures, terminal e projetos complexos não recebem tradução automática nesta versão.

Os materiais funcionam offline e não coletam código, progresso ou dados do aluno.
