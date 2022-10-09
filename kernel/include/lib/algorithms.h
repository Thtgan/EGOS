#if !defined(__ALGORITHMS_H)
#define __ALGORITHMS_H

#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>

inline int8_t max8(int8_t a, int8_t b) {
    return a > b ? a : b;
}

inline uint8_t umax8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

inline int16_t max16(int16_t a, int16_t b) {
    return a > b ? a : b;
}

inline uint16_t umax16(uint16_t a, uint16_t b) {
    return a > b ? a : b;
}

inline int32_t max32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

inline uint32_t umax32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

inline int64_t max64(int64_t a, int64_t b) {
    return a > b ? a : b;
}

inline uint64_t umax64(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

inline int8_t min8(int8_t a, int8_t b) {
    return a < b ? a : b;
}

inline uint8_t umin8(uint8_t a, uint8_t b) {
    return a < b ? a : b;
}

inline int16_t min16(int16_t a, int16_t b) {
    return a < b ? a : b;
}

inline uint16_t umin16(uint16_t a, uint16_t b) {
    return a < b ? a : b;
}

inline int32_t min32(int32_t a, int32_t b) {
    return a < b ? a : b;
}

inline uint32_t umin32(uint32_t a, uint32_t b) {
    return a < b ? a : b;
}

inline int64_t min64(int64_t a, int64_t b) {
    return a < b ? a : b;
}

inline uint64_t umin64(uint64_t a, uint64_t b) {
    return a < b ? a : b;
}

inline uint8_t abs8(int8_t val) {
    return val < 0 ? -val : val;
}

inline uint8_t abs16(int16_t val) {
    return val < 0 ? -val : val;
}

inline uint8_t abs32(int32_t val) {
    return val < 0 ? -val : val;
}

inline uint8_t abs64(int64_t val) {
    return val < 0 ? -val : val;
}

inline void swap8(uint8_t* a, uint8_t* b) {
    uint8_t tmp = *a;
    *a = *b, *b = tmp;
}

inline void swap16(uint16_t* a, uint16_t* b) {
    uint16_t tmp = *a;
    *a = *b, *b = tmp;
}

inline void swap32(uint32_t* a, uint32_t* b) {
    uint32_t tmp = *a;
    *a = *b, *b = tmp;
}

inline void swap64(uint64_t* a, uint64_t* b) {
    uint64_t tmp = *a;
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
void linkedListMergeSort(LinkedList* list, size_t len, COMPARATOR_PTR(comparator, LinkedListNode));

/**
 * @brief Sort the singly linked list using merge sort
 * 
 * @param list Singly linked list
 * @param len Length of the list
 * @param comparator Comparator applied to compare the elements in the list
 */
void singlyLinkedListMergeSort(SinglyLinkedList* list, size_t len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

#endif // __ALGORITHMS_H
