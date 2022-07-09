#if !defined(__RB_TREE_H)
#define __RB_TREE_H

#include<stdbool.h>
#include<stdint.h>

typedef struct _RBtreeNode RBtreeNode;

#if !defined(HOST_POINTER)
//Get the host pointer containing the node
#define HOST_POINTER(__NODE_PTR, __TYPE, __MEMBER)  ((__TYPE*)(((void*)(__NODE_PTR)) - offsetof(__TYPE, __MEMBER)))
#endif

typedef enum {
    RB_TREE_COLOR_RED,
    RB_TREE_COLOR_BLACK
} RBtreeColor;

struct _RBtreeNode {
    RBtreeColor color;
    RBtreeNode* parent;
    RBtreeNode* left;
    RBtreeNode* right;
};

typedef struct {
    RBtreeNode* root;
    int (*cmpFunc)(RBtreeNode* node1, RBtreeNode* node2);
    int (*searchFunc)(RBtreeNode* node, uint64_t val);
    RBtreeNode NIL;
} RBtree;

void initRBtree(RBtree* tree, int (*cmpFunc)(RBtreeNode*, RBtreeNode*), int (*searchFunc)(RBtreeNode*, uint64_t));

void initRBtreeNode(RBtree* tree, RBtreeNode* node);

RBtreeNode* searchNode(RBtree* tree, uint64_t val);

bool insertNode(RBtree* tree, RBtreeNode* newNode);

RBtreeNode* deleteNode(RBtree* tree, uint64_t val);

RBtreeNode* getPredecessor(RBtree* tree, RBtreeNode* node);

RBtreeNode* getSuccessor(RBtree* tree, RBtreeNode* node);

#endif // __RB_TREE_H
