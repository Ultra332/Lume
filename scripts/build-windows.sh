#!/usr/bin/env sh
set -eu

make clean
make
make test

artifact="lume.exe"
if [ ! -f "$artifact" ]; then
    artifact="lume"
fi
if [ ! -f "$artifact" ]; then
    echo "Erro: executável da Lume não foi gerado." >&2
    exit 1
fi

mkdir -p dist
cp "$artifact" dist/lume.exe
./dist/lume.exe --versao
./dist/lume.exe --ajuda >/dev/null
echo "Release Windows criada em dist/lume.exe"

