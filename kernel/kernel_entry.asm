[BITS 32]
[EXTERN _kernel_main]

section .text
    global _kernel_entry
    
_kernel_entry:
    ; Set up stack
    mov esp, kernel_stack_top
    
    ; Call kernel main function
    call _kernel_main
    
    ; Hang if kernel returns
    cli
    hlt
    jmp $

; IDT loading function
global _idt_load
_idt_load:
    mov eax, [esp + 4]
    lidt [eax]
    ret

; Keyboard interrupt handler wrapper
global _keyboard_handler_asm
extern _keyboard_handler
_keyboard_handler_asm:
    pusha
    call _keyboard_handler
    popa
    iretd

; Timer interrupt handler wrapper
global _timer_handler_asm
extern _timer_handler
_timer_handler_asm:
    pusha
    call _timer_handler
    popa
    iretd

; GDT flush
global _gdt_flush
_gdt_flush:
    mov eax, [esp + 4]  ; Get GDT pointer arg
    lgdt [eax]          ; Load GDT

    mov ax, 0x10        ; Kernel Data Segment offset
    mov ds, ax
    mov es, ax
    mov fs, ax
    mov gs, ax
    mov ss, ax
    
    jmp 0x08:.flush     ; Far jump to reload CS (0x08)
.flush:
    ret

; System Call handler wrapper
global _syscall_handler_asm
extern _syscall_handler
_syscall_handler_asm:
    pusha               ; Save registers (EAX is syscall num)
    call _syscall_handler
    popa                ; Restore registers (EAX restored to original value, so no return val support yet)
    iretd

; Page Fault Handler (INT 14)
global _isr14
extern _page_fault_handler
_isr14:
    pusha
    mov eax, [esp + 32] ; Retrieve Error Code (below pusha)
    push eax            ; Push as argument
    call _page_fault_handler
    add esp, 4          ; Pop argument
    popa
    add esp, 4          ; Pop Error Code
    iretd

; GPF Handler (INT 13)
global _isr13
extern _gpf_handler
_isr13:
    pusha
    call _gpf_handler
    popa
    add esp, 4
    iretd

; Double Fault Handler (INT 8)
global _isr8
extern _double_fault_handler
_isr8:
    pusha
    call _double_fault_handler
    popa
    add esp, 4
    iretd



section .bss
    align 16
kernel_stack_bottom:
    resb 16384  ; 16 KB stack
kernel_stack_top:
