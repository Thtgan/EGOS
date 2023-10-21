#include<fs/fsutil.h>

#include<error.h>
#include<fs/fileSystemEntry.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<string.h>

Result directoryLookup(Directory directory, FileSystemEntryDescriptor* desc) {
    FileSystemEntryDescriptor tmpDesc;
    rawFileSystemEntrySeek(directory, 0);
    while (rawFileSystemEntryReadChildren(directory, &tmpDesc, 1) == RESULT_CONTINUE) {
        if (strcmp(desc->name, tmpDesc.name) == 0 && desc->type == tmpDesc.type) {
            memcpy(desc, &tmpDesc, sizeof(FileSystemEntryDescriptor));
            return RESULT_SUCCESS;
        }
        clearFileSystemEntryDescriptor(&tmpDesc);
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