#!/bin/bash
# Build script for NaoKernel with integrated nano shell

mkdir -p bin
mkdir -p compile

# Compile kernel
echo "Compiling kernel..."
nasm -f elf32 kernel.asm -o bin/kasm.o
gcc -fno-stack-protector -m32 -c kernel.c -o bin/kc.o

# Compile shell
echo "Compiling shell..."
gcc -fno-stack-protector -m32 -c shell/shell.c -o bin/shell.o

# Link kernel with shell
echo "Linking kernel..."
ld -m elf_i386 -T link.ld -o bin/kernel bin/kasm.o bin/kc.o bin/shell.o

# Run in QEMU
echo "Starting NaoKernel with integrated nano shell..."
qemu-system-i386 -kernel bin/kernel