# Instalação da Lume no Windows

## Para usar a Lume

Baixe na página de releases do repositório oficial:

- `Lume-0.1.1-Windows-x64-Setup.exe` — opção recomendada;
- `Lume-0.1.1-Windows-x64.zip` — opção portátil;
- `SHA256SUMS.txt` — verificação de integridade.

### Instalador

Execute o Setup e avance pelas telas. A instalação usa
`%LOCALAPPDATA%\Programs\Lume`, não exige administrador e adiciona essa pasta ao
`PATH` do usuário sem substituir as entradas existentes. Abra um terminal novo:

```powershell
lume --versao
```

O resultado esperado é `Lume 0.1.1`. PowerShell, CMD e o terminal integrado do
VS Code reconhecem o mesmo `PATH`; terminais que já estavam abertos precisam ser
reiniciados.

#### Se `lume` não funcionar no terminal do VS Code

O VS Code herda o `PATH` existente no momento em que é aberto. Feche todas as
janelas do VS Code, confirme que não há `Code.exe` no Gerenciador de Tarefas e
abra-o novamente. Depois execute:

```powershell
Get-Command lume
lume --versao
```

Para confirmar a instalação diretamente:

```powershell
& "$env:LOCALAPPDATA\Programs\Lume\lume.exe" --versao
```

Se o comando direto funcionar, mas `Get-Command lume` não encontrar nada,
confira a entrada no `PATH` do usuário:

```powershell
[Environment]::GetEnvironmentVariable("Path", "User") -split ";"
```

Ela deve conter `%LOCALAPPDATA%\Programs\Lume`. Não substitua o conteúdo inteiro
do `PATH` para corrigir uma entrada ausente.

Para remover, use Configurações → Aplicativos → Aplicativos instalados → Lume →
Desinstalar. O desinstalador remove somente a pasta da Lume do `PATH` do usuário.

### Pacote portátil

Extraia todo o ZIP. Não mova somente `lume.exe`, pois arquivos adicionais do
pacote contêm licença e instruções. Na pasta extraída:

```powershell
.\lume.exe --versao
.\lume.exe programa.lume
```

O modo portátil não altera o `PATH`. Essa alteração, caso desejada, é manual.

### Verificar o checksum

No PowerShell, compare o resultado com `SHA256SUMS.txt`:

```powershell
Get-FileHash .\Lume-0.1.1-Windows-x64-Setup.exe -Algorithm SHA256
Get-FileHash .\Lume-0.1.1-Windows-x64.zip -Algorithm SHA256
```

### SmartScreen

A primeira release pode não possuir assinatura de código. Nesse caso, o Windows
SmartScreen pode exibir um aviso de editor desconhecido. Verifique se o download
veio do repositório oficial e compare o checksum. Não desabilite o SmartScreen.

## Para desenvolver a Lume

Usuários comuns não precisam de GCC, Make, MSYS2 ou código-fonte. Essas
ferramentas são necessárias somente para modificar a implementação. Consulte
[desenvolvimento.md](desenvolvimento.md) e [CONTRIBUTING.md](../CONTRIBUTING.md).

Depois de instalar, continue pelo [guia de uso](guia-de-uso.md).
