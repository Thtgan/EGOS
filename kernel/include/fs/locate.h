#if !defined(__FS_LOCATE_H)
#define __FS_LOCATE_H

#include<fs/fcntl.h>
#include<fs/fsIdentifier.h>
#include<fs/fsNode.h>
#include<fs/superblock.h>
#include<kit/types.h>

fsNode* locate(fsIdentifier* identifier, FCNTLopenFlags flags, iNode** parentDirInodeOut, SuperBlock** finalSuperBlockOut);

#endif // __FS_LOCATE_H
