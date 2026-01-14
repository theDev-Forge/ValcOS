[BITS 32]
global _switch_to_task

; void switch_to_task(process_t *next_process);
; Switches from current_process to next_process
_switch_to_task:
    ; Get next_process argument (pushed on stack before call)
    mov eax, [esp + 4]  ; eax = next_process

    ; 1. Save current state
    pusha               ; Push EAX, ECX, EDX, EBX, ESP, EBP, ESI, EDI
    pushf               ; Push EFLAGS

    ; 2. Save ESP to current_process->esp
    ; We need to know who 'current_process' is.
    ; It's a C variable: extern process_t *current_process;
    extern _current_process
    mov edx, [_current_process] ; edx = current_process pointer
    mov [edx], esp      ; current_process->esp = current ESP

    ; 3. Load ESP from next_process->esp
    ; eax holds next_process pointer
    mov esp, [eax]      ; ESP = next_process->esp

    ; 4. Update current_process pointer
    mov [_current_process], eax

    ; 5. Restore state
    popf
    popa
    
    ret
