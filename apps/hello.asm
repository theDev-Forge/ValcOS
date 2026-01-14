[BITS 32]
[ORG 0]

start:
    ; Syscall 0 (Print)
    mov eax, 0          ; Syscall number
    mov ebx, msg        ; Address of string (Relative to load address?)
                        ; Since we use ORG 0, we might need position independent code 
                        ; or rely on finding our address.
                        ; BUT, if we just use relative addressing for data...
                        ; In flat binary 32-bit, `mov ebx, msg` usually uses absolute address.
                        ; If loaded at random address, `msg` will be wrong.
                        ; Trick: Use delta offset.
                        
    call get_eip
get_eip:
    pop ecx             ; ECX = Address of get_eip
    sub ecx, get_eip    ; ECX = Load Address (Start)
    
    lea ebx, [ecx + msg] ; Calculate real address of msg
    
    int 0x80
    
    ; Exit loop
    jmp $

msg db "Hello from Disk!", 0xA, 0
