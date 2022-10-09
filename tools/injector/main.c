#include<algorithms.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<blowup.h>
#include<devices/block/blockDevice.h>
#include<devices/block/imageDevice.h>
#include<fs/fileSystem.h>
#include<fs/inode.h>
#include<fs/phospherus/phospherus.h>
#include<kit/types.h>

typedef enum {
    ADD_FILE,
    ADD_DIRECTORY,
    LIST,
    REMOVE_FILE,
    REMOVE_DIRECTORY,
    DEPLOY
} Mode;

static Directory* __navigateDirectory(FileSystem* fs, Directory* rootDir, char* path);

static bool __addFile(FileSystem* fs, Directory* rootDir, char* path, char* filePath);

static bool __addDirectory(FileSystem* fs, Directory* rootDir, char* path, char* directoryName);

static bool __list(FileSystem* fs, Directory* rootDir, char* path);

static bool __removeFile(FileSystem* fs, Directory* rootDir, char* path);

static bool __removeDirectory(FileSystem* fs, Directory* rootDir, char* path);

//Yes, I know, THIS IS TOTALLY A MESS, but whatever, as long as it works

int main(int argc, char** argv) {
    char option;
    char* imgFile = NULL, * injectFile = NULL, * path = NULL, * deviceName = NULL, * modeStr = NULL;
    size_t base = 0, size = -1;
    while ((option = getopt(argc, argv, "i:f:p:d:m:b:s:")) != -1) {
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
            case 'd':
                deviceName = optarg;
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

    if (imgFile == NULL || deviceName == NULL) {
        blowup("Image name and device name must be given");
    }

    if (mode != DEPLOY && path == NULL) {
        blowup("Non-deploy command must give the path for operation");
    }

    if ((mode == ADD_FILE || mode == ADD_DIRECTORY) && injectFile == NULL) {
        blowup("Add file/directory must have object to inject");
    }

    initBlockDeviceManager();

    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);
    BlockDevice* device = createImageDevice(imgFile, deviceName, base, size);
    if (device == NULL) {
        blowup("Device create failed");
    }
    registerBlockDevice(device);
    printf("Device %s: %lu blocks contained\n", device->name, device->availableBlockNum);

    if (mode == DEPLOY) {
        if (!deployFileSystem(device, FILE_SYSTEM_TYPE_PHOSPHERUS)) {
            blowup("Deploy failed");
        }
        return 0;
    }

    if (checkFileSystem(device) != FILE_SYSTEM_TYPE_PHOSPHERUS) {
        blowup("Disk has no phospherus file system");
    }
    FileSystem* fs = openFileSystem(device, FILE_SYSTEM_TYPE_PHOSPHERUS);
    if (fs == NULL) {
        blowup("Couldn't open phospherus file system");
    }

    iNode* rootDirInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, fs->rootDirectoryInode);
    Directory* rootDir = fs->opearations->directoryGlobalOperations->openDirectory(rootDirInode);

    switch(mode) {
        case ADD_FILE:
            if (!__addFile(fs, rootDir, path, injectFile)) {
                blowup("Add file failed");
            }
            break;
        case ADD_DIRECTORY:
            if (!__addDirectory(fs, rootDir, path, injectFile)) {
                blowup("Add directory failed");
            }
            break;
        case LIST:
            if (!__list(fs, rootDir, path)) {
                blowup("List failed");
            }
            break;
        case REMOVE_FILE:
            if (!__removeFile(fs, rootDir, path)) {
                blowup("Remove file failed");
            }
            break;
        case REMOVE_DIRECTORY:
            if (!__removeDirectory(fs, rootDir, path)) {
                blowup("Remove directory failed");
            }
            break;
        default:
            blowup("Unknown operation");
    }

    fs->opearations->directoryGlobalOperations->closeDirectory(rootDir);
    fs->opearations->iNodeGlobalOperations->closeInode(rootDirInode);

    closeFileSystem(fs);
    deleteImageDevice(device);
    printf("Done\n");
}

const char* sep = "/";
static Directory* __navigateDirectory(FileSystem* fs, Directory* rootDir, char* path) {
    if (path[0] == '\0' || strcmp(path, "/") == 0) {
        return rootDir;
    }

    if (path[0] != '/') {
        return NULL;
    }

    Directory* currentDir = rootDir;
    const char* next = strtok(path + 1, sep);
    while (next != NULL) {
        Index64 entryIndex = currentDir->operations->lookupEntry(currentDir, next, INODE_TYPE_DIRECTORY);
        if (entryIndex == PHOSPHERUS_NULL) {
            return NULL;
        }
        Directory* copy = currentDir;

        DirectoryEntry* entry = currentDir->operations->getEntry(currentDir, entryIndex);
        iNode* nextDirectoryInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, entry->iNodeIndex);
        free(entry);

        currentDir = fs->opearations->directoryGlobalOperations->openDirectory(nextDirectoryInode);

        if (copy != rootDir) {
            iNode* iNodeToClose = copy->iNode;
            fs->opearations->directoryGlobalOperations->closeDirectory(copy);
            fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
        }

        next = strtok(NULL, sep);
    }

    return currentDir;
}

#define TRANSFER_BUFFER_SIZE (1llu << 20)

static bool __addFile(FileSystem* fs, Directory* rootDir, char* path, char* filePath) {
    Directory* directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL) {
        return false;
    }

    bool ret = true;
    do {
        FILE* file = fopen(filePath, "rb");
        if (file == NULL) {
            ret = false;
            break;
        }
        
        char* fileName = strrchr(filePath, '/');
        if (fileName == NULL) {
            fileName = filePath;
        } else {
            *fileName = '\0';
            ++fileName;
        }
        
        if (directory->operations->lookupEntry(directory, fileName, INODE_TYPE_FILE) != PHOSPHERUS_NULL) {
            printf("File already existed\n");
            fclose(file);

            ret = false;
            break;
        }

        printf("Inserting file %s\n", fileName);

        Index64 newFileInodeIndex = fs->opearations->iNodeGlobalOperations->createInode(fs->device, INODE_TYPE_FILE);
        if (newFileInodeIndex == PHOSPHERUS_NULL) {
            fclose(file);

            ret = false;
            break;
        }

        void* buffer = malloc(TRANSFER_BUFFER_SIZE);
        
        iNode* fileInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, newFileInodeIndex);
        File* fileInside = fs->opearations->fileGlobalOperations->openFile(fileInode);

        fseek(file, 0L, SEEK_END);
        size_t fileSize = ftell(file), currentPointer = 0;
        while (fileSize > 0) {
            size_t batchSize = umin64(fileSize, TRANSFER_BUFFER_SIZE);
            fseek(file, currentPointer, SEEK_SET);
            fileInside->operations->seek(fileInside, currentPointer);

            if (fread(buffer, batchSize, 1, file) != 1) {
                fclose(file);
                free(buffer);
                fs->opearations->iNodeGlobalOperations->deleteInode(fs->device, newFileInodeIndex);

                ret = false;
                break;
            }

            fileInside->operations->write(fileInside, buffer, batchSize);
            fileSize -= batchSize;
            currentPointer += batchSize;
        }

        if (!ret) {
            break;
        }

        free(buffer);
        fclose(file);
        
        if (directory->operations->addEntry(directory, fileInode, fileName) == -1) {
            fs->opearations->iNodeGlobalOperations->deleteInode(fs->device, newFileInodeIndex);
            ret = false;
        }

        fs->opearations->fileGlobalOperations->closeFile(fileInside);
        fs->opearations->iNodeGlobalOperations->closeInode(fileInode);
    } while (0);

    if (directory != rootDir) {
        iNode* iNodeToClose = directory->iNode;
        fs->opearations->directoryGlobalOperations->closeDirectory(directory);
        fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
    }
    
    return ret;
}

static bool __addDirectory(FileSystem* fs, Directory* rootDir, char* path, char* directoryName) {
    Directory* directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL || strchr(directoryName, '/') != NULL) {
        return false;
    }

    bool ret = true;
    do {
        if (directory->operations->lookupEntry(directory, directoryName, INODE_TYPE_DIRECTORY) != PHOSPHERUS_NULL) {
            printf("Directory already existed\n");
            ret = false;
            break;
        }

        printf("Inserting directory %s\n", directoryName);

        Index64 newDirectoryInodeIndex = fs->opearations->iNodeGlobalOperations->createInode(fs->device, INODE_TYPE_DIRECTORY);
        if (newDirectoryInodeIndex == PHOSPHERUS_NULL) {
            ret = false;
            break;
        }

        iNode* directoryInode = fs->opearations->iNodeGlobalOperations->openInode(fs->device, newDirectoryInodeIndex);

        if (directory->operations->addEntry(directory, directoryInode, directoryName) == -1) {
            fs->opearations->iNodeGlobalOperations->deleteInode(fs->device, newDirectoryInodeIndex);
            ret = false;
        }

        fs->opearations->iNodeGlobalOperations->closeInode(directoryInode);

    } while (0);

    if (directory != rootDir) {
        iNode* iNodeToClose = directory->iNode;
        fs->opearations->directoryGlobalOperations->closeDirectory(directory);
        fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
    }

    return ret;
}

static bool __list(FileSystem* fs, Directory* rootDir, char* path) {
    Directory* directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL) {
        return false;
    }

    printf("%lu items found in %s:\n", directory->size, path);
    for (int i = 0; i < directory->size; ++i) {
        DirectoryEntry* entry = directory->operations->getEntry(directory, i);
        if (strlen(entry->name) > 61) {
            printf("%61s...  ", entry->name);
        } else {
            printf("%-64s  ", entry->name);
        }

        if (entry->type == INODE_TYPE_DIRECTORY) {
            printf("Directory\n");
        } else {
            printf("File\n");
        }

        free(entry);
    }

    if (directory != rootDir) {
        iNode* iNodeToClose = directory->iNode;
        fs->opearations->directoryGlobalOperations->closeDirectory(directory);
        fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
    }

    return true;
}

static bool __removeFile(FileSystem* fs, Directory* rootDir, char* path) {
    size_t len = strlen(path);
    char* copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/'), * fileName = NULL;
    Directory* directory = NULL;
    if (sep != NULL) {
        *sep = '\0';
        directory = __navigateDirectory(fs, rootDir, copy);
        fileName = sep + 1;
    } else {
        directory = rootDir;
        fileName = copy;
    }

    bool ret = true;

    do {
        if (directory == NULL) {
            ret = false;
            break;
        }

        Index64 entryIndex = directory->operations->lookupEntry(directory, fileName, INODE_TYPE_FILE);
        if (entryIndex == PHOSPHERUS_NULL) {
            printf("File not exist\n");
        } else {
            DirectoryEntry* entry = directory->operations->getEntry(directory, entryIndex);
            fs->opearations->iNodeGlobalOperations->deleteInode(fs->device, entry->iNodeIndex);
            free(entry);
            directory->operations->removeEntry(directory, entryIndex);
        }
    } while (0);

    free(copy);
    if (directory != rootDir) {
        iNode* iNodeToClose = directory->iNode;
        fs->opearations->directoryGlobalOperations->closeDirectory(directory);
        fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
    }

    return ret;
}

static bool __removeDirectory(FileSystem* fs, Directory* rootDir, char* path) {
    size_t len = strlen(path);
    char* copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/'), * directoryName = NULL;
    Directory* directory = NULL;
    if (sep != NULL) {
        *sep = '\0';
        directory = __navigateDirectory(fs, rootDir, copy);
        directoryName = sep + 1;
    } else {
        directory = rootDir;
        directoryName = copy;
    }
    
    bool ret = true;
    do {
        if (directory == NULL) {
            ret = false;
            break;
        }

        Index64 entryIndex = directory->operations->lookupEntry(directory, directoryName, INODE_TYPE_DIRECTORY);
        if (entryIndex == PHOSPHERUS_NULL) {
            printf("Directory not exist\n");
        } else {
            DirectoryEntry* entry = directory->operations->getEntry(directory, entryIndex);
            fs->opearations->iNodeGlobalOperations->deleteInode(fs->device, entry->iNodeIndex);
            free(entry);
            directory->operations->removeEntry(directory, entryIndex);
        }
    } while (0);

    free(copy);
    if (directory != rootDir) {
        iNode* iNodeToClose = directory->iNode;
        fs->opearations->directoryGlobalOperations->closeDirectory(directory);
        fs->opearations->iNodeGlobalOperations->closeInode(iNodeToClose);
    }

    return true;
}