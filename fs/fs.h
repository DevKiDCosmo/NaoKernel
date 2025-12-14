/* fs/fs.h - Updated header */
#ifndef FS_H
#define FS_H

/* IDE Controller Ports */
#define IDE_PRIMARY_DATA       0x1F0
#define IDE_PRIMARY_ERROR      0x1F1
#define IDE_PRIMARY_SECTORS    0x1F2
#define IDE_PRIMARY_LBA_LO     0x1F3
#define IDE_PRIMARY_LBA_MID    0x1F4
#define IDE_PRIMARY_LBA_HI     0x1F5
#define IDE_PRIMARY_DRIVE      0x1F6
#define IDE_PRIMARY_STATUS     0x1F7
#define IDE_PRIMARY_COMMAND    0x1F7

#define IDE_SECONDARY_DATA     0x170
#define IDE_SECONDARY_STATUS   0x177
#define IDE_SECONDARY_COMMAND  0x177

/* IDE Status Register Bits */
#define IDE_STATUS_ERR  0x01
#define IDE_STATUS_DRQ  0x08
#define IDE_STATUS_SRV  0x10
#define IDE_STATUS_DF   0x20
#define IDE_STATUS_RDY  0x40
#define IDE_STATUS_BSY  0x80

/* Drive Types */
typedef enum {
    DRIVE_TYPE_NONE = 0,
    DRIVE_TYPE_ATA,
    DRIVE_TYPE_ATAPI,
    DRIVE_TYPE_UNKNOWN
} DriveType;

/* Filesystem Types */
typedef enum {
    FS_TYPE_UNKNOWN = 0,
    FS_TYPE_FAT12,
    FS_TYPE_FAT16,
    FS_TYPE_FAT32,
    FS_TYPE_EXT2,
    FS_TYPE_EXT4,
    FS_TYPE_NTFS
} FilesystemType;

/* Drive Information */
typedef struct {
    int drive_number;       /* 0=Primary Master, 1=Primary Slave, 2=Secondary Master, 3=Secondary Slave */
    DriveType type;
    FilesystemType fs_type;
    unsigned int size_mb;
    char model[41];         /* 40 chars + null terminator */
    int present;
    char idNAME[16];   /* Identifier name for mounting and device identification */
} DriveInfo;

/* Filesystem Map */
#define MAX_DRIVES 4
typedef struct {
    DriveInfo drives[MAX_DRIVES];
    int drive_count;
} FilesystemMap;

/* Functions */
void fs_init(FilesystemMap *fs_map);
void fs_list(FilesystemMap *fs_map);

#endif /* FS_H */