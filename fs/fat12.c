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

// Total clusters available (FAT12 supports up to 4084 clusters)
#define TOTAL_CLUSTERS 4080

// ============================================================================
// HELPER FUNCTIONS (Refactored from duplicated code)
// ============================================================================

static uint8_t *get_sector_ptr(uint32_t lba) {
    if (lba == 0) return (uint8_t*)BOOT_SECTOR;
    return (uint8_t*)(RAMDISK_START + (lba - 1) * SECTOR_SIZE);
}

// Parse filename into FAT12 format (8.3, uppercase, space-padded)
static int fat12_parse_filename(const char *filename, char *fat_name) {
    // Initialize with spaces
    for(int n=0; n<11; n++) fat_name[n] = ' ';
    
    int i = 0, j = 0;
    
    // Process name part (up to 8 chars)
    while (filename[i] != '\0' && filename[i] != '.') {
        if (j >= 8) return FAT12_INVALID_NAME; // Name too long
        char c = filename[i];
        if (c >= 'a' && c <= 'z') c -= 32; // To uppercase
        fat_name[j++] = c;
        i++;
    }
    
    // Process extension part (up to 3 chars)
    if (filename[i] == '.') {
        i++;
        j = 8;
        while (filename[i] != '\0') {
            if (j >= 11) return FAT12_INVALID_NAME; // Extension too long
            char c = filename[i];
            if (c >= 'a' && c <= 'z') c -= 32; // To uppercase
            fat_name[j++] = c;
            i++;
        }
    }
    
    return FAT12_SUCCESS;
}

// Find a file in root directory, returns pointer to entry or NULL
static directory_entry_t* fat12_find_file(const char *filename) {
    char fat_name[11];
    if (fat12_parse_filename(filename, fat_name) != FAT12_SUCCESS) {
        return NULL;
    }
    
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        uint8_t *sector = get_sector_ptr(ROOT_DIR_START + i);
        directory_entry_t *entry = (directory_entry_t*)sector;
        
        for (int k = 0; k < SECTOR_SIZE / 32; k++) {
            if (entry[k].name[0] == 0x00) return NULL; // End of directory
            if ((uint8_t)entry[k].name[0] == 0xE5) continue; // Deleted entry
            
            // Compare 11 bytes
            int match = 1;
            for(int m=0; m<11; m++) {
                if(entry[k].name[m] != fat_name[m]) {
                    match = 0;
                    break;
                }
            }
            
            if (match) return &entry[k];
        }
    }
    
    return NULL;
}

// Get FAT entry (12-bit)
static uint16_t fat12_get_fat_entry(uint16_t cluster) {
    uint8_t *fat = get_sector_ptr(FAT_START);
    uint32_t offset = cluster + (cluster / 2);
    
    uint16_t val = *(uint16_t*)&fat[offset];
    
    if (cluster & 1) {
        return val >> 4;
    } else {
        return val & 0xFFF;
    }
}

// Set FAT entry (12-bit) and mirror to second FAT
static void fat12_set_fat_entry(uint16_t cluster, uint16_t value) {
    // Update FAT 1
    uint8_t *fat1 = get_sector_ptr(FAT_START);
    uint32_t offset = cluster + (cluster / 2);
    
    if (cluster & 1) {
        fat1[offset] = (fat1[offset] & 0x0F) | ((value << 4) & 0xF0);
        fat1[offset+1] = (value >> 4) & 0xFF;
    } else {
        fat1[offset] = value & 0xFF;
        fat1[offset+1] = (fat1[offset+1] & 0xF0) | ((value >> 8) & 0x0F);
    }
    
    // Mirror to FAT 2
    uint8_t *fat2 = get_sector_ptr(FAT_START + SECTORS_PER_FAT);
    fat2[offset] = fat1[offset];
    fat2[offset+1] = fat1[offset+1];
}

// Find first free cluster
static uint16_t fat12_find_free_cluster(void) {
    for(uint16_t i=2; i<TOTAL_CLUSTERS; i++) {
        if (fat12_get_fat_entry(i) == 0x000) {
            return i;
        }
    }
    return 0xFFFF; // Disk full
}

// Find free root directory entry
static directory_entry_t* fat12_find_free_root_entry(void) {
    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        uint8_t *sector = get_sector_ptr(ROOT_DIR_START + i);
        directory_entry_t *entry = (directory_entry_t*)sector;
        
        for (int j = 0; j < SECTOR_SIZE / 32; j++) {
            if ((uint8_t)entry[j].name[0] == 0x00 || (uint8_t)entry[j].name[0] == 0xE5) {
                return &entry[j];
            }
        }
    }
    return NULL;
}

// Free entire cluster chain
static void fat12_free_cluster_chain(uint16_t start_cluster) {
    uint16_t cluster = start_cluster;
    
    while (cluster >= 2 && cluster < 0xFF8) {
        uint16_t next = fat12_get_fat_entry(cluster);
        fat12_set_fat_entry(cluster, 0x000); // Mark as free
        cluster = next;
    }
}

// ============================================================================
// PUBLIC API FUNCTIONS
// ============================================================================

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
    directory_entry_t *found = fat12_find_file(filename);
    if (!found) return FAT12_NOT_FOUND;
    
    if (found->attributes == ATTR_DIRECTORY) return FAT12_NOT_A_FILE;
    
    uint16_t cluster = found->first_cluster_low;
    uint32_t offset = 0;
    
    while (cluster >= 2 && cluster < 0xFF8) {
        uint8_t *data = get_sector_ptr(DATA_START + (cluster - 2));
        memcpy(buffer + offset, data, SECTOR_SIZE);
        offset += SECTOR_SIZE;
        
        cluster = fat12_get_fat_entry(cluster);
    }
    
    return found->size;
}

int fat12_create_file(const char *filename) {
    // Check if file already exists
    if (fat12_find_file(filename) != NULL) {
        return FAT12_ALREADY_EXISTS;
    }
    
    directory_entry_t *entry = fat12_find_free_root_entry();
    if (!entry) return FAT12_DISK_FULL;
    
    // Parse filename
    char fat_name[11];
    int result = fat12_parse_filename(filename, fat_name);
    if (result != FAT12_SUCCESS) return result;
    
    // Initialize entry
    memset((uint8_t*)entry, 0, sizeof(directory_entry_t));
    memcpy(entry->name, fat_name, 8);
    memcpy(entry->ext, fat_name + 8, 3);
    
    entry->attributes = ATTR_ARCHIVE;
    entry->size = 0;
    entry->first_cluster_low = 0; // Empty file
    
    return FAT12_SUCCESS;
}

int fat12_write_file(const char *filename, const uint8_t *data, uint32_t size) {
    directory_entry_t *found = fat12_find_file(filename);
    if (!found) return FAT12_NOT_FOUND;
    
    if (found->attributes == ATTR_DIRECTORY) return FAT12_NOT_A_FILE;
    
    // Free old cluster chain if exists
    if (found->first_cluster_low >= 2) {
        fat12_free_cluster_chain(found->first_cluster_low);
        found->first_cluster_low = 0;
    }
    
    if (size == 0) {
        found->size = 0;
        return FAT12_SUCCESS;
    }
    
    // Allocate first cluster
    uint16_t first_cluster = fat12_find_free_cluster();
    if (first_cluster == 0xFFFF) return FAT12_DISK_FULL;
    
    found->first_cluster_low = first_cluster;
    fat12_set_fat_entry(first_cluster, 0xFFF); // Mark as end
    
    uint32_t written = 0;
    uint16_t current_cluster = first_cluster;
    
    while (written < size) {
        uint8_t *sector = get_sector_ptr(DATA_START + (current_cluster - 2));
        uint32_t chunk = size - written;
        if (chunk > SECTOR_SIZE) chunk = SECTOR_SIZE;
        
        memcpy(sector, data + written, chunk);
        if (chunk < SECTOR_SIZE) {
            // Zero out remainder
            memset(sector + chunk, 0, SECTOR_SIZE - chunk);
        }
        written += chunk;
        
        if (written < size) {
            // Need next cluster
            uint16_t new_cluster = fat12_find_free_cluster();
            if (new_cluster == 0xFFFF) {
                found->size = written;
                return FAT12_DISK_FULL; // Partial write
            }
            fat12_set_fat_entry(current_cluster, new_cluster);
            fat12_set_fat_entry(new_cluster, 0xFFF);
            current_cluster = new_cluster;
        }
    }
    
    found->size = written;
    return FAT12_SUCCESS;
}

int fat12_append_file(const char *filename, const uint8_t *data, uint32_t size) {
    directory_entry_t *found = fat12_find_file(filename);
    if (!found) return FAT12_NOT_FOUND;
    
    if (found->attributes == ATTR_DIRECTORY) return FAT12_NOT_A_FILE;
    
    // Find last cluster
    uint16_t cluster = found->first_cluster_low;
    if (cluster < 2) {
        // Empty file, just write
        return fat12_write_file(filename, data, size);
    }
    
    uint16_t last_cluster = cluster;
    while (cluster >= 2 && cluster < 0xFF8) {
        last_cluster = cluster;
        cluster = fat12_get_fat_entry(cluster);
    }
    
    // Append data
    uint32_t written = 0;
    uint16_t current_cluster = last_cluster;
    
    while (written < size) {
        uint16_t new_cluster = fat12_find_free_cluster();
        if (new_cluster == 0xFFFF) return FAT12_DISK_FULL;
        
        fat12_set_fat_entry(current_cluster, new_cluster);
        fat12_set_fat_entry(new_cluster, 0xFFF);
        
        uint8_t *sector = get_sector_ptr(DATA_START + (new_cluster - 2));
        uint32_t chunk = size - written;
        if (chunk > SECTOR_SIZE) chunk = SECTOR_SIZE;
        
        memcpy(sector, data + written, chunk);
        if (chunk < SECTOR_SIZE) {
            memset(sector + chunk, 0, SECTOR_SIZE - chunk);
        }
        written += chunk;
        current_cluster = new_cluster;
    }
    
    found->size += written;
    return FAT12_SUCCESS;
}

int fat12_delete_file(const char *filename) {
    directory_entry_t *found = fat12_find_file(filename);
    if (!found) return FAT12_NOT_FOUND;
    
    if (found->attributes == ATTR_DIRECTORY) return FAT12_NOT_A_DIR;
    
    // Free cluster chain
    if (found->first_cluster_low >= 2) {
        fat12_free_cluster_chain(found->first_cluster_low);
    }
    
    // Mark directory entry as deleted
    found->name[0] = 0xE5;
    
    return FAT12_SUCCESS;
}

int fat12_get_file_size(const char *filename) {
    directory_entry_t *found = fat12_find_file(filename);
    if (!found) return FAT12_NOT_FOUND;
    
    if (found->attributes == ATTR_DIRECTORY) return FAT12_NOT_A_FILE;
    
    return found->size;
}

int fat12_file_exists(const char *filename) {
    return fat12_find_file(filename) != NULL ? 1 : 0;
}

uint32_t fat12_get_free_space(void) {
    uint32_t free_clusters = 0;
    
    for (uint16_t i = 2; i < TOTAL_CLUSTERS; i++) {
        if (fat12_get_fat_entry(i) == 0x000) {
            free_clusters++;
        }
    }
    
    return free_clusters * SECTOR_SIZE;
}

uint32_t fat12_get_total_space(void) {
    return TOTAL_CLUSTERS * SECTOR_SIZE;
}

const char* fat12_get_error_string(int error) {
    switch (error) {
        case FAT12_SUCCESS: return "Success";
        case FAT12_NOT_FOUND: return "File not found";
        case FAT12_DISK_FULL: return "Disk full";
        case FAT12_INVALID_NAME: return "Invalid filename";
        case FAT12_ALREADY_EXISTS: return "File already exists";
        case FAT12_IO_ERROR: return "I/O error";
        case FAT12_NOT_A_FILE: return "Not a file";
        case FAT12_NOT_A_DIR: return "Not a directory";
        default: return "Unknown error";
    }
}
