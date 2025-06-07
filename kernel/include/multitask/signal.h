#if !defined(__MULTITASK_SIGNAL_H)
#define __MULTITASK_SIGNAL_H

typedef struct Sigaction Sigaction;
typedef union Sigval Sigval;
typedef struct Siginfo Siginfo;
typedef enum SignalDefaultHandler SignalDefaultHandler;

#include<kit/types.h>

typedef Uint32 SignalQueue;

#define SIGNAL_SIGHUP     1     /* Terminate   Hang up controlling terminal or process                          */
#define SIGNAL_SIGINT     2     /* Terminate   Interrupt from keyboard, Control-C                               */
#define SIGNAL_SIGQUIT    3     /* Dump        Quit from keyboard, Control-\                                    */
#define SIGNAL_SIGILL     4     /* Dump        Illegal instruction                                              */
#define SIGNAL_SIGTRAP    5     /* Dump        Breakpoint for debugging                                         */
#define SIGNAL_SIGABRT    6     /* Dump        Abnormal termination                                             */
#define SIGNAL_SIGIOT     6     /* Dump        Equivalent to SIGABRT                                            */
#define SIGNAL_SIGBUS     7     /* Dump        Bus error                                                        */
#define SIGNAL_SIGFPE     8     /* Dump        Floating-point exception                                         */
#define SIGNAL_SIGKILL    9     /* Terminate   Forced-process termination                                       */
#define SIGNAL_SIGUSR1    10    /* Terminate   Available to processes                                           */
#define SIGNAL_SIGSEGV    11    /* Dump        Invalid memory reference                                         */
#define SIGNAL_SIGUSR2    12    /* Terminate   Available to processes                                           */
#define SIGNAL_SIGPIPE    13    /* Terminate   Write to pipe with no readers                                    */
#define SIGNAL_SIGALRM    14    /* Terminate   Real-timer clock                                                 */
#define SIGNAL_SIGTERM    15    /* Terminate   Process termination                                              */
#define SIGNAL_SIGSTKFLT  16    /* Terminate   Coprocessor stack error                                          */
#define SIGNAL_SIGCHLD    17    /* Ignore      Child process stopped or terminated or got a signal if traced    */
#define SIGNAL_SIGCONT    18    /* Continue    Resume execution, if stopped                                     */
#define SIGNAL_SIGSTOP    19    /* Stop        Stop process execution, Ctrl-Z                                   */
#define SIGNAL_SIGTSTP    20    /* Stop        Stop process issued from tty                                     */
#define SIGNAL_SIGTTIN    21    /* Stop        Background process requires input    */
#define SIGNAL_SIGTTOU    22    /* Stop        Background process requires output   */
#define SIGNAL_SIGURG     23    /* Ignore      Urgent condition on socket           */
#define SIGNAL_SIGXCPU    24    /* Dump        CPU time limit exceeded              */
#define SIGNAL_SIGXFSZ    25    /* Dump        File size limit exceeded             */
#define SIGNAL_SIGVTALRM  26    /* Terminate   Virtual timer clock                  */
#define SIGNAL_SIGPROF    27    /* Terminate   Profile timer clock                  */
#define SIGNAL_SIGWINCH   28    /* Ignore      Window resizing                      */
#define SIGNAL_SIGIO      29    /* Terminate   I/O now possible                     */
#define SIGNAL_SIGPOLL    29    /* Terminate   Equivalent to SIGIO                  */
#define SIGNAL_SIGPWR     30    /* Terminate   Power supply failure                 */
#define SIGNAL_SIGSYS     31    /* Dump        Bad system call                      */
#define SIGNAL_SIGUNUSED  31    /* Dump        Equivalent to SIGSYS                 */

typedef void (*SignalHandler)(int signal);
typedef void (*SignalAction)(int signal, Siginfo* info, void* ucontext);    //TODO: ucontext not used yet

typedef struct Sigaction {
    union {
        SignalHandler handler;
#define SIGACTION_HANDLER_DFL   (SignalHandler)0
#define SIGACTION_HANDLER_IGN   (SignalHandler)1
        SignalAction sigaction;
    };
    Flags32 mask;
    Flags32 flags;  //TODO: Flags not implemented
} Sigaction;

typedef union Sigval {
	int sival_int;
	void* sival_ptr;
} Sigval;

typedef struct Siginfo {
    int             signo;          /* Signal number */
    int             errno;          /* An errno value */
    int             code;           /* Signal code */
    int             trapno;         /* Trap number that caused hardware-generated signal (unused on most architectures) */
    Uint16          pid;            /* Sending process ID */
    Uint16          uid;            /* Real user ID of sending process */
    int             status;         /* Exit value or signal */
    long            utime;          /* User time consumed */
    long            stime;          /* System time consumed */
    Sigval          si_value;       /* Signal value */
    int             si_int;         /* POSIX.1b signal */
    void*           si_ptr;         /* POSIX.1b signal */
    int             si_overrun;     /* Timer overrun count; POSIX.1b timers */
    int             si_timerid;     /* Timer ID; POSIX.1b timers */
    void*           si_addr;        /* Memory location which caused fault */
    long            si_band;        /* Band event (was int in glibc 2.3.2 and earlier) */
    int             si_fd;          /* File descriptor */
    short           si_addr_lsb;    /* Least significant bit of address (since Linux 2.6.32) */
    void*           si_lower;       /* Lower bound when address violation occurred (since Linux 3.19) */
    void*           si_upper;       /* Upper bound when address violation occurred (since Linux 3.19) */
    int             si_pkey;        /* Protection key on PTE that caused fault (since Linux 4.6) */
    void*           si_call_addr;   /* Address of system call instruction (since Linux 3.5) */
    int             si_syscall;     /* Number of attempted system call (since Linux 3.5) */
    unsigned int    si_arch;        /* Architecture of attempted system call (since Linux 3.5) */
} Siginfo;

void signal_defaultHandlerTerminate(int signal);

void signal_defaultHandlerDump(int signal);

void signal_defaultHandlerIgnore(int signal);

void signal_defaultHandlerStop(int signal);

typedef enum SignalDefaultHandler {
    SIGNAL_DEFAULT_HANDLER_TERMINATE,
    SIGNAL_DEFAULT_HANDLER_DUMP,
    SIGNAL_DEFAULT_HANDLER_IGNORE,
    SIGNAL_DEFAULT_HANDLER_STOP,
} SignalDefaultHandler;

extern const SignalHandler signalDefaultHandler_funcs[];

extern ConstCstring signal_names[];

extern const SignalDefaultHandler signal_defaultHandlers[];

void signalQueue_initStruct(SignalQueue* queue);

void signalQueue_pend(SignalQueue* queue, int signal);

int signalQueue_getPending(SignalQueue* queue);

#endif // __MULTITASK_SIGNAL_H
