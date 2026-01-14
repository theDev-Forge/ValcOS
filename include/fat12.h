#ifndef FAT12_H
#define FAT12_H

#include <stdint.h>

// Directory Entry Attribute Config
#define ATTR_READ_only  0x01
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

void fat12_init(void);
void fat12_list_directory(void);
int fat12_read_file(const char *filename, uint8_t *buffer);

#endif
