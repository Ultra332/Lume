# Jogos no terminal

A Lume 0.2 combina `lume/terminal`, `lume/tempo` e `lume/aleatorio` para criar
projetos interativos sem bibliotecas externas. O exemplo completo está em
[`exemplos/projetos/cobrinha`](../exemplos/projetos/cobrinha).

Execute na raiz do repositório:

```sh
lume executar exemplos/projetos/cobrinha
```

Use W, A, S e D para mover e Q para sair. Rode em PowerShell, CMD ou terminal
Linux real; consoles que não interpretam ANSI não conseguem redesenhar a tela.

## 1. Representar a cobra

Cada parte é uma lista `[x, y]`, e a cobra é uma lista dessas partes. A cabeça
fica no último índice. `para parte em cobra` deixa o desenho e a colisão legíveis
sem obrigar o estudante a administrar índices.

## 2. Guardar posição e direção

`dx` e `dy` descrevem quanto a cabeça anda a cada quadro. Por exemplo, `[1, 0]`
vai para a direita. A nova posição soma direção e cabeça atual.

## 3. Movimentar

O programa acrescenta a nova cabeça com `adicione`. Quando não comeu, remove a
cauda no índice zero. Quando comeu, conserva a cauda, fazendo a cobra crescer.

## 4. Ler uma tecla

`terminal.tecla()` retorna imediatamente: um texto quando há tecla, ou `nulo`.
Assim o jogo continua se movendo sem exigir Enter nem criar busy-wait. As regras
impedem uma inversão direta que faria a cabeça colidir com o corpo.

## 5. Desenhar e colorir

`terminal.limpe()` é chamado uma vez no início. Cada célula usa dois espaços
com cor de fundo, aproximando sua largura da altura visual de uma linha. A
Cobrinha combina trechos criados por `terminal.estilize`, monta o quadro inteiro
em um texto e só então usa `terminal.posicione(1, 1)` e `escreva`. Assim cada
frame requer uma única escrita, sem acumular tabuleiros no scroll.

Antes de limpar ou ocultar o cursor, `terminal.tamanho()` confirma uma área
mínima de 64 colunas por 21 linhas. Terminais menores recebem uma orientação e
o jogo não inicia.

## 6. Gerar comida

`aleatorio.inteiro(1, limite)` escolhe coordenadas inclusivas. A função
`nova_comida` repete a escolha enquanto a posição estiver ocupada.

## 7. Detectar colisão

Comparações encerram o jogo ao ultrapassar as bordas. A função `ocupa` percorre
a cobra e identifica uma coordenada já usada. Toda essa lógica está escrita em
Lume; C fornece apenas primitivas genéricas de terminal e aleatoriedade.

## 8. Aumentar a pontuação

Ao alcançar a comida, o programa soma um ponto, mantém a cauda e sorteia outra
posição. `tempo.durma(180)` controla o ritmo sem espera ocupada.

Este terminal trabalha com teclas textuais simples, não com um sistema geral de
eventos. No encerramento normal ou em erro tratado, o módulo restaura cores e
cursor. Uma interrupção externa que mate o processo imediatamente não permite
ao programa executar essa limpeza; nesse caso, reabra o terminal.
