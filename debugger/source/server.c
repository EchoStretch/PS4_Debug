// golden
// 6/12/2018
//

#include "server.h"


#define TRUE     1
#define FALSE    0
#define ERRCODE -1

struct server_client servclients[SERVER_MAXCLIENTS];
struct server_client *alloc_client() {
    for (int i = 0; i < SERVER_MAXCLIENTS; i++) {
        if (servclients[i].id == 0) {
            servclients[i].id = i + 1;
            return &servclients[i];
        }
    }

    return NULL;
}

void free_client(struct server_client *svc) {
    svc->id = 0;
    sceNetSocketClose(svc->fd);

    if (svc->debugging) {
        debug_cleanup(&svc->dbgctx);
    }

    memset(svc, NULL, sizeof(struct server_client));
}

int handle_version(int fd, struct cmd_packet *packet) {
    uint32_t len = strlen(PACKET_VERSION);
    net_send_data(fd, &len, sizeof(uint32_t));
    net_send_data(fd, PACKET_VERSION, len);
    return 0;
}

int cmd_handler(int fd, struct cmd_packet *packet) {
    if (!VALID_CMD(packet->cmd))
        return 1;

    uprintf("cmd_handler %X", packet->cmd);

    if (packet->cmd == CMD_VERSION)
        return handle_version(fd, packet);


    if (VALID_PROC_CMD(packet->cmd)) {
        return proc_handle(fd, packet);
    } else if (VALID_DEBUG_CMD(packet->cmd)) {
        return debug_handle(fd, packet);
    } else if (VALID_KERN_CMD(packet->cmd)) {
        return kern_handle(fd, packet);
    } else if (VALID_CONSOLE_CMD(packet->cmd)) {
        return console_handle(fd, packet);
    }

    return 0;
}

int check_debug_interrupt() {
    struct debug_interrupt_packet resp;
    struct debug_breakpoint *breakpoint;
    struct ptrace_lwpinfo *lwpinfo;
    uint8_t int3;
    int status;
    int signal;
    int r;

    if (!wait4(curdbgctx->pid, &status, WNOHANG, NULL))
        return 0;

    signal = WSTOPSIG(status);
    uprintf("check_debug_interrupt signal %i", signal);

    if (signal == SIGSTOP) {
        uprintf("passed on a SIGSTOP");
        return 0;
    } else if (signal == SIGKILL) {
        debug_cleanup(curdbgctx);

        // the process will die
        ptrace(PT_CONTINUE, curdbgctx->pid, (void *)1, SIGKILL);

        uprintf("sent final SIGKILL");
        return 0;
    }

    // If lwpinfo is on the stack it fails, maybe I should patch ptrace? idk
    lwpinfo = (struct ptrace_lwpinfo *)pfmalloc(sizeof(struct ptrace_lwpinfo));
    if (!lwpinfo) {
        uprintf("could not allocate memory for thread information");
        return 1;
    }

    // grab interrupt data
    if (ptrace(PT_LWPINFO, curdbgctx->pid, lwpinfo, sizeof(struct ptrace_lwpinfo))) {
        uprintf("could not get lwpinfo errno %i", errno);
    }

    // fill response
    memset(&resp, NULL, DEBUG_INTERRUPT_PACKET_SIZE);
    resp.lwpid = lwpinfo->pl_lwpid;
    resp.status = status;

    // TODO: fix size mismatch with these fields
    // resp.tdname is tdname[40]
    // lwpinfo->pl_tdname is pl_tdname[24]
    memcpy(resp.tdname, lwpinfo->pl_tdname, sizeof(lwpinfo->pl_tdname));

    if (ptrace(PT_GETREGS, resp.lwpid, &resp.reg64, NULL)) {
        uprintf("could not get registers errno %i", errno);
    }

    if (ptrace(PT_GETFPREGS, resp.lwpid, &resp.savefpu, NULL)) {
        uprintf("could not get float registers errno %i", errno);
    }

    if (ptrace(PT_GETDBREGS, resp.lwpid, &resp.dbreg64, NULL)) {
        uprintf("could not get debug registers errno %i", errno);
    }

    // if it is a software breakpoint we need to handle it accordingly
    breakpoint = NULL;
    for (int i = 0; i < MAX_BREAKPOINTS; i++) {
        if (curdbgctx->breakpoints[i].address == resp.reg64.r_rip - 1) {
            breakpoint = &curdbgctx->breakpoints[i];
            break;
        }
    }

    if (breakpoint) {
        uprintf("dealing with software breakpoint");
        uprintf("breakpoint: %llX %X", breakpoint->address, breakpoint->original);

        // write old instruction
        sys_proc_rw(curdbgctx->pid, breakpoint->address, &breakpoint->original, 1, 1);

        // backstep 1 instruction
        resp.reg64.r_rip -= 1;
        ptrace(PT_SETREGS, resp.lwpid, &resp.reg64, NULL);

        // single step over the instruction
        ptrace(PT_STEP, resp.lwpid, (void *)1, NULL);
        while (!wait4(curdbgctx->pid, &status, WNOHANG, NULL)) {
            sceKernelUsleep(4000);
        }

        uprintf("waited for signal %i", WSTOPSIG(status));

        // set breakpoint again
        int3 = 0xCC;
        sys_proc_rw(curdbgctx->pid, breakpoint->address, &int3, 1, 1);
    } else {
        uprintf("dealing with hardware breakpoint");
    }

    r = net_send_data(curdbgctx->dbgfd, &resp, DEBUG_INTERRUPT_PACKET_SIZE);
    if (r != DEBUG_INTERRUPT_PACKET_SIZE) {
        uprintf("net_send_data failed %i %i", r, errno);
    }

    uprintf("check_debug_interrupt interrupt data sent");

    free(lwpinfo);

    return 0;
}

int handle_client(struct server_client *svc) {
    struct cmd_packet packet;
    uint32_t rsize;
    uint32_t length;
    void *data;
    int r;
    int fd = svc->fd;

    // Setup time val for select
    struct timeval tv;
    memset(&tv, NULL, sizeof(tv));
    tv.tv_usec = 1000;

    while (TRUE) {
        // Do a select
        fd_set sfd;
        FD_ZERO(&sfd);
        FD_SET(fd, &sfd);
        errno = NULL;
        net_select(FD_SETSIZE, &sfd, NULL, NULL, &tv);

        // Check if we can recieve
        if (FD_ISSET(fd, &sfd)) {
            // Zero out
            memset(&packet, NULL, CMD_PACKET_SIZE);

            // Recieve our data, and check if the length of the recieved data
            // indicates an error occured/no data was received and handle it
            rsize = net_recv_data(fd, &packet, CMD_PACKET_SIZE, 0);
            if (rsize <= 0) goto error;

            // Check if disconnected
            if (errno == ECONNRESET)
                goto error;

        } else {
            // if we have a valid debugger context then check for interrupt
            // this does not block, as wait is called with option WNOHANG
            if (svc->debugging) {
                if (check_debug_interrupt())
                    goto error;
            }

            // check if disconnected
            if (errno == ECONNRESET)
                goto error;

            sceKernelUsleep(25000);
            continue;
        }

        uprintf("client packet recieved");

        // invalid packet
        if (packet.magic != PACKET_MAGIC) {
            uprintf("invalid packet magic %X!", packet.magic);
            continue;
        }

        // mismatch received size
        if (rsize != CMD_PACKET_SIZE) {
            uprintf("invalid recieve size %i!", rsize);
            continue;
        }

        length = packet.datalen;
        if (length) {

            // Allocate prefaulted memory for data
            data = pfmalloc(length);
            if (data == NULL)
                goto error;

            uprintf("recieving data length %i", length);

            // Receive data
            if (!net_recv_data(fd, data, length, 1))
                goto error;

            // Set data
            packet.data = data;
        } else {
            packet.data = NULL;
        }

        // special case when attaching
        // if we are debugging then the handler for CMD_DEBUG_ATTACH will send back the right error
        if (!g_debugging && packet.cmd == CMD_DEBUG_ATTACH) {
            curdbgcli = svc;
            curdbgctx = &svc->dbgctx;
        }
        
        // Check if data memory is valid, and deallocate it
        if (data != NULL) {
            free(data);
            data = NULL;
        }

        // Handle the packet, and check the cmd handler for error
        if (cmd_handler(fd, &packet))
            goto error;
    }

    error:
    uprintf("client disconnected");
    free_client(svc);

    return 0;
}

void configure_socket(int fd) {
    int flag;

    flag = 1;
    sceNetSetsockopt(fd, SOL_SOCKET, SO_NBIO, (char *)&flag, sizeof(flag));

    flag = 1;
    sceNetSetsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(flag));

    flag = 1;
    sceNetSetsockopt(fd, SOL_SOCKET, SO_NOSIGPIPE, (char *)&flag, sizeof(flag));
}

void *broadcast_thread(void *arg) {
    struct sockaddr_in server;
    struct sockaddr_in client;
    unsigned int clisize;
    int serv;
    int flag;
    int r;
    uint32_t magic;

    uprintf("broadcast server started");

    // setup server
    server.sin_len = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = IN_ADDR_ANY;
    server.sin_port = sceNetHtons(BROADCAST_PORT);
    memset(server.sin_zero, NULL, sizeof(server.sin_zero));

    serv = sceNetSocket("broadsock", AF_INET, SOCK_DGRAM, 0);
    if (serv < 0) {
        uprintf("failed to create broadcast server");
        return NULL;
    }

    flag = 1;
    sceNetSetsockopt(serv, SOL_SOCKET, SO_BROADCAST, (char *)&flag, sizeof(flag));

    r = sceNetBind(serv, (struct sockaddr *)&server, sizeof(server));
    if (r) {
        uprintf("failed to bind broadcast server");
        return NULL;
    }

    // TODO: XXX: clean this up, but meh not too dirty? is it? hmmm
    int libNet = sceKernelLoadStartModule("libSceNet.sprx", 0, NULL, 0, 0, 0);
    int (*sceNetRecvfrom)(int s, void *buf, unsigned int len, int flags, struct sockaddr *from, unsigned int *fromlen);
    int (*sceNetSendto)(int s, void *msg, unsigned int len, int flags, struct sockaddr *to, unsigned int tolen);
    RESOLVE(libNet, sceNetRecvfrom);
    RESOLVE(libNet, sceNetSendto);

    while (1) {
        scePthreadYield();

        magic = 0;
        clisize = sizeof(client);
        r = sceNetRecvfrom(serv, &magic, sizeof(uint32_t), 0, (struct sockaddr *)&client, &clisize);

        if (r >= 0) {
            uprintf("broadcast server received a message");
            if (magic == BROADCAST_MAGIC) {
                sceNetSendto(serv, &magic, sizeof(uint32_t), 0, (struct sockaddr *)&client, clisize);
            }
        } else {
            uprintf("sceNetRecvfrom failed");
        }

        sceKernelSleep(1);
    }

    return NULL;
}

// Function responsible for initializing, and starting the PS4Debug Server
int start_server() {
    struct sockaddr_in server; // Structure for server address
    struct sockaddr_in client; // Structure for client address
    struct server_client *svc; // Pointer to structure for server-client communication
    unsigned int len = sizeof(client); // Size of client address structure
    int serv; // Server socket file descriptor
    int fd; // Client socket file descriptor
    int r; // Generic return value holder

    // Print server start message
    uprintf("ps4debug " PACKET_VERSION " server started");

    // Create broadcast thread
    ScePthread broadcast;
    scePthreadCreate(&broadcast, NULL, broadcast_thread, NULL, "broadcast");

    // Initialize server address structure
    server.sin_len = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = IN_ADDR_ANY;
    server.sin_port = sceNetHtons(SERVER_PORT);

    // Clear padding in address structure
    memset(server.sin_zero, NULL, sizeof(server.sin_zero));

    // Create server socket, then check if the socket creation 
    // failed, and if so handle the error
    serv = sceNetSocket("debugserver", AF_INET, SOCK_STREAM, 0);
    if (serv < 0) {
        uprintf("could not create socket!");
        return 1;
    }

    // Configure server socket
    configure_socket(serv);

    // Bind server socket to address
    if (sceNetBind(serv, (struct sockaddr *)&server, sizeof(server))) {
        uprintf("bind failed!");
        return 1;
    }

    // Listen for incoming connections
    if (sceNetListen(serv, SERVER_MAXCLIENTS * 2)) {
        uprintf("bind failed!");
        return 1;
    }

    // Reset client structures
    memset(servclients, NULL, sizeof(struct server_client) * SERVER_MAXCLIENTS);

    // Reset debugging variables
    g_debugging = 0;
    curdbgcli = NULL;
    curdbgctx = NULL;

    // Main server loop
    while (1) {
        // TODO: Comment this line
        scePthreadYield();

        errno = NULL;

        // Accept incoming connection, and check if a connection was has
        // successfully accepted 
        fd = sceNetAccept(serv, (struct sockaddr *)&client, &len);
        if (fd > -1 && !errno) {
            uprintf("accepted a new client");

            // TODO: Comment this part
            svc = alloc_client();
            if (!svc) {
                uprintf("server can not accept anymore clients");
                continue;
            }

            // TODO: Fully comment this part!
            // Configure ? socket
            configure_socket(fd);

            svc->fd = fd;
            svc->debugging = 0;

            // Copy client address to client structure
            memcpy(&svc->client, &client, sizeof(svc->client));

            // Clear debugging context
            memset(&svc->dbgctx, NULL, sizeof(svc->dbgctx));

            // Create client handler thread
            ScePthread thread;
            scePthreadCreate(&thread, NULL, (void *(*)(void *))handle_client, (void *)svc, "clienthandler");
        }

        // Sleep for 2 ticks
        sceKernelSleep(2);
    }

    return 0;
}
