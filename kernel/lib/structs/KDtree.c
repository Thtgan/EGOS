#include<structs/KDtree.h>

#include<algorithms.h>
#include<kit/types.h>

KDtreeNode* __KDtree_doInsert(KDtree* tree, KDtreeNode* currentNode, KDtreeNode* node, int currentDepth);

KDtreeNode* __KDtree_doDelete(KDtree* tree, KDtreeNode* currentNode, Object key, int currentDepth, KDtreeNode** deletedRet);

KDtreeNode* __KDtree_findSubTreeMin(KDtree* tree, KDtreeNode* currentNode, int round, int currentDepth);

void __KDtree_doNearestNeighbour(KDtree* tree, KDtreeNode* currentNode, Object key, KDtreeNode** closestNode, Uint64* minDistance2, int currentDepth);

void KDtreeNode_initStruct(KDtreeNode* node, Object key) {
    node->left = node->right = NULL;
    node->key = key;
}

void KDtree_initStruct(KDtree* tree, int k, KDtreeCompareFunc compare, KDtreeDistanceFunc distance2) {
    tree->k = k;
    tree->size = 0;
    tree->root = NULL;
    tree->compare = compare;
    tree->distance2 = distance2;
}

Result KDtree_insert(KDtree* tree, KDtreeNode* node) {
    KDtreeNode* newRoot = __KDtree_doInsert(tree, tree->root, node, 0);
    if (newRoot != NULL) {
        tree->root = newRoot;
        ++tree->size;
    }

    return RESULT_SUCCESS;
}

KDtreeNode* KDtree_delete(KDtree* tree, Object key) {
    KDtreeNode* ret = NULL;
    KDtreeNode* newRoot = __KDtree_doDelete(tree, tree->root, key, 0, &ret);
    if (ret != NULL) {
        tree->root = newRoot;
        --tree->size;
    }

    return ret;
}

KDtreeNode* KDtree_search(KDtree* tree, Object key) {
    KDtreeNode* currentNode = tree->root;
    int currentDepth = 0;

    while (currentNode != NULL) {
        if (tree->distance2(key, currentNode->key) == 0) {
            return currentNode;
        }

        if (tree->compare(key, currentNode->key, 0, currentDepth % tree->k) < 0) {
            currentNode = currentNode->left;
        } else {
            currentNode = currentNode->right;
        }

        ++currentDepth;
    }

    return NULL;
}

Result KDtree_nearestNeighbour(KDtree* tree, Object key, Object* closestKeyRet) {
    KDtreeNode* closestNode = NULL;
    Uint64 minDistance2 = (Uint64)-1;
    __KDtree_doNearestNeighbour(tree, tree->root, key, &closestNode, &minDistance2, 0);

    if (closestNode == NULL) {
        return RESULT_FAIL;
    }

    *closestKeyRet = closestNode->key;
    return RESULT_SUCCESS;
}

KDtreeNode* __KDtree_doInsert(KDtree* tree, KDtreeNode* currentNode, KDtreeNode* node, int currentDepth) {
    if (currentNode == NULL) {
        return node;
    }

    if (tree->distance2(node->key, currentNode->key) == 0) {
        return NULL;
    }  

    if (tree->compare(node->key, currentNode->key, 0, currentDepth % tree->k) < 0) {
        currentNode->left = __KDtree_doInsert(tree, currentNode->left, node, currentDepth + 1);
    } else {
        currentNode->right = __KDtree_doInsert(tree, currentNode->right, node, currentDepth + 1);
    }

    return currentNode;
}

KDtreeNode* __KDtree_doDelete(KDtree* tree, KDtreeNode* currentNode, Object key, int currentDepth, KDtreeNode** deletedRet) {
    int round = currentDepth % tree->k;

    KDtreeNode* ret = currentNode;
    if (tree->distance2(currentNode->key, key) == 0) {
        if (currentNode->right != NULL) {
            KDtreeNode* rightMin = __KDtree_findSubTreeMin(tree, currentNode->right, round, currentDepth + 1);
            currentNode->key = rightMin->key;
            currentNode->right = __KDtree_doDelete(tree, currentNode->right, rightMin->key, currentDepth + 1, deletedRet);
        } else if (currentNode->left != NULL) {
            KDtreeNode* leftMin = __KDtree_findSubTreeMin(tree, currentNode->left, round, currentDepth + 1);
            currentNode->key = leftMin->key;
            currentNode->left = __KDtree_doDelete(tree, currentNode->left, leftMin->key, currentDepth + 1, deletedRet);
            algorithms_swap64((Uintptr*)&currentNode->left, (Uintptr*)&currentNode->right);
        } else {
            *deletedRet = currentNode;
            ret = NULL;
        }
    } else {
        if (tree->compare(key, currentNode->key, 0, round) < 0) {
            currentNode->left = __KDtree_doDelete(tree, currentNode->left, key, currentDepth + 1, deletedRet);
        } else {
            currentNode->right = __KDtree_doDelete(tree, currentNode->right, key, currentDepth + 1, deletedRet);
        }
    }

    return ret;
}

KDtreeNode* __KDtree_findSubTreeMin(KDtree* tree, KDtreeNode* currentNode, int round, int currentDepth) {
    if (currentNode == NULL) {
        return NULL;
    }

    KDtreeNode* ret = currentNode;
    if (currentNode->right != NULL && round != currentDepth % tree->k) {
        KDtreeNode* rightMin = __KDtree_findSubTreeMin(tree, currentNode->right, round, currentDepth + 1);
        if (tree->compare(rightMin->key, ret->key, 0, round) < 0) {
            ret = rightMin;
        }
    }

    if (currentNode->left != NULL) {
        KDtreeNode* leftMin = __KDtree_findSubTreeMin(tree, currentNode->left, round, currentDepth + 1);
        if (tree->compare(leftMin->key, ret->key, 0, round) < 0) {
            ret = leftMin;
        }
    }
    
    return ret;
}

void __KDtree_doNearestNeighbour(KDtree* tree, KDtreeNode* currentNode, Object key, KDtreeNode** closestNode, Uint64* minDistance2, int currentDepth) {
    if (currentNode == NULL) {
        return;
    }

    Uint64 distance2 = tree->distance2(key, currentNode->key);
    if (distance2 < *minDistance2) {
        *minDistance2 = distance2;
        *closestNode = currentNode;
    }

    if (tree->compare(key, currentNode->key, 0, currentDepth % tree->k) < 0) {
        __KDtree_doNearestNeighbour(tree, currentNode->left, key, closestNode, minDistance2, currentDepth + 1);
        if (tree->compare(key, currentNode->key, (int)algorithms_sqrt(*minDistance2), currentDepth % tree->k) >= 0) {
            __KDtree_doNearestNeighbour(tree, currentNode->right, key, closestNode, minDistance2, currentDepth + 1);
        }
    } else {
        __KDtree_doNearestNeighbour(tree, currentNode->right, key, closestNode, minDistance2, currentDepth + 1);
        if (tree->compare(key, currentNode->key, -(int)algorithms_sqrt(*minDistance2), currentDepth % tree->k) <= 0) {
            __KDtree_doNearestNeighbour(tree, currentNode->left, key, closestNode, minDistance2, currentDepth + 1);
        }
    }
}