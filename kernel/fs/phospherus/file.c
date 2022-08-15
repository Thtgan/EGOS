#include<fs/phospherus/file.h>

#include<algorithms.h>
#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/phospherus/allocator.h>
#include<fs/phospherus/inode.h>
#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>
#include<stdbool.h>
#include<stddef.h>

block_index_t phospherus_createFile(Phospherus_Allocator* allocator) {
    block_index_t inodeIndex = phospherus_createInode(allocator, 0);
    if (inodeIndex == PHOSPHERUS_NULL) {
        return PHOSPHERUS_NULL;
    }

    iNodeDesc* inode = phospherus_openInode(allocator, inodeIndex);
    if (inode == NULL) {
        return PHOSPHERUS_NULL;
    }
    phospherus_setInodeDataSize(inode, 0);
    phospherus_closeInode(inode);

    return inodeIndex;
}

bool phospherus_deleteFile(Phospherus_Allocator* allocator, block_index_t inodeBlock) {
    return phospherus_deleteInode(allocator, inodeBlock);
}

File* phospherus_openFile(Phospherus_Allocator* allocator, block_index_t inodeBlock) {
    iNodeDesc* inode = phospherus_openInode(allocator, inodeBlock);
    if (inode == NULL) {
        return NULL;
    }

    File* ret = vMalloc(sizeof(File));
    ret->inode = inode;
    ret->pointer = 0;

    return ret;
}

void phospherus_closeFile(File* file) {
    phospherus_closeInode(file->inode);
    file->inode = NULL;
    file->pointer = 0;
    vFree(file);
}

size_t phospherus_getFilePointer(File* file) {
    return file->pointer;
}

void phospherus_seekFile(File* file, size_t pointer) {
    if (pointer == PHOSPHERUS_NULL) {
        return;
    }
    size_t fileSize = phospherus_getFileSize(file);
    file->pointer = pointer >= fileSize ? PHOSPHERUS_NULL : pointer;
}

size_t phospherus_getFileSize(File* file) {
    return phospherus_getInodeDataSize(file->inode);
}

static const char* _readFileFailedInfo = "Read file failed";

size_t phospherus_readFile(File* file, void* buffer, size_t size) {
    size_t pointer = file->pointer;
    if (pointer == PHOSPHERUS_NULL) {
        return PHOSPHERUS_NULL;
    }

    size_t fileSize = phospherus_getInodeDataSize(file->inode);
    size_t remainByteToRead = umin64(fileSize - pointer, size), currentReadPointer = pointer;
    void* currentBuffer = buffer, * tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    do {
        size_t flooredReadPointer = currentReadPointer / BLOCK_SIZE * BLOCK_SIZE;
        if (flooredReadPointer < currentReadPointer) {
            phospherus_readInodeBlocks(file->inode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);
            size_t indexInBlock = currentReadPointer - flooredReadPointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToRead);
            memcpy(currentBuffer, tmpBuffer + indexInBlock, frontByteNum);

            currentBuffer += frontByteNum;
            currentReadPointer += frontByteNum;
            remainByteToRead -= frontByteNum;
        }

        if (remainByteToRead == 0) {
            break;
        }

        size_t midBlockNum = remainByteToRead / BLOCK_SIZE;
        if (midBlockNum > 0) {
            phospherus_readInodeBlocks(file->inode, currentBuffer, currentReadPointer / BLOCK_SIZE, midBlockNum);
            size_t midByteNum = midBlockNum * BLOCK_SIZE;

            currentBuffer += midByteNum;
            currentReadPointer += midByteNum;
            remainByteToRead -= midByteNum;
        }

        if (remainByteToRead == 0) {
            break;
        }

        phospherus_readInodeBlocks(file->inode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToRead, BLOCK_SIZE);
        memcpy(currentBuffer, tmpBuffer, remainByteToRead);

        currentBuffer += tailByteNum;
        currentReadPointer += tailByteNum;
        remainByteToRead -= tailByteNum;
    } while (false);

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);

    if (remainByteToRead != 0) {
        blowup(_readFileFailedInfo);
    }

    file->pointer = currentReadPointer >= fileSize ? PHOSPHERUS_NULL : currentReadPointer;
    return file->pointer;
}

static const char* _writeFileFailedInfo = "Write file failed";

size_t phospherus_writeFile(File* file, const void* buffer, size_t size) {
    size_t pointer = file->pointer;
    if (phospherus_getInodeWriteProtection(file->inode)) {
        return pointer;
    }

    pointer = (pointer == PHOSPHERUS_NULL) ? phospherus_getFileSize(file) : pointer;
    size_t blockNum = phospherus_getInodeBlockSize(file->inode), leastBlockNumAfterWrite = (pointer + size + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (leastBlockNumAfterWrite > blockNum) {
        if (!phospherus_resizeInode(file->inode, leastBlockNumAfterWrite)) {
            blowup(_writeFileFailedInfo);
        }
    }

    size_t remainByteToWrite = size, currentWritePointer = pointer;
    const void* currentBuffer = buffer;
    void* tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    bool successed = true;
    do {
        size_t flooredWritePointer = currentWritePointer / BLOCK_SIZE * BLOCK_SIZE;
        if (flooredWritePointer < currentWritePointer) {
            phospherus_readInodeBlocks(file->inode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);
            size_t indexInBlock = currentWritePointer - flooredWritePointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToWrite);
            memcpy(tmpBuffer + indexInBlock, currentBuffer, frontByteNum);
            successed &= phospherus_writeInodeBlocks(file->inode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);

            currentBuffer += frontByteNum;
            currentWritePointer += frontByteNum;
            remainByteToWrite -= frontByteNum;
        }

        if (!successed || remainByteToWrite == 0) {
            break;
        }

        size_t midBlockNum = remainByteToWrite / BLOCK_SIZE;
        if (midBlockNum > 0) {
            successed &= phospherus_writeInodeBlocks(file->inode, currentBuffer, currentWritePointer / BLOCK_SIZE, midBlockNum);
            size_t midByteNum = midBlockNum * BLOCK_SIZE;

            currentBuffer += midByteNum;
            currentWritePointer += midByteNum;
            remainByteToWrite -= midByteNum;
        }

        if (!successed || remainByteToWrite == 0) {
            break;
        }

        phospherus_readInodeBlocks(file->inode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToWrite, BLOCK_SIZE);
        memcpy(tmpBuffer, currentBuffer, remainByteToWrite);
        successed &= phospherus_writeInodeBlocks(file->inode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);

        currentBuffer += tailByteNum;
        currentWritePointer += tailByteNum;
        remainByteToWrite -= tailByteNum;
    } while (false);

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);

    if (!successed || remainByteToWrite != 0) {
        blowup(_writeFileFailedInfo);
    }

    size_t newFileSize = umax64(phospherus_getInodeDataSize(file->inode), pointer + size);
    phospherus_setInodeDataSize(file->inode, newFileSize);
    file->pointer = currentWritePointer >= newFileSize ? PHOSPHERUS_NULL : currentWritePointer;
    return file->pointer;
}

bool phospherus_truncateFile(File* file, size_t truncateAt) {
    if (truncateAt >= phospherus_getInodeDataSize(file->inode) || phospherus_getInodeWriteProtection(file->inode)) {
        return false;
    }

    size_t truncatedBlockNum = (truncateAt + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (!phospherus_resizeInode(file->inode, truncatedBlockNum)) {
        return false;
    }

    if (truncateAt % BLOCK_SIZE != 0) {
        void* buffer = allocateBuffer(BUFFER_SIZE_512);
        phospherus_readInodeBlocks(file->inode, buffer, truncatedBlockNum - 1, 1);
        memset(buffer + (truncateAt % BLOCK_SIZE), 0, BLOCK_SIZE - (truncateAt % BLOCK_SIZE));
        if (!phospherus_writeInodeBlocks(file->inode, buffer, truncatedBlockNum - 1, 1)) {
            return false;
        }
        releaseBuffer(buffer, BUFFER_SIZE_512);
    }
    phospherus_setInodeDataSize(file->inode, truncateAt);

    return true;
}

bool phospherus_getFileWriteProtection(File* file) {
    return phospherus_getInodeWriteProtection(file->inode);
}

void phospherus_setFileWriteProtection(File* file, bool writeProrection) {
    phospherus_setInodeWriteProtection(file->inode, writeProrection);
}