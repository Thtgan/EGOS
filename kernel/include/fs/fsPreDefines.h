#if !defined(__FS_PRE_DEFINES)
#define __FS_PRE_DEFINES

#include<kit/oop.h>

STRUCT_PRE_DEFINE(SuperBlock);
STRUCT_PRE_DEFINE(iNode);
STRUCT_PRE_DEFINE(FSentry);
STRUCT_PRE_DEFINE(FSentryDesc);
STRUCT_PRE_DEFINE(FSentryIdentifier);

typedef FSentry Directory;
typedef FSentry File;

#endif // __FS_PRE_DEFINES
