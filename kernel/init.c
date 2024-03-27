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

static Result __printBootSlogan();

static Result __enableInterrupt();

static __InitFunc _initFuncs[] = {
    { initTerminalSwitch, "Terminal"    },
    { __printBootSlogan , NULL          },  //TODO: May crash after print slogan
    { initIDT           , "Interrupt"   },
    { initMemoryManager , "Memory"      },
    { initTSS           , "TSS"         },
    { initKeyboard      , "Keyboard"    },
    { device_init       , "Device"      },
    { initTime          , "Time"        },
    { initSchedule      , "Schedule"    },
    { __enableInterrupt , NULL          },
    { initATAdevices    , "ATA Devices" },
    { fs_init           , "File System" },
    { initUsermode      , "User Mode"   },
    { NULL, NULL }
};

Result initKernel() {
    for (int i = 0; _initFuncs[i].func != NULL; ++i) {
        if (_initFuncs[i].func() == RESULT_SUCCESS) {
            continue;
        }

        if (_initFuncs[i].name != NULL) {
            printf(TERMINAL_LEVEL_DEBUG, "Initialization of %s failed\n", _initFuncs[i].name);
        }

        return RESULT_FAIL;
    }

    printf(TERMINAL_LEVEL_DEBUG, "All Initializations passed\n");
    return RESULT_SUCCESS;
}

static Result __printBootSlogan() {
    printf(TERMINAL_LEVEL_OUTPUT, "EGOS starts booting...\n");  //FACE THE SELF, MAKE THE EGOS
    return RESULT_SUCCESS;
}

static Result __enableInterrupt() {
    sti();
    return RESULT_SUCCESS;
}