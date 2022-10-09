#include<fs/phospherus/phospherus_file.h>

#include<algorithms.h>
#include<devices/block/blockDevice.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>
#include<memory/memory.h>
#include<memory/virtualMalloc.h>

int __seek(THIS_ARG_APPEND(File, size_t seekTo));

int __read(THIS_ARG_APPEND(File, void* buffer, size_t n));

int __write(THIS_ARG_APPEND(File, const void* buffer, size_t n));

FileOperations fileOperations = {
    .seek = __seek,
    .read = __read,
    .write = __write
};

File* __openFile(iNode* iNode);

int __closeFile(File* file);

FileGlobalOperations fileGlobalOperations = {
    .openFile = __openFile,
    .closeFile = __closeFile
};

FileGlobalOperations* phospherusInitFiles() {
    return &fileGlobalOperations;
}

int __seek(THIS_ARG_APPEND(File, size_t seekTo)) {
    if (seekTo == PHOSPHERUS_NULL) {
        return PHOSPHERUS_NULL;
    }
    size_t fileSize = this->iNode->onDevice.dataSize;
    return this->pointer = seekTo >= fileSize ? PHOSPHERUS_NULL : seekTo;
}

int __read(THIS_ARG_APPEND(File, void* buffer, size_t n)) {
    Index64 pointer = this->pointer;
    if (pointer == PHOSPHERUS_NULL) {
        return -1;
    }

    iNode* iNode = this->iNode;

    size_t fileSize = iNode->onDevice.dataSize;
    size_t remainByteToRead = umin64(fileSize - pointer, n), currentReadPointer = pointer;
    void* currentBuffer = buffer, * tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    Index64 flooredReadPointer = currentReadPointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredReadPointer < currentReadPointer) {
        iNode->operations->readBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);

        Index64 indexInBlock = currentReadPointer - flooredReadPointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToRead);
        memcpy(currentBuffer, tmpBuffer + indexInBlock, frontByteNum);

        currentBuffer += frontByteNum;
        currentReadPointer += frontByteNum;
        remainByteToRead -= frontByteNum;
    }

    size_t midBlockNum = remainByteToRead / BLOCK_SIZE;
    if (midBlockNum > 0) {
        iNode->operations->readBlocks(iNode, currentBuffer, currentReadPointer / BLOCK_SIZE, midBlockNum);
        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentReadPointer += midBlockNum * BLOCK_SIZE;
        remainByteToRead -= midBlockNum * BLOCK_SIZE;
    }

    if (remainByteToRead > 0) {
        iNode->operations->readBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToRead, BLOCK_SIZE);
        memcpy(currentBuffer, tmpBuffer, tailByteNum);

        currentBuffer += tailByteNum;
        currentReadPointer += tailByteNum;
        remainByteToRead -= tailByteNum;
    }

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    if (remainByteToRead != 0) {
        return -1;
    }

    __seek(this, currentReadPointer);
    return 0;
}

int __write(THIS_ARG_APPEND(File, const void* buffer, size_t n)) {
    iNode* iNode = this->iNode;

    size_t pointer = this->pointer;
    pointer = (pointer == PHOSPHERUS_NULL) ? iNode->onDevice.dataSize : pointer;

    size_t blockNum = iNode->onDevice.availableBlockSize, leastBlockNumAfterWrite = (pointer + n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (leastBlockNumAfterWrite > blockNum) {
        if (iNode->operations->resize(iNode, leastBlockNumAfterWrite) == -1) {
            return -1;
        }
    }

    size_t remainByteToWrite = n, currentWritePointer = pointer;
    const void* currentBuffer = buffer;
    void* tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    size_t flooredWritePointer = currentWritePointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredWritePointer < currentWritePointer) {
        iNode->operations->readBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);

        size_t indexInBlock = currentWritePointer - flooredWritePointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToWrite);
        memcpy(tmpBuffer + indexInBlock, currentBuffer, frontByteNum);
        if (iNode->operations->writeBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == -1) {
            return -1;
        }

        currentBuffer += frontByteNum;
        currentWritePointer += frontByteNum;
        remainByteToWrite -= frontByteNum;
    }

    size_t midBlockNum = remainByteToWrite / BLOCK_SIZE;
    if (midBlockNum > 0) {
        iNode->operations->writeBlocks(iNode, currentBuffer, currentWritePointer / BLOCK_SIZE, midBlockNum);
        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentWritePointer += midBlockNum * BLOCK_SIZE;
        remainByteToWrite -= midBlockNum * BLOCK_SIZE;
    }
    
    if (remainByteToWrite > 0) {
        iNode->operations->readBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToWrite, BLOCK_SIZE);
        memcpy(tmpBuffer, currentBuffer, tailByteNum);
        if (iNode->operations->writeBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == -1) {
            return -1;
        }

        currentBuffer += tailByteNum;
        currentWritePointer += tailByteNum;
        remainByteToWrite -= tailByteNum;
    }

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    if (remainByteToWrite != 0) {
        return -1;
    }

    iNode->onDevice.dataSize = umax64(iNode->onDevice.dataSize, pointer + n);
    THIS_ARG_APPEND_CALL(iNode->device, operations->writeBlocks, iNode->blockIndex, &iNode->onDevice, 1);

    __seek(this, currentWritePointer);
    return 0;
}

File* __openFile(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_FILE) {
        return NULL;
    }

    File* ret = vMalloc(sizeof(File));
    ret->iNode = iNode;
    ret->operations = &fileOperations;
    ret->pointer = 0;

    return ret;
}

int __closeFile(File* file) {
    file->iNode = NULL;
    vFree(file);

    return 0;
}