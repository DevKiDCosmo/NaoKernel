/* fs/mount/mount.c - Drive mounting implementation */

#include "mount.h"
#include "../fs.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);

/* External assembly port I/O functions */
extern char read_port(unsigned short port);
extern void write_port(unsigned short port, unsigned char data);
extern unsigned short read_word_port(unsigned short port);

/* Port I/O wrappers */
static unsigned char inb(unsigned short port)
{
    return (unsigned char)read_port(port);
}

static void outb(unsigned short port, unsigned char data)
{
    write_port(port, data);
}

static unsigned short inw(unsigned short port)
{
    return read_word_port(port);
}

/* String utilities */
static int strcmp_local(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

static void strcpy_local(char *dest, const char *src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
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

/* Read a single sector from ATA drive */
static int ata_read_sector(DriveInfo *drive, unsigned int lba, void *buffer)
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
    
    /* Send READ SECTORS command (0x20) */
    outb(base_port + 7, 0x20);
    
    /* Wait for drive ready */
    if (!ata_wait_ready(base_port, 10000)) {
        return 0;
    }
    
    /* Read 256 words (512 bytes) */
    for (i = 0; i < 256; i++) {
        buf[i] = inw(base_port);
    }
    
    return 1;
}

/* Check if drive is formatted by reading boot sector */
int is_drive_formatted(DriveInfo *drive)
{
    unsigned char sector[512];
    
    if (!drive || !drive->present) {
        return 0;
    }
    
    /* Read boot sector (LBA 0) */
    if (!ata_read_sector(drive, 0, sector)) {
        return 0;
    }
    
    /* Check for boot signature */
    if (sector[510] != 0x55 || sector[511] != 0xAA) {
        return 0;
    }
    
    /* Check for FAT filesystem signatures */
    /* FAT12/16 check - "FAT12   " or "FAT16   " at offset 54 */
    if ((sector[54] == 'F' && sector[55] == 'A' && sector[56] == 'T' && sector[57] == '1') ||
        (sector[82] == 'F' && sector[83] == 'A' && sector[84] == 'T' && sector[85] == '3')) {
        
        /* Detect specific filesystem type */
        if (sector[54] == 'F' && sector[57] == '1' && sector[58] == '2') {
            drive->fs_type = FS_TYPE_FAT12;
        } else if (sector[54] == 'F' && sector[57] == '1' && sector[58] == '6') {
            drive->fs_type = FS_TYPE_FAT16;
        } else if (sector[82] == 'F' && sector[85] == '3' && sector[86] == '2') {
            drive->fs_type = FS_TYPE_FAT32;
        } else {
            drive->fs_type = FS_TYPE_UNKNOWN;
        }
        
        return 1;
    }
    
    return 0;
}

/* Initialize mount table */
void mount_init(MountTable *table)
{
    int i;
    table->current_mount = -1;
    
    for (i = 0; i < MAX_MOUNTS; i++) {
        table->mounts[i].drive = 0;
        table->mounts[i].is_mounted = 0;
        table->mounts[i].mount_point[0] = '\0';
    }
}

/* Mount a drive */
MountResult mount_drive(MountTable *table, DriveInfo *drive)
{
    int i;
    
    if (!drive || !drive->present) {
        return MOUNT_ERROR_INVALID_DRIVE;
    }
    
    /* Check if drive is formatted */
    if (!is_drive_formatted(drive)) {
        return MOUNT_ERROR_NOT_FORMATTED;
    }
    
    /* Check if this specific drive is already mounted */
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (table->mounts[i].is_mounted && 
            table->mounts[i].drive == drive) {
            /* Drive already mounted, no need to mount again */
            return MOUNT_SUCCESS;
        }
    }
    
    /* Automatically unmount all previously mounted drives */
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (table->mounts[i].is_mounted) {
            kprint("Automount: Dismounting ");
            kprint(table->mounts[i].mount_point);
            kprint("...\n");
            
            MountResult result = unmount_drive(table, i);
            if (result != MOUNT_SUCCESS) {
                kprint("  Warning: Dismount failed for ");
                kprint(table->mounts[i].mount_point);
                kprint("\n");
            }
        }
    }
    
    /* Find free mount slot */
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (!table->mounts[i].is_mounted) {
            table->mounts[i].drive = drive;
            table->mounts[i].is_mounted = 1;
            strcpy_local(table->mounts[i].mount_point, drive->idNAME);
            
            /* Set as current mount */
            table->current_mount = i;
            
            kprint("Automount: Successfully mounted ");
            kprint(drive->idNAME);
            kprint(" (");
            kprint(drive->model);
            kprint(")\n");
            
            return MOUNT_SUCCESS;
        }
    }
    
    return MOUNT_ERROR_UNSUPPORTED_FS;
}

/* Unmount a drive */
MountResult unmount_drive(MountTable *table, int mount_index)
{
    if (mount_index < 0 || mount_index >= MAX_MOUNTS) {
        return MOUNT_ERROR_INVALID_DRIVE;
    }
    
    if (!table->mounts[mount_index].is_mounted) {
        return MOUNT_ERROR_INVALID_DRIVE;
    }
    
    table->mounts[mount_index].is_mounted = 0;
    table->mounts[mount_index].drive = 0;
    table->mounts[mount_index].mount_point[0] = '\0';
    
    /* Update current mount if it was unmounted */
    if (table->current_mount == mount_index) {
        table->current_mount = -1;
        
        /* Find first available mount */
        int i;
        for (i = 0; i < MAX_MOUNTS; i++) {
            if (table->mounts[i].is_mounted) {
                table->current_mount = i;
                break;
            }
        }
    }
    
    return MOUNT_SUCCESS;
}

/* Get human-readable result string */
const char* get_mount_result_string(MountResult result)
{
    switch (result) {
        case MOUNT_SUCCESS: return "Success";
        case MOUNT_ERROR_INVALID_DRIVE: return "Invalid drive";
        case MOUNT_ERROR_NOT_FORMATTED: return "Drive not formatted";
        case MOUNT_ERROR_ALREADY_MOUNTED: return "Already mounted";
        case MOUNT_ERROR_UNSUPPORTED_FS: return "Unsupported filesystem";
        default: return "Unknown error";
    }
}

/* Get current prompt based on mounted drive */
const char* get_current_prompt(MountTable *table)
{
    static char prompt[32];
    
    if (table->current_mount >= 0 && table->current_mount < MAX_MOUNTS &&
        table->mounts[table->current_mount].is_mounted) {
        
        /* Build prompt like "ide0> " */
        const char *mount_point = table->mounts[table->current_mount].mount_point;
        int i = 0;
        while (mount_point[i] != '\0' && i < 16) {
            prompt[i] = mount_point[i];
            i++;
        }
        prompt[i++] = '>';
        prompt[i++] = ' ';
        prompt[i] = '\0';
        
        return prompt;
    }
    
    return "> ";
}

/* Set current drive by drive ID */
int set_current_drive(MountTable *table, const char *drive_id)
{
    int i;
    
    for (i = 0; i < MAX_MOUNTS; i++) {
        if (table->mounts[i].is_mounted && 
            strcmp_local(table->mounts[i].mount_point, drive_id) == 0) {
            table->current_mount = i;
            return 1;
        }
    }
    
    return 0;
}
