#if !defined(__FS_SUPERBLOCK_H)
#define __FS_SUPERBLOCK_H

#include<devices/block/blockDevice.h>
#include<fs/fsStructs.h>
#include<fs/inode.h>
#include<kit/oop.h>
#include<kit/types.h>
#include<structs/hashTable.h>

static inline Result superBlock_rawOpenInode(SuperBlock* superBlock, iNode* iNode, fsEntryDesc* desc) {
    return superBlock->operations->openInode(superBlock, iNode, desc);
}

static inline Result superBlock_rawCloseInode(SuperBlock* superBlock, iNode* iNode) {
    return superBlock->operations->closeInode(superBlock, iNode);
}

static inline Result superBlock_rawOpenfsEntry(SuperBlock* superBlock, fsEntry* entry, fsEntryDesc* desc) {
    return superBlock->operations->openfsEntry(superBlock, entry, desc);
}

static inline Result superBlock_rawClosefsEntry(SuperBlock* superBlock, fsEntry* entry) {
    return superBlock->operations->closefsEntry(superBlock, entry);
}

static inline Result superBlock_rawMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc) {
    return superBlock->operations->mount(superBlock, identifier, targetSuperBlock, targetDesc);
}

static inline Result superBlock_rawUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier) {
    return superBlock->operations->unmount(superBlock, identifier);
}

void superBlock_initStruct(SuperBlock* superBlock, SuperBlockInitArgs* args);

Result superBlock_readfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc* descOut);

//TODO: Rework this
fsEntryDesc* superBlock_getfsEntryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** superBlockOut);

Result superBlock_releasefsEntryDesc(SuperBlock* superBlock, fsEntryDesc* entryDesc);

Result superBlock_openfsEntry(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntry* entryOut);

Result superBlock_closefsEntry(fsEntry* entry);

Result mount_initStruct(Mount* mount, ConstCstring path, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

void mount_clearStruct(Mount* mount);

Result superBlock_genericMount(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock* targetSuperBlock, fsEntryDesc* targetDesc);

Result superBlock_genericUnmount(SuperBlock* superBlock, fsEntryIdentifier* identifier);

#endif // __FS_SUPERBLOCK_H
