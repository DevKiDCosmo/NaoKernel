/* fs/fileops.h - File operations header */
#ifndef FILEOPS_H
#define FILEOPS_H

/* File attributes (standard DOS) */
#define ATTR_READ_ONLY 0x01
#define ATTR_HIDDEN 0x02
#define ATTR_SYSTEM 0x04
#define ATTR_VOLUME_LABEL 0x08
#define ATTR_DIRECTORY 0x10
#define ATTR_ARCHIVE 0x20

/* Directory entry markers */
#define DIR_ENTRY_FREE 0x00
#define DIR_ENTRY_DELETED 0xE5

#define MAX_ROOT_ENTRIES 16
#define FAT_BLOCKS 2
#define ROOT_DIR_BLOCK 3
#define DATA_BLOCK_START 4
#define MAX_DATA_CLUSTERS 500  /* 500 data clusters available (blocks 4-503) */

/* Root directory entry (32 bytes) */
typedef struct {
    char name[8];           /* 8.3 format: FILENAME */
    char ext[3];            /* .EXT */
    unsigned char attributes;     /* File attributes */
    unsigned char reserved;
    unsigned char creation_time_10ms;
    unsigned short creation_time; /* Hours, minutes, seconds */
    unsigned short creation_date; /* Year, month, day */
    unsigned short last_access;   /* Date only */
    unsigned short high_cluster;  /* High word of cluster (0 for FAT12) */
    unsigned short write_time;    /* Last write time */
    unsigned short write_date;    /* Last write date */
    unsigned short start_cluster; /* Starting cluster number */
    unsigned int file_size;     /* File size in bytes */
} DirectoryEntry;

/* Filesystem operations */
int fileops_init(void);
int fileops_format(void);  /* Create empty filesystem */

/* Directory operations */
int fileops_list_root(DirectoryEntry **out_entries);  /* Return count, set *out_entries */
DirectoryEntry* fileops_find_entry(const char *name);

/* File operations */
int fileops_create_file(const char *name);
int fileops_delete_file(const char *name);
int fileops_read_file(const char *name, unsigned char *buffer, unsigned int max_size);
int fileops_write_file(const char *name, const unsigned char *data, unsigned int size);
int fileops_copy_file(const char *src_name, const char *dest_name);

/* Directory operations */
int fileops_create_dir(const char *name);

/* Utilities */
void fileops_format_name(const char *src, char *name_out, char *ext_out);

#endif /* FILEOPS_H */
