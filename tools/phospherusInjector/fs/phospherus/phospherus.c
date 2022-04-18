#include<fs/phospherus/phospherus.h>

#include<fs/phospherus/allocator.h>
#include<fs/phospherus/directory.h>
#include<fs/phospherus/file.h>
#include<fs/phospherus/inode.h>
#include<malloc.h>
#include<string.h>

#define PHOSPHERUS_ROOT_DIRECTORY_INODE 1

typedef struct {
    void* allocator;
    void* rootDirectory;
} __Phospherus_FileSystem;

static block_index_t    __createFile    (FileSystem* fileSystem);
static bool             __deleteFile    (FileSystem* fileSystem, block_index_t inode);
static FilePtr          __openFile      (FileSystem* fileSystem, block_index_t inode);
static void             __closeFile     (FileSystem* fileSystem, FilePtr file);
static size_t           __getFileSize   (FileSystem* fileSystem, FilePtr file);
static void             __seekFile      (FileSystem* fileSystem, FilePtr file, size_t pointer);
static size_t           __readFile      (FileSystem* fileSystem, FilePtr file, void* buffer, size_t n);
static size_t           __writeFile     (FileSystem* fileSystem, FilePtr file, const void* buffer, size_t n);
static bool             __truncateFile  (FileSystem* fileSystem, FilePtr file, size_t truncateAt);

static DirectoryPtr     __getRootDirectory                  (FileSystem* fileSystem);
static block_index_t    __createDirectory                   (FileSystem* fileSystem);
static bool             __deleteDirectory                   (FileSystem* fileSystem, block_index_t inode);
static DirectoryPtr     __openDirectory                     (FileSystem* fileSystem, block_index_t inode);
static void             __closeDirectory                    (FileSystem* fileSystem, DirectoryPtr directory);
static size_t           __getDirectoryItemNum               (FileSystem* fileSystem, DirectoryPtr directory);
static size_t           __searchDirectoryItem               (FileSystem* fileSystem, DirectoryPtr directory, const char* itemName, bool isDirectory);
static block_index_t    __getDirectoryItemInode             (FileSystem* fileSystem, DirectoryPtr directory, size_t index);
static bool             __checkIsDirectoryItemDirectory     (FileSystem* fileSystem, DirectoryPtr directory, size_t index);
static void             __readDirectoryItemName             (FileSystem* fileSystem, DirectoryPtr directory, size_t index, char* buffer, size_t n);
static block_index_t    __removeDirectoryItem               (FileSystem* fileSystem, DirectoryPtr directory, size_t itemIndex);
static bool             __insertDirectoryItem               (FileSystem* fileSystem, DirectoryPtr directory, block_index_t inode, const char* itemName, bool isDirectory);

void phospherus_initFileSystem() {
    phospherus_initAllocator();
    phospherus_initInode();
}

bool phospherus_deployFileSystem(BlockDevice* device) {
    phospherus_deployAllocator(device);
    if (!phospherus_checkBlockDevice(device)) {
        return false;
    }

    Phospherus_Allocator* allocator = phospherus_openAllocator(device);
    block_index_t rootDir = phospherus_createRootDirectory(allocator);
    phospherus_closeAllocator(allocator);
    if (rootDir != PHOSPHERUS_ROOT_DIRECTORY_INODE) {
        return false;
    }

    return true;
}

bool phospherus_checkFileSystem(BlockDevice* device) {
    if (!phospherus_checkBlockDevice(device)) {
        return false;
    }

    Phospherus_Allocator* allocator = phospherus_openAllocator(device);
    Phospherus_Directory* rootDir = phospherus_openDirectory(allocator, PHOSPHERUS_ROOT_DIRECTORY_INODE);

    if (rootDir == NULL || !phospherus_checkIsRootDirectory(rootDir)) {
        phospherus_closeAllocator(allocator);

        return false;
    }

    phospherus_closeAllocator(allocator);
    phospherus_closeDirectory(rootDir);

    return true;
}

FileSystem* phospherus_openFileSystem(BlockDevice* device) {
    __Phospherus_FileSystem* specificSystem = malloc(sizeof(__Phospherus_FileSystem));
    specificSystem->allocator = phospherus_openAllocator(device);
    if (specificSystem->allocator == NULL) {
        free(specificSystem);
        return NULL;
    }

    specificSystem->rootDirectory = phospherus_openDirectory(specificSystem->allocator, PHOSPHERUS_ROOT_DIRECTORY_INODE);
    if (specificSystem->rootDirectory == NULL) {
        free(specificSystem);
        return NULL;
    }

    if (!phospherus_checkIsRootDirectory(specificSystem->rootDirectory)) {
        phospherus_closeDirectory(specificSystem->rootDirectory);
        free(specificSystem);
        return NULL;
    }
    
    FileSystem* ret = malloc(sizeof(FileSystem));

    strcpy(ret->name, "phospherus");
    ret->type = FILE_SYSTEM_TYPE_PHOSPHERUS;
    ret->specificFileSystem = specificSystem;
    
    ret->fileOperations = (FileOperations) {
        __createFile,
        __deleteFile,
        __openFile,
        __closeFile,
        __getFileSize,
        __seekFile,
        __readFile,
        __writeFile,
        __truncateFile
    };

    ret->pathOperations = (PathOperations) {
        __getRootDirectory,
        __createDirectory,
        __deleteDirectory,
        __openDirectory,
        __closeDirectory,
        __getDirectoryItemNum,
        __searchDirectoryItem,
        __getDirectoryItemInode,
        __checkIsDirectoryItemDirectory,
        __readDirectoryItemName,
        __removeDirectoryItem,
        __insertDirectoryItem
    };

    return ret;
}

void phospherus_closeFileSystem(FileSystem* system) {
    __Phospherus_FileSystem* specificSystem = system->specificFileSystem;
    phospherus_closeDirectory(specificSystem->rootDirectory);
    phospherus_closeAllocator(specificSystem->allocator);
    free(specificSystem);
    free(system);
}

static block_index_t __createFile(FileSystem* fileSystem) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_createFile(specificSystem->allocator);
}

static bool __deleteFile(FileSystem* fileSystem, block_index_t inode) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_deleteFile(specificSystem->allocator, inode);
}

static FilePtr __openFile(FileSystem* fileSystem, block_index_t inode) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_openFile(specificSystem->allocator, inode);
}

static void __closeFile(FileSystem* fileSystem, FilePtr file) {
    phospherus_closeFile(file);
}

static size_t __getFileSize(FileSystem* fileSystem, FilePtr file) {
    phospherus_getFileSize(file);
}

static void __seekFile(FileSystem* fileSystem, FilePtr file, size_t pointer) {
    phospherus_seekFile(file, pointer);
}

static size_t __readFile(FileSystem* fileSystem, FilePtr file, void* buffer, size_t n) {
    return phospherus_readFile(file, buffer, n);
}

static size_t __writeFile(FileSystem* fileSystem, FilePtr file, const void* buffer, size_t n) {
    return phospherus_writeFile(file, buffer, n);
}

static bool __truncateFile(FileSystem* fileSystem, FilePtr file, size_t truncateAt) {
    return phospherus_truncateFile(file, truncateAt);
}

//---------------------------------------------------------------------------------------------------

static DirectoryPtr __getRootDirectory(FileSystem* fileSystem) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return specificSystem->rootDirectory;
}

static block_index_t __createDirectory(FileSystem* fileSystem) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_createDirectory(specificSystem->allocator);
}

static bool __deleteDirectory(FileSystem* fileSystem, block_index_t inode) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_deleteDirectory(specificSystem->allocator, inode);
}

static DirectoryPtr __openDirectory(FileSystem* fileSystem, block_index_t inode) {
    __Phospherus_FileSystem* specificSystem = fileSystem->specificFileSystem;
    return phospherus_openDirectory(specificSystem->allocator, inode);
}

static void __closeDirectory(FileSystem* fileSystem, DirectoryPtr directory) {
    phospherus_closeDirectory(directory);
}

static size_t __getDirectoryItemNum(FileSystem* fileSystem, DirectoryPtr directory) {
    return phospherus_getDirectoryEntryNum(directory);
}

static size_t __searchDirectoryItem(FileSystem* fileSystem, DirectoryPtr directory, const char* itemName, bool isDirectory) {
    return phospherus_directorySearchEntry(directory, itemName, isDirectory);
}

static block_index_t __getDirectoryItemInode(FileSystem* fileSystem, DirectoryPtr directory, size_t index) {
    return phospherus_getDirectoryEntryInodeIndex(directory, index);
}

static bool __checkIsDirectoryItemDirectory(FileSystem* fileSystem, DirectoryPtr directory, size_t index) {
    return phospherus_checkDirectoryEntryIsDirectory(directory, index);
}

static void __readDirectoryItemName(FileSystem* fileSystem, DirectoryPtr directory, size_t index, char* buffer, size_t n) {
    const char* namePtr = phospherus_getDirectoryEntryName(directory, index);
    strncpy(buffer, namePtr, n);
}

static block_index_t __removeDirectoryItem(FileSystem* fileSystem, DirectoryPtr directory, size_t itemIndex) {
    block_index_t ret = phospherus_getDirectoryEntryInodeIndex(directory, itemIndex);
    phospherus_deleteDirectoryEntry(directory, itemIndex);
    return ret;
}

static bool __insertDirectoryItem(FileSystem* fileSystem, DirectoryPtr directory, block_index_t inode, const char* itemName, bool isDirectory) {
    return phospherus_directoryAddEntry(directory, inode, itemName, isDirectory);
}