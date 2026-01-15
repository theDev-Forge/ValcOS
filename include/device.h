#ifndef DEVICE_H
#define DEVICE_H

#include <stdint.h>
#include "list.h"
#include "fs.h"

/**
 * Device Driver Framework (Linux-inspired)
 */

// Device types
#define DEV_CHAR  1
#define DEV_BLOCK 2

// Device structure
struct device {
    char name[32];              // Device name
    int type;                   // Device type
    int major;                  // Major number
    int minor;                  // Minor number
    struct file_operations *fops; // File operations
    struct list_head list;      // Device list
    void *private_data;         // Driver private data
};

/**
 * device_init - Initialize device subsystem
 */
void device_init(void);

/**
 * device_register - Register device
 */
int device_register(struct device *dev);

/**
 * device_unregister - Unregister device
 */
void device_unregister(struct device *dev);

/**
 * device_find - Find device by name
 */
struct device *device_find(const char *name);

/**
 * register_chrdev - Register character device
 */
int register_chrdev(int major, const char *name, struct file_operations *fops);

#endif /* DEVICE_H */
