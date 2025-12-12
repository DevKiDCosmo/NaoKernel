# Plan: Custom Bootloader und OS-Entwicklung

Entwickle einen mehrstufigen Bootloader für NaoKernel, der von echter Hardware bootet, und erweitere das System um essenzielle OS-Features wie Filesystem, Memory Management und Prozessverwaltung.

## Steps

1. **Zweistufigen Bootloader erstellen** – Implementiere Stage 1 MBR Bootloader (512 Bytes) in Assembly, der Stage 2 von Disk lädt, und Stage 2 Bootloader mit BIOS INT 0x13 Disk I/O, A20-Gate Aktivierung und Protected Mode Switch zum Laden des Kernels

2. **FAT12 Filesystem Reader implementieren** – Baue FAT12-Parser im Stage 2 Bootloader, um `KERNEL.BIN` von Disk zu lesen, und implementiere die in FILESYSTEM.md geplante Ramdisk-Funktionalität mit Shell-Befehlen (`ls`, `cat`, `mkdir`, `rm`)

3. **Memory Management aufbauen** – Implementiere Paging für virtuellen Speicher, Heap-Allocator (`malloc`/`free`) ab 0x160000, und Physical Memory Manager für Speicherblöcke

4. **Prozess- und Task-Management** – Entwickle Prozessstrukturen mit PID/PCB, kooperatives Multitasking mit Timer (PIT IRQ0), und Context Switching zwischen Tasks

5. **Bootable Disk Image erstellen** – Baue `.img`/`.iso` mit Bootloader + FAT12 Partition + Kernel, teste auf echter Hardware oder USB-Boot, und erweitere build.sh um Image-Generierung

## Further Considerations

1. **Bootloader-Architektur** – Stage 1 nur im MBR (512 Bytes) oder zusätzliche Sektoren nutzen? Legacy BIOS oder UEFI-Unterstützung planen? Multiboot-Header beibehalten für GRUB-Kompatibilität?

2. **Filesystem-Wahl** – FAT12 (wie in FILESYSTEM.md geplant) für Einfachheit, oder moderneres ext2/FAT32 für größere Partitionen und mehr Features?

3. **Memory Management Strategie** – Simple Bitmap-Allocator für MVP oder ausgefeiltes Buddy System? Paging sofort oder erst später nach Basic Heap?

## Aktuelle Projektübersicht

### Implementierte Features
- **Kernel**: IDT-Setup, Keyboard Interrupts, 8 KB Stack
- **Output**: VGA Text Mode (80×25), Cursor-Management, farbige Ausgabe
- **Input**: Vollständige Tastatur mit Scancode-Mapping, Line-Buffering, Command History (100 Befehle)
- **Shell**: 7 Befehle (`help`, `clear`, `echo`, `about`, `exit`, `test`, `history`), Case-insensitive

### Was fehlt
- ❌ **Filesystem** – Keine Disk I/O, keine FAT/ext2
- ❌ **Memory Management** – Kein Heap, kein malloc(), keine Paging
- ❌ **Prozesse** – Keine Multitasking, keine Prozessverwaltung
- ❌ **Hardware-Shutdown** – exit macht nur HLT, kein ACPI
- ❌ **Timer** – Keine PIT/APIC Timer
- ❌ **Eigener Bootloader** – Nutzt aktuell QEMU's -kernel Flag

### Memory Layout
```
0x00000000 - 0x000FFFFF: BIOS/Real Mode (1 MB)
0x000B8000 - 0x000B8FA0: VGA Text Memory (4 KB)
0x00100000 - 0x0010XXXX: Kernel Code (.text, .data, .bss)
0x0010XXXX - 0x0012XXXX: 8 KB Stack (wächst nach unten)
0x00120000 - 0x00160000: GEPLANT: FAT12 Ramdisk (256 KB)
0x00160000+:              GEPLANT: Heap für malloc()
```

## Implementation Order

### Phase 1: Bootloader Foundation
1. Stage 1 Bootloader (bootsector.asm) - 512 Bytes MBR
2. Stage 2 Bootloader (bootloader.asm) - Disk I/O & Kernel Loading
3. A20 Gate & Protected Mode Transition
4. FAT12 Reader für Kernel-Laden

### Phase 2: Filesystem Support
1. Disk-Treiber (ATA PIO Mode)
2. FAT12 Ramdisk Implementation (wie in FILESYSTEM.md)
3. VFS-Layer für Datei-Operationen
4. Shell-Befehle: ls, cat, mkdir, rm, touch

### Phase 3: Memory Management
1. Physical Memory Manager (Bitmap/Stack-Allocator)
2. Paging Setup (Identity Mapping + Kernel Space)
3. Heap Implementation (malloc/free)
4. Virtual Memory Manager

### Phase 4: Multitasking
1. PIT Timer Setup (IRQ0)
2. Process Control Block (PCB) Strukturen
3. Scheduler (Round-Robin)
4. Context Switching
5. Syscall Interface

### Phase 5: Hardware & Polish
1. ACPI Shutdown Support
2. RTC/CMOS Zeit
3. Bootable Disk Image (.img/.iso)
4. USB-Boot Testing
