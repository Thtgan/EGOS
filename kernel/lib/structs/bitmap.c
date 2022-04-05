#include<structs/bitmap.h>

#include<kit/bit.h>

//TODO: Make a malloc version?
void initBitmap(Bitmap* b, size_t bitSize, void* bitPtr) {
    b->bitNum = bitSize;
    b->bitSetNum = 0;
    b->bitPtr = bitPtr;
}

#define __RAW_SET(__PTR, __INDEX)   SET_FLAG_BACK(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))        //Set the bit
#define __RAW_CLEAR(__PTR, __INDEX) CLEAR_FLAG_BACK(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))      //Clear the bit
#define __RAW_TEST(__PTR, __INDEX)  TEST_FLAGS_CONTAIN(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))   //Test the bit

bool testBit(Bitmap* b, const size_t index) {
    return __RAW_TEST(b->bitPtr, index);
}

void setBit(Bitmap* b, const size_t index) {
    setBits(b, index, 1);
}

void setBits(Bitmap* b, const size_t index, const size_t n) {
    size_t endIndex = index + n - 1;

    for (size_t i = index; i <= endIndex; ++i) {

        if (__RAW_TEST(b->bitPtr, i)) {
            continue; //Skip the bit already set
        }

        __RAW_SET(b->bitPtr, i);
        ++b->bitSetNum;
    }
}

void clearBit(Bitmap* b, const size_t index) {
    clearBits(b, index, 1);
}

void clearBits(Bitmap* b, const size_t index, const size_t n) {
    size_t endIndex = index + n - 1;

    for (size_t i = index; i <= endIndex; ++i) {

        if (!__RAW_TEST(b->bitPtr, i)) {
            continue;
        }

        __RAW_CLEAR(b->bitPtr, i);
        --b->bitSetNum;
    }
}