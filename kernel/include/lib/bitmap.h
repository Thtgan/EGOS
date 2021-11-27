#if !defined(__BITMAP_H)
#define __BITMAP_H

#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

/**
 * @brief A structure refer to a set of bit
 */
struct Bitmap {
    size_t bitSize; //Total bits contained in map
    size_t size;    //Num of bits set to 1
    uint8_t* bitPtr;
};

typedef struct Bitmap Bitmap;

/**
 * @brief Initialize the bitmap, once initialized, it should be truncated or extended
 * 
 * @param b Bitmap
 * @param bitSize Bit num contained by bitmap
 * @param bitPtr Pointer to bits
 */
void initBitmap(Bitmap* b, size_t bitSize, void* bitPtr);

/**
 * @brief Test if the bit on the index is set
 * 
 * @param b Bitmap
 * @param index Index of the bit to test
 * @return true If the bit is set
 * @return false If the bit is not set, or the index is out of range
 */
bool testBit(Bitmap* b, size_t index);

/**
 * @brief Set bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to set
 * @return true If the bit is set successfully
 * @return false If the bit not set successfully
 */
bool setBit(Bitmap* b, size_t index);

/**
 * @brief Set n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to set
 * @return true If the bits are set successfully
 * @return false If the bits not set successfully
 */
bool setBits(Bitmap* b, size_t index, size_t n);

/**
 * @brief Clear the bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to clear
 * @return true If the bit is cleared successfully
 * @return false If the bit not cleared successfully
 */
bool clearBit(Bitmap* b, size_t index);

/**
 * @brief Clear n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to clear
 * @return true If the bits are cleared successfully
 * @return false If the bits not cleared successfully
 */
bool clearBits(Bitmap* b, size_t index, size_t n);

#endif // __BITMAP_H
