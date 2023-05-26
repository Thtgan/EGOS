#include<memory/pageNode.h>

#include<kit/types.h>
#include<structs/linkedList.h>
#include<system/pageTable.h>

PageNode* initPageNode(void* pageAreaBegin, Size pageAreaLength) {
    pageAreaBegin = pageAreaBegin;
    PageNode* node = (PageNode*)pageAreaBegin;

    node->base = pageAreaBegin;
    node->length = pageAreaLength;

    initLinkedListNode(&node->node);

    return node;
}

PageNode* initPageNodeList(LinkedList* listHead, void* pageAreaBegin, Size pageAreaLength) {
    PageNode* firstNode = initPageNode(pageAreaBegin, pageAreaLength);

    initLinkedList(listHead);
    linkedListInsertBack(listHead, &firstNode->node);

    return firstNode;
}

PageNode* getNextPageNode(PageNode* node) {
    return HOST_POINTER(node->node.next, PageNode, node);
}

void setNextPageNode(PageNode* node, PageNode* nextNode) {
    node->node.next = &nextNode->node;
}

PageNode* getPrevPageNode(PageNode* node) {
    return HOST_POINTER(node->node.prev, PageNode, node);
}

void setPrevPageNode(PageNode* node, PageNode* prevNode) {
    node->node.prev = &prevNode->node;
}

//================================================================================


void removePageNode(PageNode* node) {
    linkedListDelete(&node->node);
}

void insertPageNodeFront(PageNode* node, PageNode* newNode) {
    linkedListInsertFront(&node->node, &newNode->node);
}

void insertPageNodeBack(PageNode* node, PageNode* newNode) {
    linkedListInsertBack(&node->node, &newNode->node);
}


//================================================================================


PageNode* splitPageNode(PageNode* node, Size splitLength) {
    if (splitLength > node->length) {       //Impossible to split
        return NULL;
    }
    else if (splitLength == node->length) { //splitLength fits page's length
        return getNextPageNode(node);
    }

    PageNode* prev = getPrevPageNode(node);
    Size length = node->length;
    void* newNodeBegin = (void*)node + (splitLength << PAGE_SIZE_SHIFT);

    removePageNode(node);

    PageNode* ret = initPageNode(newNodeBegin, length - splitLength);   //Remove original node, and reform two new nodes
    PageNode* ptr = initPageNode((void*)node, splitLength);

    insertPageNodeBack(prev, ptr);
    insertPageNodeBack(ptr, ret);

    return ret;
}

PageNode* cutPageNodeFront(PageNode* node, Size cutLength) {
    Size length = node->length;
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (cutLength > length) {       //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        removePageNode(node);
        return next;
    }

    removePageNode(node);

    void* newNodeBegin = (void*)node + (cutLength << PAGE_SIZE_SHIFT);

    PageNode* ret = initPageNode(newNodeBegin, length - cutLength); //Remove and form new node

    insertPageNodeBack(prev, ret);  //Previous node must exists

    return ret;
}

PageNode* cutPageNodeBack(PageNode* node, Size cutLength) {
    Size length = node->length;
    PageNode* prev = getPrevPageNode(node);

    if (cutLength > length) {       //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        removePageNode(node);
        return prev;
    }

    node->length = length - cutLength;  //Just update the length

    return node;
}

PageNode* combineNextPageNode(PageNode* node) {
    PageNode* next = getNextPageNode(node);
    
    Size length1 = node->length, length2 = next->length;

    removePageNode(next);
    node->length = length1 + length2;   //Remove second node and extend first node

    return node;
}

PageNode* combinePrevPageNode(PageNode* node) {
    PageNode* prev = getPrevPageNode(node);

    Size length1 = prev->length, length2 = node->length;

    removePageNode(node);
    prev->length = length1 + length2;   //Remove second node and extend first node

    return prev;
}


//================================================================================


PageNode* firstFitFindPages(LinkedList* list, Size size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next;

    for (; !(node == list || HOST_POINTER(node, PageNode, node)->length >= size); node = linkedListGetNext(node));

    return HOST_POINTER(node, PageNode, node);
}

PageNode* bestFitFindPages(LinkedList* list, Size size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next, * ret = NULL;
    Size bestLen = 0;
    
    for (; node != list; node = linkedListGetNext(node)) {
        Size len = HOST_POINTER(node, PageNode, node)->length;
        if (len >= size && (ret == NULL || len < bestLen)) {
            ret = node;
            bestLen = len;
        }
    }

    return HOST_POINTER(node, PageNode, node);
}

PageNode* worstFitFindPages(LinkedList* list, Size size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next, * ret = NULL;
    Size worstLen = 0;
    
    for (; node != list; node = linkedListGetNext(node)) {
        Size len = HOST_POINTER(node, PageNode, node)->length;
        if (len > worstLen) {
            ret = node;
            worstLen = len;
        }
    }

    return worstLen >= size ? HOST_POINTER(ret, PageNode, node) : NULL;
}