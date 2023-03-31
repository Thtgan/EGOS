#if !defined(__FS_MANAGER_H)
#define __FS_MANAGER_H

#include<fs/fileSystem.h>
#include<kit/types.h>

void initFSManager();

bool registerDeviceFS(FileSystem* fs);

FileSystem* unregisterDeviceFS(ID device);

FileSystem* getDeviceFS(ID device);

#endif // __FS_MANAGER_H