# Guia de uso da Lume

Este guia ensina o fluxo cotidiano depois da instalação. Para instalar no
Windows, consulte [instalacao-windows.md](instalacao-windows.md).

## 1. Confirmar a instalação

Abra um terminal novo e execute:

```powershell
lume --versao
```

Resultado esperado:

```text
Lume 0.1.0
```

Use `lume --ajuda` sempre que quiser rever os comandos disponíveis.

## 2. Experimentar no REPL

Em uma pasta que não contenha `lume.projeto`, execute:

```powershell
lume
```

O prompt `>>>` aceita expressões e instruções:

```text
>>> 10 + 20
30
>>> "Olá"
Olá
>>> escreva("Olá, Lume!")
Olá, Lume!
>>> nulo
nulo
```

`escreva` retorna `nulo` internamente, mas o REPL não mostra uma segunda linha
automática. Digite `:ajuda` para comandos do REPL e `:sair` para encerrá-lo.

## 3. Executar um script

Crie `ola.lume`:

```lume
variavel nome = "Lume"
escreva("Olá, " + nome + "!")
```

No diretório do arquivo:

```powershell
lume ola.lume
```

Você também pode informar um caminho completo ou usar `lume executar ola.lume`.

## 4. Criar um projeto

```powershell
lume novo meu_projeto
cd meu_projeto
lume
```

Dentro de uma pasta com `lume.projeto`, executar apenas `lume` inicia o projeto
atual. Fora de projetos, o mesmo comando abre o REPL. A busca não sobe para
diretórios pais.

Estrutura inicial:

```text
meu_projeto/
├── lume.projeto
└── src/
    └── principal.lume
```

Comandos úteis:

```text
lume executar              executa o projeto atual
lume verificar             analisa sem executar efeitos
lume resolver              atualiza lume.lock
lume testar                executa tests/*.lume
```

## 5. Adicionar testes simples

Crie a pasta `tests` na raiz do projeto e adicione `tests/soma.lume`:

```lume
variavel resultado = 10 + 20
se resultado != 30 {
    variavel falha = 1 / 0 // provoca erro se o cálculo estiver errado
}
```

Execute:

```powershell
lume testar
```

Cada arquivo `.lume` diretamente em `tests/` é executado em ordem lexicográfica.
Exit code `0` significa que o script terminou sem erros; não existe palavra-chave
especial de teste ou `assert` nesta versão. Subdiretórios não são percorridos.

## 6. Entender o programa

```powershell
lume --analisar ola.lume
lume --explicar ola.lume
lume --passo ola.lume
```

- `--analisar` examina sem executar;
- `--explicar` executa e descreve acontecimentos;
- `--passo` pausa entre etapas.

Avisos educacionais não impedem a execução normal.

## 7. Trabalhar no VS Code

Abra a pasta do programa, use Terminal → Novo Terminal e execute os mesmos
comandos. A instalação não modifica o VS Code nem instala extensão. Se o comando
não for reconhecido, feche todas as janelas do VS Code e abra novamente para que
ele receba o `PATH` atualizado.

## 8. Próximos passos

- [sintaxe básica](comecando.md);
- [projetos e CLI](projetos.md);
- [biblioteca padrão](biblioteca-padrao.md);
- [algoritmos e exercícios](algoritmos.md);
- [especificação completa](../LANGUAGE.md).
