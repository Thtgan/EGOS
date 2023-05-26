#include<memory/buffer.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/physicalPages.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

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
Result __addPage(BufferSizes size);

Result initBuffer() {
    for (int i = 0; i < 9; ++i) {
        initSinglyLinkedList(&bufferLists[i]);

        _bufferNums[i] = _freeBufferNums[i] = 0;
        if (__addPage((BufferSizes)i) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return RESULT_SUCCESS;
}

Size getTotalBufferNum(BufferSizes size) {
    return _bufferNums[size];
}

Size getFreeBufferNum(BufferSizes size) {
    return _freeBufferNums[size];
}

void* allocateBuffer(BufferSizes size) {
    if (isSinglyListEmpty(&bufferLists[size])) {
        __addPage(size);
    }

    void* ret = singlyLinkedListGetNext(&bufferLists[size]);
    singlyLinkedListDeleteNext(&bufferLists[size]);
    return ret;
}

void releaseBuffer(void* buffer, BufferSizes size) {
    SinglyLinkedListNode* node = (SinglyLinkedListNode*)buffer;

    initSinglyLinkedListNode(node);
    singlyLinkedListInsertNext(&bufferLists[size], node);
}

Result __addPage(BufferSizes size) {
    _bufferNums[size] += PAGE_SIZE / _bufferSizes[size];
    _freeBufferNums[size] += PAGE_SIZE / _bufferSizes[size];

    void* page = pageAlloc(1, PHYSICAL_PAGE_TYPE_NORMAL);
    if (page == NULL) {
        return RESULT_FAIL;
    }

    for (int j = 0; j < PAGE_SIZE; j += _bufferSizes[size]) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)(page + j);
        initSinglyLinkedListNode(node);

        singlyLinkedListInsertNext(&bufferLists[size], node);
    }

    return RESULT_SUCCESS;
}