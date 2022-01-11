#include<lib/algorithms.h>

#include<lib/linkedList.h>
#include<lib/singleLinkedList.h>
#include<stdbool.h>
#include<stdint.h>

int8_t max8(int8_t a, int8_t b) {
    return a > b ? a : b;
}

uint8_t umax8(uint8_t a, uint8_t b) {
    return a > b ? a : b;
}

int16_t max16(int16_t a, int16_t b) {
    return a > b ? a : b;
}

uint16_t umax16(uint16_t a, uint16_t b) {
    return a > b ? a : b;
}

int32_t max32(int32_t a, int32_t b) {
    return a > b ? a : b;
}

uint32_t umax32(uint32_t a, uint32_t b) {
    return a > b ? a : b;
}

int64_t max64(int64_t a, int64_t b) {
    return a > b ? a : b;
}

uint64_t umax64(uint64_t a, uint64_t b) {
    return a > b ? a : b;
}

static LinkedListNode* __linkedListMergeSort(LinkedListNode* list, size_t len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    if (len == 1) {
        return list;
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) {
            LinkedListNode* prev = list->prev, * node1 = list, * node2 = node1->next, * next = node2->next;

            prev->next = next;
            next->prev = prev;

            //->prev<->node1<->node2<->next<- --> ->prev<->node2<->node1<->next<-
            linkedListInsertBack(prev, node1);
            linkedListInsertBack(prev, node2);

            list = node2;
        }
        return list;
    }

    size_t len1 = len >> 1, len2 = len - len1;
    LinkedListNode* prev = list->prev, * subList1 = list, * subList2 = list, * next = NULL;

    for (int i = 0; i < len1; ++i) {
        subList2 = subList2->next;
    }

    next = subList2;
    for (int i = 0; i < len2; ++i) {
        next = next->next;
    }

    subList1 = __linkedListMergeSort(subList1, len1, comparator);
    subList2 = __linkedListMergeSort(subList2, len2, comparator);

    prev->next = next;
    next->prev = prev;

    LinkedListNode* tail = prev;
    while (!(len1 == 0 && len2 == 0)) {
        if (len1 != 0 && len2 != 0) {
            if (comparator(subList1, subList2) < 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListGetNext(subList1);
                linkedListInsertBack(tail, node);
                tail = node;
                --len1;
            } else {
                LinkedListNode* node = subList2;
                subList2 = linkedListGetNext(subList2);
                linkedListInsertBack(tail, node);
                tail = node;
                --len2;
            }
        } else if (len1 != 0) {
            while (len1 != 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListGetNext(subList1);
                linkedListInsertBack(tail, node);
                tail = node;
                --len1;
            }
        } else {
            while (len2 != 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListGetNext(subList1);
                linkedListInsertBack(tail, node);
                tail = node;
                --len1;
            }
        }
    }

    return prev->next;
}

void linkedListMergeSort(LinkedList* list, size_t len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    __linkedListMergeSort(list->next, len, comparator);
}

static SingleLinkedListNode* __singleLinkedListMergeSort(SingleLinkedListNode* prev, SingleLinkedListNode* list, size_t len, COMPARATOR_PTR(comparator, SingleLinkedListNode)) {
    if (len == 1) {
        return list;
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) {
            SingleLinkedListNode* node1 = list, * node2 = node1->next, * next = node2->next;

            prev->next = next;

            //->prev->node1->node2->next-> --> ->prev->node2->node1->next->
            singleLinkedListInsertBack(prev, node1);
            singleLinkedListInsertBack(prev, node2);

            list = node2;
        }
        return list;
    }

    size_t len1 = len >> 1, len2 = len - len1;
    SingleLinkedListNode* subList1 = list, * subPrev = NULL, * subList2 = NULL, * next = NULL;

    subPrev = subList1 = __singleLinkedListMergeSort(prev, subList1, len1, comparator);

    for (int i = 1; i < len1; ++i) {
        subPrev = subPrev->next;
    }

    next = subList2 = subPrev->next;
    for (int i = 0; i < len2; ++i) {
        next = next->next;
    }

    subList2 = __singleLinkedListMergeSort(subPrev, subList2, len2, comparator);

    prev->next = next;

    SingleLinkedListNode* tail = prev;
    while (!(len1 == 0 && len2 == 0)) {
        if (len1 != 0 && len2 != 0) {
            if (comparator(subList1, subList2) < 0) {
                SingleLinkedListNode* node = subList1;
                subList1 = singleLinkedListGetNext(subList1);
                singleLinkedListInsertBack(tail, node);
                tail = node;
                --len1;
            } else {
                SingleLinkedListNode* node = subList2;
                subList2 = singleLinkedListGetNext(subList2);
                singleLinkedListInsertBack(tail, node);
                tail = node;
                --len2;
            }
        } else if (len1 != 0) {
            while (len1 != 0) {
                SingleLinkedListNode* node = subList1;
                subList1 = singleLinkedListGetNext(subList1);
                singleLinkedListInsertBack(tail, node);
                tail = node;
                --len1;
            }
        } else {
            while (len2 != 0) {
                SingleLinkedListNode* node = subList2;
                subList2 = singleLinkedListGetNext(subList2);
                singleLinkedListInsertBack(tail, node);
                tail = node;
                --len2;
            }
        }
    }

    return prev->next;
}

void singleLinkedListMergeSort(SingleLinkedList* list, size_t len, COMPARATOR_PTR(comparator, SingleLinkedListNode)) {
    __singleLinkedListMergeSort(list, list->next, len, comparator);
}