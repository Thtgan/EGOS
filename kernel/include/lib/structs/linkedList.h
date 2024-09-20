#if !defined(__LIB_STRUCTS_LINKEDLIST_H)
#define __LIB_STRUCTS_LINKEDLIST_H

#include<kit/types.h>

struct __LinkedListNode {
    struct __LinkedListNode* next;
    struct __LinkedListNode* prev;
};

//Node of the double linked list
typedef struct __LinkedListNode LinkedListNode;
//Header of the double linked list, which is actually a double linked list node
typedef struct __LinkedListNode LinkedList;

/**
 * @brief Initialize a double linked list
 * 
 * @param list Header of the double linked list
 */
static inline void linkedList_initStruct(LinkedList* list) {
    list->prev = list->next = list;
}

/**
 * @brief Initialize a double linked list
 * 
 * @param node Node of the double linked list
 */
static inline void linkedListNode_initStruct(LinkedListNode* node) {
    node->prev = node->next = NULL;
}

/**
 * @brief Check is the double linked list empty
 * 
 * @param list Header of the double linked list
 * @return bool is the double linked list empty
 */
static inline bool linkedList_isEmpty(LinkedListNode* list) {
    return list->next == list;
}

/**
 * @brief Get the next node of a double linked list node
 * 
 * @param node The double linked list node, could be list header
 * @return LinkedListNode* Next node
 */
static inline LinkedListNode* linkedListNode_getNext(LinkedListNode* node) {
    return node->next;
}

/**
 * @brief Get the previous node of a double linked list node
 * 
 * @param node The double linked list node, could be list header
 * @return LinkedListNode* Previous node
 */
static inline LinkedListNode* linkedListNode_getPrev(LinkedListNode* node) {
    return node->prev;
}

/**
 * @brief Insert a node to the next position of a double linked list node
 * 
 * @param node New node will be inserted to the next position of this double linked list node
 * @param newNode New double linked list to insert
 */
static inline void linkedListNode_insertBack(LinkedListNode* node, LinkedListNode* newNode) {
    LinkedListNode* next = node->next;
    newNode->prev = node, newNode->next = next;
    node->next = next->prev = newNode;
}

/**
 * @brief Insert a node to the previous position of a double linked list node
 * 
 * @param node New node will be inserted to the previous position of this double linked list node
 * @param newNode New double linked list to insert
 */
static inline void linkedListNode_insertFront(LinkedListNode* node, LinkedListNode* newNode) {
    LinkedListNode* prev = node->prev;
    newNode->prev = prev, newNode->next = node;
    prev->next = node->prev = newNode;
}

/**
 * @brief Remove this node from the double linked list node
 * 
 * @param node Node to remove
 */
static inline void linkedListNode_delete(LinkedListNode* node) {
    node->prev->next = node->next;
    node->next->prev = node->prev;
}

#endif // __LIB_STRUCTS_LINKEDLIST_H
