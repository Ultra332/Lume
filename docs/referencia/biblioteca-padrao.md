# Biblioteca padrão da Lume 0.4

Os módulos oficiais usam o namespace reservado `lume/*`, acompanham a versão instalada e não aparecem em `lume.lock`.

## `lume/matematica`

```lume
importe "lume/matematica" como mat
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `absoluto(numero)` | módulo de um número: `mat.absoluto(-4)` | exige número; `INT64_MIN` inteiro ultrapassa o limite |
| `minimo(a, b)` | menor valor: `mat.minimo(3, 8)` | exige dois números |
| `maximo(a, b)` | maior valor: `mat.maximo(3, 8)` | exige dois números |
| `potencia(base, expoente)` | potência decimal: `mat.potencia(2, 3)` | rejeita tipos e resultado não representável |
| `raiz(numero)` | raiz quadrada: `mat.raiz(81)` | número negativo não possui raiz real |
| `piso(numero)` | arredonda para baixo: `mat.piso(2.8)` | exige número |
| `teto(numero)` | arredonda para cima: `mat.teto(2.2)` | exige número |
| `arredonde(numero)` | arredonda metades para longe de zero: `mat.arredonde(2.5)` | exige número |
| `seno(radianos)` | seno: `mat.seno(mat.PI / 2)` | exige ângulo numérico em radianos |
| `cosseno(radianos)` | cosseno: `mat.cosseno(0)` | exige ângulo numérico em radianos |
| `tangente(radianos)` | tangente: `mat.tangente(0)` | rejeita tipo ou resultado não representável |
| `graus(radianos)` | converte para graus: `mat.graus(mat.PI)` | exige número |
| `radianos(graus)` | converte para radianos: `mat.radianos(180)` | exige número |

`PI` e `E` são constantes decimais. Operações com dois inteiros preservam inteiro quando documentado; as demais retornam decimal.

## `lume/texto`

```lume
importe "lume/texto" como txt
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `maiusculo(texto)` | converte ASCII: `txt.maiusculo("Lume")` | exige texto; bytes UTF-8 não ASCII são preservados |
| `minusculo(texto)` | converte ASCII: `txt.minusculo("LUME")` | exige texto |
| `contem(texto, trecho)` | testa ocorrência: `txt.contem("Lume", "um")` | exige dois textos |
| `comeca_com(texto, trecho)` | testa prefixo: `txt.comeca_com("Lume", "Lu")` | exige dois textos |
| `termina_com(texto, trecho)` | testa sufixo: `txt.termina_com("Lume", "me")` | exige dois textos |
| `aparar(texto)` | remove espaços ASCII nas pontas: `txt.aparar(" oi ")` | exige texto |
| `subtexto(texto, inicio, fim)` | recorta por pontos de código: `txt.subtexto("Lume", 0, 2)` | índices inteiros válidos; fim é exclusivo |
| `substitua(texto, busca, novo)` | troca ocorrências: `txt.substitua("banana", "a", "o")` | exige três textos; busca vazia é erro |
| `separe(texto, separador)` | cria lista: `txt.separe("a,b", ",")` | separador deve ser texto não vazio |
| `junte(lista, separador)` | une textos: `txt.junte(["a", "b"], ",")` | exige lista só de textos e separador textual |

Não há normalização Unicode, locale ou conversão implícita.

## `lume/arquivo`

```lume
importe "lume/arquivo" como arquivo
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `existe(caminho)` | testa existência: `arquivo.existe("dados.txt")` | caminho deve ser texto |
| `leia(caminho)` | lê conteúdo: `arquivo.leia("dados.txt")` | caminho, permissão ou leitura inválidos |
| `escreva(caminho, texto)` | sobrescreve: `arquivo.escreva("dados.txt", "oi")` | exige dois textos e permissão de escrita |
| `adicione(caminho, texto)` | acrescenta: `arquivo.adicione("log.txt", "linha\n")` | exige dois textos e permissão de escrita |
| `remova(caminho)` | remove: `arquivo.remova("temporario.txt")` | arquivo inexistente ou sem permissão |

Em projetos, caminhos relativos partem da raiz; em scripts, do diretório do script principal. A API acessa o filesystem diretamente e não usa shell ou sandbox.

## `lume/tempo`

```lume
importe "lume/tempo" como tempo
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `timestamp()` | segundos Unix: `tempo.timestamp()` | falha do relógio do sistema |
| `agora()` | hora local `AAAA-MM-DD HH:MM:SS`: `tempo.agora()` | falha ao consultar ou converter a hora |
| `durma(ms)` | suspende sem busy-wait: `tempo.durma(180)` | inteiro entre 0 e 86.400.000 |

## `lume/aleatorio`

```lume
importe "lume/aleatorio" como aleatorio
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `inteiro(minimo, maximo)` | intervalo inclusivo: `aleatorio.inteiro(1, 6)` | limites inteiros e mínimo não maior que máximo |
| `decimal()` | valor em `[0.0, 1.0)`: `aleatorio.decimal()` | não recebe argumentos |
| `escolha(lista)` | escolhe elemento: `aleatorio.escolha(["a", "b"])` | exige lista não vazia |

O gerador é adequado a exercícios e jogos, não a criptografia.

## `lume/terminal`

```lume
importe "lume/terminal" como terminal
```

| Assinatura | Descrição e exemplo | Erros relevantes |
| --- | --- | --- |
| `limpe()` | limpa e volta ao início: `terminal.limpe()` | saída indisponível |
| `posicione(coluna, linha)` | move o cursor: `terminal.posicione(1, 1)` | inteiros positivos; coordenadas começam em 1 |
| `oculte_cursor()` | oculta: `terminal.oculte_cursor()` | terminal sem suporte ANSI/VT |
| `mostre_cursor()` | mostra: `terminal.mostre_cursor()` | terminal sem suporte ANSI/VT |
| `tamanho()` | retorna `[colunas, linhas]`: `terminal.tamanho()` | em saída não interativa usa `[80, 24]` |
| `leia_tecla()` | espera uma tecla: `terminal.leia_tecla()` | requer terminal interativo compatível |
| `tecla()` | lê sem bloquear: `terminal.tecla()` | retorna `nulo` sem tecla; requer terminal compatível |
| `cor_texto(cor)` | altera texto: `terminal.cor_texto("verde")` | exige nome de cor válido |
| `cor_fundo(cor)` | altera fundo: `terminal.cor_fundo("azul")` | exige nome de cor válido |
| `resetar_cor()` | restaura cores: `terminal.resetar_cor()` | saída indisponível |
| `estilize(texto, cor_texto, cor_fundo)` | cria texto autocontido: `terminal.estilize("OK", "branco", "verde")` | texto e duas cores válidas |

Cores: `preto`, `vermelho`, `verde`, `amarelo`, `azul`, `magenta`, `ciano`, `branco`, suas variantes `_claro`, além de `cinza` e `padrao`. A biblioteca restaura cores e cursor ao encerrar a sessão.

## Funções nativas relacionadas

`leia()` lê uma linha. `leia("Convite: ")` mostra o convite sem quebra de linha. O convite deve ser texto. Conversões são explícitas com `inteiro` e `decimal`.

Listas não possuem um módulo `lume/listas` nesta versão. As operações básicas são as nativas `tamanho(lista)`, `adicione(lista, valor)` e `remova(lista, indice)`, descritas na [referência da linguagem](linguagem.md).
