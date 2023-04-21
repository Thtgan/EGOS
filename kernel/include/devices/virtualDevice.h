#if !defined(__VIRTUAL_DEVICE_H)
#define __VIRTUAL_DEVICE_H

#include<kit/types.h>
#include<fs/file.h>

typedef struct {
    FileOperations* fileOperations;
    void* device;
} VirtualDeviceINodeData;

/**
 * @brief Initialize virtual devices, create a folder in path '/dev' which is mounted on memory block device to manage devices
 * 
 * @return int 0 if succeeded
 */
int initVirtualDevices();

/**
 * @brief Close vitual devices
 * 
 * @return int 0 if succeeded
 */
int closeVitualDevices();

/**
 * @brief Register device as a virtual device can be accessed by file operations
 * 
 * @param device Device to register
 * @param name Name of device
 * @param operations Operations of devices, in file operation format
 * @return int 0 if succeeded
 */
int registerVirtualDevice(void* device, ConstCstring name, FileOperations* operations);

#endif // __VIRTUAL_DEVICE_H
