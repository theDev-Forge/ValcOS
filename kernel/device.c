#include "device.h"
#include "memory.h"
#include "string.h"
#include "printk.h"

// Global device list
static LIST_HEAD(device_list);
static int next_major = 1;

void device_init(void) {
    pr_info("Device subsystem initialized\n");
}

int device_register(struct device *dev) {
    if (!dev) return -1;
    
    list_add_tail(&dev->list, &device_list);
    pr_info("Registered device: %s (type=%d, major=%d, minor=%d)\n",
            dev->name, dev->type, dev->major, dev->minor);
    
    return 0;
}

void device_unregister(struct device *dev) {
    if (!dev) return;
    
    list_del(&dev->list);
    pr_info("Unregistered device: %s\n", dev->name);
}

struct device *device_find(const char *name) {
    struct device *dev;
    
    list_for_each_entry(dev, &device_list, list) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }
    
    return NULL;
}

int register_chrdev(int major, const char *name, struct file_operations *fops) {
    struct device *dev = (struct device *)kmalloc(sizeof(struct device));
    if (!dev) return -1;
    
    // Allocate major number if not specified
    if (major == 0) {
        major = next_major++;
    }
    
    strncpy(dev->name, name, sizeof(dev->name) - 1);
    dev->name[sizeof(dev->name) - 1] = '\0';
    dev->type = DEV_CHAR;
    dev->major = major;
    dev->minor = 0;
    dev->fops = fops;
    dev->private_data = NULL;
    INIT_LIST_HEAD(&dev->list);
    
    device_register(dev);
    return major;
}
