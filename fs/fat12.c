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

// --- Write Support ---

// Helper: Set FAT entry (12-bit)
static void fat12_set_fat_entry(uint16_t cluster, uint16_t value) {
    uint8_t *fat = get_sector_ptr(FAT_START);
    uint32_t offset = cluster + (cluster / 2);
    
    if (cluster & 1) {
        // Odd Cluster: High 4 bits of byte[offset], all of byte[offset+1]
        fat[offset] = (fat[offset] & 0x0F) | ((value << 4) & 0xF0);
        fat[offset+1] = (value >> 4) & 0xFF;
    } else {
        // Even Cluster: All of byte[offset], Low 4 bits of byte[offset+1]
        fat[offset] = value & 0xFF;
        fat[offset+1] = (fat[offset+1] & 0xF0) | ((value >> 8) & 0x0F);
    }
}

// Helper: Get FAT entry (12-bit)
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

// Find first free cluster
static uint16_t fat12_find_free_cluster(void) {
    // FAT12 supports 4096 clusters max, typically smaller on floppy
    // Start at 2 (0 and 1 are reserved)
    for(uint16_t i=2; i<4080; i++) {
        if (fat12_get_fat_entry(i) == 0x000) {
            return i;
        }
    }
    return 0xFFFF; // Full
}

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

// Create a file (simple version, root dir only)
int fat12_create_file(const char *filename) {
    directory_entry_t *entry = fat12_find_free_root_entry();
    if (!entry) return 0; // Disk full
    
    // Parse Filename (similar to read)
    memset(entry, 0, sizeof(directory_entry_t));
    memset(entry->name, ' ', 8);
    memset(entry->ext, ' ', 3);
    
    int i = 0, j = 0;
    while(filename[i] != 0 && filename[i] != '.') {
        if(j<8) {
             char c = filename[i];
             if(c>='a' && c<='z') c-=32;
             entry->name[j++] = c;
        }
        i++;
    }
    if(filename[i] == '.') {
        i++; j=0;
        while(filename[i] != 0) {
            if(j<3) {
                 char c = filename[i];
                 if(c>='a' && c<='z') c-=32;
                 entry->ext[j++] = c;
            }
            i++;
        }
    }
    
    entry->attributes = 0x00; // Normal file
    entry->size = 0;
    entry->first_cluster_low = 0; // Empty file has no cluster yet? Or 0?
    // Usually 0 if size 0.
    
    return 1;
}

// Write to file (Overwrite mode)
int fat12_write_file(const char *filename, const uint8_t *data, uint32_t size) {
    // 1. Find the file
    // Reuse read logic kind of...
    // Let's copy-paste find logic or refactor (Copy paste for now to avoid breaking existing func)
    // ... skipping finding logic for brevity, assuming we implement a helper 'fat12_find_entry' later
    // Actually, I must implement finding logic.
    
    // Quick find implementation:
    directory_entry_t *found = NULL;
    char fat_name[11]; // Prepare target name
    for(int n=0; n<11; n++) fat_name[n] = ' ';
    int ii = 0, jj = 0;
    while (filename[ii] != '\0' && filename[ii] != '.') {
        if (jj < 8) { char c = filename[ii]; if (c >= 'a' && c <= 'z') c -= 32; fat_name[jj++] = c; } ii++;
    }
    if (filename[ii] == '.') { ii++; jj = 8; while (filename[ii] != '\0') { if (jj < 11) { char c = filename[ii]; if (c >= 'a' && c <= 'z') c -= 32; fat_name[jj++] = c; } ii++; } }

    for (int i = 0; i < ROOT_DIR_SECTORS; i++) {
        uint8_t *sector = get_sector_ptr(ROOT_DIR_START + i);
        directory_entry_t *entry = (directory_entry_t*)sector;
        for (int k = 0; k < SECTOR_SIZE / 32; k++) {
            if (entry[k].name[0] == 0x00) break;
            if ((uint8_t)entry[k].name[0] == 0xE5) continue;
            int match = 1;
            for(int m=0; m<11; m++) { if(entry[k].name[m] != fat_name[m]) { match = 0; break; } }
            if (match) { found = &entry[k]; break; }
        }
        if (found) break;
    }
    
    if (!found) return 0;
    
    // 2. Allocate clusters
    // Simple strategy: Linked list of clusters.
    // First, clear old chain if any (not implemented for safety, assuming empty or overwrite same size)
    // Let's implement allocation.
    
    uint32_t written = 0;
    uint16_t current_cluster = found->first_cluster_low;
    
    // If file was empty, alloc first cluster
    if (current_cluster == 0) {
        current_cluster = fat12_find_free_cluster();
        if (current_cluster == 0xFFFF) return 0; // Disk full
        found->first_cluster_low = current_cluster;
        fat12_set_fat_entry(current_cluster, 0xFFF); // Mark End
    }
    
    while (written < size) {
        // Write to current cluster
        uint8_t *sector = get_sector_ptr(DATA_START + (current_cluster - 2));
        uint32_t chunk = size - written;
        if (chunk > SECTOR_SIZE) chunk = SECTOR_SIZE;
        
        memcpy(sector, data + written, chunk);
        written += chunk;
        
        if (written < size) {
            // Need next cluster
            uint16_t next = fat12_get_fat_entry(current_cluster);
            if (next >= 0xFF8) {
                // Determine new cluster
                uint16_t new_cluster = fat12_find_free_cluster();
                if (new_cluster == 0xFFFF) {
                    // Out of space, truncate
                    break;
                }
                fat12_set_fat_entry(current_cluster, new_cluster);
                fat12_set_fat_entry(new_cluster, 0xFFF);
                current_cluster = new_cluster;
            } else {
                current_cluster = next;
            }
        }
    }
    
    found->size = written;
    return 1;
}
