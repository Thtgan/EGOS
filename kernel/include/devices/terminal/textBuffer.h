#if !defined(__DEVICES_TERMINAL_TEXTBUFFER_H)
#define __DEVICES_TERMINAL_TEXTBUFFER_H

typedef struct TextBuffer TextBuffer;

#include<kit/types.h>
#include<structs/loopArray.h>
#include<structs/vector.h>

typedef struct TextBuffer {
    Uint32 maxPartNum;
    Uint16 maxPartLen;
    Uint16 lastPartLen;
    Size releasedPageNum;
    Size totalByteNum;
    LoopArray partEntries;
    Vector partDataPages;
} TextBuffer;

void textBuffer_initStruct(TextBuffer* textBuffer, Uint32 maxPartNum, Uint16 maxPartLen);

void textBuffer_clearStruct(TextBuffer* textBuffer);

static inline Size textBuffer_getPartNum(TextBuffer* textBuffer) {
    return textBuffer->partEntries.size;
}

bool textBuffer_getPart(TextBuffer* textBuffer, Index32 partIndex, Cstring buffer, Uint16* partLengthRet);

Uint16 textBuffer_getPartLength(TextBuffer* textBuffer, Index32 partIndex);

Size textBuffer_pushText(TextBuffer* textBuffer, ConstCstring buffer, Size n);  //Not considering escape characters (yet)

bool textBuffer_pushChar(TextBuffer* textBuffer, char ch);  //Not considering escape characters (yet)

void textBuffer_popData(TextBuffer* textBuffer, Size n);

bool textBuffer_finishPart(TextBuffer* textBuffer);

void textBuffer_dump(TextBuffer* textBuffer, void* dumpTo, Size dumpByteNum);

void textBuffer_resize(TextBuffer* textBuffer, Size newMaxPartNum, Uint16 newMaxPartLen);

#endif // __DEVICES_TERMINAL_TEXTBUFFER_H
