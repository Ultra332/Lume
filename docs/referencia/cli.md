# Referência da CLI

## Aprendizado

```text
lume aprender                         abre o menu interativo
lume aprender listar                  lista as lições
lume aprender 03                      abre uma lição
lume aprender proxima 03              abre a próxima lição
lume aprender desafio 03              mostra somente o desafio
lume aprender dica 03                 mostra somente a dica
```

As lições são arquivos locais instalados com a Lume. O comando não usa rede, conta, telemetria ou envio de código. Esta versão não corrige desafios automaticamente.

## Scripts e REPL

```text
lume arquivo.lume                     executa um script
lume executar arquivo.lume            forma explícita equivalente
lume                                  executa o projeto atual ou abre o REPL
lume --expr "10 + 20"                 avalia uma expressão
lume --tokens arquivo.lume            mostra tokens
```

## Ferramentas educacionais

```text
lume --analisar arquivo.lume           analisa sem executar
lume --explicar arquivo.lume           executa com explicação
lume --passo arquivo.lume              executa passo a passo
```

## Projetos

```text
lume novo nome                         cria um projeto
lume executar [diretorio]              executa um projeto
lume verificar [diretorio]             verifica sem executar
lume resolver [diretorio]              resolve dependências e atualiza lume.lock
lume testar [diretorio]                executa tests/*.lume
```

## Informações

```text
lume --versao                          mostra a versão
lume --ajuda                           mostra a ajuda
```

Exit code 0 indica conclusão normal, 1 indica erro de linguagem/conteúdo e 2 indica uso inválido da CLI.

Consulte [Projetos e módulos](projetos-e-modulos.md) e [Modos educacionais](modos-educacionais.md).
