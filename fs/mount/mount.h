/* fs/mount/mount.h - Drive mounting header */
#ifndef MOUNT_H
#define MOUNT_H

#include "../fs.h"

/* Mount result codes */
typedef enum {
    MOUNT_SUCCESS = 0,
    MOUNT_ERROR_INVALID_DRIVE,
    MOUNT_ERROR_NOT_FORMATTED,
    MOUNT_ERROR_ALREADY_MOUNTED,
    MOUNT_ERROR_UNSUPPORTED_FS
} MountResult;

/* Mount point structure */
typedef struct {
    DriveInfo *drive;
    int is_mounted;
    char mount_point[16];  /* e.g., "ide0", "ide1" */
} MountPoint;

/* Global mount table */
#define MAX_MOUNTS 4
typedef struct {
    MountPoint mounts[MAX_MOUNTS];
    int current_mount;  /* Index of currently active mount, -1 if none */
} MountTable;

/* Functions */
void mount_init(MountTable *table);
MountResult mount_drive(MountTable *table, DriveInfo *drive);
MountResult unmount_drive(MountTable *table, int mount_index);
int is_drive_formatted(DriveInfo *drive);
const char* get_mount_result_string(MountResult result);
const char* get_current_prompt(MountTable *table);
int set_current_drive(MountTable *table, const char *drive_id);

#endif /* MOUNT_H */
