/* shell/fs_commands.c - Filesystem command implementations */

#include "../fs/fileops.h"
#include "../output/output.h"

/* External output functions */
extern void kprint(const char *str);
extern void kprint_newline(void);
extern void kprint_char(char c);

/* Helper: print unsigned integer */
static void kprint_uint(unsigned int num)
{
    char buf[12];
    int idx = 0;
    
    if (num == 0) {
        kprint("0");
        return;
    }
    
    /* Convert to string in reverse */
    while (num > 0) {
        buf[idx++] = '0' + (num % 10);
        num /= 10;
    }
    
    /* Print in correct order */
    while (idx > 0) {
        kprint_char(buf[--idx]);
    }
}

/* Helper: Skip leading spaces */
static char *skip_spaces(char *str)
{
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    return str;
}

/* Helper: Find first space */
static char *find_space(char *str)
{
    while (*str != '\0' && *str != ' ' && *str != '\t') {
        str++;
    }
    return str;
}

/* List files in directory */
void cmd_ls(char *args)
{
    DirectoryEntry *entries;
    int count;
    int i;
    int j;
    
    count = fileops_list_root(&entries);
    
    if (count == 0) {
        kprint("(empty)\n");
        return;
    }
    
    kprint("Files in root directory:\n");
    for (i = 0; i < MAX_ROOT_ENTRIES; i++) {
        if (entries[i].name[0] != DIR_ENTRY_FREE && entries[i].name[0] != DIR_ENTRY_DELETED) {
            /* Print name */
            for (j = 0; j < 8; j++) {
                if (entries[i].name[j] != ' ') {
                    kprint_char(entries[i].name[j]);
                }
            }
            
            /* Print extension if present */
            if (entries[i].ext[0] != ' ') {
                kprint_char('.');
                for (j = 0; j < 3; j++) {
                    if (entries[i].ext[j] != ' ') {
                        kprint_char(entries[i].ext[j]);
                    }
                }
            }
            
            /* Print directory marker or size */
            if (entries[i].attributes & ATTR_DIRECTORY) {
                kprint("/");
            } else {
                kprint("  ");
                kprint_uint(entries[i].file_size);
                kprint(" bytes");
            }
            
            kprint_newline();
        }
    }
}

/* Display file contents */
void cmd_cat(char *args)
{
    unsigned char buffer[512];
    int bytes;
    int i;
    char *filename;
    
    filename = skip_spaces(args);
    if (filename[0] == '\0') {
        kprint("Usage: cat <filename>\n");
        return;
    }
    
    bytes = fileops_read_file(filename, buffer, sizeof(buffer));
    
    if (bytes < 0) {
        kprint("Error: File not found or cannot be read\n");
        return;
    }
    
    if (bytes == 0) {
        kprint("(empty file)\n");
        return;
    }
    
    for (i = 0; i < bytes; i++) {
        if (buffer[i] == '\n') {
            kprint_newline();
        } else if (buffer[i] >= 32 && buffer[i] < 127) {
            kprint_char(buffer[i]);
        } else if (buffer[i] == '\t') {
            kprint("    ");  /* Tab as 4 spaces */
        }
    }
    kprint_newline();
}

/* Create file */
void cmd_touch(char *args)
{
    char *filename;
    
    filename = skip_spaces(args);
    if (filename[0] == '\0') {
        kprint("Usage: touch <filename>\n");
        return;
    }
    
    if (fileops_create_file(filename) == 0) {
        kprint("Created: ");
        kprint(filename);
        kprint_newline();
    } else {
        kprint("Error: Cannot create file (may already exist or directory full)\n");
    }
}

/* Create directory */
void cmd_mkdir(char *args)
{
    char *dirname;
    
    dirname = skip_spaces(args);
    if (dirname[0] == '\0') {
        kprint("Usage: mkdir <dirname>\n");
        return;
    }
    
    if (fileops_create_dir(dirname) == 0) {
        kprint("Created directory: ");
        kprint(dirname);
        kprint_newline();
    } else {
        kprint("Error: Cannot create directory\n");
    }
}

/* Delete file */
void cmd_rm(char *args)
{
    char *filename;
    
    filename = skip_spaces(args);
    if (filename[0] == '\0') {
        kprint("Usage: rm <filename>\n");
        return;
    }
    
    if (fileops_delete_file(filename) == 0) {
        kprint("Deleted: ");
        kprint(filename);
        kprint_newline();
    } else {
        kprint("Error: File not found\n");
    }
}

/* Copy file */
void cmd_cp(char *args)
{
    char *src;
    char *dest;
    char *space_pos;
    int result;
    
    src = skip_spaces(args);
    if (src[0] == '\0') {
        kprint("Usage: cp <source> <destination>\n");
        return;
    }
    
    space_pos = find_space(src);
    if (space_pos[0] == '\0') {
        kprint("Usage: cp <source> <destination>\n");
        return;
    }
    
    /* Temporarily null-terminate source */
    *space_pos = '\0';
    dest = skip_spaces(space_pos + 1);
    
    if (dest[0] == '\0') {
        *space_pos = ' ';  /* Restore space */
        kprint("Usage: cp <source> <destination>\n");
        return;
    }
    
    result = fileops_copy_file(src, dest);
    *space_pos = ' ';  /* Restore space */
    
    if (result >= 0) {
        kprint("Copied ");
        kprint_uint(result);
        kprint(" bytes from ");
        kprint(src);
        kprint(" to ");
        kprint(dest);
        kprint_newline();
    } else {
        kprint("Error: Copy failed (code ");
        kprint_uint(-result);
        kprint(")\n");
    }
}

/* Print current directory (stub for now) */
void cmd_pwd(void)
{
    kprint("/\n");  /* Only root directory implemented */
}

/* Change directory (stub for now) */
void cmd_cd(char *args)
{
    char *dirname;
    
    dirname = skip_spaces(args);
    if (dirname[0] == '\0' || (dirname[0] == '/' && dirname[1] == '\0')) {
        kprint("Changed to /\n");
    } else {
        kprint("Error: Subdirectories not yet supported\n");
    }
}

/* Write text to file */
void cmd_echo_to_file(char *args)
{
    char *text_start;
    char *redir_pos;
    char *filename;
    int text_len;
    int i;
    
    /* Find ">" redirect operator */
    redir_pos = args;
    while (*redir_pos && *redir_pos != '>') {
        redir_pos++;
    }
    
    if (*redir_pos != '>') {
        kprint("Usage: echo <text> > <filename>\n");
        return;
    }
    
    /* Extract text (before >) */
    text_start = skip_spaces(args);
    text_len = redir_pos - text_start;
    
    /* Remove trailing spaces from text */
    while (text_len > 0 && (text_start[text_len - 1] == ' ' || text_start[text_len - 1] == '\t')) {
        text_len--;
    }
    
    /* Extract filename (after >) */
    filename = skip_spaces(redir_pos + 1);
    if (filename[0] == '\0') {
        kprint("Usage: echo <text> > <filename>\n");
        return;
    }
    
    /* Find end of filename */
    i = 0;
    while (filename[i] && filename[i] != ' ' && filename[i] != '\t') {
        i++;
    }
    filename[i] = '\0';  /* Null-terminate filename */
    
    /* Create file if it doesn't exist */
    if (fileops_find_entry(filename) == 0) {
        if (fileops_create_file(filename) < 0) {
            kprint("Error: Cannot create file\n");
            return;
        }
    }
    
    /* Write text to file */
    if (fileops_write_file(filename, (unsigned char*)text_start, text_len) < 0) {
        kprint("Error: Write failed\n");
    } else {
        kprint("Wrote ");
        kprint_uint(text_len);
        kprint(" bytes to ");
        kprint(filename);
        kprint_newline();
    }
}
