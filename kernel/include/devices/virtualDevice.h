#if !defined(__VIRTUAL_DEVICE_H)
#define __VIRTUAL_DEVICE_H

// #include<kit/types.h>
// #include<fs/file.h>

// typedef struct {
//     FileOperations* fileOperations;
//     void* device;
// } VirtualDeviceINodeData;

// /**
//  * @brief Initialize virtual devices, create a folder in path '/dev' which is mounted on memory block device to manage devices, sets errorcode to indicate error
//  * 
//  * @return Result Result of the operation
//  */
// Result initVirtualDevices();

// /**
//  * @brief Close vitual devices, sets errorcode to indicate error
//  * 
//  * @return Result Result of the operation
//  */
// Result closeVitualDevices();

// /**
//  * @brief Register device as a virtual device can be accessed by file operations, sets errorcode to indicate error
//  * 
//  * @param device Device to register
//  * @param name Name of device
//  * @param operations Operations of devices, in file operation format
//  * @return Result Result of the operation
//  */
// Result registerVirtualDevice(void* device, ConstCstring name, FileOperations* operations);

// /**
//  * @brief Get file refering to standard output
//  * 
//  * @return File* Standard output file
//  */
// File* getStandardOutputFile();

#endif // __VIRTUAL_DEVICE_H
