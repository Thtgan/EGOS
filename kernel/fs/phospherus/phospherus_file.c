#include<fs/phospherus/phospherus_file.h>

#include<algorithms.h>
#include<devices/block/blockDevice.h>
#include<fs/file.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

static Index64 __seek(File* this, size_t seekTo);

static ReturnValue __read(File* this, void* buffer, size_t n);

static ReturnValue __write(File* this, const void* buffer, size_t n);

FileOperations fileOperations = {
    .seek = __seek,
    .read = __read,
    .write = __write
};

static File* __openFile(iNode* iNode);

static ReturnValue __closeFile(File* file);

FileGlobalOperations fileGlobalOperations = {
    .openFile = __openFile,
    .closeFile = __closeFile
};

FileGlobalOperations* phospherusInitFiles() {
    return &fileGlobalOperations;
}

static Index64 __seek(File* this, size_t seekTo) {
    if (seekTo == INVALID_INDEX) {
        return this->pointer = INVALID_INDEX;
    }
    size_t fileSize = this->iNode->onDevice.dataSize;
    return this->pointer = seekTo >= fileSize ? INVALID_INDEX : seekTo;
}

static ReturnValue __read(File* this, void* buffer, size_t n) {
    Index64 pointer = this->pointer;
    if (pointer == INVALID_INDEX) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_INDEX, RETURN_VALUE_STATUS_OUT_OF_BOUND);
    }

    iNode* iNode = this->iNode;

    size_t fileSize = iNode->onDevice.dataSize;
    size_t remainByteToRead = umin64(fileSize - pointer, n), currentReadPointer = pointer;
    void* currentBuffer = buffer, * tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    Index64 flooredReadPointer = currentReadPointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredReadPointer < currentReadPointer) {
        iNodeReadBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);

        Index64 indexInBlock = currentReadPointer - flooredReadPointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToRead);
        memcpy(currentBuffer, tmpBuffer + indexInBlock, frontByteNum);

        currentBuffer += frontByteNum;
        currentReadPointer += frontByteNum;
        remainByteToRead -= frontByteNum;
    }

    size_t midBlockNum = remainByteToRead / BLOCK_SIZE;
    if (midBlockNum > 0) {
        iNodeReadBlocks(iNode, currentBuffer, currentReadPointer / BLOCK_SIZE, midBlockNum);
        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentReadPointer += midBlockNum * BLOCK_SIZE;
        remainByteToRead -= midBlockNum * BLOCK_SIZE;
    }

    if (remainByteToRead > 0) {
        iNodeReadBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToRead, BLOCK_SIZE);
        memcpy(currentBuffer, tmpBuffer, tailByteNum);

        currentBuffer += tailByteNum;
        currentReadPointer += tailByteNum;
        remainByteToRead -= tailByteNum;
    }

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    if (remainByteToRead != 0) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
    }

    __seek(this, currentReadPointer);
    return RETURN_VALUE_RETURN_NORMALLY;
}

static ReturnValue __write(File* this, const void* buffer, size_t n) {
    iNode* iNode = this->iNode;

    size_t pointer = this->pointer;
    pointer = (pointer == INVALID_INDEX) ? iNode->onDevice.dataSize : pointer;

    size_t blockNum = iNode->onDevice.availableBlockSize, leastBlockNumAfterWrite = (pointer + n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (leastBlockNumAfterWrite > blockNum) {
        if (iNodeResize(iNode, leastBlockNumAfterWrite) == -1) {
            return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
        }
    }

    size_t remainByteToWrite = n, currentWritePointer = pointer;
    const void* currentBuffer = buffer;
    void* tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    size_t flooredWritePointer = currentWritePointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredWritePointer < currentWritePointer) {
        iNodeReadBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);

        size_t indexInBlock = currentWritePointer - flooredWritePointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToWrite);
        memcpy(tmpBuffer + indexInBlock, currentBuffer, frontByteNum);
        if (iNodeWriteBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == -1) {
            return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
        }

        currentBuffer += frontByteNum;
        currentWritePointer += frontByteNum;
        remainByteToWrite -= frontByteNum;
    }

    size_t midBlockNum = remainByteToWrite / BLOCK_SIZE;
    if (midBlockNum > 0) {
        iNodeWriteBlocks(iNode, currentBuffer, currentWritePointer / BLOCK_SIZE, midBlockNum);
        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentWritePointer += midBlockNum * BLOCK_SIZE;
        remainByteToWrite -= midBlockNum * BLOCK_SIZE;
    }
    
    if (remainByteToWrite > 0) {
        iNodeReadBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1);
        size_t tailByteNum = umin64(remainByteToWrite, BLOCK_SIZE);
        memcpy(tmpBuffer, currentBuffer, tailByteNum);
        if (iNodeWriteBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == -1) {
            return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
        }

        currentBuffer += tailByteNum;
        currentWritePointer += tailByteNum;
        remainByteToWrite -= tailByteNum;
    }

    releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    if (remainByteToWrite != 0) {
        return BUILD_ERROR_RETURN_VALUE(RETURN_VALUE_OBJECT_EXECUTION, RETURN_VALUE_STATUS_OPERATION_FAIL);
    }

    iNode->onDevice.dataSize = umax64(iNode->onDevice.dataSize, pointer + n);
    blockDeviceWriteBlocks(iNode->device, iNode->blockIndex, &iNode->onDevice, 1);

    __seek(this, currentWritePointer);
    return RETURN_VALUE_RETURN_NORMALLY;
}

static File* __openFile(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_FILE) {
        return NULL;
    }

    File* ret = kMalloc(sizeof(File), MEMORY_TYPE_NORMAL);
    ret->iNode = iNode;
    ret->operations = &fileOperations;
    ret->pointer = 0;

    return ret;
}

static ReturnValue __closeFile(File* file) {
    file->iNode = NULL;
    kFree(file);

    return RETURN_VALUE_RETURN_NORMALLY;
}