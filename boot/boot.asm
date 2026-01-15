[BITS 16]
[ORG 0x7C00]

    jmp short start
    nop

    ; FAT12 BPB (BIOS Parameter Block)
    oem_name db "ValcOS  "
    bytes_per_sector dw 512
    sectors_per_cluster db 1
    reserved_sectors dw 1024    ; Reserved for Bootloader + Kernel
    fat_count db 2
    root_entries dw 224
    total_sectors dw 2880
    media_descriptor db 0xF0
    sectors_per_fat dw 9
    sectors_per_track dw 18
    head_count dw 2
    hidden_sectors dd 0
    total_sectors_big dd 0
    drive_number db 0
    reserved db 0
    signature db 0x29
    volume_id dd 0x12345678
    volume_label db "VALCOS     "
    file_system db "FAT12   "

start:
    ; Set up segments
    xor ax, ax
    mov ds, ax
    mov es, ax
    mov ss, ax
    mov sp, 0x7C00
    
    mov [boot_drive], dl ; Save boot drive
    
    ; Print loading message
    mov si, load_msg
    call print_string
    
    ; Load kernel from disk
    mov ax, 0x1000
    mov es, ax          ; ES = 0x1000 (Load to 0x10000)
    mov bx, 0           ; BX = 0
    mov dh, 0           ; Head 0
    mov dl, [boot_drive] ; Use saved boot drive
    mov cx, 0x0002      ; CH=0 (Cyl), CL=2 (Sector)
    mov si, 1024        ; Read 1024 sectors (Kernel + FS) to Ramdisk

read_loop:
    push cx
    push dx
    push bx
    
    mov di, 5           ; Retry count 5

.retry:
    mov ah, 0x02
    mov al, 1
    push di
    int 0x13
    pop di
    jnc .success        ; Success?
    
    ; Reset disk and retry
    xor ah, ah
    int 0x13
    dec di
    jnz .retry
    
    jmp disk_error      ; Failed after retries

.success:
    pop bx
    pop dx
    pop cx
    
    ; mov ah, 0x0E ; Debug dot
    ; mov al, '.'
    ; int 0x10

    ; Increment ES by 32 (0x20) * 16 = 512 bytes
    mov ax, es
    add ax, 0x20
    mov es, ax
    dec si              ; Decrement count
    jz read_done
    
    inc cl              ; Next sector
    cmp cl, 19
    jl read_loop
    
    mov cl, 1           ; Sector 1
    inc dh              ; Next head
    cmp dh, 2
    jl read_loop
    
    mov dh, 0           ; Head 0
    inc ch
    jmp read_loop

read_done:
    xor ax, ax
    mov es, ax          ; Reset ES to 0 for E820 map

    ; Get memory map (E820)
    mov di, 0x7004      ; Store map entries at 0x7004 (below bootloader)
    xor ebx, ebx        ; continuation context
    xor bp, bp          ; entry count
    mov edx, 0x534D4150 ; 'SMAP' signature
    mov eax, 0xE820
    mov ecx, 24         ; buffer size
    int 0x15
    jc memory_error     ; unsupported

mem_loop:
    inc bp              ; increment entry count
    add di, 24          ; point to next entry
    test ebx, ebx       ; if ebx is 0, list is done
    jz mem_done
    mov edx, 0x534D4150 ; 'SMAP' signature (reset required)
    mov eax, 0xE820
    mov ecx, 24
    int 0x15
    jnc mem_loop

mem_done:
    mov [0x7000], bp    ; store entry count at 0x7000
    
    ; Switch to protected mode
    cli
    lgdt [gdt_descriptor]
    
    mov eax, cr0
    or eax, 1
    mov cr0, eax
    
    jmp CODE_SEG:protected_mode

disk_error:
    mov si, error_msg
    call print_string
    jmp $

memory_error:
    mov si, mem_error_msg
    call print_string
    jmp $

print_string:
    lodsb
    or al, al
    jz done
    mov ah, 0x0E
    int 0x10
    jmp print_string
done:
    ret

error_msg db 'Disk read error!', 0
mem_error_msg db 'Memory map error!', 0
load_msg db 'Loading ValcOS...', 13, 10, 0
boot_drive db 0

; GDT (Global Descriptor Table)
gdt_start:
    ; Null descriptor
    dq 0

gdt_code:
    ; Code segment descriptor
    dw 0xFFFF       ; Limit
    dw 0x0000       ; Base (low)
    db 0x00         ; Base (middle)
    db 10011010b    ; Access byte
    db 11001111b    ; Flags + Limit (high)
    db 0x00         ; Base (high)

gdt_data:
    ; Data segment descriptor
    dw 0xFFFF
    dw 0x0000
    db 0x00
    db 10010010b
    db 11001111b
    db 0x00

gdt_end:

gdt_descriptor:
    dw gdt_end - gdt_start - 1
    dd gdt_start

CODE_SEG equ gdt_code - gdt_start
DATA_SEG equ gdt_data - gdt_start

[BITS 32]
protected_mode:
    ; Set up segments
    mov ax, DATA_SEG
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    mov esp, 0x90000
    
    ; Jump to kernel
    jmp 0x10000

; Boot signature
times 510-($-$$) db 0
dw 0xAA55
