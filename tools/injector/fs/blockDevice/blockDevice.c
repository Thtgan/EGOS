#include<fs/blockDevice/blockDevice.h>

#include<algorithms.h>
#include<malloc.h>
#include<memory.h>
#include<stddef.h>
#include<stdio.h>
#include<string.h>
#include<blowup.h>

static void __readBlocks(void* additionalData, block_index_t blockIndex, void* buffer, size_t n);
static void __writeBlocks(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n);

typedef struct {
    FILE* file;
    size_t base;
} __AdditionalData;

BlockDevice* createBlockDevice(const char* filePath, const char* deviceName, size_t base, size_t size) {
    FILE* file = fopen(filePath, "rb+");
    if (file == NULL) {
        return NULL;
    }

    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file) / BLOCK_SIZE;
    if (fileSize <= base) {
        fclose(file);
        return NULL;
    }

    BlockDevice* ret = malloc(sizeof(BlockDevice));
    strcpy(ret->name, deviceName);
    __AdditionalData* additionalData = malloc(sizeof(__AdditionalData));
    additionalData->file = file;
    additionalData->base = base;
    ret->additionalData = additionalData;

    ret->availableBlockNum = umin64(fileSize - base, size);
    ret->readBlocks = __readBlocks;
    ret->writeBlocks = __writeBlocks;

    return ret;
}

void deleteBlockDevice(BlockDevice* device) {
    __AdditionalData* additional = device->additionalData;
    fclose(additional->file);
    free(additional);
    free(device);
}

static void __readBlocks(void* additionalData, block_index_t blockIndex, void* buffer, size_t n) {
    __AdditionalData* additional = additionalData;
    FILE* file = additional->file;
    fseek(file, (additional->base + blockIndex) * BLOCK_SIZE, SEEK_SET);
    if (fread(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Read failed");
    }
}

static void __writeBlocks(void* additionalData, block_index_t blockIndex, const void* buffer, size_t n) {
    __AdditionalData* additional = additionalData;
    FILE* file = additional->file;
    fseek(file, (additional->base + blockIndex) * BLOCK_SIZE, SEEK_SET);
    if (fwrite(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Write failed");
    }
}