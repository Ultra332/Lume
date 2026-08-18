# Lume

Lume é uma linguagem de programação educacional em português, pequena o bastante para ser estudada e completa o bastante para ensinar programas reais. A implementação de referência é escrita em C11 e inclui interpretador, REPL, análise estática, módulos, projetos reproduzíveis e uma biblioteca padrão organizada.

> Estado: **v0.1.0 candidata à primeira release pública**. A linguagem é utilizável, mas ainda está em evolução e pode receber mudanças incompatíveis antes da versão 1.0.

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
- `se`, `senao`, `enquanto` e `para`;
- funções, recursão e retornos;
- módulos com `importe`, projetos e dependências locais;
- biblioteca padrão para texto, matemática, listas, conversão e entrada/saída;
- REPL e diagnósticos em português;
- análise estática educacional, explicação e execução passo a passo.

A referência da sintaxe e semântica está em [LANGUAGE.md](LANGUAGE.md).

## Instalação

### Windows (MSYS2 UCRT64)

1. Instale o [MSYS2](https://www.msys2.org/) e abra o terminal **MSYS2 UCRT64**.
2. Instale o compilador e o Make:

   ```sh
   pacman -Syu
   pacman -S --needed mingw-w64-ucrt-x86_64-gcc make
   ```

3. Na raiz do repositório, compile e valide:

   ```sh
   make clean
   make
   make test
   ```

4. Execute `./lume.exe --versao`. Você pode copiar `lume.exe` para uma pasta presente no `PATH`.

### Linux

Com GCC e Make instalados:

```sh
make clean
make
make test
./lume --versao
```

## Uso

```text
lume arquivo.lume              executa um arquivo
lume                           abre o REPL
lume --analisar arquivo.lume   analisa sem executar
lume --explicar arquivo.lume   explica durante a execução
lume --passo arquivo.lume      executa passo a passo
lume novo nome                 cria um projeto
lume executar [diretorio]      executa um projeto
lume verificar [diretorio]     valida um projeto
lume resolver [diretorio]      gera lume.lock
lume --ajuda                   mostra a ajuda completa
```

## Ferramentas de aprendizado

- `--analisar` percorre o código antes da execução e aponta usos, fluxos e sombreamentos; avisos não impedem a execução normal.
- `--explicar` acompanha a execução e descreve acontecimentos observados.
- `--passo` pausa entre etapas, permitindo acompanhar o estado do programa.

Guias: [começando](docs/comecando.md), [modos educacionais](docs/modos-educacionais.md), [algoritmos](docs/algoritmos.md), [uso em sala](docs/para-professores.md) e [próximos estudos](docs/depois-da-lume.md).

## Projetos e módulos

Crie um projeto com `lume novo meu_projeto`. O manifesto `lume.projeto` define entrada, fonte e dependências locais; `lume resolver` produz o `lume.lock`, que deve ser versionado. Consulte [LANGUAGE.md](LANGUAGE.md) e [docs/biblioteca-padrao.md](docs/biblioteca-padrao.md).

## Desenvolvimento

O build usa C11 e os avisos `-Wall -Wextra -Wpedantic -Wconversion -Wshadow` como parte do contrato de qualidade.

```sh
make clean
make
make test
make sanitize   # quando AddressSanitizer/UBSan estiverem disponíveis
```

A arquitetura está descrita em [ARCHITECTURE.md](ARCHITECTURE.md), os testes em [tests/README.md](tests/README.md) e as regras de colaboração em [CONTRIBUTING.md](CONTRIBUTING.md).

## Projeto e comunidade

- [Changelog](CHANGELOG.md)
- [Roadmap](ROADMAP.md)
- [Código de conduta](CODE_OF_CONDUCT.md)
- [Política de segurança](SECURITY.md)
- [Preparação da release](docs/release.md)

## Licença

Lume é distribuída sob a [Apache License 2.0](LICENSE) (`Apache-2.0`).
