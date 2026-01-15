#ifndef FS_H
#define FS_H

#include <stdint.h>
#include <stddef.h>

/**
 * Virtual File System (VFS) - Linux-inspired
 * 
 * Provides abstraction layer for file systems.
 */

// Forward declarations
struct inode;
struct dentry;
struct file;
struct super_block;

// File operations structure
struct file_operations {
    int (*open)(struct inode *inode, struct file *file);
    int (*read)(struct file *file, char *buf, size_t count, uint32_t *offset);
    int (*write)(struct file *file, const char *buf, size_t count, uint32_t *offset);
    int (*close)(struct file *file);
};

// Inode operations
struct inode_operations {
    struct dentry *(*lookup)(struct inode *dir, const char *name);
};

// Inode structure (file metadata)
struct inode {
    uint32_t i_ino;                         // Inode number
    uint32_t i_size;                        // File size
    uint32_t i_mode;                        // File mode/permissions
    struct inode_operations *i_op;          // Inode operations
    struct file_operations *i_fop;          // File operations
    struct super_block *i_sb;               // Superblock
    void *i_private;                        // Filesystem-specific data
};

// Directory entry (name -> inode mapping)
struct dentry {
    char d_name[256];                       // Name
    struct inode *d_inode;                  // Associated inode
    struct dentry *d_parent;                // Parent dentry
};

// Open file instance
struct file {
    struct dentry *f_dentry;                // Associated dentry
    struct inode *f_inode;                  // Associated inode
    uint32_t f_pos;                         // File position
    uint32_t f_flags;                       // File flags
    struct file_operations *f_op;           // File operations
};

// Superblock (filesystem instance)
struct super_block {
    uint32_t s_blocksize;                   // Block size
    struct inode *s_root;                   // Root inode
    void *s_fs_info;                        // Filesystem-specific info
};

// File flags
#define O_RDONLY    0x0000
#define O_WRONLY    0x0001
#define O_RDWR      0x0002
#define O_CREAT     0x0100
#define O_TRUNC     0x0200
#define O_APPEND    0x0400

// File descriptor table size
#define MAX_FDS     16

/**
 * vfs_init - Initialize VFS subsystem
 */
void vfs_init(void);

/**
 * vfs_open - Open file
 * @path: File path
 * @flags: Open flags
 * Returns: File descriptor or -1 on error
 */
int vfs_open(const char *path, int flags);

/**
 * vfs_read - Read from file
 * @fd: File descriptor
 * @buf: Buffer
 * @count: Bytes to read
 * Returns: Bytes read or -1 on error
 */
int vfs_read(int fd, void *buf, size_t count);

/**
 * vfs_write - Write to file
 * @fd: File descriptor
 * @buf: Buffer
 * @count: Bytes to write
 * Returns: Bytes written or -1 on error
 */
int vfs_write(int fd, const void *buf, size_t count);

/**
 * vfs_close - Close file
 * @fd: File descriptor
 * Returns: 0 on success, -1 on error
 */
int vfs_close(int fd);

#endif /* FS_H */
