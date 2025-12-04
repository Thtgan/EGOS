#include<debug.h>
#include<devices/bus/pci.h>
#include<init.h>
#include<kit/types.h>
#include<memory/allocators/allocator.h>
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

static void printFileFromEXT2();

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

#include<memory/allocators/buddyHeapAllocator.h>

#include<memory/mapping.h>

#include<uart.h>

Timer timer1, timer2, timer3;

#include<multitask/locks/conditionVar.h>
#include<multitask/locks/mutex.h>
#include<multitask/ipc.h>

ConditionVar var1, var2;
Timer condTimer;
bool flag1 = false, flag2 = false;
Mutex conditionLock;

char pipeBuffer[16];

bool __conditionVarFunc1(void* arg) {
    return flag1;
}

bool __conditionVarFunc2(void* arg) {
    return flag2;
}

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
    print_printf("HANDLER CALL FROM TIMER3\n"); //TODO: TTY output semaphore may fall into sleep in ISR
    if (--timer->data == 0) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
}

static void __testSigaction(int signal) {
    print_printf("Signal %s triggered\n", signal_names[signal]);
}

static BuddyHeapAllocator cowTestAllocator;

void kernelMain(SystemInfo* info) {
    sysInfo = (SystemInfo*)info;
    if (sysInfo->magic != SYSTEM_INFO_MAGIC) {
        debug_blowup("Boot magic not match");
    }

    INIT_KERNEL();

    for (int i = 0 ; i < 5; ++i) {
        uart_print("UART TEST\n");
    }

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

    printFileFromEXT2();

    Uint32 pciDeviceNum = pci_getDeviceNum();
    debug_printf("%u PCI devices found\n", pciDeviceNum);
    if (pciDeviceNum != 0) {
        debug_printf("Bus Dev Func Vendor Device Class SubClass\n");
        for (int i = 0; i < pciDeviceNum; ++i) {
            PCIdevice* device = pci_getDevice(i);
            debug_printf(
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
    semaphore_initStruct(&sema2, 0);

    buddyHeapAllocator_initStruct(&cowTestAllocator, mm->frameAllocator, DEFAULT_MEMORY_OPERATIONS_TYPE_COW);

    arr1 = mm_allocate(1 * sizeof(int)), arr2 = mm_allocateDetailed(1 * sizeof(int), &cowTestAllocator.allocator, DEFAULT_MEMORY_OPERATIONS_TYPE_COW);
    int* mapped = mapping_mmap(NULL, sizeof(int), MAPPING_MMAP_PROT_READ | MAPPING_MMAP_PROT_WRITE, MAPPING_MMAP_FLAGS_ANON | MAPPING_MMAP_FLAGS_TYPE_SHARED, NULL, 0);
    print_printf("%p %p\n", arr1, arr2);
    print_printf("%p\n", mapped);
    arr1[0] = 1, arr2[0] = 114514;
    rootTID = schedule_getCurrentThread()->tid;

    mutex_initStruct(&conditionLock, EMPTY_FLAGS);

    conditionVar_initStruct(&var1);
    conditionVar_initStruct(&var2);

    timer_initStruct(&condTimer, 1, TIME_UNIT_SECOND);
    SET_FLAG_BACK(condTimer.flags, TIMER_FLAGS_SYNCHRONIZE);

    int writeFD, readFD;
    ipc_pipe(&writeFD, &readFD);

    Process* forked = schedule_fork();
    if (forked != NULL) {
        Thread* currentThread = schedule_getCurrentThread();
        print_printf("This is main thread, TID: %u, PID: %u\n", currentThread->tid, currentThread->process->pid);

        timer_start(&condTimer);
        
        mutex_acquire(&conditionLock);
        flag1 = true;
        conditionVar_notify(&var1);
        mutex_release(&conditionLock);

        print_printf("Condition Variable test-1\n");
        
        mutex_acquire(&conditionLock);
        conditionVar_wait(&var2, &conditionLock, __conditionVarFunc2, NULL);
        mutex_release(&conditionLock);

        print_printf("Condition Variable test-3\n");

        Process* process = schedule_getCurrentProcess();
        File* pipeFile = process_getFSentry(process, writeFD);
        process_removeFSentry(process, writeFD);
        fs_fileClose(pipeFile);
    } else {
        Thread* currentThread = schedule_getCurrentThread();
        print_printf("This is child thread, TID: %u, PID: %u\n", currentThread->tid, currentThread->process->pid);
        
        mutex_acquire(&conditionLock);
        conditionVar_wait(&var1, &conditionLock, __conditionVarFunc1, NULL);
        mutex_release(&conditionLock);

        print_printf("Condition Variable test-2\n");

        timer_start(&condTimer);
        
        mutex_acquire(&conditionLock);
        flag2 = true;
        conditionVar_notify(&var2);
        mutex_release(&conditionLock);

        print_printf("Condition Variable test-4\n");

        Process* process = schedule_getCurrentProcess();
        File* pipeFile = process_getFSentry(process, readFD);
        process_removeFSentry(process, readFD);
        fs_fileClose(pipeFile);
    }

    Thread* currentThread = schedule_getCurrentThread();
    if (rootTID == currentThread->tid) {
        arr1[0] = 3;
        semaphore_up(&sema1);

        File* pipeFile = process_getFSentry(schedule_getCurrentProcess(), readFD);
        fs_fileRead(pipeFile, pipeBuffer, 16);
        pipeBuffer[6] = '\0';
        print_printf("Received from pipe: %s\n", pipeBuffer);

        semaphore_down(&sema2);
        print_printf("DONE 0, arr1: %d, arr2: %d, mapped: %d\n", arr1[0], arr2[0], *mapped);
    } else {
        Sigaction sigaction = {
            .handler = __testSigaction
        };
        
        process_sigaction(schedule_getCurrentProcess(), SIGNAL_SIGTRAP, &sigaction, NULL);

        semaphore_down(&sema1);
        arr1[0] = 2, arr2[0] = 1919810;
        *mapped = 114514;
        print_printf("DONE 1, arr1: %d, arr2: %d, mapped: %d\n", arr1[0], arr2[0], *mapped);
        while (true) {
            print_printf("Waiting for input: ");
            int len = teletype_rawRead(tty_getCurrentTTY(), str, INFINITE);
            ERROR_CHECKPOINT();
            str[--len] = '\0';

            if (cstring_strcmp(str, "PASS") == 0 || len == 0) {
                break;
            }
            print_printf("%s-%d\n", str, len);
        }

        File* pipeFile = process_getFSentry(schedule_getCurrentProcess(), writeFD);
        fs_fileWrite(pipeFile, "415411", 6);

        Cstring testArgv[] = {
            "test", "arg1", "arg2", "arg3", NULL
        };
        Cstring testEnvp[] = {
            "ENV1=114514", "ENV2=1919", "ENV3=810", NULL
        };
        int ret = usermode_execute("/bin/test", testArgv, testEnvp);
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

    mm_free(arr1);
    mm_free(arr2);

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
    FS_fileStat stat;
    File* file = fs_fileOpen("/LOGO.txt", FCNTL_OPEN_READ_ONLY);    //TODO: What if file closed before munmap?
    ERROR_CHECKPOINT();
    void* addr = mapping_mmap(NULL, file->vnode->size, MAPPING_MMAP_PROT_READ, MAPPING_MMAP_FLAGS_TYPE_PRIVATE, file, 0);
    print_printf("%s\n", addr);
    mapping_munmap(addr, file->vnode->size);

    fs_fileStat(file, &stat);
    ERROR_CHECKPOINT();

    print_printf("%lu\n", stat.vnodeID);
    print_printf("%lu\n", stat.createTime.second);
    print_printf("%lu\n", stat.accessTime.second);
    print_printf("%lu\n", stat.modifyTime.second);

    BlockDevice* device = file->vnode->fscore->blockDevice;
    fs_fileClose(file);
    ERROR_CHECKPOINT();
    blockDevice_flush(device);
    ERROR_CHECKPOINT();
}

static void printFileFromEXT2() {
    FS_fileStat stat;
    File* file = fs_fileOpen("/mnt/test_directory_a/test_file_b.txt", FCNTL_OPEN_READ_ONLY);    //TODO: What if file closed before munmap?
    ERROR_CHECKPOINT();
    void* addr = mapping_mmap(NULL, file->vnode->size, MAPPING_MMAP_PROT_READ, MAPPING_MMAP_FLAGS_TYPE_PRIVATE, file, 0);
    print_printf("%s\n", addr);
    mapping_munmap(addr, file->vnode->size);

    fs_fileStat(file, &stat);
    ERROR_CHECKPOINT();

    print_printf("%lu\n", stat.vnodeID);
    print_printf("%lu\n", stat.createTime.second);
    print_printf("%lu\n", stat.accessTime.second);
    print_printf("%lu\n", stat.modifyTime.second);

    BlockDevice* device = file->vnode->fscore->blockDevice;
    fs_fileClose(file);
    ERROR_CHECKPOINT();
    blockDevice_flush(device);
    ERROR_CHECKPOINT();
}