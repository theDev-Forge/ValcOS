import struct
import sys
import os

# Configuration
BOOTLOADER_FILE = "build/boot.bin"
KERNEL_FILE = "build/kernel.bin"
OUTPUT_FILE = "build/ValcOS.img"
SAMPLE_FILE = "hello.txt"
SAMPLE_CONTENT = "Hello from FAT12! This is a text file read from the disk."

# FAT12 Constants
SECTOR_SIZE = 512
SECTORS_PER_CLUSTER = 1
RESERVED_SECTORS = 200 # Must match bootloader
FAT_COUNT = 2
ROOT_ENTRIES = 224
TOTAL_SECTORS = 2880
MEDIA_DESCRIPTOR = 0xF0
SECTORS_PER_FAT = 9
SECTORS_PER_TRACK = 18
HEAD_COUNT = 2

def create_image():
    # 1. Create empty image
    image_size = TOTAL_SECTORS * SECTOR_SIZE
    data = bytearray(image_size)

    # 2. Write Bootloader (Sector 0)
    print(f"Reading bootloader from {BOOTLOADER_FILE}...")
    with open(BOOTLOADER_FILE, "rb") as f:
        bootloader = f.read()
        if len(bootloader) > 512:
            print(f"Warning: Bootloader size {len(bootloader)} > 512 bytes!")
        data[0:len(bootloader)] = bootloader

    # 3. Write Kernel (Reserved Sectors, starting at Sector 1)
    print(f"Reading kernel from {KERNEL_FILE}...")
    with open(KERNEL_FILE, "rb") as f:
        kernel = f.read()
        kernel_sectors = (len(kernel) + SECTOR_SIZE - 1) // SECTOR_SIZE
        print(f"Kernel size: {len(kernel)} bytes ({kernel_sectors} sectors)")
        
        if kernel_sectors > (RESERVED_SECTORS - 1):
             print(f"Error: Kernel too big for reserved area! Increase RESERVED_SECTORS in boot.asm and here.")
             return

        data[SECTOR_SIZE:SECTOR_SIZE + len(kernel)] = kernel

    # 4. Initialize FAT Tables
    # FAT Entry 0: Media Descriptor (0xF0) + FF
    # FAT Entry 1: End of Chain marker (0xFF) + F (packed as F0 FF FF)
    # Actually for FAT12:
    # Entry 0 = 0xF0
    # Entry 1 = 0xFF
    # Low 12 bits of Entry 2 (Cluster 2)
    # In bytes: F0 FF FF (matches Media + 2 reserved entries)
    
    fat_start = RESERVED_SECTORS * SECTOR_SIZE
    fat_size = SECTORS_PER_FAT * SECTOR_SIZE
    
    # Initialize both FATs with Empty clusters (0x00)
    # But first 2 entries are reserved
    fat_data = bytearray(fat_size)
    fat_data[0] = 0xF0
    fat_data[1] = 0xFF
    fat_data[2] = 0xFF
    
    # Write FAT1
    data[fat_start : fat_start + fat_size] = fat_data
    # Write FAT2
    data[fat_start + fat_size : fat_start + 2*fat_size] = fat_data

    # 5. Root Directory
    root_dir_start = fat_start + 2*fat_size
    root_dir_size = ROOT_ENTRIES * 32
    
    # Add Sample File
    file_cluster = 2 # First available cluster
    file_size = len(SAMPLE_CONTENT)
    
    # Create Directory Entry
    # Name (8), Ext (3), Attr (1), Res(10), Time(2), Date(2), Cluster(2), Size(4)
    name = b"HELLO   TXT"
    attr = 0x20 # Archive
    cluster = struct.pack("<H", file_cluster)
    size = struct.pack("<I", file_size)
    
    entry = name + struct.pack("B", attr) + b'\x00'*10 + b'\x00\x00\x00\x00' + cluster + size
    
    # Write to first entry in Root Dir
    data[root_dir_start : root_dir_start + 32] = entry
    
    # 6. Write File Content
    data_region_start = root_dir_start + root_dir_size
    # Cluster 2 is the first cluster in data region.
    # Data region index 0 corresponds to Cluster 2.
    file_offset = data_region_start + (file_cluster - 2) * SECTORS_PER_CLUSTER * SECTOR_SIZE
    
    data[file_offset : file_offset + file_size] = SAMPLE_CONTENT.encode('utf-8')
    
    # Mark Cluster 2 as End of Chain (0xFFF) in FAT
    # Entry 2 is at byte offset (2 * 12bits) / 8 = 3 bytes.
    # 0 -> F0 FF FF (Entries 0 and 1)
    # Entry 2 starts at nibble 0 of byte 3? 
    # FAT12 packing:
    # Byte 0: Entry 0 (low 8) -> F0
    # Byte 1: Entry 0 (high 4) | Entry 1 (low 4) -> F | F -> FF
    # Byte 2: Entry 1 (high 8) -> FF
    # Byte 3: Entry 2 (low 8)
    # Byte 4: Entry 2 (high 4) | Entry 3 (low 4)
    
    # We want Entry 2 to be 0xFFF.
    # Offset 3 (byte index 3) is 0x00 by default.
    # We need to write 0xFF to Byte 3, and 0x0F to low nibble of Byte 4.
    
    # Updating FAT copy in memory buffer
    fat_offset = fat_start
    data[fat_offset + 3] = 0xFF
    data[fat_offset + 4] = 0x0F # Low nibble F, high nibble 0 (Entry 3, which is free)
    
    # Update FAT2 as well
    fat2_offset = fat_start + fat_size
    data[fat2_offset + 3] = 0xFF
    data[fat2_offset + 4] = 0x0F

    # 7. Write Image
    print(f"Writing disk image to {OUTPUT_FILE}...")
    with open(OUTPUT_FILE, "wb") as f:
        f.write(data)
    print("Done!")

if __name__ == "__main__":
    create_image()
