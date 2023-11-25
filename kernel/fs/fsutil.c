#include<fs/fsutil.h>

#include<error.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<string.h>
#include<time/time.h>

Result fileSystemEntryOpen(FileSystemEntry* entry, FileSystemEntryDescriptor* desc, ConstCstring path, FileSystemEntryType type) {
    char* buffer = allocateBuffer(BUFFER_SIZE_512);

    Result res = RESULT_SUCCESS;

    SuperBlock* superBlock = rootFileSystem->superBlock;
    Directory currentDirectory = superBlock->rootDirectory;
    FileSystemEntry tmpEntry;
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
        
        FileSystemEntryIdentifier identifier = {
            .name   = buffer,
            .type   = *next == '\0' ? type : FILE_SYSTEM_ENTRY_TYPE_DIRECTOY,
            .parent = entry->descriptor
        };

        if (directoryLookup(currentDirectory, desc, NULL, &identifier) != RESULT_SUCCESS) {
            res = RESULT_FAIL;
            break;
        }

        if (rawSuperNodeOpenFileSystemEntry(superBlock, &tmpEntry, desc) == RESULT_FAIL) {
            res = RESULT_FAIL;
            break;
        }
        
        if (*next == '\0') {
            memcpy(entry, &tmpEntry, sizeof(FileSystemEntry));
            break;
        } else {
            path = next;
            if (currentDirectory != superBlock->rootDirectory) {
                if (superBlock->operations->closeFileSystemEntry(superBlock, entry) == RESULT_FAIL) {
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

Result fileSystemEntryClose(FileSystemEntry* entry) {
    SuperBlock* superBlock = entry->iNode->superBlock;
    FileSystemEntryIdentifier* identifier = &entry->descriptor->identifier;
    if (identifier->type != FILE_SYSTEM_ENTRY_TYPE_DIRECTOY) {
        FileSystemEntry parentEntry;
        if (rawSuperNodeOpenFileSystemEntry(superBlock, &parentEntry, identifier->parent) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (rawFileSystemEntryUpdateChild(&parentEntry, identifier, entry->descriptor) == RESULT_FAIL) {
            return RESULT_FAIL;
        }

        if (rawSuperNodeCloseFileSystemEntry(superBlock, &parentEntry) == RESULT_FAIL) {
            return RESULT_FAIL;
        }
    }

    return superBlock->operations->closeFileSystemEntry(superBlock, entry);
}

Result directoryLookup(Directory directory, FileSystemEntryDescriptor* descriptor, Size* entrySizePtr, FileSystemEntryIdentifier* identifier) {
    FileSystemEntryDescriptor tmpDesc;
    Size entrySize;
    rawFileSystemEntrySeek(directory, 0);
    Result res = RESULT_FAIL;

    while ((res = rawFileSystemEntryReadChild(directory, &tmpDesc, &entrySize)) != RESULT_FAIL) {
        if (res == RESULT_SUCCESS) {
            return RESULT_CONTINUE; //TODO: Bad, set a new result for failed but no error
        }

        if (strcmp(identifier->name, tmpDesc.identifier.name) == 0 && identifier->type == tmpDesc.identifier.type) {
            if (descriptor != NULL) {
                memcpy(descriptor, &tmpDesc, sizeof(FileSystemEntryDescriptor));
            }

            if (entrySizePtr != NULL) {
                *entrySizePtr = entrySize;
            }
            return RESULT_SUCCESS;
        }

        clearFileSystemEntryDescriptor(&tmpDesc);
        rawFileSystemEntrySeek(directory, directory->pointer + entrySize);
    }

    return RESULT_FAIL;
}

Result fileSeek(File file, Int64 offset, Uint8 begin) {
    Index64 base = file->pointer;
    switch (begin) {
        case FILE_SEEK_BEGIN:
            base = 0;
            break;
        case FILE_SEEK_CURRENT:
            break;
        case FILE_SEEK_END:
            base = file->descriptor->dataRange.length;
            break;
        default:
            break;
    }
    base += offset;

    if ((Int64)base < 0 || base > file->descriptor->dataRange.length) {
        SET_ERROR_CODE(ERROR_OBJECT_INDEX, ERROR_STATUS_OUT_OF_BOUND);
        return RESULT_FAIL;
    }

    if (rawFileSystemEntrySeek(file, base) == INVALID_INDEX) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}

Index64 fileGetPointer(File file) {
    return file->pointer;
}

Result fileRead(File file, void* buffer, Size n) {
    if (rawFileSystemEntryRead(file, buffer, n) == RESULT_FAIL || rawFileSystemEntrySeek(file, file->pointer + n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Timestamp timestamp;
    readTimestamp(&timestamp);

    file->descriptor->lastAccessTime = timestamp.second;

    return RESULT_SUCCESS;
}    

Result fileWrite(File file, const void* buffer, Size n) {
    if (rawFileSystemEntryWrite(file, buffer, n) == RESULT_FAIL || rawFileSystemEntrySeek(file, file->pointer + n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Timestamp timestamp;
    readTimestamp(&timestamp);

    file->descriptor->lastAccessTime = file->descriptor->lastModifyTime = timestamp.second;

    return RESULT_SUCCESS;
}