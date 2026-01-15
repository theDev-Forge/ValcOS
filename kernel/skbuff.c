#include "skbuff.h"
#include "memory.h"
#include "string.h"

struct sk_buff *alloc_skb(unsigned int size) {
    struct sk_buff *skb = (struct sk_buff *)kmalloc(sizeof(struct sk_buff));
    if (!skb) return NULL;
    
    skb->data = (unsigned char *)kmalloc(size);
    if (!skb->data) {
        kfree(skb);
        return NULL;
    }
    
    skb->len = 0;
    skb->dev = NULL;
    INIT_LIST_HEAD(&skb->list);
    
    return skb;
}

void free_skb(struct sk_buff *skb) {
    if (!skb) return;
    
    if (skb->data) {
        kfree(skb->data);
    }
    kfree(skb);
}

void skb_queue_tail(struct sk_buff_head *list, struct sk_buff *skb) {
    if (!list || !skb) return;
    
    list_add_tail(&skb->list, &list->list);
    list->qlen++;
}

struct sk_buff *skb_dequeue(struct sk_buff_head *list) {
    if (!list || list_empty(&list->list)) {
        return NULL;
    }
    
    struct sk_buff *skb = list_first_entry(&list->list, struct sk_buff, list);
    list_del(&skb->list);
    list->qlen--;
    
    return skb;
}
