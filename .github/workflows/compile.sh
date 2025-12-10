#!/bin/bash
mkdir -p bin

echo "Compiling kernel assembly..."
nasm -f elf32 kernel.asm -o bin/kasm.o

echo "Compiling output subsystem..."
gcc -fno-stack-protector -m32 -c output/output.c -o bin/output.o

echo "Compiling input subsystem..."
gcc -fno-stack-protector -m32 -c input/input.c -o bin/input.o

echo "Compiling shell..."
gcc -fno-stack-protector -m32 -c shell/shell.c -o bin/shell.o

echo "Compiling kernel..."
gcc -fno-stack-protector -m32 -c kernel.c -o bin/kc.o

echo "Linking kernel..."
ld -m elf_i386 -T link.ld -o bin/kernel bin/kasm.o bin/kc.o bin/output.o bin/input.o bin/shell.o