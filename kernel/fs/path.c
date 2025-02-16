#include<fs/path.h>

#include<kit/types.h>
#include<memory/memory.h>
#include<structs/string.h>
#include<debug.h>
#include<error.h>

static bool __path_isPathValid(String* path);

static inline Index64 __path_step(String* path, Index64 begin) {
    Index64 index = begin;
    while (index < path->length && path->data[index] != PATH_SEPERATOR) {
        ++index;
    }
    return index;
}

bool path_isAbsolute(String* path) {
    DEBUG_ASSERT_SILENT(string_isAvailable(path));
    return path->length > 0 && path->data[0] == PATH_SEPERATOR;
}

void path_join(String* des, String* base, String* path) {
    DEBUG_ASSERT_SILENT(string_isAvailable(des));
    DEBUG_ASSERT_SILENT(string_isAvailable(base));
    DEBUG_ASSERT_SILENT(string_isAvailable(path));

    if (base->length == 0 && path->length == 0) {
        string_clear(des);
        return;
    } else if (base->length == 0) {
        string_copy(des, path);
        ERROR_GOTO_IF_ERROR(0);
        return;
    } else if (path->length == 0) {
        string_copy(des, base);
        ERROR_GOTO_IF_ERROR(0);
        return;
    }

    if (des != base) {
        string_copy(des, base);
        ERROR_GOTO_IF_ERROR(0);
    }

    if (des->data[des->length - 1] == PATH_SEPERATOR && path->data[0] == PATH_SEPERATOR) {
        string_slice(des, des, 0, des->length - 1);
        ERROR_GOTO_IF_ERROR(0);
    } else if (des->data[des->length - 1] != PATH_SEPERATOR && path->data[0] != PATH_SEPERATOR) {
        string_append(des, des, PATH_SEPERATOR);
        ERROR_GOTO_IF_ERROR(0);
    }
    
    string_concat(des, des, path);
    ERROR_GOTO_IF_ERROR(0);

    // if (path_isAbsolute(path)) {
    //     string_copy(des, path);
    //     ERROR_GOTO_IF_ERROR(0);
    // } else {
    //     if (des != base) {
    //         string_copy(des, base);
    //         ERROR_GOTO_IF_ERROR(0);
    //     }

    //     if (des->data[des->length - 1] != PATH_SEPERATOR) {
    //         string_append(des, path, PATH_SEPERATOR);
    //         ERROR_GOTO_IF_ERROR(0);
    //     }

    //     string_append(des, des, PATH_SEPERATOR);
    //     ERROR_GOTO_IF_ERROR(0);
    // }

    return;
    ERROR_FINAL_BEGIN(0);
}

Index64 path_walk(String* path, Index64 beginIndex, String* walkedOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(path));
    if (beginIndex >= path->length) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Index64 index1 = beginIndex;
    if (path->data[index1] == PATH_SEPERATOR) {
        ++index1;
    }
    DEBUG_ASSERT_SILENT(!(index1 < path->length && path->data[index1] == PATH_SEPERATOR));
    Index64 index2 = index1;

    for (; index2 < path->length && path->data[index2] != PATH_SEPERATOR; ++index2);

    string_slice(walkedOut, path, index1, index2);

    // if (index2 < path->length && path->data[index2] != PATH_SEPERATOR) {
    //     ++index2;
    // }
    
    return index2 == path->length ? INVALID_INDEX : index2;
    ERROR_FINAL_BEGIN(0);
    return INVALID_INDEX;
}

void path_normalize(String* path) {
    DEBUG_ASSERT_SILENT(string_isAvailable(path) && path->length > 0 && path->data[0] == PATH_SEPERATOR);
    if (!__path_isPathValid(path)) {
        ERROR_THROW(ERROR_ID_ILLEGAL_ARGUMENTS, 0); //TODO: Invalid format error?
    }

    Index64 index1 = 0, index2 = 0;

#define __PATH_NORMALIZE_SITUATION_REGULAR              0
#define __PATH_NORMALIZE_SITUATION_CURRENT_DIRECTORY    1
#define __PATH_NORMALIZE_SITUATION_LAST_DIRECTORY       2
    int situation = 0;
    while (index1 < path->length) {
        if (path->data[index1] == PATH_SEPERATOR) {
            path->data[index2++] = PATH_SEPERATOR;
            while (index1 < path->length && path->data[index1] == PATH_SEPERATOR) {
                ++index1;
            }
        }

        Index64 nextIndex1 = __path_step(path, index1);
        Size partLength = nextIndex1 - index1;
        if (path->data[nextIndex1 - 1] == '.') {
            if (partLength == 1) {
                situation = __PATH_NORMALIZE_SITUATION_CURRENT_DIRECTORY;
            } else if (partLength == 2 && path->data[index1] == '.') {
                situation = __PATH_NORMALIZE_SITUATION_LAST_DIRECTORY;
            } else {
                ERROR_THROW(ERROR_ID_UNKNOWN, 0);
            }
        } else {
            situation = __PATH_NORMALIZE_SITUATION_REGULAR;
        }

        switch (situation) {
            case __PATH_NORMALIZE_SITUATION_REGULAR: {
                memory_memmove(path->data + index2, path->data + index1, partLength);
                index2 += partLength;
                break;
            }
            case __PATH_NORMALIZE_SITUATION_CURRENT_DIRECTORY: {
                break;
            }
            case __PATH_NORMALIZE_SITUATION_LAST_DIRECTORY: {
                if (index2 > 0) {
                    DEBUG_ASSERT_SILENT(index2 >= 2);
                    index2 -= 2;
                    for (; index2 > 0 && path->data[index2 - 1] != PATH_SEPERATOR; --index2);
                }
                break;
            }
            default: {
                ERROR_THROW(ERROR_ID_UNKNOWN, 0);
            }
        }

        index1 = nextIndex1;
    }

    while (index2 > 1 && path->data[index2 - 1] == PATH_SEPERATOR) {
        --index2;
    }

    path->length = index2;

    return;
    ERROR_FINAL_BEGIN(0);
}

void path_basename(String* path, String* basenameOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(path));
    DEBUG_ASSERT_SILENT(string_isAvailable(basenameOut));

    if (path->length == 0 || path->data[path->length - 1] == PATH_SEPERATOR) {
        string_clear(basenameOut);
        return;
    }
    
    Index64 begin = path->length - 1;
    while (begin > 0 && path->data[begin] != PATH_SEPERATOR) {
        --begin;
    }

    string_slice(basenameOut, path, begin + 1, path->length);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

void path_dirname(String* path, String* dirnameOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(path));
    DEBUG_ASSERT_SILENT(string_isAvailable(dirnameOut));

    if (path->length == 0 || path->data[path->length - 1] == PATH_SEPERATOR) {
        string_clear(dirnameOut);
        return;
    }
    
    Index64 end = path->length - 1;
    while (end > 0 && path->data[end] != PATH_SEPERATOR) {
        --end;
    }

    string_slice(dirnameOut, path, 0, end);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static bool __path_isPathValid(String* path) {
    for (Index64 i = 0; i < path->length; ) {
        while (i < path->length && path->data[i] == PATH_SEPERATOR) {
            ++i;
        }

        if (i == path->length) {
            break;
        }

        Index64 nextI = __path_step(path, i);
        if (path->data[nextI - 1] == '.') {
            if (nextI - i != 1 && !(nextI - i == 2 && path->data[i] == '.')) {
                return false;
            }
        }

        i = nextI;
    }

    return true;
}