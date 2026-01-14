[BITS 32]
[ORG 0x400000]

section .text
global _start

_start:
    ; Syscall: Print (EAX=0)
    ; Arguments: EBX = String Pointer
    mov eax, 0
    mov ebx, msg        ; Address of message
    int 0x80
    
    ; Syscall: Yield/Exit (Hang for now)
    jmp $

msg: db "Hello from Disk!", 0xA, 0
