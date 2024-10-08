// golden
// 6/12/2018
//

#include "proc.h"

struct proc *proc_find_by_name(const char *name) {
    struct proc *p;

    if (!name) {
        return NULL;
    }

    p = *allproc;
    do {
        if (!memcmp(p->p_comm, name, strlen(name))) {
            return p;
        }
    } while ((p = p->p_forw));

    return NULL;
}

struct proc *proc_find_by_pid(int pid) {
    struct proc *p;

    p = *allproc;
    do {
        if (p->pid == pid) {
            return p;
        }
    } while ((p = p->p_forw));

    return NULL;
}

int proc_get_vm_map(struct proc *p, struct proc_vm_map_entry **entries, uint64_t *num_entries) {
    struct proc_vm_map_entry *info = NULL;
    struct vm_map_entry *entry = NULL;
    int r = 0;

    struct vmspace *vm = p->p_vmspace;
    struct vm_map *map = &vm->vm_map;

    vm_map_lock_read(map);

    int num = map->nentries;
    if (!num) goto error;

    r = vm_map_lookup_entry(map, NULL, &entry);
    if (r) goto error;


    info = (struct proc_vm_map_entry *)malloc(num * sizeof(struct proc_vm_map_entry), M_TEMP, 2);
    if (!info) {
        r = 1;
        goto error;
    }

    for (int i = 0; i < num; i++) {
        info[i].start = entry->start;
        info[i].end = entry->end;
        info[i].offset = entry->offset;
        info[i].prot = entry->prot & (entry->prot >> 8);
        memcpy(info[i].name, entry->name, sizeof(info[i].name));

        if (!(entry = entry->next))
            break;
    }

    error:
    vm_map_unlock_read(map);

    if (entries)     *entries = info;
    if (num_entries) *num_entries = num;

    return 0;
}

int proc_rw_mem(struct proc *p, void *ptr, uint64_t size, void *data, uint64_t *n, int write) {
    if (!p) return 1;

    if (size == 0) {
        if (n) *n = 0;
        return 0;
    }

    struct thread *td = curthread();
    struct iovec iov;
    struct uio uio;
    int r = 0;

    // Initialize both the uio and iovec struct instances
    // to NULL, clearing out any junk
    memset(&iov, NULL, sizeof(iov));
    memset(&uio, NULL, sizeof(uio));

    // TODO: Comment what the purpose of iovec is
    iov.iov_base = (uint64_t)data;
    iov.iov_len = size;

    // TODO: Comment what the purpose of uio is
    uio.uio_iov = (uint64_t)&iov;
    uio.uio_iovcnt = 1;
    uio.uio_offset = (uint64_t)ptr;
    uio.uio_resid = (uint64_t)size;
    uio.uio_segflg = UIO_SYSSPACE;
    uio.uio_rw = write ? UIO_WRITE : UIO_READ;
    uio.uio_td = td;

    r = proc_rwmem(p, &uio);

    if (n) *n = (uint64_t)((uint64_t)size - uio.uio_resid);

    return r;
}

inline int proc_read_mem(struct proc *p, void *ptr, uint64_t size, void *data, uint64_t *n) {
    return proc_rw_mem(p, ptr, size, data, n, 0);
}

inline int proc_write_mem(struct proc *p, void *ptr, uint64_t size, void *data, uint64_t *n) {
    return proc_rw_mem(p, ptr, size, data, n, 1);
}

int proc_allocate(struct proc *p, void **address, uint64_t size) {
    if (!address) return 1;

    uint64_t addr = NULL;
    int r = 0;
    uint64_t alignedSize = (size + 0x3FFFull) & ~0x3FFFull;
    struct vmspace *vm = p->p_vmspace;
    struct vm_map *map = &vm->vm_map;

    vm_map_lock(map);

    r = vm_map_findspace(map, NULL, size, &addr);
    if (r) {
        vm_map_unlock(map);
        return r;
    }

    r = vm_map_insert(map, NULL, NULL, addr, addr + alignedSize, VM_PROT_ALL, VM_PROT_ALL, 0);
    vm_map_unlock(map);

    if (r) return r;
    if (address) *address = (void *)addr;

    return r;
}

int proc_deallocate(struct proc *p, void *address, uint64_t size) {
    uint64_t alignedSize = (size + 0x3FFFull) & ~0x3FFFull;

    struct vmspace *vm = p->p_vmspace;
    struct vm_map *map = &vm->vm_map;

    vm_map_lock(map);
    int result = vm_map_delete(map, (uint64_t)address, (uint64_t)address + alignedSize);
    vm_map_unlock(map);

    return result;
}

int proc_mprotect(struct proc *p, void *address, uint64_t size, int new_prot) {
    uint64_t alignedSize = (size + 0x3FFFull) & ~0x3FFFull;
    uint64_t addr = (uint64_t)address;
    uint64_t addrend = addr + alignedSize;

    struct vmspace *vm = p->p_vmspace;
    struct vm_map *map = &vm->vm_map;

    // update the max prot then set new prot
    int result = vm_map_protect(map, addr, addrend, new_prot, 1);
    if (result) return result;

    return vm_map_protect(map, addr, addrend, new_prot, 0);
}

int proc_create_thread(struct proc *p, uint64_t address) {
    struct proc_vm_map_entry *entries = NULL;
    void *rpcldraddr = NULL;
    void *stackaddr = NULL;
    uint64_t num_entries = 0;
    uint64_t n = 0;
    int r = 0;

    // Calculate the size of our Loader
    uint64_t ldrsize = sizeof(rpcldr);
    ldrsize += (PAGE_SIZE - (ldrsize % PAGE_SIZE));

    // Set the Stack Max? Size
    uint64_t stacksize = 0x80000;

    // Allocate Memory for RPC Loader in Process
    r = proc_allocate(p, &rpcldraddr, ldrsize);
    if (r) goto error;

    // Allocate Memory for RPC Stack? in Process
    r = proc_allocate(p, &stackaddr, stacksize);
    if (r) goto error;

    // Write the RPC Loader to the Allocated Memory 
    r = proc_write_mem(p, rpcldraddr, sizeof(rpcldr), (void *)rpcldr, &n);
    if (r) goto error;

    // Create our Donor Thread
    struct thread *thr = TAILQ_FIRST(&p->p_threads);

    // Try to find libkernel base
    r = proc_get_vm_map(p, &entries, &num_entries);
    if (r) goto error;

    // The memory addresses/offsets? to the coresponding functions present
    // in the memory of either one of following system modules:
    // libkernel.sprx, libkernel_web.sprx, or libkernel_sys.sprx
    uint64_t _scePthreadAttrInit = 0;
    uint64_t _scePthreadAttrSetstacksize = 0;
    uint64_t _scePthreadCreate = 0;
    uint64_t _thr_initial = 0;

    for (int i = 0; i < num_entries; i++) {
        // First we make sure that the protection set for the current entry inside
        // entries, prevents it's memory? from being read and executed? if true we
        // skip to the next entry in entries and try again
        if (entries[i].prot != (PROT_READ | PROT_EXEC))
            continue;

       if (!memcmp(entries[i].name, "libkernel.sprx", 14)) {
           switch (cachedFirmware) {
           case 505:
               _scePthreadAttrInit = entries[i].start + 0x12660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x12680;
               _scePthreadCreate = entries[i].start + 0x12AA0;
               _thr_initial = entries[i].start + 0x84C20;
               break;
           case 672:
               _scePthreadAttrInit = entries[i].start + 0x13A40;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13A60;
               _scePthreadCreate = entries[i].start + 0x13E80;
               _thr_initial = entries[i].start + 0x435420;
               break;
           case 702:
               _scePthreadAttrInit = entries[i].start + 0x136E0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13700;
               _scePthreadCreate = entries[i].start + 0x13B20;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 750:
               _scePthreadAttrInit = entries[i].start + 0x13630;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13650;
               _scePthreadCreate = entries[i].start + 0x13A70;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 751:
               _scePthreadAttrInit = entries[i].start + 0x13630;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13650;
               _scePthreadCreate = entries[i].start + 0x13A70;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 755:
               _scePthreadAttrInit = entries[i].start + 0x13630;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13650;
               _scePthreadCreate = entries[i].start + 0x13A70;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 800:
               _scePthreadAttrInit = entries[i].start + 0x135D0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x135F0;
               _scePthreadCreate = entries[i].start + 0x13A10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 801:
               _scePthreadAttrInit = entries[i].start + 0x135D0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x135F0;
               _scePthreadCreate = entries[i].start + 0x13A10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 803:
               _scePthreadAttrInit = entries[i].start + 0x135D0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x135F0;
               _scePthreadCreate = entries[i].start + 0x13A10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 850:
               _scePthreadAttrInit = entries[i].start + 0x13660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13680;
               _scePthreadCreate = entries[i].start + 0x13AA0;
               _thr_initial = entries[i].start + 0x89430;
               break;
           case 852:
               _scePthreadAttrInit = entries[i].start + 0x13660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13680;
               _scePthreadCreate = entries[i].start + 0x13AA0;
               _thr_initial = entries[i].start + 0x89430;
               break;
           case 900:
               _scePthreadAttrInit = entries[i].start + 0x13660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13680;
               _scePthreadCreate = entries[i].start + 0x13AA0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 903:
               _scePthreadAttrInit = entries[i].start + 0x13660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13680;
               _scePthreadCreate = entries[i].start + 0x13AA0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 904:
               _scePthreadAttrInit = entries[i].start + 0x13660;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13680;
               _scePthreadCreate = entries[i].start + 0x13AA0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 950:
               _scePthreadAttrInit = entries[i].start + 0x132F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13310;
               _scePthreadCreate = entries[i].start + 0x13730;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 951:
               _scePthreadAttrInit = entries[i].start + 0x132F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13310;
               _scePthreadCreate = entries[i].start + 0x13730;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 960:
               _scePthreadAttrInit = entries[i].start + 0x132F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13310;
               _scePthreadCreate = entries[i].start + 0x13730;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1000:
               _scePthreadAttrInit = entries[i].start + 0x132F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13310;
               _scePthreadCreate = entries[i].start + 0x13730;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1001:
               _scePthreadAttrInit = entries[i].start + 0x132F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13310;
               _scePthreadCreate = entries[i].start + 0x13730;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1050:
               _scePthreadAttrInit = entries[i].start + 0x13400;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13420;
               _scePthreadCreate = entries[i].start + 0x13840;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1070:
               _scePthreadAttrInit = entries[i].start + 0x13400;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13420;
               _scePthreadCreate = entries[i].start + 0x13840;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1071:
               _scePthreadAttrInit = entries[i].start + 0x13400;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13420;
               _scePthreadCreate = entries[i].start + 0x13840;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1100:
               _scePthreadAttrInit = entries[i].start + 0x134A0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x134C0;
               _scePthreadCreate = entries[i].start + 0x138E0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           }
           break;
       }
       if (!memcmp(entries[i].name, "libkernel_web.sprx", 18)) {
           switch (cachedFirmware) {
           case 505:
               _scePthreadAttrInit = entries[i].start + 0x1E730;
               _scePthreadAttrSetstacksize = entries[i].start + 0xFA80;
               _scePthreadCreate = entries[i].start + 0x98C0;
               _thr_initial = entries[i].start + 0x84C20;
               break;
           case 672:
               _scePthreadAttrInit = entries[i].start + 0x1FD20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10540;
               _scePthreadCreate = entries[i].start + 0xA0F0;
               _thr_initial = entries[i].start + 0x435420;
               break;
           case 702:
               _scePthreadAttrInit = entries[i].start + 0x1F9B0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x103C0;
               _scePthreadCreate = entries[i].start + 0x9FF0;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 750:
               _scePthreadAttrInit = entries[i].start + 0x1FA70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10380;
               _scePthreadCreate = entries[i].start + 0x9F10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 751:
               _scePthreadAttrInit = entries[i].start + 0x1FA70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10380;
               _scePthreadCreate = entries[i].start + 0x9F10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 755:
               _scePthreadAttrInit = entries[i].start + 0x1FA70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10380;
               _scePthreadCreate = entries[i].start + 0x9F10;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 800:
               _scePthreadAttrInit = entries[i].start + 0x1F920;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10270;
               _scePthreadCreate = entries[i].start + 0x9F60;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 801:
               _scePthreadAttrInit = entries[i].start + 0x1F920;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10270;
               _scePthreadCreate = entries[i].start + 0x9F60;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 803:
               _scePthreadAttrInit = entries[i].start + 0x1F920;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10270;
               _scePthreadCreate = entries[i].start + 0x9F60;
               _thr_initial = entries[i].start + 0x8D420;
               break;
           case 850:
               _scePthreadAttrInit = entries[i].start + 0x18D50;
               _scePthreadAttrSetstacksize = entries[i].start + 0x86C0;
               _scePthreadCreate = entries[i].start + 0x286B0;
               _thr_initial = entries[i].start + 0x89430;
               break;
           case 852:
               _scePthreadAttrInit = entries[i].start + 0x18D50;
               _scePthreadAttrSetstacksize = entries[i].start + 0x86C0;
               _scePthreadCreate = entries[i].start + 0x286B0;
               _thr_initial = entries[i].start + 0x89430;
               break;
           case 900:
               _scePthreadAttrInit = entries[i].start + 0x87F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x1A580;
               _scePthreadCreate = entries[i].start + 0x204C0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 903:
               _scePthreadAttrInit = entries[i].start + 0x87F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x1A580;
               _scePthreadCreate = entries[i].start + 0x204C0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 904:
               _scePthreadAttrInit = entries[i].start + 0x87F0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x1A580;
               _scePthreadCreate = entries[i].start + 0x204C0;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1000:
               _scePthreadAttrInit = entries[i].start + 0x14C20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x106D0;
               _scePthreadCreate = entries[i].start + 0x5800;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1001:
               _scePthreadAttrInit = entries[i].start + 0x14C20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x106D0;
               _scePthreadCreate = entries[i].start + 0x5800;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1050:
               _scePthreadAttrInit = entries[i].start + 0x17D80;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10660;
               _scePthreadCreate = entries[i].start + 0x1BE60;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1070:
               _scePthreadAttrInit = entries[i].start + 0x17D80;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10660;
               _scePthreadCreate = entries[i].start + 0x1BE60;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1071:
               _scePthreadAttrInit = entries[i].start + 0x17D80;
               _scePthreadAttrSetstacksize = entries[i].start + 0x10660;
               _scePthreadCreate = entries[i].start + 0x1BE60;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           case 1100:
               _scePthreadAttrInit = entries[i].start + 0x15990;
               _scePthreadAttrSetstacksize = entries[i].start + 0xE800;
               _scePthreadCreate = entries[i].start + 0x20D90;
               _thr_initial = entries[i].start + 0x8E430;
               break;
           }
           break;
       }
       if (!memcmp(entries[i].name, "libkernel_sys.sprx", 18)) {
           switch (cachedFirmware) {
           case 505:
               _scePthreadAttrInit = entries[i].start + 0x13190;
               _scePthreadAttrSetstacksize = entries[i].start + 0x131B0;
               _scePthreadCreate = entries[i].start + 0x135D0;
               _thr_initial = entries[i].start + 0x89030;
               break;
           case 672:
               _scePthreadAttrInit = entries[i].start + 0x14570;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14590;
               _scePthreadCreate = entries[i].start + 0x149B0;
               _thr_initial = entries[i].start + 0x435830;
               break;
           case 700:
               _scePthreadAttrInit = entries[i].start + 0x14160;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14180;
               _scePthreadCreate = entries[i].start + 0x145A0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 701:
               _scePthreadAttrInit = entries[i].start + 0x14210;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14230;
               _scePthreadCreate = entries[i].start + 0x14650;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 702:
               _scePthreadAttrInit = entries[i].start + 0x14210;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14230;
               _scePthreadCreate = entries[i].start + 0x14650;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 750:
               _scePthreadAttrInit = entries[i].start + 0x14160;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14180;
               _scePthreadCreate = entries[i].start + 0x145A0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 751:
               _scePthreadAttrInit = entries[i].start + 0x14160;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14180;
               _scePthreadCreate = entries[i].start + 0x145A0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 755:
               _scePthreadAttrInit = entries[i].start + 0x14160;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14180;
               _scePthreadCreate = entries[i].start + 0x145A0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 800:
               _scePthreadAttrInit = entries[i].start + 0x153C0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x154F0;
               _scePthreadCreate = entries[i].start + 0x14540;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 801:
               _scePthreadAttrInit = entries[i].start + 0x153C0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x154F0;
               _scePthreadCreate = entries[i].start + 0x14540;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 803:
               _scePthreadAttrInit = entries[i].start + 0x153C0;
               _scePthreadAttrSetstacksize = entries[i].start + 0x154F0;
               _scePthreadCreate = entries[i].start + 0x14540;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 850:
               _scePthreadAttrInit = entries[i].start + 0x15450;
               _scePthreadAttrSetstacksize = entries[i].start + 0x15580;
               _scePthreadCreate = entries[i].start + 0x145D0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 852:
               _scePthreadAttrInit = entries[i].start + 0x15450;
               _scePthreadAttrSetstacksize = entries[i].start + 0x15580;
               _scePthreadCreate = entries[i].start + 0x145D0;
               _thr_initial = entries[i].start + 0x8D830;
               break;
           case 900:
               _scePthreadAttrInit = entries[i].start + 0x14190;
               _scePthreadAttrSetstacksize = entries[i].start + 0x141B0;
               _scePthreadCreate = entries[i].start + 0x145D0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 903:
               _scePthreadAttrInit = entries[i].start + 0x14190;
               _scePthreadAttrSetstacksize = entries[i].start + 0x141B0;
               _scePthreadCreate = entries[i].start + 0x145D0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 904:
               _scePthreadAttrInit = entries[i].start + 0x14190;
               _scePthreadAttrSetstacksize = entries[i].start + 0x141B0;
               _scePthreadCreate = entries[i].start + 0x145D0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 950:
               _scePthreadAttrInit = entries[i].start + 0x13E20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13E40;
               _scePthreadCreate = entries[i].start + 0x14260;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 951:
               _scePthreadAttrInit = entries[i].start + 0x13E20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13E40;
               _scePthreadCreate = entries[i].start + 0x14260;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 960:
               _scePthreadAttrInit = entries[i].start + 0x13E20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13E40;
               _scePthreadCreate = entries[i].start + 0x14260;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1000:
               _scePthreadAttrInit = entries[i].start + 0x13E20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13E40;
               _scePthreadCreate = entries[i].start + 0x14260;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1001:
               _scePthreadAttrInit = entries[i].start + 0x13E20;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13E40;
               _scePthreadCreate = entries[i].start + 0x14260;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1050:
               _scePthreadAttrInit = entries[i].start + 0x13F70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13F90;
               _scePthreadCreate = entries[i].start + 0x143B0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1070:
               _scePthreadAttrInit = entries[i].start + 0x13F70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13F90;
               _scePthreadCreate = entries[i].start + 0x143B0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1071:
               _scePthreadAttrInit = entries[i].start + 0x13F70;
               _scePthreadAttrSetstacksize = entries[i].start + 0x13F90;
               _scePthreadCreate = entries[i].start + 0x143B0;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           case 1100:
               _scePthreadAttrInit = entries[i].start + 0x14010;
               _scePthreadAttrSetstacksize = entries[i].start + 0x14030;
               _scePthreadCreate = entries[i].start + 0x14450;
               _thr_initial = entries[i].start + 0x8E830;
               break;
           }
           break;
       }
   }

    // If the scePthreadAttrInit function address is invalid we
    // jump to the goto label, after which cleanup code is.
    if (!_scePthreadAttrInit) goto error;

    // Begin Writing Variables to the RPC Loader    
    // Write the address of the stub entry point to the RPC Loader's stubentry field
    r = proc_write_mem(p, rpcldraddr + offsetof(struct rpcldr_header, stubentry), sizeof(address), (void *)&address, &n);
    if (r) goto error;

    // Write the address of the scePthreadAttrInit function to the RPC Loader's scePthreadAttrInit field
    r = proc_write_mem(p, rpcldraddr + offsetof(struct rpcldr_header, scePthreadAttrInit), sizeof(_scePthreadAttrInit), (void *)&_scePthreadAttrInit, &n);
    if (r) goto error;

    // Write the address of the scePthreadAttrSetstacksize function to the RPC Loader's scePthreadAttrSetstacksize field
    r = proc_write_mem(p, rpcldraddr + offsetof(struct rpcldr_header, scePthreadAttrSetstacksize), sizeof(_scePthreadAttrSetstacksize), (void *)&_scePthreadAttrSetstacksize, &n);
    if (r) goto error;

    // Write the address of the scePthreadCreate function to the RPC Loader's scePthreadCreate field
    r = proc_write_mem(p, rpcldraddr + offsetof(struct rpcldr_header, scePthreadCreate), sizeof(_scePthreadCreate), (void *)&_scePthreadCreate, &n);
    if (r) goto error;

    // Write the address of the initial thread function to the RPC Loader's thr_initial field
    r = proc_write_mem(p, rpcldraddr + offsetof(struct rpcldr_header, thr_initial), sizeof(_thr_initial), (void *)&_thr_initial, &n);
    if (r) goto error;

    // Execute the RPC Loader? or just Loader?
    // NOTE: Do not enter in the pid information as it expects it to be stored in userland
    uint64_t ldrentryaddr = (uint64_t)rpcldraddr + *(uint64_t *)(rpcldr + 4);
    r = create_thread(
        thr,
        NULL,
        (void *)ldrentryaddr,
        NULL,
        stackaddr,
        stacksize,
        NULL, NULL, NULL, 0, NULL
    );
    if (r) goto error;

    // Begin waiting until the loader is done!
    uint8_t ldrdone = 0;
    while (!ldrdone) {
        // TODO: Comment this part
        r = proc_read_mem(
            p,
            (void *)(rpcldraddr + offsetof(struct rpcldr_header, ldrdone)),
            sizeof(ldrdone),
            &ldrdone, &n
        );
        if (r) goto error;
    }

    // Cleanup Part
    error:;
    if (entries != NULL)    free(entries, M_TEMP);
    if (rpcldraddr != NULL) proc_deallocate(p, rpcldraddr, ldrsize);
    if (stackaddr != NULL)  proc_deallocate(p, stackaddr, stacksize);
    return r;
}

int proc_map_elf(struct proc *p, void *elf, void *exec) {
    // Cast the input elf pointer to a 64-bit ELF header structure
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)elf;

    // Obtain the program header from the ELF header
    struct Elf64_Phdr *phdr = elf_pheader(ehdr);
    if (phdr) {
        // Iterate over all program headers if they exist (use segments)
        for (int i = 0; i < ehdr->e_phnum; i++) {
            // TODO: Comment this line
            struct Elf64_Phdr *phdr = elf_segment(ehdr, i);

            // If the segment has a file size (its not empty)
            if (phdr->p_filesz) {
                proc_write_mem(
                    p,
                    // Physical address of the segment in the exec space?
                    (void *)((uint8_t *)exec + phdr->p_paddr),
                    // Size of the segment in the file?
                    phdr->p_filesz,
                    // Address in the ELF file where the segment data is located?
                    (void *)((uint8_t *)elf + phdr->p_offset),
                    NULL // Unknown?
                );
            }
        }
    } else {
        // Iterate over all section headers if program headers do not exist (use sections)
        for (int i = 0; i < ehdr->e_shnum; i++) {
            // TODO: Comment this line
            struct Elf64_Shdr *shdr = elf_section(ehdr, i);

            // TODO: Is this comment true? do i need to adjust it? 
            // Skip sections that are not allocated in memory?
            if (!(shdr->sh_flags & SHF_ALLOC))
                continue;

            // Check if the section has a size (its not empty)
            if (shdr->sh_size) {
                proc_write_mem(
                    p,
                    (void *)((uint8_t *)exec + shdr->sh_addr),
                    shdr->sh_size,
                    (void *)((uint8_t *)elf + shdr->sh_offset),
                    NULL // Unknown?
                );
            }
        }
    }

    return 0;
}

int proc_relocate_elf(struct proc *p, void *elf, void *exec) {
    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)elf;

    for (int i = 0; i < ehdr->e_shnum; i++) {
        struct Elf64_Shdr *shdr = elf_section(ehdr, i);

        // check table
        if (shdr->sh_type == SHT_REL) {
            // process each entry in the table
            for (int j = 0; j < shdr->sh_size / shdr->sh_entsize; j++) {
                struct Elf64_Rela *reltab = &((struct Elf64_Rela *)((uint64_t)ehdr + shdr->sh_offset))[j];
                uint8_t **ref = (uint8_t **)((uint8_t *)exec + reltab->r_offset);
                uint8_t *value = NULL;

                switch (ELF64_R_TYPE(reltab->r_info)) {
                    case R_X86_64_RELATIVE:
                        // *ref = (uint8_t *)exec + reltab->r_addend;
                        value = (uint8_t *)exec + reltab->r_addend;
                        proc_write_mem(p, ref, sizeof(value), (void *)&value, NULL);
                        break;
                    case R_X86_64_64:
                    case R_X86_64_JUMP_SLOT:
                    case R_X86_64_GLOB_DAT:
                        // not supported
                        break;
                }
            }
        }
    }

    return 0;
}

int proc_load_elf(struct proc *p, void *elf, uint64_t *elfbase, uint64_t *entry) {
    void *elfaddr = NULL;
    uint64_t msize = 0;
    int r = 0;

    struct Elf64_Ehdr *ehdr = (struct Elf64_Ehdr *)elf;

    r = elf_mapped_size(elf, &msize);
    if (r) {
        goto error;
    }

    // resize to pages
    msize += (PAGE_SIZE - (msize % PAGE_SIZE));

    // allocate
    r = proc_allocate(p, &elfaddr, msize);
    if (r) {
        goto error;
    }

    // map
    r = proc_map_elf(p, elf, elfaddr);
    if (r) {
        goto error;
    }

    // relocate
    r = proc_relocate_elf(p, elf, elfaddr);
    if (r) {
        goto error;
    }

    if (elfbase) {
        *elfbase = (uint64_t)elfaddr;
    }

    if (entry) {
        *entry = (uint64_t)elfaddr + ehdr->e_entry;
    }

    error:
    return r;
}
