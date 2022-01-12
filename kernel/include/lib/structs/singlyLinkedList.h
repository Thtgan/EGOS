#if !defined(__SINGLE_LINKED_LIST_H)
#define __SINGLE_LINKED_LIST_H

#include<stdbool.h>
#include<stddef.h>

struct __SinglyLinkedListNode {
    struct __SinglyLinkedListNode* next;
};

#if !defined(HOST_POINTER)
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))
#endif

typedef struct __SinglyLinkedListNode SinglyLinkedListNode;
typedef struct __SinglyLinkedListNode SinglyLinkedList;

void initSinglyLinkedList(SinglyLinkedList* list);

void initSinglyLinkedListNode(SinglyLinkedListNode* node);

bool isSingleListEmpty(SinglyLinkedList* list);

SinglyLinkedListNode* singleLinkedListGetNext(SinglyLinkedListNode* node);

void singleLinkedListInsertBack(SinglyLinkedList* node, SinglyLinkedListNode* newNode);

void singleLinkedListDeleteNext(SinglyLinkedList* node);

#endif // __SINGLE_LINKED_LIST_H
