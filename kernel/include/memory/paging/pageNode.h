#if !defined(__PAGING_NODE_H)
#define __PAGING_NODE_H

#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

/**
 * A serial of continued node could form a node in convenience to page allocation
 * It is useful to locate a free page, but there is trouble when releasing it, so it is necessary to use
 * other struct to assist it determining a page is free or not(is not reliable to determine a page is free or not from the content of the page) 
 * 
 * +-----------------------+
 * |                       |
 * V                       |
 * +---------------------+ |
 * |      Page Head      | |
 * +---------------------+ |
 * |                     | |
 * |         ...         | |
 * |                     | |
 * +---------------------+ |
 *                         |
 *                         |
 *       prevNode----------+
 *           ^
 *           |
 * +-base<-+ |
 * |       | |
 * V       | |
 * +---------------------+
 * |      Page Head      |-+
 * +---------------------+ |
 * | |                   | |
 * | V       ...         | |
 * | Length              | |
 * +---------------------+ |
 *                         |
 * +-------nextNode<-------+
 * V
 * +---------------------+
 * |      Page Head      |
 * ...
 * /

/**
 * @brief This struct is used at the beginning of the continued pages to form a node.
 */
struct PageHead {
    struct PageHead* nextNode;
    struct PageHead* prevNode;
    void* base;
    size_t length;          //Length of the page node(in size of num of pages)
};

typedef struct PageHead PageNode;

/**
 * @brief Initialize a page node
 * 
 * @param nodeBegin     Beginning of a serial of cntinued pages
 * @param nodeLength    Length of continued pages (In number of pages)
 */
PageNode* initPageNode(void* nodeBegin, size_t nodeLength);

PageNode* initPageNodeList(PageNode* listHead, void* nodeBegin, size_t nodeLength);

/**
 * @brief Get node's next page node
 * 
 * @param node Page node
 * @return PageNode* Next page node, NULL if there is no next page node
 */
PageNode* getNextPageNode(PageNode* node);

/**
 * @brief Set node's next page node
 * 
 * @param node Page node
 * @param nextNode Next page node
 */
void setNextPageNode(PageNode* node, PageNode* nextNode);

/**
 * @brief Get node's previous page node
 * 
 * @param node Page node
 * @return PageNode* Previous page node, NULL if there is no previous page node
 */
PageNode* getPrevPageNode(PageNode* node);

/**
 * @brief Set node's previous node
 * 
 * @param node Page node
 * @param prevNode Previous page node
 */
void setPrevPageNode(PageNode* node, PageNode* prevNode);

/**
 * @brief Get the page node's length (in number of pages)
 * 
 * @param node Page node
 * @return size_t Page node's length (in number of pages)
 */
size_t getPageNodeLength(PageNode* node);

/**
 * @brief Set the page node's length
 * 
 * @param node Page node
 * @return size_t Page node's new length
 */
void setPageNodeLength(PageNode* node, size_t length);

/**
 * @brief Get the page node's base
 * 
 * @param node Page node
 * @return void* The base of the page node
 */
void* getPageNodeBase(PageNode* node);


//================================================================================


/**
 * @brief Remove the page node from linked list
 * 
 * @param node Page node to remove
 */
void removePageNode(PageNode* node);

/**
 * @brief Insert a page node to another node's front
 * 
 * @param node The position to insert
 * @param newNode Page node to insert
 */
void insertPageNodeFront(PageNode* node, PageNode* newNode);

/**
 * @brief Insert a page node to another node's back
 * 
 * @param node The position to insert
 * @param newNode Page node to insert
 */
void insertPageNodeBack(PageNode* node, PageNode* newNode);


//================================================================================


/**
 * @brief Split a large page node into two smaller page node
 * 
 * @param node Page node
 * @param splitLength Length to split from the node (in number of pages)
 * @return PageNode* The Later page node, NULL if split failed (e.g. splitLength too large), if splitLength fit the length of the node, return the next node.
 */
PageNode* splitPageNode(PageNode* node, size_t splitLength);

/**
 * @brief Cut a large page node as a free memory area from node's front
 * 
 * @param node Page node
 * @param cutLength Length to cut from the node (in number of pages)
 * @return PageNode* The page node after the cut area, NULL if cut failed (e.g. cutLength too large) or there is no next node (When cutLength fits the node length).
 */
PageNode* cutPageNodeFront(PageNode* node, size_t cutLength);

/**
 * @brief Cut a large page node as a free memory area from node's back
 * 
 * @param node Page node
 * @param cutLength Length to cut from the node (in number of pages)
 * @return PageNode* The page node before the cut area, NULL if cut failed (e.g. cutLength too large) or there is no previous node (When cutLength fits the node length).
 */
PageNode* cutPageNodeBack(PageNode* node, size_t cutLength);

/**
 * @brief Combine given page and linked continued next page node into one
 * !Next page node must be not NULL, free, initialized and continued in physical memory space!
 * 
 * -+   +---+             +------>
 *  |   |   |             |
 *  V   |   V             |
 * +------+----------------+-----+
 * | Node | Next page node | ... |
 * +------+----------------+-----+
 * 
 *                 |
 *                 V
 * -+                     +------>
 *  |                     |
 *  V                     |
 * +-----------------------+-----+
 * |     Combined node     | ... |
 * +-----------------------+-----+
 * 
 * @param node Page node
 * @return PageNode* Combined page node
 */
PageNode* combineNextPageNode(PageNode* node);

/**
 * @brief Combine given page and linked continued previous page node into one
 * !Previous page node must be not NULL, free, initialized and continued in physical memory space!
 * 
 * <------+                +-----+  +---
 *        |                |     |  |
 *        |                V     |  V
 * +-----+--------------------+------+
 * | ... | Previous page node | Node |
 * +-----+--------------------+------+
 * 
 *                |
 *                V
 * 
 * <------+                         +---
 *        |                         |
 *        |                         V
 * +-----+---------------------------+
 * | ... |       Combined node       |
 * +-----+---------------------------+
 * 
 * @param node Page node
 * @return PageNode* Combined page node
 */
PageNode* combinePrevPageNode(PageNode* node);


//================================================================================


/**
 * @brief Find a node whose length is equal or greater than size, use first fit
 * 
 * @param list Page node list
 * @param size Size wanted
 * @return PageNode* Page node contains enough pages, NULL if there is no such page
 */
PageNode* firstFitFindPages(PageNode* list, size_t size);

/**
 * @brief Find a node whose length is equal or greater than size, use best fit
 * 
 * @param list Page node list
 * @param size Size wanted
 * @return PageNode* Page node contains enough pages, NULL if there is no such page
 */
PageNode* bestFitFindPages(PageNode* list, size_t size);

/**
 * @brief Find a node whose length is equal or greater than size, use best fit
 * 
 * @param list Page node list
 * @param size Size wanted
 * @return PageNode* Page node contains enough pages, NULL if there is no such page
 */
PageNode* worstFitFindPages(PageNode* list, size_t size);

#endif // __PAGING_NODE_H
