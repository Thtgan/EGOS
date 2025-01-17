#if !defined(__LIB_STRUCTS_KDTREE_H)
#define __LIB_STRUCTS_KDTREE_H

typedef struct KDtreeNode KDtreeNode;
typedef struct KDtree KDtree;

#include<kit/types.h>
#include<result.h>

typedef int (*KDtreeCompareFunc)(Object o1, Object o2, int offset, int round);
typedef Uint64 (*KDtreeDistanceFunc)(Object o1, Object o2);

typedef struct KDtreeNode {
    Object key;
    KDtreeNode* left, *right;
} KDtreeNode;

void KDtreeNode_initStruct(KDtreeNode* node, Object key);

typedef struct KDtree {
    int k;
    Size size;
    KDtreeNode* root;
    KDtreeCompareFunc compare;
    KDtreeDistanceFunc distance2;
} KDtree;

void KDtree_initStruct(KDtree* tree, int k, KDtreeCompareFunc compare, KDtreeDistanceFunc distance2);

Result* KDtree_insert(KDtree* tree, KDtreeNode* node);

KDtreeNode* KDtree_delete(KDtree* tree, Object key);

KDtreeNode* KDtree_search(KDtree* tree, Object key);

Result* KDtree_nearestNeighbour(KDtree* tree, Object key, Object* closestKeyRet);

#endif // __LIB_STRUCTS_KDTREE_H
