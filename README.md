# Lume

<p align="center">
  <img src="assets/lume.png" alt="Logo da linguagem Lume" width="144">
</p>

[![CI](https://github.com/Ultra332/Lume/actions/workflows/ci.yml/badge.svg)](https://github.com/Ultra332/Lume/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Ultra332/Lume?label=release)](https://github.com/Ultra332/Lume/releases)
[![License](https://img.shields.io/github/license/Ultra332/Lume)](LICENSE)
[![C11](https://img.shields.io/badge/C-11-blue.svg)](https://en.wikipedia.org/wiki/C11_(C_standard_revision))

Lume é uma linguagem de programação em português criada para ajudar quem está começando a entender não apenas como escrever código, mas como ele funciona.

> Estado: **Lume v0.4.0 — Aprender Fazendo**. Versão experimental com uma trilha de aprendizado offline para alunos e percursos próprios para professores e contribuidores.

```lume
variavel nome = "Maria"
escreva("Olá, " + nome + "!")
```

## Comece em poucos minutos

### Windows

Baixe o instalador `Lume-0.4.0-Windows-x64-Setup.exe` na [página de releases](https://github.com/Ultra332/Lume/releases), instale e abra um terminal novo. Você não precisa instalar GCC, Make ou MSYS2 para usar a linguagem.

```powershell
lume --versao
lume aprender
```

Também existe um ZIP portátil. O [guia de instalação no Windows](docs/referencia/instalacao-windows.md) explica as duas opções, checksums e desinstalação.

### Linux

Ainda não há pacote binário oficial para Linux. Para experimentar, compile o código-fonte com GCC e Make:

```sh
make
./lume aprender
```

Veja os requisitos no [guia de compilação](docs/desenvolvimento/compilando.md).

## Aprendendo

Comece pelo guia [Primeiros passos](docs/aluno/primeiros-passos.md) ou estude diretamente no terminal com `lume aprender`. A trilha começa pelo primeiro programa e avança por variáveis, entrada, decisões, repetições, funções e listas.

## Para professores

O percurso [Lume para professores](docs/professor/comecando.md) reúne preparação do laboratório, sequência sugerida, plano de aula, atividades e formas de usar os modos educacionais em sala.

## Recursos educacionais

```text
lume --explicar programa.lume   executa e explica o que acontece
lume --passo programa.lume      acompanha a execução passo a passo
lume --analisar programa.lume   analisa o código sem executá-lo
```

Esses recursos têm papéis diferentes. Veja [Modos educacionais](docs/referencia/modos-educacionais.md).

## O que dá para construir?

Os [exemplos](exemplos) vão de um “Olá, mundo” e pequenos algoritmos até projetos inteiramente escritos em Lume:

- [Cobrinha](exemplos/projetos/cobrinha);
- [Ping-pong](exemplos/projetos/ping-pong);
- [Doom/raycaster com BSP](exemplos/projetos/Doom).

## Documentação

A [página inicial da documentação](docs/README.md) leva você ao percurso certo para aprender, ensinar, consultar a linguagem ou contribuir.

## Contribuindo

Leia o [guia de contribuição](.github/CONTRIBUTING.md). A implementação de referência usa C11 e mantém sua arquitetura e seus testes em uma área separada da documentação introdutória.

## Licença

Lume é distribuída sob a [Apache License 2.0](LICENSE) (`Apache-2.0`).
