#include<fs/fsIdentifier.h>

#include<cstring.h>
#include<error.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<fs/path.h>
#include<kit/types.h>
#include<structs/string.h>

void fsIdentifier_initStruct(fsIdentifier* identifier, vNode* baseVnode, ConstCstring path, bool isDirectory) {
    identifier->baseVnode = baseVnode;
    string_initStructStr(&identifier->path, path);
    ERROR_GOTO_IF_ERROR(0);
    identifier->isDirectory = isDirectory;

    return;
    ERROR_FINAL_BEGIN(0);
}

void fsIdentifier_clearStruct(fsIdentifier* identifier) {
    string_clearStruct(&identifier->path);
}

void fsIdentifier_getAbsolutePath(fsIdentifier* identifier, String* pathOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(pathOut));
    string_clear(pathOut);

    fsNode_getAbsolutePath(identifier->baseVnode->fsNode, pathOut);
    ERROR_GOTO_IF_ERROR(0);
    path_join(pathOut, pathOut, &identifier->path);
    ERROR_GOTO_IF_ERROR(0);
    path_normalize(pathOut);
    ERROR_GOTO_IF_ERROR(0);
    
    return;
    ERROR_FINAL_BEGIN(0);
}