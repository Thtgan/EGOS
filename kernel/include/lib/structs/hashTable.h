#if !defined(__LIB_STRUCTS_HASHTABLE_H)
#define __LIB_STRUCTS_HASHTABLE_H

typedef struct HashTable HashTable;
typedef struct HashChainNode HashChainNode;

#include<kit/oop.h>
#include<kit/types.h>
#include<structs/singlyLinkedList.h>

typedef Size (*HashTableHashFunc)(HashTable* this, Object key);

typedef struct HashTable {
    Size size, bucket;
    SinglyLinkedList* chains;
    HashTableHashFunc hashFunc;
} HashTable;

typedef struct HashChainNode {
    SinglyLinkedListNode node;
    Object key;
} HashChainNode;

Size hashTable_defaultHashFunc(HashTable* this, Object key);

/**
 * @brief Initialize hash table
 * 
 * @param table Hash table struct
 * @param bucket Num of hash chains
 * @param chains Hash chain list
 * @param hashFunc Hash function to apply
 */
void hashTable_initStruct(HashTable* table, Size bucket, SinglyLinkedList* chains, HashTableHashFunc hashFunc);

/**
 * @brief Initialize hash chain node
 * 
 * @param node Hash chain node
 */
void hashChainNode_initStruct(HashChainNode* node);

/**
 * @brief Insert value to hash table
 * 
 * @param table Hash table
 * @param key Key of value
 * @param node Value's hash chain node to insert
 */
void hashTable_insert(HashTable* table, Object key, HashChainNode* node);

/**
 * @brief Delete a value corresponded to key
 * 
 * @param table Hash table
 * @param key Key of value to delete
 * @return HashChainNode* Deleted value's hash chain node, NULL if key not exist
 */
HashChainNode* hashTable_delete(HashTable* table, Object key);

/**
 * @brief Find value correspond to key
 * 
 * @param table Hash table
 * @param key Key of value to find
 * @return HashChainNode* Value's hash chain node, NULL if key not exist
 */
HashChainNode* hashTable_find(HashTable* table, Object key);

#endif // __LIB_STRUCTS_HASHTABLE_H
