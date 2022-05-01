#include<algorithms.h>
#include<stddef.h>
#include<stdio.h>
#include<stdlib.h>
#include<string.h>
#include<unistd.h>
#include<blowup.h>
#include<fs/blockDevice/blockDevice.h>
#include<fs/fileSystem.h>
#include<fs/phospherus/phospherus.h>

typedef enum {
    ADD_FILE,
    ADD_DIRECTORY,
    LIST,
    REMOVE_FILE,
    REMOVE_DIRECTORY,
    DEPLOY
} Mode;

static DirectoryPtr __navigateDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path);

static bool __addFile(FileSystem* fs, DirectoryPtr rootDir, char* path, char* filePath);

static bool __addDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path, char* directoryName);

static bool __list(FileSystem* fs, DirectoryPtr rootDir, char* path);

static bool __removeFile(FileSystem* fs, DirectoryPtr rootDir, char* path);

static bool __removeDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path);

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
        blowup("Add file/directory must have file to inject");
    }
    
    initFileSystem(FILE_SYSTEM_TYPE_PHOSPHERUS);
    BlockDevice* device = createBlockDevice(imgFile, deviceName, base, size);
    if (device == NULL) {
        blowup("Device create failed");
    }
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

    switch(mode) {
        case ADD_FILE:
            if (!__addFile(fs, fs->pathOperations.getRootDirectory(fs), path, injectFile)) {
                blowup("Add file failed");
            }
            break;
        case ADD_DIRECTORY:
            if (!__addDirectory(fs, fs->pathOperations.getRootDirectory(fs), path, injectFile)) {
                blowup("Add directory failed");
            }
            break;
        case LIST:
            if (!__list(fs, fs->pathOperations.getRootDirectory(fs), path)) {
                blowup("List failed");
            }
            break;
        case REMOVE_FILE:
            if (!__removeFile(fs, fs->pathOperations.getRootDirectory(fs), path)) {
                blowup("Remove file failed");
            }
            break;
        case REMOVE_DIRECTORY:
            if (!__removeDirectory(fs, fs->pathOperations.getRootDirectory(fs), path)) {
                blowup("Remove directory failed");
            }
            break;
        default:
            blowup("Unknown operation");
    }

    closeFileSystem(fs);
    deleteBlockDevice(device);
    printf("Done\n");
}

const char* sep = "/";
static DirectoryPtr __navigateDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path) {
    if (path[0] == '\0' || strcmp(path, "/") == 0) {
        return rootDir;
    }

    if (path[0] != '/') {
        return NULL;
    }

    DirectoryPtr currentDir = rootDir;
    char* currentPath = path + 1;
    const char* next = strtok(currentPath, sep);
    while (next != NULL) {
        size_t index = fs->pathOperations.searchDirectoryItem(fs, currentDir, next, true);
        if (index == PHOSPHERUS_NULL) {
            return NULL;
        }
        block_index_t inode = fs->pathOperations.getDirectoryItemInode(fs, currentDir, index);
        DirectoryPtr copy = currentDir;
        currentDir = fs->pathOperations.openDirectory(fs, inode);

        if (copy != rootDir) {
            fs->pathOperations.closeDirectory(fs, copy);
        }

        next = strtok(NULL, sep);
    }

    return currentDir;
}

#define TRANSFER_BUFFER_SIZE (1llu << 20)

static bool __addFile(FileSystem* fs, DirectoryPtr rootDir, char* path, char* filePath) {
    DirectoryPtr directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL) {
        return false;
    }

    FILE* file = fopen(filePath, "rb");
    if (file == NULL) {
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        return false;
    }
    
    char* fileName = strrchr(filePath, '/');
    if (fileName == NULL) {
        fileName = filePath;
    } else {
        *fileName = '\0';
        ++fileName;
    }
    
    if (fs->pathOperations.searchDirectoryItem(fs, directory, fileName, true) != PHOSPHERUS_NULL) {
        printf("File already existed\n");
        fclose(file);
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        return true;
    }

    printf("Inserting file %s\n", fileName);

    block_index_t inode = fs->fileOperations.createFile(fs);
    if (inode == PHOSPHERUS_NULL) {
        fclose(file);
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        return false;
    }

    void* buffer = malloc(TRANSFER_BUFFER_SIZE);
    FilePtr fileInside = fs->fileOperations.openFile(fs, inode);
    fseek(file, 0L, SEEK_END);
    size_t fileSize = ftell(file), currentPointer = 0;
    while (fileSize > 0) {
        size_t batchSize = umin64(fileSize, TRANSFER_BUFFER_SIZE);
        fseek(file, currentPointer, SEEK_SET);
        fs->fileOperations.seekFile(fs, fileInside, currentPointer);

        if (fread(buffer, batchSize, 1, file) != 1) {
            fclose(file);
            free(buffer);
            fs->fileOperations.deleteFile(fs, inode);
            if (directory != rootDir) {
                fs->pathOperations.closeDirectory(fs, directory);
            }
            return false;
        }
        fs->fileOperations.writeFile(fs, fileInside, buffer, batchSize);
        fileSize -= batchSize;
        currentPointer += batchSize;
    }
    fs->fileOperations.closeFile(fs, fileInside);
    free(buffer);
    fclose(file);

    bool ret = true;
    if (!fs->pathOperations.insertDirectoryItem(fs, directory, inode, fileName, false)) {
        fs->fileOperations.deleteFile(fs, inode);
        ret = false;
    }

    if (directory != rootDir) {
        fs->pathOperations.closeDirectory(fs, directory);
    }
    
    return ret;
}

static bool __addDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path, char* directoryName) {
    DirectoryPtr directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL || strchr(directoryName, '/') != NULL) {
        return false;
    }

    if (fs->pathOperations.searchDirectoryItem(fs, directory, directoryName, true) != PHOSPHERUS_NULL) {
        printf("Directory already existed\n");
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        return true;
    }

    printf("Inserting directory %s\n", directoryName);

    block_index_t inode = fs->pathOperations.createDirectory(fs);
    if (inode == PHOSPHERUS_NULL) {
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        return false;
    }

    bool ret = true;
    if (!fs->pathOperations.insertDirectoryItem(fs, directory, inode, directoryName, true)) {
        fs->pathOperations.deleteDirectory(fs, inode);
        ret = false;
    }

    if (directory != rootDir) {
        fs->pathOperations.closeDirectory(fs, directory);
    }

    return ret;
}

static char nameBuffer[64];

static bool __list(FileSystem* fs, DirectoryPtr rootDir, char* path) {
    DirectoryPtr directory = __navigateDirectory(fs, rootDir, path);
    if (directory == NULL) {
        return false;
    }

    size_t itemNum = fs->pathOperations.getDirectoryItemNum(fs, directory);
    printf("%lu items found in %s:\n", itemNum, path);
    for (int i = 0; i < itemNum; ++i) {
        fs->pathOperations.readDirectoryItemName(fs, directory, i, nameBuffer, 62);
        if (strlen(nameBuffer) > 61) {
            printf("%61s...  ", nameBuffer);
        } else {
            printf("%-64s  ", nameBuffer);
        }

        bool isDirectory = fs->pathOperations.checkIsDirectoryItemDirectory(fs, directory, i);
        if (isDirectory) {
            printf("Directory\n");
        } else {
            printf("File\n");
        }
    }

    if (directory != rootDir) {
        fs->pathOperations.closeDirectory(fs, directory);
    }
    return true;
}

static bool __removeFile(FileSystem* fs, DirectoryPtr rootDir, char* path) {
    size_t len = strlen(path);
    char* copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/'), * fileName = NULL;
    DirectoryPtr directory = NULL;
    if (sep != NULL) {
        *sep = '\0';
        directory = __navigateDirectory(fs, rootDir, copy);
        fileName = sep + 1;
    } else {
        directory = rootDir;
        fileName = copy;
    }
    
    if (directory == NULL) {
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        free(copy);
        return false;
    }

    size_t index = fs->pathOperations.searchDirectoryItem(fs, directory, fileName, false);
    if (index == PHOSPHERUS_NULL) {
        printf("File not exist\n");
    } else {
        fs->fileOperations.deleteFile(
            fs,
            fs->pathOperations.getDirectoryItemInode(fs, directory, index)
        );
        fs->pathOperations.removeDirectoryItem(fs, directory, index);
    }

    if (directory != rootDir) {
        fs->pathOperations.closeDirectory(fs, directory);
    }

    free(copy);
    return true;
}

static bool __removeDirectory(FileSystem* fs, DirectoryPtr rootDir, char* path) {
    size_t len = strlen(path);
    char* copy = malloc(len + 1);
    strcpy(copy, path);

    char* sep = strrchr(copy, '/'), * directoryName = NULL;
    DirectoryPtr directory = NULL;
    if (sep != NULL) {
        *sep = '\0';
        directory = __navigateDirectory(fs, rootDir, copy);
        directoryName = sep + 1;
    } else {
        directory = rootDir;
        directoryName = copy;
    }
    
    if (directory == NULL) {
        if (directory != rootDir) {
            fs->pathOperations.closeDirectory(fs, directory);
        }
        free(copy);
        return false;
    }

    size_t index = fs->pathOperations.searchDirectoryItem(fs, directory, directoryName, true);
    if (index == PHOSPHERUS_NULL) {
        printf("Directory not exist\n");
    } else {
        fs->pathOperations.deleteDirectory(
            fs,
            fs->pathOperations.getDirectoryItemInode(fs, directory, index)
        );
        fs->pathOperations.removeDirectoryItem(fs, directory, index);
    }

    if (directory != rootDir) {
        fs->pathOperations.closeDirectory(fs, directory);
    }

    free(copy);
    return true;
}