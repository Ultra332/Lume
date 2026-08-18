# Desenvolvendo a Lume

Este fluxo é destinado a contribuidores, não a usuários da distribuição
binária.

No Windows, instale MSYS2, abra o terminal UCRT64 e instale GCC e Make. Depois de
clonar o repositório:

```sh
make clean
make
make test
```

Mudanças devem permanecer compatíveis com C11 e sem warnings sob `-Wall
-Wextra -Wpedantic -Wconversion -Wshadow`. Consulte [CONTRIBUTING.md](../CONTRIBUTING.md)
para critérios completos e [release.md](release.md) somente se estiver preparando
artefatos oficiais.
