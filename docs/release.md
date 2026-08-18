# Processo de release

## Gerar o executável Windows

No terminal MSYS2 UCRT64, a partir da raiz:

```sh
sh scripts/build-windows.sh
```

O script limpa, compila, executa as 15 suítes e copia o resultado para `dist/lume.exe`. A pasta `dist` é ignorada pelo Git; binários de release devem ser anexados à release do GitHub, não versionados.

## Checklist técnico

- conferir `src/common.h` e `lume --versao`;
- executar `make clean`, `make`, `make test` e, se disponível, `make sanitize`;
- testar `dist/lume.exe --ajuda`, `--versao`, REPL e um arquivo fora da árvore-fonte;
- revisar CHANGELOG, README, SECURITY e o arquivo `LICENSE` (Apache-2.0);
- criar tag anotada `v0.1.0` somente após revisão humana;
- publicar checksums do artefato e indicar a arquitetura/ambiente usado.

O script não assina, publica, cria tag ou altera o repositório remoto.
