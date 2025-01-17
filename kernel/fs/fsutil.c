#include<fs/fsutil.h>

#include<cstring.h>
#include<fs/fcntl.h>
#include<fs/fs.h>
#include<fs/fsEntry.h>
#include<fs/superblock.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<time/time.h>

Index64 fsutil_fileSeek(File* file, Int64 offset, Uint8 begin) {
    Index64 base = file->pointer;
    switch (begin) {
        case FS_FILE_SEEK_BEGIN:
            base = 0;
            break;
        case FS_FILE_SEEK_CURRENT:
            break;
        case FS_FILE_SEEK_END:
            base = file->desc->dataRange.length;
            break;
        default:
            break;
    }
    base += offset;

    if ((Int64)base < 0 || base > file->desc->dataRange.length) {
        // ERROR_CODE_SET(ERROR_CODE_OBJECT_INDEX, ERROR_CODE_STATUS_OUT_OF_BOUND);
        return INVALID_INDEX;
    }

    if (fsEntry_rawSeek(file, base) == INVALID_INDEX) {
        return INVALID_INDEX;
    }

    return file->pointer;
}

Index64 fsutil_fileGetPointer(File* file) {
    return file->pointer;
}

OldResult fsutil_fileRead(File* file, void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->openFlags) == FCNTL_OPEN_WRITE_ONLY) {
        return RESULT_FAIL;
    }

    if (fsEntry_rawRead(file, buffer, n) != RESULT_SUCCESS || fsEntry_rawSeek(file, file->pointer + n) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (TEST_FLAGS_FAIL(file->openFlags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);

        file->desc->lastAccessTime = timestamp.second;
    }

    return RESULT_SUCCESS;
}    

OldResult fsutil_fileWrite(File* file, const void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->openFlags) == FCNTL_OPEN_READ_ONLY) {
        return RESULT_FAIL;
    }

    if (TEST_FLAGS(file->openFlags, FCNTL_OPEN_APPEND)) {
        fsutil_fileSeek(file, 0, FS_FILE_SEEK_END);
    }

    if (fsEntry_rawWrite(file, buffer, n) != RESULT_SUCCESS || fsutil_fileSeek(file, n, FS_FILE_SEEK_CURRENT) == INVALID_INDEX) {
        return RESULT_ERROR;
    }

    if (TEST_FLAGS_FAIL(file->openFlags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);

        file->desc->lastAccessTime = timestamp.second;
    }

    return RESULT_SUCCESS;
}

OldResult fsutil_openfsEntry(SuperBlock* superBlock, ConstCstring path, fsEntry* entryOut, FCNTLopenFlags flags) {
    fsEntryIdentifier identifier;
    if (fsEntryIdentifier_initStruct(&identifier, path, TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY)) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    OldResult res = superBlock_openfsEntry(superBlock, &identifier, entryOut, flags);
    fsEntryIdentifier_clearStruct(&identifier);
    return res;
}

OldResult fsutil_closefsEntry(fsEntry* entry) {
    return superBlock_closefsEntry(entry);
}

OldResult fsutil_lookupEntryDesc(Directory* directory, ConstCstring name, bool isDirectory, fsEntryDesc* descOut, Size* entrySizeOut) {
    fsEntryDesc tmpDesc;
    Size entrySize;
    OldResult res = RESULT_ERROR;
    fsEntry_rawSeek(directory, 0);
    while ((res = fsEntry_rawDirReadEntry(directory, &tmpDesc, &entrySize)) != RESULT_ERROR) {
        if (res == RESULT_SUCCESS) {    //Meaning no more entries
            return RESULT_FAIL;
        }

        if (cstring_strcmp(name, tmpDesc.identifier.name.data) == 0 && isDirectory == tmpDesc.identifier.isDirectory) {
            if (descOut != NULL) {
                memory_memcpy(descOut, &tmpDesc, sizeof(fsEntryDesc));
            }

            if (entrySizeOut != NULL) {
                *entrySizeOut = entrySize;
            }
            return RESULT_SUCCESS;
        }

        fsEntryDesc_clearStruct(&tmpDesc);
        fsEntry_rawSeek(directory, directory->pointer + entrySize);
    }

    return RESULT_ERROR;
}

OldResult fsutil_loacateRealIdentifier(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** superBlockOut, fsEntryIdentifier* identifierOut) {
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        fsEntryIdentifier_copy(identifierOut, identifier);
        return RESULT_SUCCESS;
    }

    String fullPath;
    if (string_append(&fullPath, &identifier->parentPath, FS_PATH_SEPERATOR) != RESULT_SUCCESS || string_concat(&fullPath, &fullPath, &identifier->name) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }

    SuperBlock* currentSuperBlock = superBlock, * nextSuperBlock = NULL;
    Index64 pathSplit = INVALID_INDEX;
    fsEntryDesc* mountedToDesc;
    OldResult res;
    while ((res = superBlock_stepSearchMount(currentSuperBlock, &fullPath, &pathSplit, &nextSuperBlock, &mountedToDesc)) == RESULT_CONTINUE) {
        String tmp;
        string_initStruct(&tmp, "");
        if (mountedToDesc != nextSuperBlock->rootDirDesc) {
            string_append(&tmp, &mountedToDesc->identifier.parentPath, FS_PATH_SEPERATOR);
            string_concat(&tmp, &tmp, &mountedToDesc->identifier.name);
        }

        string_slice(&fullPath, &fullPath, pathSplit, -1);
        string_concat(&fullPath, &tmp, &fullPath);
        
        string_clearStruct(&tmp);
        currentSuperBlock = nextSuperBlock;
    }

    if (res == RESULT_ERROR) {
        return RESULT_ERROR;
    } else {
        fsEntryIdentifier_initStruct(identifierOut, fullPath.data, identifier->isDirectory);
        *superBlockOut = currentSuperBlock;
    }
    string_clearStruct(&fullPath);
    
    return RESULT_SUCCESS;
}

OldResult fsutil_seekLocalFSentryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc** descOut) {
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        *descOut = superBlock->rootDirDesc;
        return RESULT_SUCCESS;
    }

    fsEntry currentDirectory;
    fsEntryDesc currentEntryDesc, nextEntryDesc;
    memory_memcpy(&currentEntryDesc, superBlock->rootDirDesc, sizeof(fsEntryDesc));

    String fullPath;
    fsEntryIdentifier tmpIdentifier;
    if (string_append(&fullPath, &identifier->parentPath, FS_PATH_SEPERATOR) != RESULT_SUCCESS || string_concat(&fullPath, &fullPath, &identifier->name) != RESULT_SUCCESS) {
        return RESULT_ERROR;
    }
    ConstCstring cFullPath = fullPath.data, name = cFullPath;
    while (true) {
        if (*name != FS_PATH_SEPERATOR) {
            return RESULT_ERROR;
        }
        ++name;

        ConstCstring next = name;
        while (*next != FS_PATH_SEPERATOR && *next != '\0') {
            ++next;
        }
        
        //TODO: Ugly initialization
        if (
            string_initStructN(&tmpIdentifier.name, name, ARRAY_POINTER_TO_INDEX(name, next)) != RESULT_SUCCESS ||
            string_initStructN(&tmpIdentifier.parentPath, cFullPath, ARRAY_POINTER_TO_INDEX(cFullPath, name) - 1) != RESULT_SUCCESS
        ) {
            return RESULT_ERROR;
        }
        tmpIdentifier.isDirectory = *next == '\0' ? identifier->isDirectory : true;

        if (superBlock_rawOpenfsEntry(superBlock, &currentDirectory, &currentEntryDesc, FCNTL_OPEN_FILE_DEFAULT_FLAGS) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        OldResult lookupRes;
        if ((lookupRes = fsutil_lookupEntryDesc(&currentDirectory, tmpIdentifier.name.data, tmpIdentifier.isDirectory, &nextEntryDesc, NULL)) == RESULT_ERROR) {
            return RESULT_ERROR;
        }

        fsEntryIdentifier_clearStruct(&tmpIdentifier);
        if (superBlock_rawClosefsEntry(superBlock, &currentDirectory) != RESULT_SUCCESS) {
            return RESULT_ERROR;
        }

        if (lookupRes == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (*next == '\0') {
            if (*descOut == NULL) {
                return RESULT_ERROR;
            }
            memory_memcpy(*descOut, &nextEntryDesc, sizeof(fsEntryDesc));
            break;
        }
        
        name = next;
        memory_memcpy(&currentEntryDesc, &nextEntryDesc, sizeof(fsEntryDesc));
    }

    return RESULT_SUCCESS;
}