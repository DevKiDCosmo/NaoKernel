/* fs/fileops.c - File operations implementation */

#include "fileops.h"
#include "ramdisk.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);

/* Simple cluster allocation bitmap (1 bit per cluster) */
static unsigned char cluster_bitmap[MAX_DATA_CLUSTERS / 8 + 1];

/* Helper: Mark cluster as used */
static void mark_cluster_used(unsigned int cluster)
{
    if (cluster > 0 && cluster <= MAX_DATA_CLUSTERS) {
        cluster_bitmap[(cluster - 1) / 8] |= (1 << ((cluster - 1) % 8));
    }
}

/* Helper: Mark cluster as free */
static void mark_cluster_free(unsigned int cluster)
{
    if (cluster > 0 && cluster <= MAX_DATA_CLUSTERS) {
        cluster_bitmap[(cluster - 1) / 8] &= ~(1 << ((cluster - 1) % 8));
    }
}

/* Helper: Check if cluster is used */
static int is_cluster_used(unsigned int cluster)
{
    if (cluster == 0 || cluster > MAX_DATA_CLUSTERS) {
        return 1;  /* Invalid clusters are considered used */
    }
    return (cluster_bitmap[(cluster - 1) / 8] & (1 << ((cluster - 1) % 8))) != 0;
}

/* Helper: Allocate a free cluster */
static unsigned int allocate_cluster(void)
{
    unsigned int cluster;
    for (cluster = 1; cluster <= MAX_DATA_CLUSTERS; cluster++) {
        if (!is_cluster_used(cluster)) {
            mark_cluster_used(cluster);
            return cluster;
        }
    }
    return 0;  /* No free clusters */
}

/* Helper: Simple string operations */
static int strlen_local(const char *str)
{
    int len = 0;
    while (str[len]) len++;
    return len;
}

static void strcpy_local(char *dest, const char *src)
{
    while (*src) {
        *dest++ = *src++;
    }
    *dest = '\0';
}

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

static int strcmp_local(const char *s1, const char *s2)
{
    while (*s1 && (*s1 == *s2)) {
        s1++;
        s2++;
    }
    return (unsigned char)*s1 - (unsigned char)*s2;
}

/* Convert filename to 8.3 format */
void fileops_format_name(const char *src, char *name_out, char *ext_out)
{
    int i = 0;
    int j = 0;
    int dot_pos = -1;
    
    /* Find dot position */
    for (i = 0; src[i]; i++) {
        if (src[i] == '.') {
            dot_pos = i;
            break;
        }
    }
    
    /* Copy name part (up to 8 chars) */
    memset_local(name_out, ' ', 8);
    for (i = 0; i < 8 && src[i] && src[i] != '.'; i++) {
        if (src[i] >= 'a' && src[i] <= 'z') {
            name_out[i] = src[i] - 32;  /* Convert to uppercase */
        } else {
            name_out[i] = src[i];
        }
    }
    
    /* Copy extension part (up to 3 chars) */
    memset_local(ext_out, ' ', 3);
    if (dot_pos >= 0) {
        for (i = 0, j = dot_pos + 1; i < 3 && src[j]; i++, j++) {
            if (src[j] >= 'a' && src[j] <= 'z') {
                ext_out[i] = src[j] - 32;  /* Convert to uppercase */
            } else {
                ext_out[i] = src[j];
            }
        }
    }
}

/* Initialize filesystem */
int fileops_init(void)
{
    ramdisk_init();
    fileops_format();  /* Create empty filesystem */
    return 0;
}

/* Format filesystem: create empty FAT and root directory */
int fileops_format(void)
{
    unsigned char fat_block[BLOCK_SIZE];
    unsigned char root_block[BLOCK_SIZE];
    
    /* Block 1-2: FAT (initialize to 0) */
    memset_local(fat_block, 0, BLOCK_SIZE);
    ramdisk_write(1, fat_block);
    ramdisk_write(2, fat_block);
    
    /* Block 3: Root directory (initialize to 0) */
    memset_local(root_block, 0, BLOCK_SIZE);
    ramdisk_write(ROOT_DIR_BLOCK, root_block);
    
    kprint("  Ramdisk formatted (FAT12-style).\n");
    
    return 0;
}

/* List root directory entries */
int fileops_list_root(DirectoryEntry **out_entries)
{
    unsigned char *root;
    DirectoryEntry *entries;
    int count = 0;
    int i;
    
    root = ramdisk_get_block(ROOT_DIR_BLOCK);
    *out_entries = (DirectoryEntry*)root;
    entries = (DirectoryEntry*)root;
    
    /* Count non-empty entries */
    for (i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] != DIR_ENTRY_FREE && 
            entries[i].name[0] != DIR_ENTRY_DELETED) {
            count++;
        }
    }
    
    return count;
}

/* Find entry in root directory by name */
DirectoryEntry* fileops_find_entry(const char *name)
{
    DirectoryEntry *entries;
    char search_name[8];
    char search_ext[3];
    int i, j;
    int match;
    
    fileops_list_root(&entries);
    fileops_format_name(name, search_name, search_ext);
    
    for (i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] == DIR_ENTRY_FREE || entries[i].name[0] == DIR_ENTRY_DELETED) {
            continue;
        }
        
        /* Compare name and extension */
        match = 1;
        for (j = 0; j < 8; j++) {
            if (entries[i].name[j] != search_name[j]) {
                match = 0;
                break;
            }
        }
        if (match) {
            for (j = 0; j < 3; j++) {
                if (entries[i].ext[j] != search_ext[j]) {
                    match = 0;
                    break;
                }
            }
        }
        
        if (match) {
            return &entries[i];
        }
    }
    
    return (DirectoryEntry*)0;
}

/* Create empty file */
int fileops_create_file(const char *name)
{
    DirectoryEntry *entries;
    DirectoryEntry *free_entry = (DirectoryEntry*)0;
    int i;
    char fname[8];
    char fext[3];
    
    /* Check if file already exists */
    if (fileops_find_entry(name) != (DirectoryEntry*)0) {
        return -1;  /* File already exists */
    }
    
    /* Find free directory entry */
    fileops_list_root(&entries);
    
    for (i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] == DIR_ENTRY_FREE || entries[i].name[0] == DIR_ENTRY_DELETED) {
            free_entry = &entries[i];
            break;
        }
    }
    
    if (free_entry == (DirectoryEntry*)0) {
        return -1;  /* No free entries */
    }
    
    /* Format name */
    fileops_format_name(name, fname, fext);
    
    /* Fill directory entry */
    memcpy_local(free_entry->name, fname, 8);
    memcpy_local(free_entry->ext, fext, 3);
    free_entry->attributes = ATTR_ARCHIVE;
    free_entry->start_cluster = 0;
    free_entry->file_size = 0;
    
    return 0;
}

/* Delete file */
int fileops_delete_file(const char *name)
{
    DirectoryEntry *entry = fileops_find_entry(name);
    
    if (entry == (DirectoryEntry*)0) {
        return -1;  /* File not found */
    }
    
    /* Free the cluster if allocated */
    if (entry->start_cluster != 0) {
        mark_cluster_free(entry->start_cluster);
    }
    
    /* Mark entry as deleted */
    entry->name[0] = DIR_ENTRY_DELETED;
    
    return 0;
}

/* Read entire file into buffer */
int fileops_read_file(const char *name, unsigned char *buffer, unsigned int max_size)
{
    DirectoryEntry *entry = fileops_find_entry(name);
    unsigned char *block;
    unsigned int bytes_to_copy;
    unsigned int cluster;
    
    if (entry == (DirectoryEntry*)0 || (entry->attributes & ATTR_DIRECTORY)) {
        return -1;  /* File not found or is directory */
    }
    
    if (entry->file_size == 0) {
        return 0;  /* Empty file */
    }
    
    /* For simplicity, read from single cluster */
    cluster = entry->start_cluster;
    if (cluster == 0 || cluster > MAX_DATA_CLUSTERS) {
        return 0;  /* Invalid cluster */
    }
    
    block = ramdisk_get_block(DATA_BLOCK_START + cluster);
    bytes_to_copy = entry->file_size;
    if (bytes_to_copy > max_size) {
        bytes_to_copy = max_size;
    }
    
    memcpy_local(buffer, block, bytes_to_copy);
    
    return bytes_to_copy;
}

/* Write data to file */
int fileops_write_file(const char *name, const unsigned char *data, unsigned int size)
{
    DirectoryEntry *entry = fileops_find_entry(name);
    unsigned char *block;
    unsigned int cluster;
    
    if (entry == (DirectoryEntry*)0) {
        return -1;  /* File not found */
    }
    
    if (size > BLOCK_SIZE) {
        size = BLOCK_SIZE;  /* Limit to one block for simplicity */
    }
    
    /* Allocate cluster if needed */
    if (entry->start_cluster == 0) {
        cluster = allocate_cluster();
        if (cluster == 0) {
            return -1;  /* No free clusters */
        }
        entry->start_cluster = cluster;
    } else {
        cluster = entry->start_cluster;
    }
    
    if (cluster == 0 || cluster > MAX_DATA_CLUSTERS) {
        return -1;  /* Invalid cluster */
    }
    
    block = ramdisk_get_block(DATA_BLOCK_START + cluster);
    memcpy_local(block, data, size);
    
    entry->file_size = size;
    
    return size;
}

/* Copy file */
int fileops_copy_file(const char *src_name, const char *dest_name)
{
    unsigned char buffer[BLOCK_SIZE];
    int bytes_read;
    
    /* Check if source exists */
    if (fileops_find_entry(src_name) == (DirectoryEntry*)0) {
        return -1;  /* Source not found */
    }
    
    /* Check if destination already exists */
    if (fileops_find_entry(dest_name) != (DirectoryEntry*)0) {
        return -2;  /* Destination already exists */
    }
    
    /* Read source file */
    bytes_read = fileops_read_file(src_name, buffer, BLOCK_SIZE);
    if (bytes_read < 0) {
        return -3;  /* Read failed */
    }
    
    /* Create destination file */
    if (fileops_create_file(dest_name) < 0) {
        return -4;  /* Create failed */
    }
    
    /* Write to destination */
    if (fileops_write_file(dest_name, buffer, bytes_read) < 0) {
        fileops_delete_file(dest_name);  /* Clean up on failure */
        return -5;  /* Write failed */
    }
    
    return bytes_read;
}

/* Create directory */
int fileops_create_dir(const char *name)
{
    DirectoryEntry *entries;
    DirectoryEntry *free_entry = (DirectoryEntry*)0;
    char fname[8];
    char fext[3];
    int i;
    
    /* Check if already exists */
    if (fileops_find_entry(name) != (DirectoryEntry*)0) {
        return -1;  /* Already exists */
    }
    
    /* Find free directory entry */
    fileops_list_root(&entries);
    
    for (i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] == DIR_ENTRY_FREE || entries[i].name[0] == DIR_ENTRY_DELETED) {
            free_entry = &entries[i];
            break;
        }
    }
    
    if (free_entry == (DirectoryEntry*)0) {
        return -1;  /* No free entries */
    }
    
    /* Format name */
    fileops_format_name(name, fname, fext);
    
    /* Fill directory entry */
    memcpy_local(free_entry->name, fname, 8);
    memcpy_local(free_entry->ext, fext, 3);
    free_entry->attributes = ATTR_DIRECTORY;
    free_entry->file_size = 0;
    
    return 0;
}
