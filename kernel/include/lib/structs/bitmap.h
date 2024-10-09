#if !defined(__LIB_STRUCTS_BITMAP_H)
#define __LIB_STRUCTS_BITMAP_H

typedef struct Bitmap Bitmap;

#include<kit/types.h>

/**
 * @brief A structure refer to a set of bit
 */
typedef struct Bitmap {
    Size bitNum;        //Total bits contained in map
    Size bitSetNum;     //Num of bits set to 1
    Uint8* bitPtr;
} Bitmap;

/**
 * @brief Initialize the bitmap, once initialized, it should be truncated or extended
 * 
 * @param b Bitmap
 * @param bitSize Bit num contained by bitmap
 * @param bitPtr Pointer to bits
 */
void bitmap_initStruct(Bitmap* b, Size bitSize, void* bitPtr);

/**
 * @brief Test if the bit on the index is set
 * 
 * @param b Bitmap
 * @param index Index of the bit to test
 * @return If the bit is set
 */
bool bitmap_testBit(Bitmap* b, Index64 index);

/**
 * @brief Set bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to set
 */
void bitmap_setBit(Bitmap* b, Size index);

/**
 * @brief Set n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to set
 */
void bitmap_setBits(Bitmap* b, Size index, Size n);

/**
 * @brief Clear the bit on the index
 * 
 * @param b Bitmap
 * @param index Index of the bit to clear
 */
void bitmap_clearBit(Bitmap* b, Size index);

/**
 * @brief Clear n bits start from the index
 * 
 * @param b Bitmap
 * @param index Index of the beginning bit
 * @param n Num of bits to clear
 */
void bitmap_clearBits(Bitmap* b, Size index, Size n);

/**
 * @brief Find first bit set
 * 
 * @param b Bitmap
 * @param begin Begin of the search
 * @return Size Index of first bit set, INVALID_INDEX if not exist
 */
Index64 bitmap_findFirstSet(Bitmap* b, Size begin);

/**
 * @brief Find first bit clear
 * 
 * @param b Bitmap
 * @param begin Begin of the search
 * @return Size Index of first bit clear, INVALID_INDEX if not exist
 */
Index64 bitmap_findFirstClear(Bitmap* b, Size begin);

#endif // __LIB_STRUCTS_BITMAP_H
