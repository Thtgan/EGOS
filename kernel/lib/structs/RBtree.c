#include<structs/RBtree.h>

#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

/**
 * RB tree should obey following rules:
 * 
 * 1. Only Red/Black nodes
 * 2. Root is Black (Optional)
 * 3. NIL leaves are Black
 * 4. No Red child for Red node
 * 5. All nodes to its leaves contains same number of Black nodes
 */

static void __lRotate(RBtree* tree, RBtreeNode* node);
static void __rRotate(RBtree* tree, RBtreeNode* node);
static void __updateParent(RBtree* tree, RBtreeNode* oldChild, RBtreeNode* newChild);
static RBtreeNode* __getSibling(RBtreeNode* node);
static void __insertFix(RBtree* tree, RBtreeNode* newNode);
static void __deleteFix(RBtree* tree, RBtreeNode* newNode);

void initRBtree(RBtree* tree, int (*cmpFunc)(RBtreeNode*, RBtreeNode*), int (*searchFunc)(RBtreeNode*, uint64_t)) {
    RBtreeNode* NIL = &tree->NIL;
    NIL->color = RB_TREE_COLOR_BLACK;
    NIL->left = NIL->right = NIL->parent = NULL;

    tree->cmpFunc = cmpFunc;
    tree->searchFunc = searchFunc;

    tree->root = &tree->NIL;
}

void initRBtreeNode(RBtree* tree, RBtreeNode* node) {
    node->color = RB_TREE_COLOR_RED;
    node->parent = NULL;
    node->left = node->right = &tree->NIL;
}

RBtreeNode* searchNode(RBtree* tree, uint64_t val) {
    int cmp;
    for (RBtreeNode* node = tree->root; node != &tree->NIL;) {
        cmp = tree->searchFunc(node, val);
        if (cmp == 0) {
            return node;
        } else if (cmp > 0) {
            node = node->left;
        } else {
            node = node->right;
        }
    }

    return NULL;
}

bool insertNode(RBtree* tree, RBtreeNode* newNode) {
    RBtreeNode* parent = NULL;

    int cmp;
    for (RBtreeNode* node = tree->root; node != &tree->NIL;) {
        cmp = tree->cmpFunc(newNode, node);

        if (cmp == 0) {
            return false;
        } else {
            parent = node;
            node = cmp < 0 ? node->left : node->right;
        }
    }

    if (parent == NULL) {
        tree->root = newNode;
    } else if (cmp < 0) {
        parent->left = newNode;
    } else {
        parent->right = newNode;
    }

    __insertFix(tree, newNode);

    return true;
}

RBtreeNode* deleteNode(RBtree* tree, uint64_t val) {
    RBtreeNode* found = searchNode(tree, val);

    if (found == NULL) {
        return NULL;
    }

    RBtreeNode* replaceNode;
    RBtreeColor removedColor;
    if (found->left == &tree->NIL) { //If node has no child or has a right child
        replaceNode = found->right; //If no child, this is a NIL
        removedColor = found->color;
        __updateParent(tree, found, replaceNode);
    } else if (found->right == &tree->NIL) { //If node has a left child
        replaceNode = found->left;
        removedColor = found->color;
        __updateParent(tree, found, replaceNode);
    } else { //Node has two children
        RBtreeNode* successor = getSuccessor(tree, found);

        replaceNode = successor->right;
        removedColor = successor->color;
        __updateParent(tree, successor, replaceNode);

        __updateParent(tree, found, successor);

        successor->color = found->color;
        successor->left = found->left;
        successor->left->parent = successor;
        successor->right = found->right;
        successor->right->parent = successor;
    }

    if (removedColor == RB_TREE_COLOR_BLACK) {
        __deleteFix(tree, replaceNode);
    }

    if (replaceNode == &tree->NIL) {
        replaceNode->parent = NULL; //Set back the NIL parent
    }

    found->parent = NULL;
    found->left = found->right = &tree->NIL;
    return found;
}

RBtreeNode* getPredecessor(RBtree* tree, RBtreeNode* node) {
    if (node->left != &tree->NIL) {
        RBtreeNode* ret = node->left;
        while (ret->right != &tree->NIL) {
            ret = ret->right;
        }
        return ret;
    }

    RBtreeNode* parent = node->parent;
    while (parent != NULL && parent->left == node) {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

RBtreeNode* getSuccessor(RBtree* tree, RBtreeNode* node) {
    if (node->right != &tree->NIL) {
        RBtreeNode* ret = node->right;
        while (ret->left != &tree->NIL) {
            ret = ret->left;
        }
        return ret;
    }

    RBtreeNode* parent = node->parent;
    while (parent != NULL && parent->right == node) {
        node = parent;
        parent = node->parent;
    }

    return parent;
}

static void __lRotate(RBtree* tree, RBtreeNode* node) {
    RBtreeNode* parent = node->parent;
    RBtreeNode* right = node->right;

    node->right = right->left;
    if (node->right != &tree->NIL) {
        node->right->parent = node;
    }

    __updateParent(tree, node, right);

    right->left = node;
    node->parent = right;
}

static void __rRotate(RBtree* tree, RBtreeNode* node) {
    RBtreeNode* parent = node->parent;
    RBtreeNode* left = node->left;

    node->left = left->right;
    if (node->left != &tree->NIL) {
        node->left->parent = node;
    }

    __updateParent(tree, node, left);

    left->right = node;
    node->parent = left;
}

static void __updateParent(RBtree* tree, RBtreeNode* oldChild, RBtreeNode* newChild) {
    RBtreeNode* parent = oldChild->parent;
    if (parent == NULL) {
        tree->root = newChild;
    } else if (oldChild == parent->left) {
        parent->left = newChild;
    } else if (oldChild == parent->right) {
        parent->right = newChild;
    }

    if (newChild != NULL) {
        newChild->parent = parent;
    }
}

static RBtreeNode* __getSibling(RBtreeNode* node) {
    RBtreeNode* parent = node->parent;
    return node == parent->right ? parent->left : parent->right;
}

static void __insertFix(RBtree* tree, RBtreeNode* newNode) {
    RBtreeNode* parent = newNode->parent;

    if (parent == NULL) {
        newNode->color = RB_TREE_COLOR_BLACK;
        return;
    }

    //newNode is not root, and is red

    for (; parent != NULL && parent->color == RB_TREE_COLOR_RED; parent = newNode->parent) {
        RBtreeNode* grandparent = parent->parent, * uncle = __getSibling(parent);

        if (uncle->color == RB_TREE_COLOR_RED) {
            parent->color = uncle->color = RB_TREE_COLOR_BLACK;
            grandparent->color = RB_TREE_COLOR_RED;

            newNode = grandparent;
        } else {
            if (parent == grandparent->left) {
                if (newNode == parent->right) {
                    __lRotate(tree, parent);

                    parent = newNode;
                    grandparent = parent->parent;
                }

                parent->color = RB_TREE_COLOR_BLACK;
                grandparent->color = RB_TREE_COLOR_RED;

                __rRotate(tree, grandparent);
            } else {
                if (newNode == parent->left) {
                    __rRotate(tree, parent);

                    parent = newNode;
                    grandparent = parent->parent;
                }

                parent->color = RB_TREE_COLOR_BLACK;
                grandparent->color = RB_TREE_COLOR_RED;

                __lRotate(tree, grandparent);
            }
        }
    }
    tree->root->color = RB_TREE_COLOR_BLACK;
}

static void __deleteFix(RBtree* tree, RBtreeNode* newNode) {
    //We want to fix the tree we cut exactly one black node to maintain rule 5
    //newNode stands for the sub-tree follows rule 5, we want to fix the higher tree
    //However, if newNode is red, we can simply set it to black, so we have 1 black node back (LINK kernel/lib/structs/RBtree.c#RBtree_deletion_end_set_red)
    while (newNode != tree->root && newNode->color == RB_TREE_COLOR_BLACK) {
        RBtreeNode* sibling = __getSibling(newNode);
        RBtreeNode* parent = newNode->parent;

        if (sibling->color == RB_TREE_COLOR_RED) {
            sibling->color = RB_TREE_COLOR_BLACK;
            parent->color = RB_TREE_COLOR_RED;

            if (newNode == parent->left) {
                __lRotate(tree, parent);
            } else {
                __rRotate(tree, parent);
            }

            continue;
        }

        if (sibling->left->color == RB_TREE_COLOR_BLACK && sibling->right->color == RB_TREE_COLOR_BLACK) {
            sibling->color = RB_TREE_COLOR_RED;
            newNode = parent;
        } else {
            if (newNode == parent->left) {
                if (sibling->right->color == RB_TREE_COLOR_BLACK) { //Left child of sibling must be red
                    sibling->color = RB_TREE_COLOR_RED;
                    sibling->left->color = RB_TREE_COLOR_BLACK;

                    __rRotate(tree, sibling);
                    sibling = __getSibling(newNode);
                    parent = newNode->parent;
                }

                //Right child of sibling must be red
                sibling->color = parent->color;
                parent->color = sibling->right->color = RB_TREE_COLOR_BLACK;
                __lRotate(tree, parent);
            } else if (newNode == parent->right) {
                if (sibling->left->color == RB_TREE_COLOR_BLACK) { //Right child of sibling must be red
                    sibling->color = RB_TREE_COLOR_RED;
                    sibling->right->color = RB_TREE_COLOR_BLACK;

                    __lRotate(tree, sibling);
                    sibling = __getSibling(newNode);
                    parent = newNode->parent;
                }

                //Left child of sibling must be red
                sibling->color = parent->color;
                parent->color = sibling->left->color = RB_TREE_COLOR_BLACK;
                __rRotate(tree, parent);
            }

            break;
        }
    }

    //ANCHOR[id=RBtree_deletion_end_set_red]
    newNode->color = RB_TREE_COLOR_BLACK;
}