#if !defined(__PORTIO_H)
#define __PORTIO_H

#include<stdint.h>

/**
 * @brief Read a byte from a IO port
 * 
 * @param port Port number
 * 
 * @return uint8_t The byte read from the port
 */
uint8_t inb(uint16_t port);

/**
 * @brief Write a byte to a IO port
 * 
 * @param port Port number
 * @param value Value to write
 */
void outb(uint16_t port, uint8_t value);

/**
 * @brief Read a word from a IO port
 * 
 * @param port Port number
 * 
 * @return uint16_t The word read from the port
 */
uint16_t inw(uint16_t port);

/**
 * @brief Write a word to a IO port
 * 
 * @param port Port number
 * @param value Value to write
 */
void outw(uint16_t port, uint16_t value);

/**
 * @brief Read a long from a IO port
 * 
 * @param port Port number
 * 
 * @return uint32_t The long read from the port
 */
uint32_t inl(uint16_t port);

/**
 * @brief Write a long to a IO port
 * 
 * @param port Port number
 * @param value Value to write
 */
void outl(uint16_t port, uint32_t value);

#endif // __PORTIO_H