#if !defined(__VIRTUAL_DEVICE_H)
#define __VIRTUAL_DEVICE_H

#include<kit/types.h>

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

#endif // __VIRTUAL_DEVICE_H
