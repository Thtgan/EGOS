// #include<devices/virtualDevice.h>

// // #include<devices/block/blockDevice.h>
// // #include<devices/block/memoryBlockDevice.h>
// // #include<devices/terminal/terminal.h>
// // #include<devices/terminal/terminalSwitch.h>
// // #include<error.h>
// // #include<fs/directory.h>
// // #include<fs/fileSystem.h>
// // #include<fs/fsutil.h>
// // #include<fs/inode.h>
// #include<kit/types.h>
// // #include<memory/buffer.h>
// // #include<memory/memory.h>
// // #include<memory/physicalPages.h>
// // #include<system/pageTable.h>

// #define __DEVICE_DIR_PARENT     "/"
// #define __DEVICE_DIR_NAME       "dev"
// #define __DEVICE_DIR_PATH       __DEVICE_DIR_PARENT __DEVICE_DIR_NAME
// #define __MEMORY_DEVICE_SIZE    4 * DATA_UNIT_MB
// #define __MEMORY_DEVICE_NAME    "Virtual devices"

// // static File _standardOutputFile;

// // static Result __doRegisterVirtualDevice(void* device, ConstCstring name, FileOperations* operations, void** onDevicePtr, iNode** iNodePtr, Directory** directoryPtr);

// // static Result __openStandardOutput();

// // static Result __doOpenStandardOutput(DirectoryEntry* entry, iNode** iNodePtr);

// Result initVirtualDevices() {
//     // void* region = pageAlloc(__MEMORY_DEVICE_SIZE / PAGE_SIZE, MEMORY_TYPE_PUBLIC);
//     // BlockDevice* memDevice = createMemoryBlockDevice(region, __MEMORY_DEVICE_SIZE, __MEMORY_DEVICE_NAME);
//     // if (memDevice == NULL) {
//     //     return RESULT_FAIL;
//     // }

//     // if (registerBlockDevice(memDevice) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // if (deployFileSystem(memDevice, FILE_SYSTEM_TYPE_PHOSPHERUS) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // DirectoryEntry entry;
//     // if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) != RESULT_FAIL) {
//     //     deleteEntry(__DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY);
//     // }
    
//     // FS* fs = fs_open(memDevice);
//     // if (fs == NULL || createEntry(__DEVICE_DIR_PARENT, __DEVICE_DIR_NAME, BUILD_INODE_ID(fs->device, fs->rootDirectoryInodeIndex), INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // if (registerVirtualDevice(getLevelTerminal(TERMINAL_LEVEL_OUTPUT), "tty", initTerminalDevice()) == RESULT_FAIL || __openStandardOutput() == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // return RESULT_SUCCESS;
//     return RESULT_FAIL;
// }

// Result closeVitualDevices() {
//     // DirectoryEntry entry;
//     // if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // if (deleteEntry(__DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // BlockDevice* memDevice = getBlockDeviceByID(INODE_ID_GET_DEVICE_ID(entry.iNodeID));
//     // if (memDevice == NULL) {
//     //     return RESULT_FAIL;
//     // }

//     // FS* fs = fs_open(memDevice);
//     // if (fs == NULL || fs_close(fs) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // if (unregisterBlockDevice(memDevice->deviceID) == NULL) {
//     //     SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
//     //     return RESULT_FAIL;
//     // }

//     // void* region = (void*)memDevice->handle;
//     // releaseBlockDevice(memDevice);
//     // pageFree(region);

//     // return RESULT_SUCCESS;
//     return RESULT_FAIL;
// }

// Result registerVirtualDevice(void* device, ConstCstring name, FileOperations* operations) {
//     // void* onDevice = NULL;
//     // iNode* iNode = NULL;
//     // Directory* directory = NULL;

//     // Result ret = __doRegisterVirtualDevice(device, name, operations, &onDevice, &iNode, &directory);

//     // if (directory != NULL) {
//     //     if (rawDirectoryClose(directory) == RESULT_FAIL) {
//     //         return RESULT_FAIL;
//     //     }
//     // }

//     // if (iNode != NULL) {
//     //     if (rawInodeClose(iNode) == RESULT_FAIL) {
//     //         return RESULT_FAIL;
//     //     }
//     // }
 
//     // if (onDevice != NULL) {
//     //     releaseBuffer(onDevice, BUFFER_SIZE_512);
//     // }

//     // return ret;
//     return RESULT_FAIL;
// }

// // File* getStandardOutputFile() {
// //     return &_standardOutputFile;
// // }

// static Result __doRegisterVirtualDevice(void* device, ConstCstring name, FileOperations* operations, void** onDevicePtr, iNode** iNodePtr, Directory** directoryPtr) {
//     // DirectoryEntry entry;
//     // if (tracePath(&entry, __DEVICE_DIR_PATH, INODE_TYPE_DIRECTORY) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // BlockDevice* memDevice = getBlockDeviceByID(INODE_ID_GET_DEVICE_ID(entry.iNodeID));
//     // if (memDevice == NULL) {
//     //     SET_ERROR_CODE(ERROR_OBJECT_DEVICE, ERROR_STATUS_NOT_FOUND);
//     //     return RESULT_FAIL;
//     // }

//     // FS* fs = fs_open(memDevice);
//     // if (fs == NULL) {
//     //     SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
//     //     return RESULT_FAIL;
//     // }

//     // Index64 iNodeIndex = fs->opearations->iNodeGlobalOperations->createInode(fs, INODE_TYPE_DEVICE);
//     // if (iNodeIndex == INVALID_INDEX) {
//     //     SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
//     //     return RESULT_FAIL;
//     // }

//     // RecordOnDevice* onDevice = NULL;
//     // *onDevicePtr = onDevice = allocateBuffer(BUFFER_SIZE_512);
//     // if (onDevice == NULL) {
//     //     return RESULT_FAIL;
//     // }
    
//     // VirtualDeviceINodeData* data = (VirtualDeviceINodeData*)onDevice->data;
//     // data->fileOperations = operations;
//     // data->device = device;

//     // if (blockDeviceWriteBlocks(memDevice, iNodeIndex, onDevice, 1) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // iNode* iNode = NULL;
//     // *iNodePtr = iNode = rawInodeOpen(entry.iNodeID);
//     // if (iNode == NULL) {
//     //     return RESULT_FAIL;
//     // }

//     // Directory* directory = NULL;
//     // *directoryPtr = directory = rawDirectoryOpen(iNode);
//     // if (directory == NULL) {
//     //     return RESULT_FAIL;
//     // }

//     // ID iNodeID = BUILD_INODE_ID(memDevice->deviceID, iNodeIndex);
//     // if (rawDirectoryLookupEntry(directory, name, INODE_TYPE_DEVICE) != INVALID_INDEX) {
//     //     return RESULT_FAIL;
//     // }

//     // if (rawDirectoryAddEntry(directory, iNodeID, INODE_TYPE_DEVICE, name) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // return RESULT_SUCCESS;
//     return RESULT_FAIL;
// }

// static Result __openStandardOutput() {
//     // DirectoryEntry entry;
//     // if (tracePath(&entry, __DEVICE_DIR_PATH "/tty", INODE_TYPE_DEVICE) == RESULT_FAIL) {
//     //     return RESULT_FAIL;
//     // }

//     // iNode* iNode = NULL;

//     // Result res = __doOpenStandardOutput(&entry, &iNode);

//     // if (res == RESULT_FAIL) {
//     //     if (iNode != NULL) {
//     //         if (rawInodeClose(iNode) == RESULT_FAIL) {
//     //             return RESULT_FAIL;
//     //         }
//     //     }

//     //     return RESULT_FAIL;
//     // }

//     // return RESULT_SUCCESS;
//     return RESULT_FAIL;
// }

// static Result __doOpenStandardOutput(DirectoryEntry* entry, iNode** iNodePtr) {
//     // iNode* iNode = NULL;
//     // *iNodePtr = iNode = rawInodeOpen(entry->iNodeID);
//     // if (iNode == NULL) {
//     //     return RESULT_FAIL;
//     // }

//     // _standardOutputFile.iNode = iNode;
//     // memset(&_standardOutputFile.operations, 0, sizeof(FileOperations));

//     // FileOperations* operations = ((VirtualDeviceINodeData*)iNode->onDevice.data)->fileOperations;
//     // _standardOutputFile.operations.write = operations->write;
//     // _standardOutputFile.pointer = 0;

//     // return RESULT_SUCCESS;
//     return RESULT_FAIL;
// }