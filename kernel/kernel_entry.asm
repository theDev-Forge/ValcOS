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

section .bss
    align 16
kernel_stack_bottom:
    resb 16384  ; 16 KB stack
kernel_stack_top:
