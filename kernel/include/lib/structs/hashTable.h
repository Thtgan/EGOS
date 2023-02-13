#if !defined(__HASH_TABLE_H)
#define __HASH_TABLE_H

#include<kit/oop.h>
#include<kit/types.h>
#include<structs/singlyLinkedList.h>

RECURSIVE_REFER_STRUCT(HashTable) {
    size_t size, hashSize;
    SinglyLinkedList* chains;
    size_t (*hashFunc)(HashTable* this, Object key);
};

typedef struct {
    SinglyLinkedListNode node;
    Object key;
} HashChainNode;

#define HASH_FUNC size_t (*hashFunc)(HashTable* this, Object key)

void initHashTable(HashTable* table, size_t hashSize, SinglyLinkedList* chains, HASH_FUNC);

void initHashChainNode(HashChainNode* node);

bool hashTableInsert(HashTable* table, Object key, HashChainNode* node);

HashChainNode* hashTableDelete(HashTable* table, Object key);

HashChainNode* hashTableFind(HashTable* table, Object key);

#endif // __HASH_TABLE_H
