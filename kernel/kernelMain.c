#include<debug.h>
#include<devices/bus/pci.h>
#include<init.h>
#include<kit/types.h>
#include<memory/allocator.h>
#include<multitask/schedule.h>
#include<multitask/locks/semaphore.h>
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

Uint16 rootTID = 0;

#include<kit/util.h>
#include<system/memoryLayout.h>
#include<memory/paging.h>
#include<interrupt/IDT.h>
#include<fs/fs.h>
#include<fs/fcntl.h>
#include<fs/fat32/fat32.h>
#include<fs/fsEntry.h>

#include<memory/memory.h>
#include<memory/mm.h>

#include<realmode.h>

#include<devices/display/vga/registers.h>
#include<devices/display/vga/dac.h>
#include<devices/display/vga/vga.h>
#include<devices/display/vga/memory.h>
#include<error.h>

#include<devices/terminal/tty.h>

Timer timer1, timer2, timer3;

static void __timerFunc1(Timer* timer) {
    print_printf("HANDLER CALL FROM TIMER1\n");
}

static void __timerFunc2(Timer* timer) {
    print_printf("HANDLER CALL FROM TIMER2\n");
    if (--timer->data == 0) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
}

static void __timerFunc3(Timer* timer) {
    print_printf("HANDLER CALL FROM TIMER3\n");
    if (--timer->data == 0) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
}

static void __testSigaction(int signal) {
    print_printf("Signal %s triggered\n", signal_names[signal]);
}

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        debug_blowup("Boot magic not match");
    }

    INIT_KERNEL();

    vga_switchMode(vgaMode_getModeHeader(VGA_MODE_TYPE_TEXT_50X80_D4), false);
    tty_switchDisplayMode(DISPLAY_MODE_VGA);

    VGAmodeHeader* mode = vga_getCurrentMode();
    DisplayPosition p1 = {0, 0};
    DisplayPosition p2 = {mode->height - 1, mode->width - 1};
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

    // File file;
    // fs_fileOpen(&file, "/a.txt", FCNTL_OPEN_READ_WRITE | FCNTL_OPEN_CREAT);
    // fs_fileSeek(&file, FS_FILE_SEEK_BEGIN, 0);
    // for (int i = 0; i < 64; ++i) {
    //     fs_fileWrite(&file, "TEST", 4);
    // }
    // fs_fileClose(&file);

    Uint32 pciDeviceNum = pci_getDeviceNum();
    print_debugPrintf("%u PCI devices found\n", pciDeviceNum);
    if (pciDeviceNum != 0) {
        print_debugPrintf("Bus Dev Func Vendor Device Class SubClass\n");
        for (int i = 0; i < pciDeviceNum; ++i) {
            PCIdevice* device = pci_getDevice(i);
            print_debugPrintf(
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

    semaphore_initStruct(&sema1, 0);
    semaphore_initStruct(&sema2, -1);

    MemoryPreset* cowPreset = extraPageTableContext_getDefaultPreset(&mm->extraPageTableContext, MEMORY_DEFAULT_PRESETS_TYPE_COW);
    arr1 = memory_allocate(1 * sizeof(int)), arr2 = memory_allocateDetailed(1 * sizeof(int), cowPreset);
    print_printf("%p %p\n", arr1, arr2);
    arr1[0] = 1, arr2[0] = 114514;
    rootTID = schedule_getCurrentThread()->tid;
    Process* forked = schedule_fork();
    if (forked != NULL) {
        Thread* currentThread = schedule_getCurrentThread();
        print_printf("This is main thread, TID: %u, PID: %u\n", currentThread->tid, currentThread->process->pid);
    } else {
        Thread* currentThread = schedule_getCurrentThread();
        print_printf("This is child thread, TID: %u, PID: %u\n", currentThread->tid, currentThread->process->pid);
    }

    Thread* currentThread = schedule_getCurrentThread();
    if (rootTID == currentThread->tid) {
        arr1[0] = 3;
        semaphore_up(&sema1);
        semaphore_down(&sema2);
        print_printf("DONE 0, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
    } else {
        Sigaction sigaction = {
            .handler = __testSigaction
        };
        
        process_sigaction(schedule_getCurrentProcess(), SIGNAL_SIGTRAP, &sigaction, NULL);

        semaphore_down(&sema1);
        arr1[0] = 2, arr2[0] = 1919810;
        print_printf("DONE 1, arr1: %d, arr2: %d\n", arr1[0], arr2[0]);
        semaphore_up(&sema2);
        while (true) {
            print_printf("Waiting for input: ");
            int len = teletype_rawRead(tty_getCurrentTTY(), str, INFINITE);
            ERROR_CHECKPOINT();

            if (cstring_strcmp(str, "PASS") == 0 || len == 0) {
                break;
            }
            print_printf("%s-%d\n", str, len);
        }

        int ret = usermode_execute("/bin/test");
        print_printf("USER PROGRAM RETURNED %d\n", ret);

        semaphore_up(&sema2);

        timer_initStruct(&timer3, 500, TIME_UNIT_MILLISECOND);
        SET_FLAG_BACK(timer3.flags, TIMER_FLAGS_SYNCHRONIZE | TIMER_FLAGS_REPEAT);
        timer3.data = 5;
        timer3.handler = __timerFunc3;

        timer_start(&timer3);   //TODO: What if process stopped by signal after starting timer?

        print_printf("Killing self\n");
        process_signal(schedule_getCurrentProcess(), SIGNAL_SIGKILL);
    }

    memory_free(arr1);
    memory_free(arr2);

    print_printf("FINAL %u\n", schedule_getCurrentThread()->tid);

    {
        File* file = fs_fileOpen("/dev/null", FCNTL_OPEN_READ_WRITE);
        ERROR_CHECKPOINT();
        fs_fileWrite(file, "1145141919810", 14);
        ERROR_CHECKPOINT();
        fs_fileClose(file);
    }

    timer_initStruct(&timer1, 500, TIME_UNIT_MILLISECOND);
    timer_initStruct(&timer2, 500, TIME_UNIT_MILLISECOND);
    SET_FLAG_BACK(timer2.flags, TIMER_FLAGS_SYNCHRONIZE | TIMER_FLAGS_REPEAT);

    timer1.handler = __timerFunc1;
    timer2.data = 5;
    timer2.handler = __timerFunc2;

    process_signal(forked, SIGNAL_SIGTRAP);
    process_signal(forked, SIGNAL_SIGSTOP);
    timer_start(&timer1);
    ERROR_CHECKPOINT();
    timer_start(&timer2);
    ERROR_CHECKPOINT();
    process_signal(forked, SIGNAL_SIGCONT);

    fs_close(fs_rootFS); //TODO: Move to better place

    print_printf("DEAD\n");

    die();
}

static void printLOGO() {
    char buffer[1024];
    FS_fileStat stat;
    File* file = fs_fileOpen("/LOGO.txt", FCNTL_OPEN_READ_ONLY);
    ERROR_CHECKPOINT();
    fs_fileSeek(file, 0, FS_FILE_SEEK_END);
    fs_fileStat(file, &stat);
    ERROR_CHECKPOINT();
    fs_fileSeek(file, 0, FS_FILE_SEEK_BEGIN);

    fs_fileRead(file, buffer, stat.size);
    ERROR_CHECKPOINT();
    buffer[stat.size] = '\0';
    print_printf("%s\n", buffer);

    print_printf("%lu\n", stat.createTime.second);
    print_printf("%lu\n", stat.accessTime.second);
    print_printf("%lu\n", stat.modifyTime.second);

    BlockDevice* device = file->inode->superBlock->blockDevice;
    fs_fileClose(file);
    ERROR_CHECKPOINT();
    blockDevice_flush(device);
    ERROR_CHECKPOINT();
}