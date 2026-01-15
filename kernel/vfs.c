#include "fs.h"
#include "process.h"
#include "memory.h"
#include "string.h"
#include "printk.h"

// Per-process file descriptor table
// For simplicity, using current_process directly
// In a full implementation, this would be in process_t

static struct file *fd_table[MAX_FDS];

void vfs_init(void) {
    // Initialize file descriptor table
    for (int i = 0; i < MAX_FDS; i++) {
        fd_table[i] = NULL;
    }
    
    pr_info("VFS subsystem initialized\n");
}

// Find free file descriptor
static int fd_alloc(void) {
    for (int i = 0; i < MAX_FDS; i++) {
        if (fd_table[i] == NULL) {
            return i;
        }
    }
    return -1;  // No free descriptors
}

// Install file in descriptor table
static void fd_install(int fd, struct file *file) {
    if (fd >= 0 && fd < MAX_FDS) {
        fd_table[fd] = file;
    }
}

// Get file from descriptor
static struct file *fd_get(int fd) {
    if (fd < 0 || fd >= MAX_FDS) {
        return NULL;
    }
    return fd_table[fd];
}

int vfs_open(const char *path, int flags) {
    if (!path) return -1;
    
    // Allocate file descriptor
    int fd = fd_alloc();
    if (fd < 0) {
        pr_err("VFS: No free file descriptors\n");
        return -1;
    }
    
    // Allocate file structure
    struct file *file = (struct file *)kmalloc(sizeof(struct file));
    if (!file) {
        return -1;
    }
    
    // For now, we'll create a dummy file
    // In a real implementation, this would:
    // 1. Parse path
    // 2. Lookup inode
    // 3. Call filesystem-specific open
    
    file->f_dentry = NULL;
    file->f_inode = NULL;
    file->f_pos = 0;
    file->f_flags = flags;
    file->f_op = NULL;
    
    // Install in descriptor table
    fd_install(fd, file);
    
    pr_debug("VFS: Opened file '%s' as fd %d\n", path, fd);
    return fd;
}

int vfs_read(int fd, void *buf, size_t count) {
    struct file *file = fd_get(fd);
    if (!file || !buf) return -1;
    
    // Call filesystem-specific read
    if (file->f_op && file->f_op->read) {
        return file->f_op->read(file, (char *)buf, count, &file->f_pos);
    }
    
    // No read operation
    return -1;
}

int vfs_write(int fd, const void *buf, size_t count) {
    struct file *file = fd_get(fd);
    if (!file || !buf) return -1;
    
    // Call filesystem-specific write
    if (file->f_op && file->f_op->write) {
        return file->f_op->write(file, (const char *)buf, count, &file->f_pos);
    }
    
    // No write operation
    return -1;
}

int vfs_close(int fd) {
    struct file *file = fd_get(fd);
    if (!file) return -1;
    
    // Call filesystem-specific close
    if (file->f_op && file->f_op->close) {
        file->f_op->close(file);
    }
    
    // Free file structure
    kfree(file);
    
    // Remove from descriptor table
    fd_table[fd] = NULL;
    
    pr_debug("VFS: Closed fd %d\n", fd);
    return 0;
}
