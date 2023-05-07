#if !defined(__FS_MANAGER_H)
#define __FS_MANAGER_H

#include<fs/fileSystem.h>
#include<kit/types.h>

/**
 * @brief Initialize file system manager (The buffer connects device and its mounted file system)
 */
void initFSManager();

/**
 * @brief Register file system to manager
 * 
 * @param fs File system to register
 * @return Result Result of the operation
 */
Result registerDeviceFS(FileSystem* fs);

/**
 * @brief Unregister file system
 * 
 * @param device Device where file system deployed to
 * @return FileSystem* Unregistered file system, NULL if not exist
 */
FileSystem* unregisterDeviceFS(ID device);

/**
 * @brief Get file system from device its mounted at
 * 
 * @param device Device where file system is mounted
 * @return FileSystem* file system returned, NULL if not exist
 */
FileSystem* getDeviceFS(ID device);

#endif // __FS_MANAGER_H