#if !defined(__DIRECT_ACCESS_H)
#define __DIRECT_ACCESS_H

/**
 * @brief Do initialization for direct memory access
 */
void initDirectAccess();

/**
 * @brief Enable direct memory access without paging by using a fake paging table
 */
void enableDirectAccess();

/**
 * @brief Go back to normal paging
 */
void disableDirectAccess();

#endif // __DIRECT_ACCESS_H
