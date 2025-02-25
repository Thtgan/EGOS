#include<fs/fsEntry.h>

#include<algorithms.h>
#include<cstring.h>
#include<devices/blockDevice.h>
#include<devices/charDevice.h>
#include<fs/fs.h>
#include<kernel.h>
#include<kit/bit.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<system/pageTable.h>
#include<error.h>

#include<structs/refCounter.h>

void fsEntry_initStruct(fsEntry* entry, iNode* inode, fsEntryOperations* operations, FCNTLopenFlags flags) {
    entry->flags = flags;
    entry->pointer = 0;
    entry->inode = inode;
    entry->operations = operations;
}

void fsEntry_clearStruct(fsEntry* entry) {
    memory_memset(entry, 0, sizeof(fsEntry));
}

Index64 fsEntry_genericSeek(fsEntry* entry, Index64 seekTo) {
    if (seekTo > entry->inode->sizeInByte) {
        return INVALID_INDEX;
    }
    
    return entry->pointer = seekTo;
}

void fsEntry_genericRead(fsEntry* entry, void* buffer, Size n) {
    iNode* inode = entry->inode;

    if (entry->pointer + n > inode->sizeInByte) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    iNode_rawReadData(inode, entry->pointer, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsEntry_genericWrite(fsEntry* entry, const void* buffer, Size n) {
    iNode* inode = entry->inode;

    if (entry->pointer + n > inode->sizeInByte) {
        iNode_rawResize(inode, entry->pointer + n);
        ERROR_GOTO_IF_ERROR(0);
    }

    iNode_rawWriteData(inode, entry->pointer, buffer, n);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}