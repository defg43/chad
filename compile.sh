#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

cd "$SCRIPT_DIR"

# -Wno-psabi: gcc notes that passing a union with a long double changed in gcc 4.4, which is of no use here
gcc -c src/*.c -Iinclude -std=c23 -O2 -Wall -Wno-psabi
ar rcs libchad.a *.o

rm *.o

echo "✓ libchad.a created"
