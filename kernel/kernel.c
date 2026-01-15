#include "vga.h"
#include "gdt.h"
#include "tss.h"
#include "idt.h"
#include "syscall.h"
#include "timer.h"
#include "keyboard.h"
#include "shell.h"
#include "memory.h"
#include "slab.h"
#include "ktimer.h"
#include "workqueue.h"
#include "signal.h"
#include "netdevice.h"
#include "socket.h"
#include "loopback.h"
#include "fs.h"
#include "blkdev.h"
#include "device.h"
#include "process.h"
#include "pmm.h"
#include "vmm.h"
#include "fat12.h"
#include "vga_gfx.h" // Keep this from original, as it's not explicitly removed and might be used elsewhere

void kernel_main(void) {
    // Initialize VGA
    vga_init();
    vga_print("ValcOS Kernel Starting...\n");
    
    // Initialize GDT
    vga_print("Initializing GDT...\n");
    init_gdt();
    
    // Initialize TSS
    vga_print("Initializing TSS...\n");
    init_tss();
    
    // Initialize IDT
    vga_print("Initializing IDT...\n");
    idt_init();
    
    // Initialize syscalls
    vga_print("Initializing Syscalls...\n");
    init_syscalls();
    
    // Initialize timer (100 Hz)
    vga_print("Initializing Timer...\n");
    init_timer(100);
    
    // Initialize kernel timer subsystem
    ktimer_subsystem_init();
    
    // Initialize signal subsystem
    signal_init();
    
    // Initialize PMM
    pmm_init(128 * 1024 * 1024); // 128 MB
    
    // Initialize VMM
    vmm_init();
    
    // Initialize memory (heap)
    memory_init();
    
    // Initialize slab allocator
    slab_init();
    
    // Initialize FAT12
    vga_print("Initializing FAT12...\n");
    fat12_init();
    
    // Initialize multitasking
    process_init();
    
    // Initialize work queue subsystem
    workqueue_init();
    
    // Initialize network subsystem
    netdev_init();
    socket_init();
    loopback_init();
    
    // Initialize VFS
    vfs_init();
    
    // Initialize block device subsystem
    blkdev_init();
    
    // Initialize device subsystem
    device_init();
    
    // Initialize keyboard
    vga_print("Initializing Keyboard...\n");
    keyboard_init();
    
    vga_print("\nValcOS Ready!\n\n");
    
    // Enable interrupts (Starts Multitasking via Timer)
    __asm__ volatile("sti");

    // Start shell
    shell_init();
    shell_run();
}
