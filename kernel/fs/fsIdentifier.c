#include<fs/fsIdentifier.h>

#include<cstring.h>
#include<lib/errorPosix.h>
#include<fs/fsNode.h>
#include<fs/vnode.h>
#include<fs/path.h>
#include<kit/types.h>
#include<structs/string.h>

void fsIdentifier_initStruct(fsIdentifier* identifier, vNode* baseVnode, ConstCstring path, bool isDirectory) {
    identifier->baseVnode = baseVnode;
    string_initStructStr(&identifier->path, path);
    CHECK_ERROR(error_out);
    identifier->isDirectory = isDirectory;

    return;
error_out:
}

void fsIdentifier_clearStruct(fsIdentifier* identifier) {
    string_clearStruct(&identifier->path);
}

void fsIdentifier_getAbsolutePath(fsIdentifier* identifier, String* pathOut) {
    DEBUG_ASSERT_SILENT(string_isAvailable(pathOut));
    string_clear(pathOut);

    fsnode_getAbsolutePath(identifier->baseVnode->fsNode, pathOut);
    CHECK_ERROR(error_out);
    path_join(pathOut, pathOut, &identifier->path);
    CHECK_ERROR(error_out);
    path_normalize(pathOut);
    CHECK_ERROR(error_out);
    
    return;
error_out:
}
