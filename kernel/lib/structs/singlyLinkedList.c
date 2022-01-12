#include<lib/structs/singlyLinkedList.h>

#include<stddef.h>

inline void initSinglyLinkedList(SinglyLinkedList* list) {
    list->next = list;
}

inline void initSinglyLinkedListNode(SinglyLinkedListNode* node) {
    node->next = NULL;
}

inline bool isSingleListEmpty(SinglyLinkedList* list) {
    return list->next == list;
}

inline SinglyLinkedListNode* singleLinkedListGetNext(SinglyLinkedListNode* node) {
    return node->next;
}

inline void singleLinkedListInsertBack(SinglyLinkedList* node, SinglyLinkedListNode* newNode) {
    newNode->next = node->next;
    node->next = newNode;
}

inline void singleLinkedListDeleteNext(SinglyLinkedList* node) {
    node->next = node->next->next;
}