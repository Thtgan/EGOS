#include<algorithms.h>
#include<blowup.h>
#include<devices/block/blockDevice.h>
#include<devices/block/imageDevice.h>
#include<error.h>
#include<fs/directory.h>
#include<fs/fileSystem.h>
#include<fs/fsutil.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<injector.h>
#include<kit/types.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>

typedef enum {
    ADD_FILE,
    ADD_DIRECTORY,
    LIST,
    REMOVE_FILE,
    REMOVE_DIRECTORY,
    DEPLOY
} Mode;

int errorCode = 0;

static int __addFile(ConstCstring path, ConstCstring filePath);

static int __addDirectory(ConstCstring path, ConstCstring directoryName);

static int __list(ConstCstring path);

static int __removeFile(ConstCstring path);

static int __removeDirectory(ConstCstring path);

//Yes, I know, THIS IS TOTALLY A MESS, but whatever, as long as it works

int main(int argc, char** argv) {
    char option;
    char* imgFile = NULL, * injectFile = NULL, * path = NULL, * modeStr = NULL;
    size_t base = 0, size = -1;
    while ((option = getopt(argc, argv, "i:f:p:m:b:s:")) != -1) {
        switch (option) {
            case 'i':
                imgFile = optarg;
                break;
            case 'f':
                injectFile = optarg;
                break;
            case 'p':
                path = optarg;
                break;
            case 'm':
                modeStr = optarg;
                break;
            case 'b':
                base = (size_t)strtol(optarg, NULL, 0);
                break;
            case 's':
                size = (size_t)strtol(optarg, NULL, 0);
                break;
            default:
                blowup("Unknown option found");
        }
    }

    Mode mode = ADD_FILE;
    if (modeStr != NULL) {
        printf("Command: %s\n", modeStr);
        do {
            if (strcmp(modeStr, "addFile") == 0) {
                mode = ADD_FILE;
                break;
            }

            if (strcmp(modeStr, "addDirectory") == 0) {
                mode = ADD_DIRECTORY;
                break;
            }

            if (strcmp(modeStr, "list") == 0) {
                mode = LIST;
                break;
            }

            if (strcmp(modeStr, "removeFile") == 0) {
                mode = REMOVE_FILE;
                break;
            }

            if (strcmp(modeStr, "removeDirectory") == 0) {
                mode = REMOVE_DIRECTORY;
                break;
            }

            if (strcmp(modeStr, "deploy") == 0) {
                mode = DEPLOY;
                break;
            }

            blowup("Unknown mode found");
        } while (false);
    }

    if (imgFile == NULL) {
        blowup("Image name must be given");
    }

    if (mode != DEPLOY && path == NULL) {
        blowup("Non-deploy command must give the path for operation");
    }

    if ((mode == ADD_FILE || mode == ADD_DIRECTORY) && injectFile == NULL) {
        blowup("Add file/directory must have object to inject");
    }

    initBlockDeviceManager();

    BlockDevice* device = createImageDevice(imgFile, "hda", base, size);
    if (device == NULL) {
        blowup("Device create failed");
    }
    registerBlockDevice(device);
    printf("Device %s: %lu blocks contained\n", device->name, device->availableBlockNum);

    if (mode == DEPLOY) {
        if (deployFileSystem(device, FILE_SYSTEM_TYPE_PHOSPHERUS) == -1) {
            printf("Return value: %#010X\n", errorCode);
            blowup("Deploy failed");
        }
        return 0;
    }

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);
    if (checkFileSystem(device) != FILE_SYSTEM_TYPE_PHOSPHERUS) {
        blowup("Disk has no phospherus file system");
    }
    
    if (rootFileSystem == NULL) {
        blowup("Couldn't open phospherus file system");
    }

    switch(mode) {
        case ADD_FILE:
            if (__addFile(path, injectFile) == -1) {
                printf("Return value: %#010X\n", errorCode);
                blowup("Add file failed");
            }
            break;
        case ADD_DIRECTORY:
            if (__addDirectory(path, injectFile) == -1) {
                printf("Return value: %#010X\n", errorCode);
                blowup("Add directory failed");
            }
            break;
        case LIST:
            if (__list(path) == -1) {
                printf("Return value: %#010X\n", errorCode);
                blowup("List failed");
            }
            break;
        case REMOVE_FILE:
            if (__removeFile(path) == -1) {
                printf("Return value: %#010X\n", errorCode);
                blowup("Remove file failed");
            }
            break;
        case REMOVE_DIRECTORY:
            if (__removeDirectory(path) == -1) {
                printf("Return value: %#010X\n", errorCode);
                blowup("Remove directory failed");
            }
            break;
        default:
            blowup("Unknown operation");
    }

    closeFileSystem(rootFileSystem);
    deleteImageDevice(device);
    printf("Done\n");
}

#define TRANSFER_BUFFER_SIZE (1llu << 20)

static int __addFile(ConstCstring path, ConstCstring filePath) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;
    FILE* file = fopen(filePath, "rb");
    do {
        if (file == NULL) {
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
            ret = -1;
            break;
        }
        
        ConstCstring fileName = strrchr(filePath, '/');
        if (fileName == NULL) {
            fileName = filePath;
        } else {
            ++fileName;
        }

        if (directoryLookupEntry(directory, fileName, INODE_TYPE_FILE) != INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_ALREADY_EXIST);
            ret = -1;
            break;
        }

        printf("Inserting file %s\n", fileName);

        Index64 newFileInodeIndex = iNodeCreate(rootFileSystem->device, INODE_TYPE_FILE);
        if (newFileInodeIndex == INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            ret = -1;
            break;
        }

        ID iNodeID = BUILD_INODE_ID(rootFileSystem->device, newFileInodeIndex);
        iNode* fileInode = iNodeOpen(iNodeID);
        File* fileInside = fileOpen(fileInode);

        fseek(file, 0L, SEEK_END);
        size_t remainToWrite = ftell(file), currentPointer = 0;

        void* buffer = malloc(TRANSFER_BUFFER_SIZE);
        while (remainToWrite > 0) {
            size_t batchSize = umin64(remainToWrite, TRANSFER_BUFFER_SIZE);
            fseek(file, currentPointer, SEEK_SET);
            fileSeek(fileInside, currentPointer);

            if (fread(buffer, 1, batchSize, file) != batchSize) {
                SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
                ret = -1;
                break;
            }

            fileWrite(fileInside, buffer, batchSize);
            remainToWrite -= batchSize;
            currentPointer += batchSize;
        }
        free(buffer);
        
        fileClose(fileInside);
        iNodeClose(fileInode);
        if (ret == -1 || directoryAddEntry(directory, iNodeID, INODE_TYPE_FILE, fileName) == -1) {
            iNodeDelete(iNodeID);
            ret = -1;
        }
    } while (0);
    fclose(file);

    directoryClose(directory);
    iNodeClose(directoryInode);
    
    return ret;
}

static int __addDirectory(ConstCstring path, ConstCstring directoryName) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;
    do {
        if (directoryLookupEntry(directory, directoryName, INODE_TYPE_DIRECTORY) != INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_ALREADY_EXIST);
            ret = -1;
            break;
        }

        printf("Inserting directory %s\n", directoryName);

        Index64 newDirectoryInodeIndex = iNodeCreate(rootFileSystem->device, INODE_TYPE_DIRECTORY);
        if (newDirectoryInodeIndex == INVALID_INDEX) {
            SET_ERROR_CODE(ERROR_OBJECT_EXECUTION, ERROR_STATUS_OPERATION_FAIL);
            ret = -1;
            break;
        }

        ID iNodeID = BUILD_INODE_ID(rootFileSystem->device, newDirectoryInodeIndex);
        if (directoryAddEntry(directory, iNodeID, INODE_TYPE_DIRECTORY, directoryName) == -1) {
            iNodeDelete(iNodeID);
            ret = -1;
        }
    } while (0);

    directoryClose(directory);
    iNodeClose(directoryInode);

    return ret;
}

static int __list(ConstCstring path) {
    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    printf("%lu items found in %s:\n", directory->size, path);
    for (int i = 0; i < directory->size; ++i) {
        if (directoryReadEntry(directory, &entry, i) == -1) {
            return -1;
        }

        if (strlen(entry.name) > 61) {
            printf("%61s...  ", entry.name);
        } else {
            printf("%-64s  ", entry.name);
        }

        if (entry.type == INODE_TYPE_DIRECTORY) {
            printf("Directory\n");
        } else {
            printf("File\n");
        }
    }

    directoryClose(directory);
    iNodeClose(directoryInode);

    return 0;
}

static int __removeFile(ConstCstring path) {
    size_t len = strlen(path);
    Cstring copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/');
    ConstCstring dir = NULL, fileName = NULL;
    *(sep + (sep == copy)) = '\0';
    dir = copy;
    fileName = path + (sep - copy) + 1;

    DirectoryEntry entry;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;
    Index64 entryIndex = directoryLookupEntry(directory, fileName, INODE_TYPE_FILE);

    if (entryIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_FILE, ERROR_STATUS_NOT_FOUND);
        ret = -1;
    } else {
        directoryReadEntry(directory, &entry, entryIndex);
        directoryRemoveEntry(directory, entryIndex);
        iNodeDelete(entry.iNodeID);
    }

    directoryClose(directory);
    iNodeClose(directoryInode);
    free(copy);

    return ret;
}

static int __removeDirectory(ConstCstring path) {
    size_t len = strlen(path);
    char* copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/');
    ConstCstring dir = NULL, directoryName = NULL;
    *(sep + (sep == copy)) = '\0';
    dir = copy;
    directoryName = path + (sep - copy) + 1;
    
    DirectoryEntry entry;
    int res = 0;
    if (tracePath(&entry, path, INODE_TYPE_DIRECTORY) == -1) {
        return -1;
    }

    iNode* directoryInode = iNodeOpen(entry.iNodeID);
    Directory* directory = directoryOpen(directoryInode);

    int ret = 0;

    Index64 entryIndex = directoryLookupEntry(directory, directoryName, INODE_TYPE_DIRECTORY);
    if (entryIndex == INVALID_INDEX) {
        SET_ERROR_CODE(ERROR_OBJECT_ITEM, ERROR_STATUS_NOT_FOUND);
        ret = -1;
    } else {
        directoryReadEntry(directory, &entry, entryIndex);
        directoryRemoveEntry(directory, entryIndex);
        iNodeDelete(entry.iNodeID);
    }

    directoryClose(directory);
    iNodeClose(directoryInode);
    free(copy);

    return ret;
}