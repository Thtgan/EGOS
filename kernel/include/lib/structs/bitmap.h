#if !defined(__BITMAP_H)
#define __BITMAP_H

#include<stdbool.h>
#include<stddef.h>
#include<stdint.h>

/**
 * @brief A structure refer to a set of bit
 */
typedef struct {
    size_t bitNum;      //Total bits contained in map
    size_t bitSetNum;   //Num of bits set to 1
    uint8_t* bitPtr;
} Bitmap;

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
 * @return If the bit is set
 */
bool testBit(Bitmap* b, size_t index);

/**
 * @brief Set bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to set
 */
void setBit(Bitmap* b, size_t index);

/**
 * @brief Set n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to set
 */
void setBits(Bitmap* b, size_t index, size_t n);

/**
 * @brief Clear the bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to clear
 */
void clearBit(Bitmap* b, size_t index);

/**
 * @brief Clear n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to clear
 */
void clearBits(Bitmap* b, size_t index, size_t n);

size_t findFirstSet(Bitmap* b, size_t begin);

size_t findFirstClear(Bitmap* b, size_t begin);

#endif // __BITMAP_H
