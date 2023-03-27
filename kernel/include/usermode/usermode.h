#if !defined(__USERMODE_H)
#define __USERMODE_H

#include<fs/fileSystem.h>
#include<kit/types.h>
#include<returnValue.h>

void initUsermode();

ReturnValue execute(ConstCstring path);

#endif // __USERMODE_H
