// golden
// 6/12/2018
//

#ifndef _PROC_H
#define _PROC_H
#include <stdbool.h>
#include "../../ps4-payload-sdk/include/types.h"
#include "./protocol.h"
#include "./net.h"

// Structure representing a memory map entry for a process
struct proc_vm_map_entry {
    char name[32]; // Name of the mapped region
    uint64_t start; // Start address of the mapped region
    uint64_t end; // End address of the mapped region
    uint64_t offset; // Offset of the mapped region
    uint16_t prot; // Protection flags of the mapped region
} __attribute__((packed));

int proc_list_handle(int fd, struct cmd_packet *packet);
int proc_read_handle(int fd, struct cmd_packet *packet);
int proc_write_handle(int fd, struct cmd_packet *packet);
int proc_maps_handle(int fd, struct cmd_packet *packet);
int proc_install_handle(int fd, struct cmd_packet *packet);
int proc_call_handle(int fd, struct cmd_packet *packet);
int proc_protect_handle(int fd, struct cmd_packet *packet);
int proc_scan_handle(int fd, struct cmd_packet *packet);
int proc_info_handle(int fd, struct cmd_packet *packet);
int proc_alloc_handle(int fd, struct cmd_packet *packet);
int proc_free_handle(int fd, struct cmd_packet *packet);

int proc_handle(int fd, struct cmd_packet *packet);

#endif
