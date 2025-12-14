/* fs/format/format.h - Drive formatting header */
#ifndef FORMAT_H
#define FORMAT_H

#include "../fs.h"

/* Format result codes */
typedef enum {
    FORMAT_SUCCESS = 0,
    FORMAT_ERROR_INVALID_DRIVE,
    FORMAT_ERROR_WRITE_FAILED,
    FORMAT_ERROR_UNSUPPORTED,
    FORMAT_ERROR_TOO_LARGE
} FormatResult;

/* Media type detection */
typedef enum {
    MEDIA_TYPE_FLOPPY_360KB,   /* 5.25" DD */
    MEDIA_TYPE_FLOPPY_720KB,   /* 3.5" DD */
    MEDIA_TYPE_FLOPPY_1_2MB,   /* 5.25" HD */
    MEDIA_TYPE_FLOPPY_1_44MB,  /* 3.5" HD */
    MEDIA_TYPE_FLOPPY_2_88MB,  /* 3.5" ED */
    MEDIA_TYPE_HDD_SMALL,      /* < 32MB - FAT12 */
    MEDIA_TYPE_HDD_MEDIUM,     /* 32MB - 512MB - FAT16 */
    MEDIA_TYPE_HDD_LARGE       /* > 512MB - FAT32 */
} MediaType;

/* Format options */
typedef struct {
    char volume_label[12];     /* 11 chars + null */
    int quick_format;          /* 1 = quick, 0 = full (zero all sectors) */
} FormatOptions;

/* Function declarations */
MediaType detect_media_type(DriveInfo *drive);
FormatResult format_drive(DriveInfo *drive, FormatOptions *options);
const char* get_format_result_string(FormatResult result);

#endif /* FORMAT_H */
