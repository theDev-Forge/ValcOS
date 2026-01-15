#include "netdevice.h"
#include "printk.h"
#include "string.h"

// Global list of network devices
static LIST_HEAD(netdev_list);

void netdev_init(void) {
    pr_info("Network device subsystem initialized\n");
}

int register_netdev(struct net_device *dev) {
    if (!dev) return -1;
    
    // Add to device list
    list_add_tail(&dev->list, &netdev_list);
    
    pr_info("Registered network device: %s\n", dev->name);
    return 0;
}

void unregister_netdev(struct net_device *dev) {
    if (!dev) return;
    
    list_del(&dev->list);
    pr_info("Unregistered network device: %s\n", dev->name);
}

struct net_device *netdev_find(const char *name) {
    struct net_device *dev;
    
    list_for_each_entry(dev, &netdev_list, list) {
        if (strcmp(dev->name, name) == 0) {
            return dev;
        }
    }
    
    return NULL;
}
