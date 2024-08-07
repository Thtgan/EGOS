#include<devices/terminal/inputBuffer.h>

#include<debug.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<structs/queue.h>
#include<system/pageTable.h>

#define __INPUT_BUFFER_NODE_BUFFER_SIZE  (PAGE_SIZE - sizeof(QueueNode) - 2 * sizeof(Uint16))

typedef struct {
    QueueNode node;
    Uint16 begin, end;
    char buffer[__INPUT_BUFFER_NODE_BUFFER_SIZE];
} __InputBufferNode;

DEBUG_ASSERT_COMPILE(sizeof(__InputBufferNode) == PAGE_SIZE);

/**
 * @brief Allocate a buffer node
 * 
 * @return __InputBufferNode* Buffer node
 */
static __InputBufferNode* __inputBufferNode_allocateNode();

/**
 * @brief Release a buffer node
 * 
 * @param node Buffer node
 */
static void __inputBufferNode_freeNode(__InputBufferNode* node);

void inputBuffer_initStruct(InputBuffer* buffer) {
    queue_initStruct(&buffer->bufferQueue);
    buffer->bufferSize = 0;
    initSemaphore(&buffer->queueLock, 1);
    initSemaphore(&buffer->lineNumSema, 0);
}

void inputBuffer_inputChar(InputBuffer* buffer, char ch) {
    semaphore_down(&buffer->queueLock);
    if (ch == '\b') {
        __InputBufferNode* lastNode = HOST_POINTER(buffer->bufferQueue.qTail, __InputBufferNode, node);
        bool deleted = false;
        if (deleted = (buffer->bufferSize != 0 && (lastNode->buffer[lastNode->end - 1] != '\n'))) {
            lastNode->buffer[--lastNode->end] = '\0';
            --buffer->bufferSize;
        }

        if (deleted && lastNode->end == 0) {
            SinglyLinkedListNode* last = &buffer->bufferQueue.q;
            while (last->next != &lastNode->node) {
                last = last->next;
            }
            singlyLinkedList_deleteNext(last);
            buffer->bufferQueue.qTail = last;
            __inputBufferNode_freeNode(lastNode);
        }
    } else {
        __InputBufferNode* lastNode = HOST_POINTER(buffer->bufferQueue.qTail, __InputBufferNode, node);
        if (queue_isEmpty(&buffer->bufferQueue) || lastNode->end == __INPUT_BUFFER_NODE_BUFFER_SIZE) {
            lastNode = __inputBufferNode_allocateNode();
            queue_push(&buffer->bufferQueue, &lastNode->node);
        }
        lastNode->buffer[lastNode->end++] = ch;
        ++buffer->bufferSize;

        if (ch == '\n') {
            semaphore_up(&buffer->lineNumSema);
        }
    }

    semaphore_up(&buffer->queueLock);
}

int inputBuffer_getChar(InputBuffer* buffer) {
    semaphore_down(&buffer->queueLock);
    semaphore_down(&buffer->lineNumSema);

    __InputBufferNode* firstNode = HOST_POINTER(queue_front(&buffer->bufferQueue), __InputBufferNode, node);
    int ret = (int)firstNode->buffer[firstNode->begin++];
    if (firstNode->begin == firstNode->end) {
        queue_pop(&buffer->bufferQueue);
        __inputBufferNode_freeNode(firstNode);
    }
    --buffer->bufferSize;

    if (ret != '\n') {
        semaphore_up(&buffer->lineNumSema);
    }

    semaphore_up(&buffer->queueLock);
    return ret;
}

int inputBuffer_getLine(InputBuffer* buffer, char* writeTo) {
    semaphore_down(&buffer->lineNumSema);
    semaphore_down(&buffer->queueLock);

    int ret = 0;
    __InputBufferNode* node = HOST_POINTER(queue_front(&buffer->bufferQueue), __InputBufferNode, node);
    while (!queue_isEmpty(&buffer->bufferQueue) && node->buffer[node->begin] != '\n') {
        writeTo[ret++] = node->buffer[node->begin++];
        if (node->begin == node->end) {
            queue_pop(&buffer->bufferQueue);
            __inputBufferNode_freeNode(node);
            node = HOST_POINTER(queue_front(&buffer->bufferQueue), __InputBufferNode, node);
        }
    }
    ++node->begin;
    writeTo[ret] = '\0';

    semaphore_up(&buffer->queueLock);
    return ret;
}

static __InputBufferNode* __inputBufferNode_allocateNode() {
    __InputBufferNode* ret = paging_convertAddressP2V(memory_allocateFrame(1));

    queueNode_initStruct(&ret->node);
    ret->begin = ret->end = 0;
    memory_memset(ret->buffer, '\0', __INPUT_BUFFER_NODE_BUFFER_SIZE);
    
    return ret;
}

static void __inputBufferNode_freeNode(__InputBufferNode* node) {
    memory_memset(node, 0, PAGE_SIZE);
    memory_freeFrame(paging_convertAddressV2P(node));
}