#include<devices/virtualDevice.h>

#include<devices/block/blockDevice.h>
#include<devices/block/memoryBlockDevice.h>
#include<error.h>
#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<memory/pageAlloc.h>
#include<memory/physicalPages.h>
#include<system/address.h>
#include<system/pageTable.h>

#define __DEVICE_DIR_PARENT     "/"
#define __DEVICE_DIR_NAME       "dev"
#define __DEVICE_DIR_PATH       __DEVICE_DIR_PARENT __DEVICE_DIR_NAME
#define __MEMORY_DEVICE_SIZE    4 * DATA_UNIT_MB
#define __MEMORY_DEVICE_NAME    "Virtual devices"

int initVirtualDevices() {
    DirectoryEntry entry;
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) != -1) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
        return -1;
    }

    void* region = pageAlloc(__MEMORY_DEVICE_SIZE / PAGE_SIZE, PHYSICAL_PAGE_TYPE_PUBLIC);
    BlockDevice* memDevice = createMemoryBlockDevice(region, __MEMORY_DEVICE_SIZE, __MEMORY_DEVICE_NAME);
    registerBlockDevice(memDevice);
    if (deployFileSystem(memDevice, FILE_SYSTEM_TYPE_PHOSPHERUS) == -1) {
        return -1;
    }

    FileSystem* fs = openFileSystem(memDevice);
    createEntry(__DEVICE_DIR_PARENT, __DEVICE_DIR_NAME, BUILD_INODE_ID(fs->device, fs->rootDirectoryInodeIndex), INODE_TYPE_DIRECTORY);
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    return 0;
}

int closeVitualDevices() {
    DirectoryEntry entry;
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    if (deleteEntry(__DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    BlockDevice* memDevice = getBlockDeviceByID(INODE_ID_GET_DEVICE_ID(entry.iNodeID));
    closeFileSystem(openFileSystem(memDevice));

    if (unregisterBlockDevice(memDevice->deviceID) == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    void* region = (void*)memDevice->additionalData;
    releaseBlockDevice(memDevice);

    pageFree(region, __MEMORY_DEVICE_SIZE / PAGE_SIZE);

    return 0;
}