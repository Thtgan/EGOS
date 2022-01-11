#if !defined(__LINKED_LIST)
#define __LINKED_LIST

#include<stdbool.h>
#include<stddef.h>

struct __LinkedListNode {
    struct __LinkedListNode* next;
    struct __LinkedListNode* prev;
};

#if !defined(hostPointer)
#define hostPointer(nodePtr, type, member)  ((type*)(((void*)(nodePtr)) - offsetof(type, member)))
#endif

typedef struct __LinkedListNode LinkedListNode;
typedef struct __LinkedListNode LinkedList;

void initLinkedList(LinkedList* list);

void initLinkedListNode(LinkedListNode* node);

bool isListEmpty(LinkedListNode* list);

LinkedListNode* linkedListGetNext(LinkedListNode* node);

LinkedListNode* linkedListGetPrev(LinkedListNode* node);

void linkedListInsertBack(LinkedListNode* node, LinkedListNode* newNode);

void linkedListInsertFront(LinkedListNode* node, LinkedListNode* newNode);

void linkedListDelete(LinkedListNode* node);

#endif // __LINKED_LIST
