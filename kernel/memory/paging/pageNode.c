#include<memory/paging/pageNode.h>

#include<memory/paging/paging.h>
#include<stddef.h>
#include<structs/linkedList.h>

PageNode* initPageNode(void* pageAreaBegin, size_t pageAreaLength) {
    PageNode* node = (PageNode*)pageAreaBegin;

    node->base = pageAreaBegin;
    node->length = pageAreaLength;

    initLinkedListNode(&node->node);

    return node;
}

PageNode* initPageNodeList(LinkedList* listHead, void* pageAreaBegin, size_t pageAreaLength) {
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

size_t getPageNodeLength(PageNode* node) {
    return node->length;
}

void setPageNodeLength(PageNode* node, size_t length) {
    node->length = length;
}

void* getPageNodeBase(PageNode* node) {
    return node->base;
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


PageNode* splitPageNode(PageNode* node, size_t splitLength) {
    if (splitLength > node->length) {       //Impossible to split
        return NULL;
    }
    else if (splitLength == node->length) { //splitLength fits page's length
        return getNextPageNode(node);
    }

    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);
    size_t length = getPageNodeLength(node);
    void* newNodeBegin = (void*)node + (splitLength << PAGE_SIZE_BIT);

    removePageNode(node);

    PageNode* ret = initPageNode(newNodeBegin, length - splitLength);   //Remove original node, and reform two new nodes
    PageNode* ptr = initPageNode((void*)node, splitLength);

    insertPageNodeBack(prev, ptr);
    insertPageNodeBack(ptr, ret);

    return ret;
}

PageNode* cutPageNodeFront(PageNode* node, size_t cutLength) {
    size_t length = getPageNodeLength(node);
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (cutLength > length) {       //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        removePageNode(node);
        return next;
    }

    removePageNode(node);

    void* newNodeBegin = (void*)node + (cutLength << PAGE_SIZE_BIT);

    PageNode* ret = initPageNode(newNodeBegin, length - cutLength); //Remove and form new node

    insertPageNodeBack(prev, ret);  //Previous node must exists

    return ret;
}

PageNode* cutPageNodeBack(PageNode* node, size_t cutLength) {
    size_t length = getPageNodeLength(node);
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (cutLength > length) {       //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        removePageNode(node);
        return prev;
    }

    setPageNodeLength(node, length - cutLength); //Just form modify the length

    return node;
}

PageNode* combineNextPageNode(PageNode* node) {
    PageNode* next = getNextPageNode(node);
    
    size_t length1 = getPageNodeLength(node), length2 = getPageNodeLength(next);

    removePageNode(next);
    setPageNodeLength(node, length1 + length2); //Remove second node and extend first node

    return node;
}

PageNode* combinePrevPageNode(PageNode* node) {
    PageNode* prev = getPrevPageNode(node);

    size_t length1 = getPageNodeLength(prev), length2 = getPageNodeLength(node);

    removePageNode(node);
    setPageNodeLength(prev, length1 + length2); //Remove second node and extend first node

    return prev;
}


//================================================================================


PageNode* firstFitFindPages(LinkedList* list, size_t size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next;

    for (; !(node == list || getPageNodeLength(HOST_POINTER(node, PageNode, node)) >= size); node = linkedListGetNext(node));

    return HOST_POINTER(node, PageNode, node);
}

PageNode* bestFitFindPages(LinkedList* list, size_t size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next, * ret = NULL;
    size_t bestLen = 0;
    
    for (; node != list; node = linkedListGetNext(node)) {
        size_t len = getPageNodeLength(HOST_POINTER(node, PageNode, node));
        if (len >= size && (ret == NULL || len < bestLen)) {
            ret = node;
            bestLen = len;
        }
    }

    return HOST_POINTER(node, PageNode, node);
}

PageNode* worstFitFindPages(LinkedList* list, size_t size) {
    if (isListEmpty(list)) {
        return NULL;
    }

    LinkedListNode* node = list->next, * ret = NULL;
    size_t worstLen = 0;
    
    for (; node != list; node = linkedListGetNext(node)) {
        size_t len = getPageNodeLength(HOST_POINTER(node, PageNode, node));
        if (len > worstLen) {
            ret = node;
            worstLen = len;
        }
    }

    return worstLen >= size ? HOST_POINTER(ret, PageNode, node) : NULL;
}