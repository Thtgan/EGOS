#if !defined(__USER_MODE_H)
#define __USER_MODE_H

__attribute__((naked))
void jumpToUserMode(void* programBegin, void* stackBottom);

#endif // __USER_MODE_H
