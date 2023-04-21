#include<devices/virtualDevice.h>

#include<devices/block/blockDevice.h>
#include<devices/block/memoryBlockDevice.h>
#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<error.h>
#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<kit/types.h>
#include<memory/buffer.h>
#include<memory/memory.h>
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
    void* region = pageAlloc(__MEMORY_DEVICE_SIZE / PAGE_SIZE, PHYSICAL_PAGE_TYPE_PUBLIC);
    BlockDevice* memDevice = createMemoryBlockDevice(region, __MEMORY_DEVICE_SIZE, __MEMORY_DEVICE_NAME);
    registerBlockDevice(memDevice);
    if (deployFileSystem(memDevice, FILE_SYSTEM_TYPE_PHOSPHERUS) == -1) {
        return -1;
    }

    FileSystem* fs = openFileSystem(memDevice);

    DirectoryEntry entry;
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) != -1) {
        deleteEntry(__DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY);
    }
    
    createEntry(__DEVICE_DIR_PARENT, __DEVICE_DIR_NAME, BUILD_INODE_ID(fs->device, fs->rootDirectoryInodeIndex), INODE_TYPE_DIRECTORY);
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    if (registerVirtualDevice(getLevelTerminal(TERMINAL_LEVEL_OUTPUT), "tty", initTerminalDevice()) == -1) {
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

int registerVirtualDevice(void* device, ConstCstring name, FileOperations* operations) {
    DirectoryEntry entry;
    if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    BlockDevice* memDevice = getBlockDeviceByID(INODE_ID_GET_DEVICE_ID(entry.iNodeID));
    if (memDevice == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
        return -1;
    }

    FileSystem* fs = openFileSystem(memDevice);
    if (fs == NULL) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return -1;
    }

    Index64 iNodeIndex = fs->opearations->iNodeGlobalOperations->createInode(fs, INODE_TYPE_DEVICE);
    if (iNodeIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
        return -1;
    }

    RecordOnDevice* onDevice = allocateBuffer(BUFFER_SIZE_512);
    
    VirtualDeviceINodeData* data = (VirtualDeviceINodeData*)onDevice->data;
    data->fileOperations = operations;
    data->device = device;

    blockDeviceWriteBlocks(memDevice, iNodeIndex, onDevice, 1);

    releaseBuffer(onDevice, BUFFER_SIZE_512);

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    ID iNodeID = BUILD_INODE_ID(memDevice->deviceID, iNodeIndex);

    int ret = 0;
    do {
        if (directoryLookupEntry(directory, name, INODE_TYPE_DEVICE) != INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
            ret = -1;
            break;
        }

        if (directoryAddEntry(directory, iNodeID, INODE_TYPE_DEVICE, name) == -1) {
            iNodeDelete(iNodeID);
            ret = -1;
        }
    } while (0);

    directoryClose(directory);
    iNodeClose(directoryInode);

    return ret;
}