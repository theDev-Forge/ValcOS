#ifndef FLOPPY_H
#define FLOPPY_H

#include <stdint.h>

// Floppy Controller Ports
#define FDC_DOR  0x3F2
#define FDC_MSR  0x3F4
#define FDC_FIFO 0x3F5
#define FDC_CCR  0x3F7

// Functions
void floppy_init(void);
uint8_t *floppy_read_sector(int lba);

#endif
