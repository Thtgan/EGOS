#include<lib/structs/linkedList.h>

#include<stdbool.h>
#include<stddef.h>

inline void initLinkedList(LinkedList* list) {
    list->prev = list->next = list;
}

inline void initLinkedListNode(LinkedListNode* node) {
    node->prev = node->next = NULL;
}

inline bool isListEmpty(LinkedListNode* list) {
    return list->next == list;
}

inline LinkedListNode* linkedListGetNext(LinkedListNode* node) {
    return node->next;
}

inline LinkedListNode* linkedListGetPrev(LinkedListNode* node) {
    return node->prev;
}

inline void linkedListInsertBack(LinkedListNode* node, LinkedListNode* newNode) {
    LinkedListNode* next = node->next;
    newNode->prev = node, newNode->next = next;
    node->next = next->prev = newNode;
}

inline void linkedListInsertFront(LinkedListNode* node, LinkedListNode* newNode) {
    LinkedListNode* prev = node->prev;
    newNode->prev = prev, newNode->next = node;
    prev->next = node->prev = newNode;
}

inline void linkedListDelete(LinkedListNode* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}