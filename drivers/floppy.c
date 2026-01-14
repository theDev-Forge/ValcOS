#include "floppy.h"
#include "idt.h"
#include "timer.h" // For sleep
#include "vga.h"
#include "memory.h" // For DMA buffer allocation

// Standard Floppy Geometry (1.44MB)
#define SECTORS_PER_TRACK 18
#define HEADS 2

// DMA Buffer (must be below 16MB and 64K contiguous)
// For simplicity in PIO mode (if not using DMA), or using a static buffer.
// Actually, standard FDC uses DMA Channel 2.
// But writing a full DMA driver is complex.
// Let's us a simple "Polling" read if possible, or support DMA just enough.
// Alternatively, we can use a "DMA Buffer" in low memory (e.g. 0x1000) if we weren't using it for kernel.
// Let's use a static buffer in BSS for now, assuming we don't enable DMA controller yet.
// Wait, FDC *requires* DMA for data transfer usually.
// Implementing a proper DMA driver is strict.
//
// Short cut: For a simple OS, we can rely on the fact that we just boot up.
// OR, we can try to use BIOS interrupts? No, we are in Protected Mode.
//
// Okay, implementing a minimal FDC driver with DMA is the way.
//
// Let's allocate a DMA buffer in checks. 
static uint8_t *dma_buffer = (uint8_t*)0x1000; // Low memory locations

// FDC Commands
#define CMD_SPECIFY 0x03
#define CMD_RECALIBRATE 0x07
#define CMD_SENSE_INTERRUPT 0x08
#define CMD_READ_DATA 0xE6
#define CMD_SEEK 0x0F

static volatile int received_irq = 0;

// Interrupt Handler
extern void floppy_handler_asm(void);

void floppy_handler_c(void) {
    received_irq = 1;
    outb(0x20, 0x20); // EOI
}

static int floppy_wait_irq(void) {
    int timeout = 10000000; // Increased timeout
    while (!received_irq && timeout-- > 0);
    if (timeout <= 0) {
        vga_print("FDC: IRQ TIMEOUT!\n");
        return -1;
    }
    received_irq = 0;
    return 0;
}
static void fdc_send_byte(uint8_t byte) {
    for (int i = 0; i < 1000; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & 0x80) && !(msr & 0x40)) {
            outb(FDC_FIFO, byte);
            return;
        }
    }
    vga_print("FDC: Snd Timeout\n");
}

static int fdc_read_byte(void) {
    for (int i = 0; i < 1000; i++) {
        uint8_t msr = inb(FDC_MSR);
        if ((msr & 0x80) && (msr & 0x40)) {
            return inb(FDC_FIFO);
        }
    }
    vga_print("FDC: Rcv Timeout\n");
    return -1;
}

static void fdc_check_interrupt(int *st0, int *cyl) {
    fdc_send_byte(CMD_SENSE_INTERRUPT);
    *st0 = fdc_read_byte();
    *cyl = fdc_read_byte();
}

static void fdc_motor_on(void) {
    outb(FDC_DOR, 0x1C); // Drive 0, Motor 0, Enable, Reset
    timer_wait(20);      // Wait for motor spin up
}

static void fdc_motor_off(void) {
    outb(FDC_DOR, 0x0C);
}

void floppy_init(void) {
    // Register IRQ6
    idt_set_gate(38, (uint32_t)floppy_handler_asm, 0x08, 0x8E);
    
    // Unmask IRQ6
    outb(0x21, inb(0x21) & ~0x40);

    // Reset FDC
    outb(FDC_DOR, 0x00); // Disable
    outb(FDC_DOR, 0x0C); // Enable (IRQ/DMA)
    
    // Wait for reset interrupt
    if (floppy_wait_irq() < 0) {
        vga_print("Floppy: Reset Timeout\n");
        return; 
    }
    
    // Check interrupt status for 4 drives (Required by FDC after reset)
    int st0, cyl;
    for(int i=0; i<4; i++) {
        fdc_check_interrupt(&st0, &cyl);
    }
    
    // Set data rate (500Kbps)
    outb(FDC_CCR, 0x00);
    
    // Specify command (Step rate, Load time, Unload time, DMA enable)
    fdc_send_byte(CMD_SPECIFY);
    fdc_send_byte(0xDF); // SRT=D, HUT=F
    fdc_send_byte(0x02); // HLT=1, ND=0 (DMA enabled)
    
    // Recalibrate
    fdc_motor_on();
    fdc_send_byte(CMD_RECALIBRATE);
    fdc_send_byte(0); // Drive 0
    
    if (floppy_wait_irq() < 0) {
        vga_print("Floppy: Recal Timeout\n");
        fdc_motor_off();
        return;
    }
    
    fdc_check_interrupt(&st0, &cyl);
    fdc_motor_off();
}

// Minimal DMA setup for Channel 2
void dma_init(void *buffer, size_t length) {
    uint32_t address = (uint32_t)buffer;
    
    outb(0x0A, 0x06); // Mask DMA channel 2
    outb(0x0C, 0xFF); // Reset Flip-Flop
    outb(0x04, address & 0xFF);
    outb(0x04, (address >> 8) & 0xFF);
    
    // Page register (for >64K, usually 0x81)
    // Assuming low memory buffer for now < 64KB
    outb(0x81, (address >> 16) & 0xFF); 
    
    outb(0x0C, 0xFF);
    outb(0x05, (length - 1) & 0xFF);
    outb(0x05, ((length - 1) >> 8) & 0xFF);
    
    outb(0x0B, 0x46); // Mode: Read, Single, Decrement, Auto, Channel 2
    outb(0x0A, 0x02); // Unmask Channel 2
}

uint8_t *floppy_read_sector(int lba) {
    int cylinder = lba / (2 * 18);
    int head = (lba % (2 * 18)) / 18;
    int sector = (lba % 18) + 1;
    
    fdc_motor_on();
    
    // Fill buffer with 0xCC to detect DMA failure
    for(int i=0; i<512; i++) dma_buffer[i] = 0xCC;
    
    dma_init(dma_buffer, 512);
    
    // Seek
    fdc_send_byte(CMD_SEEK);
    fdc_send_byte(0); // Head 0, Drive 0
    fdc_send_byte(cylinder);
    
    if (floppy_wait_irq() < 0) {
        fdc_motor_off();
        return 0;
    }
    
    int st0, cyl;
    fdc_check_interrupt(&st0, &cyl);
    
    // Read Data
    fdc_send_byte(CMD_READ_DATA | 0x80 | 0x40); // MFM | MT
    fdc_send_byte((head << 2) | 0);
    fdc_send_byte(cylinder);
    fdc_send_byte(head);
    fdc_send_byte(sector);
    fdc_send_byte(2); // 512 bytes
    fdc_send_byte(18); // EOT (Sectors per track)
    fdc_send_byte(0x1B); // GAP3
    fdc_send_byte(0xFF); // DTL
    
    if (floppy_wait_irq() < 0) {
        vga_print("FDC: Read Timeout\n");
        fdc_motor_off();
        return 0;
    }
    
    // Read result status (7 bytes)
    st0 = fdc_read_byte();
    for(int i=0; i<6; i++) fdc_read_byte(); // Ignore rest for now
    
    // Check for error (Bits 6 and 7 of ST0)
    // 00 = Normal, 01 = Abnormal, 10 = Invalid, 11 = Terminated
    if ((st0 & 0xC0) != 0) {
        vga_print("FDC Err: ST0=");
        char c = '0' + (st0 >> 6);
        char s[2] = {c, 0};
        vga_print(s);
        vga_print("\n");
        fdc_motor_off();
        return 0; // Return NULL
    }
    
    fdc_motor_off();
    
    return dma_buffer;
}
