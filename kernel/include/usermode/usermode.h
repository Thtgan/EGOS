#if !defined(__USERMODE_H)
#define __USERMODE_H

#include<fs/fileSystem.h>
#include<kit/types.h>

void initUsermode();

int execute(FileSystem* fs, ConstCstring path);

#endif // __USERMODE_H
