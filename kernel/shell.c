#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fat12.h"
#include "memory.h"
#include "process.h"
#include "pmm.h"

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
        vga_print("  help  - Display this help message\n");
        vga_print("  clear - Clear the screen\n");
        vga_print("  ls    - List files\n");
        vga_print("  cat   - Read file content\n");
        vga_print("  ps    - List processes\n");
        vga_print("  mem   - Show memory usage\n");
        vga_print("  kill  - Kill process <pid>\n");
        vga_print("  kill  - Kill process <pid>\n");
        vga_print("  color - Set text color <fg> <bg>\n");
        vga_print("  about - Show OS information\n");
        vga_print("  echo  - Print text to screen\n\n");
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
             if (fat12_write_file(filename, (uint8_t*)text, strlen(text))) {
                  vga_print("\nFile written.\n\n");
             } else {
                  vga_print("\nAvailable file not found to write.\n\n");
             }
             // Restore space for history integrity (optional)
             filename[i] = ' ';
        } else {
             vga_print("\nUsage: write <filename> <text>\n\n");
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
