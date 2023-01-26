#if !defined(__SCHEDULE_H)
#define __SCHEDULE_H

#include<multitask/process.h>

/**
 * @brief Schedule the process
 */
void schedule(ProcessStatus newStatus);

/**
 * @brief Initialize the schedule
 */
void initSchedule();

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

#endif // __SCHEDULE_H
