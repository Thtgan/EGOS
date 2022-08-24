#if !defined(__HASH_TABLE_H)
#define __HASH_TABLE_H

#include<kit/oop.h>
#include<kit/types.h>
#include<stddef.h>
#include<structs/singlyLinkedList.h>

RECURSIVE_REFER_STRUCT(HashTable) {
    size_t size, hashSize;
    SinglyLinkedList* table;
    size_t (*hashFunc)(THIS_ARG_APPEND(HashTable, Object key));
};

#define HASH_FUNC size_t (*hashFunc)(THIS_ARG_APPEND(HashTable, Object key))

void initHashTable(HashTable* table, size_t hashSize, HASH_FUNC);

void destroyHashTable(HashTable* table);

bool hashTableInsert(HashTable* table, Object key, Object value);

bool hashTableDelete(HashTable* table, Object key, Object* valueReturn);

bool hashTableFind(HashTable* table, Object key, Object* valueReturn);

#endif // __HASH_TABLE_H
