#include<mm/mm.h>

#include<kit/types.h>
#include<kit/util.h>
#include<lib/blowup.h>
#include<lib/memory.h>
#include<lib/print.h>
#include<lib/singlyLinkedList.h>
#include<system/E820.h>
#include<system/memoryLayout.h>
#include<system/memoryMap.h>

#define __DEFAULT_ALIGN 16

static SinglyLinkedList _freeList;

typedef struct {
    SinglyLinkedListNode node;
    Size nodeLength;
} __MemoryNode;

static __MemoryNode* __initMemoryNode(void* base, Size length);

static SinglyLinkedListNode* __firstFitSearch(Size size, Size align);

Result initMemoryManager(MemoryMap* mMap) {
    MemoryMapEntry* entry = findE820Entry(mMap, MEMORY_LAYOUT_BOOT_MEMALLOC_BEGIN, MEMORY_LAYOUT_BOOT_MEMALLOC_END - MEMORY_LAYOUT_BOOT_MEMALLOC_BEGIN, true);
    if (entry == NULL) {
        return RESULT_ERROR;
    }

    if (E820SplitEntry(mMap, entry, MEMORY_LAYOUT_BOOT_MEMALLOC_BEGIN - entry->base, &entry) == RESULT_ERROR) {
        return RESULT_ERROR;
    }
    entry->type = MEMORY_MAP_ENTRY_TYPE_RESERVED;

    if (E820SplitEntry(mMap, entry, MEMORY_LAYOUT_BOOT_MEMALLOC_END - MEMORY_LAYOUT_BOOT_MEMALLOC_BEGIN, &entry) == RESULT_ERROR) {
        return RESULT_ERROR;
    }

    entry->type = MEMORY_MAP_ENTRY_TYPE_RAM;

    initSinglyLinkedList(&_freeList);
    __MemoryNode* firstNode = __initMemoryNode((void*)(Uintptr)entry->base, entry->length);
    singlyLinkedListInsertNext(&_freeList, &firstNode->node);

    return RESULT_SUCCESS;
}

void* bMalloc(Size n) {
    return bMallocAligned(n, __DEFAULT_ALIGN);
}

void* bMallocAligned(Size n, Size align) {
    if (n == 0) {
        return NULL;
    }

    SinglyLinkedListNode* lastNode = __firstFitSearch(n, align);
    if (lastNode == NULL) {
        blowup("ERROR: Not enough memory\n");
        return NULL;
    }

    __MemoryNode* node = HOST_POINTER(lastNode->next, __MemoryNode, node);
    void* ret = (void*)ALIGN_UP((Uintptr)node, align);
    Uintptr 
        base1 = (Uintptr)node, length1 = ALIGN_UP(base1, align) - base1,
        base2 = ALIGN_UP((Uintptr)ret + n, __DEFAULT_ALIGN), length2 = (Uintptr)node + node->nodeLength - base2;
    singlyLinkedListDeleteNext(lastNode);

    if (length2 != 0) {
        if (length2 < sizeof(__MemoryNode)) {
            printf("WARNING: Region (%#010X, %#010X) is not enough to form memory node\n", base2, length2);
        } else {
            __MemoryNode* newNode = __initMemoryNode((void*)base2, length2);
            singlyLinkedListInsertNext(lastNode, &newNode->node);
        }
    }

    if (length1 != 0) {
        if (length1 < sizeof(__MemoryNode)) {
            printf("WARNING: Region (%#010X, %#010X) is not enough to form memory node\n", base1, length1);
        } else {
            __MemoryNode* newNode = __initMemoryNode((void*)base1, length1);
            singlyLinkedListInsertNext(lastNode, &newNode->node);
        }
    }

    memset(ret, 0, n);
    return ret;
}

void bFree(void* p, Size n) {
    SinglyLinkedListNode* position = &_freeList;
    n = ALIGN_UP((Uintptr)p + n, __DEFAULT_ALIGN) - (Uintptr)p;
    __MemoryNode* memoryNode = __initMemoryNode(p, n);

    for (
        SinglyLinkedListNode* next = singlyLinkedListGetNext(position);
        !(next == &_freeList || (void*)next > (void*)&memoryNode->node);
        position = next, next = singlyLinkedListGetNext(position)
        );
    
    singlyLinkedListInsertNext(position, &memoryNode->node);

    SinglyLinkedListNode* prevListNode = position, * nextListNode = singlyLinkedListGetNext(&memoryNode->node);
    __MemoryNode* combinedNode = memoryNode;
    if (prevListNode != &_freeList) {
        __MemoryNode* prevNode = HOST_POINTER(prevListNode, __MemoryNode, node);
        if ((void*)prevNode + prevNode->nodeLength == (void*)combinedNode) {
            singlyLinkedListDeleteNext(&prevNode->node);
            prevNode->nodeLength += combinedNode->nodeLength;
            combinedNode = prevNode;
        }
    }

    if (nextListNode != &_freeList) {
        __MemoryNode* nextNode = HOST_POINTER(nextListNode, __MemoryNode, node);
        if ((void*)combinedNode + combinedNode->nodeLength == (void*)nextNode) {
            singlyLinkedListDeleteNext(&combinedNode->node);
            combinedNode->nodeLength += nextNode->nodeLength;
        }
    }
}

static __MemoryNode* __initMemoryNode(void* base, Size length) {
    __MemoryNode* ret = (__MemoryNode*)base;
    initSinglyLinkedListNode(&ret->node);
    ret->nodeLength = length;

    return ret;
}

static SinglyLinkedListNode* __firstFitSearch(Size size, Size align) {
    if (isSinglyListEmpty(&_freeList)) {
        return NULL;
    }

    for (SinglyLinkedListNode* node = _freeList.next, * last = &_freeList; node != &_freeList; last = node, node = singlyLinkedListGetNext(node)) {
        __MemoryNode* memoryNode = HOST_POINTER(node, __MemoryNode, node);
        Uintptr base = (Uintptr)memoryNode;
        if (base + memoryNode->nodeLength > ALIGN_UP(base, align) && base + memoryNode->nodeLength - ALIGN_UP(base, align) >= size) {
            return last;
        }
    }

    return NULL;
}
