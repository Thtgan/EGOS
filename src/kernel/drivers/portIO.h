#pragma once

#include<stdint.h>

/**
 * Read a byte from a IO port
 * 
 * port: Port number
 * 
 * return: The byte read from the port
*/
uint8_t inb(uint16_t port);

/**
 * Write a byte to a IO port
 * 
 * port: Port number
 * value: Value to write
*/
void outb(uint16_t port, uint8_t value);

/**
 * Read a word from a IO port
 * 
 * port: Port number
 * 
 * return: The word read from the port
*/
uint16_t inw(uint16_t port);

/**
 * Write a word to a IO port
 * 
 * port: Port number
 * value: Value to write
*/
void outw(uint16_t port, uint16_t value);

/**
 * Read a long from a IO port
 * 
 * port: Port number
 * 
 * return: The long read from the port
*/
uint32_t inl(uint16_t port);

/**
 * Write a long to a IO port
 * 
 * port: Port number
 * value: Value to write
*/
void outl(uint16_t port, uint32_t value);