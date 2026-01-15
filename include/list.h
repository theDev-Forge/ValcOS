#ifndef LIST_H
#define LIST_H

#include <stddef.h>

/**
 * Linux-Style Intrusive Doubly-Linked Circular Lists
 * 
 * This implementation is adapted from the Linux kernel's list.h
 * License: GPL v2 (compatible with ValcOS)
 * 
 * Key Concepts:
 * - Intrusive: list_head is embedded directly in your data structure
 * - Doubly-linked: each node has prev and next pointers
 * - Circular: head->prev points to tail, tail->next points to head
 * - Type-safe: use container_of() to get parent structure
 * 
 * Example Usage:
 * 
 *   struct my_data {
 *       int value;
 *       struct list_head list;  // Embed list node
 *   };
 * 
 *   LIST_HEAD(my_list);  // Create list head
 * 
 *   struct my_data *item = kmalloc(sizeof(*item));
 *   item->value = 42;
 *   list_add(&item->list, &my_list);  // Add to list
 * 
 *   struct my_data *pos;
 *   list_for_each_entry(pos, &my_list, list) {
 *       printk("Value: %d\n", pos->value);
 *   }
 */

/**
 * struct list_head - Doubly-linked list node
 * @next: pointer to next node
 * @prev: pointer to previous node
 */
struct list_head {
    struct list_head *next, *prev;
};

/**
 * LIST_HEAD_INIT - Static initializer for list_head
 * @name: name of the list_head variable
 */
#define LIST_HEAD_INIT(name) { &(name), &(name) }

/**
 * LIST_HEAD - Declare and initialize a list head
 * @name: name of the list_head variable
 */
#define LIST_HEAD(name) \
    struct list_head name = LIST_HEAD_INIT(name)

/**
 * INIT_LIST_HEAD - Initialize a list_head structure
 * @list: list_head structure to initialize
 * 
 * Initializes the list to point to itself (empty circular list)
 */
static inline void INIT_LIST_HEAD(struct list_head *list)
{
    list->next = list;
    list->prev = list;
}

/**
 * __list_add - Internal: Insert a new entry between two known consecutive entries
 * @new: new entry to be added
 * @prev: previous entry
 * @next: next entry
 * 
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_add(struct list_head *new,
                              struct list_head *prev,
                              struct list_head *next)
{
    next->prev = new;
    new->next = next;
    new->prev = prev;
    prev->next = new;
}

/**
 * list_add - Add a new entry after the specified head
 * @new: new entry to be added
 * @head: list head to add it after
 * 
 * Insert a new entry after the specified head.
 * This is good for implementing stacks.
 */
static inline void list_add(struct list_head *new, struct list_head *head)
{
    __list_add(new, head, head->next);
}

/**
 * list_add_tail - Add a new entry before the specified head
 * @new: new entry to be added
 * @head: list head to add it before
 * 
 * Insert a new entry before the specified head.
 * This is useful for implementing queues.
 */
static inline void list_add_tail(struct list_head *new, struct list_head *head)
{
    __list_add(new, head->prev, head);
}

/**
 * __list_del - Internal: Delete a list entry by making the prev/next entries point to each other
 * @prev: previous entry
 * @next: next entry
 * 
 * This is only for internal list manipulation where we know
 * the prev/next entries already!
 */
static inline void __list_del(struct list_head *prev, struct list_head *next)
{
    next->prev = prev;
    prev->next = next;
}

/**
 * list_del - Delete entry from list
 * @entry: the element to delete from the list
 * 
 * Note: list_empty() on entry does not return true after this, the entry is
 * in an undefined state.
 */
static inline void list_del(struct list_head *entry)
{
    __list_del(entry->prev, entry->next);
    entry->next = NULL;
    entry->prev = NULL;
}

/**
 * list_replace - Replace old entry by new one
 * @old: the element to be replaced
 * @new: the new element to insert
 */
static inline void list_replace(struct list_head *old, struct list_head *new)
{
    new->next = old->next;
    new->next->prev = new;
    new->prev = old->prev;
    new->prev->next = new;
}

/**
 * list_move - Delete from one list and add as another's head
 * @list: the entry to move
 * @head: the head that will precede our entry
 */
static inline void list_move(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add(list, head);
}

/**
 * list_move_tail - Delete from one list and add as another's tail
 * @list: the entry to move
 * @head: the head that will follow our entry
 */
static inline void list_move_tail(struct list_head *list, struct list_head *head)
{
    __list_del(list->prev, list->next);
    list_add_tail(list, head);
}

/**
 * list_empty - Test whether a list is empty
 * @head: the list to test
 */
static inline int list_empty(const struct list_head *head)
{
    return head->next == head;
}

/**
 * list_is_singular - Test whether a list has exactly one entry
 * @head: the list to test
 */
static inline int list_is_singular(const struct list_head *head)
{
    return !list_empty(head) && (head->next == head->prev);
}

/**
 * container_of - Cast a member of a structure out to the containing structure
 * @ptr: the pointer to the member
 * @type: the type of the container struct this is embedded in
 * @member: the name of the member within the struct
 * 
 * This is the magic that makes intrusive lists work!
 * Given a pointer to list_head, get the containing structure.
 */
#define container_of(ptr, type, member) ({                      \
    const typeof( ((type *)0)->member ) *__mptr = (ptr);        \
    (type *)( (char *)__mptr - offsetof(type, member) );})

/**
 * list_entry - Get the struct for this entry
 * @ptr: the &struct list_head pointer
 * @type: the type of the struct this is embedded in
 * @member: the name of the list_head within the struct
 */
#define list_entry(ptr, type, member) \
    container_of(ptr, type, member)

/**
 * list_first_entry - Get the first element from a list
 * @ptr: the list head to take the element from
 * @type: the type of the struct this is embedded in
 * @member: the name of the list_head within the struct
 * 
 * Note: list must not be empty
 */
#define list_first_entry(ptr, type, member) \
    list_entry((ptr)->next, type, member)

/**
 * list_last_entry - Get the last element from a list
 * @ptr: the list head to take the element from
 * @type: the type of the struct this is embedded in
 * @member: the name of the list_head within the struct
 * 
 * Note: list must not be empty
 */
#define list_last_entry(ptr, type, member) \
    list_entry((ptr)->prev, type, member)

/**
 * list_next_entry - Get the next element in list
 * @pos: the type * to cursor
 * @member: the name of the list_head within the struct
 */
#define list_next_entry(pos, member) \
    list_entry((pos)->member.next, typeof(*(pos)), member)

/**
 * list_prev_entry - Get the previous element in list
 * @pos: the type * to cursor
 * @member: the name of the list_head within the struct
 */
#define list_prev_entry(pos, member) \
    list_entry((pos)->member.prev, typeof(*(pos)), member)

/**
 * list_for_each - Iterate over a list
 * @pos: the &struct list_head to use as a loop cursor
 * @head: the head for your list
 */
#define list_for_each(pos, head) \
    for (pos = (head)->next; pos != (head); pos = pos->next)

/**
 * list_for_each_safe - Iterate over a list safe against removal of list entry
 * @pos: the &struct list_head to use as a loop cursor
 * @n: another &struct list_head to use as temporary storage
 * @head: the head for your list
 */
#define list_for_each_safe(pos, n, head) \
    for (pos = (head)->next, n = pos->next; pos != (head); \
         pos = n, n = pos->next)

/**
 * list_for_each_entry - Iterate over list of given type
 * @pos: the type * to use as a loop cursor
 * @head: the head for your list
 * @member: the name of the list_head within the struct
 */
#define list_for_each_entry(pos, head, member)                  \
    for (pos = list_first_entry(head, typeof(*pos), member);    \
         &pos->member != (head);                                \
         pos = list_next_entry(pos, member))

/**
 * list_for_each_entry_safe - Iterate over list of given type safe against removal
 * @pos: the type * to use as a loop cursor
 * @n: another type * to use as temporary storage
 * @head: the head for your list
 * @member: the name of the list_head within the struct
 */
#define list_for_each_entry_safe(pos, n, head, member)          \
    for (pos = list_first_entry(head, typeof(*pos), member),    \
         n = list_next_entry(pos, member);                      \
         &pos->member != (head);                                \
         pos = n, n = list_next_entry(n, member))

/**
 * list_for_each_entry_reverse - Iterate backwards over list of given type
 * @pos: the type * to use as a loop cursor
 * @head: the head for your list
 * @member: the name of the list_head within the struct
 */
#define list_for_each_entry_reverse(pos, head, member)          \
    for (pos = list_last_entry(head, typeof(*pos), member);     \
         &pos->member != (head);                                \
         pos = list_prev_entry(pos, member))

#endif /* LIST_H */
