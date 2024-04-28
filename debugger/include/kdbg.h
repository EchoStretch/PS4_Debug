#pragma once
#ifndef _KDEBUGGER_H
#define _KDEBUGGER_H

#include <stdarg.h>
#include "../../ps4-payload-sdk/include/ps4.h"

// This can be implemented in the future, but for now it's non-implemented
#define nlPrintf(...) 

// TODO:
// Rewrite this macro as a function, rather than macro. 
// - Currently every time it's used, a nested { ... } code-block is created in the function where it's called.
//   This may or may not have an impact on the performace.
// ----------------------
// Macro acts as a custom printf-like function
#define uprintf(fmt, ...) {                          \
    char buffer[256];                                \
    snprintf(buffer, 256, fmt, ##__VA_ARGS__);       \
    sys_console_cmd(SYS_CONSOLE_CMD_PRINT, buffer);  \
}

// Related to custom syscall 109
#define SYS_PROC_CMD_ALLOC      1
#define SYS_PROC_CMD_FREE       2
#define SYS_PROC_CMD_PROTECT    3
#define SYS_PROC_CMD_CALL       5

#define SYS_PROC_ALLOC      1
#define SYS_PROC_FREE       2
#define SYS_PROC_PROTECT    3
#define SYS_PROC_VM_MAP     4
#define SYS_PROC_INSTALL    5
#define SYS_PROC_CALL       6
#define SYS_PROC_ELF        7
#define SYS_PROC_INFO       8
#define SYS_PROC_THRINFO    9

// Related to custom syscall 112
#define SYS_CONSOLE_CMD_REBOOT       1
#define SYS_CONSOLE_CMD_PRINT        2
#define SYS_CONSOLE_CMD_JAILBREAK    3

// Related to custom syscall 107
// The structure of a Process List Entry
struct proc_list_entry  {
    char p_comm[32]; // Process name
    int pid;         // Process ID
} __attribute__((packed));

// Arguments for calling proc_alloc
struct sys_proc_alloc_args {
    uint64_t address; // Address for allocation
    uint64_t length;  // Length of memory to allocate
} __attribute__((packed));

// Arguments for calling proc_free
struct sys_proc_free_args {
    uint64_t address; // Address of memory to free
    uint64_t length; // Length of memory to free
} __attribute__((packed));

// Arguments for calling proc_protect
struct sys_proc_protect_args {
    uint64_t address; // Address of memory region
    uint64_t length; // Length of memory region
    uint64_t prot; // Protection flags
} __attribute__((packed));

// Arguments for calling proc_vm_map
struct sys_proc_vm_map_args {
    struct proc_vm_map_entry *maps; // Pointer to an array of memory maps
    uint64_t num; // Number of memory maps
} __attribute__((packed));

// Arguments for calling proc_install
struct sys_proc_install_args {
    uint64_t stubentryaddr; // Address of the stub entry
} __attribute__((packed));

// Arguments for calling proc_call
struct sys_proc_call_args {
    uint32_t pid; // Process ID
    uint64_t rpcstub; // Address of the remote procedure call stub
    uint64_t rax; // Value of RAX register
    uint64_t rip; // Value of RIP register
    uint64_t rdi; // Value of RDI register
    uint64_t rsi; // Value of RSI register
    uint64_t rdx; // Value of RDX register
    uint64_t rcx; // Value of RCX register
    uint64_t r8; // Value of R8 register
    uint64_t r9; // Value of R9 register
} __attribute__((packed));

// Arguments for calling proc_elf
struct sys_proc_elf_args {
    void *elf; // Pointer to ELF data
} __attribute__((packed));

// Arguments for calling proc_info
struct sys_proc_info_args {
    int pid; // Process ID
    char name[40]; // Process name
    char path[64]; // Path to the process executable
    char titleid[16]; // Title ID of the process
    char contentid[64]; // Content ID of the process
} __attribute__((packed));

// Arguments for calling proc_thrinfo
struct sys_proc_thrinfo_args {
    uint32_t lwpid; // LWP ID
    uint32_t priority; // Priority of the thread
    char name[32]; // Name of the thread
} __attribute__((packed));

// Custom syscall 108
int sys_proc_rw(uint64_t pid, uint64_t address, void *data, uint64_t length, uint64_t write);
// custom syscall 109
int sys_proc_cmd(uint64_t pid, uint64_t cmd, void *data);
// custom syscall 110
int sys_kern_base(uint64_t *kbase);
// custom syscall 111
int sys_kern_rw(uint64_t address, void *data, uint64_t length, uint64_t write);
// custom syscall 112
int sys_console_cmd(uint64_t cmd, void *data);
// custom syscall 107
int sys_proc_list(struct proc_list_entry *procs, uint64_t *num);

// Other functions
void prefault(void *address, size_t size);
void *pfmalloc(size_t size);
void hexdump(void *data, size_t size);
void *pfrealloc(void *ptr, size_t size, size_t old_size) ;
#endif