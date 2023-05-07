#include<fs/phospherus/phospherus_file.h>

#include<algorithms.h>
#include<devices/block/blockDevice.h>
#include<error.h>
#include<fs/file.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

static Index64 __seek(File* this, size_t seekTo);

static Result __read(File* this, void* buffer, size_t n);

static Result __write(File* this, const void* buffer, size_t n);

FileOperations fileOperations = {
    .seek = __seek,
    .read = __read,
    .write = __write
};

static File* __fileOpen(iNode* iNode);

static Result __fileClose(File* file);

static Result __doRead(File* this, void* buffer, size_t n, void** tmpBufferPtr);

static Result __doWrite(File* this, const void* buffer, size_t n, void** tmpBufferPtr);

FileGlobalOperations fileGlobalOperations = {
    .fileOpen = __fileOpen,
    .fileClose = __fileClose
};

FileGlobalOperations* phospherusInitFiles() {
    return &fileGlobalOperations;
}

static Index64 __seek(File* this, size_t seekTo) {
    size_t fileSize = this->iNode->onDevice.dataSize;
    if (seekTo > fileSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return INVALID_INDEX;
    }

    return this->pointer = seekTo;
}

static Result __read(File* this, void* buffer, size_t n) {
    void* tmpBuffer = NULL;

    Result ret = __doRead(this, buffer, n, &tmpBuffer);

    if (tmpBuffer != NULL) {
        releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    }

    return ret;
}

static Result __write(File* this, const void* buffer, size_t n) {
    void* tmpBuffer = NULL;

    Result ret = __doWrite(this, buffer, n, &tmpBuffer);

    if (tmpBuffer != NULL) {
        releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    }

    return ret;
}

static File* __fileOpen(iNode* iNode) {
    if (iNode->onDevice.type != INODE_TYPE_FILE) {
        return NULL;
    }

    if (iNode->entryReference != NULL) {
        ++iNode->referenceCnt;
        return iNode->entryReference;
    }

    File* ret = kMalloc(sizeof(File), MEMORY_TYPE_NORMAL);
    if (ret == NULL) {
        return NULL;
    }

    ret->iNode = iNode;
    ret->operations = &fileOperations;
    ret->pointer = 0;

    iNode->entryReference = (void*)ret;
    iNode->referenceCnt = 1;

    return ret;
}

static Result __fileClose(File* file) {
    file->iNode = NULL;
    kFree(file);

    return RESULT_SUCCESS;
}

static Result __doRead(File* this, void* buffer, size_t n, void** tmpBufferPtr) {
    Index64 pointer = this->pointer;
    iNode* iNode = this->iNode;
    size_t fileSize = iNode->onDevice.dataSize;
    if (pointer > fileSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    size_t remainByteToRead = umin64(fileSize - pointer, n), currentReadPointer = pointer;
    void* currentBuffer = buffer, * tmpBuffer = NULL;
    *tmpBufferPtr = tmpBuffer = allocateBuffer(BUFFER_SIZE_512);
    if (tmpBuffer == NULL) {
        return RESULT_FAIL;
    }

    Index64 flooredReadPointer = currentReadPointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredReadPointer < currentReadPointer) {
        if (rawInodeReadBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        Index64 indexInBlock = currentReadPointer - flooredReadPointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToRead);
        memcpy(currentBuffer, tmpBuffer + indexInBlock, frontByteNum);

        currentBuffer += frontByteNum;
        currentReadPointer += frontByteNum;
        remainByteToRead -= frontByteNum;
    }

    size_t midBlockNum = remainByteToRead / BLOCK_SIZE;
    if (midBlockNum > 0) {
        if (rawInodeReadBlocks(iNode, currentBuffer, currentReadPointer / BLOCK_SIZE, midBlockNum) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentReadPointer += midBlockNum * BLOCK_SIZE;
        remainByteToRead -= midBlockNum * BLOCK_SIZE;
    }

    if (remainByteToRead > 0) {
        if (rawInodeReadBlocks(iNode, tmpBuffer, currentReadPointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        size_t tailByteNum = umin64(remainByteToRead, BLOCK_SIZE);
        memcpy(currentBuffer, tmpBuffer, tailByteNum);

        currentBuffer += tailByteNum;
        currentReadPointer += tailByteNum;
        remainByteToRead -= tailByteNum;
    }

    if (remainByteToRead != 0) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    if (__seek(this, currentReadPointer) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

static Result __doWrite(File* this, const void* buffer, size_t n, void** tmpBufferPtr) {
    iNode* iNode = this->iNode;
    size_t pointer = (this->pointer == INVALID_INDEX) ? iNode->onDevice.dataSize : this->pointer;

    size_t blockNum = iNode->onDevice.availableBlockSize, leastBlockNumAfterWrite = (pointer + n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (leastBlockNumAfterWrite > blockNum && rawInodeResize(iNode, leastBlockNumAfterWrite) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    size_t remainByteToWrite = n, currentWritePointer = pointer;
    const void* currentBuffer = buffer;
    void* tmpBuffer = NULL;
    *tmpBufferPtr = tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    size_t flooredWritePointer = currentWritePointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredWritePointer < currentWritePointer) {
        if (rawInodeReadBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        size_t indexInBlock = currentWritePointer - flooredWritePointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToWrite);
        memcpy(tmpBuffer + indexInBlock, currentBuffer, frontByteNum);
        if (rawInodeWriteBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentBuffer += frontByteNum;
        currentWritePointer += frontByteNum;
        remainByteToWrite -= frontByteNum;
    }

    size_t midBlockNum = remainByteToWrite / BLOCK_SIZE;
    if (midBlockNum > 0) {
        if (rawInodeWriteBlocks(iNode, currentBuffer, currentWritePointer / BLOCK_SIZE, midBlockNum) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentBuffer += midBlockNum * BLOCK_SIZE;
        currentWritePointer += midBlockNum * BLOCK_SIZE;
        remainByteToWrite -= midBlockNum * BLOCK_SIZE;
    }
    
    if (remainByteToWrite > 0) {
        if (rawInodeReadBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        size_t tailByteNum = umin64(remainByteToWrite, BLOCK_SIZE);
        memcpy(tmpBuffer, currentBuffer, tailByteNum);

        if (rawInodeWriteBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentBuffer += tailByteNum;
        currentWritePointer += tailByteNum;
        remainByteToWrite -= tailByteNum;
    }

    if (remainByteToWrite != 0) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return RESULT_FAIL;
    }

    iNode->onDevice.dataSize = umax64(iNode->onDevice.dataSize, pointer + n);
    if (blockDeviceWriteBlocks(iNode->device, iNode->blockIndex, &iNode->onDevice, 1) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    if (__seek(this, currentWritePointer) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}