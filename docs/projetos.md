# Projetos e comandos da CLI

Um projeto Lume possui `lume.projeto` na raiz e normalmente guarda o programa
principal em `src/principal.lume`.

```sh
lume novo meu_app
cd meu_app
lume
```

Sem argumentos, `lume` examina somente o diretório atual. Se encontrar
`lume.projeto`, executa o projeto, como `lume executar`. Caso contrário, abre o
REPL. A CLI não procura manifestos em diretórios pais.

## Execução e manutenção

```sh
lume arquivo.lume          # executa um script solto
lume executar              # executa o projeto atual
lume executar outro/app    # executa outro projeto
lume executar tarefa.lume  # executa diretamente um script
lume verificar             # analisa o projeto sem executar seus efeitos
lume resolver              # atualiza lume.lock
```

`verificar` e `resolver` também aceitam o caminho de outro projeto. O lockfile
gerado deve ser versionado.

## Testes de projeto

Esta versão usa uma convenção intencionalmente pequena, sem framework ou
sintaxe especial:

```text
meu_app/
├── lume.projeto
├── src/
│   └── principal.lume
└── tests/
    ├── lista.lume
    └── soma.lume
```

Execute na raiz:

```sh
lume testar
```

Somente arquivos `.lume` diretamente dentro de `tests/` são considerados;
subdiretórios não são percorridos. Os nomes são ordenados lexicograficamente.
Cada arquivo roda como script independente e não precisa de manifesto próprio.

## Alias de módulo

O nome local padrão continua sendo o último componente do caminho. Quando esse
nome colide ou quando um nome menor deixa o código mais claro, use `como`:

```lume
importe "lume/texto" como texto_lib

escreva(texto(123))
escreva(texto_lib.maiusculo("lume"))
```

Aliases diferentes não carregam novamente o mesmo módulo; o cache usa o caminho
normalizado do arquivo ou módulo padrão.

Em caso de sucesso, a saída do script é ocultada e aparece apenas o resumo. Em
caso de falha, o diagnóstico e a saída capturada são mostrados. Nenhum teste
encontrado é sucesso; qualquer arquivo com erro faz o comando retornar código
diferente de zero.

`lume dev` e execução contínua não existem nesta versão.
