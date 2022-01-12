#if !defined(__LINKED_LIST)
#define __LINKED_LIST

#include<stdbool.h>
#include<stddef.h>

struct __LinkedListNode {
    struct __LinkedListNode* next;
    struct __LinkedListNode* prev;
};

#if !defined(HOST_POINTER)
//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))
#endif

//Node of the double linked list
typedef struct __LinkedListNode LinkedListNode;
//Header of the double linked list, which is actually a double linked list node
typedef struct __LinkedListNode LinkedList;

/**
 * @brief Initialize a double linked list
 * 
 * @param list Header of the double linked list
 */
void initLinkedList(LinkedList* list);

/**
 * @brief Initialize a double linked list
 * 
 * @param node Node of the double linked list
 */
void initLinkedListNode(LinkedListNode* node);

/**
 * @brief Check is the double linked list empty
 * 
 * @param list Header of the double linked list
 * @return bool is the double linked list empty
 */
bool isListEmpty(LinkedListNode* list);

/**
 * @brief Get the next node of a double linked list node
 * 
 * @param node The double linked list node, could be list header
 * @return LinkedListNode* Next node
 */
LinkedListNode* linkedListGetNext(LinkedListNode* node);

/**
 * @brief Get the previous node of a double linked list node
 * 
 * @param node The double linked list node, could be list header
 * @return LinkedListNode* Previous node
 */
LinkedListNode* linkedListGetPrev(LinkedListNode* node);

/**
 * @brief Insert a node to the next position of a double linked list node
 * 
 * @param node New node will be inserted to the next position of this double linked list node
 * @param newNode New double linked list to insert
 */
void linkedListInsertBack(LinkedListNode* node, LinkedListNode* newNode);

/**
 * @brief Insert a node to the previous position of a double linked list node
 * 
 * @param node New node will be inserted to the previous position of this double linked list node
 * @param newNode New double linked list to insert
 */
void linkedListInsertFront(LinkedListNode* node, LinkedListNode* newNode);

/**
 * @brief Remove this node from the double linked list node
 * 
 * @param node Node to remove
 */
void linkedListDelete(LinkedListNode* node);

#endif // __LINKED_LIST
