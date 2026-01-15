#ifndef NETDEVICE_H
#define NETDEVICE_H

#include <stdint.h>
#include "list.h"

/**
 * Network Device Abstraction (Linux-inspired)
 * 
 * Simple network device layer for packet transmission.
 */

// Forward declarations
struct sk_buff;
struct net_device;

// Network device structure
struct net_device {
    char name[16];                                      // Device name (e.g., "lo", "eth0")
    int (*xmit)(struct sk_buff *skb, struct net_device *dev);  // Transmit function
    int (*recv)(struct sk_buff *skb);                   // Receive function
    struct list_head list;                              // Device list
    void *priv;                                         // Private data
};

/**
 * register_netdev - Register network device
 * @dev: Device to register
 */
int register_netdev(struct net_device *dev);

/**
 * unregister_netdev - Unregister network device
 * @dev: Device to unregister
 */
void unregister_netdev(struct net_device *dev);

/**
 * netdev_find - Find network device by name
 * @name: Device name
 */
struct net_device *netdev_find(const char *name);

/**
 * netdev_init - Initialize network device subsystem
 */
void netdev_init(void);

#endif /* NETDEVICE_H */
