import struct
import sys
import os

# Configuration
BOOTLOADER_FILE = "build/boot.bin"
KERNEL_FILE = "build/kernel.bin"
OUTPUT_FILE = "build/ValcOS.img"
ROOT_DIR = "rootfs"

# FAT12 Constants
SECTOR_SIZE = 512
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS = 1024 # Must match bootloader
FAT_COUNT = 2
ROOT_ENTRIES = 224
TOTAL_SECTORS = 2880
MEDIA_DESCRIPTOR = 0xF0
SECTORS_PER_FAT = 9

class FAT12:
    def __init__(self, filename):
        self.filename = filename
        self.data = bytearray(TOTAL_SECTORS * SECTOR_SIZE)
        self.fat_start = RESERVED_SECTORS * SECTOR_SIZE
        self.fat_size = SECTORS_PER_FAT * SECTOR_SIZE
        self.root_dir_start = self.fat_start + (FAT_COUNT * self.fat_size)
        self.root_dir_size = ROOT_ENTRIES * 32
        self.data_start = self.root_dir_start + self.root_dir_size
        self.current_cluster = 2

    def format(self):
        # 1. Write Bootloader
        if os.path.exists(BOOTLOADER_FILE):
            print(f"Reading bootloader from {BOOTLOADER_FILE}...")
            with open(BOOTLOADER_FILE, "rb") as f:
                boot = f.read()
                if len(boot) > 512:
                    print("Warning: Bootloader > 512 bytes")
                self.data[0:len(boot)] = boot
        else:
            print(f"Warning: {BOOTLOADER_FILE} not found")

        # 2. Write Kernel
        if os.path.exists(KERNEL_FILE):
            print(f"Reading kernel from {KERNEL_FILE}...")
            with open(KERNEL_FILE, "rb") as f:
                kernel = f.read()
                k_sectors = (len(kernel) + SECTOR_SIZE - 1) // SECTOR_SIZE
                print(f"Kernel size: {len(kernel)} bytes ({k_sectors} sectors)")
                if k_sectors > (RESERVED_SECTORS - 1):
                    raise Exception("Kernel too big for reserved area")
                self.data[SECTOR_SIZE:SECTOR_SIZE+len(kernel)] = kernel
        else:
            print(f"Warning: {KERNEL_FILE} not found")

        # 3. Initialize FATs
        # Entry 0: Media (F0) + FF
        # Entry 1: End Chain (FF) + High nibble of Entry 1 usually? 
        # Standard: F0 FF FF
        self.data[self.fat_start] = 0xF0
        self.data[self.fat_start+1] = 0xFF
        self.data[self.fat_start+2] = 0xFF
        
        # Copy to FAT2
        self.data[self.fat_start+self.fat_size : self.fat_start+self.fat_size+3] = self.data[self.fat_start:self.fat_start+3]

    def add_file(self, filename, content):
        # Find free directory entry
        entry_idx = -1
        for i in range(ROOT_ENTRIES):
            offset = self.root_dir_start + (i * 32)
            if self.data[offset] == 0:
                entry_idx = i
                break
        
        if entry_idx == -1:
            raise Exception("Root directory full")

        file_size = len(content)
        start_cluster = self.current_cluster
        
        # Calculate valid 8.3 filename
        # Basic conversion
        upper_name = filename.upper()
        name_parts = upper_name.split('.')
        base = name_parts[0][:8].ljust(8)
        ext = name_parts[1][:3].ljust(3) if len(name_parts) > 1 else "   "
        
        dos_name = (base + ext).encode('ascii')
        
        # Helper to pack
        # Name(11), Attr(1), Res(10), Time(2), Date(2), Cluster(2), Size(4)
        entry_offset = self.root_dir_start + (entry_idx * 32)
        
        self.data[entry_offset:entry_offset+11] = dos_name
        self.data[entry_offset+11] = 0x20 # Archive
        struct.pack_into("<H", self.data, entry_offset+26, start_cluster)
        struct.pack_into("<I", self.data, entry_offset+28, file_size)

        # Write Data
        # Assume contiguous for now
        clusters_needed = (file_size + SECTOR_SIZE - 1) // SECTOR_SIZE
        
        for i in range(clusters_needed):
            cluster_num = start_cluster + i
            
            # Write FAT entry
            is_last = (i == clusters_needed - 1)
            next_cluster = 0xFFF if is_last else (cluster_num + 1)
            
            self.set_fat_entry(cluster_num, next_cluster)
            
            # Write Data Sector
            # Cluster 2 is at offset 0 of data area
            data_offset = self.data_start + ((cluster_num - 2) * SECTOR_SIZE)
            
            chunk = content[i*SECTOR_SIZE : (i+1)*SECTOR_SIZE]
            self.data[data_offset:data_offset+len(chunk)] = chunk

        self.current_cluster += clusters_needed

    def set_fat_entry(self, cluster, value):
        # FAT12: 12 bits per entry.
        # Offset = floor(cluster * 1.5)
        offset = self.fat_start + int(cluster * 1.5)
        
        # Read 16 bits
        val = struct.unpack_from("<H", self.data, offset)[0]
        
        if cluster % 2 == 0:
            # Low 12 bits
            val = (val & 0xF000) | (value & 0x0FFF)
        else:
            # High 12 bits
            val = (val & 0x000F) | ((value & 0x0FFF) << 4)
            
        struct.pack_into("<H", self.data, offset, val)
        # Mirror to FAT2
        struct.pack_into("<H", self.data, offset + self.fat_size, val)

    def save(self):
        print(f"Writing disk image to {OUTPUT_FILE}...")
        with open(OUTPUT_FILE, "wb") as f:
            f.write(self.data)

if __name__ == "__main__":
    fs = FAT12(OUTPUT_FILE)
    fs.format()
    
    if not os.path.exists(ROOT_DIR):
        os.makedirs(ROOT_DIR)
        with open(os.path.join(ROOT_DIR, "README.TXT"), "w") as f:
            f.write("Welcome to ValcOS!\n")

    print(f"Scanning {ROOT_DIR}...")
    for fname in os.listdir(ROOT_DIR):
        fpath = os.path.join(ROOT_DIR, fname)
        if os.path.isfile(fpath):
            print(f"Adding {fname}...")
            with open(fpath, "rb") as f:
                fs.add_file(fname, f.read())

    fs.save()
