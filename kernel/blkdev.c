#include "blkdev.h"
#include "memory.h"
#include "printk.h"
#include "string.h"

// Global list of block devices
static LIST_HEAD(blkdev_list);

void blkdev_init(void) {
    pr_info("Block device subsystem initialized\n");
}

int blkdev_register(struct block_device *bdev) {
    if (!bdev) return -1;
    
    list_add_tail(&bdev->list, &blkdev_list);
    pr_info("Registered block device: %s (%d sectors)\n", bdev->name, bdev->size);
    
    return 0;
}

struct bio *blkdev_alloc_bio(uint32_t sector, uint32_t size, int rw) {
    struct bio *bio = (struct bio *)kmalloc(sizeof(struct bio));
    if (!bio) return NULL;
    
    bio->sector = sector;
    bio->size = size;
    bio->rw = rw;
    bio->data = (char *)kmalloc(size);
    if (!bio->data) {
        kfree(bio);
        return NULL;
    }
    
    INIT_LIST_HEAD(&bio->list);
    return bio;
}

void blkdev_free_bio(struct bio *bio) {
    if (!bio) return;
    if (bio->data) kfree(bio->data);
    kfree(bio);
}

int blkdev_submit_bio(struct block_device *bdev, struct bio *bio) {
    if (!bdev || !bio) return -1;
    
    // Simple FIFO scheduling - just submit immediately
    if (bdev->submit) {
        return bdev->submit(bio);
    }
    
    return -1;
}
