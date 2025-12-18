#if !defined(__LIB_TEST_H)
#define __LIB_TEST_H

typedef struct TestGroup TestGroup;
typedef struct TestList TestList;

#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>

typedef void* (*TestGroupPrepare)();

typedef bool (*TestUnitFunc)(void* ctx);

typedef void* (*TestGroupClear)(void* ctx);

typedef struct TestList {
    Uint8 num;
    ConstCstring name;
    Flags64 isFuncBitmap;
    union {
        TestUnitFunc func;
        TestList* subList;
    } entries[0];
} TestEntry;

typedef struct TestGroup {
    TestGroupPrepare prepare;
    TestList* rootList;
    TestGroupClear clear;
} TestGroup;

bool test_exec(TestGroup* group);

bool test_ask(ConstCstring question);

#define TEST_SETUP_LIST(__LIST_NAME, ...)                                                               \
static struct {                                                                                         \
    TestList list;                                                                                      \
    void* entries[MACRO_COUNT(__VA_ARGS__)];                                                            \
} MACRO_CONCENTRATE3(__, __LIST_NAME, _testList) = {                                                    \
    .list = {                                                                                           \
        .num = MACRO_COUNT(__VA_ARGS__),                                                                \
        .name = MACRO_STR(__LIST_NAME),                                                                 \
        .isFuncBitmap = MACRO_FOREACH_CALL_WITH_INDEX(__TEST_SETUP_BUILD_FLAG, |, __VA_ARGS__)          \
    },                                                                                                  \
    .entries = {                                                                                        \
        MACRO_FOREACH_CALL(__TEST_SETUP_BUILD_LIST,, __VA_ARGS__)                                       \
    }                                                                                                   \
}

#define __TEST_SETUP_BUILD_FLAG(__INDEX, __ENTRY)           ((__TEST_SETUP_BUILD_FLAG_HELPER __ENTRY) * FLAG64(__INDEX))
#define __TEST_SETUP_BUILD_FLAG_HELPER(__IS_FUNC, __DUMMY)  __IS_FUNC

#define __TEST_SETUP_BUILD_LIST(__ENTRY)                    __TEST_SETUP_BUILD_LIST_HELPER __ENTRY
#define __TEST_SETUP_BUILD_LIST_HELPER(__IS_FUNC, __ENTRY)  __ENTRY,

#define TEST_SETUP_GROUP(__GROUP, __PREPARE, __ROOT_LIST_NAME, __CLEAR)     \
struct TestGroup __GROUP = {                                                \
    .prepare = (__PREPARE),                                                 \
    .rootList = &MACRO_CONCENTRATE3(__, __ROOT_LIST_NAME, _testList).list,  \
    .clear = (__CLEAR)                                                      \
}

#define TEST_EXPOSE_GROUP(__GROUP)  extern TestGroup __GROUP

#endif // __LIB_TEST_H
