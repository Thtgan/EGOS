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
#include<error.h>

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

void fsutil_fileRead(File* file, void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->openFlags) == FCNTL_OPEN_WRITE_ONLY) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    fsEntry_rawRead(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    fsEntry_rawSeek(file, file->pointer + n);

    if (TEST_FLAGS_FAIL(file->openFlags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);

        file->desc->lastAccessTime = timestamp.second;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}    

void fsutil_fileWrite(File* file, const void* buffer, Size n) {
    if (FCNTL_OPEN_EXTRACL_ACCESS_MODE(file->openFlags) == FCNTL_OPEN_READ_ONLY) {
        ERROR_THROW(ERROR_ID_PERMISSION_ERROR, 0);
    }

    if (TEST_FLAGS(file->openFlags, FCNTL_OPEN_APPEND)) {
        fsutil_fileSeek(file, 0, FS_FILE_SEEK_END);
    }

    fsEntry_rawWrite(file, buffer, n);
    ERROR_GOTO_IF_ERROR(0);
    fsutil_fileSeek(file, n, FS_FILE_SEEK_CURRENT);

    if (TEST_FLAGS_FAIL(file->openFlags, FCNTL_OPEN_NOATIME)) {
        Timestamp timestamp;
        time_getTimestamp(&timestamp);

        file->desc->lastAccessTime = timestamp.second;
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsutil_openfsEntry(SuperBlock* superBlock, ConstCstring path, fsEntry* entryOut, FCNTLopenFlags flags) {
    fsEntryIdentifier identifier;
    fsEntryIdentifier_initStruct(&identifier, path, TEST_FLAGS(flags, FCNTL_OPEN_DIRECTORY));
    ERROR_GOTO_IF_ERROR(0);

    superBlock_openfsEntry(superBlock, &identifier, entryOut, flags);
    ERROR_GOTO_IF_ERROR(1);
    fsEntryIdentifier_clearStruct(&identifier);

    return;
    ERROR_FINAL_BEGIN(0);
    return;
    ERROR_FINAL_BEGIN(1);
    fsEntryIdentifier_clearStruct(&identifier); //TODO: Need a method to identify is identifier active
    ERROR_GOTO(0);
}

void fsutil_closefsEntry(fsEntry* entry) {
    superBlock_closefsEntry(entry); //Error passthrough
}

void fsutil_lookupEntryDesc(Directory* directory, ConstCstring name, bool isDirectory, fsEntryDesc* descOut, Size* entrySizeOut) {
    fsEntryDesc tmpDesc;
    Size entrySize;
    fsEntry_rawSeek(directory, 0);
    bool found = false;
    while (!found) {
        bool noMoreEntry = fsEntry_rawDirReadEntry(directory, &tmpDesc, &entrySize);
        ERROR_GOTO_IF_ERROR(0);
        if (noMoreEntry) {  //Meaning no more entries
            ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
        }

        if (cstring_strcmp(name, tmpDesc.identifier.name.data) == 0 && isDirectory == tmpDesc.identifier.isDirectory) {
            if (descOut != NULL) {
                memory_memcpy(descOut, &tmpDesc, sizeof(fsEntryDesc));
            }

            if (entrySizeOut != NULL) {
                *entrySizeOut = entrySize;
            }
            found = true;
            break;
        }

        fsEntryDesc_clearStruct(&tmpDesc);
        fsEntry_rawSeek(directory, directory->pointer + entrySize);
    }

    if (!found) {
        ERROR_THROW(ERROR_ID_NOT_FOUND, 0);
    }
    return;
    ERROR_FINAL_BEGIN(0);
}

void fsutil_loacateRealIdentifier(SuperBlock* superBlock, fsEntryIdentifier* identifier, SuperBlock** superBlockOut, fsEntryIdentifier* identifierOut) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifierOut));
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        fsEntryIdentifier_copy(identifierOut, identifier);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    String fullPath;
    string_initStruct(&fullPath, identifier->parentPath.data);
    ERROR_GOTO_IF_ERROR(0);
    string_append(&fullPath, &fullPath, FS_PATH_SEPERATOR);
    ERROR_GOTO_IF_ERROR(0);
    string_concat(&fullPath, &fullPath, &identifier->name);
    ERROR_GOTO_IF_ERROR(0);

    SuperBlock* currentSuperBlock = superBlock, * nextSuperBlock = NULL;
    Index64 pathSplit = INVALID_INDEX;
    fsEntryDesc* mountedToDesc;

    String tmp;
    while (true) {
        bool noMoreMount = superBlock_stepSearchMount(currentSuperBlock, &fullPath, &pathSplit, &nextSuperBlock, &mountedToDesc);
        ERROR_GOTO_IF_ERROR(0);
        if (noMoreMount) {
            break;
        }
        
        string_initStruct(&tmp, "");
        ERROR_GOTO_IF_ERROR(0);
        if (mountedToDesc != nextSuperBlock->rootDirDesc) {
            string_append(&tmp, &mountedToDesc->identifier.parentPath, FS_PATH_SEPERATOR);
            ERROR_GOTO_IF_ERROR(0);
            string_concat(&tmp, &tmp, &mountedToDesc->identifier.name);
            ERROR_GOTO_IF_ERROR(0);
        }

        string_slice(&fullPath, &fullPath, pathSplit, -1);
        ERROR_GOTO_IF_ERROR(0);
        string_concat(&fullPath, &tmp, &fullPath);
        ERROR_GOTO_IF_ERROR(0);
        
        string_clearStruct(&tmp);
        currentSuperBlock = nextSuperBlock;
    }

    *superBlockOut = currentSuperBlock;
    fsEntryIdentifier_initStruct(identifierOut, fullPath.data, identifier->isDirectory);
    ERROR_GOTO_IF_ERROR(0);
    string_clearStruct(&fullPath);
    
    return;
    ERROR_FINAL_BEGIN(0);
    if (STRING_IS_AVAILABLE(&tmp)) {
        string_clearStruct(&tmp);
    }

    if (STRING_IS_AVAILABLE(&fullPath)) {
        string_clearStruct(&fullPath);
    }
}

void fsutil_seekLocalFSentryDesc(SuperBlock* superBlock, fsEntryIdentifier* identifier, fsEntryDesc** descOut) {
    DEBUG_ASSERT_SILENT(fsEntryIdentifier_isActive(identifier));
    if (identifier->name.length == 0 && identifier->parentPath.length == 0) {
        *descOut = superBlock->rootDirDesc;
        return;
    }

    fsEntry currentDirectory;
    fsEntryDesc currentEntryDesc, nextEntryDesc;
    memory_memcpy(&currentEntryDesc, superBlock->rootDirDesc, sizeof(fsEntryDesc));

    String fullPath;
    string_initStruct(&fullPath, identifier->parentPath.data);
    ERROR_GOTO_IF_ERROR(0);
    string_append(&fullPath, &fullPath, FS_PATH_SEPERATOR);
    ERROR_GOTO_IF_ERROR(0);
    string_concat(&fullPath, &fullPath, &identifier->name);
    ERROR_GOTO_IF_ERROR(0);

    ConstCstring cFullPath = fullPath.data, name = cFullPath;
    fsEntryIdentifier tmpIdentifier;
    while (true) {
        if (*name != FS_PATH_SEPERATOR) {
            ERROR_THROW(ERROR_ID_DATA_ERROR, 0);
        }
        ++name;

        ConstCstring next = name;
        while (*next != FS_PATH_SEPERATOR && *next != '\0') {
            ++next;
        }
        
        fsEntryIdentifier_initStructSepN(
            &tmpIdentifier,
            cFullPath, ARRAY_POINTER_TO_INDEX(cFullPath, name) - 1,
            name, ARRAY_POINTER_TO_INDEX(name, next),
            *next == '\0' ? identifier->isDirectory : true
        );
        ERROR_GOTO_IF_ERROR(0);

        superBlock_rawOpenfsEntry(superBlock, &currentDirectory, &currentEntryDesc, FCNTL_OPEN_FILE_DEFAULT_FLAGS);
        ERROR_GOTO_IF_ERROR(0);

        fsutil_lookupEntryDesc(&currentDirectory, tmpIdentifier.name.data, tmpIdentifier.isDirectory, &nextEntryDesc, NULL);
        ERROR_GOTO_IF_ERROR(0);

        fsEntryIdentifier_clearStruct(&tmpIdentifier);
        superBlock_rawClosefsEntry(superBlock, &currentDirectory);
        ERROR_GOTO_IF_ERROR(0);

        if (*next == '\0') {
            DEBUG_ASSERT_SILENT(*descOut != NULL);
            memory_memcpy(*descOut, &nextEntryDesc, sizeof(fsEntryDesc));
            break;
        }
        
        name = next;
        memory_memcpy(&currentEntryDesc, &nextEntryDesc, sizeof(fsEntryDesc));
    }

    return;
    ERROR_FINAL_BEGIN(0);
    if (fsEntryIdentifier_isActive(&tmpIdentifier)) {
        fsEntryIdentifier_clearStruct(&tmpIdentifier);
    }
}