#include<fs/fsEntry.h>

#include<algorithms.h>
#include<cstring.h>
#include<devices/blockDevice.h>
#include<devices/charDevice.h>
#include<fs/fs.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<structs/refCounter.h>
#include<structs/string.h>
#include<system/pageTable.h>
#include<lib/errorPosix.h>


void fsEntry_initStruct(fsEntry* entry, vNode* vnode, fsEntryOperations* operations, FCNTLopenFlags flags) {
    entry->flags = flags;
    entry->pointer = 0;
    entry->vnode = vnode;
    entry->operations = operations;
}

void fsEntry_clearStruct(fsEntry* entry) {
    memory_memset(entry, 0, sizeof(fsEntry));
}

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    if (seekTo > entry->vnode->size) {
        return INVALID_INDEX64;
    }
    
    return entry->pointer = seekTo;
}

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    vNode* vnode = entry->vnode;

    ERROR_THROW_NEW_IF(entry->pointer + n > vnode->size, ERROR_INVALID_ARGUMENT, error_out);

    vNode_rawReadData(vnode, entry->pointer, buffer, n);
    CHECK_ERROR(error_out);

    return;
error_out:
}

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    vNode* vnode = entry->vnode;

    if (entry->pointer + n > vnode->size) {
        vNode_rawResize(vnode, entry->pointer + n);
        CHECK_ERROR(error_out);
    }

    vNode_rawWriteData(vnode, entry->pointer, buffer, n);
    CHECK_ERROR(error_out);

    return;
error_out:
}

fsEntry* fsEntry_copy(fsEntry* entry) {
    fsEntry* ret = mm_allocate(sizeof(fsEntry));
    ERROR_THROW_NEW_IF(ret == NULL, ERROR_OUT_OF_MEMORY, error_out);

    memory_memcpy(ret, entry, sizeof(fsEntry));
    REF_COUNTER_REFER(ret->vnode->refCounter);

    return ret;
error_out:
    return NULL;
}
