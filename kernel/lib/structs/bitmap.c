#include<lib/structs/bitmap.h>

#include<kit/bit.h>

void initBitmap(Bitmap* b, const size_t bitSize, void* bitPtr) {
    b->bitSize = bitSize;
    b->bitPtr = bitPtr;
    b->size = 0;
}

#define __CHECK_INDEX(__INDEX, __LIMIT) (0 <= __INDEX && __INDEX < __LIMIT)                                 //Check is index out of range
#define __RAW_SET(__PTR, __INDEX) SET_FLAG_BACK(__PTR[index >> 3], FLAG8(TRIM_VAL(__INDEX, 0x7)));          //Set the bit
#define __RAW_CLEAR(__PTR, __INDEX) CLEAR_FLAG_BACK(__PTR[index >> 3], FLAG8(TRIM_VAL(__INDEX, 0x7)));      //Clear the bit
#define __RAW_TEST(__PTR, __INDEX) TEST_FLAGS_CONTAIN(__PTR[__INDEX >> 3], FLAG8(TRIM_VAL(__INDEX, 0x7)))   //Test the bit

bool testBit(Bitmap* b, const size_t index) {
    if (!__CHECK_INDEX(index, b->bitSize)) {
        return false;
    }

    return __RAW_TEST(b->bitPtr, index);
}

bool setBit(Bitmap* b, const size_t index) {
    return setBits(b, index, 1);
}

bool setBits(Bitmap* b, const size_t index, const size_t n) {
    size_t endIndex = index + n - 1;

    if (!__CHECK_INDEX(index, b->bitSize) || !__CHECK_INDEX(endIndex, b->bitSize)) { //Only check the beginning and the ending
        return false;
    }

    for (size_t i = index; i <= endIndex; ++i) {

        if (__RAW_TEST(b->bitPtr, i)) {
            continue; //Skip the bit already set
        }

        __RAW_SET(b->bitPtr, i);
        ++b->size;
    }
    return true;
}

bool clearBit(Bitmap* b, const size_t index) {
    return clearBits(b, index, 1);
}

bool clearBits(Bitmap* b, const size_t index, const size_t n) {
    size_t endIndex = index + n - 1;

    if (!__CHECK_INDEX(index, b->bitSize) || !__CHECK_INDEX(endIndex, b->bitSize)) { //Only check the beginning and the ending
        return false;
    }

    for (size_t i = index; i <= endIndex; ++i) {

        if (!__RAW_TEST(b->bitPtr, i)) {
            continue;
        }

        __RAW_CLEAR(b->bitPtr, i);
        --b->size;
    }
    return true;
}