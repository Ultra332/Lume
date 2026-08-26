# Contribuindo com a Lume

Obrigado pelo interesse. A Lume prioriza clareza pedagógica, diagnósticos em português, comportamento determinístico e uma implementação C pequena e legível.

## Antes de começar

- Para correções pequenas, abra uma issue ou envie diretamente um pull request bem explicado.
- Para mudanças de sintaxe, semântica ou arquitetura, abra primeiro uma discussão/issue com exemplos e impacto educacional.
- Não introduza recursos incompatíveis silenciosamente nem reduza validações para fazer testes passarem.

## Preparando o ambiente

Use GCC e Make. No Windows, use preferencialmente o terminal MSYS2 UCRT64.

```sh
make clean
make
make test
```

O código deve continuar compatível com C11 e sem warnings sob `-Wall -Wextra -Wpedantic -Wconversion -Wshadow`. Execute `make sanitize` quando o ambiente disponibilizar ASan e UBSan.

## Mudanças e testes

1. Mantenha cada mudança focada.
2. Adicione ou atualize testes para todo comportamento observável alterado.
3. Preserve testes de regressão; nunca os enfraqueça para ocultar um bug.
4. Atualize documentação quando comandos, arquitetura ou linguagem mudarem.
5. Execute toda a suíte antes de enviar.

Commits curtos e descritivos em português ou inglês são bem-vindos. Um pull request deve explicar o problema, a solução, como foi validada e quaisquer limitações.

## Critérios de revisão

Revisões consideram correção, segurança de memória, mensagens educacionais, compatibilidade, cobertura de testes e simplicidade. Contribuições devem seguir o [Código de Conduta](CODE_OF_CONDUCT.md). Vulnerabilidades não devem ser expostas em issues públicas; siga [SECURITY.md](SECURITY.md).

