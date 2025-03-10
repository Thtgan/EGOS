#include<devices/terminal/textBuffer.h>

#include<kit/bit.h>
#include<kit/types.h>
#include<kit/util.h>
#include<memory/memory.h>
#include<memory/paging.h>
#include<structs/loopArray.h>
#include<structs/vector.h>
#include<system/pageTable.h>
#include<algorithms.h>
#include<error.h>

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
    ERROR_GOTO_IF_ERROR(0);
    
    __textBuffer_enqueueDataPage(textBuffer);
    ERROR_GOTO_IF_ERROR(0);

    textBuffer->totalByteNum = 0;
    
    __textBuffer_pushNewPart(textBuffer, __TEXT_BUFFER_PART_POSITION_BUILD(0, 0));  //Error passthrough

    return;
    ERROR_FINAL_BEGIN(0);
}

void textBuffer_clearStruct(TextBuffer* textBuffer) {
    for (int i = 0; i < textBuffer->partDataPages.size; ++i) {
        void* dataPage = (void*)vector_get(&textBuffer->partDataPages, i);
        ERROR_GOTO_IF_ERROR(0);

        dataPage = paging_convertAddressV2P(dataPage);
        memory_freeFrame(dataPage);
    }
    
    vector_clearStruct(&textBuffer->partDataPages);
    loopArray_clearStruct(&textBuffer->partEntries);

    return;
    ERROR_FINAL_BEGIN(0);
}

bool textBuffer_getPart(TextBuffer* textBuffer, Index32 partIndex, Cstring buffer, Uint16* partLengthRet) {
    DEBUG_ASSERT_SILENT(partLengthRet != NULL);
    
    Object partEntry = loopArray_get(&textBuffer->partEntries, partIndex);
    ERROR_GOTO_IF_ERROR(0);
    Uint16 partLength = textBuffer_getPartLength(textBuffer, partIndex);
    if (partLength == (Uint16)INFINITE) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

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
    ERROR_FINAL_BEGIN(0);
    return false;
}

Uint16 textBuffer_getPartLength(TextBuffer* textBuffer, Index32 partIndex) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    if (partIndex >= partNum) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    if (partIndex == partNum - 1) {
        return textBuffer->lastPartLen;
    }

    Object partEntry = loopArray_get(&textBuffer->partEntries, partIndex);
    ERROR_GOTO_IF_ERROR(0);

    Object nextPartEntry = loopArray_get(&textBuffer->partEntries, partIndex + 1);
    ERROR_GOTO_IF_ERROR(0);

    Index64 beginPosition = __TEXT_BUFFER_PART_GET_POSITION(partEntry), endPosition = __TEXT_BUFFER_PART_GET_POSITION(nextPartEntry);
    DEBUG_ASSERT_SILENT(endPosition - beginPosition <= textBuffer->maxPartLen);
    
    return (Uint16)(endPosition - beginPosition);

    ERROR_FINAL_BEGIN(0);
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
        ERROR_GOTO_IF_ERROR(0);
        Object currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
        ERROR_GOTO_IF_ERROR(0);

        if (TEST_FLAGS(currentLastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL) || currentLastPartLength == textBuffer->maxPartLen) {
            Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
            if (__textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition)) {
                ++ret;
            }
            ERROR_GOTO_IF_ERROR(0);
        }

        Uint16 pushedByteNum = __textBuffer_pushDataToLastPart(textBuffer, currentBuffer, remainByteNum);
        ERROR_GOTO_IF_ERROR(0);
        DEBUG_ASSERT_SILENT(pushedByteNum > 0);

        textBuffer->totalByteNum += pushedByteNum;
        
        remainByteNum -= pushedByteNum;
        currentBuffer += pushedByteNum;
    }

    return ret;
    ERROR_FINAL_BEGIN(0);
    return 0;
}

bool textBuffer_pushChar(TextBuffer* textBuffer, char ch) {
    Size currentPartNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(currentPartNum > 0);
    Uint16 currentLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 1);
    ERROR_GOTO_IF_ERROR(0);
    Object currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
    ERROR_GOTO_IF_ERROR(0);

    bool ret = false;
    if (TEST_FLAGS(currentLastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL) || currentLastPartLength == textBuffer->maxPartLen) {
        Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
        ret = __textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition);
        ERROR_GOTO_IF_ERROR(0);

        currentPartNum = textBuffer_getPartNum(textBuffer);
        currentLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 1);
        ERROR_GOTO_IF_ERROR(0);
        currentLastPartEntry = loopArray_get(&textBuffer->partEntries, currentPartNum - 1);
        ERROR_GOTO_IF_ERROR(0);
    }

    Index64 currentPosition = __TEXT_BUFFER_PART_GET_POSITION(currentLastPartEntry) + currentLastPartLength;
    Cstring pushBegin = (Cstring)__textBuffer_getDataPage(textBuffer, currentPosition, true) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPosition);
    ERROR_GOTO_IF_ERROR(0);

    *pushBegin = ch;
    ++textBuffer->lastPartLen;
    ++textBuffer->totalByteNum;
    
    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}

void textBuffer_popData(TextBuffer* textBuffer, Size n) {
    if (n > textBuffer->totalByteNum) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    Size remainByteNum = n;
    while (remainByteNum > 0) {
        Uint16 poppedByteNum = __textBuffer_popDataFromLastPart(textBuffer, remainByteNum);
        ERROR_GOTO_IF_ERROR(0);

        textBuffer->totalByteNum -= poppedByteNum;
        remainByteNum -= poppedByteNum;

        Size currentPartNum = textBuffer_getPartNum(textBuffer);
        if (currentPartNum >= 2 && textBuffer_getPartLength(textBuffer, currentPartNum - 1) == 0) {
            Uint16 newLastPartLength = textBuffer_getPartLength(textBuffer, currentPartNum - 2);
            ERROR_GOTO_IF_ERROR(0);
            
            loopArray_popBack(&textBuffer->partEntries);
            ERROR_GOTO_IF_ERROR(0);

            textBuffer->lastPartLen = newLastPartLength;
        }
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

bool textBuffer_finishPart(TextBuffer* textBuffer) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);

    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);

    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL));
    SET_FLAG_BACK(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL);
    
    loopArray_set(&textBuffer->partEntries, partNum - 1, lastPartEntry);
    ERROR_GOTO_IF_ERROR(0);

    Index64 newPartPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry) + lastPartLength;
    bool ret = __textBuffer_pushNewPartWithPopFirst(textBuffer, newPartPosition);
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}

void textBuffer_dump(TextBuffer* textBuffer, Size dumpPartNum) {

}

void textBuffer_resize(TextBuffer* textBuffer, Size newMaxPartNum, Uint16 newMaxPartLen) {
    DEBUG_ASSERT_SILENT(newMaxPartLen == textBuffer->maxPartLen);   //TODO: Not completed
    DEBUG_ASSERT_SILENT(newMaxPartNum == textBuffer->maxPartNum);
}

static void* __textBuffer_getDataPage(TextBuffer* textBuffer, Index64 position, bool createIfNotExist) {
    Index64 realPageIndex = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(position) - textBuffer->releasedPageNum;
    Vector* pages = &textBuffer->partDataPages;
    if (realPageIndex > pages->size) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    } else if (realPageIndex == pages->size) {
        if (!createIfNotExist) {
            ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
        }

        __textBuffer_enqueueDataPage(textBuffer);
        ERROR_GOTO_IF_ERROR(0);
    }
    
    void* ret = (void*)vector_get(pages, realPageIndex);
    ERROR_GOTO_IF_ERROR(0);

    return ret;

    ERROR_FINAL_BEGIN(0);
    return NULL;
}

static void __textBuffer_enqueueDataPage(TextBuffer* textBuffer) {
    void* newPage = memory_allocateFrame(1);
    if (newPage == NULL) {
        ERROR_ASSERT_ANY();
        ERROR_GOTO(0);
    }

    newPage = paging_convertAddressP2V(newPage);
    memory_memset(newPage, 0, PAGE_SIZE);

    vector_push(&textBuffer->partDataPages, (Object)newPage);
    ERROR_GOTO_IF_ERROR(0);

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __textBuffer_dequeueDataPageFront(TextBuffer* textBuffer, Size releasePageNum) {
    if (releasePageNum > textBuffer->partDataPages.size - 1) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }
    
    for (int i = 0; i < releasePageNum; ++i) {
        void* page = (void*)vector_get(&textBuffer->partDataPages, i);
        ERROR_GOTO_IF_ERROR(0);
        page = paging_convertAddressV2P(page);
        memory_freeFrame(page);
    }

    vector_ereaseN(&textBuffer->partDataPages, 0, releasePageNum);
    ERROR_GOTO_IF_ERROR(0);

    textBuffer->releasedPageNum += releasePageNum;

    return;
    ERROR_FINAL_BEGIN(0);
}

static void __textBuffer_dequeueDataPageBack(TextBuffer* textBuffer, Size releasePageNum) {
    if (releasePageNum > textBuffer->partDataPages.size - 1) {
        ERROR_THROW(ERROR_ID_OUT_OF_BOUND, 0);
    }

    for (int i = 0; i < releasePageNum; ++i) {
        void* page = (void*)vector_get(&textBuffer->partDataPages, textBuffer->partDataPages.size - 1);
        ERROR_GOTO_IF_ERROR(0);
        page = paging_convertAddressV2P(page);
        memory_freeFrame(page);
        vector_pop(&textBuffer->partDataPages);
        ERROR_GOTO_IF_ERROR(0);
    }

    textBuffer->releasedPageNum += releasePageNum;

    return;
    ERROR_FINAL_BEGIN(0);
}

static Uint16 __textBuffer_pushDataToLastPart(TextBuffer* textBuffer, ConstCstring buffer, Size n) {
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);
    
    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);
    DEBUG_ASSERT_SILENT(TEST_FLAGS_FAIL(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL));

    Index64 lastPartPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry);
    Index64 lastPartEndPosition = lastPartPosition + lastPartLength;

    Uint16 realN = algorithms_umin64(textBuffer->maxPartLen - lastPartLength, n);

    Uint16 remainPushByteNum = realN;
    Index64 currentPosition = lastPartEndPosition;
    while (remainPushByteNum > 0) {
        Uint16 pushByteNum = algorithms_umin64(remainPushByteNum, ALIGN_UP_SHIFT(currentPosition + 1, PAGE_SIZE_SHIFT) - currentPosition);
    
        Cstring pushBegin = (Cstring)__textBuffer_getDataPage(textBuffer, currentPosition, true) + __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_OFFSET(currentPosition);
        ERROR_GOTO_IF_ERROR(0);
    
        memory_memcpy(pushBegin, buffer, pushByteNum);

        remainPushByteNum -= pushByteNum;
        currentPosition += pushByteNum;
    }
    textBuffer->lastPartLen += realN;

    return realN;
    ERROR_FINAL_BEGIN(0);
    return (Uint16)INFINITE;
}

static Uint16 __textBuffer_popDataFromLastPart(TextBuffer* textBuffer, Size n) {    //TODO: Count tail flag into pop data
    Size partNum = textBuffer_getPartNum(textBuffer);
    DEBUG_ASSERT_SILENT(partNum > 0);
    
    Uint16 lastPartLength = textBuffer_getPartLength(textBuffer, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);
    Object lastPartEntry = loopArray_get(&textBuffer->partEntries, partNum - 1);
    ERROR_GOTO_IF_ERROR(0);

    Index64 beforePopEndPosition = __TEXT_BUFFER_PART_GET_POSITION(lastPartEntry) + lastPartLength;
    
    if (TEST_FLAGS(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL)) {
        CLEAR_FLAG_BACK(lastPartEntry, __TEXT_BUFFER_PART_ENTRY_FLAG_IS_TAIL);
        loopArray_set(&textBuffer->partEntries, partNum - 1, lastPartEntry);
        ERROR_GOTO_IF_ERROR(0);
    }
    
    Uint16 popByteNum = algorithms_umin64(lastPartLength, n);
    textBuffer->lastPartLen -= popByteNum;

    Index64 afterPopEndPosition = beforePopEndPosition - popByteNum;
    Size releasePageNum = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(beforePopEndPosition) - __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(afterPopEndPosition);
    if (releasePageNum > 0) {
        DEBUG_ASSERT_SILENT(releasePageNum <= textBuffer->partDataPages.size - 1);
        __textBuffer_dequeueDataPageBack(textBuffer, releasePageNum);
        ERROR_GOTO_IF_ERROR(0);
    }

    return popByteNum;
    ERROR_FINAL_BEGIN(0);
    return (Uint16)INFINITE;
}

static void __textBuffer_popFirstPart(TextBuffer* textBuffer) {
    Object firstPartEntry = loopArray_get(&textBuffer->partEntries, 0);
    ERROR_GOTO_IF_ERROR(0);
    Index64 firstPartPosition = __TEXT_BUFFER_PART_GET_POSITION(firstPartEntry);
    Uint16 firstPartLength = textBuffer_getPartLength(textBuffer, 0);
    ERROR_GOTO_IF_ERROR(0);
    
    Index64 firstPartEndPosition = firstPartPosition + firstPartLength;
    Size releasePageNum = __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(firstPartEndPosition) - __TEXT_BUFFER_PART_POSITION_GET_BEGIN_PAGE_INDEX(firstPartPosition);
    if (releasePageNum > 0) {
        DEBUG_ASSERT_SILENT(releasePageNum <= textBuffer->partDataPages.size - 1);
        __textBuffer_dequeueDataPageFront(textBuffer, releasePageNum);
        ERROR_GOTO_IF_ERROR(0);
    }

    return;
    ERROR_FINAL_BEGIN(0);
}

static bool __textBuffer_pushNewPartWithPopFirst(TextBuffer* textBuffer, Index64 newPartPosition) {
    bool ret = false;
    if (textBuffer_getPartNum(textBuffer) == textBuffer->maxPartNum) {  //Push new part will remove current first part
        __textBuffer_popFirstPart(textBuffer);
        ERROR_GOTO_IF_ERROR(0);
        ret = true;
    }
    
    __textBuffer_pushNewPart(textBuffer, newPartPosition);
    ERROR_GOTO_IF_ERROR(0);

    return ret;
    ERROR_FINAL_BEGIN(0);
    return false;
}