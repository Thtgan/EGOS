#if !defined(__LINKED_LIST)
#define __LINKED_LIST

#include<stdbool.h>
#include<stddef.h>

struct __LinkedListNode {
    struct __LinkedListNode* next;
    struct __LinkedListNode* prev;
};

#if !defined(HOST_POINTER)
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))
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
