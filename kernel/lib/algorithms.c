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
static LinkedListNode* __algorithms_linkedList_doMergeSort(LinkedListNode* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode));

void algorithms_linkedList_mergeSort(LinkedList* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    __algorithms_linkedList_doMergeSort(list->next, len, comparator);
}

static LinkedListNode* __algorithms_linkedList_doMergeSort(LinkedListNode* list, Size len, COMPARATOR_PTR(comparator, LinkedListNode)) {
    if (len == 1) {
        return list; //Just return
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) { //Just swap nodes
            LinkedListNode* prev = list->prev, * node1 = list, * node2 = node1->next, * next = node2->next;

            prev->next = next;
            next->prev = prev;

            //->prev<->node1<->node2<->next<- --> ->prev<->node2<->node1<->next<-
            linkedListNode_insertBack(prev, node1);
            linkedListNode_insertBack(prev, node2);

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

    subList1 = __algorithms_linkedList_doMergeSort(subList1, len1, comparator);
    subList2 = __algorithms_linkedList_doMergeSort(subList2, len2, comparator);

    prev->next = next; //Sort connect the previous and next node of the sub-list
    next->prev = prev;

    LinkedListNode* tail = prev; //The smallest node insert back to
    while (!(len1 == 0 && len2 == 0)) { //Merge
        if (len1 != 0 && len2 != 0) {
            if (comparator(subList1, subList2) < 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListNode_getNext(subList1);
                linkedListNode_insertBack(tail, node);
                tail = node;
                --len1;
            } else {
                LinkedListNode* node = subList2;
                subList2 = linkedListNode_getNext(subList2);
                linkedListNode_insertBack(tail, node);
                tail = node;
                --len2;
            }
        } else if (len1 != 0) {
            while (len1 != 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListNode_getNext(subList1);
                linkedListNode_insertBack(tail, node);
                tail = node;
                --len1;
            }
        } else {
            while (len2 != 0) {
                LinkedListNode* node = subList1;
                subList1 = linkedListNode_getNext(subList1);
                linkedListNode_insertBack(tail, node);
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
static SinglyLinkedListNode* __algorithms_singlyLinkedList_doMergeSort(SinglyLinkedListNode* prev, SinglyLinkedListNode* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode));

void algorithms_singlyLinkedList_mergeSort(SinglyLinkedList* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode)) {
    __algorithms_singlyLinkedList_doMergeSort(list, list->next, len, comparator);
}

static SinglyLinkedListNode* __algorithms_singlyLinkedList_doMergeSort(SinglyLinkedListNode* prev, SinglyLinkedListNode* list, Size len, COMPARATOR_PTR(comparator, SinglyLinkedListNode)) {
    if (len == 1) {
        return list; //Just return
    } else if (len == 2) {
        if (comparator(list, list->next) > 0) { //Just swap nodes
            SinglyLinkedListNode* node1 = list, * node2 = node1->next, * next = node2->next;

            prev->next = next;

            //->prev->node1->node2->next-> --> ->prev->node2->node1->next->
            singlyLinkedList_insertNext(prev, node1);
            singlyLinkedList_insertNext(prev, node2);

            list = node2;
        }
        return list;
    }

    Size len1 = len >> 1, len2 = len - len1;
    SinglyLinkedListNode* subList1 = list, * subPrev = NULL, * subList2 = NULL, * next = NULL;

    subPrev = subList1 = __algorithms_singlyLinkedList_doMergeSort(prev, subList1, len1, comparator);

    for (int i = 1; i < len1; ++i) {
        subPrev = subPrev->next;
    }

    next = subList2 = subPrev->next;
    for (int i = 0; i < len2; ++i) {
        next = next->next;
    }

    subList2 = __algorithms_singlyLinkedList_doMergeSort(subPrev, subList2, len2, comparator);

    prev->next = next; //Short connect the previous and next node of the sub-list

    SinglyLinkedListNode* tail = prev; //The smallest node insert back to
    while (!(len1 == 0 && len2 == 0)) {
        if (len1 != 0 && len2 != 0) {
            if (comparator(subList1, subList2) < 0) {
                SinglyLinkedListNode* node = subList1;
                subList1 = singlyLinkedList_getNext(subList1);
                singlyLinkedList_insertNext(tail, node);
                tail = node;
                --len1;
            } else {
                SinglyLinkedListNode* node = subList2;
                subList2 = singlyLinkedList_getNext(subList2);
                singlyLinkedList_insertNext(tail, node);
                tail = node;
                --len2;
            }
        } else if (len1 != 0) {
            while (len1 != 0) {
                SinglyLinkedListNode* node = subList1;
                subList1 = singlyLinkedList_getNext(subList1);
                singlyLinkedList_insertNext(tail, node);
                tail = node;
                --len1;
            }
        } else {
            while (len2 != 0) {
                SinglyLinkedListNode* node = subList2;
                subList2 = singlyLinkedList_getNext(subList2);
                singlyLinkedList_insertNext(tail, node);
                tail = node;
                --len2;
            }
        }
    }

    return prev->next;
}

static void __algorithms_doQuickSort(Object* arr, Index64 low, Index64 high, QuickSortCompareFunc compare);

static Index64 __algorithms_partition(Object* arr, Index64 low, Index64 high, QuickSortCompareFunc compare);

void algorithms_quickSort(Object* arr, Size n, QuickSortCompareFunc compare) {
    __algorithms_doQuickSort(arr, 0, n, compare);
}

static void __algorithms_doQuickSort(Object* arr, Index64 low, Index64 high, QuickSortCompareFunc compare) {
    if (low >= high) {
        return;
    }

    Index64 pivot = __algorithms_partition(arr, low, high, compare);
    __algorithms_doQuickSort(arr, low, pivot, compare);
    __algorithms_doQuickSort(arr, pivot + 1, high, compare);
}

static Index64 __algorithms_partition(Object* arr, Index64 low, Index64 high, QuickSortCompareFunc compare) {
    Object pivot = arr[low];
    Index64 l = low + 1, r = high - 1;

    while (l <= r) {
        while (l <= r && compare(pivot, arr[l]) >= 0) {
            ++l;
        }

        while (l <= r && compare(pivot, arr[r]) <= 0) {
            --r;
        }

        if (l <= r) {
            algorithms_swap64((Uint64*)&arr[l], (Uint64*)&arr[r]);
        }
    }

    algorithms_swap64((Uint64*)&arr[low], (Uint64*)&arr[r]);

    return r;
}

//TODO: Make a real sqrt
Uint32 algorithms_sqrt(Uint64 n) {
    if (n == 0 || n == 1) {
        return n;
    }

    Uint64 l = 1, r = algorithms_umin64(n, (Uint32)-1);
    while (l < r) {
        Uint64 mid = (l + r) >> 1;
        if (mid * mid >= n) {
            r = mid;
        } else {
            l = mid + 1;
        }
    }

    Uint64 ret = l;
    if (n - (ret - 1) * (ret - 1) < ret * ret - n) {
        --ret;
    }

    return l;
}