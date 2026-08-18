#!/usr/bin/env sh
set -eu

VERSION="0.1.0"
SCRIPT_DIR=$(CDPATH= cd -- "$(dirname -- "$0")" && pwd)
cd "$SCRIPT_DIR/.."
DIST="dist"
BUILD="build"
STAGE="$BUILD/windows-package"
ZIP_NAME="Lume-$VERSION-Windows-x64.zip"
SETUP_NAME="Lume-$VERSION-Windows-x64-Setup.exe"
RELEASE_CFLAGS="-std=c11 -Wall -Wextra -Wpedantic -Wconversion -Wshadow -O2"

make clean
make
make test

rm -rf "$BUILD" "$DIST"
mkdir -p "$STAGE" "$DIST"
cp installer/lume-version.rc "$BUILD/lume-version.rc"
if [ -f assets/lume.ico ]; then
    printf '\nIDI_LUME ICON "../assets/lume.ico"\n' >> "$BUILD/lume-version.rc"
fi

if ! command -v windres >/dev/null 2>&1; then
    echo "Erro: windres do MinGW-w64 nao foi encontrado." >&2
    exit 1
fi
windres --codepage=65001 "$BUILD/lume-version.rc" -O coff -o "$BUILD/lume-version.o"

make clean
make TARGET=lume.exe CFLAGS="$RELEASE_CFLAGS" \
    LDFLAGS="$BUILD/lume-version.o -static -static-libgcc"

if [ ! -f lume.exe ]; then
    echo "Erro: lume.exe nao foi gerado." >&2
    exit 1
fi

cp lume.exe "$STAGE/lume.exe"
cp LICENSE "$STAGE/LICENSE"
cp packaging/README-Windows.txt "$STAGE/README.txt"

if command -v objdump >/dev/null 2>&1; then
    objdump -p "$STAGE/lume.exe" | sed -n 's/^[[:space:]]*DLL Name: /DLL: /p' > "$BUILD/windows-runtime-dependencies.txt"
    if grep -Ei 'libgcc|libstdc\+\+|libwinpthread|msys-[0-9]' "$BUILD/windows-runtime-dependencies.txt" >/dev/null; then
        echo "Erro: lume.exe depende de uma DLL de desenvolvimento nao empacotada." >&2
        cat "$BUILD/windows-runtime-dependencies.txt" >&2
        exit 1
    fi
else
    echo "Erro: objdump nao foi encontrado para auditar as dependencias." >&2
    exit 1
fi

if command -v bsdtar >/dev/null 2>&1; then
    bsdtar -C "$STAGE" -a -cf "$DIST/$ZIP_NAME" lume.exe README.txt LICENSE
else
    echo "Erro: bsdtar nao foi encontrado para criar o ZIP portatil." >&2
    exit 1
fi

ISCC_PATH="${ISCC:-}"
if [ -z "$ISCC_PATH" ] && command -v iscc >/dev/null 2>&1; then ISCC_PATH=$(command -v iscc); fi
if [ -z "$ISCC_PATH" ] && [ -x "/c/Program Files (x86)/Inno Setup 6/ISCC.exe" ]; then ISCC_PATH="/c/Program Files (x86)/Inno Setup 6/ISCC.exe"; fi
if [ -z "$ISCC_PATH" ] && [ -x "/c/Program Files/Inno Setup 6/ISCC.exe" ]; then ISCC_PATH="/c/Program Files/Inno Setup 6/ISCC.exe"; fi
if [ -n "$ISCC_PATH" ]; then
    "$ISCC_PATH" installer/lume.iss
else
    echo "Aviso: Inno Setup 6 nao encontrado; o instalador nao foi gerado." >&2
fi

(cd "$DIST" && sha256sum --text "$ZIP_NAME" > SHA256SUMS.txt)
if [ -f "$DIST/$SETUP_NAME" ]; then
    (cd "$DIST" && sha256sum --text "$SETUP_NAME" >> SHA256SUMS.txt)
fi

"$STAGE/lume.exe" --versao
echo "Pacote portatil: dist/$ZIP_NAME"
if [ -f "$DIST/$SETUP_NAME" ]; then echo "Instalador: dist/$SETUP_NAME"; fi
echo "Checksums: dist/SHA256SUMS.txt"
