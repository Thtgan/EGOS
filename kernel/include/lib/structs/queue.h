#if !defined(__QUEUE_H)
#define __QUEUE_H

#include<structs/singlyLinkedList.h>
#include<kit/types.h>

typedef struct {
    SinglyLinkedList q;
    SinglyLinkedListNode* qTail;
} Queue;

typedef SinglyLinkedListNode QueueNode;

static inline void initQueue(Queue* q) {
    initSinglyLinkedList(&q->q);
    q->qTail = &q->q;
}

static inline void initQueueNode(QueueNode* node) {
    initSinglyLinkedListNode(node);
}

static inline bool isQueueEmpty(Queue* q) {
    return isSinglyListEmpty(&q->q);
}

static inline void queuePush(Queue* q, QueueNode* newNode) {
    singlyLinkedListInsertNext(q->qTail, newNode);
    q->qTail = newNode;
}

static inline void queuePop(Queue* q) {
    singlyLinkedListDeleteNext(&q->q);
    if (isSinglyListEmpty(&q->q)) {
        q->qTail = &q->q;
    }
}

static inline QueueNode* queueFront(Queue* q) {
    return isSinglyListEmpty(&q->q) ? NULL : singlyLinkedListGetNext(&q->q);
}

#endif // __QUEUE_H
