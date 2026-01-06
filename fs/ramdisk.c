/* fs/ramdisk.c - RAM disk implementation */

#include "ramdisk.h"

/* External output functions for debugging */
extern void kprint(const char *str);
extern void kprint_newline(void);

static unsigned char *ramdisk = (unsigned char*)RAMDISK_BASE;

/* Initialize ramdisk in memory */
void ramdisk_init(void)
{
    unsigned int i;
    
    kprint("  Initializing ramdisk at 0x120000 (256 KB)...\n");
    
    /* Clear ramdisk */
    for (i = 0; i < RAMDISK_SIZE; i++) {
        ramdisk[i] = 0;
    }
    
    kprint("  Ramdisk initialized.\n");
}

/* Read block from ramdisk */
int ramdisk_read(unsigned int block_num, unsigned char *buffer)
{
    unsigned int i;
    unsigned int offset;
    
    if (block_num >= MAX_BLOCKS) {
        return -1;  /* Invalid block */
    }
    
    offset = block_num * BLOCK_SIZE;
    for (i = 0; i < BLOCK_SIZE; i++) {
        buffer[i] = ramdisk[offset + i];
    }
    
    return BLOCK_SIZE;
}

/* Write block to ramdisk */
int ramdisk_write(unsigned int block_num, const unsigned char *data)
{
    unsigned int i;
    unsigned int offset;
    
    if (block_num >= MAX_BLOCKS) {
        return -1;  /* Invalid block */
    }
    
    offset = block_num * BLOCK_SIZE;
    for (i = 0; i < BLOCK_SIZE; i++) {
        ramdisk[offset + i] = data[i];
    }
    
    return BLOCK_SIZE;
}

/* Get direct pointer to block */
unsigned char* ramdisk_get_block(unsigned int block_num)
{
    if (block_num >= MAX_BLOCKS) {
        return (unsigned char*)0;
    }
    
    return ramdisk + (block_num * BLOCK_SIZE);
}
