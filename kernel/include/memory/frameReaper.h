#if !defined(__MEMORY_FRAMEREAPER_H)
#define __MEMORY_FRAMEREAPER_H

typedef struct FrameReaperNode FrameReaperNode;

#include<structs/linkedList.h>

typedef LinkedList FrameReaper;

static inline void frameReaper_initStruct(FrameReaper* reaper) {
    linkedList_initStruct(reaper);
}

void frameReaper_collect(FrameReaper* reaper, void* frames, Size n);

void frameReaper_reap(FrameReaper* reaper);

#endif // __MEMORY_FRAMEREAPER_H
