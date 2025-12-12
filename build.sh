#!/bin/bash
# Build script for NaoKernel with modular input/output and shell

mkdir -p bin

# Compile kernel assembly
echo "Compiling kernel assembly..."
nasm -f elf32 kernel.asm -o bin/kasm.o

# Compile output subsystem
echo "Compiling output subsystem..."
gcc -fno-stack-protector -m32 -c output/output.c -o bin/output.o

# Compile input subsystem
echo "Compiling input subsystem..."
gcc -fno-stack-protector -m32 -c input/input.c -o bin/input.o

# Compile shell
echo "Compiling shell..."
gcc -fno-stack-protector -m32 -c shell/shell.c -o bin/shell.o

# Compile kernel
echo "Compiling kernel..."
gcc -fno-stack-protector -m32 -c kernel.c -o bin/kc.o

echo "Compiling FS subsystem..."
gcc -fno-stack-protector -m32 -c fs/fs.c -o bin/fs.o

# Link everything together
echo "Linking kernel..."
ld -m elf_i386 -T link.ld -o bin/kernel bin/kasm.o bin/kc.o bin/output.o bin/input.o bin/shell.o bin/fs.o
# Run in QEMU if --run argument is provided
if [[ "$1" == "--run" ]]; then
    echo "Starting NaoKernel..."
    qemu-system-i386 -kernel bin/kernel
fi

