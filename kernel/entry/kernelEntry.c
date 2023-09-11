#include<devices/terminal/terminal.h>
#include<devices/terminal/terminalSwitch.h>
#include<kit/types.h>
#include<real/simpleAsmLines.h>
#include<system/GDT.h>
#include<system/memoryMap.h>
#include<system/systemInfo.h> 

extern void kernelMain(SystemInfo* info);

SystemInfo info;

static Terminal _terminal;
static char _terminalBuffer[4 * TEXT_MODE_WIDTH * TEXT_MODE_HEIGHT];

int a = 0;

void kernelEntry(MemoryMap* mMap, GDTDesc64* desc) {
    info.magic      = SYSTEM_INFO_MAGIC;
    info.memoryMap  = (Uint64)mMap;
    info.gdtDesc    = (Uint64)desc;

    kernelMain(&info);
}