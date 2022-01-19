#include<lib/structs/linkedList.h>

//Since all implemented transfer to be inline, these codes are deprecated

// #include<stdbool.h>
// #include<stddef.h>

// void initLinkedList(LinkedList* list) {
//     list->prev = list->next = list;
// }

// void initLinkedListNode(LinkedListNode* node) {
//     node->prev = node->next = NULL;
// }

// bool isListEmpty(LinkedListNode* list) {
//     return list->next == list;
// }

// LinkedListNode* linkedListGetNext(LinkedListNode* node) {
//     return node->next;
// }

// LinkedListNode* linkedListGetPrev(LinkedListNode* node) {
//     return node->prev;
// }

// void linkedListInsertBack(LinkedListNode* node, LinkedListNode* newNode) {
//     LinkedListNode* next = node->next;
//     newNode->prev = node, newNode->next = next;
//     node->next = next->prev = newNode;
// }

// void linkedListInsertFront(LinkedListNode* node, LinkedListNode* newNode) {
//     LinkedListNode* prev = node->prev;
//     newNode->prev = prev, newNode->next = node;
//     prev->next = node->prev = newNode;
// }

// void linkedListDelete(LinkedListNode* node) {
//     node->prev->next = node->next;
//     node->next->prev = node->prev;
// }