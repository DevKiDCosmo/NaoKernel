/* fs/fs.c - Drive detection implementation */

#include "fs.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);

/* Port I/O using inline assembly */
static unsigned char inb(unsigned short port)
{
    unsigned char result;
    __asm__ volatile ("inb %1, %0" : "=a" (result) : "Nd" (port));
    return result;
}

static void outb(unsigned short port, unsigned char data)
{
    __asm__ volatile ("outb %0, %1" : : "a" (data), "Nd" (port));
}

/* Helper: Wait for drive to be ready */
static void ide_wait_ready(unsigned short status_port)
{
    unsigned char status;
    int timeout = 10000;
    
    while (timeout-- > 0) {
        status = inb(status_port);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_RDY)) {
            return;
        }
    }
}

/* Helper: Detect if a drive exists */
static int ide_detect_drive(unsigned short base_port, int is_slave)
{
    unsigned char status;
    
    /* Select drive (0xA0 = master, 0xB0 = slave) */
    outb(base_port + 6, is_slave ? 0xB0 : 0xA0);
    
    /* Small delay */
    for (int i = 0; i < 1000; i++) {
        inb(base_port + 7);  /* Read status multiple times for delay */
    }
    
    /* Read status */
    status = inb(base_port + 7);
    
    /* Check if drive exists (status should not be 0xFF) */
    if (status == 0xFF || status == 0x00) {
        return 0;  /* No drive */
    }
    
    return 1;  /* Drive present */
}

/* Helper: Simple string copy */
static void strcpy_local(char *dest, const char *src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

/* Initialize filesystem and detect all drives */
void fs_init(FilesystemMap *fs_map)
{
    int i;
    
    kprint("Initializing filesystem...\n");
    
    /* Clear filesystem map */
    fs_map->drive_count = 0;
    for (i = 0; i < MAX_DRIVES; i++) {
        fs_map->drives[i].drive_number = i;
        fs_map->drives[i].type = DRIVE_TYPE_NONE;
        fs_map->drives[i].fs_type = FS_TYPE_UNKNOWN;
        fs_map->drives[i].size_mb = 0;
        fs_map->drives[i].present = 0;
        fs_map->drives[i].model[0] = '\0';
    }
    
    kprint("Detecting drives...\n");
    
    /* Detect Primary Master (drive 0) */
    if (ide_detect_drive(IDE_PRIMARY_DATA, 0)) {
        kprint("  [0] Primary Master: DETECTED\n");
        fs_map->drives[0].present = 1;
        fs_map->drives[0].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[0].model, "Primary Master");
        fs_map->drive_count++;
    } else {
        kprint("  [0] Primary Master: Not found\n");
    }
    
    /* Detect Primary Slave (drive 1) */
    if (ide_detect_drive(IDE_PRIMARY_DATA, 1)) {
        kprint("  [1] Primary Slave: DETECTED\n");
        fs_map->drives[1].present = 1;
        fs_map->drives[1].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[1].model, "Primary Slave");
        fs_map->drive_count++;
    } else {
        kprint("  [1] Primary Slave: Not found\n");
    }
    
    /* Detect Secondary Master (drive 2) */
    if (ide_detect_drive(IDE_SECONDARY_DATA, 0)) {
        kprint("  [2] Secondary Master: DETECTED\n");
        fs_map->drives[2].present = 1;
        fs_map->drives[2].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[2].model, "Secondary Master");
        fs_map->drive_count++;
    } else {
        kprint("  [2] Secondary Master: Not found\n");
    }
    
    /* Detect Secondary Slave (drive 3) */
    if (ide_detect_drive(IDE_SECONDARY_DATA, 1)) {
        kprint("  [3] Secondary Slave: DETECTED\n");
        fs_map->drives[3].present = 1;
        fs_map->drives[3].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[3].model, "Secondary Slave");
        fs_map->drive_count++;
    } else {
        kprint("  [3] Secondary Slave: Not found\n");
    }
    
    /* Print summary */
    kprint("\nDrive detection complete: ");
    if (fs_map->drive_count > 0) {
        /* Simple integer to string conversion */
        char count_str[2];
        count_str[0] = '0' + fs_map->drive_count;
        count_str[1] = '\0';
        kprint(count_str);
        kprint(" drive(s) found\n");
    } else {
        kprint("No drives found\n");
    }

    kprint_newline();
}