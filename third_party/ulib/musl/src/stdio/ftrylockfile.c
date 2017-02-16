#include "pthread_impl.h"
#include "stdio_impl.h"
#include <limits.h>

void __do_orphaned_stdio_locks(void) {
    FILE* f;
    for (f = __pthread_self()->stdio_locks; f; f = f->next_locked)
        atomic_store(&f->lock, 0x40000000);
}

void __unlist_locked_file(FILE* f) {
    if (f->lockcount) {
        if (f->next_locked)
            f->next_locked->prev_locked = f->prev_locked;
        if (f->prev_locked)
            f->prev_locked->next_locked = f->next_locked;
        else
            __pthread_self()->stdio_locks = f->next_locked;
    }
}

int ftrylockfile(FILE* f) {
    pthread_t self = __pthread_self();
    int tid = __thread_get_tid();
    if (f->lock == tid) {
        if (f->lockcount == LONG_MAX)
            return -1;
        f->lockcount++;
        return 0;
    }
    if (atomic_load(&f->lock) < 0)
        atomic_store(&f->lock, 0);
    if (atomic_load(&f->lock) || a_cas_shim(&f->lock, 0, tid))
        return -1;
    f->lockcount = 1;
    f->prev_locked = 0;
    f->next_locked = self->stdio_locks;
    if (f->next_locked)
        f->next_locked->prev_locked = f;
    self->stdio_locks = f;
    return 0;
}
