#if !defined(__LIB_STRUCTS_RBTREE_H)
#define __LIB_STRUCTS_RBTREE_H

typedef enum RBtreeColor {
    RB_TREE_COLOR_RED,
    RB_TREE_COLOR_BLACK
} RBtreeColor;

typedef struct RBtreeNode RBtreeNode;
typedef struct RBtree RBtree;

#include<kit/types.h>

typedef struct RBtreeNode {
    RBtreeColor color;
    RBtreeNode* parent;
    RBtreeNode* left;
    RBtreeNode* right;
} RBtreeNode;

typedef struct RBtree {
    RBtreeNode* root;
    int (*cmpFunc)(RBtreeNode* node1, RBtreeNode* node2);
    int (*searchFunc)(RBtreeNode* node, Object val);
    RBtreeNode NIL;
} RBtree;

/**
 * @brief Initialize the RB tree
 * 
 * @param tree RB tree to initialize
 * @param cmpFunc Comparation function between nodes
 * @param searchFunc Comparation function between node and value
 */
void RBtree_initStruct(RBtree* tree, int (*cmpFunc)(RBtreeNode*, RBtreeNode*), int (*searchFunc)(RBtreeNode*, Object));

/**
 * @brief Initialize the RB tree node
 * 
 * @param tree RB tree
 * @param node RB tree to initialize
 */
void RBtreeNode_initStruct(RBtree* tree, RBtreeNode* node);

static inline bool RBtree_isEmpty(RBtree* tree) {
    return tree->root == &tree->NIL;
}

RBtreeNode* RBtree_getFirst(RBtree* tree);

/**
 * @brief Search node with key
 * 
 * @param tree RB tree
 * @param val Can be anything, a integer or a pointer, depending on how is the comparation function designed
 * @return RBtreeNode* Founded node, NULL if node not exist
 */
RBtreeNode* RBtree_search(RBtree* tree, Object val);

/**
 * @brief Insert new node
 * 
 * @param tree RB tree
 * @param newNode New node to insert
 */
RBtreeNode* RBtree_insert(RBtree* tree, RBtreeNode* newNode);

/**
 * @brief Delete node
 * 
 * @param tree RB tree
 * @param val Can be anything, a integer or a pointer, depending on how is the comparation function designed
 * @return RBtreeNode* Deleted node, NULL if node not exist
 */
RBtreeNode* RBtree_delete(RBtree* tree, Object val);

void RBtree_directDelete(RBtree* tree, RBtreeNode* node);

/**
 * @brief Get the predecessor of a node
 * 
 * @param tree RB tree
 * @param node RB tree node
 * @return RBtreeNode* predecessor of the node
 */
RBtreeNode* RBtree_getPredecessor(RBtree* tree, RBtreeNode* node);

/**
 * @brief Get the successor of a node
 * 
 * @param tree RB tree
 * @param node RB tree node
 * @return RBtreeNode* successor of the node
 */
RBtreeNode* RBtree_getSuccessor(RBtree* tree, RBtreeNode* node);

RBtreeNode* RBtree_lowerBound(RBtree* tree, Object val);

RBtreeNode* RBtree_upperBound(RBtree* tree, Object val);

#endif // __LIB_STRUCTS_RBTREE_H
