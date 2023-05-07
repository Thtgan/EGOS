#if !defined(__SCHEDULE_H)
#define __SCHEDULE_H

#include<kit/types.h>
#include<multitask/process.h>

/**
 * @brief Schedule the process
 */
void schedule(ProcessStatus newStatus);

/**
 * @brief Initialize the schedule
 * 
 * @return Result Result of the operation
 */
Result initSchedule();

/**
 * @brief Change the status of process, and add it to the end of corresponding queue
 * 
 * @param process Process
 * @param status Process status
 */
void setProcessStatus(Process* process, ProcessStatus status);

/**
 * @brief Get head process of status queue
 * 
 * @param status Process status
 * @return Process* Process at the head of status queue, NULL if head is not exist
 */
Process* getStatusQueueHead(ProcessStatus status);

/**
 * @brief Function reserved for idle process
 */
void idle();

#endif // __SCHEDULE_H
