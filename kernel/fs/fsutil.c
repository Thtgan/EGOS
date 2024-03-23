#include<fs/fsutil.h>

#include<cstring.h>
#include<error.h>
#include<fs/fsEntry.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<time/time.h>

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

Result fsutil_openfsEntry(SuperBlock* superBlock, ConstCstring path, fsEntryType type, fsEntry* entryOut) {
    fsEntryIdentifier identifier;
    if (fsEntryIdentifier_initStruct(&identifier, path, type) == RESULT_FAIL) {
        return RESULT_FAIL;
    }

    Result res = superBlock_openfsEntry(superBlock, &identifier, entryOut);
    fsEntryIdentifier_clearStruct(&identifier);
    return res;
}

Result fsutil_closefsEntry(fsEntry* entry) {
    return superBlock_closefsEntry(entry);
}

Result fsutil_lookupEntryDesc(Directory* directory, fsEntryIdentifier* identifier, fsEntryDesc* descOut, Size* entrySizeOut) {
    fsEntryDesc tmpDesc;
    Size entrySize;
    Result res = RESULT_FAIL;
    fsEntry_rawSeek(directory, 0);
    while ((res = fsEntry_rawDirReadEntry(directory, &tmpDesc, &entrySize)) != RESULT_FAIL) {
        if (res == RESULT_SUCCESS) {
            return RESULT_CONTINUE; //TODO: Bad, set a new result for failed but no error
        }

        if (strcmp(identifier->name.data, tmpDesc.identifier.name.data) == 0 && identifier->type == tmpDesc.identifier.type) {
            if (descOut != NULL) {
                memcpy(descOut, &tmpDesc, sizeof(fsEntryDesc));
            }

            if (entrySizeOut != NULL) {
                *entrySizeOut = entrySize;
            }
            return RESULT_SUCCESS;
        }

        fsEntryDesc_clearStruct(&tmpDesc);
        fsEntry_rawSeek(directory, directory->pointer + entrySize);
    }

    return RESULT_FAIL;
}