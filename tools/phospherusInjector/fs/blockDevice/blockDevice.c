#include<fs/blockDevice/blockDevice.h>

#include<malloc.h>
#include<memory.h>
#include<stdio.h>
#include<string.h>
#include<blowup.h>

static void __readBlocks(void* additionalData, block_index_t blockIndex, void* buffer, size_t n);
static void __writeBlocks(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n);

BlockDevice* createBlockDevice(const char* filePath, const char* deviceName) {
    FILE* file = fopen(filePath, "rb+");
    if (file == NULL) {
        return NULL;
    }

    BlockDevice* ret = malloc(sizeof(BlockDevice));
    strcpy(ret->name, deviceName);
    ret->additionalData = file;

    fseek(file, 0L, SEEK_END);
    ret->availableBlockNum = ftell(file) / BLOCK_SIZE;
    ret->readBlocks = __readBlocks;
    ret->writeBlocks = __writeBlocks;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    fclose(device->additionalData);
    free(device);
}

static void __readBlocks(void* additionalData, block_index_t blockIndex, void* buffer, size_t n) {
    FILE* file = additionalData;
    fseek(file, blockIndex * BLOCK_SIZE, SEEK_SET);
    if (fread(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Read failed");
    }
}

static void __writeBlocks(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n) {
    FILE* file = additionalData;
    fseek(file, blockIndex * BLOCK_SIZE, SEEK_SET);
    if (fwrite(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Write failed");
    }
}