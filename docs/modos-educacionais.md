# Modos educacionais

Os três modos respondem perguntas diferentes:

| Comando | Executa? | Ajuda a entender |
|---|---:|---|
| `lume --analisar arquivo.lume` | não | estrutura e problemas detectáveis antes da execução |
| `lume --explicar arquivo.lume` | sim | acontecimentos observados durante a execução |
| `lume --passo arquivo.lume` | sim, com pausas | sequência e estado de cada etapa |

Comece com `--analisar` para revisar o código sem efeitos colaterais. Use `--explicar` para relacionar o texto do programa ao comportamento. Use `--passo` em exemplos pequenos; em programas longos, as pausas podem produzir muito conteúdo.

Avisos da análise são educacionais e não bloqueiam o comando normal. Nenhum modo atribui nota ou tenta adivinhar a intenção de quem escreveu o programa.

