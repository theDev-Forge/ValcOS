[BITS 32]
global _switch_to_task
extern _current_process

; void switch_to_task(process_t *next_process);
; Switches from current_process to next_process
_switch_to_task:
    ; 1. Save current state
    pushf               ; Push EFLAGS
    pusha               ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI

    ; 2. Save current ESP to current_process->esp
    mov edi, [_current_process]  ; Get pointer to current_process
    mov [edi], esp               ; Save ESP to current_process->esp (offset 0)
    
    ; 3. Get next_process argument from stack
    ; Stack layout after pushf + pusha:
    ; [REGS-32bytes] [EFLAGS-4bytes] [RET-4bytes] [ARG-4bytes]
    ; So ARG is at esp + 32 + 4 + 4 = esp + 40
    mov esi, [esp + 40]          ; esi = next_process
    
    ; 4. Update current_process global to point to next
    mov [_current_process], esi
    
    ; 5. Load new ESP from next_process->esp
    mov esp, [esi]              ; Load next_process->esp (offset 0)
    
    ; 6. Restore new process state
    popa                        ; Pop registers
    popf                        ; Pop EFLAGS
    ret                         ; Return to new process

; Helper to jump to User Mode via IRET
; Stack should already have: [SS] [ESP] [EFLAGS] [CS] [EIP]
global _enter_user_mode
_enter_user_mode:
    ; Set up data segment registers for user mode
    ; IRET only loads CS and SS, we must manually set DS/ES/FS/GS
    mov ax, 0x23        ; User Data Selector (Index 4, RPL 3)
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    
    ; IRET will load CS, EIP, EFLAGS, SS, ESP from stack
    iretd
