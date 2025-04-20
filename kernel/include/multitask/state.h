#if !defined(__MULTITASK_STATE_H)
#define __MULTITASK_STATE_H

typedef enum State {
    STATE_RUNNING,
    STATE_SLEEP,    //TODO: Make interruptable and uninterruptable when adding signal
    STATE_STOPPED,
    STATE_ZOMBIE
} State;

#endif // __MULTITASK_STATE_H
