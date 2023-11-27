#if !defined(__HASH_TABLE_H)
#define __HASH_TABLE_H

#include<kit/oop.h>
#include<kit/types.h>
#include<structs/singlyLinkedList.h>

RECURSIVE_REFER_STRUCT(HashTable) {
    Size size, hashSize;
    SinglyLinkedList* chains;
    Size (*hashFunc)(HashTable* this, Object key);
};

typedef struct {
    SinglyLinkedListNode node;
    Object key;
} HashChainNode;

#define HASH_FUNC Size (*hashFunc)(HashTable* this, Object key)

/**
 * @brief Initialize hash table
 * 
 * @param table Hash table struct
 * @param hashSize Num of hash chains
 * @param chains Hash chain list
 * @param hashFunc Hash function to apply
 */
void hashTable_initStruct(HashTable* table, Size hashSize, SinglyLinkedList* chains, HASH_FUNC);

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
 * @return Result Result of the operation
 */
Result hashTable_insert(HashTable* table, Object key, HashChainNode* node);

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

#endif // __HASH_TABLE_H
