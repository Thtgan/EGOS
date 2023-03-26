#if !defined(__FS_MANAGER_H)
#define __FS_MANAGER_H

#include<fs/fileSystem.h>
#include<devices/block/blockDevice.h>

void initFSManager();

bool registerDeviceFS(BlockDevice* device, FileSystem* fs);

FileSystem* unregisterDeviceFS(BlockDevice* device);

FileSystem* getDeviceFS(BlockDevice* device);

#endif // __FS_MANAGER_H