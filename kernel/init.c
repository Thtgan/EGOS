#include<init.h>

#include<devices/ata/ata.h>
#include<devices/device.h>
#include<devices/keyboard/keyboard.h>
#include<devices/terminal/terminalSwitch.h>
#include<fs/fs.h>
#include<interrupt/IDT.h>
#include<interrupt/TSS.h>
#include<kit/types.h>
#include<memory/mm.h>
#include<multitask/schedule.h>
#include<print.h>
#include<real/simpleAsmLines.h>
#include<time/time.h>
#include<usermode/usermode.h>

typedef struct {
    Result (*func)();
    ConstCstring name;
} __InitFunc;

static Result __init_printBootSlogan();

static Result __init_enableInterrupt();

static __InitFunc _initFuncs[] = {
    { terminalSwitch_init       ,   "Terminal"    },
    { __init_printBootSlogan    ,   NULL          },  //TODO: May crash after print slogan
    { idt_init                  ,   "Interrupt"   },
    { mm_init                   ,   "Memory"      },
    { tss_init                  ,   "TSS"         },
    { keyboard_init             ,   "Keyboard"    },
    { device_init               ,   "Device"      },
    { time_init                 ,   "Time"        },
    { schedule_init             ,   "Schedule"    },
    { __init_enableInterrupt    ,   NULL          },
    { ata_initdevices           ,   "ATA Devices" },
    { fs_init                   ,   "File System" },
    { usermode_init             ,   "User Mode"   },
    { NULL, NULL }
};

Result init_initKernel() {
    for (int i = 0; _initFuncs[i].func != NULL; ++i) {
        if (_initFuncs[i].func() == RESULT_SUCCESS) {
            continue;
        }

        if (_initFuncs[i].name != NULL) {
            print_printf(TERMINAL_LEVEL_DEBUG, "Initialization of %s failed\n", _initFuncs[i].name);
        }

        return RESULT_FAIL;
    }

    print_printf(TERMINAL_LEVEL_DEBUG, "All Initializations passed\n");
    return RESULT_SUCCESS;
}

static Result __init_printBootSlogan() {
    print_printf(TERMINAL_LEVEL_OUTPUT, "EGOS starts booting...\n");  //FACE THE SELF, MAKE THE EGOS
    return RESULT_SUCCESS;
}

static Result __init_enableInterrupt() {
    sti();
    return RESULT_SUCCESS;
}