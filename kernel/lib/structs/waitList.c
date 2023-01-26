#include<structs/waitList.h>

#include<kit/types.h>
#include<structs/singlyLinkedList.h>

void initWaitList(WaitList* list) {
    initSinglyLinkedList(&list->waitList);
    list->tail = NULL;
    list->size = 0;
}

void addWaitListTail(WaitList* list, WaitListNode* node) {
    if (isSinglyListEmpty(&list->waitList)) {
        singlyLinkedListInsertNext(&list->waitList, &node->node);
    } else {
        singlyLinkedListInsertNext(list->tail, &node->node);
    }
    list->tail = &node->node;

    ++list->size;
}

void initWaitListNode(WaitListNode* node, Process* process) {
    initSinglyLinkedListNode(&node->node);
    node->process = process;
}

void removeWaitListHead(WaitList* list) {
    if (isSinglyListEmpty(&list->waitList)) {
        return;
    }

    if (singlyLinkedListGetNext(&list->waitList) == list->tail) {
        list->tail = NULL;
    }

    singlyLinkedListDeleteNext(&list->waitList);
    --list->size;
}

WaitListNode* getWaitListHead(WaitList* list) {
    if (isSinglyListEmpty(&list->waitList)) {
        return NULL;
    }
    return HOST_POINTER(singlyLinkedListGetNext(&list->waitList), WaitListNode, node);
}