#include<fs/fsutil.h>

#include<error.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<string.h>
#include<time/time.h>

Result fsutil_openFSentry(FSentry* entry, FSentryDesc* desc, ConstCstring path, FSentryType type) {
    char* buffer = allocateBuffer(BUFFER_SIZE_512);

    Result res = RESULT_SUCCESS;

    SuperBlock* superBlock = rootFS->superBlock;
    Directory* currentDirectory = superBlock->rootDirectory;
    FSentry tmpEntry;
    while (true) {
        if (*path != '\\') {    //TODO: Seperator should be '/'
            res = RESULT_FAIL;
            break;
        }
        ++path;

        ConstCstring next = path;
        while (*next != '\\' && *next != '\0') {
            ++next;
        }

        Size len = ARRAY_POINTER_TO_INDEX(path, next);
        if (len >= POWER_2(BUFFER_SIZE_512)) {
            res = RESULT_FAIL;
            break;
        }

        strncpy(buffer, path, len);
        
        FSentryIdentifier identifier = {
            .name   = buffer,
            .type   = *next == '\0' ? type : FS_ENTRY_TYPE_DIRECTORY,
            .parent = entry->desc
        };

        if (fsutil_dirLookup(currentDirectory, desc, NULL, &identifier) != RESULT_SUCCESS) {
            res = RESULT_FAIL;
            break;
        }

        if (superBlock_rawOpenFSentry(superBlock, &tmpEntry, desc) == RESULT_FAIL) {
            res = RESULT_FAIL;
            break;
        }
        
        if (*next == '\0') {
            memcpy(entry, &tmpEntry, sizeof(FSentry));
            break;
        } else {
            path = next;
            if (currentDirectory != superBlock->rootDirectory) {
                if (superBlock->operations->closeFSentry(superBlock, entry) == RESULT_FAIL) {
                    res = RESULT_FAIL;
                    break;
                }
            }
            currentDirectory = &tmpEntry;
        }
    }

    releaseBuffer(buffer, BUFFER_SIZE_512);

    return res;
}

Result fsutil_closeFSentry(FSentry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    FSentryIdentifier* identifier = &entry->desc->identifier;
    if (identifier->type != FS_ENTRY_TYPE_DIRECTORY) {
        FSentry parentEntry;
        if (superBlock_rawOpenFSentry(superBlock, &parentEntry, identifier->parent) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (fsEntry_rawUpdateChild(&parentEntry, identifier, entry->desc) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (superBlock_rawCloseFSentry(superBlock, &parentEntry) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return superBlock->operations->closeFSentry(superBlock, entry);
}

Result fsutil_dirLookup(Directory* directory, FSentryDesc* desc, Size* entrySizePtr, FSentryIdentifier* identifier) {
    FSentryDesc tmpDesc;
    Size entrySize;
    fsEntry_rawSeek(directory, 0);
    Result res = RESULT_FAIL;

    while ((res = fsEntry_rawReadChild(directory, &tmpDesc, &entrySize)) != RESULT_FAIL) {
        if (res == RESULT_SUCCESS) {
            return RESULT_CONTINUE; //TODO: Bad, set a new result for failed but no error
        }

        if (strcmp(identifier->name, tmpDesc.identifier.name) == 0 && identifier->type == tmpDesc.identifier.type) {
            if (desc != NULL) {
                memcpy(desc, &tmpDesc, sizeof(FSentryDesc));
            }

            if (entrySizePtr != NULL) {
                *entrySizePtr = entrySize;
            }
            return RESULT_SUCCESS;
        }

        FSentryDesc_clearStruct(&tmpDesc);
        fsEntry_rawSeek(directory, directory->pointer + entrySize);
    }

    return RESULT_FAIL;
}

Result fsutil_fileSeek(File* file, Int64 offset, Uint8 begin) {
    Index64 base = file->pointer;
    switch (begin) {
        case FILE_SEEK_BEGIN:
            base = 0;
            break;
        case FILE_SEEK_CURRENT:
            break;
        case FILE_SEEK_END:
            base = file->desc->dataRange.length;
            break;
        default:
            break;
    }
    base += offset;

    if ((Int64)base < 0 || base > file->desc->dataRange.length) {
        ERROR_CODE_SET(ERROR_CODE_OBJECT_INDEX, ERROR_CODE_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    if (fsEntry_rawSeek(file, base) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Index64 fsutil_fileGetPointer(File* file) {
    return file->pointer;
}

Result fsutil_fileRead(File* file, void* buffer, Size n) {
    if (fsEntry_rawRead(file, buffer, n) == RESULT_FAIL || fsEntry_rawSeek(file, file->pointer + n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Timestamp timestamp;
    readTimestamp(&timestamp);

    file->desc->lastAccessTime = timestamp.second;

    return RESULT_SUCCESS;
}    

Result fsutil_fileWrite(File* file, const void* buffer, Size n) {
    if (fsEntry_rawWrite(file, buffer, n) == RESULT_FAIL || fsEntry_rawSeek(file, file->pointer + n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Timestamp timestamp;
    readTimestamp(&timestamp);

    file->desc->lastAccessTime = file->desc->lastModifyTime = timestamp.second;

    return RESULT_SUCCESS;
}