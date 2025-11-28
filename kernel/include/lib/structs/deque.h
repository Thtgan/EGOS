#if !defined(__LIB_STRUCTS_DEQUE_H)
#define __LIB_STRUCTS_DEQUE_H

typedef struct Deque Deque;

#include<structs/linkedList.h>

typedef LinkedListNode DequeNode;

#include<kit/types.h>

typedef struct Deque {
    LinkedList q;
} Deque;

static inline void deque_initStruct(Deque* q) {
    linkedList_initStruct(&q->q);
}

static inline void dequeNode_initStruct(DequeNode* node) {
    linkedListNode_initStruct(node);
}

static inline bool deque_isEmpty(Deque* q) {
    return linkedList_isEmpty(&q->q);
}

static inline void deque_pushFirst(Deque* q, DequeNode* newNode) {
    linkedListNode_insertBack(&q->q, newNode);
}

static inline void deque_pushLast(Deque* q, DequeNode* newNode) {
    linkedListNode_insertFront(&q->q, newNode);
}

static inline bool deque_popFirst(Deque* q) {
    if (linkedList_isEmpty(&q->q)) {
        return false;
    }

    DequeNode* firstNode = linkedListNode_getNext(&q->q);
    linkedListNode_delete(firstNode);
    return true;
}

static inline bool deque_popLast(Deque* q) {
    if (linkedList_isEmpty(&q->q)) {
        return false;
    }

    DequeNode* lastNode = linkedListNode_getPrev(&q->q);
    linkedListNode_delete(lastNode);
    return true;
}

static inline DequeNode* deque_peekFirst(Deque* q) {
    return linkedList_isEmpty(&q->q) ? NULL : linkedListNode_getNext(&q->q);
}

static inline DequeNode* deque_peekLast(Deque* q) {
    return linkedList_isEmpty(&q->q) ? NULL : linkedListNode_getPrev(&q->q);
}

#endif // __LIB_STRUCTS_DEQUE_H
