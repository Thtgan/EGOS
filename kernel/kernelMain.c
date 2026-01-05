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
char str[128];

static void printLOGO();

static void printFileFromEXT2();

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

#include<memory/allocators/slabHeapAllocator.h>

#include<memory/mapping.h>

#include<uart.h>

static void __timerFunc1(Timer* timer) {
    print_printf("HANDLER CALL FROM TIMER1\n");
}

static void __timerFunc2(Timer* timer) {
    print_printf("HANDLER CALL FROM TIMER2\n");
    if (--timer->data == 0) {
        CLEAR_FLAG_BACK(timer->flags, TIMER_FLAGS_REPEAT);
    }
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

    {
        File* file = fs_fileOpen("/dev/null", FCNTL_OPEN_READ_WRITE);
        ERROR_CHECKPOINT();
        fs_fileWrite(file, "1145141919810", 14);
        ERROR_CHECKPOINT();
        fs_fileClose(file);
    }

    print_printf("Closing FS\n");

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