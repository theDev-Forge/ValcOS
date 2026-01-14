; input.asm - Interactive Test App
[BITS 32]
[ORG 0x400000] ; User mode load address

section .text
user_entry:
    ; Print prompt
    mov eax, 0          ; Syscall 0: PRINT
    mov ebx, prompt
    int 0x80

loop:
    ; Read char
    mov eax, 3          ; Syscall 3: READ
    mov ebx, 0          ; stdin
    mov ecx, buffer     ; buffer
    mov edx, 1          ; count
    int 0x80
    
    ; Check if read something (EAX > 0)
    cmp eax, 0
    jle loop
    
    ; Check for 'q' to quit
    mov al, [buffer]
    cmp al, 'q'
    je exit
    
    ; Echo back
    ; Loop Echo:
    mov eax, 0          ; Syscall 0: PRINT
    mov ebx, buffer
    int 0x80
    
    jmp loop

exit:
    mov eax, 1          ; Syscall 1: EXIT
    int 0x80

prompt db "Type characters (q to quit): ", 0
buffer db 0, 0 ; Char + Null terminator
