#ifndef SKBUFF_H
#define SKBUFF_H

#include <stdint.h>
#include "list.h"

/**
 * Socket Buffer (sk_buff) - Linux-inspired
 * 
 * Represents a network packet.
 */

struct net_device;

// Socket buffer structure
struct sk_buff {
    unsigned char *data;        // Packet data
    unsigned int len;           // Data length
    struct net_device *dev;     // Device
    struct list_head list;      // For queuing
};

// Socket buffer queue head
struct sk_buff_head {
    struct list_head list;
    unsigned int qlen;          // Queue length
};

/**
 * alloc_skb - Allocate socket buffer
 * @size: Size of data buffer
 */
struct sk_buff *alloc_skb(unsigned int size);

/**
 * free_skb - Free socket buffer
 * @skb: Buffer to free
 */
void free_skb(struct sk_buff *skb);

/**
 * skb_queue_head_init - Initialize sk_buff queue
 * @list: Queue head
 */
static inline void skb_queue_head_init(struct sk_buff_head *list) {
    INIT_LIST_HEAD(&list->list);
    list->qlen = 0;
}

/**
 * skb_queue_tail - Add buffer to tail of queue
 * @list: Queue head
 * @skb: Buffer to add
 */
void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *skb);

/**
 * skb_dequeue - Remove buffer from head of queue
 * @list: Queue head
 * Returns: Buffer or NULL if empty
 */
struct sk_buff *skb_dequeue(struct sk_buff_head *list);

#endif /* SKBUFF_H */
