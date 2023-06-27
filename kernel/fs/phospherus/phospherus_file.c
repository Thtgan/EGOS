#include<fs/phospherus/phospherus_file.h>

#include<algorithms.h>
#include<devices/block/blockDevice.h>
#include<error.h>
#include<fs/file.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/bit.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/kMalloc.h>
#include<memory/memory.h>

static Index64 __seek(File* this, Size seekTo);

static Result __read(File* this, void* buffer, Size n);

static Result __write(File* this, const void* buffer, Size n);

FileOperations fileOperations = {
    .seek = __seek,
    .read = __read,
    .write = __write
};

static File* __fileOpen(iNode* iNode, Flags8 flags);

static Result __fileClose(File* file);

static Result __doRead(File* this, void* buffer, Size n, void** tmpBufferPtr);

static Result __doWrite(File* this, const void* buffer, Size n, void** tmpBufferPtr);

FileGlobalOperations fileGlobalOperations = {
    .fileOpen = __fileOpen,
    .fileClose = __fileClose
};

FileGlobalOperations* phospherusInitFiles() {
    return &fileGlobalOperations;
}

static Index64 __seek(File* this, Size seekTo) {
    Size fileSize = this->iNode->onDevice.dataSize;
    if (seekTo > fileSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return INVALID_INDEX;
    }

    return this->pointer = seekTo;
}

static Result __read(File* this, void* buffer, Size n) {
    void* tmpBuffer = NULL;

    Result ret = __doRead(this, buffer, n, &tmpBuffer);

    if (tmpBuffer != NULL) {
        releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    }

    return ret;
}

static Result __write(File* this, const void* buffer, Size n) {
    void* tmpBuffer = NULL;

    Result ret = __doWrite(this, buffer, n, &tmpBuffer);

    if (tmpBuffer != NULL) {
        releaseBuffer(tmpBuffer, BUFFER_SIZE_512);
    }

    return ret;
}

static File* __fileOpen(iNode* iNode, Flags8 flags) {
    if (iNode->onDevice.type != INODE_TYPE_FILE) {
        return NULL;
    }

    File* ret = kMalloc(sizeof(File));
    if (ret == NULL) {
        return NULL;
    }

    ret->iNode = iNode;
    memset(&ret->operations, 0, sizeof(FileOperations));
    if (VAL_AND(flags, FILE_FLAG_RW_MASK) == 0) {
        kFree(ret);
        return NULL;
    }

    ret->operations.seek = __seek;
    switch (VAL_AND(flags, FILE_FLAG_RW_MASK)) {
        case FILE_FLAG_READ_ONLY:
            ret->operations.read = __read;
            break;
        case FILE_FLAG_WRITE_ONLY:
            ret->operations.write = __write;
            break;
        case FILE_FLAG_READ_WRITE:
            ret->operations.read = __read;
            ret->operations.write = __write;
            break;
        default:
            break;
    }

    ret->pointer = 0;
    ret->flags = flags;

    return ret;
}

static Result __fileClose(File* file) {
    file->iNode = NULL;
    kFree(file);

    return RESULT_SUCCESS;
}

static Result __doRead(File* this, void* buffer, Size n, void** tmpBufferPtr) {
    Index64 pointer = this->pointer;
    iNode* iNode = this->iNode;
    Size fileSize = iNode->onDevice.dataSize;
    if (pointer > fileSize) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    Size remainByteToRead = umin64(fileSize - pointer, n), currentReadPointer = pointer;
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

    Size midBlockNum = remainByteToRead / BLOCK_SIZE;
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

        Size tailByteNum = umin64(remainByteToRead, BLOCK_SIZE);
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

static Result __doWrite(File* this, const void* buffer, Size n, void** tmpBufferPtr) {
    iNode* iNode = this->iNode;
    Size pointer = (this->pointer == INVALID_INDEX) ? iNode->onDevice.dataSize : this->pointer;

    Size blockNum = iNode->onDevice.availableBlockSize, leastBlockNumAfterWrite = (pointer + n + BLOCK_SIZE - 1) / BLOCK_SIZE;
    if (leastBlockNumAfterWrite > blockNum && rawInodeResize(iNode, leastBlockNumAfterWrite) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Size remainByteToWrite = n, currentWritePointer = pointer;
    const void* currentBuffer = buffer;
    void* tmpBuffer = NULL;
    *tmpBufferPtr = tmpBuffer = allocateBuffer(BUFFER_SIZE_512);

    Size flooredWritePointer = currentWritePointer / BLOCK_SIZE * BLOCK_SIZE;
    if (flooredWritePointer < currentWritePointer) {
        if (rawInodeReadBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        Size indexInBlock = currentWritePointer - flooredWritePointer, frontByteNum = umin64(BLOCK_SIZE - indexInBlock, remainByteToWrite);
        memcpy(tmpBuffer + indexInBlock, currentBuffer, frontByteNum);
        if (rawInodeWriteBlocks(iNode, tmpBuffer, currentWritePointer / BLOCK_SIZE, 1) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        currentBuffer += frontByteNum;
        currentWritePointer += frontByteNum;
        remainByteToWrite -= frontByteNum;
    }

    Size midBlockNum = remainByteToWrite / BLOCK_SIZE;
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

        Size tailByteNum = umin64(remainByteToWrite, BLOCK_SIZE);
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