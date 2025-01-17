#include<init.h>

#include<devices/ata/ata.h>
#include<devices/bus/pci.h>
#include<devices/device.h>
#include<devices/display/display.h>
#include<devices/keyboard/keyboard.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/fs.h>
#include<fs/fsSyscall.h>
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
#include<result.h>

typedef struct {
    Result* (*func)();
    ConstCstring name;
} __InitFunc;

static Result* __init_printBootSlogan();

static Result* __init_enableInterrupt();    //TODO: Maybe remove these

static Result* __init_disableInterrupt();   //TODO: Maybe remove these

static Result* __init_initVideo();          //TODO: Maybe remove these

static __InitFunc _initFuncs[] = {
    { result_init               ,   "Result"      },
    { display_init              ,   "Display"     },
    { terminalSwitch_init       ,   "Terminal"    },
    { __init_printBootSlogan    ,   NULL          },  //TODO: May crash after print slogan
    { idt_init                  ,   "Interrupt"   },
    { mm_init                   ,   "Memory"      },
    { tss_init                  ,   "TSS"         },
    { keyboard_init             ,   "Keyboard"    },
    { device_init               ,   "Device"      },
    { pci_init                  ,   "PCI bus"     },
    { __init_enableInterrupt    ,   NULL          },
    { ata_initDevices           ,   "ATA Devices" },
    { fs_init                   ,   "File System" },
    { usermode_init             ,   "User Mode"   },
    { fsSyscall_init            ,   "FS syscall"  },
    { __init_disableInterrupt   ,   NULL          },
    { time_init                 ,   "Time"        },
    { schedule_init             ,   "Schedule"    },
    { realmode_init             ,   "Realmode"    },
    { __init_enableInterrupt    ,   NULL          },
    { __init_initVideo          ,   "Video"       },
    { NULL, NULL }
};

Result* init_initKernel() {
    for (int i = 0; _initFuncs[i].func != NULL; ++i) {
        ERROR_TRY_CATCH_DIRECT(
            _initFuncs[i].func(), 
            {
                if (_initFuncs[i].name != NULL) {
                    print_printf(TERMINAL_LEVEL_DEBUG, "Initialization of %s failed\n", _initFuncs[i].name);
                }
                return ERROR_CATCH_RESULT_NAME;
            }
        );
    }

    print_printf(TERMINAL_LEVEL_DEBUG, "All Initializations passed\n");
    ERROR_RETURN_OK();
}

static Result* __init_printBootSlogan() {
    print_printf(TERMINAL_LEVEL_OUTPUT, "EGOS starts booting...\n");  //FACE THE SELF, MAKE THE EGOS
    ERROR_RETURN_OK();
}

static Result* __init_enableInterrupt() {
    sti();
    ERROR_RETURN_OK();
}

static Result* __init_disableInterrupt() {
    cli();
    ERROR_RETURN_OK();
}

static Result* __init_initVideo() {
    return display_initMode(DISPLAY_MODE_VGA);
}