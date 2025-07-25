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
#include<error.h>


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
    if (seekTo > entry->vnode->sizeInByte) {
        return INVALID_INDEX64;
    }
    
    return entry->pointer = seekTo;
}

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    vNode* vnode = entry->vnode;

    if (entry->pointer + n > vnode->sizeInByte) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    vNode_rawReadData(vnode, entry->pointer, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    vNode* vnode = entry->vnode;

    if (entry->pointer + n > vnode->sizeInByte) {
        vNode_rawResize(vnode, entry->pointer + n);
        ERROR_GOTO_IF_ERROR(0);
    }

    vNode_rawWriteData(vnode, entry->pointer, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

fsEntry* fsEntry_copy(fsEntry* entry) {
    fsEntry* ret = mm_allocate(sizeof(fsEntry));
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    memory_memcpy(ret, entry, sizeof(fsEntry));
    REF_COUNTER_REFER(ret->vnode->refCounter);

    return ret;
    ERROR_FINAL_BEGIN(0);
    return NULL;
}