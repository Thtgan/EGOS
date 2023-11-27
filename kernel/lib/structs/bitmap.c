#include<structs/bitmap.h>

#include<kit/bit.h>
#include<kit/types.h>

void bitmap_initStruct(Bitmap* b, Size bitSize, void* bitPtr) {
    b->bitNum = bitSize;
    b->bitSetNum = 0;
    b->bitPtr = bitPtr;
}

#define __BITMAP_RAW_SET(__PTR, __INDEX)   SET_FLAG_BACK(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))        //Set the bit
#define __BITMAP_RAW_CLEAR(__PTR, __INDEX) CLEAR_FLAG_BACK(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))      //Clear the bit
#define __BITMAP_RAW_TEST(__PTR, __INDEX)  TEST_FLAGS_CONTAIN(__PTR[EXTRACT_VAL(__INDEX, 64, 3, 64)], FLAG8(TRIM_VAL(__INDEX, 0x7)))   //Test the bit

bool bitmap_testBit(Bitmap* b, const Size index) {
    return __BITMAP_RAW_TEST(b->bitPtr, index);
}

void bitmap_setBit(Bitmap* b, const Size index) {
    bitmap_setBits(b, index, 1);
}

void bitmap_setBits(Bitmap* b, const Size index, const Size n) {
    Size endIndex = index + n - 1;

    for (Size i = index; i <= endIndex; ++i) {

        if (__BITMAP_RAW_TEST(b->bitPtr, i)) {
            continue; //Skip the bit already set
        }

        __BITMAP_RAW_SET(b->bitPtr, i);
        ++b->bitSetNum;
    }
}

void bitmap_clearBit(Bitmap* b, const Size index) {
    bitmap_clearBits(b, index, 1);
}

void bitmap_clearBits(Bitmap* b, const Size index, const Size n) {
    Size endIndex = index + n - 1;

    for (Size i = index; i <= endIndex; ++i) {

        if (!__BITMAP_RAW_TEST(b->bitPtr, i)) {
            continue;
        }

        __BITMAP_RAW_CLEAR(b->bitPtr, i);
        --b->bitSetNum;
    }
}

Size bitmap_findFirstSet(Bitmap* b, Size begin) {
    if (b->bitSetNum == 0) {
        return INVALID_INDEX;
    }

    Uint64* ptr = (void*)&b->bitPtr[begin >> 3];

    Size i = begin, limit = CLEAR_VAL_SIMPLE(begin, 64, 6) + 64;
    for (; i < limit && i < b->bitNum; ++i) {
        if (__BITMAP_RAW_TEST(b->bitPtr, i)) {
            return i;
        }
    }

    i += 64;

    while (i < b->bitNum && *ptr == EMPTY_FLAGS) {
        ++ptr;
        i += 64;
    }

    for (; i < b->bitNum; ++i) {
        if (__BITMAP_RAW_TEST(b->bitPtr, i)) {
            return i;
        }
    }

    return INVALID_INDEX;
}

Size bitmap_findFirstClear(Bitmap* b, Size begin) {
    if (b->bitSetNum == b->bitNum) {
        return INVALID_INDEX;
    }

    Uint64* ptr = (void*)&b->bitPtr[begin >> 3];

    Size i = begin, limit = CLEAR_VAL_SIMPLE(begin, 64, 6) + 64;
    for (; i < limit && i < b->bitNum; ++i) {
        if (!__BITMAP_RAW_TEST(b->bitPtr, i)) {
            return i;
        }
    }

    i += 64;

    while (i < b->bitNum && *ptr == FULL_MASK(64)) {
        ++ptr;
        i += 64;
    }

    for (; i < b->bitNum; ++i) {
        if (!__BITMAP_RAW_TEST(b->bitPtr, i)) {
            return i;
        }
    }

    return INVALID_INDEX;
}