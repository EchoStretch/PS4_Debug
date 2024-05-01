// golden
// 6/12/2018
//

#include "../include/kern.h"

int kern_base_handle(int fd, struct cmd_packet *packet) {
    uint64_t kernbase;

    sys_kern_base(&kernbase);

    net_send_status(fd, CMD_SUCCESS);
    net_send_data(fd, &kernbase, sizeof(uint64_t));

    return 0;
}

int kern_read_handle(int fd, struct cmd_packet *packet) {
    // Define variables that are going to be used
    struct cmd_kern_read_packet *kReadPkt;
    void *data;

    // TODO: Comment this part
    kReadPkt = (struct cmd_kern_read_packet *)packet->data;
    if (!kReadPkt) { 
        // Send status indicating null data
        net_send_status(fd, CMD_DATA_NULL);
        return -1;
    }

    // Try to allocate memory, that's prefaulted for the <data>
    // variable, and if memory allocation failed handle it
    data = pfmalloc(kReadPkt->length);
    if (data == NULL) {
        // Send status indicating null data
        net_send_status(fd, CMD_DATA_NULL);
        return -1;
    }

    // Read data from kernel memory
    sys_kern_rw(kReadPkt->address, data, kReadPkt->length, 0);

    // Send status indicating successful operation
    net_send_status(fd, CMD_SUCCESS);
    
    // Send the read data
    net_send_data(fd, data, kReadPkt->length);

    // Perform cleanup, and return
    free(data);
    return 0;
}

int kern_write_handle(int fd, struct cmd_packet *packet) {
    // Define variables that are going to be used
    struct cmd_kern_write_packet *kWritePkt; 
    void *data;
    
    // TODO: Comment this part
    kWritePkt = (struct cmd_kern_write_packet *)packet->data;
    if (!kWritePkt) {
        // Send status indicating null data
        net_send_status(fd, CMD_DATA_NULL);
        return -1;
    }
    
    // Try to allocate memory, that's prefaulted for the <data>
    // variable, and if memory allocation failed handle it
    data = pfmalloc(kWritePkt->length);
    if (data == NULL) {
        // Send status indicating null data
        net_send_status(fd, CMD_DATA_NULL);
        return -1;
    }

    // Send status indicating successful operation
    net_send_status(fd, CMD_SUCCESS);
    
    // Receive write data from network
    net_recv_data(fd, data, kWritePkt->length, 1);

    // Write data to kernel memory
    sys_kern_rw(kWritePkt->address, data, kWritePkt->length, 1); 

    // Send status indicating successful operation
    net_send_status(fd, CMD_SUCCESS);
    
    // Perform cleanup, and return
    free(data);
    return 0;
}

int kern_handle(int fd, struct cmd_packet *packet) {
    switch (packet->cmd) {
        case CMD_KERN_BASE:
            return kern_base_handle(fd, packet);
        case CMD_KERN_READ:
            return kern_read_handle(fd, packet);
        case CMD_KERN_WRITE:
            return kern_write_handle(fd, packet);
    }

    return 1;
}
