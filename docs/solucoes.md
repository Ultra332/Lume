# Soluções de referência

Estas soluções correspondem aos exercícios de [algoritmos.md](algoritmos.md). Tente primeiro e teste casos pequenos, vazios e limites.

```lume
// 1. Conta pares
funcao contar_pares(numeros) {
    variavel total = 0
    para i de 0 ate tamanho(numeros) - 1 {
        se numeros[i] % 2 == 0 { total = total + 1 }
    }
    retorne total
}
```

Os exercícios 2–5 foram deixados sem solução completa nesta candidata para favorecer uso em aula. Contribuições podem adicioná-las com explicação passo a passo e testes de casos-limite.
