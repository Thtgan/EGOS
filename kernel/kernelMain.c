#include<debug.h>
#include<devices/bus/pci.h>
#include<fs/fsutil.h>
#include<init.h>
#include<kit/types.h>
#include<memory/allocator.h>
#include<multitask/schedule.h>
#include<multitask/semaphore.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<cstring.h>
#include<system/systemInfo.h>
#include<time/time.h>
#include<time/timer.h>
#include<usermode/usermode.h>

SystemInfo* sysInfo;

Semaphore sema1, sema2;
char str[128];

int* arr1, * arr2;

static void printLOGO();

#include<kit/util.h>
#include<system/memoryLayout.h>
#include<memory/paging.h>
#include<interrupt/IDT.h>
#include<fs/fat32/fat32.h>
#include<fs/fsutil.h>
#include<fs/fsEntry.h>

#include<memory/memory.h>

#include<realmode.h>

#include<devices/display/vga/registers.h>
#include<devices/display/vga/dac.h>
#include<devices/display/vga/vga.h>
#include<devices/display/vga/memory.h>

static void __timerFunc1(Timer* timer) {
    print_printf(TERMINAL_LEVEL_OUTPUT, "HANDLER CALL FROM TIMER1\n");
}

static void __timerFunc2(Timer* timer) {
    print_printf(TERMINAL_LEVEL_OUTPUT, "HANDLER CALL FROM TIMER2\n");
    if (--timer->data == 0) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
}

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        debug_blowup("Boot magic not match");
    }

    if (init_initKernel() == RESULT_FAIL) {
        debug_blowup("Initialization failed");
    }

    vga_switchMode(vgaMode_getModeHeader(VGA_MODE_TYPE_TEXT_50X80_D4), false);
    terminalSwitch_switchDisplayMode(DISPLAY_MODE_VGA);

    DisplayPosition p1 = {15, 15};
    DisplayPosition p2 = {29, 29};
    display_fill(&p1, &p2, 0x00FF00);

    // vga_switchMode(vgaMode_getModeHeader(VGA_MODE_TYPE_GRAPHIC_200X320_D8), false);

    // DisplayPosition pos = { 30, 30 };
    // display_printString(&pos, "TEST TEST-1145141919810  \n11 \r", 30, 0x00FF00);
    
    // vga_switchMode(vgaMode_getModeHeader(VGA_MODE_TYPE_TEXT_25X80_D4), false);
    // VGAmodeHeader* mode = vga_getCurrentMode();

    // DisplayPosition pos = { 0, 0 };
    // for (int i = 0; i < mode->height; ++i) {
    //     pos.x = i;
    //     vgaMemory_linearSetPixel(HOST_POINTER(mode, VGAgraphicMode, header), &pos, i, mode->width);
    // }
    // DisplayPosition p1 = {0, 0};
    // DisplayPosition p2 = {mode->height - 1, mode->width - 1};

    printLOGO();

    Uint32 pciDeviceNum = pci_getDeviceNum();
    print_printf(TERMINAL_LEVEL_OUTPUT, "%u PCI devices found\n", pciDeviceNum);
    if (pciDeviceNum != 0) {
        print_printf(TERMINAL_LEVEL_OUTPUT, "Bus Dev Func Vendor Device Class SubClass\n");
        for (int i = 0; i < pciDeviceNum; ++i) {
            PCIdevice* device = pci_getDevice(i);
            print_printf(
                TERMINAL_LEVEL_OUTPUT, 
                "%02X  %02X  %02X   %04X   %04X   %02X    %02X\n", 
                PCI_BUS_NUMBER_FROM_ADDR(device->baseAddr),
                PCI_DEVICE_NUMBER_FROM_ADDR(device->baseAddr),
                PCI_FUNCTION_NUMBER_FROM_ADDR(device->baseAddr),
                device->vendorID,
                device->deviceID,
                device->class,
                device->subClass
            );
        }
    }

    initSemaphore(&sema1, 0);
    initSemaphore(&sema2, -1);

    arr1 = memory_allocate(1 * sizeof(int)), arr2 = memory_allocateDetailed(1 * sizeof(int), EXTRA_PAGE_TABLE_CONTEXT_DEFAULT_PRESET_TYPE_TO_ID(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_COW));
    print_printf(TERMINAL_LEVEL_OUTPUT, "%p %p\n", arr1, arr2);
    arr1[0] = 1, arr2[0] = 114514;
    if (process_fork("Forked") != NULL) {
        print_printf(TERMINAL_LEVEL_OUTPUT, "This is main process, name: %s\n", scheduler_getCurrentProcess()->name);
    } else {
        print_printf(TERMINAL_LEVEL_OUTPUT, "This is child process, name: %s\n", scheduler_getCurrentProcess()->name);
    }

    if (cstring_strcmp(scheduler_getCurrentProcess()->name, "Init") == 0) {
        arr1[0] = 3;
        semaphore_up(&sema1);
        semaphore_down(&sema2);
        print_printf(TERMINAL_LEVEL_OUTPUT, "DONE 0, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
    } else {
        semaphore_down(&sema1);
        arr1[0] = 2, arr2[0] = 1919810;
        print_printf(TERMINAL_LEVEL_OUTPUT, "DONE 1, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
        semaphore_up(&sema2);
        while (true) {
            print_printf(TERMINAL_LEVEL_OUTPUT, "Waiting for input: ");
            int len = terminal_getline(terminalSwitch_getLevel(TERMINAL_LEVEL_OUTPUT), str);

            if (cstring_strcmp(str, "PASS") == 0 || len == 0) {
                break;
            }
            print_printf(TERMINAL_LEVEL_OUTPUT, "%s-%d\n", str, len);
        }

        //TODO: Calling userprogram goes wrong here

        semaphore_up(&sema2);
        process_exit();
    }

    int ret = usermode_exsecute("/bin/test");
    print_printf(TERMINAL_LEVEL_OUTPUT, "USER PROGRAM RETURNED %d\n", ret);

    memory_free(arr1);
    memory_free(arr2);

    print_printf(TERMINAL_LEVEL_OUTPUT, "FINAL %s\n", scheduler_getCurrentProcess()->name);

    fsEntry entry;
    if (fsutil_openfsEntry(rootFS->superBlock, "/dev/null", FS_ENTRY_TYPE_DEVICE, &entry) == RESULT_SUCCESS) {  //TODO: Seem it opens stdout instead, fix it
        fsutil_fileWrite(&entry, "1145141919810", 14);
    }

    Timer timer1, timer2;
    timer_initStruct(&timer1, 500, TIME_UNIT_MILLISECOND);
    timer_initStruct(&timer2, 500, TIME_UNIT_MILLISECOND);
    SET_FLAG_BACK(timer2.flags, TIMER_FLAGS_SYNCHRONIZE | TIMER_FLAGS_REPEAT);

    timer1.handler = __timerFunc1;
    timer2.data = 5;
    timer2.handler = __timerFunc2;


    timer_start(&timer1);
    timer_start(&timer2);
    fs_close(rootFS); //TODO: Move to better place

    print_printf(TERMINAL_LEVEL_OUTPUT, "DEAD\n");

    die();
}

static void printLOGO() {
    char buffer[1024];
    fsEntry entry;
    if (fsutil_openfsEntry(rootFS->superBlock, "/LOGO.txt", FS_ENTRY_TYPE_FILE, &entry) == RESULT_SUCCESS) {
        fsutil_fileSeek(&entry, 0, FSUTIL_FILE_SEEK_END);
        Size fileSize = fsutil_fileGetPointer(&entry);
        fsutil_fileSeek(&entry, 0, FSUTIL_FILE_SEEK_BEGIN);

        if (fsutil_fileRead(&entry, buffer, fileSize) == RESULT_SUCCESS) {
            buffer[fileSize] = '\0';
            print_printf(TERMINAL_LEVEL_OUTPUT, "%s\n", buffer);
        }

        fsEntryDesc* desc = entry.desc;
        print_printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc->createTime);
        print_printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc->lastAccessTime);
        print_printf(TERMINAL_LEVEL_OUTPUT, "%lu\n", desc->lastModifyTime);

        BlockDevice* device = entry.iNode->superBlock->blockDevice;
        fsutil_closefsEntry(&entry);
        blockDevice_flush(device);
    }
}