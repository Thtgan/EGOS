#if !defined(__FS_LOCATE_H)
#define __FS_LOCATE_H

#include<fs/fcntl.h>
#include<fs/fsIdentifier.h>
#include<fs/fsNode.h>
#include<fs/fscore.h>
#include<kit/types.h>

fsNode* locate(fsIdentifier* identifier, FCNTLopenFlags flags, vNode** parentDirVnodeOut, FScore** finalFScoreOut);

#endif // __FS_LOCATE_H
