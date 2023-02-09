#include<devices/terminal/inputBuffer.h>

#include<memory/memory.h>
#include<memory/pageAlloc.h>
#include<memory/physicalPages.h>
#include<structs/queue.h>
#include<system/pageTable.h>

#define __NODE_BUFFER_SIZE  (PAGE_SIZE - sizeof(QueueNode) - 2 * sizeof(uint16_t) - 16) //-16 for kMalloc padding

typedef struct {
    QueueNode node;
    uint16_t begin, end;
    char buffer[__NODE_BUFFER_SIZE];
} __InputBufferNode;

static __InputBufferNode* __allocateNode();

static void __releaseNode(__InputBufferNode* node);

void initInputBuffer(InputBuffer* buffer) {
    initQueue(&buffer->bufferQueue);
    buffer->bufferSize = 0;
    initSemaphore(&buffer->queueLock, 1);
    initSemaphore(&buffer->lineNumSema, 0);
}

void inputChar(InputBuffer* buffer, char ch) {
    down(&buffer->queueLock);
    if (ch == '\b') {
        __InputBufferNode* lastNode = HOST_POINTER(buffer->bufferQueue.qTail, __InputBufferNode, node);
        bool deleted = false;
        if (deleted = (buffer->bufferSize != 0 && (lastNode->buffer[lastNode->end - 1] != '\n'))) {
            lastNode->buffer[--lastNode->end] = '\0';
            --buffer->bufferSize;
        }

        if (deleted && buffer->bufferSize == 0) {
            SinglyLinkedListNode* last = &buffer->bufferQueue.q;
            while (last->next != &lastNode->node) {
                last = last->next;
            }
            singlyLinkedListDeleteNext(last);
            buffer->bufferQueue.qTail = last;
            __releaseNode(lastNode);
        }
    } else {
        __InputBufferNode* lastNode = HOST_POINTER(buffer->bufferQueue.qTail, __InputBufferNode, node);
        if (isQueueEmpty(&buffer->bufferQueue) || lastNode->end == __NODE_BUFFER_SIZE) {
            lastNode = __allocateNode();
            queuePush(&buffer->bufferQueue, &lastNode->node);
        }
        lastNode->buffer[lastNode->end++] = ch;
        ++buffer->bufferSize;

        if (ch == '\n') {
            up(&buffer->lineNumSema);
        }
    }

    up(&buffer->queueLock);
}

int bufferGetChar(InputBuffer* buffer) {
    down(&buffer->queueLock);
    down(&buffer->lineNumSema);

    __InputBufferNode* firstNode = HOST_POINTER(queueFront(&buffer->bufferQueue), __InputBufferNode, node);
    int ret = (int)firstNode->buffer[firstNode->begin++];
    if (firstNode->begin == firstNode->end) {
        queuePop(&buffer->bufferQueue);
        __releaseNode(firstNode);
    }
    --buffer->bufferSize;

    if (ret != '\n') {
        up(&buffer->lineNumSema);
    }

    up(&buffer->queueLock);
    return ret;
}

int bufferGetLine(InputBuffer* buffer, char* writeTo) {
    down(&buffer->lineNumSema);
    down(&buffer->queueLock);

    int ret = 0;
    __InputBufferNode* node = HOST_POINTER(queueFront(&buffer->bufferQueue), __InputBufferNode, node);
    while (!isQueueEmpty(&buffer->bufferQueue) && node->buffer[node->begin] != '\n') {
        writeTo[ret++] = node->buffer[node->begin++];
        if (node->begin == node->end) {
            queuePop(&buffer->bufferQueue);
            __releaseNode(node);
            node = HOST_POINTER(queueFront(&buffer->bufferQueue), __InputBufferNode, node);
        }
    }
    ++node->begin;
    writeTo[ret] = '\0';

    up(&buffer->queueLock);
    return ret;
}

static __InputBufferNode* __allocateNode() {
    __InputBufferNode* ret = pageAlloc(1, PHYSICAL_PAGE_TYPE_PUBLIC);

    initQueueNode(&ret->node);
    ret->begin = ret->end = 0;
    memset(ret->buffer, '\0', __NODE_BUFFER_SIZE);
    
    return ret;
}

static void __releaseNode(__InputBufferNode* node) {
    memset(node, 0, PAGE_SIZE);
    pageFree(node, 1);
}