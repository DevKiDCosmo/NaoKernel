# Plan: Filesystem Implementation for NaoKernel
# PLAN: Memory Allocation in RAM

Add to shell command `disk list` to list all avaible disk
For activating a disk or medium run `disk activate {device}`

## Executive Summary

Implement a **FAT12 ramdisk filesystem** with essential file operations (create, read, delete, list). Store filesystem in kernel memory starting at 0x120000 with 512 byte blocks, supporting up to 256 KB of user files. Add 6 shell commands: `ls`, `cd`, `pwd`, `cat`, `mkdir`, `touch`, `rm`.

---

## Current State Analysis

### Existing Architecture
- **Kernel size:** ~15-20 KB (kernel.c, shell, input, output)
- **Memory layout:** Kernel at 0x100000, VGA text at 0xB8000, 8 KB stack
- **Shell:** 7 commands with working input/output, command-line parsing, 1000-line history
- **Integration ready:** Command dispatcher (command_map array), argument parsing, output formatting

### What Filesystem Needs to Integrate With
- **Shell command loop** (nano_shell() in shell.c) - Add 7 filesystem commands to command_map
- **Current working directory** - Add `cwd` state to shell struct
- **Memory management** - Use static allocation in .bss for ramdisk blocks
- **Error handling** - Return error codes from filesystem operations

---

## Recommended Approach: FAT12 Ramdisk

### Why FAT12 (not ext2, not custom)?
- **FAT12 is simple:** Direct sector access, minimal metadata, well-documented
- **Ramdisk avoids disk complexity:** No BIOS INT 0x13, no ATA controllers - just memory reads/writes
- **Fast:** Everything in RAM, no hardware latency
- **Learning value:** Same filesystem on floppy/USB, transferable skills
- **Practical size:** 256 KB ramdisk = 512 blocks of 512 bytes = ~250 KB user files

### Memory Layout
```
Physical Memory:
├── Kernel Code (0x100000 - 0x110000)
├── Kernel Data/BSS (0x110000 - 0x120000)
├── RAMDISK (0x120000 - 0x160000, 256 KB)
│   ├── Boot Sector (sector 0, 512 B) - unused in ramdisk
│   ├── FAT (sectors 1-2, 1 KB) - file allocation table
│   ├── Root Directory (sector 3, 512 B) - max 16 entries
│   └── Data Blocks (sectors 4-511, 256 KB) - user files
├── Heap (0x160000+) - future malloc
└── Stack (grows down from 0x180000)
```

### Key Constraints
- **Max files:** 16 in root directory (sector 3 holds 16 entries × 32 bytes)
- **Max file size:** 127.5 KB per file (255 clusters × 512 B) with FAT12
- **No subdirectories:** Start with flat root directory only
- **Block size:** 512 bytes (standard FAT12)
- **Cluster size:** 1 block (simplified, not multi-block clusters)

---

## Implementation Plan

### Phase 1: Filesystem Data Structures

**File:** `fs/fs.h`

```c
#ifndef FS_H
#define FS_H

#include <stdint.h>

#define BLOCK_SIZE 512
#define RAMDISK_SIZE (256 * 1024)  /* 256 KB */
#define MAX_BLOCKS (RAMDISK_SIZE / BLOCK_SIZE)  /* 512 blocks */
#define FAT_BLOCKS 2
#define ROOT_DIR_BLOCK 2
#define MAX_ROOT_ENTRIES 16
#define DATA_BLOCK_START 3

/* File attributes (standard DOS) */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_LABEL 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

/* Root directory entry (32 bytes) */
typedef struct {
    char name[8];           /* 8.3 format: FILENAME */
    char ext[3];            /* .EXT */
    uint8_t attributes;     /* File attributes */
    uint8_t reserved;
    uint8_t creation_time_10ms;
    uint16_t creation_time; /* Hours, minutes, seconds */
    uint16_t creation_date; /* Year, month, day */
    uint16_t last_access;   /* Date only */
    uint16_t high_cluster;  /* High word of cluster (0 for FAT12) */
    uint16_t write_time;    /* Last write time */
    uint16_t write_date;    /* Last write date */
    uint16_t start_cluster; /* Starting cluster number */
    uint32_t file_size;     /* File size in bytes */
} DirectoryEntry;

/* File handle for reading/writing */
typedef struct {
    DirectoryEntry *entry;
    uint32_t current_pos;   /* Current position in file */
    uint16_t current_cluster;
    int is_open;
} FileHandle;

/* Filesystem operations */
int fs_init(void);
int fs_format(void);  /* Create empty filesystem */

/* Directory operations */
int fs_list_root(DirectoryEntry **out_entries);  /* Return count, set *out_entries */
DirectoryEntry* fs_find_entry(const char *name);

/* File operations */
int fs_create_file(const char *name, uint32_t size);
int fs_delete_file(const char *name);
int fs_read_file(const char *name, uint8_t *buffer, uint32_t max_size);
int fs_write_file(const char *name, const uint8_t *data, uint32_t size);

/* Directory operations */
int fs_create_dir(const char *name);

/* Utilities */
void fs_format_name(const char *src, char *name_out, char *ext_out);
uint16_t fat_get_next_cluster(uint16_t cluster);
int fat_allocate_cluster(void);
void fat_free_cluster(uint16_t cluster);

#endif /* FS_H */
```

### Phase 2: Ramdisk Backend

**File:** `fs/ramdisk.h`

```c
#ifndef RAMDISK_H
#define RAMDISK_H

#include <stdint.h>

#define RAMDISK_BASE 0x120000

/* Initialize ramdisk in memory */
void ramdisk_init(void);

/* Read/write blocks */
int ramdisk_read(uint32_t block_num, uint8_t *buffer);
int ramdisk_write(uint32_t block_num, const uint8_t *data);

/* Get pointer to block data (low-level access) */
uint8_t* ramdisk_get_block(uint32_t block_num);

#endif /* RAMDISK_H */
```

**File:** `fs/ramdisk.c`

```c
#include "ramdisk.h"
#include "../output/output.h"

static uint8_t *ramdisk = (uint8_t*)RAMDISK_BASE;

/* Initialize ramdisk in memory */
void ramdisk_init(void)
{
    /* Clear ramdisk */
    for (uint32_t i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }
}

/* Read block from ramdisk */
int ramdisk_read(uint32_t block_num, uint8_t *buffer)
{
    if (block_num >= MAX_BLOCKS) {
        return -1;  /* Invalid block */
    }
    
    uint32_t offset = block_num * BLOCK_SIZE;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ramdisk[offset + i];
    }
    
    return BLOCK_SIZE;
}

/* Write block to ramdisk */
int ramdisk_write(uint32_t block_num, const uint8_t *data)
{
    if (block_num >= MAX_BLOCKS) {
        return -1;  /* Invalid block */
    }
    
    uint32_t offset = block_num * BLOCK_SIZE;
    for (int i = 0; i < BLOCK_SIZE; i++) {
        ramdisk[offset + i] = data[i];
    }
    
    return BLOCK_SIZE;
}

/* Get direct pointer to block */
uint8_t* ramdisk_get_block(uint32_t block_num)
{
    if (block_num >= MAX_BLOCKS) {
        return NULL;
    }
    
    return ramdisk + (block_num * BLOCK_SIZE);
}
```

### Phase 3: FAT12 Operations

**File:** `fs/fat.c`

```c
#include "fat.h"
#include "ramdisk.h"

/* FAT12 cluster chain following */
uint16_t fat_get_next_cluster(uint16_t cluster)
{
    uint8_t *fat = ramdisk_get_block(1);  /* FAT is at block 1 */
    
    if (cluster >= 0xFF0) {
        return 0xFFF;  /* End of chain */
    }
    
    /* FAT12: 3 bytes = 2 clusters */
    uint32_t fat_offset = (cluster * 3) / 2;
    uint16_t entry;
    
    if (cluster & 1) {
        /* Odd cluster - use upper 12 bits */
        entry = ((fat[fat_offset + 1] & 0x0F) << 8) | fat[fat_offset];
    } else {
        /* Even cluster - use lower 12 bits */
        entry = (fat[fat_offset + 1] << 4) | (fat[fat_offset] >> 4);
    }
    
    return entry & 0xFFF;
}

/* Allocate next free cluster */
int fat_allocate_cluster(void)
{
    uint8_t *fat = ramdisk_get_block(1);
    
    for (uint16_t cluster = 2; cluster < 0xFF0; cluster++) {
        uint16_t entry = fat_get_next_cluster(cluster);
        if (entry == 0x000) {
            /* Found free cluster - mark as end of chain */
            fat_set_cluster(cluster, 0xFFF);
            return cluster;
        }
    }
    
    return -1;  /* No free clusters */
}

/* Free cluster chain */
void fat_free_cluster(uint16_t cluster)
{
    uint8_t *fat = ramdisk_get_block(1);
    
    while (cluster != 0xFFF && cluster != 0xFF0) {
        uint16_t next = fat_get_next_cluster(cluster);
        fat_set_cluster(cluster, 0x000);  /* Mark as free */
        cluster = next;
    }
}

/* Helper: Set FAT entry */
static void fat_set_cluster(uint16_t cluster, uint16_t value)
{
    uint8_t *fat = ramdisk_get_block(1);
    uint32_t fat_offset = (cluster * 3) / 2;
    
    if (cluster & 1) {
        /* Odd cluster */
        fat[fat_offset] = (fat[fat_offset] & 0xF0) | ((value >> 8) & 0x0F);
        fat[fat_offset + 1] = value & 0xFF;
    } else {
        /* Even cluster */
        fat[fat_offset] = value & 0xFF;
        fat[fat_offset + 1] = (fat[fat_offset + 1] & 0xF0) | ((value >> 4) & 0x0F);
    }
}
```

### Phase 4: Directory and File Operations

**File:** `fs/filesystem.c`

```c
#include "fs.h"
#include "ramdisk.h"
#include "../output/output.h"
#include "../lib/string.h"

/* Initialize filesystem */
int fs_init(void)
{
    ramdisk_init();
    fs_format();  /* Create empty filesystem on first boot */
    return 0;
}

/* Format filesystem: create empty FAT and root directory */
int fs_format(void)
{
    /* Block 1-2: FAT (initialize to 0) */
    uint8_t fat_block[BLOCK_SIZE] = {0};
    ramdisk_write(1, fat_block);
    ramdisk_write(2, fat_block);
    
    /* Block 3: Root directory (initialize to 0) */
    uint8_t root_block[BLOCK_SIZE] = {0};
    ramdisk_write(ROOT_DIR_BLOCK, root_block);
    
    return 0;
}

/* List root directory entries */
int fs_list_root(DirectoryEntry **out_entries)
{
    uint8_t *root = ramdisk_get_block(ROOT_DIR_BLOCK);
    *out_entries = (DirectoryEntry*)root;
    
    /* Count non-empty entries */
    int count = 0;
    for (int i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (((DirectoryEntry*)root)[i].name[0] != 0x00 && 
            ((DirectoryEntry*)root)[i].name[0] != 0xE5) {  /* 0xE5 = deleted */
            count++;
        }
    }
    
    return count;
}

/* Find entry in root directory by name */
DirectoryEntry* fs_find_entry(const char *name)
{
    DirectoryEntry *entries;
    int count = fs_list_root(&entries);
    
    for (int i = 0; i < count; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            return &entries[i];
        }
    }
    
    return NULL;
}

/* Create empty file */
int fs_create_file(const char *name, uint32_t size)
{
    /* Find free directory entry */
    DirectoryEntry *entries;
    fs_list_root(&entries);
    
    DirectoryEntry *free_entry = NULL;
    for (int i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            free_entry = &entries[i];
            break;
        }
    }
    
    if (!free_entry) {
        return -1;  /* No free entries */
    }
    
    /* Allocate clusters if size > 0 */
    uint16_t first_cluster = 0;
    if (size > 0) {
        first_cluster = fat_allocate_cluster();
        if (first_cluster < 0) {
            return -1;  /* No free clusters */
        }
    }
    
    /* Fill directory entry */
    strcpy(free_entry->name, name);
    strcpy(free_entry->ext, "");
    free_entry->attributes = ATTR_ARCHIVE;
    free_entry->start_cluster = first_cluster;
    free_entry->file_size = size;
    
    return 0;
}

/* Delete file */
int fs_delete_file(const char *name)
{
    DirectoryEntry *entries;
    fs_list_root(&entries);
    
    for (int i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (strcmp(entries[i].name, name) == 0) {
            /* Free clusters */
            if (entries[i].start_cluster != 0) {
                fat_free_cluster(entries[i].start_cluster);
            }
            
            /* Mark entry as deleted */
            entries[i].name[0] = 0xE5;
            
            return 0;
        }
    }
    
    return -1;  /* File not found */
}

/* Read entire file into buffer */
int fs_read_file(const char *name, uint8_t *buffer, uint32_t max_size)
{
    DirectoryEntry *entry = fs_find_entry(name);
    if (!entry || entry->attributes & ATTR_DIRECTORY) {
        return -1;  /* File not found or is directory */
    }
    
    uint32_t bytes_read = 0;
    uint16_t cluster = entry->start_cluster;
    
    while (cluster != 0xFFF && bytes_read < max_size) {
        uint8_t *block = ramdisk_get_block(DATA_BLOCK_START + cluster);
        uint32_t to_copy = (entry->file_size - bytes_read > BLOCK_SIZE) ? 
                           BLOCK_SIZE : (entry->file_size - bytes_read);
        
        for (uint32_t i = 0; i < to_copy; i++) {
            buffer[bytes_read + i] = block[i];
        }
        
        bytes_read += to_copy;
        cluster = fat_get_next_cluster(cluster);
    }
    
    return bytes_read;
}

/* Write data to file */
int fs_write_file(const char *name, const uint8_t *data, uint32_t size)
{
    DirectoryEntry *entry = fs_find_entry(name);
    if (!entry) {
        return -1;  /* File not found */
    }
    
    uint32_t bytes_written = 0;
    uint16_t cluster = entry->start_cluster;
    
    while (size > 0) {
        if (cluster == 0xFFF) {
            /* Need to allocate new cluster */
            cluster = fat_allocate_cluster();
            if (cluster < 0) {
                return -1;  /* Out of space */
            }
        }
        
        uint8_t *block = ramdisk_get_block(DATA_BLOCK_START + cluster);
        uint32_t to_write = (size > BLOCK_SIZE) ? BLOCK_SIZE : size;
        
        for (uint32_t i = 0; i < to_write; i++) {
            block[i] = data[bytes_written + i];
        }
        
        bytes_written += to_write;
        size -= to_write;
        cluster = fat_get_next_cluster(cluster);
    }
    
    entry->file_size = bytes_written;
    return bytes_written;
}

/* Create directory */
int fs_create_dir(const char *name)
{
    DirectoryEntry *entries;
    fs_list_root(&entries);
    
    DirectoryEntry *free_entry = NULL;
    for (int i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] == 0x00) {
            free_entry = &entries[i];
            break;
        }
    }
    
    if (!free_entry) {
        return -1;
    }
    
    strcpy(free_entry->name, name);
    free_entry->attributes = ATTR_DIRECTORY;
    free_entry->file_size = 0;
    
    return 0;
}
```

### Phase 5: Shell Command Integration

**File:** `shell/fs_commands.c`

```c
#include "shell.h"
#include "../fs/fs.h"
#include "../output/output.h"
#include "../lib/string.h"

/* List files in directory */
void cmd_ls(char *args)
{
    DirectoryEntry *entries;
    int count = fs_list_root(&entries);
    
    if (count == 0) {
        kprint("(empty)\n");
        return;
    }
    
    for (int i = 0; i < count; i++) {
        kprint(entries[i].name);
        
        if (entries[i].attributes & ATTR_DIRECTORY) {
            kprint("/ ");
        } else {
            kprint("  ");
        }
        
        /* Print size */
        kprint_dec(entries[i].file_size);
        kprint(" bytes\n");
    }
}

/* Display file contents */
void cmd_cat(char *args)
{
    if (args[0] == '\0') {
        kprint("Usage: cat <filename>\n");
        return;
    }
    
    uint8_t buffer[4096];
    int bytes = fs_read_file(args, buffer, sizeof(buffer));
    
    if (bytes < 0) {
        kprint("File not found\n");
        return;
    }
    
    for (int i = 0; i < bytes; i++) {
        if (buffer[i] == '\n') {
            kprint("\n");
        } else if (buffer[i] >= 32 && buffer[i] < 127) {
            kprint_char(buffer[i]);
        }
    }
    kprint("\n");
}

/* Create file */
void cmd_touch(char *args)
{
    if (args[0] == '\0') {
        kprint("Usage: touch <filename>\n");
        return;
    }
    
    if (fs_create_file(args, 0) == 0) {
        kprint("Created: ");
        kprint(args);
        kprint("\n");
    } else {
        kprint("Error: Cannot create file\n");
    }
}

/* Create directory */
void cmd_mkdir(char *args)
{
    if (args[0] == '\0') {
        kprint("Usage: mkdir <dirname>\n");
        return;
    }
    
    if (fs_create_dir(args) == 0) {
        kprint("Created directory: ");
        kprint(args);
        kprint("\n");
    } else {
        kprint("Error: Cannot create directory\n");
    }
}

/* Delete file */
void cmd_rm(char *args)
{
    if (args[0] == '\0') {
        kprint("Usage: rm <filename>\n");
        return;
    }
    
    if (fs_delete_file(args) == 0) {
        kprint("Deleted: ");
        kprint(args);
        kprint("\n");
    } else {
        kprint("File not found\n");
    }
}

/* Print current directory (stub for now) */
void cmd_pwd(char *args)
{
    kprint("/\n");  /* Only root directory implemented */
}

/* Change directory (stub for now) */
void cmd_cd(char *args)
{
    if (args[0] == '\0' || strcmp(args, "/") == 0) {
        kprint("Changed to /\n");
    } else {
        kprint("Error: Subdirectories not yet supported\n");
    }
}
```

### Phase 6: Update Shell Command Map

**Update:** `shell/shell.c`

Add to `command_map[]` after existing commands:

```c
/* Filesystem commands */
{"ls", (void*)cmd_ls, 0, "List files in current directory"},
{"cat", (void*)cmd_cat, 1, "Display file contents"},
{"touch", (void*)cmd_touch, 1, "Create empty file"},
{"mkdir", (void*)cmd_mkdir, 1, "Create directory"},
{"rm", (void*)cmd_rm, 1, "Delete file"},
{"pwd", (void*)cmd_pwd, 0, "Print working directory"},
{"cd", (void*)cmd_cd, 1, "Change directory"},
```

### Phase 7: Kernel Initialization

**Update:** `kernel.c` - `kmain()` function

```c
void kmain(void)
{
    clear_screen();
    
    /* Initialize subsystems */
    idt_init();
    kb_init();
    fs_init();  /* NEW: Initialize filesystem */
    
    kprint("NaoKernel booting...\n");
    kprint("Filesystem initialized\n");
    
    nano_shell();
}
```

### Phase 8: Build System

**Update:** `build.sh`

```bash
# Compile filesystem modules
gcc -m32 -c fs/ramdisk.c -o fs/ramdisk.o
gcc -m32 -c fs/fat.c -o fs/fat.o
gcc -m32 -c fs/filesystem.c -o fs/filesystem.o
gcc -m32 -c shell/fs_commands.c -o shell/fs_commands.o

# Link filesystem objects with existing objects
ld -m elf_i386 -T link.ld kernel.o input.o output.o shell.o \
   fs/ramdisk.o fs/fat.o fs/filesystem.o shell/fs_commands.o \
   kernel.asm.o -o kernel
```

---

## Testing Strategy

### Unit Tests (Manual, on QEMU)

**Test 1: Create and List Files**
```
> touch file1.txt
Created: file1.txt
> ls
file1.txt     0 bytes
```

**Test 2: Read Files**
```
> echo "Hello World" > file1.txt  /* After implementing write support in echo */
> cat file1.txt
Hello World
```

**Test 3: Delete Files**
```
> rm file1.txt
Deleted: file1.txt
> ls
(empty)
```

**Test 4: Create Directories**
```
> mkdir mydir
Created directory: mydir
> ls
mydir/     0 bytes
```

**Test 5: Directory Navigation (Stub)**
```
> cd /
Changed to /
> pwd
/
```

---

## Estimated Code Size & Performance

| Component | Size | Notes |
|-----------|------|-------|
| ramdisk.c | 0.5 KB | Simple block read/write |
| fat.c | 2 KB | FAT12 cluster operations |
| filesystem.c | 5 KB | File/directory operations |
| fs_commands.c | 3 KB | Shell command implementations |
| Data structures (fs.h) | 0.5 KB | Structs and constants |
| **Total Code** | **~11 KB** | Lightweight filesystem |
| **Ramdisk Storage** | 256 KB | User files and metadata |
| **Overhead** | ~1 KB | FAT table + directory |

### Performance (Typical Operations)
- **File creation:** < 1 ms (single directory write)
- **File listing:** < 1 ms (read root directory sector)
- **File read:** ~10 ms per 256 KB (memcpy from ramdisk)
- **File write:** ~10 ms per 256 KB (memcpy to ramdisk)

---

## File Structure After Implementation

```
NaoKernel/
├── kernel.c
├── kernel.asm
├── shell/
│   ├── main.c
│   ├── shell.c
│   └── fs_commands.c          [NEW]
├── input/
│   ├── input.c
│   └── input.h
├── output/
│   ├── output.c
│   └── output.h
├── fs/                        [NEW FOLDER]
│   ├── ramdisk.c             [NEW]
│   ├── ramdisk.h             [NEW]
│   ├── fat.c                 [NEW]
│   ├── fat.h                 [NEW]
│   ├── filesystem.c          [NEW]
│   └── fs.h                  [NEW]
├── lib/
│   ├── string.c
│   └── string.h
├── build.sh
├── link.ld
└── bin/
    └── kernel
```

---

## Future Enhancements

1. **Subdirectories:** Track current working directory, support nested paths
2. **File write via shell:** Implement `echo "text" > file.txt` redirection
3. **Persistence:** Save ramdisk to disk image, restore on boot
4. **Access control:** File permissions, ownership (optional)
5. **Larger filesystem:** Support multiple ramdisk sections or real disk
6. **Journaling:** Add simple journaling for reliability
7. **Compression:** Store small files compressed
8. **File timestamps:** Add creation, modification dates

---

## Integration Points Summary

| Component | Change Type | Impact |
|-----------|------------|--------|
| kernel.c | Add `fs_init()` call | ~2 lines |
| shell.c | Add 7 entries to command_map | ~10 lines |
| shell/shell.c | Include fs.h | ~1 line |
| build.sh | Add fs/*.o compilation | ~5 lines |
| Memory layout | Add 256 KB ramdisk at 0x120000 | ~0 KB code |
| **Total Changes** | ~40 lines in existing files | Minimal impact |

---

## Notes

- **No external dependencies:** Pure C, no libc required
- **No disk I/O:** Ramdisk stays in memory (fast, non-persistent)
- **Simplified FAT12:** Supports clusters up to 4096 (sufficient for ramdisk)
- **Directory limitation:** Single root level only (can expand to subdirectories later)
- **Error handling:** Simple return codes (-1 for error, count/size for success)
- **Memory safety:** Fixed-size buffers, no dynamic allocation (similar to existing kernel)
