#include<devices/block/imageDevice.h>

#include<algorithms.h>
#include<blowup.h>
#include<devices/block/blockDevice.h>
#include<devices/block/blockDeviceTypes.h>
#include<kit/types.h>
#include<malloc.h>
#include<memory.h>
#include<stdio.h>
#include<string.h>

static int __readBlocks(THIS_ARG_APPEND(BlockDevice, Index64 blockIndex, void* buffer, size_t n));
static int __writeBlocks(THIS_ARG_APPEND(BlockDevice, Index64 blockIndex, const void* buffer, size_t n));

static BlockDeviceOperation operations = {
    .readBlocks = __readBlocks,
    .writeBlocks = __writeBlocks
};

typedef struct {
    FILE* file;
    size_t base;
} __AdditionalData;

BlockDevice* createImageDevice(const char* filePath, const char* deviceName, size_t base, size_t size) {
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
    
    __AdditionalData* additionalData = malloc(sizeof(__AdditionalData));
    additionalData->file = file;
    additionalData->base = base;

    return createBlockDevice(deviceName, HDD, umin64(fileSize - base, size), &operations, (Object)additionalData);
}

void deleteImageDevice(BlockDevice* device) {
    __AdditionalData* additional = (__AdditionalData*)device->additionalData;
    fclose(additional->file);
    free(additional);
    free(device);
}

static int __readBlocks(THIS_ARG_APPEND(BlockDevice, Index64 blockIndex, void* buffer, size_t n)) {
    __AdditionalData* additional = (__AdditionalData*)this->additionalData;
    FILE* file = additional->file;
    fseek(file, (additional->base + blockIndex) * BLOCK_SIZE, SEEK_SET);
    if (fread(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Read failed");
    }

    return 0;
}

static int __writeBlocks(THIS_ARG_APPEND(BlockDevice, Index64 blockIndex, const void* buffer, size_t n)) {
    __AdditionalData* additional = (__AdditionalData*)this->additionalData;
    FILE* file = additional->file;
    fseek(file, (additional->base + blockIndex) * BLOCK_SIZE, SEEK_SET);
    if (fwrite(buffer, BLOCK_SIZE, n, file) != n) {
        blowup("Write failed");
    }

    return 0;
}