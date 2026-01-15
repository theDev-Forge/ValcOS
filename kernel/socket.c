#include "socket.h"
#include "netdevice.h"
#include "memory.h"
#include "string.h"
#include "printk.h"

void socket_init(void) {
    pr_info("Socket subsystem initialized\n");
}

struct socket *socket_create(int domain, int type) {
    if (type != SOCK_DGRAM) {
        pr_warn("Only SOCK_DGRAM supported for now\n");
        return NULL;
    }
    
    struct socket *sock = (struct socket *)kmalloc(sizeof(struct socket));
    if (!sock) return NULL;
    
    sock->type = type;
    sock->bound = 0;
    sock->local_addr = 0;
    sock->local_port = 0;
    skb_queue_head_init(&sock->recv_queue);
    sock->dev = netdev_find("lo");  // Default to loopback
    
    return sock;
}

int socket_bind(struct socket *sock, uint16_t port) {
    if (!sock) return -1;
    
    sock->local_port = port;
    sock->bound = 1;
    
    pr_debug("Socket bound to port %d\n", port);
    return 0;
}

int socket_send(struct socket *sock, const void *buf, unsigned int len) {
    if (!sock || !buf || len == 0) return -1;
    if (!sock->dev) return -1;
    
    // Allocate sk_buff
    struct sk_buff *skb = alloc_skb(len);
    if (!skb) return -1;
    
    // Copy data
    memcpy(skb->data, buf, len);
    skb->len = len;
    skb->dev = sock->dev;
    
    // Transmit via device
    if (sock->dev->xmit) {
        int result = sock->dev->xmit(skb, sock->dev);
        if (result < 0) {
            free_skb(skb);
            return -1;
        }
    }
    
    return len;
}

int socket_recv(struct socket *sock, void *buf, unsigned int len) {
    if (!sock || !buf || len == 0) return -1;
    
    // Dequeue packet
    struct sk_buff *skb = skb_dequeue(&sock->recv_queue);
    if (!skb) return 0;  // No data
    
    // Copy data
    unsigned int copy_len = (skb->len < len) ? skb->len : len;
    memcpy(buf, skb->data, copy_len);
    
    free_skb(skb);
    return copy_len;
}

void socket_close(struct socket *sock) {
    if (!sock) return;
    
    // Free all queued packets
    struct sk_buff *skb;
    while ((skb = skb_dequeue(&sock->recv_queue)) != NULL) {
        free_skb(skb);
    }
    
    kfree(sock);
}
