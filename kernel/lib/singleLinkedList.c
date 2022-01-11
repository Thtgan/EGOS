#include<lib/singleLinkedList.h>

#include<stddef.h>

inline void initSingleLinkedList(SingleLinkedList* list) {
    list->next = list;
}

inline void initSingleLinkedListNode(SingleLinkedListNode* node) {
    node->next = NULL;
}

inline bool isSingleListEmpty(SingleLinkedList* list) {
    return list->next == list;
}

inline SingleLinkedListNode* singleLinkedListGetNext(SingleLinkedListNode* node) {
    return node->next;
}

inline void singleLinkedListInsertBack(SingleLinkedList* node, SingleLinkedListNode* newNode) {
    newNode->next = node->next;
    node->next = newNode;
}

inline void singleLinkedListDeleteNext(SingleLinkedList* node) {
    node->next = node->next->next;
}