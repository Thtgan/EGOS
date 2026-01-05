#include<init.h>

#include<devices/ata/ata.h>
#include<devices/bus/pci.h>
#include<devices/device.h>
#include<devices/display/display.h>
#include<devices/keyboard/keyboard.h>
#include<devices/terminal/tty.h>
#include<fs/fs.h>
#include<interrupt/IDT.h>
#include<interrupt/TSS.h>
#include<kit/types.h>
#include<memory/memory.h>
#include<memory/mm.h>
#include<multitask/schedule.h>
#include<real/simpleAsmLines.h>
#include<realmode.h>
#include<time/time.h>
#include<usermode/usermode.h>
#include<debug.h>
#include<error.h>
#include<test.h>
#include<uart.h>

typedef struct {
    void (*func)();
    ConstCstring name;
    TestGroup* testGroup;
} __Init_Task;

static void __init_printBootSlogan();

static void __init_enableInterrupt();   //TODO: Maybe remove these

static void __init_disableInterrupt();  //TODO: Maybe remove these

static void __init_initVideo();         //TODO: Maybe remove these

static void __init_dummy();

static __Init_Task _initFuncs1[] = {
    { debug_init                ,   "Debug"         , NULL  },
    { uart_init                 ,   "UART"          , UNIT_TEST_GROUP_UART  },
    { error_init                ,   "Error"         , NULL  },
    { display_init              ,   "Display"       , NULL  },
    { tty_init                  ,   "Boot TTY"      , NULL  },
    { __init_printBootSlogan    ,   NULL            , NULL  },
    { idt_init                  ,   "Interrupt"     , NULL  },
    { mm_init                   ,   "Memory Manager", UNIT_TEST_GROUP_MM    },
    { NULL, NULL, NULL }
};

static __Init_Task _initFuncs2[] = {
    { tty_initVirtTTY           ,   "Virtual TTY"   , NULL  },
    { tss_init                  ,   "TSS"           , NULL  },
    { keyboard_init             ,   "Keyboard"      , NULL  },
    { device_init               ,   "Device"        , NULL  },
    { pci_init                  ,   "PCI bus"       , NULL  },
    { __init_enableInterrupt    ,   NULL            , NULL  },
    { ata_initDevices           ,   "ATA Devices"   , NULL  },
    { fs_init                   ,   "File System"   , NULL  },
    { schedule_init             ,   "Schedule"      , NULL  },
    { time_init                 ,   "Time"          , UNIT_TEST_GROUP_SCHEDULE  },  //TODO: Timer relies on schedule, decouple it in the future
    { __init_dummy              ,   NULL            , UNIT_TEST_GROUP_TIME      },
    { realmode_init             ,   "Realmode"      , NULL  },
    { usermode_init             ,   "User Mode"     , UNIT_TEST_GROUP_USERMODE  },
    { __init_initVideo          ,   "Video"         , NULL  },
    { NULL, NULL, NULL }
};

__attribute__((aligned(PAGE_SIZE)))
static char _init_bootStack[INIT_BOOT_STACK_SIZE];

void* init_getBootStackBottom() {
    return _init_bootStack;
}

void init_initKernelStage1() {
    for (int i = 0; _initFuncs1[i].func != NULL; ++i) {
        _initFuncs1[i].func();

        ERROR_CHECKPOINT({
            if (_initFuncs1[i].name != NULL) {
                debug_printf("Initialization of %s failed\n", _initFuncs1[i].name);
            } else {
                debug_printf("Initialization failed");
            }
            error_unhandledRecord(error_getCurrentRecord());
        });

        if (_initFuncs1[i].testGroup == NULL) {
            continue;
        }

        bool testResult = test_exec(_initFuncs1[i].testGroup);

        if (!testResult) {
            debug_blowup("Unit test failed\n");
        }
    }
    return;
}

void init_initKernelStage2() {
    for (int i = 0; _initFuncs2[i].func != NULL; ++i) {
        _initFuncs2[i].func();

        ERROR_CHECKPOINT({
            if (_initFuncs2[i].name != NULL) {
                debug_printf("Initialization of %s failed\n", _initFuncs2[i].name);
            } else {
                debug_printf("Initialization failed");
            }
            error_unhandledRecord(error_getCurrentRecord());
        });

        if (_initFuncs2[i].testGroup == NULL) {
            continue;
        }

        bool testResult = test_exec(_initFuncs2[i].testGroup);

        if (!testResult) {
            debug_blowup("Unit test failed\n");
        }
    }

    debug_printf("All Initializations passed\n");
    return;
}

static void __init_printBootSlogan() {
    print_printf("EGOS starts booting...\n");  //FACE THE SELF, MAKE THE EGOS
}

static void __init_enableInterrupt() {
    sti();
}

static void __init_disableInterrupt() {
    cli();
}

static void __init_initVideo() {
    display_initMode(DISPLAY_MODE_VGA);
}

static void __init_dummy() {
    asm volatile("nop");
}