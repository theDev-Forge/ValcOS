#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fat12.h"
#include "memory.h"
#include "process.h"
#include "list.h"
#include "slab.h"
#include "printk.h"
#include "pmm.h"
#include "timer.h"
#include "rtc.h"

#define CMD_BUFFER_SIZE 256

#define HISTORY_SIZE 10
static char history[HISTORY_SIZE][CMD_BUFFER_SIZE];
static int history_count = 0;
static int history_idx = 0; // Current view index

static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_pos = 0;

static void shell_refresh_line(void) {
    // Hacky clear line: CR, lots of spaces, CR
    vga_putchar('\r');
    for(int i=0; i<78; i++) vga_putchar(' ');
    vga_putchar('\r');
    
    // Rewrite prompt and buffer
    vga_print_color("more> ", vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK)); // Simplified prompt for internal redraw or full redraw
    // Actually full redraw is better
    // But we are inside shell_run.
    // Let's assume prompt length is fixed or we redraw the prompt logic.
    // Using a simpler approach: Just support clear and print.
    vga_print_color("ValcOS", vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_print_color("> ", vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_print(cmd_buffer);
}

static void shell_add_history(const char* cmd) {
    if (strlen(cmd) == 0) return;
    
    // Move everything up if full? Or circular buffer.
    // Simple shift for now.
    if (history_count < HISTORY_SIZE) {
        strcpy(history[history_count], cmd);
        history_count++;
    } else {
        for(int i=1; i<HISTORY_SIZE; i++) strcpy(history[i-1], history[i]);
        strcpy(history[HISTORY_SIZE-1], cmd);
    }
    history_idx = history_count;
}

static void shell_print_prompt(void) {
    vga_print_color("ValcOS", vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_print_color("> ", vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
}

static void shell_execute_command(const char* cmd) {
    if (strcmp(cmd, "help") == 0) {
        vga_print_color("\nAvailable commands:\n", vga_color(VGA_COLOR_YELLOW, VGA_COLOR_BLACK));
        vga_print("  help       - Display this help message\n");
        vga_print("  clear      - Clear the screen\n");
        vga_print("  ls         - List files\n");
        vga_print("  cat        - Read file content\n");
        vga_print("  touch      - Create file\n");
        vga_print("  write      - Write to file\n");
        vga_print("  ps         - List processes\n");
        vga_print("  top        - Show processes with CPU usage\n");
        vga_print("  nice       - Set process priority <pid> <priority>\n");
        vga_print("  mem        - Show memory usage\n");
        vga_print("  kill       - Kill process <pid>\n");
        vga_print("  color      - Set text color <fg> <bg>\n");
        vga_print("  about      - Show OS information\n");
        vga_print("  echo       - Print text to screen\n");
        vga_print("  timer_info - Display timer statistics\n");
        vga_print("  mem_stats  - Enhanced memory statistics\n");
        vga_print("  slabinfo   - Show slab allocator statistics\n");
        vga_print("  loglevel   - Set kernel log level <0-7>\n");
        vga_print("  test_log   - Test kernel logging\n");
        vga_print("  fs_space   - Show filesystem space\n");
        vga_print("  fs_delete  - Delete file <filename>\n");
        vga_print("  time       - Display current time and date\n");
        vga_print("  uptime     - Show system uptime\n");
        vga_print("  cd         - Change directory <path>\n");
        vga_print("  mkdir      - Create directory <name>\n");
        vga_print("  pwd        - Print working directory\n\n");
    }
    else if (cmd[0] == 'c' && cmd[1] == 'o' && cmd[2] == 'l' && cmd[3] == 'o' && cmd[4] == 'r') {
        // Parse simple integers for now: color 15 0
        // Or color red
        // Let's do simple logic: color <fg_byte>
        if (strlen(cmd) > 6) {
             const char *arg = cmd + 6;
             // Naive atoi
             int fg = 0;
             while(*arg >= '0' && *arg <= '9') { fg = fg*10 + (*arg - '0'); arg++; }
             vga_set_color((uint8_t)fg, VGA_COLOR_BLACK);
             vga_print("\nColor changed.\n\n");
        } else {
             vga_print("\nUsage: color <number 0-15>\n\n");
        }
    }
    else if (strcmp(cmd, "ps") == 0) {
        vga_print("\n");
        process_debug_list();
        vga_print("\n");
    }
    else if (strcmp(cmd, "mem") == 0) {
        uint32_t total, used;
        pmm_get_stats(&total, &used);
        vga_print("\nMemory Stats:\n");
        vga_print("Total Blocks: ");
        char buf[16];
        int i=0; uint32_t n=total;
        if(n==0) buf[i++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[i++]=t[--j]; } buf[i]=0;
        vga_print(buf);
        vga_print("\nUsed Blocks:  ");
        i=0; n=used;
        if(n==0) buf[i++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[i++]=t[--j]; } buf[i]=0;
        vga_print(buf);
        vga_print("\n\n");
    }
    else if (cmd[0] == 'k' && cmd[1] == 'i' && cmd[2] == 'l' && cmd[3] == 'l' && cmd[4] == ' ') {
        const char *pid_str = cmd + 5;
        // Simple Atoi
        uint32_t pid = 0;
        while (*pid_str >= '0' && *pid_str <= '9') {
            pid = pid * 10 + (*pid_str - '0');
            pid_str++;
        }
        
        if (process_kill(pid)) {
            vga_print("\nProcess Killed.\n\n");
        } else {
            vga_print("\nFailed to kill process (Not found or Kernel).\n\n");
        }
    }
    else if (strcmp(cmd, "clear") == 0) {
        vga_clear();
    }
    else if (strcmp(cmd, "ls") == 0) {
        vga_print("\n");
        fat12_list_directory();
        vga_print("\n");
    }
    else if (cmd[0] == 'c' && cmd[1] == 'a' && cmd[2] == 't' && cmd[3] == ' ') {
        const char *filename = cmd + 4;
        uint8_t *buf = (uint8_t*)kmalloc(4096);
        if(buf) {
            int bytes = fat12_read_file(filename, buf);
            if (bytes > 0) {
                vga_print("\n");
                if (bytes < 4096) buf[bytes] = 0; else buf[4095] = 0;
                vga_print((char*)buf);
                vga_print("\n\n");
            } else {
                 vga_print("\nFile not found.\n\n");
            }
            kfree(buf);
        } else {
            vga_print("\nMemory error.\n\n");
        }
    }
    else if (strcmp(cmd, "about") == 0) {
        vga_print("\n");
        vga_print_color("ValcOS v0.1\n", vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
        vga_print("A simple operating system built from scratch\n");
        vga_print("Created with passion and determination!\n\n");
    }
    else if (cmd[0] == 'e' && cmd[1] == 'x' && cmd[2] == 'e' && cmd[3] == 'c' && cmd[4] == ' ') {
        const char *filename = cmd + 5;
        // Allocate executable memory (8KB for now)
        uint8_t *prog_buf = (uint8_t*)kmalloc(8192);
        if (prog_buf) {
            int bytes = fat12_read_file(filename, prog_buf);
            if (bytes > 0) {
                 vga_print("\nExecuting ");
                 vga_print((char*)filename);
                 vga_print("...\n\n");
                 process_create_user((void (*)(void))prog_buf);
            } else {
                 vga_print("\nFile not found.\n\n");
                 // kfree(prog_buf); // Memory leak until kfree implemented
            }
        } else {
             vga_print("\nMemory error.\n\n");
        }
    }
    else if (cmd[0] == 'e' && cmd[1] == 'c' && cmd[2] == 'h' && cmd[3] == 'o' && cmd[4] == ' ') {
        vga_print("\n");
        vga_print(&cmd[5]);
        vga_print("\n\n");
    }
    else if (cmd[0] == 't' && cmd[1] == 'o' && cmd[2] == 'u' && cmd[3] == 'c' && cmd[4] == 'h' && cmd[5] == ' ') {
        const char *filename = cmd + 6;
        if (fat12_create_file(filename)) {
            vga_print("\nFile created.\n\n");
        } else {
            vga_print("\nFailed to create file (Disk full or exists).\n\n");
        }
    }
    else if (cmd[0] == 'w' && cmd[1] == 'r' && cmd[2] == 'i' && cmd[3] == 't' && cmd[4] == 'e' && cmd[5] == ' ') {
        // write filename text...
        // Need to split args
        char *filename = (char*)cmd + 6;
        char *text = NULL;
        
        // Find space
        int i=0;
        while(filename[i] != 0) {
            if(filename[i] == ' ') {
                filename[i] = 0; // Split string
                text = filename + i + 1;
                break;
            }
            i++;
        }
        
        if (text) {
             int result = fat12_write_file(filename, (uint8_t*)text, strlen(text));
             if (result == 0) {
                  vga_print("\nFile written.\n\n");
             } else {
                  vga_print("\nError: ");
                  vga_print(fat12_get_error_string(result));
                  vga_print("\n\n");
             }
             // Restore space for history integrity (optional)
             filename[i] = ' ';
        } else {
             vga_print("\nUsage: write <filename> <text>\n\n");
        }
    }
    else if (strcmp(cmd, "timer_info") == 0) {
        uint32_t ticks, callbacks;
        timer_get_stats(&ticks, &callbacks);
        uint32_t uptime_ms = timer_get_uptime_ms();
        
        vga_print("\nTimer Statistics:\n");
        vga_print("  Total Ticks: ");
        char buf[16]; int idx=0; uint32_t n=ticks;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\n  Uptime (ms): ");
        idx=0; n=uptime_ms;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\n  Callbacks Executed: ");
        idx=0; n=callbacks;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("\n\n");
    }
    else if (strcmp(cmd, "mem_stats") == 0) {
        uint32_t total, used;
        pmm_get_stats(&total, &used);
        uint32_t free_mem = pmm_get_free_memory();
        uint32_t total_mem = pmm_get_total_memory();
        
        vga_print("\nMemory Statistics:\n");
        vga_print("  PMM Total Blocks: ");
        char buf[16]; int idx=0; uint32_t n=total;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\n  PMM Used Blocks:  ");
        idx=0; n=used;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\n  Free Memory (KB): ");
        idx=0; n=free_mem/1024;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\n  Total Memory (KB): ");
        idx=0; n=total_mem/1024;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("\n\n");
    }
    else if (strcmp(cmd, "slabinfo") == 0) {
        slab_stats();
    }
    else if (strncmp(cmd, "loglevel", 8) == 0) {
        // Parse level
        if (strlen(cmd) > 9) {
            int level = cmd[9] - '0';
            if (level >= 0 && level <= 7) {
                printk_set_level(level);
                vga_print("Log level set.\n");
            } else {
                vga_print("Invalid level (0-7).\n");
            }
        } else {
            vga_print("Usage: loglevel <0-7>\n");
            vga_print("Current level: ");
            char buf[2];
            buf[0] = '0' + printk_get_level();
            buf[1] = 0;
            vga_print(buf);
            vga_print("\n");
        }
    }
    else if (strcmp(cmd, "test_log") == 0) {
        pr_emerg("Emergency message\n");
        pr_alert("Alert message\n");
        pr_crit("Critical message\n");
        pr_err("Error message\n");
        pr_warn("Warning message\n");
        pr_notice("Notice message\n");
        pr_info("Info message\n");
        pr_debug("Debug message\n");
    }
    else if (strcmp(cmd, "fs_space") == 0) {
        uint32_t free_space = fat12_get_free_space();
        uint32_t total_space = fat12_get_total_space();
        uint32_t used_space = total_space - free_space;
        
        vga_print("\nFilesystem Space:\n");
        vga_print("  Total: ");
        char buf[16]; int idx=0; uint32_t n=total_space;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print(" bytes\n");
        
        vga_print("  Used:  ");
        idx=0; n=used_space;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print(" bytes\n");
        
        vga_print("  Free:  ");
        idx=0; n=free_space;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print(" bytes\n\n");
    }
    else if (cmd[0] == 'f' && cmd[1] == 's' && cmd[2] == '_' && cmd[3] == 'd' && cmd[4] == 'e' && cmd[5] == 'l' && cmd[6] == 'e' && cmd[7] == 't' && cmd[8] == 'e' && cmd[9] == ' ') {
        const char *filename = cmd + 10;
        int result = fat12_delete_file(filename);
        if (result == 0) {
            vga_print("\nFile deleted.\n\n");
        } else {
            vga_print("\nError: ");
            vga_print(fat12_get_error_string(result));
            vga_print("\n\n");
        }
    }
    else if (strcmp(cmd, "time") == 0) {
        uint8_t hour, minute, second;
        uint16_t year;
        uint8_t month, day;
        
        rtc_read_time(&hour, &minute, &second);
        rtc_read_date(&year, &month, &day);
        
        vga_print("\nCurrent Time: ");
        char buf[16]; int idx=0; uint32_t n;
        
        // Hour
        n=hour;
        if(n<10) buf[idx++]='0';
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print(":");
        
        // Minute
        idx=0; n=minute;
        if(n<10) buf[idx++]='0';
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print(":");
        
        // Second
        idx=0; n=second;
        if(n<10) buf[idx++]='0';
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        
        vga_print("\nCurrent Date: ");
        
        // Year
        idx=0; n=year;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("-");
        
        // Month
        idx=0; n=month;
        if(n<10) buf[idx++]='0';
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("-");
        
        // Day
        idx=0; n=day;
        if(n<10) buf[idx++]='0';
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("\n\n");
    }
    else if (strcmp(cmd, "uptime") == 0) {
        uint32_t uptime_ms = timer_get_uptime_ms();
        uint32_t seconds = uptime_ms / 1000;
        uint32_t minutes = seconds / 60;
        uint32_t hours = minutes / 60;
        
        seconds %= 60;
        minutes %= 60;
        
        vga_print("\nSystem Uptime: ");
        char buf[16]; int idx=0; uint32_t n;
        
        n=hours;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("h ");
        
        idx=0; n=minutes;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("m ");
        
        idx=0; n=seconds;
        if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
        vga_print(buf);
        vga_print("s\n\n");
    }
    else if (cmd[0] == 'c' && cmd[1] == 'd' && (cmd[2] == '\0' || cmd[2] == ' ')) {
        const char *path = (cmd[2] == ' ') ? cmd + 3 : "/";
        int result = fat12_change_directory(path);
        if (result == FAT12_SUCCESS) {
            vga_print("\n");
        } else {
            vga_print("\nError: ");
            vga_print(fat12_get_error_string(result));
            vga_print("\n\n");
        }
    }
    else if (cmd[0] == 'm' && cmd[1] == 'k' && cmd[2] == 'd' && cmd[3] == 'i' && cmd[4] == 'r' && cmd[5] == ' ') {
        const char *dirname = cmd + 6;
        int result = fat12_create_directory(dirname);
        if (result == FAT12_SUCCESS) {
            vga_print("\nDirectory created.\n\n");
        } else {
            vga_print("\nError: ");
            vga_print(fat12_get_error_string(result));
            vga_print("\n\n");
        }
    }
    else if (strcmp(cmd, "pwd") == 0) {
        vga_print("\n");
        vga_print(fat12_get_current_directory());
        vga_print("\n\n");
    }
    else if (cmd[0] == 'n' && cmd[1] == 'i' && cmd[2] == 'c' && cmd[3] == 'e' && cmd[4] == ' ') {
        // Parse: nice <pid> <priority>
        const char *args = cmd + 5;
        uint32_t pid = 0;
        uint8_t priority = 0;
        
        // Parse PID
        while (*args >= '0' && *args <= '9') {
            pid = pid * 10 + (*args - '0');
            args++;
        }
        
        // Skip space
        while (*args == ' ') args++;
        
        // Parse priority
        while (*args >= '0' && *args <= '9') {
            priority = priority * 10 + (*args - '0');
            args++;
        }
        
        process_set_priority(pid, priority);
        vga_print("\nPriority updated.\n\n");
    }
    else if (strcmp(cmd, "top") == 0) {
        vga_print("\nPID  | Priority | Runtime | State\n");
        vga_print("---- | -------- | ------- | --------\n");
        
        // We need to iterate through processes and display stats
        extern struct list_head ready_queue;
        extern process_t *current_process;
        
        if (list_empty(&ready_queue)) {
            vga_print("No processes.\n\n");
        } else {
            process_t *proc;
            list_for_each_entry(proc, &ready_queue, list) {
                char buf[16]; int idx;
                
                // PID
                uint32_t n = proc->pid;
                idx = 0;
                if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
                vga_print(buf);
                for(int k=idx; k<5; k++) vga_print(" ");
                vga_print("| ");
                
                // Priority
                n = proc->priority;
                idx = 0;
                if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
                vga_print(buf);
                for(int k=idx; k<9; k++) vga_print(" ");
                vga_print("| ");
                
                // Runtime
                n = proc->total_runtime;
                idx = 0;
                if(n==0) buf[idx++]='0'; else { char t[16]; int j=0; while(n>0){t[j++]='0'+(n%10);n/=10;} while(j>0) buf[idx++]=t[--j]; } buf[idx]=0;
                vga_print(buf);
                for(int k=idx; k<8; k++) vga_print(" ");
                vga_print("| ");
                
                // State
                switch(proc->state) {
                    case PROCESS_READY: vga_print("READY"); break;
                    case PROCESS_RUNNING: vga_print("RUNNING"); break;
                    case PROCESS_BLOCKED: vga_print("BLOCKED"); break;
                    case PROCESS_TERMINATED: vga_print("TERMINATED"); break;
                    default: vga_print("UNKNOWN"); break;
                }
                
                if (proc == current_process) vga_print(" (*)");
                vga_print("\n");
            }
            vga_print("\n");
        }
    }
    else if (strlen(cmd) > 0) {
        vga_print("\n");
        vga_print_color("Unknown command: ", vga_color(VGA_COLOR_LIGHT_RED, VGA_COLOR_BLACK));
        vga_print(cmd);
        vga_print("\nType 'help' for available commands.\n\n");
    }
    else {
        vga_print("\n");
    }
}

void shell_init(void) {
    vga_print_color("Welcome to ", vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_print_color("ValcOS", vga_color(VGA_COLOR_LIGHT_CYAN, VGA_COLOR_BLACK));
    vga_print_color("!\n", vga_color(VGA_COLOR_WHITE, VGA_COLOR_BLACK));
    vga_print("Type 'help' to see available commands.\n\n");
    shell_print_prompt();
}

void shell_run(void) {
    while (1) {
        char c = keyboard_getchar();
        
        if (c == '\n') {
            vga_putchar('\n');
            cmd_buffer[cmd_pos] = '\0';
            shell_add_history(cmd_buffer);
            shell_execute_command(cmd_buffer);
            cmd_pos = 0;
            history_idx = history_count; // Reset history view
            shell_print_prompt();
        }
        else if (c == '\b') {
            if (cmd_pos > 0) {
                cmd_pos--;
                vga_putchar('\b'); // Visual backspace
            }
        }
        else if (c == 0x11) { // UP Arrow
            if (history_idx > 0) {
                history_idx--;
                strcpy(cmd_buffer, history[history_idx]);
                cmd_pos = strlen(cmd_buffer);
                shell_refresh_line();
            }
        }
        else if (c == 0x12) { // DOWN Arrow
            if (history_idx < history_count) {
                history_idx++;
                if (history_idx == history_count) {
                    cmd_buffer[0] = 0;
                    cmd_pos = 0;
                } else {
                    strcpy(cmd_buffer, history[history_idx]);
                    cmd_pos = strlen(cmd_buffer);
                }
                shell_refresh_line();
            }
        }
        else if (c >= 0x20 && cmd_pos < CMD_BUFFER_SIZE - 1) { // Standard printable
            cmd_buffer[cmd_pos++] = c;
            vga_putchar(c); // Echo character
        }
    }
}
