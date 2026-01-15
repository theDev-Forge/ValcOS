#ifndef SIGNAL_H
#define SIGNAL_H

#include <stdint.h>

/**
 * POSIX-Style Signal Handling
 * 
 * Provides signal delivery and handling for processes.
 */

// Signal numbers (POSIX-compatible)
#define SIGHUP    1   // Hangup
#define SIGINT    2   // Interrupt (Ctrl+C)
#define SIGQUIT   3   // Quit
#define SIGILL    4   // Illegal instruction
#define SIGTRAP   5   // Trace trap
#define SIGABRT   6   // Abort
#define SIGBUS    7   // Bus error
#define SIGFPE    8   // Floating point exception
#define SIGKILL   9   // Kill (cannot be caught)
#define SIGUSR1   10  // User-defined signal 1
#define SIGSEGV   11  // Segmentation fault
#define SIGUSR2   12  // User-defined signal 2
#define SIGPIPE   13  // Broken pipe
#define SIGALRM   14  // Alarm clock
#define SIGTERM   15  // Termination
#define SIGCHLD   17  // Child status changed
#define SIGCONT   18  // Continue
#define SIGSTOP   19  // Stop (cannot be caught)

#define NSIG      32  // Number of signals

// Signal handler types
#define SIG_DFL  ((sighandler_t)0)  // Default action
#define SIG_IGN  ((sighandler_t)1)  // Ignore signal

// Signal handler function type
typedef void (*sighandler_t)(int);

// Signal action structure (simplified sigaction)
struct sigaction {
    sighandler_t sa_handler;
    uint32_t sa_mask;
    uint32_t sa_flags;
};

/**
 * send_signal - Send signal to process
 * @pid: Process ID
 * @sig: Signal number
 * Returns: 0 on success, -1 on error
 */
int send_signal(uint32_t pid, int sig);

/**
 * do_signal - Deliver pending signals to current process
 * Called before returning to userspace
 */
void do_signal(void);

/**
 * signal_init - Initialize signal subsystem
 */
void signal_init(void);

// Syscall implementations
int sys_kill(uint32_t pid, int sig);
sighandler_t sys_signal(int sig, sighandler_t handler);
int sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact);

#endif /* SIGNAL_H */
