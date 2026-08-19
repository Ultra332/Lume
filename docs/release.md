# Processo de release Windows

Os artefatos são produzidos localmente e anexados manualmente à GitHub Release.
O processo não publica, não cria tag e não acessa serviços externos.

## Ferramentas de empacotamento

- MSYS2 UCRT64 com GCC, Make, `windres`, `objdump`, `bsdtar` e `sha256sum`;
- Inno Setup 6 para gerar o instalador.

O Inno Setup deve ser instalado conscientemente pelo mantenedor a partir de sua
fonte oficial. O script não baixa nem instala ferramentas. Se `ISCC.exe` não
estiver nos locais padrão, defina `ISCC` com seu caminho.

## Gerar a distribuição

No terminal MSYS2 UCRT64, na raiz:

```sh
sh scripts/build-release-windows.sh
```

O script limpa o build, compila, executa as 16 suítes, gera metadata do
executável, faz link estático dos runtimes do GCC, audita DLLs, monta o ZIP,
tenta compilar o instalador e produz `SHA256SUMS.txt`.

Saída esperada:

```text
dist/
├── Lume-0.1.1-Windows-x64-Setup.exe
├── Lume-0.1.1-Windows-x64.zip
└── SHA256SUMS.txt
```

Sem Inno Setup, somente o ZIP e seu checksum são gerados. Isso não deve ser
descrito como uma release completa.

## Decisões do instalador

O arquivo `installer/lume.iss` usa instalação por usuário em
`%LOCALAPPDATA%\Programs\Lume`, sem elevação obrigatória. Ele registra um
desinstalador normal no Windows e adiciona somente `{app}` ao `PATH` do usuário.
Na desinstalação, filtra apenas essa entrada e preserva as demais.

O instalador inclui `lume.exe` e `LICENSE`; não inclui fontes, compilador ou
ferramentas de desenvolvimento. Não cria associação `.lume`, atalho de desktop,
configuração do VS Code ou download durante a instalação.

Se `assets/lume.ico` existir, o script o incorpora no executável e o Inno Setup
o usa no instalador. Sem um ícone oficial, ambos continuam sendo gerados com o
ícone padrão.

## Checklist manual

Antes da instalação, execute `where lume` e registre qualquer instalação
anterior. Depois:

1. execute o Setup e abra um terminal novo;
2. confirme `where lume` e `lume --versao`;
3. crie `teste.lume` com `escreva("Olá, Lume!")` e execute `lume teste.lume`;
4. execute `lume novo meu_projeto`, entre na pasta e execute `lume`;
5. confirme o comando em PowerShell, CMD e terminal integrado do VS Code;
6. desinstale em Configurações → Aplicativos → Aplicativos instalados;
7. abra outro terminal e confirme que a entrada instalada sumiu de `where lume`.

Teste também o ZIP após extraí-lo em uma pasta fora do repositório. Confira os
dois arquivos com `SHA256SUMS.txt` antes de publicar.

## Publicação manual

- revisar versão, README, CHANGELOG, LICENSE, SECURITY e contatos humanos;
- validar instalação e desinstalação em uma máquina limpa;
- anexar `Lume-0.1.1-Windows-x64-Setup.exe`;
- anexar `Lume-0.1.1-Windows-x64.zip`;
- anexar `SHA256SUMS.txt`;
- informar Windows x64, diretório de instalação e dependências de runtime;
- criar tag somente após revisão humana.

Sem assinatura de código, o SmartScreen pode avisar que o editor é desconhecido.
Não contorne nem desabilite essa proteção. Assinatura é uma melhoria futura.
