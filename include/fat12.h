#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>

// Error codes
typedef enum {
    FAT12_SUCCESS = 0,
    FAT12_NOT_FOUND = -1,
    FAT12_DISK_FULL = -2,
    FAT12_INVALID_NAME = -3,
    FAT12_ALREADY_EXISTS = -4,
    FAT12_IO_ERROR = -5,
    FAT12_NOT_A_FILE = -6,
    FAT12_NOT_A_DIR = -7
} fat12_error_t;

// Directory Entry Attribute Flags
#define ATTR_READ_ONLY  0x01
#define ATTR_HIDDEN     0x02
#define ATTR_SYSTEM     0x04
#define ATTR_VOLUME_ID  0x08
#define ATTR_DIRECTORY  0x10
#define ATTR_ARCHIVE    0x20

typedef struct {
    char name[8];
    char ext[3];
    uint8_t attributes;
    uint8_t reserved;
    uint8_t create_time_tenth;
    uint16_t create_time;
    uint16_t create_date;
    uint16_t last_access_date;
    uint16_t first_cluster_high;
    uint16_t last_mod_time;
    uint16_t last_mod_date;
    uint16_t first_cluster_low;
    uint32_t size;
} __attribute__((packed)) directory_entry_t;

// Initialization
void fat12_init(void);

// Directory operations
void fat12_list_directory(void);

// File operations
int fat12_read_file(const char *filename, uint8_t *buffer);
int fat12_create_file(const char *filename);
int fat12_write_file(const char *filename, const uint8_t *data, uint32_t size);
int fat12_append_file(const char *filename, const uint8_t *data, uint32_t size);
int fat12_delete_file(const char *filename);

// File info
int fat12_get_file_size(const char *filename);
int fat12_file_exists(const char *filename);

// Disk space
uint32_t fat12_get_free_space(void);
uint32_t fat12_get_total_space(void);

// Error handling
const char* fat12_get_error_string(int error);

#endif
