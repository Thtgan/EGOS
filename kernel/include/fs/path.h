#if !defined(__FS_PATH_H)
#define __FS_PATH_H

#include<kit/types.h>
#include<structs/string.h>

#define PATH_SEPERATOR '/'
#define PATH_SEPERATOR_STR "/"

bool path_isAbsolute(String* path);

void path_join(String* des, String* base, String* path);

Index64 path_walk(String* path, Index64 beginIndex, String* walkedOut);

void path_normalize(String* path);

Object path_hash(String* path);

Object path_hashAppend(Object* hash, String* appendPath);

void path_basename(String* path, String* basenameOut);

void path_dirname(String* path, String* dirnameOut);

#endif // __FS_PATH_H
