#!/usr/bin/env bash

set -e

if [ -n "$BASH_VERSION" ]; then
    RC_FILE="$HOME/.zshrc"
elif [ -n "$ZSH_VERSION" ]; then
    RC_FILE="$HOME/.bashrc"
else
    echo "Unsupported shell. Please use bash or zsh."
    exit 0
fi

touch "$RC_FILE"

if grep -qE '^\s*export\s+CUTECORE=' "$RC_FILE"; then
    echo "CUTECORE is already set in $RC_FILE. Skipping initialization."
    exit 0
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

{
    echo ""
    echo "# CUTECORE environment variable (auto added)"
    echo "export CUTECORE=\"$SCRIPT_DIR\""
} >> "$RC_FILE"

export CUTECORE="$SCRIPT_DIR"

