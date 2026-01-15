#ifndef BLKDEV_H
#define BLKDEV_H

#include <stdint.h>
#include <stddef.h>
#include "list.h"

/**
 * Block Device & I/O Scheduler (Linux-inspired)
 */

// Forward declarations
struct block_device;
struct bio;

// Block I/O request
struct bio {
    uint32_t sector;            // Starting sector
    uint32_t size;              // Size in bytes
    char *data;                 // Data buffer
    int rw;                     // READ or WRITE
    struct list_head list;      // For queuing
};

// Request queue
struct request_queue {
    struct list_head queue;     // Queue of pending requests
    uint32_t qlen;              // Queue length
};

// Block device
struct block_device {
    char name[16];              // Device name
    uint32_t size;              // Size in sectors
    struct request_queue *queue; // Request queue
    int (*submit)(struct bio *bio); // Submit I/O
    struct list_head list;      // Device list
};

// I/O direction
#define READ  0
#define WRITE 1

/**
 * blkdev_init - Initialize block device subsystem
 */
void blkdev_init(void);

/**
 * blkdev_register - Register block device
 */
int blkdev_register(struct block_device *bdev);

/**
 * blkdev_submit_bio - Submit block I/O request
 */
int blkdev_submit_bio(struct block_device *bdev, struct bio *bio);

/**
 * blkdev_alloc_bio - Allocate bio structure
 */
struct bio *blkdev_alloc_bio(uint32_t sector, uint32_t size, int rw);

/**
 * blkdev_free_bio - Free bio structure
 */
void blkdev_free_bio(struct bio *bio);

#endif /* BLKDEV_H */
