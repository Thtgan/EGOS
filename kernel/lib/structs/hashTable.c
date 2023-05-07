#include<structs/hashTable.h>

#include<kit/oop.h>
#include<kit/types.h>
#include<structs/singlyLinkedList.h>

void initHashTable(HashTable* table, size_t hashSize, SinglyLinkedList* chains, HASH_FUNC) {
    table->size = 0;
    table->hashSize = hashSize;
    table->hashFunc = hashFunc;
    table->chains = chains;

    for (int i = 0; i < hashSize; i++) {
        initSinglyLinkedList(table->chains + i);
    }
}

Result hashTableInsert(HashTable* table, Object key, HashChainNode* newNode) {
    size_t hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next; node != chain; node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            return RESULT_FAIL;
        }
    }

    newNode->key = key;
    initSinglyLinkedListNode(&newNode->node);
    singlyLinkedListInsertNext(chain, &newNode->node);

    ++table->size;

    return RESULT_SUCCESS;
}

void initHashChainNode(HashChainNode* node) {
    initSinglyLinkedListNode(&node->node);
    node->key = 0;
}

HashChainNode* hashTableDelete(HashTable* table, Object key) {
    size_t hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next, *last = chain; node != chain; last = node, node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            singlyLinkedListDeleteNext(last);
            --table->size;

            initSinglyLinkedListNode(&chainNode->node);
            return chainNode;
        }
    }

    return NULL;
}

HashChainNode* hashTableFind(HashTable* table, Object key) {
    size_t hashKey = table->hashFunc(table, key);

    SinglyLinkedList* chain = table->chains + hashKey;
    for (SinglyLinkedListNode* node = chain->next, *last = chain; node != chain; last = node, node = node->next) {
        HashChainNode* chainNode = HOST_POINTER(node, HashChainNode, node);

        if (chainNode->key == key) {
            return chainNode;
        }
    }

    return NULL;
}