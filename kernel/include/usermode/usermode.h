#if !defined(__USERMODE_USERMODE_H)
#define __USERMODE_USERMODE_H

#include<kit/config.h>
#include<kit/types.h>
#include<test.h>

void usermode_init();

/**
 * @brief Execute ELF program at certain path, sets errorcode to indicate error
 * 
 * @param path Path to ELF program file, starts from root directory
 * @return int return value of user program, -1 if error happens
 */
int usermode_execute(ConstCstring path, Cstring* argv, Cstring* envp);

extern void* usermode_executeReturn;

#if defined(CONFIG_UNIT_TEST_USERMODE)
TEST_EXPOSE_GROUP(usermode_testGroup);
#define UNIT_TEST_GROUP_USERMODE    &usermode_testGroup
#else
#define UNIT_TEST_GROUP_USERMODE    NULL
#endif

#endif // __USERMODE_USERMODE_H
