#include "signal.h"
#include "process.h"
#include "printk.h"
#include "string.h"

void signal_init(void) {
    pr_info("Signal subsystem initialized\n");
}

int send_signal(uint32_t pid, int sig) {
    if (sig < 1 || sig >= NSIG) {
        return -1;  // Invalid signal
    }
    
    // Find process by PID
    process_t *proc = process_find_by_pid(pid);
    if (proc) {
        // Set pending signal bit
        proc->pending_signals |= (1 << sig);
        return 0;
    }
    
    return -1;  // Process not found
}

void do_signal(void) {
    if (!current_process) return;
    
    // Check each signal
    for (int sig = 1; sig < NSIG; sig++) {
        if (current_process->pending_signals & (1 << sig)) {
            // Clear pending bit
            current_process->pending_signals &= ~(1 << sig);
            
            sighandler_t handler = current_process->signal_handlers[sig];
            
            // Handle signal based on handler
            if (handler == SIG_DFL) {
                // Default action
                switch (sig) {
                    case SIGKILL:
                    case SIGTERM:
                    case SIGINT:
                    case SIGQUIT:
                    case SIGABRT:
                    case SIGSEGV:
                        // Terminate process
                        pr_info("Process %d terminated by signal %d\n", 
                               current_process->pid, sig);
                        process_kill(current_process->pid);
                        return;
                    
                    case SIGSTOP:
                        // Stop process (block it)
                        current_process->state = PROCESS_BLOCKED;
                        break;
                    
                    case SIGCONT:
                        // Continue process
                        if (current_process->state == PROCESS_BLOCKED) {
                            current_process->state = PROCESS_READY;
                        }
                        break;
                    
                    default:
                        // Ignore other signals by default
                        break;
                }
            } else if (handler != SIG_IGN) {
                // User-defined handler
                // For now, we'll just print (full implementation would
                // require setting up user stack and jumping to handler)
                pr_debug("Signal %d handler at 0x%x for PID %d\n", 
                        sig, (uint32_t)handler, current_process->pid);
                
                // TODO: Execute user handler in userspace
                // This requires:
                // 1. Save current context
                // 2. Set up signal stack frame
                // 3. Jump to handler
                // 4. Restore context on return
            }
        }
    }
}

// Syscall implementations
int sys_kill(uint32_t pid, int sig) {
    return send_signal(pid, sig);
}

sighandler_t sys_signal(int sig, sighandler_t handler) {
    if (!current_process || sig < 1 || sig >= NSIG) {
        return SIG_DFL;
    }
    
    // Cannot catch SIGKILL or SIGSTOP
    if (sig == SIGKILL || sig == SIGSTOP) {
        return SIG_DFL;
    }
    
    sighandler_t old_handler = current_process->signal_handlers[sig];
    current_process->signal_handlers[sig] = handler;
    
    return old_handler;
}

int sys_sigaction(int sig, const struct sigaction *act, struct sigaction *oldact) {
    if (!current_process || sig < 1 || sig >= NSIG) {
        return -1;
    }
    
    // Cannot catch SIGKILL or SIGSTOP
    if (sig == SIGKILL || sig == SIGSTOP) {
        return -1;
    }
    
    // Save old action
    if (oldact) {
        oldact->sa_handler = current_process->signal_handlers[sig];
        oldact->sa_mask = 0;  // Simplified
        oldact->sa_flags = 0;
    }
    
    // Set new action
    if (act) {
        current_process->signal_handlers[sig] = act->sa_handler;
    }
    
    return 0;
}
