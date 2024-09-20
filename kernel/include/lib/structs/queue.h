#if !defined(__LIB_STRUCTS_QUEUE_H)
#define __LIB_STRUCTS_QUEUE_H

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
static inline void queue_initStruct(Queue* q) {
    singlyLinkedList_initStruct(&q->q);
    q->qTail = &q->q;
}

/**
 * @brief Initialize queue node
 * 
 * @param node Queue node struct
 */
static inline void queueNode_initStruct(QueueNode* node) {
    singlyLinkedListNode_initStruct(node);
}

/**
 * @brief Is queue empty?
 * 
 * @param q Queue
 * @return bool true if empty
 */
static inline bool queue_isEmpty(Queue* q) {
    return singlyLinkedList_isEmpty(&q->q);
}

/**
 * @brief Push node to tail of queue
 * 
 * @param q Queue
 * @param newNode New node to push 
 */
static inline void queue_push(Queue* q, QueueNode* newNode) {
    singlyLinkedList_insertNext(q->qTail, newNode);
    q->qTail = newNode;
}

/**
 * @brief Pop node from head of queue
 * 
 * @param q Queue
 * @return bool Is pop succeed?
 */
static inline bool queue_pop(Queue* q) {
    if (singlyLinkedList_isEmpty(&q->q)) {
        return false;
    }
    singlyLinkedList_deleteNext(&q->q);
    if (singlyLinkedList_isEmpty(&q->q)) {
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
static inline QueueNode* queue_front(Queue* q) {
    return singlyLinkedList_isEmpty(&q->q) ? NULL : singlyLinkedList_getNext(&q->q);
}

#endif // __LIB_STRUCTS_QUEUE_H
