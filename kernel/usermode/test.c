#include<kit/config.h>

#if defined(CONFIG_UNIT_TEST_USERMODE)

#include<test.h>
#include<usermode/usermode.h>

static bool __usermode_test_exec(void* arg) {

    Cstring testArgv[] = {
        "test", "114514", "1919", "810", NULL
    };
    Cstring testEnvp[] = {
        "ENV1=3", "ENV2=5", "ENV3=7", NULL
    };
    int ret = usermode_execute("/bin/test", testArgv, testEnvp);

    return ret == (114514 + 1919 + 810) * 5;
}

TEST_SETUP_LIST(    //TODO: Add test for clocks
    USERMODE,
    (1, __usermode_test_exec)
);

TEST_SETUP_GROUP(usermode_testGroup, EMPTY_FLAGS, NULL, USERMODE, NULL);

#endif
