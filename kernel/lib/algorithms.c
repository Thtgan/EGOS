#include<algorithms.h>

#include<kit/types.h>
#include<structs/linkedList.h>
#include<structs/singlyLinkedList.h>

/**
 * @brief Merge sort the sub-double linked list
 * 
 * @param list Beginning node of the-sub list
 * @param len Length of the sub-list
 * @return LinkedListNode* Beginning node of the sorted sub-list
 */
static LinkedListNode* __linkedListMergeSort(LinkedListNode* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode));

void linkedListMergeSort(LinkedList* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    __linkedListMergeSort(list->next, len, comparator);
}

static LinkedListNode* __linkedListMergeSort(LinkedListNode* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    if (len == 1) {
        return list; //Just return
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) { //Just swap nodes
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

    Size len1 = len >> 1, len2 = len - len1;
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

    prev->next = next; //Sort connect the previous and next node of the sub-list
    next->prev = prev;

    LinkedListNode* tail = prev; //The smallest node insert back to
    while (!(len1 == 0 && len2 == 0)) { //Merge
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

/**
 * @brief Merge sort the sub-singly linked list
 * 
 * @param prev Previouds node of the sub-list's first node
 * @param list Beginning node the sub-list
 * @param len Length of the sub-list
 * @return SinglyLinkedListNode* Beginning node of the sorted sub-list
 */
static SinglyLinkedListNode* __singlyLinkedListMergeSort(SinglyLinkedListNode* prev, SinglyLinkedListNode* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

void singlyLinkedListMergeSort(SinglyLinkedList* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode)) {
    __singlyLinkedListMergeSort(list, list->next, len, comparator);
}

static SinglyLinkedListNode* __singlyLinkedListMergeSort(SinglyLinkedListNode* prev, SinglyLinkedListNode* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode)) {
    if (len == 1) {
        return list; //Just return
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) { //Just swap nodes
            SinglyLinkedListNode* node1 = list, * node2 = node1->next, * next = node2->next;

            prev->next = next;

            //->prev->node1->node2->next-> --> ->prev->node2->node1->next->
            singlyLinkedListInsertNext(prev, node1);
            singlyLinkedListInsertNext(prev, node2);

            list = node2;
        }
        return list;
    }

    Size len1 = len >> 1, len2 = len - len1;
    SinglyLinkedListNode* subList1 = list, * subPrev = NULL, * subList2 = NULL, * next = NULL;

    subPrev = subList1 = __singlyLinkedListMergeSort(prev, subList1, len1, comparator);

    for (int i = 1; i < len1; ++i) {
        subPrev = subPrev->next;
    }

    next = subList2 = subPrev->next;
    for (int i = 0; i < len2; ++i) {
        next = next->next;
    }

    subList2 = __singlyLinkedListMergeSort(subPrev, subList2, len2, comparator);

    prev->next = next; //Short connect the previous and next node of the sub-list

    SinglyLinkedListNode* tail = prev; //The smallest node insert back to
    while (!(len1 == 0 && len2 == 0)) {
        if (len1 != 0 && len2 != 0) {
            if (comparator(subList1, subList2) < 0) {
                SinglyLinkedListNode* node = subList1;
                subList1 = singlyLinkedListGetNext(subList1);
                singlyLinkedListInsertNext(tail, node);
                tail = node;
                --len1;
            } else {
                SinglyLinkedListNode* node = subList2;
                subList2 = singlyLinkedListGetNext(subList2);
                singlyLinkedListInsertNext(tail, node);
                tail = node;
                --len2;
            }
        } else if (len1 != 0) {
            while (len1 != 0) {
                SinglyLinkedListNode* node = subList1;
                subList1 = singlyLinkedListGetNext(subList1);
                singlyLinkedListInsertNext(tail, node);
                tail = node;
                --len1;
            }
        } else {
            while (len2 != 0) {
                SinglyLinkedListNode* node = subList2;
                subList2 = singlyLinkedListGetNext(subList2);
                singlyLinkedListInsertNext(tail, node);
                tail = node;
                --len2;
            }
        }
    }

    return prev->next;
}