#include "netdevice.h"
#include "skbuff.h"
#include "socket.h"
#include "printk.h"
#include "string.h"

/**
 * Loopback Network Device
 * 
 * Packets sent to loopback are immediately received.
 */

// Loopback transmit function
static int loopback_xmit(struct sk_buff *skb, struct net_device *dev) {
    if (!skb) return -1;
    
    pr_debug("Loopback: transmitting %d bytes\n", skb->len);
    
    // For loopback, immediately "receive" the packet
    // In a real implementation, this would go through the network stack
    // For now, we'll just echo it back (simplified)
    
    // Note: In a full implementation, we'd deliver to all bound sockets
    // For now, the socket layer handles this
    
    free_skb(skb);  // Free after "transmission"
    return 0;
}

// Loopback device structure
static struct net_device loopback_dev = {
    .name = "lo",
    .xmit = loopback_xmit,
    .recv = NULL,
    .priv = NULL
};

void loopback_init(void) {
    // Initialize list
    INIT_LIST_HEAD(&loopback_dev.list);
    
    // Register loopback device
    register_netdev(&loopback_dev);
    
    pr_info("Loopback device initialized\n");
}
