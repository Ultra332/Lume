# Identidade visual da Lume

Arquivos oficiais:

- `lume.png`: imagem mestre em alta resolução e fundo transparente;
- `lume.ico`: ícone Windows multirresolução.

O ícone representa uma chama luminosa formando sutilmente a letra “L”, com
azul profundo e tons quentes de amarelo e laranja. A composição foi criada para
continuar reconhecível em tamanhos pequenos.

O `.ico` contém quadros de 16, 24, 32, 48, 64, 128 e 256 pixels. O pipeline
`scripts/build-release-windows.sh` incorpora esse arquivo em `lume.exe`, e o
Inno Setup o utiliza no instalador.

Não substitua esses arquivos por conversões de baixa resolução. Alterações na
identidade visual devem preservar uma imagem mestre e regenerar todas as
resoluções do `.ico`.
