/* fs/ramdisk.h - RAM disk header */
#ifndef RAMDISK_H
#define RAMDISK_H

#define RAMDISK_BASE 0x120000
#define RAMDISK_SIZE (256 * 1024)  /* 256 KB */
#define BLOCK_SIZE 512
#define MAX_BLOCKS (RAMDISK_SIZE / BLOCK_SIZE)  /* 512 blocks */

/* Initialize ramdisk in memory */
void ramdisk_init(void);

/* Read/write blocks */
int ramdisk_read(unsigned int block_num, unsigned char *buffer);
int ramdisk_write(unsigned int block_num, const unsigned char *data);

/* Get pointer to block data (low-level access) */
unsigned char* ramdisk_get_block(unsigned int block_num);

#endif /* RAMDISK_H */
