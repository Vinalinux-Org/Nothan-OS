#!/bin/bash
# install_compiler.sh — install vincc (VinixOS C Compiler) system-wide or user-local.
# Usage: bash scripts/install_compiler.sh [run [args...]]

set -e

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
SRC_COMPILER_DIR="$ROOT_DIR/compiler/toolchain"

if [[ ! -d "$SRC_COMPILER_DIR" ]]; then
    echo -e "${RED}error: compiler source not found at $SRC_COMPILER_DIR${NC}"
    exit 1
fi

# --run: execute directly from source without installing
if [[ "${1:-}" == "run" || "${1:-}" == "--run" ]]; then
    shift
    cd "$ROOT_DIR/compiler"
    exec python3 -m toolchain.main "$@"
fi

if [[ $EUID -eq 0 ]]; then
    INSTALL_DIR="/usr/local/bin"
    COMPILER_DIR="/usr/local/lib/vincc"
else
    INSTALL_DIR="$HOME/.local/bin"
    COMPILER_DIR="$HOME/.local/lib/vincc"
fi

mkdir -p "$INSTALL_DIR" "$COMPILER_DIR"

echo -e "${GREEN}Installing vincc to $INSTALL_DIR...${NC}"

if ! command -v python3 &>/dev/null; then
    echo -e "${RED}error: python3 not found${NC}"
    exit 1
fi

if ! command -v arm-linux-gnueabihf-gcc &>/dev/null; then
    echo -e "${YELLOW}warning: arm-linux-gnueabihf-gcc not found${NC}"
    echo "  install: sudo apt-get install gcc-arm-linux-gnueabihf binutils-arm-linux-gnueabihf"
fi

rm -rf "$COMPILER_DIR/toolchain"
cp -a "$SRC_COMPILER_DIR" "$COMPILER_DIR/"

cat > "$INSTALL_DIR/vincc" <<EOF
#!/bin/bash
COMPILER_DIR="$COMPILER_DIR"
if [ ! -d "\$COMPILER_DIR/toolchain" ]; then
    echo "error: vincc package not found at \$COMPILER_DIR/toolchain"
    exit 1
fi
export PYTHONPATH="\$COMPILER_DIR:\$PYTHONPATH"
exec python3 -m toolchain.main "\$@"
EOF
chmod +x "$INSTALL_DIR/vincc"

if [[ ":$PATH:" != *":$INSTALL_DIR:"* && $EUID -ne 0 ]]; then
    if ! grep -Fq "export PATH=\"\$PATH:$INSTALL_DIR\"" ~/.bashrc 2>/dev/null; then
        echo "export PATH=\"\$PATH:$INSTALL_DIR\"" >> ~/.bashrc
        echo -e "${YELLOW}added $INSTALL_DIR to ~/.bashrc — source it or restart terminal${NC}"
    fi
fi

echo -e "${GREEN}done.${NC}"
echo "  vincc --version"
echo "  vincc hello.c -o hello"
