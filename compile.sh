#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"

gcc -c src/*.c -Iinclude -std=c23 -O2 -Wall
ar rcs libchad.a *.o

rm *.o

echo "✓ libchad.a created"
