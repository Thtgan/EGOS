#include<structs/hashTable.h>

#include<kit/oop.h>
#include<kit/types.h>
#include<kit/util.h>
#include<structs/singlyLinkedList.h>

Size hashTable_defaultHashFunc(HashTable* this, Object key) {
    return key % this->bucket;
}

void hashTable_initStruct(HashTable* table, Size bucket, SinglyLinkedList* chains, HashTableHashFunc hashFunc) {
    table->size = 0;
    table->bucket = bucket;
    table->hashFunc = hashFunc;
    table->chains = chains;

    for (int i = 0; i < bucket; i++) {
        singlyLinkedList_initStruct(table->chains + i);
    }
}

Result hashTable_insert(HashTable* table, Object key, HashChainNode* newNode) {
    Size hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next; node != chain; node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            return RESULT_FAIL;
        }
    }

    newNode->key = key;
    singlyLinkedListNode_initStruct(&newNode->node);
    singlyLinkedList_insertNext(chain, &newNode->node);

    ++table->size;

    return RESULT_SUCCESS;
}

void hashChainNode_initStruct(HashChainNode* node) {
    singlyLinkedListNode_initStruct(&node->node);
    node->key = 0;
}

HashChainNode* hashTable_delete(HashTable* table, Object key) {
    Size hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next, *last = chain; node != chain; last = node, node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            singlyLinkedList_deleteNext(last);
            --table->size;

            singlyLinkedListNode_initStruct(&chainNode->node);
            return chainNode;
        }
    }

    return NULL;
}

HashChainNode* hashTable_find(HashTable* table, Object key) {
    Size hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next, *last = chain; node != chain; last = node, node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            return chainNode;
        }
    }

    return NULL;
}