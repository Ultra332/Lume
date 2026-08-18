# Changelog

Este projeto segue, de forma prática, o formato do [Keep a Changelog](https://keepachangelog.com/pt-BR/1.1.0/).

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
