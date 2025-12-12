#!/bin/bash

# Check if --clear flag is provided
if [[ "$1" == "--clear" ]]; then
    echo "Clearing previous run files..."
    rm -rf run
fi

mkdir run
cd run

dd if=/dev/zero of=disk.img bs=1M count=10
qemu-img create -f raw disk.img 10M

mkfs.fat -F 16 disk.img

# For floppy disk image
dd if=/dev/zero of=floppy.img bs=512 count=2880
mkfs.fat -F 12 floppy.img

cd ..

echo "Starting NaoKernel with mounted disk image..."

qemu-system-i386 -kernel bin/kernel -hda run/disk.img -fda run/floppy.img # -m 512M -boot c -serial stdio

# Small doc
# -hda specifies the hard disk image to use
# -fda would specify a floppy disk image (-fdb, -hdc, -hdd for additional drives)