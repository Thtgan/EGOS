#include<lib/structs/bitmap.h>

#include<kit/bit.h>

void initBitmap(Bitmap* b, size_t bitSize, void* bitPtr) {
    b->bitSize = bitSize;
    b->bitPtr = bitPtr;
    b->size = 0;
}

#define __RAW_TEST(__PTR, __INDEX) TEST_FLAGS_CONTAIN(__PTR[__INDEX >> 3], FLAG8(TRIM_VAL(__INDEX, 0x7)))
#define __CHECK_INDEX(__INDEX, __LIMIT) (0 <= __INDEX && __INDEX < __LIMIT)

bool testBit(Bitmap* b, size_t index) {
    if (!__CHECK_INDEX(index, b->bitSize)) {
        return false;
    }

    return __RAW_TEST(b->bitPtr, index);
}

bool setBit(Bitmap* b, size_t index) {
    if (!__CHECK_INDEX(index, b->bitSize)) {
        return false;
    }

    if (!__RAW_TEST(b->bitPtr, index)) {
        SET_FLAG_BACK(b->bitPtr[index >> 3], FLAG8(TRIM_VAL(index, 0x7)));
        ++b->size;
    }
    
    return true;
}

bool setBits(Bitmap* b, size_t index, size_t n) {
    size_t endIndex = index + n - 1;

    if (!__CHECK_INDEX(index, b->bitSize) || !__CHECK_INDEX(endIndex, b->bitSize)) {
        return false;
    }

    for (int i = index; i <= endIndex; ++i) {

        if (__RAW_TEST(b->bitPtr, i)) {
            continue;
        }

        SET_FLAG_BACK(b->bitPtr[i >> 3], FLAG8(TRIM_VAL(i, 0x7)));
        ++b->size;
    }
    return true;
}

bool clearBit(Bitmap* b, size_t index) {
    if (!__CHECK_INDEX(index, b->bitSize)) {
        return false;
    }

    if (__RAW_TEST(b->bitPtr, index)) {
        CLEAR_FLAG_BACK(b->bitPtr[index >> 3], FLAG8(TRIM_VAL(index, 0x7)));
        --b->size;
    }
    return true;
}

bool clearBits(Bitmap* b, size_t index, size_t n) {
    size_t endIndex = index + n - 1;

    if (!__CHECK_INDEX(index, b->bitSize) || !__CHECK_INDEX(endIndex, b->bitSize)) {
        return false;
    }

    for (int i = index; i <= endIndex; ++i) {

        if (!__RAW_TEST(b->bitPtr, i)) {
            continue;
        }

        CLEAR_FLAG_BACK(b->bitPtr[i >> 3], FLAG8(TRIM_VAL(i, 0x7)));
        --b->size;
    }
    return true;
}