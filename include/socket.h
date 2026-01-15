#ifndef SOCKET_H
#define SOCKET_H

#include <stdint.h>
#include "skbuff.h"

/**
 * Socket API (simplified BSD sockets)
 */

// Socket types
#define SOCK_STREAM 1   // TCP (not implemented yet)
#define SOCK_DGRAM  2   // UDP/datagram

// Address families
#define AF_INET  2      // IPv4 (simplified)
#define AF_LOCAL 1      // Local/Unix sockets

// Socket structure
struct socket {
    int type;                       // SOCK_STREAM or SOCK_DGRAM
    int bound;                      // Is socket bound?
    uint32_t local_addr;            // Local address (simplified)
    uint16_t local_port;            // Local port
    struct sk_buff_head recv_queue; // Receive queue
    struct net_device *dev;         // Associated device
};

/**
 * socket_create - Create socket
 * @domain: Address family
 * @type: Socket type
 * Returns: Socket pointer or NULL
 */
struct socket *socket_create(int domain, int type);

/**
 * socket_bind - Bind socket to address
 * @sock: Socket
 * @addr: Address (simplified - just port for now)
 * Returns: 0 on success, -1 on error
 */
int socket_bind(struct socket *sock, uint16_t port);

/**
 * socket_send - Send data
 * @sock: Socket
 * @buf: Data buffer
 * @len: Data length
 * Returns: Bytes sent or -1 on error
 */
int socket_send(struct socket *sock, const void *buf, unsigned int len);

/**
 * socket_recv - Receive data
 * @sock: Socket
 * @buf: Buffer to receive into
 * @len: Buffer length
 * Returns: Bytes received or -1 on error
 */
int socket_recv(struct socket *sock, void *buf, unsigned int len);

/**
 * socket_close - Close socket
 * @sock: Socket to close
 */
void socket_close(struct socket *sock);

/**
 * socket_init - Initialize socket subsystem
 */
void socket_init(void);

#endif /* SOCKET_H */
