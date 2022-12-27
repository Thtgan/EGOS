#include<structs/hashTable.h>

#include<kit/oop.h>
#include<kit/types.h>
#include<memory/virtualMalloc.h>
#include<structs/singlyLinkedList.h>

typedef struct {
    SinglyLinkedListNode node;
    Object key, value;
} __HashChainNode;

void initHashTable(HashTable* table, size_t hashSize, HASH_FUNC) {
    table->size = 0;
    table->hashSize = hashSize;
    table->hashFunc = hashFunc;

    table->table = vMalloc(hashSize * sizeof(SinglyLinkedList));
    for (int i = 0; i < hashSize; i++) {
        initSinglyLinkedList(table->table + i);
    }
}

void destroyHashTable(HashTable* table) {
    vFree(table->table);
}

bool hashTableInsert(HashTable* table, Object key, Object value) {
    size_t hashKey = THIS_ARG_APPEND_CALL(table, hashFunc, key);

    SinglyLinkedList* list = table->table + hashKey;
    
    for (SinglyLinkedListNode* node = list->next; node != list; node = node->next) {
        __HashChainNode* chainNode = HOST_POINTER(node, __HashChainNode, node);

        if (chainNode->key == key) {
            return false;
        }
    }

    __HashChainNode* newChainNode = vMalloc(sizeof(__HashChainNode));
    newChainNode->key = key, newChainNode->value = value;
    initSinglyLinkedListNode(&newChainNode->node);
    singlyLinkedListInsertNext(list, &newChainNode->node);

    ++table->size;

    return true;
}

bool hashTableDelete(HashTable* table, Object key, Object* valueReturn) {
    size_t hashKey = THIS_ARG_APPEND_CALL(table, hashFunc, key);

    SinglyLinkedList* list = table->table + hashKey;
    
    for (SinglyLinkedListNode* node = list->next, *last = list; node != list; last = node, node = node->next) {
        __HashChainNode* chainNode = HOST_POINTER(node, __HashChainNode, node);

        if (chainNode->key == key) {
            singlyLinkedListDeleteNext(last);
            *valueReturn = chainNode->value;
            vFree(chainNode);

            --table->size;

            return true;
        }
    }

    *valueReturn = OBJECT_NULL;
    return false;
}

bool hashTableFind(HashTable* table, Object key, Object* valueReturn) {
    size_t hashKey = THIS_ARG_APPEND_CALL(table, hashFunc, key);

    SinglyLinkedList* list = table->table + hashKey;
    
    for (SinglyLinkedListNode* node = list->next, *last = list; node != list; last = node, node = node->next) {
        __HashChainNode* chainNode = HOST_POINTER(node, __HashChainNode, node);

        if (chainNode->key == key) {
            *valueReturn = chainNode->value;
            return true;
        }
    }

    *valueReturn = OBJECT_NULL;
    return false;
}