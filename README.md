# Lume

<p align="center">
  <img src="assets/lume.png" alt="Logo da linguagem Lume" width="144">
</p>

[![CI](https://github.com/Ultra332/Lume/actions/workflows/ci.yml/badge.svg)](https://github.com/Ultra332/Lume/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Ultra332/Lume?label=release)](https://github.com/Ultra332/Lume/releases)
[![License](https://img.shields.io/github/license/Ultra332/Lume)](LICENSE)
[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))

Lume é uma linguagem de programação educacional em português, pequena o bastante para ser estudada e completa o bastante para ensinar programas reais. A implementação de referência é escrita em C11 e inclui interpretador, REPL, análise estática, módulos, projetos reproduzíveis e uma biblioteca padrão organizada.

> Estado: **Lume v0.2.0 — Primeiros Projetos**. Esta versão experimental permite transformar fundamentos em programas interativos e pequenos jogos no terminal.

## Um primeiro programa

```lume
variavel nome = "mundo"
escreva("Olá, " + nome + "!")
```

Salve como `ola.lume` e execute:

```sh
lume ola.lume
```

## O que já funciona

- números, textos, booleanos, listas e `nulo`;
- variáveis, constantes, escopo léxico e closures;
- `se`, `senao`, `enquanto`, `para`, `para ... em`, `pare` e `continue`;
- funções, recursão e retornos;
- módulos com `importe`, projetos e dependências locais;
- biblioteca padrão para texto, matemática, aleatoriedade, terminal, arquivos e tempo;
- REPL e diagnósticos em português;
- análise estática educacional, explicação e execução passo a passo.

A referência da sintaxe e semântica está em [LANGUAGE.md](LANGUAGE.md). Se esta
é sua primeira vez com a linguagem, siga o [guia de uso](docs/guia-de-uso.md).

## Instalação

### Windows

1. Baixe `Lume-0.2.0-Windows-x64-Setup.exe` na página de releases.
2. Execute o instalador e conclua as etapas apresentadas.
3. Abra um novo PowerShell, CMD ou terminal do VS Code.
4. Verifique:

   ```powershell
   lume --versao
   ```

O instalador é offline, instala apenas para o usuário atual e não exige GCC,
Make, MSYS2 ou o código-fonte.

### Versão portátil

Baixe `Lume-0.2.0-Windows-x64.zip`, extraia todo o conteúdo e execute
`lume.exe`. O ZIP não modifica o sistema; para chamar `lume` de qualquer pasta,
adicione manualmente a pasta extraída ao `PATH`.

Consulte o [guia completo de instalação no Windows](docs/instalacao-windows.md),
incluindo checksums, desinstalação e o possível aviso do SmartScreen.

## Uso

```text
lume arquivo.lume              executa um arquivo
lume                           executa o projeto atual ou abre o REPL
lume --analisar arquivo.lume   analisa sem executar
lume --explicar arquivo.lume   explica durante a execução
lume --passo arquivo.lume      executa passo a passo
lume novo nome                 cria um projeto
lume executar [diretorio]      executa um projeto
lume verificar [diretorio]     valida um projeto
lume resolver [diretorio]      gera lume.lock
lume testar [diretorio]        executa tests/*.lume do projeto
lume --ajuda                   mostra a ajuda completa
```

## Ferramentas de aprendizado

- `--analisar` percorre o código antes da execução e aponta usos, fluxos e sombreamentos; avisos não impedem a execução normal.
- `--explicar` acompanha a execução e descreve acontecimentos observados.
- `--passo` pausa entre etapas, permitindo acompanhar o estado do programa.

Guias: [índice da documentação](docs/README.md), [começando](docs/comecando.md),
[guia de uso](docs/guia-de-uso.md), [modos educacionais](docs/modos-educacionais.md),
[algoritmos](docs/algoritmos.md), [uso em sala](docs/para-professores.md) e
[próximos estudos](docs/depois-da-lume.md).

## Projetos e módulos

Crie e execute um projeto com:

```sh
lume novo meu_projeto
cd meu_projeto
lume
```

Quando o diretório atual contém `lume.projeto`, `lume` executa esse projeto; fora de um projeto, abre o REPL. O manifesto define entrada, fonte e dependências locais, e `lume resolver` produz o `lume.lock`, que deve ser versionado.

Para testes simples, crie `tests/exemplo.lume` na raiz do projeto e execute `lume testar`. Consulte o [guia de projetos](docs/projetos.md), [LANGUAGE.md](LANGUAGE.md) e a [biblioteca padrão](docs/biblioteca-padrao.md).

O projeto oficial [Cobrinha](exemplos/projetos/cobrinha) demonstra listas,
funções, aleatoriedade, tempo e teclado em um jogo inteiramente escrito em Lume.
Veja o tutorial [Jogos no terminal](docs/jogos-no-terminal.md).

## Desenvolvendo a Lume

Esta seção é somente para quem deseja modificar a implementação. Usuários da
linguagem não precisam instalar compilador ou baixar o código-fonte.

Contribuidores devem clonar o repositório e usar MSYS2 UCRT64/GCC e Make no
Windows. O build usa C11 e os avisos `-Wall -Wextra -Wpedantic -Wconversion
-Wshadow` como parte do contrato de qualidade.

```sh
make clean
make
make test
make sanitize   # quando AddressSanitizer/UBSan estiverem disponíveis
```

A arquitetura está descrita em [ARCHITECTURE.md](ARCHITECTURE.md), os testes em [tests/README.md](tests/README.md) e as regras de colaboração em [CONTRIBUTING.md](CONTRIBUTING.md). Veja também o [guia de desenvolvimento](docs/desenvolvimento.md).

## Projeto e comunidade

- [Changelog](CHANGELOG.md)
- [Roadmap](ROADMAP.md)
- [Código de conduta](CODE_OF_CONDUCT.md)
- [Política de segurança](SECURITY.md)
- [Preparação da release](docs/release.md)

## Licença

Lume é distribuída sob a [Apache License 2.0](LICENSE) (`Apache-2.0`).
