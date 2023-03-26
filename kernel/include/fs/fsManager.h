#include<fs/fileSystem.h>
#include<devices/block/blockDevice.h>

void initFSManager();

bool registerDeviceFS(BlockDevice* device, FileSystem* fs);

FileSystem* unregisterDeviceFS(BlockDevice* device);

FileSystem* getDeviceFS(BlockDevice* device);