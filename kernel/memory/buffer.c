#include<memory/buffer.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<structs/singlyLinkedList.h>
#include<system/pageTable.h>

static size_t _bufferSizes[] = {
    16, 32, 64, 128, 256, 512, 1024, 2048, 4096
};

static size_t _bufferNums[9];
static size_t _freeBufferNums[9];

SinglyLinkedList bufferLists[9];

/**
 * @brief Add a page to the buffer list, increase only for now
 * 
 * @param size Buffer size
 */
void __addPage(BufferSizes size);

void initBuffer() {
    for (int i = 0; i < 9; ++i) {
        initSinglyLinkedList(&bufferLists[i]);

        _bufferNums[i] = _freeBufferNums[i] = 0;
        __addPage((BufferSizes)i);
    }
}

size_t getTotalBufferNum(BufferSizes size) {
    return _bufferNums[size];
}

size_t getFreeBufferNum(BufferSizes size) {
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

void __addPage(BufferSizes size) {
    _bufferNums[size] += PAGE_SIZE / _bufferSizes[size];
    _freeBufferNums[size] += PAGE_SIZE / _bufferSizes[size];

    void* page = pageAlloc(1);
    for (int j = 0; j < PAGE_SIZE; j += _bufferSizes[size]) {
        SinglyLinkedListNode* node = (SinglyLinkedListNode*)(page + j);
        initSinglyLinkedListNode(node);

        singlyLinkedListInsertNext(&bufferLists[size], node);
    }
}