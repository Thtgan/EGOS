#include<memory/buffer.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/paging/paging.h>
#include<memory/physicalPages.h>
#include<structs/singlyLinkedList.h>

static Size _bufferSizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

static Size _bufferNums[9];
static Size _freeBufferNums[9];

SinglyLinkedList bufferLists[9];

/**
 * @brief Add a page to the buffer list, increase only for now
 * 
 * @param size Buffer size
 * 
 * @return Result Result of the operation
 */
Result __addPage(Index8 level);

Result initBuffer() {
    for (int i = 0; i < 9; ++i) {
        singlyLinkedList_initStruct(&bufferLists[i]);

        _bufferNums[i] = _freeBufferNums[i] = 0;
        if (__addPage((BufferSizes)i) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}

Size getTotalBufferNum(BufferSizes size) {
    return _bufferNums[size - BUFFER_SIZE_16];
}

Size getFreeBufferNum(BufferSizes size) {
    return _freeBufferNums[size - BUFFER_SIZE_16];
}

void* allocateBuffer(BufferSizes size) {
    Index8 level = size - BUFFER_SIZE_16;
    if (singlyLinkedList_isEmpty(&bufferLists[level])) {
        __addPage(level);
    }

    void* ret = singlyLinkedList_getNext(&bufferLists[level]);
    singlyLinkedList_deleteNext(&bufferLists[level]);
    return ret;
}

void releaseBuffer(void* buffer, BufferSizes size) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)buffer;

    singlyLinkedListNode_initStruct(node);
    singlyLinkedList_insertNext(&bufferLists[size - BUFFER_SIZE_16], node);
}

Result __addPage(Index8 level) {
    _bufferNums[level] += PAGE_SIZE / _bufferSizes[level];
    _freeBufferNums[level] += PAGE_SIZE / _bufferSizes[level];

    void* page = pageAlloc(1, MEMORY_TYPE_NORMAL);
    if (page == NULL) {
        return RESULT_FAIL;
    }

    page = convertAddressP2V(page);
    for (int j = 0; j < PAGE_SIZE; j += _bufferSizes[level]) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)(page + j);
        singlyLinkedListNode_initStruct(node);

        singlyLinkedList_insertNext(&bufferLists[level], node);
    }

    return RESULT_SUCCESS;
}