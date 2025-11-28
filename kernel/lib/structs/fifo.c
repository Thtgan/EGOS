#include<structs/fifo.h>

#include<memory/mm.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<multitask/locks/semaphore.h>
#include<structs/deque.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<debug.h>
#include<error.h>

#define __FIFO_NODE_BUFFER_SIZE (PAGE_SIZE - sizeof(DequeNode) - 2 * sizeof(Uint16))

typedef struct {
    DequeNode node;
    Uint16 begin, end;
    Uint8 buffer[__FIFO_NODE_BUFFER_SIZE];
} __FIFOnode;   //Not packed to avoid warning

DEBUG_ASSERT_COMPILE(sizeof(__FIFOnode) == PAGE_SIZE);

static __FIFOnode* __fifoNode_createNode();

static void __fifoNode_releaseNode(__FIFOnode* node);

void fifo_initStruct(FIFO* fifo) {
    deque_initStruct(&fifo->bufferQueue);
    fifo->bufferSize = 0;
    semaphore_initStruct(&fifo->queueLock, 1);
}

void fifo_write(FIFO* fifo, const void* buffer, Size n) {
    semaphore_down(&fifo->queueLock);

    if (n == 1) {
        DequeNode* lastQueueNode = deque_peekLast(&fifo->bufferQueue);
        __FIFOnode* lastNode = lastQueueNode == NULL ? NULL : HOST_POINTER(lastQueueNode, __FIFOnode, node);
        if (lastNode == NULL || lastNode->end == __FIFO_NODE_BUFFER_SIZE) {
            lastNode = __fifoNode_createNode();
            deque_pushLast(&fifo->bufferQueue, &lastNode->node);
        }

        lastNode->buffer[lastNode->end++] = *(Uint8*)buffer;
    } else {
        Size remainingN = n;
        const void* currentBuffer = buffer;
        while (remainingN > 0) {
            DequeNode* lastQueueNode = deque_peekLast(&fifo->bufferQueue);
            __FIFOnode* lastNode = lastQueueNode == NULL ? NULL : HOST_POINTER(lastQueueNode, __FIFOnode, node);
            if (lastNode == NULL || lastNode->end == __FIFO_NODE_BUFFER_SIZE) {
                lastNode = __fifoNode_createNode();
                deque_pushLast(&fifo->bufferQueue, &lastNode->node);
            }
    
            Size copyN = algorithms_umin64(__FIFO_NODE_BUFFER_SIZE - lastNode->end, remainingN);
            memory_memcpy(lastNode->buffer + lastNode->end, currentBuffer, copyN);
            lastNode->end += copyN;

            remainingN -= copyN;
            currentBuffer += copyN;
        }
    }

    fifo->bufferSize += n;

    semaphore_up(&fifo->queueLock);
}

Size fifo_regret(FIFO* fifo, Size n) {
    if (fifo->bufferSize == 0) {
        return 0;
    }

    semaphore_down(&fifo->queueLock);

    if (n == 1) {
        DequeNode* lastQueueNode = deque_peekLast(&fifo->bufferQueue);
        __FIFOnode* lastNode = lastQueueNode == NULL ? NULL : HOST_POINTER(lastQueueNode, __FIFOnode, node);
        DEBUG_ASSERT_SILENT(lastNode->begin < lastNode->end);
        --lastNode->end;

        if (lastNode->begin == lastNode->end) {
            deque_popLast(&fifo->bufferQueue);
            __fifoNode_releaseNode(lastNode);
        }
    } else {
        n = algorithms_umin64(n, fifo->bufferSize);
        Size remainingN = n;
        while (remainingN > 0) {
            DequeNode* lastQueueNode = deque_peekLast(&fifo->bufferQueue);
            __FIFOnode* lastNode = lastQueueNode == NULL ? NULL : HOST_POINTER(lastQueueNode, __FIFOnode, node);
            DEBUG_ASSERT_SILENT(lastNode->begin < lastNode->end);
            Size eraseN = algorithms_umin64(lastNode->end - lastNode->begin, remainingN);
            lastNode->end -= eraseN;
    
            if (lastNode->begin == lastNode->end) {
                deque_popLast(&fifo->bufferQueue);
                __fifoNode_releaseNode(lastNode);
            }

            remainingN -= eraseN;
        }
    }


    fifo->bufferSize -= n;

    semaphore_up(&fifo->queueLock);

    return n;
}

Size fifo_read(FIFO* fifo, void* buffer, Size n) {
    if (fifo->bufferSize == 0) {
        return 0;
    }

    semaphore_down(&fifo->queueLock);

    if (n == 1) {
        DequeNode* firstQueueNode = deque_peekFirst(&fifo->bufferQueue);
        __FIFOnode* firstNode = firstQueueNode == NULL ? NULL : HOST_POINTER(firstQueueNode, __FIFOnode, node);
        DEBUG_ASSERT_SILENT(firstNode->begin < firstNode->end);

        *(Uint8*)buffer = firstNode->buffer[firstNode->begin++];

        if (firstNode->begin == __FIFO_NODE_BUFFER_SIZE) {
            deque_popFirst(&fifo->bufferQueue);
            __fifoNode_releaseNode(firstNode);
        }
    } else {
        void* currentBuffer = buffer;
        n = algorithms_umin64(n, fifo->bufferSize);
        Size remainingN = n;
        while (remainingN > 0) {
            DequeNode* firstQueueNode = deque_peekFirst(&fifo->bufferQueue);
            __FIFOnode* firstNode = firstQueueNode == NULL ? NULL : HOST_POINTER(firstQueueNode, __FIFOnode, node);
            DEBUG_ASSERT_SILENT(firstNode->begin < firstNode->end);
            Size readN = algorithms_umin64(firstNode->end - firstNode->begin, remainingN);
    
            memory_memcpy(currentBuffer, firstNode->buffer + firstNode->begin, readN);
            firstNode->begin += readN;
    
            if (firstNode->begin == __FIFO_NODE_BUFFER_SIZE) {
                deque_popFirst(&fifo->bufferQueue);
                __fifoNode_releaseNode(firstNode);
            }

            remainingN -= readN;
            currentBuffer += readN;
        }
    }
    
    fifo->bufferSize -= n;

    semaphore_up(&fifo->queueLock);

    return n;
}

Size fifo_readTill(FIFO* fifo, void* buffer, Size n, Uint8 till) {
    if (fifo->bufferSize == 0) {
        return 0;
    }

    semaphore_down(&fifo->queueLock);

    if (n == 1) {   //No need for check
        DequeNode* firstQueueNode = deque_peekFirst(&fifo->bufferQueue);
        __FIFOnode* firstNode = firstQueueNode == NULL ? NULL : HOST_POINTER(firstQueueNode, __FIFOnode, node);
        DEBUG_ASSERT_SILENT(firstNode->begin < firstNode->end);

        *(Uint8*)buffer = firstNode->buffer[firstNode->begin++];

        if (firstNode->begin == __FIFO_NODE_BUFFER_SIZE) {
            deque_popFirst(&fifo->bufferQueue);
            __fifoNode_releaseNode(firstNode);
        }
    } else {
        void* currentBuffer = buffer;
        n = algorithms_umin64(n, fifo->bufferSize);
        Size remainingN = n;
        while (remainingN > 0) {
            DequeNode* firstQueueNode = deque_peekFirst(&fifo->bufferQueue);
            __FIFOnode* firstNode = firstQueueNode == NULL ? NULL : HOST_POINTER(firstQueueNode, __FIFOnode, node);
            void* p = paging_fastTranslate(mm->extendedTable, firstNode);
            DEBUG_ASSERT_SILENT(firstNode->begin < firstNode->end);
            Size readN = algorithms_umin64(firstNode->end - firstNode->begin, remainingN);

            Size realReadN = 1;
            for (int i = firstNode->begin; realReadN < readN; ++realReadN, ++i) {
                if (firstNode->buffer[i] == till) {
                    break;
                }
            }
    
            memory_memcpy(currentBuffer, firstNode->buffer + firstNode->begin, realReadN);
            firstNode->begin += realReadN;
    
            if (firstNode->begin == __FIFO_NODE_BUFFER_SIZE) {
                deque_popFirst(&fifo->bufferQueue);
                __fifoNode_releaseNode(firstNode);
            }

            if (realReadN != readN) {
                break;
            }

            remainingN -= realReadN;
            currentBuffer += realReadN;
        }

        n -= remainingN;    //Trick to get actual read n
    }
    
    fifo->bufferSize -= n;

    semaphore_up(&fifo->queueLock);

    return n;
}

static __FIFOnode* __fifoNode_createNode() {
    __FIFOnode* ret = PAGING_CONVERT_KERNEL_MEMORY_P2V(mm_allocateFrames(sizeof(__FIFOnode) / PAGE_SIZE));  //TODO: Make it more formal
    void* p = paging_fastTranslate(mm->extendedTable, ret);
    if (ret == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    dequeNode_initStruct(&ret->node);
    ret->begin = ret->end = 0;
    memory_memset(ret->buffer, 0, __FIFO_NODE_BUFFER_SIZE);
    
    return ret;
    
    ERROR_FINAL_BEGIN(0);
}

static void __fifoNode_releaseNode(__FIFOnode* node) {
    memory_memset(node, 0, PAGE_SIZE);
    mm_freeFrames(PAGING_CONVERT_KERNEL_MEMORY_V2P(node), sizeof(__FIFOnode) / PAGE_SIZE);
}