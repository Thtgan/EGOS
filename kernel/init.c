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
#include<memory/mm.h>
#include<multitask/schedule.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<realmode.h>
#include<time/time.h>
#include<usermode/usermode.h>
#include<error.h>

typedef struct {
    void (*func)();
    ConstCstring name;
} __InitFunc;

static void __init_printBootSlogan();

static void __init_enableInterrupt();   //TODO: Maybe remove these

static void __init_disableInterrupt();  //TODO: Maybe remove these

static void __init_initVideo();         //TODO: Maybe remove these

static __InitFunc _initFuncs[] = {
    { error_init                ,   "Error"         },
    { display_init              ,   "Display"       },
    { tty_init                  ,   "Boot TTY"      },
    { __init_printBootSlogan    ,   NULL            },  //TODO: May crash after print slogan
    { idt_init                  ,   "Interrupt"     },
    { mm_init                   ,   "Memory"        },
    { tty_initVirtTTY           ,   "Virtual TTY"   },
    { debug_init                ,   "Debug"         },
    { tss_init                  ,   "TSS"           },
    { keyboard_init             ,   "Keyboard"      },
    { device_init               ,   "Device"        },
    { pci_init                  ,   "PCI bus"       },
    { __init_enableInterrupt    ,   NULL            },
    { ata_initDevices           ,   "ATA Devices"   },
    { fs_init                   ,   "File System"   },
    { __init_disableInterrupt   ,   NULL            },
    { time_init                 ,   "Time"          },
    { schedule_init             ,   "Schedule"      },
    { realmode_init             ,   "Realmode"      },
    { usermode_init             ,   "User Mode"     },
    { __init_enableInterrupt    ,   NULL            },
    { __init_initVideo          ,   "Video"         },
    { NULL, NULL }
};

void init_initKernel() {
    for (int i = 0; _initFuncs[i].func != NULL; ++i) {
        _initFuncs[i].func();

        ERROR_CHECKPOINT({
            if (_initFuncs[i].name != NULL) {
                print_printf("Initialization of %s failed\n", _initFuncs[i].name);
            } else {
                print_printf("Initialization failed");
            }
            error_unhandledRecord(error_getCurrentRecord());
        });
    }

    print_printf("All Initializations passed\n");
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