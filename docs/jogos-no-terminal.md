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

## 5. Desenhar

`terminal.limpe()` inicia o quadro. `terminal.posicione(coluna, linha)` coloca
bordas, comida e cada parte da cobra em coordenadas absolutas. O cursor fica
oculto durante a partida e é mostrado novamente no encerramento normal.

## 6. Gerar comida

`aleatorio.inteiro(1, limite)` escolhe coordenadas inclusivas. A função
`nova_comida` repete a escolha enquanto a posição estiver ocupada.

## 7. Detectar colisão

Comparações encerram o jogo ao ultrapassar as bordas. A função `ocupa` percorre
a cobra e identifica uma coordenada já usada. Toda essa lógica está escrita em
Lume; C fornece apenas primitivas genéricas de terminal e aleatoriedade.

## 8. Aumentar a pontuação

Ao alcançar a comida, o programa soma um ponto, mantém a cauda e sorteia outra
posição. `tempo.durma(120)` controla o ritmo sem espera ocupada.

Este primeiro terminal trabalha com teclas textuais simples, não com um sistema
geral de eventos. Se o processo for encerrado à força enquanto o cursor estiver
oculto, execute `terminal.mostre_cursor()` em outro programa ou reabra o terminal.
