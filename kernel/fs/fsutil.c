#include<fs/fsutil.h>

#include<error.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<string.h> 

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

    return RESULT_SUCCESS;
}    

Result fileWrite(File file, const void* buffer, Size n) {
    if (rawFileSystemEntryWrite(file, buffer, n) == RESULT_FAIL || rawFileSystemEntrySeek(file, file->pointer + n) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    return RESULT_SUCCESS;
}