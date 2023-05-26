#if !defined(__PIT_H)
#define __PIT_H

#include<devices/timer/timer.h>
#include<kit/types.h>

#define TICK_FREQUENCY 100u

/**
 * @brief Initialize the PIT
 */
void initPIT();

/**
 * @brief Configure the PIT channel with given mode and frequency
 * 
 * @param channel PIT channel
 * @param mode Channel mode
 * @param frequency Channel frequency
 */
void configureChannel(Uint8 channel, Uint8 mode, Uint32 frequency);

/**
 * @brief Read back the configuration of the channel
 * 
 * @param channel Channel to read
 * @return Uint8 Channel configuration
 */
Uint8 readbackConfiguration(Uint8 channel);

/**
 * @brief Pause the executation for a period of time
 * 
 * @param unit Time unit
 * @param time Size of time
 */
void PITsleep(TimeUnit unit, Uint64 time);

#endif // __PIT_H
