#if !defined(__SINGLE_LINKED_LIST_H)
#define __SINGLE_LINKED_LIST_H

#include<stdbool.h>
#include<stddef.h>

struct __SinglyLinkedListNode {
    struct __SinglyLinkedListNode* next;
};

#if !defined(HOST_POINTER)
//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))
#endif

/**
 * @brief Node of the singly linked list
 */
typedef struct __SinglyLinkedListNode SinglyLinkedListNode;
/**
 * @brief Header of the singly linked list, which is actually a singly linked list node
 */
typedef struct __SinglyLinkedListNode SinglyLinkedList;

/**
 * @brief Initialize a singly linked list
 * 
 * @param list Header of the singly linked list
 */
void initSinglyLinkedList(SinglyLinkedList* list);

/**
 * @brief Initialize a singly linked list
 * 
 * @param node Node of the singly linked list
 */
void initSinglyLinkedListNode(SinglyLinkedListNode* node);

/**
 * @brief Check is the singly linked list empty
 * 
 * @param list Header of the singly linked list
 * @return bool is the singly linked list empty
 */
bool isSingleListEmpty(SinglyLinkedList* list);

/**
 * @brief Get the next node of a singly linked list node
 * 
 * @param node The singly linked list node, could be list header
 * @return SinglyLinkedListNode* Next node
 */
SinglyLinkedListNode* singleLinkedListGetNext(SinglyLinkedListNode* node);

/**
 * @brief Insert a node to the next position of a singly linked list node
 * 
 * @param node New node will be inserted to the next position of this singly linked list node
 * @param newNode New singly linked list to insert
 */
void singleLinkedListInsertBack(SinglyLinkedList* node, SinglyLinkedListNode* newNode);

/**
 * @brief Remove the next singly linked list node
 * 
 * @param node The node before the node to remove
 */
void singleLinkedListDeleteNext(SinglyLinkedList* node);

#endif // __SINGLE_LINKED_LIST_H
