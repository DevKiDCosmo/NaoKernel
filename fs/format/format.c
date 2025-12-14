/* fs/format/format.c - Drive formatting implementation */

#include "format.h"
#include "../fs.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);

/* Port I/O */
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

static void outw(unsigned short port, unsigned short data)
{
    __asm__ volatile ("outw %0, %1" : : "a" (data), "Nd" (port));
}

/* Simple string operations */
static void memset_local(void *dst, unsigned char val, unsigned int size)
{
    unsigned char *d = (unsigned char*)dst;
    while (size--) *d++ = val;
}

static void memcpy_local(void *dst, const void *src, unsigned int size)
{
    unsigned char *d = (unsigned char*)dst;
    const unsigned char *s = (const unsigned char*)src;
    while (size--) *d++ = *s++;
}

static int strlen_local(const char *str)
{
    int len = 0;
    while (str[len]) len++;
    return len;
}

/* Detect media type based on drive size */
MediaType detect_media_type(DriveInfo *drive)
{
    unsigned int size_kb = drive->size_mb * 1024;
    
    /* Floppy detection based on common sizes */
    if (size_kb <= 400) return MEDIA_TYPE_FLOPPY_360KB;
    if (size_kb <= 800) return MEDIA_TYPE_FLOPPY_720KB;
    if (size_kb <= 1300) return MEDIA_TYPE_FLOPPY_1_2MB;
    if (size_kb <= 1500) return MEDIA_TYPE_FLOPPY_1_44MB;
    if (size_kb <= 3000) return MEDIA_TYPE_FLOPPY_2_88MB;
    
    /* Hard drive detection */
    if (drive->size_mb < 32) return MEDIA_TYPE_HDD_SMALL;
    if (drive->size_mb <= 512) return MEDIA_TYPE_HDD_MEDIUM;
    return MEDIA_TYPE_HDD_LARGE;
}

/* Wait for drive ready */
static int ata_wait_ready(unsigned short base_port, int timeout)
{
    unsigned char status;
    while (timeout-- > 0) {
        status = inb(base_port + 7);
        if (!(status & IDE_STATUS_BSY) && (status & IDE_STATUS_RDY)) {
            return 1;
        }
    }
    return 0;
}

/* Write a single sector to ATA drive */
static int ata_write_sector(DriveInfo *drive, unsigned int lba, void *buffer)
{
    unsigned short base_port;
    unsigned char drive_select;
    unsigned short *buf = (unsigned short*)buffer;
    int i;
    
    /* Determine port and drive select based on drive number */
    if (drive->drive_number < 2) {
        base_port = IDE_PRIMARY_DATA;
        drive_select = (drive->drive_number == 0) ? 0xE0 : 0xF0;
    } else {
        base_port = IDE_SECONDARY_DATA;
        drive_select = (drive->drive_number == 2) ? 0xE0 : 0xF0;
    }
    
    /* LBA28 mode: select drive and set LBA bits 24-27 */
    outb(base_port + 6, drive_select | ((lba >> 24) & 0x0F));
    
    /* Write sector count (1) */
    outb(base_port + 2, 1);
    
    /* Write LBA low, mid, high */
    outb(base_port + 3, lba & 0xFF);
    outb(base_port + 4, (lba >> 8) & 0xFF);
    outb(base_port + 5, (lba >> 16) & 0xFF);
    
    /* Send WRITE SECTORS command (0x30) */
    outb(base_port + 7, 0x30);
    
    /* Wait for drive ready */
    if (!ata_wait_ready(base_port, 10000)) {
        return 0;
    }
    
    /* Write 256 words (512 bytes) */
    for (i = 0; i < 256; i++) {
        outw(base_port, buf[i]);
    }
    
    /* Flush cache */
    outb(base_port + 7, 0xE7);
    if (!ata_wait_ready(base_port, 10000)) {
        return 0;
    }
    
    return 1;
}

/* FAT12 formatting for floppy disks */
static FormatResult format_fat12(DriveInfo *drive, FormatOptions *options)
{
    unsigned char sector[512];
    unsigned int total_sectors;
    unsigned short sectors_per_fat;
    unsigned short root_dir_sectors;
    unsigned int i;
    
    /* Determine geometry based on media type */
    MediaType media = detect_media_type(drive);
    unsigned short bytes_per_sector = 512;
    unsigned char sectors_per_cluster = 1;
    unsigned short reserved_sectors = 1;
    unsigned char num_fats = 2;
    unsigned short root_entries = 224;
    unsigned char media_descriptor = 0xF0;
    
    switch (media) {
        case MEDIA_TYPE_FLOPPY_360KB:
            total_sectors = 720;
            sectors_per_cluster = 2;
            media_descriptor = 0xFD;
            break;
        case MEDIA_TYPE_FLOPPY_720KB:
            total_sectors = 1440;
            sectors_per_cluster = 2;
            media_descriptor = 0xF9;
            break;
        case MEDIA_TYPE_FLOPPY_1_2MB:
            total_sectors = 2400;
            sectors_per_cluster = 1;
            media_descriptor = 0xF9;
            break;
        case MEDIA_TYPE_FLOPPY_1_44MB:
            total_sectors = 2880;
            sectors_per_cluster = 1;
            media_descriptor = 0xF0;
            break;
        case MEDIA_TYPE_FLOPPY_2_88MB:
            total_sectors = 5760;
            sectors_per_cluster = 2;
            media_descriptor = 0xF0;
            break;
        default:
            return FORMAT_ERROR_UNSUPPORTED;
    }
    
    /* Calculate FAT size */
    root_dir_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    unsigned int tmp = total_sectors - (reserved_sectors + root_dir_sectors);
    sectors_per_fat = (tmp + (sectors_per_cluster * bytes_per_sector / 3 * 2 - 1)) / 
                      (sectors_per_cluster * bytes_per_sector / 3 * 2 + num_fats);
    
    /* Build boot sector */
    memset_local(sector, 0, 512);
    
    /* Jump instruction */
    sector[0] = 0xEB;
    sector[1] = 0x3C;
    sector[2] = 0x90;
    
    /* OEM name */
    memcpy_local(&sector[3], "NAOKER  ", 8);
    
    /* BPB (BIOS Parameter Block) */
    *(unsigned short*)&sector[11] = bytes_per_sector;      /* Bytes per sector */
    sector[13] = sectors_per_cluster;                       /* Sectors per cluster */
    *(unsigned short*)&sector[14] = reserved_sectors;      /* Reserved sectors */
    sector[16] = num_fats;                                  /* Number of FATs */
    *(unsigned short*)&sector[17] = root_entries;          /* Root entries */
    *(unsigned short*)&sector[19] = total_sectors;         /* Total sectors (16-bit) */
    sector[21] = media_descriptor;                          /* Media descriptor */
    *(unsigned short*)&sector[22] = sectors_per_fat;       /* Sectors per FAT */
    *(unsigned short*)&sector[24] = 18;                    /* Sectors per track */
    *(unsigned short*)&sector[26] = 2;                     /* Number of heads */
    *(unsigned int*)&sector[28] = 0;                       /* Hidden sectors */
    *(unsigned int*)&sector[32] = 0;                       /* Large sectors (32-bit) */
    
    /* Extended boot signature */
    sector[36] = 0x29;                                      /* Extended boot signature */
    *(unsigned int*)&sector[37] = 0x12345678;              /* Volume serial */
    
    /* Volume label */
    memset_local(&sector[43], ' ', 11);
    if (options && options->volume_label[0]) {
        int label_len = strlen_local(options->volume_label);
        if (label_len > 11) label_len = 11;
        memcpy_local(&sector[43], options->volume_label, label_len);
    } else {
        memcpy_local(&sector[43], "NO NAME    ", 11);
    }
    
    /* File system type */
    memcpy_local(&sector[54], "FAT12   ", 8);
    
    /* Boot signature */
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    /* Write boot sector */
    kprint("  Writing FAT12 boot sector...\n");
    if (!ata_write_sector(drive, 0, sector)) {
        return FORMAT_ERROR_WRITE_FAILED;
    }
    
    /* Create FAT */
    memset_local(sector, 0, 512);
    sector[0] = media_descriptor;
    sector[1] = 0xFF;
    sector[2] = 0xFF;
    
    /* Write first FAT */
    kprint("  Writing FAT tables...\n");
    for (i = 0; i < sectors_per_fat; i++) {
        if (!ata_write_sector(drive, reserved_sectors + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
        /* Clear for remaining sectors */
        if (i == 0) memset_local(sector, 0, 512);
    }
    
    /* Write second FAT (copy) */
    sector[0] = media_descriptor;
    sector[1] = 0xFF;
    sector[2] = 0xFF;
    for (i = 0; i < sectors_per_fat; i++) {
        if (!ata_write_sector(drive, reserved_sectors + sectors_per_fat + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
        if (i == 0) memset_local(sector, 0, 512);
    }
    
    /* Write empty root directory */
    kprint("  Writing root directory...\n");
    memset_local(sector, 0, 512);
    for (i = 0; i < root_dir_sectors; i++) {
        if (!ata_write_sector(drive, reserved_sectors + (num_fats * sectors_per_fat) + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
    }
    
    kprint("  FAT12 format complete.\n");
    return FORMAT_SUCCESS;
}

/* FAT16 formatting for small hard drives */
static FormatResult format_fat16(DriveInfo *drive, FormatOptions *options)
{
    unsigned char sector[512];
    unsigned int total_sectors;
    unsigned short sectors_per_fat;
    unsigned short root_dir_sectors;
    unsigned int i;
    
    total_sectors = drive->size_mb * 2048; /* MB to 512-byte sectors */
    unsigned short bytes_per_sector = 512;
    unsigned char sectors_per_cluster = (drive->size_mb < 128) ? 4 : 8;
    unsigned short reserved_sectors = 1;
    unsigned char num_fats = 2;
    unsigned short root_entries = 512;
    
    /* Calculate FAT size for FAT16 */
    root_dir_sectors = ((root_entries * 32) + (bytes_per_sector - 1)) / bytes_per_sector;
    unsigned int data_sectors = total_sectors - (reserved_sectors + root_dir_sectors);
    unsigned int total_clusters = data_sectors / sectors_per_cluster;
    sectors_per_fat = ((total_clusters * 2) + (bytes_per_sector - 1)) / bytes_per_sector;
    
    /* Build boot sector */
    memset_local(sector, 0, 512);
    
    sector[0] = 0xEB; sector[1] = 0x3C; sector[2] = 0x90;
    memcpy_local(&sector[3], "NAOKER  ", 8);
    
    *(unsigned short*)&sector[11] = bytes_per_sector;
    sector[13] = sectors_per_cluster;
    *(unsigned short*)&sector[14] = reserved_sectors;
    sector[16] = num_fats;
    *(unsigned short*)&sector[17] = root_entries;
    
    /* Use 32-bit sector count if > 65535 */
    if (total_sectors < 65536) {
        *(unsigned short*)&sector[19] = (unsigned short)total_sectors;
        *(unsigned int*)&sector[32] = 0;
    } else {
        *(unsigned short*)&sector[19] = 0;
        *(unsigned int*)&sector[32] = total_sectors;
    }
    
    sector[21] = 0xF8; /* Hard disk media descriptor */
    *(unsigned short*)&sector[22] = sectors_per_fat;
    *(unsigned short*)&sector[24] = 63;
    *(unsigned short*)&sector[26] = 255;
    *(unsigned int*)&sector[28] = 0;
    
    sector[36] = 0x29;
    *(unsigned int*)&sector[37] = 0x12345678;
    
    memset_local(&sector[43], ' ', 11);
    if (options && options->volume_label[0]) {
        int label_len = strlen_local(options->volume_label);
        if (label_len > 11) label_len = 11;
        memcpy_local(&sector[43], options->volume_label, label_len);
    } else {
        memcpy_local(&sector[43], "NO NAME    ", 11);
    }
    
    memcpy_local(&sector[54], "FAT16   ", 8);
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    kprint("  Writing FAT16 boot sector...\n");
    if (!ata_write_sector(drive, 0, sector)) {
        return FORMAT_ERROR_WRITE_FAILED;
    }
    
    /* Write FATs */
    kprint("  Writing FAT tables...\n");
    memset_local(sector, 0, 512);
    sector[0] = 0xF8; sector[1] = 0xFF; sector[2] = 0xFF; sector[3] = 0xFF;
    
    for (i = 0; i < sectors_per_fat * num_fats; i++) {
        if (!ata_write_sector(drive, reserved_sectors + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
        if (i == 0) memset_local(sector, 0, 512);
    }
    
    /* Write root directory */
    kprint("  Writing root directory...\n");
    memset_local(sector, 0, 512);
    for (i = 0; i < root_dir_sectors; i++) {
        if (!ata_write_sector(drive, reserved_sectors + (num_fats * sectors_per_fat) + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
    }
    
    kprint("  FAT16 format complete.\n");
    return FORMAT_SUCCESS;
}

/* FAT32 formatting for large hard drives */
static FormatResult format_fat32(DriveInfo *drive, FormatOptions *options)
{
    unsigned char sector[512];
    unsigned int total_sectors;
    unsigned int sectors_per_fat;
    unsigned int i;
    
    total_sectors = drive->size_mb * 2048;
    unsigned short bytes_per_sector = 512;
    unsigned char sectors_per_cluster = (drive->size_mb < 8192) ? 8 : 16;
    unsigned short reserved_sectors = 32;
    unsigned char num_fats = 2;
    
    /* Calculate FAT32 size */
    unsigned int data_sectors = total_sectors - reserved_sectors;
    unsigned int total_clusters = data_sectors / sectors_per_cluster;
    sectors_per_fat = ((total_clusters * 4) + (bytes_per_sector - 1)) / bytes_per_sector;
    
    /* Build boot sector */
    memset_local(sector, 0, 512);
    
    sector[0] = 0xEB; sector[1] = 0x58; sector[2] = 0x90;
    memcpy_local(&sector[3], "NAOKER  ", 8);
    
    *(unsigned short*)&sector[11] = bytes_per_sector;
    sector[13] = sectors_per_cluster;
    *(unsigned short*)&sector[14] = reserved_sectors;
    sector[16] = num_fats;
    *(unsigned short*)&sector[17] = 0; /* No root entries in FAT32 */
    *(unsigned short*)&sector[19] = 0; /* Use 32-bit sector count */
    sector[21] = 0xF8;
    *(unsigned short*)&sector[22] = 0; /* FAT32 uses offset 36 */
    *(unsigned short*)&sector[24] = 63;
    *(unsigned short*)&sector[26] = 255;
    *(unsigned int*)&sector[28] = 0;
    *(unsigned int*)&sector[32] = total_sectors;
    
    /* FAT32-specific fields */
    *(unsigned int*)&sector[36] = sectors_per_fat;
    *(unsigned short*)&sector[40] = 0; /* Flags */
    *(unsigned short*)&sector[42] = 0; /* Version */
    *(unsigned int*)&sector[44] = 2; /* Root cluster */
    *(unsigned short*)&sector[48] = 1; /* FSInfo sector */
    *(unsigned short*)&sector[50] = 6; /* Backup boot sector */
    
    sector[64] = 0x29;
    *(unsigned int*)&sector[65] = 0x12345678;
    
    memset_local(&sector[71], ' ', 11);
    if (options && options->volume_label[0]) {
        int label_len = strlen_local(options->volume_label);
        if (label_len > 11) label_len = 11;
        memcpy_local(&sector[71], options->volume_label, label_len);
    } else {
        memcpy_local(&sector[71], "NO NAME    ", 11);
    }
    
    memcpy_local(&sector[82], "FAT32   ", 8);
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    kprint("  Writing FAT32 boot sector...\n");
    if (!ata_write_sector(drive, 0, sector)) {
        return FORMAT_ERROR_WRITE_FAILED;
    }
    
    /* Write FSInfo sector */
    memset_local(sector, 0, 512);
    *(unsigned int*)&sector[0] = 0x41615252; /* Lead signature */
    *(unsigned int*)&sector[484] = 0x61417272; /* Struct signature */
    *(unsigned int*)&sector[488] = 0xFFFFFFFF; /* Free clusters (unknown) */
    *(unsigned int*)&sector[492] = 0xFFFFFFFF; /* Next free cluster */
    sector[510] = 0x55;
    sector[511] = 0xAA;
    
    if (!ata_write_sector(drive, 1, sector)) {
        return FORMAT_ERROR_WRITE_FAILED;
    }
    
    /* Write FATs */
    kprint("  Writing FAT32 tables...\n");
    memset_local(sector, 0, 512);
    *(unsigned int*)&sector[0] = 0x0FFFFFF8; /* Media + EOC */
    *(unsigned int*)&sector[4] = 0xFFFFFFFF; /* EOC */
    *(unsigned int*)&sector[8] = 0x0FFFFFFF; /* Root cluster marked */
    
    for (i = 0; i < sectors_per_fat * num_fats; i++) {
        if (!ata_write_sector(drive, reserved_sectors + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
        if (i == 0) memset_local(sector, 0, 512);
    }
    
    /* Zero root cluster */
    kprint("  Writing root directory cluster...\n");
    memset_local(sector, 0, 512);
    unsigned int root_lba = reserved_sectors + (num_fats * sectors_per_fat);
    for (i = 0; i < sectors_per_cluster; i++) {
        if (!ata_write_sector(drive, root_lba + i, sector)) {
            return FORMAT_ERROR_WRITE_FAILED;
        }
    }
    
    kprint("  FAT32 format complete.\n");
    return FORMAT_SUCCESS;
}

/* Main format function */
FormatResult format_drive(DriveInfo *drive, FormatOptions *options)
{
    MediaType media;
    
    if (!drive || !drive->present) {
        return FORMAT_ERROR_INVALID_DRIVE;
    }
    
    if (drive->size_mb == 0) {
        kprint("Error: Cannot format drive with unknown size.\n");
        return FORMAT_ERROR_INVALID_DRIVE;
    }
    
    media = detect_media_type(drive);
    
    kprint("Formatting ");
    kprint(drive->idNAME);
    kprint(" (");
    kprint(drive->model);
    kprint(")...\n");
    
    /* Select appropriate format method */
    switch (media) {
        case MEDIA_TYPE_FLOPPY_360KB:
        case MEDIA_TYPE_FLOPPY_720KB:
        case MEDIA_TYPE_FLOPPY_1_2MB:
        case MEDIA_TYPE_FLOPPY_1_44MB:
        case MEDIA_TYPE_FLOPPY_2_88MB:
            kprint("  Detected: Floppy disk (FAT12)\n");
            return format_fat12(drive, options);
            
        case MEDIA_TYPE_HDD_SMALL:
            kprint("  Detected: Small HDD (FAT12)\n");
            return format_fat12(drive, options);
            
        case MEDIA_TYPE_HDD_MEDIUM:
            kprint("  Detected: Medium HDD (FAT16)\n");
            return format_fat16(drive, options);
            
        case MEDIA_TYPE_HDD_LARGE:
            kprint("  Detected: Large HDD (FAT32)\n");
            return format_fat32(drive, options);
            
        default:
            return FORMAT_ERROR_UNSUPPORTED;
    }
}

/* Get human-readable result string */
const char* get_format_result_string(FormatResult result)
{
    switch (result) {
        case FORMAT_SUCCESS: return "Success";
        case FORMAT_ERROR_INVALID_DRIVE: return "Invalid drive";
        case FORMAT_ERROR_WRITE_FAILED: return "Write failed";
        case FORMAT_ERROR_UNSUPPORTED: return "Unsupported media";
        case FORMAT_ERROR_TOO_LARGE: return "Drive too large";
        default: return "Unknown error";
    }
}
