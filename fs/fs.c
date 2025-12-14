/* fs/fs.c - Drive detection implementation */

#include "fs.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);

/* External assembly port I/O functions */
extern unsigned char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern unsigned short read_word_port(unsigned short port);

/* Port I/O wrappers */
static unsigned char inb(unsigned short port)
{
    return read_port(port);
}

static void outb(unsigned short port, unsigned char data)
{
    write_port(port, data);
}

/* Read 16-bit from port */
static unsigned short inw(unsigned short port)
{
    return read_word_port(port);
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

/* Read IDENTIFY data: returns 1 on success and fills total_sectors */
static int ata_identify(unsigned short base_port, int is_slave, unsigned int *total_sectors)
{
    unsigned char status;
    unsigned short data[256];
    int i;

    /* Select drive */
    outb(base_port + 6, is_slave ? 0xB0 : 0xA0);

    /* Zero sector count and LBA registers as per spec */
    outb(base_port + 2, 0);
    outb(base_port + 3, 0);
    outb(base_port + 4, 0);
    outb(base_port + 5, 0);

    /* Send IDENTIFY DEVICE (0xEC) */
    outb(base_port + 7, 0xEC);

    /* Check if drive responded */
    status = inb(base_port + 7);
    if (status == 0) {
        return 0;
    }

    /* Wait while BUSY, then check DRQ */
    int timeout = 100000;
    while ((status = inb(base_port + 7)) & IDE_STATUS_BSY) {
        if (--timeout == 0) return 0;
    }
    if (!(status & IDE_STATUS_DRQ)) {
        return 0;
    }

    /* Read 256 words (512 bytes) */
    for (i = 0; i < 256; i++) {
        data[i] = inw(base_port + 0);
    }

    /* Total LBA28 sectors are in words 60-61 
     * ATA spec: Words are 16-bit little-endian, 32-bit value is formed from two words
     * Word 60 = bits 0-15, Word 61 = bits 16-31
     */
    unsigned int lba28 = ((unsigned int)data[61] << 16) | (unsigned int)data[60];
    if (lba28 > 0) {
        *total_sectors = lba28;
        return 1;
    }

    /* If LBA28 is zero, try LBA48 in words 100-103 (lower 32 bits for simplicity) 
     * Word 100 = bits 0-15, Word 101 = bits 16-31
     */
    unsigned int lba48_low = ((unsigned int)data[101] << 16) | (unsigned int)data[100];
    if (lba48_low > 0) {
        *total_sectors = lba48_low; /* Note: truncated if >4GiB sectors */
        return 1;
    }

    return 0;
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

/* Helper: print DL in hex (e.g., DL=0x80) */
static void kprint_dl(unsigned char dl)
{
    static const char hex[] = "0123456789ABCDEF";
    char buf[8]; /* "DL=0xHH" + NUL */
    buf[0] = 'D';
    buf[1] = 'L';
    buf[2] = '=';
    buf[3] = '0';
    buf[4] = 'x';
    buf[5] = hex[(dl >> 4) & 0xF];
    buf[6] = hex[dl & 0xF];
    buf[7] = '\0';
    kprint(buf);
}

void fs_list(FilesystemMap *fs_map)
{
    int i;
    /* Forward declaration for formatted check */
    extern int is_drive_formatted(DriveInfo *drive);
    
    kprint("Detected Drives:\n");
    for (i = 0; i < fs_map->drive_count; i++) {
        DriveInfo *drive = &fs_map->drives[i];
        if (drive->present) {
            kprint(" Drive ");
            char drive_num_str[3];
            drive_num_str[0] = '0' + drive->drive_number;
            drive_num_str[1] = '\0';
            kprint(drive_num_str);
            kprint(": ");
            kprint(drive->model);
            kprint("  id=");
            kprint(drive->idNAME);
            kprint("  size=");
            /* print size_mb as decimal */
            {
                char buf[12];
                unsigned int v = drive->size_mb;
                int idx = 0;
                if (v == 0) {
                    buf[idx++] = '0';
                } else {
                    char tmp[12];
                    int t = 0;
                    while (v > 0 && t < 11) {
                        tmp[t++] = (char)('0' + (v % 10));
                        v /= 10;
                    }
                    while (t > 0) {
                        buf[idx++] = tmp[--t];
                    }
                }
                buf[idx++] = 'M';
                buf[idx++] = 'B';
                buf[idx] = '\0';
                kprint(buf);
            }
            
            /* Check if formatted */
            kprint("  [");
            if (is_drive_formatted(drive)) {
                kprint("Formatted: ");
                switch (drive->fs_type) {
                    case FS_TYPE_FAT12: kprint("FAT12"); break;
                    case FS_TYPE_FAT16: kprint("FAT16"); break;
                    case FS_TYPE_FAT32: kprint("FAT32"); break;
                    default: kprint("Unknown"); break;
                }
            } else {
                kprint("Not formatted");
            }
            kprint("]");
            
            kprint("\n");
        }
    }
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
        fs_map->drives[i].idNAME[0] = '\0';
    }
    
    kprint("Detecting drives...\n");
    
    /* Detect Primary Master (drive 0) */
    if (ide_detect_drive(IDE_PRIMARY_DATA, 0)) {
        kprint("  [0] Primary Master: DETECTED (");
        kprint_dl(0x80);
        kprint(")\n");
        fs_map->drives[0].present = 1;
        fs_map->drives[0].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[0].model, "Primary Master");
        strcpy_local(fs_map->drives[0].idNAME, "ide0");
        {
            unsigned int sectors = 0;
            if (ata_identify(IDE_PRIMARY_DATA, 0, &sectors)) {
                /* size in MB: sectors * 512 bytes / (1024*1024) = sectors / 2048 */
                fs_map->drives[0].size_mb = sectors / 2048;
            }
        }
        fs_map->drive_count++;
    } else {
        kprint("  [0] Primary Master: Not found\n");
    }
    
    /* Detect Primary Slave (drive 1) */
    if (ide_detect_drive(IDE_PRIMARY_DATA, 1)) {
        kprint("  [1] Primary Slave: DETECTED (");
        kprint_dl(0x81);
        kprint(")\n");
        fs_map->drives[1].present = 1;
        fs_map->drives[1].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[1].model, "Primary Slave");
        strcpy_local(fs_map->drives[1].idNAME, "ide1");
        {
            unsigned int sectors = 0;
            if (ata_identify(IDE_PRIMARY_DATA, 1, &sectors)) {
                fs_map->drives[1].size_mb = sectors / 2048;
            }
        }
        fs_map->drive_count++;
    } else {
        kprint("  [1] Primary Slave: Not found\n");
    }
    
    /* Detect Secondary Master (drive 2) */
    if (ide_detect_drive(IDE_SECONDARY_DATA, 0)) {
        kprint("  [2] Secondary Master: DETECTED (");
        kprint_dl(0x82);
        kprint(")\n");
        fs_map->drives[2].present = 1;
        fs_map->drives[2].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[2].model, "Secondary Master");
        strcpy_local(fs_map->drives[2].idNAME, "ide2");
        {
            unsigned int sectors = 0;
            if (ata_identify(IDE_SECONDARY_DATA, 0, &sectors)) {
                fs_map->drives[2].size_mb = sectors / 2048;
            }
        }
        fs_map->drive_count++;
    } else {
        kprint("  [2] Secondary Master: Not found\n");
    }
    
    /* Detect Secondary Slave (drive 3) */
    if (ide_detect_drive(IDE_SECONDARY_DATA, 1)) {
        kprint("  [3] Secondary Slave: DETECTED (");
        kprint_dl(0x83);
        kprint(")\n");
        fs_map->drives[3].present = 1;
        fs_map->drives[3].type = DRIVE_TYPE_ATA;
        strcpy_local(fs_map->drives[3].model, "Secondary Slave");
        strcpy_local(fs_map->drives[3].idNAME, "ide3");
        {
            unsigned int sectors = 0;
            if (ata_identify(IDE_SECONDARY_DATA, 1, &sectors)) {
                fs_map->drives[3].size_mb = sectors / 2048;
            }
        }
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