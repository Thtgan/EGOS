#include<memory/paging/pageNode.h>

#include<memory/paging/paging.h>
#include<stddef.h>

PageNode* initPageNode(void* nodeBegin, size_t nodeLength) {
    PageNode* node = (PageNode*)nodeBegin;

    node->base = nodeBegin;
    node->length = nodeLength;
    node->nextNode = node->prevNode = NULL;

    return node;
}

PageNode* initPageNodeList(PageNode* listHead, void* nodeBegin, size_t nodeLength) {
    listHead->base = NULL;
    listHead->length = 0;
    
    PageNode* firstNode = initPageNode(nodeBegin, nodeLength);
    insertPageNodeBack(listHead, firstNode);
}

inline PageNode* getNextPageNode(PageNode* node) {
    return node->nextNode;
}

inline void setNextPageNode(PageNode* node, PageNode* nextNode) {
    node->nextNode = nextNode;
}

inline PageNode* getPrevPageNode(PageNode* node) {
    return node->prevNode;
}

inline void setPrevPageNode(PageNode* node, PageNode* prevNode) {
    node->prevNode = prevNode;
}

inline size_t getPageNodeLength(PageNode* node) {
    return node->length;
}

inline void setPageNodeLength(PageNode* node, size_t length) {
    node->length = length;
}

inline void* getPageNodeBase(PageNode* node) {
    return node->base;
}


//================================================================================


void deletePageNode(PageNode* node) {
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (next != NULL) {
        setPrevPageNode(next, prev);
    }
    if (prev != NULL) {
        setNextPageNode(prev, next);
    }
}

void insertPageNodeFront(PageNode* node, PageNode* newNode) {
    PageNode* prev = node->prevNode;
    if (prev != NULL) {
        setNextPageNode(prev, newNode);
    }

    setPrevPageNode(newNode, prev);
    setNextPageNode(newNode, node);

    setPrevPageNode(node, newNode);
}

void insertPageNodeBack(PageNode* node, PageNode* newNode) {
    PageNode* next = node->nextNode;
    if (next != NULL) {
        setPrevPageNode(next, newNode);
    }

    setPrevPageNode(newNode, node);
    setNextPageNode(newNode, next);

    setNextPageNode(node, newNode);
}


//================================================================================


PageNode* splitPageNode(PageNode* node, size_t splitLength) {
    if (splitLength > node->length) {   //Impossible to split
        return NULL;
    }
    else if (splitLength == node->length) { //splitLength fits page's length
        return getNextPageNode(node);
    }

    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);
    size_t length = getPageNodeLength(node);
    void* newNodeBegin = (void*)node + (splitLength << PAGE_SIZE_BIT);

    PageNode* ret = initPageNode(newNodeBegin, length - splitLength);
    PageNode* ptr = initPageNode((void*)node, splitLength);

    setNextPageNode(ret, next);
    setPrevPageNode(ret, ptr);

    setNextPageNode(ptr, ret);
    setPrevPageNode(ptr, prev);

    return ret;
}

PageNode* cutPageNodeFront(PageNode* node, size_t cutLength) {
    size_t length = getPageNodeLength(node);
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (cutLength > length) { //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        deletePageNode(node);
        return next;
    }

    deletePageNode(node);

    void* newNodeBegin = (void*)node + (cutLength << PAGE_SIZE_BIT);

    PageNode* ret = initPageNode(newNodeBegin, length - cutLength);

    insertPageNodeBack(prev, ret);//Previous node must exists

    return ret;
}

PageNode* cutPageNodeBack(PageNode* node, size_t cutLength) {
    size_t length = getPageNodeLength(node);
    PageNode* prev = getPrevPageNode(node), * next = getNextPageNode(node);

    if (cutLength > length) { //Impossible to cut
        return NULL;
    } 
    else if (cutLength == length) { //cutLength fits node's length
        deletePageNode(node);
        return prev;
    }

    setPageNodeLength(node, length - cutLength);

    return node;
}

PageNode* combineNextPageNode(PageNode* node) {
    PageNode* next = getNextPageNode(node);
    
    size_t length1 = getPageNodeLength(node), length2 = getPageNodeLength(next);

    deletePageNode(next);
    setPageNodeLength(node, length1 + length2);

    return node;
}

PageNode* combinePrevPageNode(PageNode* node) {
    PageNode* prev = getPrevPageNode(node);

    size_t length1 = getPageNodeLength(prev), length2 = getPageNodeLength(node);

    deletePageNode(node);
    setPageNodeLength(prev, length1 + length2);

    return prev;
}


//================================================================================


PageNode* firstFitFindPages(PageNode* list, size_t size) {
    PageNode* node = list;
    while (node != NULL && getPageNodeBase(node) == NULL) { //Find first non-head node
        node = getNextPageNode(node);
    }

    for (; !(node == NULL || getPageNodeLength(node) >= size); node = getNextPageNode(node));

    return node;
}

PageNode* bestFitFindPages(PageNode* list, size_t size) {
    PageNode* node = list;
    while (node != NULL && getPageNodeBase(node) == NULL) { //Find first non-head node
        node = getNextPageNode(node);
    }
    PageNode* ret = NULL;
    
    for (; node != NULL; node = getNextPageNode(node)) {
        if (getPageNodeLength(node) >= size && (ret == NULL || getPageNodeLength(node) < getPageNodeLength(ret))) {
            ret = node;
        }
    }

    return ret;
}

PageNode* worstFitFindPages(PageNode* list, size_t size) {
    PageNode* node = list;
    while (node != NULL && getPageNodeBase(node) == NULL) { //Find first non-head node
        node = getNextPageNode(node);
    }
    PageNode* ret = node;
    
    for (; node != NULL; node = getNextPageNode(node)) {
        if (getPageNodeLength(node) > getPageNodeLength(ret)) {
            ret = node;
        }
    }

    return getPageNodeLength(ret) >= size ? ret : NULL;
}