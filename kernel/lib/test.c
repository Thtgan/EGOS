#include<test.h>

#include<kit/bit.h>
#include<kit/macro.h>
#include<kit/types.h>
#include<debug.h>

static bool __test_doExecList(TestList* list, void* ctx, int level);

bool test_exec(TestGroup* group) {
    void* ctx = NULL;

    if (group->prepare != NULL) {
        ctx = group->prepare();
    }

    bool ret = __test_doExecList(group->rootList, ctx, 0);
    
    if (group->clear != NULL) {
        group->clear(ctx);
    }

    return ret;
}

bool test_ask(ConstCstring question) {
    debug_printf(question);
    debug_printf("[y]/n:");

    char ch = '\0', tmp = '\0';
    Size len = 0;
    while ((tmp = debug_getchar()) != '\n') {
        ch = tmp;
        ++len;
    }

    return (ch == '\0') || (ch == 'y' && len == 1);
}

static bool __test_doExecList(TestList* list, void* ctx, int level) {
    bool ret = true;

    debug_printf("Level %d-Executing test list %s:\n", level, list->name);
    
    for (int i = 0; ret && i < list->num; ++i) {
        if (TEST_FLAGS(list->isFuncBitmap, FLAG64(i))) {
            ret = list->entries[i].func(ctx);
        } else {
            ret = __test_doExecList(list->entries[i].subList, ctx, level + 1);
        }
    }

    if (ret) {
        debug_printf("SUCCESS\n");
    } else {
        debug_printf("FAILED\n");
    }

    return ret;
}