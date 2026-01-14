#include "shell.h"
#include "vga.h"
#include "keyboard.h"
#include "string.h"
#include "fat12.h"
#include "memory.h"
#include "process.h"

#define CMD_BUFFER_SIZE 256

static char cmd_buffer[CMD_BUFFER_SIZE];
static int cmd_pos = 0;

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
        vga_print("  about - Show OS information\n");
        vga_print("  echo  - Print text to screen\n\n");
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
            cmd_buffer[cmd_pos] = '\0';
            shell_execute_command(cmd_buffer);
            cmd_pos = 0;
            shell_print_prompt();
        }
        else if (c == '\b') {
            if (cmd_pos > 0) {
                cmd_pos--;
            }
        }
        else if (cmd_pos < CMD_BUFFER_SIZE - 1) {
            cmd_buffer[cmd_pos++] = c;
        }
    }
}
