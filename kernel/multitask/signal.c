#include<multitask/signal.h>

#include<kit/atomic.h>
#include<kit/types.h>
#include<multitask/process.h>
#include<real/simpleAsmLines.h>
#include<debug.h>

ConstCstring signal_names[] = {
    "UNKNOWN",
    "SIGHUP",
    "SIGINT",
    "SIGQUIT",
    "SIGILL",
    "SIGTRAP",
    "SIGABRT",
    "SIGIOT",
    "SIGBUS",
    "SIGFPE",
    "SIGKILL",
    "SIGUSR1",
    "SIGSEGV",
    "SIGUSR2",
    "SIGPIPE",
    "SIGALRM",
    "SIGTERM",
    "SIGSTKFLT",
    "SIGCHLD",
    "SIGCONT",
    "SIGSTOP",
    "SIGTSTP",
    "SIGTTIN",
    "SIGTTOU",
    "SIGURG",
    "SIGXCPU",
    "SIGXFSZ",
    "SIGVTALRM",
    "SIGPROF",
    "SIGWINCH",
    "SIGIO",
    "SIGPOLL",
    "SIGPWR",
    "SIGSYS",
    "SIGUNUSED"
};

const SignalDefaultHandler signal_defaultHandlers[] = {
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //Never touch this
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGHUP
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGINT
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGQUIT
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGILL
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGTRAP
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGABRT SIGIOT
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGBUS
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGFPE
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGKILL (Should not touch this)
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGUSR1
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGSEGV
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGUSR2
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGPIPE
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGALRM
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGTERM
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGSTKFLT
    SIGNAL_DEFAULT_HANDLER_IGNORE,      //SIGCHLD
    SIGNAL_DEFAULT_HANDLER_IGNORE,      //SIGCONT
    SIGNAL_DEFAULT_HANDLER_STOP,        //SIGSTOP (Should not touch this)
    SIGNAL_DEFAULT_HANDLER_STOP,        //SIGTSTP (Should not touch this)
    SIGNAL_DEFAULT_HANDLER_STOP,        //SIGTTIN
    SIGNAL_DEFAULT_HANDLER_STOP,        //SIGTTOU
    SIGNAL_DEFAULT_HANDLER_IGNORE,      //SIGURG
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGXCPU
    SIGNAL_DEFAULT_HANDLER_DUMP,        //SIGXFSZ
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGVTALRM
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGPROF
    SIGNAL_DEFAULT_HANDLER_IGNORE,      //SIGWINCH
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGIO SIGPOLL
    SIGNAL_DEFAULT_HANDLER_TERMINATE,   //SIGPWR
    SIGNAL_DEFAULT_HANDLER_DUMP         //SIGSYS SIGUNUSED
};

const SignalHandler signalDefaultHandler_funcs[] = {
    signal_defaultHandlerTerminate,
    signal_defaultHandlerDump,
    signal_defaultHandlerIgnore,
    signal_defaultHandlerStop
};

void signal_defaultHandlerTerminate(int signal) {
    process_die(schedule_getCurrentProcess());
}

void signal_defaultHandlerDump(int signal) {
    process_die(schedule_getCurrentProcess());
    //TODO: Dump core here
}

void signal_defaultHandlerIgnore(int signal) {
    return;
}

void signal_defaultHandlerStop(int signal) {
    process_stop(schedule_getCurrentProcess());
}

void signalQueue_initStruct(SignalQueue* queue) {
    *queue = 0;
}

void signalQueue_pend(SignalQueue* queue, int signal) {
    ATOMIC_FETCH_OR(queue, FLAG32(signal));
}

int signalQueue_getPending(SignalQueue* queue) {
    int ret = 0;
    SignalQueue currentQueue = ATOMIC_LOAD(queue), newQueue;
    Flags32 pending;
    do {
        pending = LOWER_BIT(currentQueue);
        newQueue = currentQueue ^ pending;
    } while (!ATOMIC_COMPARE_EXCHANGE_N(queue, &currentQueue, newQueue));

    DEBUG_ASSERT_SILENT(pending != 1);
    if (pending == 0) {
        return 0;
    }

    return bsfl(pending);
}