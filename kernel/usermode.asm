; User mode entry
; Switches from ring 0 (kernel) to ring 3 (user mode)

global enter_usermode

enter_usermode:
    ; Parameters:
    ; [esp+4] = entry point
    ; [esp+8] = user stack pointer
    
    mov eax, [esp+4]  ; Entry point
    mov ebx, [esp+8]  ; User stack
    
    ; Set up user data segment
    mov ax, 0x23      ; User data segment (GDT entry 4, RPL=3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; Push user stack segment
    push 0x23         ; SS (user data segment)
    push ebx          ; ESP (user stack)
    
    ; Push EFLAGS
    pushf
    pop eax
    or eax, 0x200     ; Enable interrupts
    push eax
    
    ; Push user code segment
    push 0x1B         ; CS (user code segment, GDT entry 3, RPL=3)
    
    ; Push entry point
    push dword [esp+20]  ; Entry point from original stack
    
    ; Return to user mode
    iret
