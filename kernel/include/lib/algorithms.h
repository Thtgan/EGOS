#if !defined(__LIB_ALGORITHMS_H)
#define __LIB_ALGORITHMS_H

#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>

static inline Int8 algorithms_max8(Int8 a, Int8 b) {
    return a > b ? a : b;
}

static inline Uint8 algorithms_umax8(Uint8 a, Uint8 b) {
    return a > b ? a : b;
}

static inline Int16 algorithms_max16(Int16 a, Int16 b) {
    return a > b ? a : b;
}

static inline Uint16 algorithms_umax16(Uint16 a, Uint16 b) {
    return a > b ? a : b;
}

static inline Int32 algorithms_max32(Int32 a, Int32 b) {
    return a > b ? a : b;
}

static inline Uint32 algorithms_umax32(Uint32 a, Uint32 b) {
    return a > b ? a : b;
}

static inline Int64 algorithms_max64(Int64 a, Int64 b) {
    return a > b ? a : b;
}

static inline Uint64 algorithms_umax64(Uint64 a, Uint64 b) {
    return a > b ? a : b;
}

static inline Int8 algorithms_min8(Int8 a, Int8 b) {
    return a < b ? a : b;
}

static inline Uint8 algorithms_umin8(Uint8 a, Uint8 b) {
    return a < b ? a : b;
}

static inline Int16 algorithms_min16(Int16 a, Int16 b) {
    return a < b ? a : b;
}

static inline Uint16 algorithms_umin16(Uint16 a, Uint16 b) {
    return a < b ? a : b;
}

static inline Int32 algorithms_min32(Int32 a, Int32 b) {
    return a < b ? a : b;
}

static inline Uint32 algorithms_umin32(Uint32 a, Uint32 b) {
    return a < b ? a : b;
}

static inline Int64 algorithms_min64(Int64 a, Int64 b) {
    return a < b ? a : b;
}

static inline Uint64 algorithms_umin64(Uint64 a, Uint64 b) {
    return a < b ? a : b;
}

static inline Uint8 algorithms_abs8(Int8 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 algorithms_abs16(Int16 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 algorithms_abs32(Int32 val) {
    return val < 0 ? -val : val;
}

static inline Uint8 algorithms_abs64(Int64 val) {
    return val < 0 ? -val : val;
}

static inline void algorithms_swap8(Uint8* a, Uint8* b) {
    Uint8 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void algorithms_swap16(Uint16* a, Uint16* b) {
    Uint16 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void algorithms_swap32(Uint32* a, Uint32* b) {
    Uint32 tmp = *a;
    *a = *b, *b = tmp;
}

static inline void algorithms_swap64(Uint64* a, Uint64* b) {
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
void algorithms_linkedList_mergeSort(LinkedList* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode));

/**
 * @brief Sort the singly linked list using merge sort
 * 
 * @param list Singly linked list
 * @param len Length of the list
 * @param comparator Comparator applied to compare the elements in the list
 */
void algorithms_singlyLinkedList_mergeSort(SinglyLinkedList* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

#endif // __LIB_ALGORITHMS_H
