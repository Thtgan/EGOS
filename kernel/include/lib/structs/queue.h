#if !defined(__QUEUE_H)
#define __QUEUE_H

#include<structs/singlyLinkedList.h>
#include<kit/types.h>

typedef struct {
    SinglyLinkedList q;
    SinglyLinkedListNode* qTail;
} Queue;

typedef SinglyLinkedListNode QueueNode;

/**
 * @brief Initialize queue
 * 
 * @param q Queue struct
 */
static inline void initQueue(Queue* q) {
    initSinglyLinkedList(&q->q);
    q->qTail = &q->q;
}

/**
 * @brief Initialize queue node
 * 
 * @param node Queue node struct
 */
static inline void initQueueNode(QueueNode* node) {
    initSinglyLinkedListNode(node);
}

/**
 * @brief Is queue empty?
 * 
 * @param q Queue
 * @return bool true if empty
 */
static inline bool isQueueEmpty(Queue* q) {
    return isSinglyListEmpty(&q->q);
}

/**
 * @brief Push node to tail of queue
 * 
 * @param q Queue
 * @param newNode New node to push 
 */
static inline void queuePush(Queue* q, QueueNode* newNode) {
    singlyLinkedListInsertNext(q->qTail, newNode);
    q->qTail = newNode;
}

/**
 * @brief Pop node from head of queue
 * 
 * @param q Queue
 * @return bool Is pop succeed?
 */
static inline bool queuePop(Queue* q) {
    if (isSinglyListEmpty(&q->q)) {
        return false;
    }
    singlyLinkedListDeleteNext(&q->q);
    if (isSinglyListEmpty(&q->q)) {
        q->qTail = &q->q;
    }

    return true;
}

/**
 * @brief Get the head node of queue
 * 
 * @param q Queue
 * @return QueueNode* Head node of queue, NULL if queue is empty
 */
static inline QueueNode* queueFront(Queue* q) {
    return isSinglyListEmpty(&q->q) ? NULL : singlyLinkedListGetNext(&q->q);
}

#endif // __QUEUE_H
