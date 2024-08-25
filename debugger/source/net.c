// golden
// 6/12/2018
//

#include "net.h"

int net_select(int fd, fd_set *readfds, fd_set *writefds, fd_set *exceptfds, struct timeval *timeout) {
    return syscall(93, fd, readfds, writefds, exceptfds, timeout);
}

int net_send_data(int fd, void *data, int length) {
    int left = length;
    int offset = 0;
    int sent = 0;
    errno = NULL;

    while (left > 0) {
        // Determine the chunk size to use for sending data next
        int chunk_sz = (left > NET_MAX_LENGTH) ? NET_MAX_LENGTH : left;

        // Send data and make sure that if the length of the data
        // sent is less or equal to 0, handle it
        sent = write(fd, data + offset, chunk_sz);
        if (sent <= 0) {
            if (errno && errno != EWOULDBLOCK)
                return sent;

        } else {
            offset += sent;
            left -= sent;
        }
    }

    return offset;
}

int net_recv_data(int fd, void *data, int length, int force) {
    int left = length;
    int offset = 0;
    int recv = 0;
    errno = NULL;

    while (left > 0) {
        // Determine the chunk size to use for receiving data next
        int chunk_sz = (left > NET_MAX_LENGTH) ? NET_MAX_LENGTH : left;

        // Receive data and make sure that if the length of the data
        // received is less or equal to 0, handle it
        recv = read(fd, data + offset, chunk_sz);
        if (recv <= 0) {
            if (force) {
                if (errno && errno != EWOULDBLOCK)
                    return recv;

            } else {
                return offset;
            }
        } else {
            offset += recv;
            left -= recv;
        }
    }

    return offset;
}

int net_send_status(int fd, uint32_t status) {
    uint32_t d = status;
    return net_send_data(fd, &d, sizeof(uint32_t));
}
