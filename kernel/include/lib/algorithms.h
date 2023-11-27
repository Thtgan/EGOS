#if !defined(__ALGORITHMS_H)
#define __ALGORITHMS_H

#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>

static inline Int8 max8(Int8 a, Int8 b) {
    return a > b ? a : b;
}

static inline Uint8 umax8(Uint8 a, Uint8 b) {
    return a > b ? a : b;
}

static inline Int16 max16(Int16 a, Int16 b) {
    return a > b ? a : b;
}

static inline Uint16 umax16(Uint16 a, Uint16 b) {
    return a > b ? a : b;
}

static inline Int32 max32(Int32 a, Int32 b) {
    return a > b ? a : b;
}

static inline Uint32 umax32(Uint32 a, Uint32 b) {
    return a > b ? a : b;
}

static inline Int64 max64(Int64 a, Int64 b) {
    return a > b ? a : b;
}

static inline Uint64 umax64(Uint64 a, Uint64 b) {
    return a > b ? a : b;
}

static inline Int8 min8(Int8 a, Int8 b) {
    return a < b ? a : b;
}

static inline Uint8 umin8(Uint8 a, Uint8 b) {
    return a < b ? a : b;
}

static inline Int16 min16(Int16 a, Int16 b) {
    return a < b ? a : b;
}

static inline Uint16 umin16(Uint16 a, Uint16 b) {
    return a < b ? a : b;
}

static inline Int32 min32(Int32 a, Int32 b) {
    return a < b ? a : b;
}

static inline Uint32 umin32(Uint32 a, Uint32 b) {
    return a < b ? a : b;
}

static inline Int64 min64(Int64 a, Int64 b) {
    return a < b ? a : b;
}

static inline Uint64 umin64(Uint64 a, Uint64 b) {
    return a < b ? a : b;
}

static inline Uint8 abs8(Int8 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 abs16(Int16 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 abs32(Int32 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 abs64(Int64 val) {
    return val < 0 ? -val : val;
}

static inline void swap8(Uint8* a, Uint8* b) {
    Uint8 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void swap16(Uint16* a, Uint16* b) {
    Uint16 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void swap32(Uint32* a, Uint32* b) {
    Uint32 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void swap64(Uint64* a, Uint64* b) {
    Uint64 tmp = *a;
    *a = *b, *b = tmp;
}

//Comparator function for sorting
#define COMPARATOR_PTR(__X, __T)   int (*__X)(const __T* x1, const __T* x2)

/**
 * @brief Sort the double linked list using merge sort
 * 
 * @param list Double linked list
 * @param len Length of the list
 * @param comparator Comparator applied to compare the elements in the list
 */
void algo_linkedList_mergeSort(LinkedList* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode));

/**
 * @brief Sort the singly linked list using merge sort
 * 
 * @param list Singly linked list
 * @param len Length of the list
 * @param comparator Comparator applied to compare the elements in the list
 */
void algo_singlyLinkedList_mergeSort(SinglyLinkedList* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

#endif // __ALGORITHMS_H
