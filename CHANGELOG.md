# Changelog

Este projeto segue, de forma prática, o formato do [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/).

## [0.2.1] — 2026-08-20

Atualização visual e de usabilidade para programas interativos no terminal.

### Adicionado

- cores de texto e fundo em `lume/terminal`;
- `terminal.estilize` para compor interfaces coloridas antes de uma única escrita;
- cores ANSI básicas, claras e restauração da cor padrão;
- testes de cores, argumentos inválidos, chamadas repetidas e restauração de estado.

### Alterado

- a Cobrinha usa células coloridas de duas colunas e frame completo em memória;
- o jogo exige uma área mínima de 64 colunas por 21 linhas e orienta o usuário
  antes de iniciar quando o terminal é pequeno;
- documentação e distribuição passam a identificar a versão 0.2.1.

### Corrigido

- frames da Cobrinha deixam de acumular ou piscar por limpezas e escritas pequenas;
- cores ativas e cursor oculto são restaurados no encerramento normal ou em erro tratado.

## [0.2.0] — 2026-08-20

Primeira atualização funcional após a estabilização da série 0.1.x, focada em
permitir pequenos programas e jogos interativos no terminal.

### Adicionado

- iteração direta de listas com `para item em lista`;
- `pare` e `continue` para controle do laço mais interno;
- convite opcional em `leia(texto)`, preservando `leia()`;
- módulos `lume/aleatorio` e `lume/terminal`;
- eventos educacionais das novas construções;
- exemplos da v0.2.0 e projeto/tutorial oficial da Cobrinha;
- 17ª suíte de testes dedicada às APIs interativas.

### Alterado

- o analyzer reconhece iteração de coleções e seus escopos;
- a documentação e distribuição passam a identificar a versão 0.2.0.

### Corrigido

- controle de fluxo de laços agora possui estados explícitos, sem atravessar
  fronteiras de função;
- iteração sobre lista modificada usa uma fotografia rasa segura e determinística.

## [0.1.2] — 2026-08-20

Atualização de memória e desempenho compatível com programas Lume 0.1.x.

### Corrigido

- escopos temporários de blocos, laços e chamadas deixam de permanecer na arena
  global até o fim da execução;
- loops e chamadas repetitivas deixam de consumir memória proporcionalmente à
  quantidade de trabalho;
- ambientes capturados por closures e toda a cadeia léxica necessária continuam
  preservados até o encerramento da sessão ou do módulo.

### Adicionado

- testes de ownership para atribuições, laços, funções, listas, textos, closures,
  módulos e execuções repetidas no REPL.

## [0.1.1] — 2026-08-19

Atualização de estabilidade compatível com os programas da v0.1.0.

### Corrigido

- `ErrorList` agora e dona da copia do nome citado no diagnostico (`LumeError.subject`).
  Antes, o nome apontava para dentro da AST, que `session_execute` libera no caminho de
  erro; `diagnostic_render` entao lia memoria ja liberada e imprimia lixo em vez do nome
  (`escreva(xyz)` mostrava `Nome: '   '` no lugar de `Nome: 'xyz'`).
- `tests/test_diagnostics.c` passava um comprimento maior que o do literal para
  `source_from_bytes`, causando leitura fora dos limites e abortando `make sanitize`
  antes das ultimas suites.
- recursao excessiva agora termina com diagnostico da Lume, sem esgotar a pilha C;
- o diagnostico desse limite usa a entrada corrente do REPL, preservando linha e marcador.
- expressoes aninhadas alem de 128 niveis agora produzem erro sintatico controlado;
- diagnosticos do REPL agora conservam a Source correta entre entradas;
- declaracoes POSIX de `localtime_r` e `nanosleep` ficam visiveis no build Linux.

### Adicionado

- teste de regressao que exibe o diagnostico depois da liberacao da AST;
- job de CI que executa `make sanitize` (AddressSanitizer + UBSan).
- suite de estabilidade para entradas incompletas, limites de runtime e programas maiores.
- testes de fronteira do parser e diagnosticos cross-Source de nome, tipo, indice e closure.

## [0.1.0] — 2026-08-18

Primeira candidata a release pública da Lume.

### Adicionado

- linguagem interpretada em português com funções, closures, listas e controle de fluxo;
- REPL, CLI, diagnósticos educacionais, análise estática, explicação e modo passo a passo;
- módulos, projetos de múltiplos arquivos, dependências locais e lockfile;
- biblioteca padrão organizada;
- 15 suítes automatizadas, documentação pública e automação de release para Windows.
- instalador Windows por usuário, pacote portátil e checksums SHA-256;
- comando de projeto `lume testar`, com descoberta determinística de `tests/*.lume`;
- execução automática do projeto atual ao chamar `lume` em uma pasta com manifesto;
- guias para instalação, solução de problemas, projetos e uso da linguagem.
- identidade visual e ícone multirresolução incorporado ao executável e ao instalador Windows.

### Alterado

- o REPL não exibe uma segunda linha `nulo` após chamadas como `escreva(...)`;
- a instalação para usuários finais não exige GCC, Make ou MSYS2;
- o comando `lume` sem argumentos continua abrindo o REPL fora de projetos.

### Observações

- a API e a linguagem podem evoluir antes da versão 1.0;
- o projeto é distribuído sob a Apache License 2.0 (`Apache-2.0`).
