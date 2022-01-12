#if !defined(__ALGORITHMS_H)
#define __ALGORITHMS_H

#include<lib/structs/linkedList.h>
#include<lib/structs/singlyLinkedList.h>
#include<stdbool.h>
#include<stdint.h>

int8_t max8(int8_t a, int8_t b);
uint8_t umax8(uint8_t a, uint8_t b);

int16_t max16(int16_t a, int16_t b);
uint16_t umax16(uint16_t a, uint16_t b);

int32_t max32(int32_t a, int32_t b);
uint32_t umax32(uint32_t a, uint32_t b);

int64_t max64(int64_t a, int64_t b);
uint64_t umax64(uint64_t a, uint64_t b);

#define COMPARATOR_PTR(__X, __T)   int (*__X)(__T* x1, __T* x2)

void linkedListMergeSort(LinkedList* list, size_t len, COMPARATOR_PTR(comparator, LinkedListNode));

void singlyLinkedListMergeSort(SinglyLinkedList* list, size_t len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

#endif // __ALGORITHMS_H
