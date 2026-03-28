#include<devices/terminal/textBuffer.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<memory/paging.h>
#include<structs/loopArray.h>
#include<structs/vector.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<lib/errorPosix.h>

#define __TEXT_BUFFER_PART_POSITION_BUILD(__BEGIN_PAGE_INDEX, __BEGIN_PAGE_OFFSET)  (VAL_LEFT_SHIFT(__BEGIN_PAGE_INDEX, PAGE_SIZE_SHIFT) | TRIM_VAL_SIMPLE(__BEGIN_PAGE_OFFSET, 64, PAGE_SIZE_SHIFT))
#define __TEXT_BUFFER_PART_POSITION_GET_BYTE_INDEX(__POSITION)                      EXTRACT_VAL(__POSITION, 64, 0, 48)
#define __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(__POSITION)               EXTRACT_VAL(__POSITION, 64, 0, PAGE_SIZE_SHIFT)
#define __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(__POSITION)                EXTRACT_VAL(__POSITION, 64, PAGE_SIZE_SHIFT, 64)
#define __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL                                       FLAG64(63)
#define __TEXT_BUFFER_PART_ENTRY_BUILD(__POSITION, __FLAGS)                         ((__FLAGS) | (__POSITION))
#define __TEXT_BUFFER_PART_GET_POSITION(__ENTRY)                                    EXTRACT_VAL(__ENTRY, 64, 0, 48)

static void* __textBuffer_getDataPage(TextBuffer* textBuffer, Index64 position, bool createIfNotExist);

static void __textBuffer_enqueueDataPage(TextBuffer* textBuffer);

static void __textBuffer_dequeueDataPageFront(TextBuffer* textBuffer, Size releasePageNum);

static void __textBuffer_dequeueDataPageBack(TextBuffer* textBuffer, Size releasePageNum);

static Uint16 __textBuffer_pushDataToLastPart(TextBuffer* textBuffer, ConstCstring buffer, Size n);

static Uint16 __textBuffer_popDataFromLastPart(TextBuffer* textBuffer, Size n);

static void __textBuffer_popFirstPart(TextBuffer* textBuffer);

static inline void __textBuffer_pushNewPart(TextBuffer* textBuffer, Index64 newPartPosition) {
    loopArray_pushBack(&textBuffer->partEntries, __TEXT_BUFFER_PART_ENTRY_BUILD(newPartPosition, EMPTY_FLAGS)); //Error passthrough
    textBuffer->lastPartLen = 0;
}

static bool __textBuffer_pushNewPartWithPopFirst(TextBuffer* textBuffer, Index64 newPartPosition);

void textBuffer_initStruct(TextBuffer* textBuffer, Index32 maxPartNum, Uint16 maxPartLen) {
    textBuffer->maxPartNum = maxPartNum;
    textBuffer->maxPartLen = maxPartLen;
    loopArray_initStruct(&textBuffer->partEntries, maxPartNum);

    vector_initStruct(&textBuffer->partDataPages);
    CHECK_ERROR(error_out);
    
    __textBuffer_enqueueDataPage(textBuffer);
    CHECK_ERROR(error_out);

    textBuffer->totalByteNum = 0;
    
    __textBuffer_pushNewPart(textBuffer, __TEXT_BUFFER_PART_POSITION_BUILD(0, 0));  //Error passthrough

    return;
error_out:
}

void textBuffer_clearStruct(TextBuffer* textBuffer) {
    for (int i = 0; i < textBuffer->partDataPages.size; ++i) {
        void* dataPage = (void*)vector_get(&textBuffer->partDataPages, i);
        CHECK_ERROR(error_out);

        mm_freePages(dataPage);
    }
    
    vector_clearStruct(&textBuffer->partDataPages);
    loopArray_clearStruct(&textBuffer->partEntries);

    return;
error_out:
}

bool textBuffer_getPart(TextBuffer* textBuffer, Index32 partIndex, Cstring buffer, Uint16* partLengthRet) {
    DEBUG_ASSERT_SILENT(partLengthRet != NULL);
    
    Object partEntry = loopArray_get(&textBuffer->partEntries, partIndex);
    CHECK_ERROR(error_out);
    Uint16 partLength = textBuffer_getPartLength(textBuffer, partIndex);
    ERROR_THROW_NEW_IF(partLength == (Uint16)INFINITE, ERROR_INVALID_STATE, error_out);

    DEBUG_ASSERT_SILENT(partLength <= textBuffer->maxPartLen);

    Index64 currentPartPosition = __TEXT_BUFFER_PART_GET_POSITION(partEntry);
    Size remainByteNum = partLength;
    Cstring currentBuffer = buffer;
    
    while (remainByteNum > 0) {
        Size byteReadNum = algorithms_umin64(ALIGN_UP_SHIFT(currentPartPosition + 1, PAGE_SIZE_SHIFT) - currentPartPosition, remainByteNum);
        ConstCstring readBegin = (ConstCstring)__textBuffer_getDataPage(textBuffer, currentPartPosition, false) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPartPosition);

        memory_memcpy(currentBuffer, readBegin, byteReadNum);
        
        currentPartPosition += byteReadNum;
        remainByteNum -= byteReadNum;
        currentBuffer += byteReadNum;
    }

    *partLengthRet = partLength;

    return partIndex == textBuffer_getPartNum(textBuffer) - 1 || TEST_FLAGS(partEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL);
error_out:
    return false;
}

Uint16 textBuffer_getPartLength(TextBuffer* textBuffer, Index32 partIndex) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    ERROR_THROW_NEW_IF(partIndex >= partNum, ERROR_INVALID_ARGUMENT, error_out);

    if (partIndex == partNum - 1) {
        return textBuffer->lastPartLen;
    }

    Object partEntry = loopArray_get(&textBuffer->partEntries, partIndex);
    CHECK_ERROR(error_out);

    Object nextPartEntry = loopArray_get(&textBuffer->partEntries, partIndex + 1);
    CHECK_ERROR(error_out);

    Index64 beginPosition = __TEXT_BUFFER_PART_GET_POSITION(partEntry), endPosition = __TEXT_BUFFER_PART_GET_POSITION(nextPartEntry);
    DEBUG_ASSERT_SILENT(endPosition - beginPosition <= textBuffer->maxPartLen);
    
    return (Uint16)(endPosition - beginPosition);
error_out:
    return (Uint16)INFINITE;
}

Size textBuffer_pushText(TextBuffer* textBuffer, ConstCstring buffer, Size n) { //TODO: Auto add tail for new line in buffer
    DEBUG_ASSERT_SILENT(textBuffer_getPartNum(textBuffer) > 0);

    Size remainByteNum = n;
    ConstCstring currentBuffer = buffer;

    Size ret = 0;
    while (remainByteNum > 0) {
        Size currentPartNum = textBuffer_getPartNum(textBuffer);
        Uint16 currentLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 1);
        CHECK_ERROR(error_out);
        Object currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
        CHECK_ERROR(error_out);

        if (TEST_FLAGS(currentLastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL) || currentLastPartLength == textBuffer->maxPartLen) {
            Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
            if (__textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition)) {
                ++ret;
            }
            CHECK_ERROR(error_out);
        }

        Uint16 pushedByteNum = __textBuffer_pushDataToLastPart(textBuffer, currentBuffer, remainByteNum);
        CHECK_ERROR(error_out);
        DEBUG_ASSERT_SILENT(pushedByteNum > 0);

        textBuffer->totalByteNum += pushedByteNum;
        
        remainByteNum -= pushedByteNum;
        currentBuffer += pushedByteNum;
    }

    return ret;
error_out:
    return 0;
}

bool textBuffer_pushChar(TextBuffer* textBuffer, char ch) {
    Size currentPartNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(currentPartNum > 0);
    Uint16 currentLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 1);
    CHECK_ERROR(error_out);
    Object currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
    CHECK_ERROR(error_out);

    bool ret = false;
    if (TEST_FLAGS(currentLastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL) || currentLastPartLength == textBuffer->maxPartLen) {
        Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
        ret = __textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition);
        CHECK_ERROR(error_out);

        currentPartNum = textBuffer_getPartNum(textBuffer);
        currentLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 1);
        CHECK_ERROR(error_out);
        currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
        CHECK_ERROR(error_out);
    }

    Index64 currentPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
    Cstring pushBegin = (Cstring)__textBuffer_getDataPage(textBuffer, currentPosition, true) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPosition);
    CHECK_ERROR(error_out);

    *pushBegin = ch;
    ++textBuffer->lastPartLen;
    ++textBuffer->totalByteNum;
    
    return ret;
error_out:
    return false;
}

void textBuffer_popData(TextBuffer* textBuffer, Size n) {
    ERROR_THROW_NEW_IF(n > textBuffer->totalByteNum, ERROR_INVALID_ARGUMENT, error_out);

    Size remainByteNum = n;
    while (remainByteNum > 0) {
        Uint16 poppedByteNum = __textBuffer_popDataFromLastPart(textBuffer, remainByteNum);
        CHECK_ERROR(error_out);

        textBuffer->totalByteNum -= poppedByteNum;
        remainByteNum -= poppedByteNum;

        Size currentPartNum = textBuffer_getPartNum(textBuffer);
        if (currentPartNum >= 2 && textBuffer_getPartLength(textBuffer, currentPartNum - 1) == 0) {
            Uint16 newLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 2);
            CHECK_ERROR(error_out);
            
            loopArray_popBack(&textBuffer->partEntries);
            CHECK_ERROR(error_out);

            textBuffer->lastPartLen = newLastPartLength;
        }
    }

    return;
error_out:
}

bool textBuffer_finishPart(TextBuffer* textBuffer) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);

    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    CHECK_ERROR(error_out_false3);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    CHECK_ERROR(error_out_false3);

    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL));
    SET_FLAG_BACK(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL);
    
    loopArray_set(&textBuffer->partEntries, partNum - 1, lastPartEntry);
    CHECK_ERROR(error_out_false3);

    Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry) + lastPartLength;
    bool ret = __textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition);
    CHECK_ERROR(error_out_false3);

    return ret;
error_out_false3:
    return false;
}

void textBuffer_dump(TextBuffer* textBuffer, void* dumpTo, Size dumpByteNum) {
    Size remainDumpByteNum = dumpByteNum;
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);
    Index32 currentPartIndex = partNum - 1;
    Uint16 currentPartOffset = 0;

    while (remainDumpByteNum > 0) {
        Object currentPartEntry = loopArray_get(&textBuffer->partEntries, currentPartIndex);
        CHECK_ERROR(error_out);
        if (TEST_FLAGS(currentPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL)) {
            --remainDumpByteNum;
        }

        Uint16 partLength = textBuffer_getPartLength(textBuffer, currentPartIndex);
        CHECK_ERROR(error_out);
        if (remainDumpByteNum < partLength) {
            currentPartOffset = partLength - remainDumpByteNum;
            remainDumpByteNum = 0;
        } else {
            if (currentPartIndex == 0) {
                break;
            }
            remainDumpByteNum -= partLength;
            --currentPartIndex;
        }
    }

    Cstring currentBuffer = (Cstring)dumpTo;
    for (; currentPartIndex < partNum; ++currentPartIndex) {
        Object currentPartEntry = loopArray_get(&textBuffer->partEntries, currentPartIndex);
        CHECK_ERROR(error_out);

        Index64 currentPartPosition = __TEXT_BUFFER_PART_GET_POSITION(currentPartEntry);
        ConstCstring readBegin = (ConstCstring)__textBuffer_getDataPage(textBuffer, currentPartPosition, true) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPartPosition);
        CHECK_ERROR(error_out);

        Uint16 partLength = textBuffer_getPartLength(textBuffer, currentPartIndex);
        CHECK_ERROR(error_out);

        if (currentPartOffset != 0) {
            Uint16 readByteNum = partLength - currentPartOffset;
            DEBUG_ASSERT_SILENT(remainDumpByteNum > readByteNum);
            memory_memcpy(currentBuffer, readBegin + currentPartOffset, readByteNum);
            currentBuffer += readByteNum;
            remainDumpByteNum -= readByteNum;
            currentPartOffset = 0;
        } else {
            DEBUG_ASSERT_SILENT(remainDumpByteNum > partLength);
            memory_memcpy(currentBuffer, readBegin, partLength);
            currentBuffer += partLength;
            remainDumpByteNum -= partLength;
        }

        if (TEST_FLAGS(currentPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL)) {
            DEBUG_ASSERT_SILENT(remainDumpByteNum > 0);
            --remainDumpByteNum;
            *currentBuffer = '\n';
            ++currentBuffer;
        }
    }

    return;
error_out:
}

void textBuffer_resize(TextBuffer* textBuffer, Size newMaxPartNum, Uint16 newMaxPartLen) {
    DEBUG_ASSERT_SILENT(newMaxPartLen == textBuffer->maxPartLen);   //TODO: Not completed
    DEBUG_ASSERT_SILENT(newMaxPartNum == textBuffer->maxPartNum);
}

static void* __textBuffer_getDataPage(TextBuffer* textBuffer, Index64 position, bool createIfNotExist) {
    Index64 realPageIndex = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(position) - textBuffer->releasedPageNum;
    Vector* pages = &textBuffer->partDataPages;
    ERROR_THROW_NEW_IF(realPageIndex > pages->size, ERROR_INVALID_ARGUMENT, error_out);
    if (realPageIndex == pages->size) {
        ERROR_THROW_NEW_IF(!createIfNotExist, ERROR_INVALID_ARGUMENT, error_out);

        __textBuffer_enqueueDataPage(textBuffer);
        CHECK_ERROR(error_out);
    }
    
    void* ret = (void*)vector_get(pages, realPageIndex);
    CHECK_ERROR(error_out);

    return ret;
error_out:
    return NULL;
}

static void __textBuffer_enqueueDataPage(TextBuffer* textBuffer) {
    void* newPage = mm_allocatePages(1);
    ERROR_THROW_NEW_IF(newPage == NULL, ERROR_OUT_OF_MEMORY, error_out);

    memory_memset(newPage, 0, PAGE_SIZE);

    vector_push(&textBuffer->partDataPages, (Object)newPage);
    CHECK_ERROR(error_out);

    return;
error_out:
}

static void __textBuffer_dequeueDataPageFront(TextBuffer* textBuffer, Size releasePageNum) {
    ERROR_THROW_NEW_IF(releasePageNum > textBuffer->partDataPages.size - 1, ERROR_INVALID_ARGUMENT, error_out);
    
    for (int i = 0; i < releasePageNum; ++i) {
        void* page = (void*)vector_get(&textBuffer->partDataPages, i);
        CHECK_ERROR(error_out);
        mm_freePages(page);
    }

    vector_ereaseN(&textBuffer->partDataPages, 0, releasePageNum);
    CHECK_ERROR(error_out);

    textBuffer->releasedPageNum += releasePageNum;

    return;
error_out:
}

static void __textBuffer_dequeueDataPageBack(TextBuffer* textBuffer, Size releasePageNum) {
    ERROR_THROW_NEW_IF(releasePageNum > textBuffer->partDataPages.size - 1, ERROR_INVALID_ARGUMENT, error_out);

    for (int i = 0; i < releasePageNum; ++i) {
        void* page = (void*)vector_get(&textBuffer->partDataPages, textBuffer->partDataPages.size - 1);
        CHECK_ERROR(error_out);
        mm_freePages(page);
        vector_pop(&textBuffer->partDataPages);
        CHECK_ERROR(error_out);
    }

    textBuffer->releasedPageNum += releasePageNum;

    return;
error_out:
}

static Uint16 __textBuffer_pushDataToLastPart(TextBuffer* textBuffer, ConstCstring buffer, Size n) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);
    
    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    CHECK_ERROR(error_out);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    CHECK_ERROR(error_out);
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL));

    Index64 lastPartPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry);
    Index64 lastPartEndPosition = lastPartPosition + lastPartLength;

    Uint16 realN = algorithms_umin64(textBuffer->maxPartLen - lastPartLength, n);

    Uint16 remainPushByteNum = realN;
    Index64 currentPosition = lastPartEndPosition;
    while (remainPushByteNum > 0) {
        Uint16 pushByteNum = algorithms_umin64(remainPushByteNum, ALIGN_UP_SHIFT(currentPosition + 1, PAGE_SIZE_SHIFT) - currentPosition);
    
        Cstring pushBegin = (Cstring)__textBuffer_getDataPage(textBuffer, currentPosition, true) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPosition);
        CHECK_ERROR(error_out);
    
        memory_memcpy(pushBegin, buffer, pushByteNum);

        remainPushByteNum -= pushByteNum;
        currentPosition += pushByteNum;
    }
    textBuffer->lastPartLen += realN;

    return realN;
error_out:
    return (Uint16)INFINITE;
}

static Uint16 __textBuffer_popDataFromLastPart(TextBuffer* textBuffer, Size n) {    //TODO: Count tail flag into pop data
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);
    
    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    CHECK_ERROR(error_out);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    CHECK_ERROR(error_out);

    Index64 beforePopEndPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry) + lastPartLength;
    
    if (TEST_FLAGS(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL)) {
        CLEAR_FLAG_BACK(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL);
        loopArray_set(&textBuffer->partEntries, partNum - 1, lastPartEntry);
        CHECK_ERROR(error_out);
    }
    
    Uint16 popByteNum = algorithms_umin64(lastPartLength, n);
    textBuffer->lastPartLen -= popByteNum;

    Index64 afterPopEndPosition = beforePopEndPosition - popByteNum;
    Size releasePageNum = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(beforePopEndPosition) - __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(afterPopEndPosition);
    if (releasePageNum > 0) {
        DEBUG_ASSERT_SILENT(releasePageNum <= textBuffer->partDataPages.size - 1);
        __textBuffer_dequeueDataPageBack(textBuffer, releasePageNum);
        CHECK_ERROR(error_out);
    }

    return popByteNum;
error_out:
    return (Uint16)INFINITE;
}

static void __textBuffer_popFirstPart(TextBuffer* textBuffer) {
    Object firstPartEntry = loopArray_get(&textBuffer->partEntries, 0);
    CHECK_ERROR(error_out);
    Index64 firstPartPosition = __TEXT_BUFFER_PART_GET_POSITION(firstPartEntry);
    Uint16 firstPartLength = textBuffer_getPartLength(textBuffer, 0);
    CHECK_ERROR(error_out);
    
    Index64 firstPartEndPosition = firstPartPosition + firstPartLength;
    Size releasePageNum = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(firstPartEndPosition) - __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(firstPartPosition);
    if (releasePageNum > 0) {
        DEBUG_ASSERT_SILENT(releasePageNum <= textBuffer->partDataPages.size - 1);
        __textBuffer_dequeueDataPageFront(textBuffer, releasePageNum);
        CHECK_ERROR(error_out);
    }

    return;
error_out:
}

static bool __textBuffer_pushNewPartWithPopFirst(TextBuffer* textBuffer, Index64 newPartPosition) {
    bool ret = false;
    if (textBuffer_getPartNum(textBuffer) == textBuffer->maxPartNum) {  //Push new part will remove current first part
        __textBuffer_popFirstPart(textBuffer);
        CHECK_ERROR(error_out);
        ret = true;
    }
    
    __textBuffer_pushNewPart(textBuffer, newPartPosition);
    CHECK_ERROR(error_out);

    return ret;
error_out:
    return false;
}