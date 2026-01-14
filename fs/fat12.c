#include "fat12.h"
#include "vga.h"
#include "string.h"

// Address where bootloader loads the image (starting from Sector 1)
#define RAMDISK_START 0x10000
#define BOOT_SECTOR   0x7C00

// FAT12 Config
#define SECTOR_SIZE 512
#define RESERVED_SECTORS 200
#define FAT_COUNT 2
#define SECTORS_PER_FAT 9
#define ROOT_ENTRIES 224

#define FAT_START (RESERVED_SECTORS)
#define ROOT_DIR_START (FAT_START + (FAT_COUNT * SECTORS_PER_FAT))
#define ROOT_DIR_SECTORS ((ROOT_ENTRIES * 32 + SECTOR_SIZE - 1) / SECTOR_SIZE)
#define DATA_START (ROOT_DIR_START + ROOT_DIR_SECTORS)

static uint8_t *get_sector_ptr(uint32_t lba) {
    if (lba == 0) return (uint8_t*)BOOT_SECTOR;
    // LBA 1 is at Offset 0 of logical ramdisk buffer
    return (uint8_t*)(RAMDISK_START + (lba - 1) * SECTOR_SIZE);
}

void fat12_init(void) {
    vga_print("Initializing Ramdisk FAT12...\n");
}

void fat12_list_directory(void) {
    vga_print("Files on Ramdisk:\n");
    
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        uint8_t *sector = get_sector_ptr(ROOT_DIR_START + i);
        directory_entry_t *entry = (directory_entry_t*)sector;
        
        for (int j = 0; j < SECTOR_SIZE / 32; j++) {
            if (entry[j].name[0] == 0x00) return; // End
            if ((uint8_t)entry[j].name[0] == 0xE5) continue; // Deleted
            
            if (entry[j].attributes != ATTR_VOLUME_ID && entry[j].attributes != ATTR_HIDDEN) {
                char name[13];
                int k = 0;
                for (int m = 0; m < 8; m++) {
                    if (entry[j].name[m] == ' ') break;
                    name[k++] = entry[j].name[m];
                }
                
                if (entry[j].attributes != ATTR_DIRECTORY) {
                    name[k++] = '.';
                    for (int m = 0; m < 3; m++) {
                        if (entry[j].ext[m] == ' ') break;
                        name[k++] = entry[j].ext[m];
                    }
                }
                name[k] = 0;
                
                vga_print(" - ");
                vga_print(name);
                if (entry[j].attributes == ATTR_DIRECTORY) vga_print("/");
                vga_print("\n");
            }
        }
    }
}

int fat12_read_file(const char *filename, uint8_t *buffer) {
    directory_entry_t *found = NULL;
    
    // Prepare FAT name (11 chars, space padded, uppercase)
    char fat_name[11];
    for(int n=0; n<11; n++) fat_name[n] = ' '; // memset replacement
    
    int i = 0, j = 0;
    // Process Name Part
    while (filename[i] != '\0' && filename[i] != '.') {
        if (j < 8) {
            char c = filename[i];
            if (c >= 'a' && c <= 'z') c -= 32; // To Upper
            fat_name[j++] = c;
        }
        i++;
    }
    // Process Extension Part
    if (filename[i] == '.') {
        i++;
        j = 8;
        while (filename[i] != '\0') {
            if (j < 11) {
                char c = filename[i];
                if (c >= 'a' && c <= 'z') c -= 32; // To Upper
                fat_name[j++] = c;
            }
            i++;
        }
    }
    
    // Scan Root
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        uint8_t *sector = get_sector_ptr(ROOT_DIR_START + i);
        directory_entry_t *entry = (directory_entry_t*)sector;
        
        for (int k = 0; k < SECTOR_SIZE / 32; k++) {
            if (entry[k].name[0] == 0x00) break;
            if ((uint8_t)entry[k].name[0] == 0xE5) continue;
            
            // Compare 11 bytes
            int match = 1;
            for(int m=0; m<11; m++) {
                if(entry[k].name[m] != fat_name[m]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) {
                found = &entry[k];
                break;
            }
        }
        if (found) break;
    }
    
    if (!found) return 0;
    
    uint16_t cluster = found->first_cluster_low;
    uint32_t offset = 0;
    
    // Use FAT table directly from memory
    uint8_t *fat = get_sector_ptr(FAT_START);
    
    while (cluster < 0xFF8) {
        uint8_t *data = get_sector_ptr(DATA_START + (cluster - 2));
        memcpy(buffer + offset, data, SECTOR_SIZE);
        offset += SECTOR_SIZE;
        
        // FAT Lookup
        uint16_t fat_offset = cluster + (cluster / 2);
        uint16_t val = *(uint16_t*)&fat[fat_offset];
        
        if (cluster & 1) {
            cluster = val >> 4;
        } else {
            cluster = val & 0xFFF;
        }
    }
    return found->size;
}
