#if !defined(__BLOWUP_H)
#define __BLOWUP_H

#include<kit/types.h>

__attribute__((noreturn))
/**
 * @brief Blow up the system, print info before that
 * @param format: Format of info
 * @param ...: Values printed by func
 */
void blowup(ConstCstring format, ...);

#endif // __BLOWUP_H
