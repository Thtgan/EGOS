#if !defined(__PIT_H)
#define __PIT_H

#include<devices/timer/timer.h>
#include<stdint.h>

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
void configureChannel(uint8_t channel, uint8_t mode, uint32_t frequency);

/**
 * @brief Read back the configuration of the channel
 * 
 * @param channel Channel to read
 * @return uint8_t Channel configuration
 */
uint8_t readbackConfiguration(uint8_t channel);

/**
 * @brief Pause the executation for a period of time
 * 
 * @param unit Time unit
 * @param time Size of time
 */
void PITsleep(TimeUnit unit, uint64_t time);

#endif // __PIT_H
