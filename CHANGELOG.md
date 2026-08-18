# Changelog

Este projeto segue, de forma prática, o formato do [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/).

## [Nao lancado]

### Corrigido

- `ErrorList` agora e dona da copia do nome citado no diagnostico (`LumeError.subject`).
  Antes, o nome apontava para dentro da AST, que `session_execute` libera no caminho de
  erro; `diagnostic_render` entao lia memoria ja liberada e imprimia lixo em vez do nome
  (`escreva(xyz)` mostrava `Nome: '   '` no lugar de `Nome: 'xyz'`).
- `tests/test_diagnostics.c` passava um comprimento maior que o do literal para
  `source_from_bytes`, causando leitura fora dos limites e abortando `make sanitize`
  antes das ultimas suites.
- recursao sem caso base derrubava o processo com SIGSEGV e nenhuma mensagem, por
  falta de um teto de profundidade de chamada. Agora produz um diagnostico de
  execucao normal, com a dica sobre caso base.

### Adicionado

- teste de regressao que exibe o diagnostico depois da liberacao da AST;
- teste de regressao para recursao infinita e para recursao legitima profunda;
- job de CI que executa `make sanitize` (AddressSanitizer + UBSan).

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
